#!perl
# tvpgl.* generator



$gpl = <<EOF;

	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000-2009 W.Dee <dee\@kikyou.info> and contributors

	See details of license at "license.txt"

EOF

$rev = "0.1";

;#-----------------------------------------------------------------
;# file content retrieving
;#-----------------------------------------------------------------
sub get_file_content
{
	local($name, @all);
	$name = $_[0];
	open IFH, $name;
	@all = <IFH>;
	return join('', @all);
}

;#-----------------------------------------------------------------
;# exported function gathering
;#-----------------------------------------------------------------


sub format_function
{
	local($a, $b);
	$a = $_[0];
	$a =~ s/\n//g;
	push (@function_list, $a);
}

sub get_function_list
{
	@function_list = ();
	local($content, $ret);
	$content = &get_file_content($_[0]);
	$ret = '';
	$content =~ s/\/\*export\*\/(.*?)\{/&format_function($1)/seg;
}

;#-----------------------------------------------------------------
;# loop unrolling
;#-----------------------------------------------------------------

sub loop_unroll_c
{
	local($content, $times, $unroll_times, $i);
	$content = $_[0];
	$times = $_[1];
	$unroll_times = $_[2];

	print FC <<EOF;
  if($times > 0)
  {
	int lu_n = ($times + ($unroll_times-1)) / $unroll_times;
	switch($times % $unroll_times)
	{
	case 0: do { $content;
EOF
	for($i = $unroll_times-1; $i>=1; $i--)
	{
		print FC <<EOF;
	case $i: $content;
EOF
	}
	print FC <<EOF;
	   } while(-- lu_n);
	}
  }
EOF
}

;# type 2 for more simple case
;# backward operation
sub loop_unroll_c_2_backward
{
	local($content, $times, $unroll_times, $i, $con);
	$content = $_[0];
	$times = $_[1];
	$unroll_times = $_[2];

	print FC <<EOF;
	$times --;

	while($times >= ($unroll_times -1))
	{
EOF

	for($i = 0; $i<$unroll_times; $i++)
	{
		$con = $content;
		$con =~ s/\{ofs\}/($times-$i)/g;
		print FC $con;
	}

  	print FC <<EOF;
		$times -= $unroll_times;
	}

	while($times >= 0)
	{
EOF

	$con = $content;
	$con =~ s/\{ofs\}/$times/g;
	print FC $con;

  	print FC <<EOF;
		$times --;
	}
EOF
}

;# type 2 forward
sub loop_unroll_c_2
{
	local($content, $times, $unroll_times, $i, $con);
	$content = $_[0];
	$times = $_[1];
	$unroll_times = $_[2];

	print FC <<EOF;
	{
		int ___index = 0;
		$times -= ($unroll_times-1);

		while(___index < $times)
		{
EOF

	for($i = 0; $i<$unroll_times; $i++)
	{
		$con = $content;
		$con =~ s/\{ofs\}/(___index+$i)/g;
		print FC $con;
	}

  	print FC <<EOF;
			___index += $unroll_times;
		}

		$times += ($unroll_times-1);

		while(___index < $times)
		{
EOF

		$con = $content;
		$con =~ s/\{ofs\}/___index/g;
		print FC $con;

  		print FC <<EOF;
			___index ++;
		}
	}
EOF
}


;# twice-interleaved type of type 2
sub loop_unroll_c_int_2
{
	local($content, $times, $content2, $int_times, $unroll_times, $i, $con, $con2, @cont, @cont2);
	$content = $_[0];
	$content2 = $_[1];
	$times = $_[2];
	$unroll_times = $_[3];

	print FC <<EOF;
	{
		int ___index = 0;
		$times -= ($unroll_times-1);

		while(___index < $times)
		{
EOF

	for($i = 0; $i<$unroll_times/2; $i++)
	{
		local($j);
		$con = $content;
		$con =~ s/\{ofs\}/(___index+($i*2))/g;
		$con2 = $content2;
		$con2 =~ s/\{ofs\}/(___index+($i*2+1))/g;
		$con =~ s/\n//g;
		$con2 =~ s/\n//g;
		@cont = split(/;;/, $con);
		@cont2 = split(/;;/, $con2);

		for($j = 0; $j <= $#cont; $j++)
		{
			print FC $cont[$j];
			print FC ";\n";
			print FC $cont2[$j];
			print FC ";\n";
		}
	}

  	print FC <<EOF;
			___index += $unroll_times;
		}

		$times += ($unroll_times-1);

		while(___index < $times)
		{
EOF

	$con = $content;
	$con =~ s/\{ofs\}/___index/g;
	print FC $con;

  	print FC <<EOF;
			___index ++;
		}
	}
EOF
}



$serial = 0;

sub _label_expand
{
	local($num);
	$num = $_[0];
	return "ll_". ($num + $serial);
}

sub label_expand
{
	local($content);
	$content = $_[0];
	$serial += ($content =~ s/\{(\d+)\}/&_label_expand($1)/eg);
	return $content;
}

sub _adds_expand
{
	local($reg, $num, $mul);
	$reg = $_[0];
	$num = $_[1];
	$mul = $_[2];
	$num *= $mul;
	if($num == -1)
	{
		return "dec $reg";
	}
	elsif($num == 1)
	{
		return "inc $reg";
	}
	else
	{
		return "add $reg, $num";
	}
}

sub adds_expand
{
	local($content, $mul);
	$content = $_[0];
	$mul = $_[1];
	$content =~ s/\{(.+?),(\d+)\}/&_adds_expand($1, $2, $mul)/eg;
	return $content;
}



;#-----------------------------------------------------------------
;# write the header
;#-----------------------------------------------------------------


open FC , ">tvpgl.c";
open FH , ">tvpgl.h";



;#-----------------------------------------------------------------
print FC <<EOF;
/*
$gpl
*/

/* core C routines for graphics operations */
/* this file is always generated by gengl.pl rev. $rev */

/* #include "tjsCommHead.h" */
#include <memory.h>
#include <math.h>
#include "tjsTypes.h"
#include "tvpgl.h"

#if __BORLANDC__
	#pragma warn -par /* prevent unsed parameter warning */
	#pragma warn -aus /* prevent unsed variable warning */
#endif

EOF
;#-----------------------------------------------------------------
print FH <<EOF;
/*
$gpl
*/
/* core C routines for graphics operations */
/* this file is always generated by gengl.pl rev. $rev */
#ifndef _TVPGL_H_
#define _TVPGL_H_

/*
	key to blending suffix:
	d : destination has alpha
	a : destination has additive-alpha
	o : blend with opacity
*/


/*[*/
#ifdef __cplusplus
 extern "C" {
#endif
/*]*/

/*[*/
#pragma pack(push, 4)
typedef struct
{
	/* structure used for adjustment of gamma levels */

	float RGamma; /* R gamma   ( 0.10 -- 1.00 -- 9.99) */
	tjs_int RFloor;   /* output floor value  ( 0 -- 255 ) */
	tjs_int RCeil;    /* output ceil value ( 0 -- 255 ) */
	float GGamma; /* G */
	tjs_int GFloor;
	tjs_int GCeil;
	float BGamma; /* B */
	tjs_int BFloor;
	tjs_int BCeil;
} tTVPGLGammaAdjustData;
#pragma pack(pop)
/*]*/

#ifdef _WIN32
#define TVP_GL_FUNC_DECL(rettype, funcname, arg)  rettype __cdecl funcname arg
#define TVP_GL_FUNC_EXTERN_DECL(rettype, funcname, arg)  extern rettype __cdecl funcname arg
#define TVP_GL_FUNC_PTR_DECL(rettype, funcname, arg) rettype (__cdecl * funcname) arg
#define TVP_GL_FUNC_PTR_EXTERN_DECL_(rettype, funcname, arg) extern rettype (__cdecl * funcname) arg
#define TVP_GL_FUNC_PTR_EXTERN_DECL TVP_GL_FUNC_PTR_EXTERN_DECL_
#endif

extern unsigned char TVPDivTable[256*256];
extern unsigned char TVP252DitherPalette[3][256];

#define TVP_TLG6_H_BLOCK_SIZE 8
#define TVP_TLG6_W_BLOCK_SIZE 8

/* put platform dependent declaration here */

EOF
;#-----------------------------------------------------------------


;#-----------------------------------------------------------------
;# tables used by various routines
;#-----------------------------------------------------------------

print FC &get_file_content('maketab.c');


;#-----------------------------------------------------------------
;# common functions
;#-----------------------------------------------------------------

;# some common inline functions.
;# mainly for additive-alpha blending mode, by historical reason.

$cnt =  <<EOF;

/* add here compiler specific inline directives */
#if defined( __BORLANDC__ ) || ( _MSC_VER )
	#define TVP_INLINE_FUNC __inline
#else
	#define TVP_INLINE_FUNC 
#endif

static tjs_uint32 TVP_INLINE_FUNC TVPSaturatedAdd(tjs_uint32 a, tjs_uint32 b)
{
	/* Add each byte of packed 8bit values in two 32bit uint32, with saturation. */
	tjs_uint32 tmp = (  ( a & b ) + ( ((a ^ b)>>1) & 0x7f7f7f7f)  ) & 0x80808080;
	tmp = (tmp<<1) - (tmp>>7);
	return (a + b - tmp) | tmp;
}

/*
	TVPAddAlphaBlend_dest_src[_o]
	dest/src    :    a(additive-alpha)  d(alpha)  n(none alpha)
	_o          :    with opacity
*/

static tjs_uint32 TVP_INLINE_FUNC TVPAddAlphaBlend_n_a(tjs_uint32 dest, tjs_uint32 src)
{
	tjs_uint32 sopa = (~src) >> 24;
	return TVPSaturatedAdd((((dest & 0xff00ff)*sopa >> 8) & 0xff00ff) + 
		(((dest & 0xff00)*sopa >> 8) & 0xff00), src);
}

static tjs_uint32 TVP_INLINE_FUNC TVPAddAlphaBlend_HDA_n_a(tjs_uint32 dest, tjs_uint32 src)
{
	return (dest & 0xff000000) + (TVPAddAlphaBlend_n_a(dest, src) & 0xffffff);
}

static tjs_uint32 TVP_INLINE_FUNC TVPAddAlphaBlend_n_a_o(tjs_uint32 dest, tjs_uint32 src, tjs_int opa)
{
	src = (((src & 0xff00ff)*opa >> 8) & 0xff00ff) + (((src >> 8) & 0xff00ff)*opa & 0xff00ff00);
	return TVPAddAlphaBlend_n_a(dest, src);
}

static tjs_uint32 TVP_INLINE_FUNC TVPAddAlphaBlend_HDA_n_a_o(tjs_uint32 dest, tjs_uint32 src, tjs_int opa)
{
	return (dest & 0xff000000) + (TVPAddAlphaBlend_n_a_o(dest, src, opa) & 0xffffff);
}

static tjs_uint32 TVP_INLINE_FUNC TVPAddAlphaBlend_a_a(tjs_uint32 dest, tjs_uint32 src)
{
	/*
		Di = sat(Si, (1-Sa)*Di)
		Da = Sa + Da - SaDa
	*/

	tjs_uint32 dopa = dest >> 24;
	tjs_uint32 sopa = src >> 24;
	dopa = dopa + sopa - (dopa*sopa >> 8);
	dopa -= (dopa >> 8); /* adjust alpha */
	sopa ^= 0xff;
	src &= 0xffffff;
	return (dopa << 24) + 
		TVPSaturatedAdd((((dest & 0xff00ff)*sopa >> 8) & 0xff00ff) +
			(((dest & 0xff00)*sopa >> 8) & 0xff00), src);
}

static tjs_uint32 TVP_INLINE_FUNC TVPAddAlphaBlend_a_ca(tjs_uint32 dest, tjs_uint32 sopa, tjs_uint32 sopa_inv, tjs_uint32 src)
{
	/*
		Di = sat(Si, (1-Sa)*Di)
		Da = Sa + Da - SaDa
	*/

	tjs_uint32 dopa = dest >> 24;
	dopa = dopa + sopa - (dopa*sopa >> 8);
	dopa -= (dopa >> 8); /* adjust alpha */
	return (dopa << 24) + 
		TVPSaturatedAdd((((dest & 0xff00ff)*sopa_inv >> 8) & 0xff00ff) +
			(((dest & 0xff00)*sopa_inv >> 8) & 0xff00), src);
}

static tjs_uint32 TVP_INLINE_FUNC TVPAddAlphaBlend_a_a_o(tjs_uint32 dest, tjs_uint32 src, tjs_int opa)
{
	src = (((src & 0xff00ff)*opa >> 8) & 0xff00ff) + (((src >> 8) & 0xff00ff)*opa & 0xff00ff00);
	return TVPAddAlphaBlend_a_a(dest, src);
}

static tjs_uint32 TVP_INLINE_FUNC TVPMulColor(tjs_uint32 color, tjs_uint32 fac)
{
	return (((((color & 0x00ff00) * fac) & 0x00ff0000) +
			(((color & 0xff00ff) * fac) & 0xff00ff00) ) >> 8);
}

static tjs_uint32 TVP_INLINE_FUNC TVPAlphaAndColorToAdditiveAlpha(tjs_uint32 alpha, tjs_uint32 color)
{
	return TVPMulColor(color, alpha) + (color & 0xff000000);

}

static tjs_uint32 TVP_INLINE_FUNC TVPAlphaToAdditiveAlpha(tjs_uint32 a)
{
	return TVPAlphaAndColorToAdditiveAlpha(a >> 24, a);
}

static tjs_uint32 TVP_INLINE_FUNC TVPAddAlphaBlend_a_d(tjs_uint32 dest, tjs_uint32 src)
{
	return TVPAddAlphaBlend_a_a(dest, TVPAlphaToAdditiveAlpha(src));
}

static tjs_uint32 TVP_INLINE_FUNC TVPAddAlphaBlend_a_d_o(tjs_uint32 dest, tjs_uint32 src, tjs_int opa)
{
	src = (src & 0xffffff) + ((((src >> 24) * opa) >> 8) << 24);
	return TVPAddAlphaBlend_a_d(dest, src);
}

/* TVPAddAlphaBlend_d_a is not yet implemented because the expression may loose precision. */


static tjs_uint32 TVP_INLINE_FUNC TVPBlendARGB(tjs_uint32 b, tjs_uint32 a, tjs_int ratio)
{
	/* returns a * ratio + b * (1 - ratio) */
	tjs_uint32 b2;
	tjs_uint32 t;
	b2 = b & 0x00ff00ff;
	t = (b2 + (((a & 0x00ff00ff) - b2) * ratio >> 8)) & 0x00ff00ff;
	b2 = (b & 0xff00ff00) >> 8;
	return t + 
		(((b2 + (( ((a & 0xff00ff00) >> 8) - b2) * ratio >> 8)) << 8)& 0xff00ff00);
}




EOF
print FH $cnt;


;#-----------------------------------------------------------------
;# pixel alpha blending
;#-----------------------------------------------------------------

;# simple pixel alpha blending ( destination alpha alpha/additive-alpha/off )
;# alpha blending with opacity ( destination alpha alpha/additive-alpha/off )


print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPAlphaBlend_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	tjs_uint32 d1, s, d, sopa;
EOF


$content = <<EOF;
{
	s = *src;
	src++;
	d = *dest;
	sopa = s >> 24;
	d1 = d & 0xff00ff;
	d1 = (d1 + (((s & 0xff00ff) - d1) * sopa >> 8)) & 0xff00ff;
	d &= 0xff00;
	s &= 0xff00;
	*dest = d1 + ((d + ((s - d) * sopa >> 8)) & 0xff00);
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/* HDA : hold destination alpha */

/*export*/
TVP_GL_FUNC_DECL(void, TVPAlphaBlend_HDA_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	tjs_uint32 d1, s, d, sopa;
EOF

$content = <<EOF;
{
	s = *src;
	src++;
	d = *dest;
	sopa = s >> 24;
	d1 = d & 0xff00ff;
	d1 = ((d1 + (((s & 0xff00ff) - d1) * sopa >> 8)) & 0xff00ff) + (d & 0xff000000); /* hda */
	d &= 0xff00;
	s &= 0xff00;
	*dest = d1 + ((d + ((s - d) * sopa >> 8)) & 0xff00);
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPAlphaBlend_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	tjs_uint32 d1, s, d, sopa;
EOF


$content = <<EOF;
{
	s = *src;
	src++;
	d = *dest;
	sopa = ((s >> 24) * opa) >> 8;
	d1 = d & 0xff00ff;
	d1 = (d1 + (((s & 0xff00ff) - d1) * sopa >> 8)) & 0xff00ff;
	d &= 0xff00;
	s &= 0xff00;
	*dest = d1 + ((d + ((s - d) * sopa >> 8)) & 0xff00);
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;

/*export*/
TVP_GL_FUNC_DECL(void, TVPAlphaBlend_HDA_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	tjs_uint32 d1, s, d, sopa;
EOF

$content = <<EOF;
{
	s = *src;
	src++;
	d = *dest;
	sopa = ((s >> 24) * opa) >> 8;
	d1 = d & 0xff00ff;
	d1 = ((d1 + (((s & 0xff00ff) - d1) * sopa >> 8)) & 0xff00ff) + (d & 0xff000000);
	d &= 0xff00;
	s &= 0xff00;
	*dest = d1 + ((d + ((s - d) * sopa >> 8)) & 0xff00);
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPAlphaBlend_d_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	tjs_uint32 d1, s, d, sopa, addr, destalpha;
EOF


$content = <<EOF;
{
	s = *src;
	src++;
	d = *dest;
	addr = ((s >> 16) & 0xff00) + (d>>24);
	destalpha = TVPNegativeMulTable[addr]<<24;
	sopa = TVPOpacityOnOpacityTable[addr];
	d1 = d & 0xff00ff;
	d1 = (d1 + (((s & 0xff00ff) - d1) * sopa >> 8)) & 0xff00ff;
	d &= 0xff00;
	s &= 0xff00;
	*dest = d1 + ((d + ((s - d) * sopa >> 8)) & 0xff00) + destalpha;
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------


print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPAlphaBlend_a_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	//tjs_uint32 d1, s, d, sopa, addr, destalpha;
EOF


$content = <<EOF;
	dest[{ofs}] = TVPAddAlphaBlend_a_d(dest[{ofs}], src[{ofs}]);
EOF

&loop_unroll_c_2($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------


print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPAlphaBlend_do_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	tjs_uint32 d1, s, d, sopa, addr, destalpha;
EOF


$content = <<EOF;
{
	s = *src;
	src++;
	d = *dest;
	addr = (( (s>>24)*opa) & 0xff00) + (d>>24);
	destalpha = TVPNegativeMulTable[addr]<<24;
	sopa = TVPOpacityOnOpacityTable[addr];
	d1 = d & 0xff00ff;
	d1 = (d1 + (((s & 0xff00ff) - d1) * sopa >> 8)) & 0xff00ff;
	d &= 0xff00;
	s &= 0xff00;
	*dest = d1 + ((d + ((s - d) * sopa >> 8)) & 0xff00) + destalpha;
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------


print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPAlphaBlend_ao_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
EOF


$content = <<EOF;
	dest[{ofs}] = TVPAddAlphaBlend_a_d_o(dest[{ofs}], src[{ofs}], opa);
EOF

&loop_unroll_c_2($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------
;# alpha blend matting
;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPAlphaColorMat_c, (tjs_uint32 *dest, const tjs_uint32 color, tjs_int len))
{
	tjs_uint32 d1, s, d, sopa;
EOF


$content = <<EOF;
{
	s = *dest;
	d = color;
	sopa = s >> 24;
	d1 = d & 0xff00ff;
	d1 = (d1 + (((s & 0xff00ff) - d1) * sopa >> 8)) & 0xff00ff;
	d &= 0xff00;
	s &= 0xff00;
	*dest = d1 + ((d + ((s - d) * sopa >> 8)) & 0xff00) + 0xff000000;
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF


;#-----------------------------------------------------------------
;# pixel additive alpha blending
;#-----------------------------------------------------------------

;# pixel additive alpha blending ( destination alpha is alpha/additive-alpha/off )
;# additive alpha blending with opacity ( destination alpha alpha/additive-alpha/off )


print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPAdditiveAlphaBlend_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
EOF


$content = <<EOF;
	dest[{ofs}] = TVPAddAlphaBlend_n_a(dest[{ofs}], src[{ofs}]);
EOF

&loop_unroll_c_2($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------


print FC <<EOF;
/* HDA : hold destination alpha */

/*export*/
TVP_GL_FUNC_DECL(void, TVPAdditiveAlphaBlend_HDA_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
EOF

$content = <<EOF;
	dest[{ofs}] = TVPAddAlphaBlend_HDA_n_a(dest[{ofs}], src[{ofs}]);
EOF

&loop_unroll_c_2($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPAdditiveAlphaBlend_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
EOF


$content = <<EOF;
	dest[{ofs}] = TVPAddAlphaBlend_n_a_o(dest[{ofs}], src[{ofs}], opa);
EOF

&loop_unroll_c_2($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;

/*export*/
TVP_GL_FUNC_DECL(void, TVPAdditiveAlphaBlend_HDA_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
EOF

$content = <<EOF;
	dest[{ofs}] = TVPAddAlphaBlend_HDA_n_a_o(dest[{ofs}], src[{ofs}], opa);
EOF

&loop_unroll_c_2($content, 'len', 4);

print FC <<EOF;
}

EOF


;#-----------------------------------------------------------------


print FC <<EOF;
/*not export*/
TVP_GL_FUNC_DECL(void, TVPAdditiveAlphaBlend_d_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{/*YET NOT IMPLEMENTED*//*MAY LOOSE ADDITIVE STUFF*/
EOF


$content = <<EOF;
	dest[{ofs}] = 0;
EOF

&loop_unroll_c_2($content, 'len', 4);

print FC <<EOF;
}

EOF


;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPAdditiveAlphaBlend_a_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
EOF


$content = <<EOF;
	dest[{ofs}] = TVPAddAlphaBlend_a_a(dest[{ofs}], src[{ofs}]);
EOF

&loop_unroll_c_2($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*not export*/
TVP_GL_FUNC_DECL(void, TVPAdditiveAlphaBlend_do_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{/*YET NOT IMPLEMENTED*//*MAY LOOSE ADDITIVE STUFF*/
EOF


$content = <<EOF;
	dest[{ofs}] = 0;
EOF

&loop_unroll_c_2($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPAdditiveAlphaBlend_ao_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
EOF


$content = <<EOF;
	dest[{ofs}] = TVPAddAlphaBlend_a_a_o(dest[{ofs}], src[{ofs}], opa);
EOF

&loop_unroll_c_2($content, 'len', 4);

print FC <<EOF;
}

EOF


;#-----------------------------------------------------------------
;# conversion between additive-alpha and simple alpha
;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPConvertAdditiveAlphaToAlpha_c, (tjs_uint32 *buf, tjs_int len))
{/*MAY LOOSE ADDITIVE STUFF*/
	tjs_uint32 tmp;
	const tjs_uint8 * t;
EOF


$content = <<EOF;
{
	tmp = *buf;
	t = ((tmp >> 16) & 0xff00) + TVPDivTable;
	*buf = (tmp & 0xff000000) +
		(t[(tmp >> 16) & 0xff] << 16) +
		(t[(tmp >>  8) & 0xff] <<  8) +
		(t[ tmp        & 0xff]      );
	buf++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;

/*export*/
TVP_GL_FUNC_DECL(void, TVPConvertAlphaToAdditiveAlpha_c, (tjs_uint32 *buf, tjs_int len))
{
EOF


$content = <<EOF;
	buf[{ofs}] = TVPAlphaToAdditiveAlpha(buf[{ofs}]);
EOF

&loop_unroll_c_2($content, 'len', 4);

print FC <<EOF;
}

EOF


;#-----------------------------------------------------------------
;# stretching pixel alpha blending
;#-----------------------------------------------------------------

;# stretching simple pixel alpha blending ( destination alpha on/off )
;# stretching alpha blending with opacity ( destination alpha on/off )


print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPStretchAlphaBlend_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep))
{
	tjs_uint32 d1, s, d, sopa;
EOF


$content = <<EOF;
{
	s = src[srcstart >> 16];
	srcstart += srcstep;
	d = *dest;
	sopa = s >> 24;
	d1 = d & 0xff00ff;
	d1 = (d1 + (((s & 0xff00ff) - d1) * sopa >> 8)) & 0xff00ff;
	d &= 0xff00;
	s &= 0xff00;
	*dest = d1 + ((d + ((s - d) * sopa >> 8)) & 0xff00);
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/* HDA : hold destination alpha */

/*export*/
TVP_GL_FUNC_DECL(void, TVPStretchAlphaBlend_HDA_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep))
{
	tjs_uint32 d1, s, d, sopa;
EOF

$content = <<EOF;
{
	s = src[srcstart >> 16];
	srcstart += srcstep;
	d = *dest;
	sopa = s >> 24;
	d1 = d & 0xff00ff;
	d1 = ((d1 + (((s & 0xff00ff) - d1) * sopa >> 8)) & 0xff00ff) + (d & 0xff000000); /* hda */
	d &= 0xff00;
	s &= 0xff00;
	*dest = d1 + ((d + ((s - d) * sopa >> 8)) & 0xff00);
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPStretchAlphaBlend_o_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep, tjs_int opa))
{
	tjs_uint32 d1, s, d, sopa;
EOF


$content = <<EOF;
{
	s = src[srcstart >> 16];
	srcstart += srcstep;
	d = *dest;
	sopa = ((s >> 24) * opa) >> 8;
	d1 = d & 0xff00ff;
	d1 = (d1 + (((s & 0xff00ff) - d1) * sopa >> 8)) & 0xff00ff;
	d &= 0xff00;
	s &= 0xff00;
	*dest = d1 + ((d + ((s - d) * sopa >> 8)) & 0xff00);
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;

/*export*/
TVP_GL_FUNC_DECL(void, TVPStretchAlphaBlend_HDA_o_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep, tjs_int opa))
{
	tjs_uint32 d1, s, d, sopa;
EOF

$content = <<EOF;
{
	s = src[srcstart >> 16];
	srcstart += srcstep;
	d = *dest;
	sopa = ((s >> 24) * opa) >> 8;
	d1 = d & 0xff00ff;
	d1 = ((d1 + (((s & 0xff00ff) - d1) * sopa >> 8)) & 0xff00ff) + (d & 0xff000000);
	d &= 0xff00;
	s &= 0xff00;
	*dest = d1 + ((d + ((s - d) * sopa >> 8)) & 0xff00);
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPStretchAlphaBlend_d_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep))
{
	tjs_uint32 d1, s, d, sopa, addr, destalpha;
EOF


$content = <<EOF;
{
	s = src[srcstart >> 16];
	srcstart += srcstep;
	d = *dest;
	addr = ((s >> 16) & 0xff00) + (d>>24);
	destalpha = TVPNegativeMulTable[addr]<<24;
	sopa = TVPOpacityOnOpacityTable[addr];
	d1 = d & 0xff00ff;
	d1 = (d1 + (((s & 0xff00ff) - d1) * sopa >> 8)) & 0xff00ff;
	d &= 0xff00;
	s &= 0xff00;
	*dest = d1 + ((d + ((s - d) * sopa >> 8)) & 0xff00) + destalpha;
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPStretchAlphaBlend_a_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep))
{
EOF


$content = <<EOF;
{
	*dest = TVPAddAlphaBlend_a_d(*dest, src[srcstart >> 16]);
	srcstart += srcstep;
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPStretchAlphaBlend_do_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep, tjs_int opa))
{
	tjs_uint32 d1, s, d, sopa, addr, destalpha;
EOF


$content = <<EOF;
{
	s = src[srcstart >> 16];
	srcstart += srcstep;
	d = *dest;
	addr = (( (s>>24)*opa) & 0xff00) + (d>>24);
	destalpha = TVPNegativeMulTable[addr]<<24;
	sopa = TVPOpacityOnOpacityTable[addr];
	d1 = d & 0xff00ff;
	d1 = (d1 + (((s & 0xff00ff) - d1) * sopa >> 8)) & 0xff00ff;
	d &= 0xff00;
	s &= 0xff00;
	*dest = d1 + ((d + ((s - d) * sopa >> 8)) & 0xff00) + destalpha;
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPStretchAlphaBlend_ao_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep, tjs_int opa))
{
EOF


$content = <<EOF;
{
	*dest = TVPAddAlphaBlend_a_d_o(*dest, src[srcstart >> 16], opa);
	srcstart += srcstep;
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF



;#-----------------------------------------------------------------
;# stretching additive alpha blending
;#-----------------------------------------------------------------

;# stretching pixel additive alpha blending ( destination alpha alpha/additive-alpha/off )
;# stretching alpha blending with opacity ( destination alpha alpha/additive-alpha/off )


print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPStretchAdditiveAlphaBlend_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep))
{
EOF


$content = <<EOF;
{
	*dest = TVPAddAlphaBlend_n_a(*dest, src[srcstart >> 16]);
	srcstart += srcstep;
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPInterpStretchAdditiveAlphaBlend_c, (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src1, const tjs_uint32 *src2, tjs_int blend_y, tjs_int srcstart, tjs_int srcstep))
{
	/* stretching additive alpha blend with bilinear interpolation */
	tjs_int blend_x;
	tjs_int sp;

	blend_y += blend_y >> 7; /* adjust blend ratio */

	destlen -= 1;
	while(destlen > 0)
	{
		blend_x = (srcstart & 0xffff) >> 8;
		sp = srcstart >> 16;
		dest[0] = TVPAddAlphaBlend_n_a(dest[0], TVPBlendARGB(
			TVPBlendARGB(src1[sp], src1[sp+1], blend_x),
			TVPBlendARGB(src2[sp], src2[sp+1], blend_x),
				blend_y));
		srcstart += srcstep;

		blend_x = (srcstart & 0xffff) >> 8;
		sp = srcstart >> 16;
		dest[1] = TVPAddAlphaBlend_n_a(dest[1], TVPBlendARGB(
			TVPBlendARGB(src1[sp], src1[sp+1], blend_x),
			TVPBlendARGB(src2[sp], src2[sp+1], blend_x),
				blend_y));
		srcstart += srcstep;

		dest += 2;
		destlen -= 2;
	}

	destlen += 1;

	while(destlen > 0)
	{
		blend_x = (srcstart & 0xffff) >> 8;
		sp = srcstart >> 16;
		dest[0] = TVPAddAlphaBlend_n_a(dest[0], TVPBlendARGB(
			TVPBlendARGB(src1[sp], src1[sp+1], blend_x),
			TVPBlendARGB(src2[sp], src2[sp+1], blend_x),
				blend_y));
		srcstart += srcstep;
		dest ++;
		destlen --;
	}
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/* HDA : hold destination alpha */

/*export*/
TVP_GL_FUNC_DECL(void, TVPStretchAdditiveAlphaBlend_HDA_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep))
{
EOF

$content = <<EOF;
{
	*dest = TVPAddAlphaBlend_HDA_n_a(*dest, src[srcstart >> 16]);
	srcstart += srcstep;
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPStretchAdditiveAlphaBlend_o_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep, tjs_int opa))
{
EOF


$content = <<EOF;
{
	*dest = TVPAddAlphaBlend_n_a_o(*dest, src[srcstart >> 16], opa);
	srcstart += srcstep;
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF


;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPInterpStretchAdditiveAlphaBlend_o_c, (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src1, const tjs_uint32 *src2, tjs_int blend_y, tjs_int srcstart, tjs_int srcstep, tjs_int opa))
{
	/* stretching additive alpha blend with bilinear interpolation */
	tjs_int blend_x;
	tjs_int sp;

	blend_y += blend_y >> 7; /* adjust blend ratio */

	destlen -= 1;
	while(destlen > 0)
	{
		blend_x = (srcstart & 0xffff) >> 8;
		sp = srcstart >> 16;
		dest[0] = TVPAddAlphaBlend_n_a_o(dest[0], TVPBlendARGB(
			TVPBlendARGB(src1[sp], src1[sp+1], blend_x),
			TVPBlendARGB(src2[sp], src2[sp+1], blend_x),
				blend_y), opa);
		srcstart += srcstep;

		blend_x = (srcstart & 0xffff) >> 8;
		sp = srcstart >> 16;
		dest[1] = TVPAddAlphaBlend_n_a_o(dest[1], TVPBlendARGB(
			TVPBlendARGB(src1[sp], src1[sp+1], blend_x),
			TVPBlendARGB(src2[sp], src2[sp+1], blend_x),
				blend_y), opa);
		srcstart += srcstep;

		dest += 2;
		destlen -= 2;
	}

	destlen += 1;

	while(destlen > 0)
	{
		blend_x = (srcstart & 0xffff) >> 8;
		sp = srcstart >> 16;
		dest[0] = TVPAddAlphaBlend_n_a_o(dest[0], TVPBlendARGB(
			TVPBlendARGB(src1[sp], src1[sp+1], blend_x),
			TVPBlendARGB(src2[sp], src2[sp+1], blend_x),
				blend_y), opa);
		srcstart += srcstep;
		dest ++;
		destlen --;
	}
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;

/*export*/
TVP_GL_FUNC_DECL(void, TVPStretchAdditiveAlphaBlend_HDA_o_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep, tjs_int opa))
{
EOF

$content = <<EOF;
{
	*dest = TVPAddAlphaBlend_HDA_n_a_o(*dest, src[srcstart >> 16], opa);
	srcstart += srcstep;
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*not export*/
TVP_GL_FUNC_DECL(void, TVPStretchAdditiveAlphaBlend_d_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep))
{/*YET NOT IMPLEMENTED*//*MAY LOOSE ADDITIVE STUFF*/
EOF


$content = <<EOF;
{
	srcstart += srcstep;
	*dest = 0;
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPStretchAdditiveAlphaBlend_a_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep))
{
EOF


$content = <<EOF;
{
	*dest = TVPAddAlphaBlend_a_a(*dest, src[srcstart >> 16]);
	srcstart += srcstep;
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*not export*/
TVP_GL_FUNC_DECL(void, TVPStretchAdditiveAlphaBlend_do_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep, tjs_int opa))
{/*YET NOT IMPLEMENTED*//*MAY LOOSE ADDITIVE STUFF*/
EOF


$content = <<EOF;
{
	srcstart += srcstep;
	*dest = 0;
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF


;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPStretchAdditiveAlphaBlend_ao_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep, tjs_int opa))
{
EOF


$content = <<EOF;
{
	*dest = TVPAddAlphaBlend_a_a_o(*dest, src[srcstart >> 16], opa);
	srcstart += srcstep;
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF



;#-----------------------------------------------------------------
;# linear transforming pixel alpha blending
;#-----------------------------------------------------------------

;# linear transforming simple pixel alpha blending ( destination alpha alpha/additive-alpha/off )
;# linear transforming alpha blending with opacity ( destination alpha alpha/additive-alpha/off )

;# 'linear transformation' does not means that does linear interpolation.

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPLinTransAlphaBlend_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch))
{
	tjs_uint32 d1, s, d, sopa;
EOF


$content = <<EOF;
{
	s = *( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16)*srcpitch) + (sx>>16));
	sx += stepx;
	sy += stepy;
	d = *dest;
	sopa = s >> 24;
	d1 = d & 0xff00ff;
	d1 = (d1 + (((s & 0xff00ff) - d1) * sopa >> 8)) & 0xff00ff;
	d &= 0xff00;
	s &= 0xff00;
	*dest = d1 + ((d + ((s - d) * sopa >> 8)) & 0xff00);
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------


print FC <<EOF;
/* HDA : hold destination alpha */

/*export*/
TVP_GL_FUNC_DECL(void, TVPLinTransAlphaBlend_HDA_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch))
{
	tjs_uint32 d1, s, d, sopa;
EOF

$content = <<EOF;
{
	s = *( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16)*srcpitch) + (sx>>16));
	sx += stepx;
	sy += stepy;
	d = *dest;
	sopa = s >> 24;
	d1 = d & 0xff00ff;
	d1 = ((d1 + (((s & 0xff00ff) - d1) * sopa >> 8)) & 0xff00ff) + (d & 0xff000000); /* hda */
	d &= 0xff00;
	s &= 0xff00;
	*dest = d1 + ((d + ((s - d) * sopa >> 8)) & 0xff00);
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPLinTransAlphaBlend_o_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa))
{
	tjs_uint32 d1, s, d, sopa;
EOF


$content = <<EOF;
{
	s = *( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16)*srcpitch) + (sx>>16));
	sx += stepx;
	sy += stepy;
	d = *dest;
	sopa = ((s >> 24) * opa) >> 8;
	d1 = d & 0xff00ff;
	d1 = (d1 + (((s & 0xff00ff) - d1) * sopa >> 8)) & 0xff00ff;
	d &= 0xff00;
	s &= 0xff00;
	*dest = d1 + ((d + ((s - d) * sopa >> 8)) & 0xff00);
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;

/*export*/
TVP_GL_FUNC_DECL(void, TVPLinTransAlphaBlend_HDA_o_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa))
{
	tjs_uint32 d1, s, d, sopa;
EOF

$content = <<EOF;
{
	s = *( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16)*srcpitch) + (sx>>16));
	sx += stepx;
	sy += stepy;
	d = *dest;
	sopa = ((s >> 24) * opa) >> 8;
	d1 = d & 0xff00ff;
	d1 = ((d1 + (((s & 0xff00ff) - d1) * sopa >> 8)) & 0xff00ff) + (d & 0xff000000);
	d &= 0xff00;
	s &= 0xff00;
	*dest = d1 + ((d + ((s - d) * sopa >> 8)) & 0xff00);
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPLinTransAlphaBlend_d_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch))
{
	tjs_uint32 d1, s, d, sopa, addr, destalpha;
EOF


$content = <<EOF;
{
	s = *( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16)*srcpitch) + (sx>>16));
	sx += stepx;
	sy += stepy;
	d = *dest;
	addr = ((s >> 16) & 0xff00) + (d>>24);
	destalpha = TVPNegativeMulTable[addr]<<24;
	sopa = TVPOpacityOnOpacityTable[addr];
	d1 = d & 0xff00ff;
	d1 = (d1 + (((s & 0xff00ff) - d1) * sopa >> 8)) & 0xff00ff;
	d &= 0xff00;
	s &= 0xff00;
	*dest = d1 + ((d + ((s - d) * sopa >> 8)) & 0xff00) + destalpha;
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------


print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPLinTransAlphaBlend_a_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch))
{
EOF


$content = <<EOF;
{
	*dest = TVPAddAlphaBlend_a_d(*dest, 
		*( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16)*srcpitch) + (sx>>16)));
	sx += stepx;
	sy += stepy;
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPLinTransAlphaBlend_do_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa))
{
	tjs_uint32 d1, s, d, sopa, addr, destalpha;
EOF


$content = <<EOF;
{
	s = *( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16)*srcpitch) + (sx>>16));
	sx += stepx;
	sy += stepy;
	d = *dest;
	addr = (( (s>>24)*opa) & 0xff00) + (d>>24);
	destalpha = TVPNegativeMulTable[addr]<<24;
	sopa = TVPOpacityOnOpacityTable[addr];
	d1 = d & 0xff00ff;
	d1 = (d1 + (((s & 0xff00ff) - d1) * sopa >> 8)) & 0xff00ff;
	d &= 0xff00;
	s &= 0xff00;
	*dest = d1 + ((d + ((s - d) * sopa >> 8)) & 0xff00) + destalpha;
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPLinTransAlphaBlend_ao_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa))
{
EOF


$content = <<EOF;
{
	*dest = TVPAddAlphaBlend_a_d_o(*dest, 
		*( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16)*srcpitch) + (sx>>16)), opa);
	sx += stepx;
	sy += stepy;
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF




;#-----------------------------------------------------------------
;# linear transforming pixel additive alpha blending
;#-----------------------------------------------------------------

;# linear transforming pixel additive alpha blending ( destination alpha alpha/additive-alpha/off )
;# linear transforming additive alpha blending with opacity ( destination alpha alpha/additive-alpha/off )

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPLinTransAdditiveAlphaBlend_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch))
{
EOF


$content = <<EOF;
{
	*dest = TVPAddAlphaBlend_n_a(*dest, 
		*( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16)*srcpitch) + (sx>>16)));
	sx += stepx;
	sy += stepy;
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF


;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPInterpLinTransAdditiveAlphaBlend_c, (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch))
{
	/* bilinear interpolation version */
	/* note that srcpitch unit is in byte */
	destlen -= 1;
	while(destlen > 0)
	{
		int blend_x, blend_y;
		const tjs_uint32 *p0, *p1;

		blend_x = (sx & 0xffff) >> 8;
		blend_x += blend_x >> 7;
		blend_y = (sy & 0xffff) >> 8;
		blend_y += blend_y >> 7;
		p0 = (const tjs_uint32*)((const tjs_uint8*)src + ((sy>>16)  )*srcpitch) + (sx>>16);
		p1 = (const tjs_uint32*)((const tjs_uint8*)p0 + srcpitch);
		dest[0] = TVPAddAlphaBlend_n_a(dest[0], TVPBlendARGB(
			TVPBlendARGB(p0[0], p0[1], blend_x),
			TVPBlendARGB(p1[0], p1[1], blend_x),
				blend_y));
		sx += stepx, sy += stepy;

		blend_x = (sx & 0xffff) >> 8;
		blend_x += blend_x >> 7;
		blend_y = (sy & 0xffff) >> 8;
		blend_y += blend_y >> 7;
		p0 = (const tjs_uint32*)((const tjs_uint8*)src + ((sy>>16)  )*srcpitch) + (sx>>16);
		p1 = (const tjs_uint32*)((const tjs_uint8*)p0 + srcpitch);
		dest[1] = TVPAddAlphaBlend_n_a(dest[1], TVPBlendARGB(
			TVPBlendARGB(p0[0], p0[1], blend_x),
			TVPBlendARGB(p1[0], p1[1], blend_x),
				blend_y));
		sx += stepx, sy += stepy;

		dest += 2;
		destlen -= 2;
	}

	destlen += 1;

	while(destlen > 0)
	{
		int blend_x, blend_y;
		const tjs_uint32 *p0, *p1;

		blend_x = (sx & 0xffff) >> 8;
		blend_x += blend_x >> 7;
		blend_y = (sy & 0xffff) >> 8;
		blend_y += blend_y >> 7;
		p0 = (const tjs_uint32*)((const tjs_uint8*)src + ((sy>>16)  )*srcpitch) + (sx>>16);
		p1 = (const tjs_uint32*)((const tjs_uint8*)p0 + srcpitch);
		dest[0] = TVPAddAlphaBlend_n_a(dest[0], TVPBlendARGB(
			TVPBlendARGB(p0[0], p0[1], blend_x),
			TVPBlendARGB(p1[0], p1[1], blend_x),
				blend_y));
		sx += stepx, sy += stepy;

		dest ++;
		destlen --;
	}
}

EOF

;#-----------------------------------------------------------------


print FC <<EOF;
/* HDA : hold destination alpha */

/*export*/
TVP_GL_FUNC_DECL(void, TVPLinTransAdditiveAlphaBlend_HDA_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch))
{
EOF

$content = <<EOF;
{
	*dest = TVPAddAlphaBlend_HDA_n_a(*dest, 
		*( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16)*srcpitch) + (sx>>16)));
	sx += stepx;
	sy += stepy;
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------


print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPLinTransAdditiveAlphaBlend_o_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa))
{
EOF


$content = <<EOF;
{
	*dest = TVPAddAlphaBlend_n_a_o(*dest, 
		*( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16)*srcpitch) + (sx>>16)), opa);
	sx += stepx;
	sy += stepy;
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPInterpLinTransAdditiveAlphaBlend_o_c, (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa))
{
	/* bilinear interpolation version */
	/* note that srcpitch unit is in byte */
	opa += opa >> 7;

	destlen -= 1;
	while(destlen > 0)
	{
		int blend_x, blend_y;
		const tjs_uint32 *p0, *p1;

		blend_x = (sx & 0xffff) >> 8;
		blend_x += blend_x >> 7;
		blend_y = (sy & 0xffff) >> 8;
		blend_y += blend_y >> 7;
		p0 = (const tjs_uint32*)((const tjs_uint8*)src + ((sy>>16)  )*srcpitch) + (sx>>16);
		p1 = (const tjs_uint32*)((const tjs_uint8*)p0 + srcpitch);
		dest[0] = TVPAddAlphaBlend_n_a_o(dest[0], TVPBlendARGB(
			TVPBlendARGB(p0[0], p0[1], blend_x),
			TVPBlendARGB(p1[0], p1[1], blend_x),
				blend_y), opa);
		sx += stepx, sy += stepy;

		blend_x = (sx & 0xffff) >> 8;
		blend_x += blend_x >> 7;
		blend_y = (sy & 0xffff) >> 8;
		blend_y += blend_y >> 7;
		p0 = (const tjs_uint32*)((const tjs_uint8*)src + ((sy>>16)  )*srcpitch) + (sx>>16);
		p1 = (const tjs_uint32*)((const tjs_uint8*)p0 + srcpitch);
		dest[1] = TVPAddAlphaBlend_n_a_o(dest[1], TVPBlendARGB(
			TVPBlendARGB(p0[0], p0[1], blend_x),
			TVPBlendARGB(p1[0], p1[1], blend_x),
				blend_y), opa);
		sx += stepx, sy += stepy;

		dest += 2;
		destlen -= 2;
	}

	destlen += 1;

	while(destlen > 0)
	{
		int blend_x, blend_y;
		const tjs_uint32 *p0, *p1;

		blend_x = (sx & 0xffff) >> 8;
		blend_x += blend_x >> 7;
		blend_y = (sy & 0xffff) >> 8;
		blend_y += blend_y >> 7;
		p0 = (const tjs_uint32*)((const tjs_uint8*)src + ((sy>>16)  )*srcpitch) + (sx>>16);
		p1 = (const tjs_uint32*)((const tjs_uint8*)p0 + srcpitch);
		dest[0] = TVPAddAlphaBlend_n_a_o(dest[0], TVPBlendARGB(
			TVPBlendARGB(p0[0], p0[1], blend_x),
			TVPBlendARGB(p1[0], p1[1], blend_x),
				blend_y), opa);
		sx += stepx, sy += stepy;

		dest ++;
		destlen --;
	}
}

EOF


;#-----------------------------------------------------------------

print FC <<EOF;

/*export*/
TVP_GL_FUNC_DECL(void, TVPLinTransAdditiveAlphaBlend_HDA_o_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa))
{
EOF

$content = <<EOF;
{
	*dest = TVPAddAlphaBlend_HDA_n_a_o(*dest, 
		*( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16)*srcpitch) + (sx>>16)), opa);
	sx += stepx;
	sy += stepy;
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*not export*/
TVP_GL_FUNC_DECL(void, TVPLinTransAdditiveAlphaBlend_d_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch))
{/*YET NOT IMPLEMENTED*//*MAY LOOSE ADDITIVE STUFF*/
EOF


$content = <<EOF;
{
	*dest = 0;
	sx += stepx;
	sy += stepy;
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPLinTransAdditiveAlphaBlend_a_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch))
{
EOF


$content = <<EOF;
{
	*dest = TVPAddAlphaBlend_a_a(*dest, 
		*( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16)*srcpitch) + (sx>>16)));
	sx += stepx;
	sy += stepy;
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*not export*/
TVP_GL_FUNC_DECL(void, TVPLinTransAdditiveAlphaBlend_do_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa))
{/*YET NOT IMPLEMENTED*//*MAY LOOSE ADDITIVE STUFF*/
EOF


$content = <<EOF;
{
	*dest = 0;
	sx += stepx;
	sy += stepy;
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPLinTransAdditiveAlphaBlend_ao_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa))
{
EOF


$content = <<EOF;
{
	*dest = TVPAddAlphaBlend_a_a_o(*dest, 
		*( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16)*srcpitch) + (sx>>16)), opa);
	sx += stepx;
	sy += stepy;
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF






;#-----------------------------------------------------------------
;# constant ratio alpha blending
;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPCopyOpaqueImage_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	tjs_uint32 t1, t2;
EOF


$content = <<EOF;
	t1 = src[{ofs}];;
	t1 |= 0xff000000;;
	dest[{ofs}] = t1;;
EOF

$content2 = <<EOF;
	t2 = src[{ofs}];;
	t2 |= 0xff000000;;
	dest[{ofs}] = t2;;
EOF
	

&loop_unroll_c_int_2($content, $content2, 'len', 8);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPConstAlphaBlend_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	tjs_uint32 d1, s, d;
EOF


$content = <<EOF;
{
	s = *src;
	src++;
	d = *dest;
	d1 = d & 0xff00ff;
	d1 = (d1 + (((s & 0xff00ff) - d1) * opa >> 8)) & 0xff00ff;
	d &= 0xff00;
	s &= 0xff00;
	*dest = d1 | ((d + ((s - d) * opa >> 8)) & 0xff00);
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPConstAlphaBlend_HDA_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	tjs_uint32 d1, s, d;
EOF


$content = <<EOF;
{
	s = *src;
	src++;
	d = *dest;
	d1 = d & 0xff00ff;
	d1 = ((d1 + (((s & 0xff00ff) - d1) * opa >> 8)) & 0xff00ff) + (d & 0xff000000);
	d &= 0xff00;
	s &= 0xff00;
	*dest = d1 | ((d + ((s - d) * opa >> 8)) & 0xff00);
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------


print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPConstAlphaBlend_d_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	tjs_uint32 d1, s, d, addr;
	tjs_int alpha;
	opa <<= 8;
EOF

$content = <<EOF;
{
	s = *src;
	src++;
	d = *dest;
	addr = opa + (d>>24);
	alpha = TVPOpacityOnOpacityTable[addr];
	d1 = d & 0xff00ff;
	d1 = ((d1 + (((s & 0xff00ff) - d1) * alpha >> 8)) & 0xff00ff) +
		(TVPNegativeMulTable[addr]<<24);
	d &= 0xff00;
	s &= 0xff00;
	*dest = d1 | ((d + ((s - d) * alpha >> 8)) & 0xff00);
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPConstAlphaBlend_a_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	opa <<= 24;
EOF

$content = <<EOF;
	dest[{ofs}] = TVPAddAlphaBlend_a_a(dest[{ofs}], (src[{ofs}] & 0xffffff) | opa);
EOF

&loop_unroll_c_2($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------
;# constant ratio alpha blending with stretching
;#-----------------------------------------------------------------

;#  (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep)

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPStretchCopyOpaqueImage_c, (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep))
{

	destlen -= 3;
	while(destlen > 0)
	{
		dest[0] = 0xff000000 | src[srcstart >> 16];
		srcstart += srcstep;
		dest[1] = 0xff000000 | src[srcstart >> 16];
		srcstart += srcstep;
		dest[2] = 0xff000000 | src[srcstart >> 16];
		srcstart += srcstep;
		dest[3] = 0xff000000 | src[srcstart >> 16];
		srcstart += srcstep;
		dest += 4;
		destlen -= 4;
	}

	destlen += 3;

	while(destlen > 0)
	{
		dest[0] = 0xff000000 | src[srcstart >> 16];
		srcstart += srcstep;
		dest ++;
		destlen --;
	}
}

EOF

;#-----------------------------------------------------------------


print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPStretchConstAlphaBlend_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep, tjs_int opa))
{
	tjs_uint32 d1, s, d;
EOF


$content = <<EOF;
{
	s = src[srcstart>>16];
	srcstart += srcstep;
	d = *dest;
	d1 = d & 0xff00ff;
	d1 = (d1 + (((s & 0xff00ff) - d1) * opa >> 8)) & 0xff00ff;
	d &= 0xff00;
	s &= 0xff00;
	*dest = d1 | ((d + ((s - d) * opa >> 8)) & 0xff00);
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------
print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPInterpStretchConstAlphaBlend_c, (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src1, const tjs_uint32 *src2, tjs_int blend_y, tjs_int srcstart, tjs_int srcstep, tjs_int opa))
{
	/* stretch copy with bilinear interpolation */
	tjs_int blend_x;
	tjs_int sp;

	blend_y += blend_y >> 7; /* adjust blend ratio */
	opa += opa > 7; /* adjust opa */

	destlen -= 1;
	while(destlen > 0)
	{
		blend_x = (srcstart & 0xffff) >> 8;
		sp = srcstart >> 16;
		dest[0] = TVPBlendARGB(dest[0], TVPBlendARGB(
			TVPBlendARGB(src1[sp], src1[sp+1], blend_x),
			TVPBlendARGB(src2[sp], src2[sp+1], blend_x),
				blend_y), opa);
		srcstart += srcstep;

		blend_x = (srcstart & 0xffff) >> 8;
		sp = srcstart >> 16;
		dest[1] = TVPBlendARGB(dest[1], TVPBlendARGB(
			TVPBlendARGB(src1[sp], src1[sp+1], blend_x),
			TVPBlendARGB(src2[sp], src2[sp+1], blend_x),
				blend_y), opa);
		srcstart += srcstep;

		dest += 2;
		destlen -= 2;
	}

	destlen += 1;

	while(destlen > 0)
	{
		blend_x = (srcstart & 0xffff) >> 8;
		sp = srcstart >> 16;
		dest[0] = TVPBlendARGB(dest[0], TVPBlendARGB(
			TVPBlendARGB(src1[sp], src1[sp+1], blend_x),
			TVPBlendARGB(src2[sp], src2[sp+1], blend_x),
				blend_y), opa);
		srcstart += srcstep;
		dest ++;
		destlen --;
	}
}

EOF
;#-----------------------------------------------------------------


print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPStretchConstAlphaBlend_HDA_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep, tjs_int opa))
{
	tjs_uint32 d1, s, d;
EOF


$content = <<EOF;
{
	s = src[srcstart >> 16];
	srcstart += srcstep;
	d = *dest;
	d1 = d & 0xff00ff;
	d1 = ((d1 + (((s & 0xff00ff) - d1) * opa >> 8)) & 0xff00ff) + (d & 0xff000000);
	d &= 0xff00;
	s &= 0xff00;
	*dest = d1 | ((d + ((s - d) * opa >> 8)) & 0xff00);
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPStretchConstAlphaBlend_d_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep, tjs_int opa))
{
	tjs_uint32 d1, s, d, addr;
	tjs_int alpha;
	if(opa > 128) opa ++; /* adjust for error */
EOF

$content = <<EOF;
{
	s = src[srcstart >> 16];
	srcstart += srcstep;
	d = *dest;
	addr = (( (s>>24)*opa) & 0xff00) + (d>>24);
	alpha = TVPOpacityOnOpacityTable[addr];
	d1 = d & 0xff00ff;
	d1 = ((d1 + (((s & 0xff00ff) - d1) * alpha >> 8)) & 0xff00ff) +
		(TVPNegativeMulTable[addr]<<24);
	d &= 0xff00;
	s &= 0xff00;
	*dest = d1 | ((d + ((s - d) * alpha >> 8)) & 0xff00);
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPStretchConstAlphaBlend_a_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep, tjs_int opa))
{
	opa <<= 24;
EOF

$content = <<EOF;
{
	*dest = TVPAddAlphaBlend_a_a(*dest, (src[srcstart >> 16] & 0xffffff) | opa);
	srcstart += srcstep;
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF





;#-----------------------------------------------------------------
;# constant ratio alpha blending with linear transformation
;#-----------------------------------------------------------------

;#  (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch)

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPLinTransCopyOpaqueImage_c, (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch))
{

	destlen -= 3;
	while(destlen > 0)
	{
		dest[0] = 0xff000000 | *( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16)*srcpitch) + (sx>>16));
		sx += stepx;
		sy += stepy;
		dest[1] = 0xff000000 | *( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16)*srcpitch) + (sx>>16));
		sx += stepx;
		sy += stepy;
		dest[2] = 0xff000000 | *( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16)*srcpitch) + (sx>>16));
		sx += stepx;
		sy += stepy;
		dest[3] = 0xff000000 | *( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16)*srcpitch) + (sx>>16));
		sx += stepx;
		sy += stepy;
		dest += 4;
		destlen -= 4;
	}

	destlen += 3;

	while(destlen > 0)
	{
		dest[0] = 0xff000000 | *( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16)*srcpitch) + (sx>>16));
		sx += stepx;
		sy += stepy;
		dest ++;
		destlen --;
	}
}

EOF

;#-----------------------------------------------------------------


print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPLinTransConstAlphaBlend_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa))
{
	tjs_uint32 d1, s, d;
EOF


$content = <<EOF;
{
	s = *( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16)*srcpitch) + (sx>>16));
	sx += stepx;
	sy += stepy;
	d = *dest;
	d1 = d & 0xff00ff;
	d1 = (d1 + (((s & 0xff00ff) - d1) * opa >> 8)) & 0xff00ff;
	d &= 0xff00;
	s &= 0xff00;
	*dest = d1 | ((d + ((s - d) * opa >> 8)) & 0xff00);
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPInterpLinTransConstAlphaBlend_c, (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa))
{
	/* bilinear interpolation version */
	/* note that srcpitch unit is in byte */
	opa += opa >> 7; /* adjust opacity */

	destlen -= 1;
	while(destlen > 0)
	{
		int blend_x, blend_y;
		const tjs_uint32 *p0, *p1;

		blend_x = (sx & 0xffff) >> 8;
		blend_x += blend_x >> 7;
		blend_y = (sy & 0xffff) >> 8;
		blend_y += blend_y >> 7;
		p0 = (const tjs_uint32*)((const tjs_uint8*)src + ((sy>>16)  )*srcpitch) + (sx>>16);
		p1 = (const tjs_uint32*)((const tjs_uint8*)p0 + srcpitch);
		dest[0] = TVPBlendARGB(dest[0], TVPBlendARGB(
			TVPBlendARGB(p0[0], p0[1], blend_x),
			TVPBlendARGB(p1[0], p1[1], blend_x),
				blend_y), opa);
		sx += stepx, sy += stepy;

		blend_x = (sx & 0xffff) >> 8;
		blend_x += blend_x >> 7;
		blend_y = (sy & 0xffff) >> 8;
		blend_y += blend_y >> 7;
		p0 = (const tjs_uint32*)((const tjs_uint8*)src + ((sy>>16)  )*srcpitch) + (sx>>16);
		p1 = (const tjs_uint32*)((const tjs_uint8*)p0 + srcpitch);
		dest[1] = TVPBlendARGB(dest[1], TVPBlendARGB(
			TVPBlendARGB(p0[0], p0[1], blend_x),
			TVPBlendARGB(p1[0], p1[1], blend_x),
				blend_y), opa);
		sx += stepx, sy += stepy;

		dest += 2;
		destlen -= 2;
	}

	destlen += 1;

	while(destlen > 0)
	{
		int blend_x, blend_y;
		const tjs_uint32 *p0, *p1;

		blend_x = (sx & 0xffff) >> 8;
		blend_x += blend_x >> 7;
		blend_y = (sy & 0xffff) >> 8;
		blend_y += blend_y >> 7;
		p0 = (const tjs_uint32*)((const tjs_uint8*)src + ((sy>>16)  )*srcpitch) + (sx>>16);
		p1 = (const tjs_uint32*)((const tjs_uint8*)p0 + srcpitch);
		dest[0] = TVPBlendARGB(dest[0], TVPBlendARGB(
			TVPBlendARGB(p0[0], p0[1], blend_x),
			TVPBlendARGB(p1[0], p1[1], blend_x),
				blend_y), opa);
		sx += stepx, sy += stepy;

		dest ++;
		destlen --;
	}
}

EOF


;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPLinTransConstAlphaBlend_HDA_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa))
{
	tjs_uint32 d1, s, d;
EOF


$content = <<EOF;
{
	s = *( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16)*srcpitch) + (sx>>16));
	sx += stepx;
	sy += stepy;
	d = *dest;
	d1 = d & 0xff00ff;
	d1 = ((d1 + (((s & 0xff00ff) - d1) * opa >> 8)) & 0xff00ff) + (d & 0xff000000);
	d &= 0xff00;
	s &= 0xff00;
	*dest = d1 | ((d + ((s - d) * opa >> 8)) & 0xff00);
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPLinTransConstAlphaBlend_d_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa))
{
	tjs_uint32 d1, s, d, addr;
	tjs_int alpha;
	opa <<= 8;
EOF

$content = <<EOF;
{
	s = *( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16)*srcpitch) + (sx>>16));
	sx += stepx;
	sy += stepy;
	d = *dest;
	addr = opa + (d>>24);
	alpha = TVPOpacityOnOpacityTable[addr];
	d1 = d & 0xff00ff;
	d1 = ((d1 + (((s & 0xff00ff) - d1) * alpha >> 8)) & 0xff00ff) +
		(TVPNegativeMulTable[addr]<<24);
	d &= 0xff00;
	s &= 0xff00;
	*dest = d1 | ((d + ((s - d) * alpha >> 8)) & 0xff00);
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPLinTransConstAlphaBlend_a_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa))
{
	opa <<= 24;
EOF

$content = <<EOF;
{
	*dest = TVPAddAlphaBlend_a_a(*dest, 
		((*( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16)*srcpitch) + (sx>>16))) & 0xffffff) | opa);
	sx += stepx;
	sy += stepy;
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF







;#-----------------------------------------------------------------
;# constant ratio alpha blending ( separated destination )
;#-----------------------------------------------------------------


print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPConstAlphaBlend_SD_c, (tjs_uint32 *dest, const tjs_uint32 *src1, const tjs_uint32 *src2, tjs_int len, tjs_int opa))
{
	tjs_uint32 s1_, s1, s2;
EOF


$content = <<EOF;
{
	s1 = *src1;
	s2 = *src2;
	s1_ = s1 & 0xff00ff;
	s1_ = (s1_ + (((s2 & 0xff00ff) - s1_) * opa >> 8)) & 0xff00ff;
	src1++;
	src2++;
	s2 &= 0xff00;
	s1 &= 0xff00;
	*dest = s1_ | ((s1 + ((s2 - s1) * opa >> 8)) & 0xff00);
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPConstAlphaBlend_SD_a_c, (tjs_uint32 *dest, const tjs_uint32 *src1, const tjs_uint32 *src2, tjs_int len, tjs_int opa))
{
EOF


$content = <<EOF;
	dest[{ofs}] = TVPBlendARGB(src1[{ofs}], src2[{ofs}], opa);
EOF

&loop_unroll_c_2($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------



print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPConstAlphaBlend_SD_d_c, (tjs_uint32 *dest, const tjs_uint32 *src1, const tjs_uint32 *src2, tjs_int len, tjs_int opa))
{/* alpha vs alpha, destination has alpha */
	tjs_uint32 s1_, s2, s1, addr;
	tjs_uint32 a1, a2;
	tjs_int alpha;
	tjs_int iopa;
	if(opa > 127) opa ++; /* adjust for error */
	iopa = 256 - opa;
	/* blending function for 'alpha-per-pixel enabled alpha blending' is complex. */
EOF

$content = <<EOF;
{
	s1 = *src1;
	s2 = *src2;
	a1 = s1 >> 24;
	a2 = s2 >> 24;
	addr = (a2*opa & 0xff00) + (a1*iopa >> 8);
	alpha = TVPOpacityOnOpacityTable[addr];
	s1_ = s1 & 0xff00ff;
	s1_ = ((s1_ + (((s2 & 0xff00ff) - s1_) * alpha >> 8)) & 0xff00ff);
	src1++;
	src2++;
	s1 &= 0xff00;
	s2 &= 0xff00;
	s1_ |= (a1 + ((a2 - a1)*opa >> 8)) << 24;
	*dest = s1_ | ((s1 + ((s2 - s1) * alpha >> 8)) & 0xff00);
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF



;#-----------------------------------------------------------------
;# blending function for universal transition
;#-----------------------------------------------------------------

;# Note: blending incompatible alpha type (such as additive-alpha vs alpha)
;#       is not yet implemented.


print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPInitUnivTransBlendTable_c, (tjs_uint32 *table, tjs_int phase, tjs_int vague))
{
	tjs_int i;
	tjs_int phasemax = phase;
	phase -= vague;
	for(i = 0; i<256; i++)
	{
		if(i<phase) table[i] = 255;
		else if(i>=phasemax) table[i] = 0;
		else
		{
			int tmp = (255-(( i - phase )*255 / vague));
			if(tmp<0) tmp = 0;
			if(tmp>255) tmp = 255;
			table[i] = tmp;
		}
	}
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPInitUnivTransBlendTable_d_c, (tjs_uint32 *table, tjs_int phase, tjs_int vague))
{
	/* alias to TVPInitUnivTransBlendTable_c */
	TVPInitUnivTransBlendTable_c(table, phase, vague);
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPInitUnivTransBlendTable_a_c, (tjs_uint32 *table, tjs_int phase, tjs_int vague))
{
	/* alias to TVPInitUnivTransBlendTable_c */
	TVPInitUnivTransBlendTable_c(table, phase, vague);
}

EOF


;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPUnivTransBlend_c, (tjs_uint32 *dest, const tjs_uint32 *src1, const tjs_uint32 *src2, const tjs_uint8 *rule, const tjs_uint32 *table, tjs_int len))
{
	tjs_uint32 s1_, s1, s2;
	tjs_int opa;
EOF


$content = <<EOF;
{
	opa = table[*rule];
	rule++;
	s1 = *src1;
	src1++;
	s2 = *src2;
	src2++;
	s1_ = s1 & 0xff00ff;
	s1_ = (s1_ + (((s2 & 0xff00ff) - s1_) * opa >> 8)) & 0xff00ff;
	s2 &= 0xff00;
	s1 &= 0xff00;
	*dest = s1_ | ((s1 + ((s2 - s1) * opa >> 8)) & 0xff00);
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPUnivTransBlend_switch_c, (tjs_uint32 *dest, const tjs_uint32 *src1, const tjs_uint32 *src2, const tjs_uint8 *rule, const tjs_uint32 *table, tjs_int len, tjs_int src1lv, tjs_int src2lv))
{
	tjs_uint32 s1_, s1, s2;
	tjs_int opa;
EOF


$content = <<EOF;
{
	opa = *rule;
	if(opa >= src1lv)
	{
		*dest = *src1;
		rule++; src1++; src2++; dest++;
	}
	else if(opa < src2lv)
	{
		*dest = *src2;
		rule++; src1++; src2++; dest++;
	}
	else
	{
		opa = table[opa];
		rule++;
		s1 = *src1;
		src1++;
		s2 = *src2;
		src2++;
		s1_ = s1 & 0xff00ff;
		s1_ = (s1_ + (((s2 & 0xff00ff) - s1_) * opa >> 8)) & 0xff00ff;
		s2 &= 0xff00;
		s1 &= 0xff00;
		*dest = s1_ | ((s1 + ((s2 - s1) * opa >> 8)) & 0xff00);
		dest++;
	}
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPUnivTransBlend_d_c, (tjs_uint32 *dest, const tjs_uint32 *src1, const tjs_uint32 *src2, const tjs_uint8 *rule, const tjs_uint32 *table, tjs_int len))
{
	tjs_uint32 s1_, s2, s1, addr;
	tjs_uint32 a1, a2;
	tjs_int alpha;
	tjs_int opa;
EOF

$content = <<EOF;
{
	opa = table[*rule];
	rule++;
	s1 = *src1;
	s2 = *src2;
	a1 = s1 >> 24;
	a2 = s2 >> 24;
	addr = (a2*opa & 0xff00) + (a1*(256-opa) >> 8);
	alpha = TVPOpacityOnOpacityTable[addr];
	s1_ = s1 & 0xff00ff;
	s1_ = ((s1_ + (((s2 & 0xff00ff) - s1_) * alpha >> 8)) & 0xff00ff);
	src1++;
	src2++;
	s1 &= 0xff00;
	s2 &= 0xff00;
	s1_ |= (a1 + ((a2 - a1)*opa >> 8)) << 24;
	*dest = s1_ | ((s1 + ((s2 - s1) * alpha >> 8)) & 0xff00);
	dest++;
}
EOF


&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------


print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPUnivTransBlend_switch_d_c, (tjs_uint32 *dest, const tjs_uint32 *src1, const tjs_uint32 *src2, const tjs_uint8 *rule, const tjs_uint32 *table, tjs_int len, tjs_int src1lv, tjs_int src2lv))
{
	tjs_uint32 s1_, s2, s1, addr;
	tjs_uint32 a1, a2;
	tjs_int alpha;
	tjs_int opa;
EOF

$content = <<EOF;
{
	opa = *rule;
	if(opa >= src1lv)
	{
		*dest = *src1;
		rule++; src1++; src2++; dest++;
	}
	else if(opa < src2lv)
	{
		*dest = *src2;
		rule++; src1++; src2++; dest++;
	}
	else
	{
		opa = table[opa];
		rule++;
		s1 = *src1;
		s2 = *src2;
		a1 = s1 >> 24;
		a2 = s2 >> 24;
		addr = (a2*opa & 0xff00) + (a1*(256-opa) >> 8);
		alpha = TVPOpacityOnOpacityTable[addr];
		s1_ = s1 & 0xff00ff;
		s1_ = ((s1_ + (((s2 & 0xff00ff) - s1_) * alpha >> 8)) & 0xff00ff) +
			(TVPNegativeMulTable[addr]<<24);
		src1++;
		src2++;
		s1 &= 0xff00;
		s2 &= 0xff00;
		*dest = s1_ | ((s1 + ((s2 - s1) * alpha >> 8)) & 0xff00);
		dest++;
	}
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF


;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPUnivTransBlend_a_c, (tjs_uint32 *dest, const tjs_uint32 *src1, const tjs_uint32 *src2, const tjs_uint8 *rule, const tjs_uint32 *table, tjs_int len))
{
EOF

$content = <<EOF;
	dest[{ofs}] = TVPBlendARGB(src1[{ofs}], src2[{ofs}], table[rule[{ofs}]]);
EOF


&loop_unroll_c_2($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------


print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPUnivTransBlend_switch_a_c, (tjs_uint32 *dest, const tjs_uint32 *src1, const tjs_uint32 *src2, const tjs_uint8 *rule, const tjs_uint32 *table, tjs_int len, tjs_int src1lv, tjs_int src2lv))
{
	tjs_int opa;
EOF

$content = <<EOF;
	opa = rule[{ofs}];
	if(opa >= src1lv)
		dest[{ofs}] = src1[{ofs}];
	else if(opa < src2lv)
		dest[{ofs}] = src2[{ofs}];
	else
		dest[{ofs}] = TVPBlendARGB(src1[{ofs}], src2[{ofs}], table[opa]);
EOF

&loop_unroll_c_2($content, 'len', 4);

print FC <<EOF;
}

EOF



;#-----------------------------------------------------------------
;# alpha color mapping
;#-----------------------------------------------------------------


sub alpha_color_map
{
	local($bit, $namesuffix);
	$bit = $_[0];
	$namesuffix = $_[1];

print FC <<EOF;

/*export*/
TVP_GL_FUNC_DECL(void, TVPApplyColorMap${namesuffix}_c, (tjs_uint32 *dest, const tjs_uint8 *src, tjs_int len, tjs_uint32 color))
{
	tjs_uint32 d1, d, sopa;
	tjs_uint32 c1 = color & 0xff00ff;
	color = color & 0x00ff00;
EOF


$content = <<EOF;
{
	d = *dest;
	sopa = *src;
	d1 = d & 0xff00ff;
	d1 = ((d1 + ((c1 - d1) * sopa >> $bit)) & 0xff00ff);
	d &= 0xff00;
	*dest = d1 | ((d + ((color - d) * sopa >> $bit)) & 0x00ff00);
	src++;
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF


print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPApplyColorMap${namesuffix}_o_c, (tjs_uint32 *dest, const tjs_uint8 *src, tjs_int len, tjs_uint32 color, tjs_int opa))
{
	tjs_uint32 d1, d, sopa;
	tjs_uint32 c1 = color & 0xff00ff;
	color = color & 0x00ff00;
EOF


$content = <<EOF;
{
	d = *dest;
	sopa = (*src * opa) >> 8;
	d1 = d & 0xff00ff;
	d1 = ((d1 + ((c1 - d1) * sopa >> $bit)) & 0xff00ff);
	d &= 0x00ff00;
	*dest = d1 | ((d + ((color - d) * sopa >> $bit)) & 0x00ff00);
	src++;
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF




}

&alpha_color_map(8, '');
&alpha_color_map(6, '65');

;#-----------------------------------------------------------------

sub alpha_color_map_hda
{
	local($bit, $namesuffix);
	$bit = $_[0];
	$namesuffix = $_[1];

print FC <<EOF;

/*export*/
TVP_GL_FUNC_DECL(void, TVPApplyColorMap${namesuffix}_HDA_c, (tjs_uint32 *dest, const tjs_uint8 *src, tjs_int len, tjs_uint32 color))
{
	tjs_uint32 d1, d, sopa;
	tjs_uint32 c1 = color & 0xff00ff;
	color = color & 0x00ff00;
EOF


$content = <<EOF;
{
	d = *dest;
	sopa = *src;
	d1 = d & 0xff00ff;
	d1 = ((d1 + ((c1 - d1) * sopa >> $bit)) & 0xff00ff) + (d & 0xff000000);
	d &= 0xff00;
	*dest = d1 | ((d + ((color - d) * sopa >> $bit)) & 0x00ff00);
	src++;
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF


print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPApplyColorMap${namesuffix}_HDA_o_c, (tjs_uint32 *dest, const tjs_uint8 *src, tjs_int len, tjs_uint32 color, tjs_int opa))
{
	tjs_uint32 d1, d, sopa;
	tjs_uint32 c1 = color & 0xff00ff;
	color = color & 0x00ff00;
EOF


$content = <<EOF;
{
	d = *dest;
	sopa = (*src * opa) >> 8;
	d1 = d & 0xff00ff;
	d1 = ((d1 + ((c1 - d1) * sopa >> $bit)) & 0xff00ff) + (d & 0xff000000);
	d &= 0x00ff00;
	*dest = d1 | ((d + ((color - d) * sopa >> $bit)) & 0x00ff00);
	src++;
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF




}

&alpha_color_map_hda(8, '');
&alpha_color_map_hda(6, '65');

;#-----------------------------------------------------------------

sub alpha_color_map_d
{
	local($namesuffix);
	$namesuffix = $_[0];


print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPApplyColorMap${namesuffix}_d_c, (tjs_uint32 *dest, const tjs_uint8 *src, tjs_int len, tjs_uint32 color))
{
	tjs_uint32 d1, d, sopa, addr, destalpha;
	tjs_uint32 c1 = color & 0xff00ff;
	color = color & 0x00ff00;
EOF


$content = <<EOF;
{
	d = *dest;
	addr = (*src<<8) + (d>>24);
	destalpha = TVPNegativeMulTable${namesuffix}[addr]<<24;
	sopa = TVPOpacityOnOpacityTable${namesuffix}[addr];
	d1 = d & 0xff00ff;
	d1 = (d1 + ((c1 - d1) * sopa >> 8)) & 0xff00ff;
	d &= 0x00ff00;
	*dest = d1 + ((d + ((color - d) * sopa >> 8)) & 0x00ff00) + destalpha;
	src++;
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

}

&alpha_color_map_d('');
&alpha_color_map_d('65');


;#-----------------------------------------------------------------


sub alpha_color_map_a
{
	local($namesuffix, $shift);
	$namesuffix = $_[0];
	$shift = $_[1];


print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPApplyColorMap${namesuffix}_a_c, (tjs_uint32 *dest, const tjs_uint8 *src, tjs_int len, tjs_uint32 color))
{
	tjs_uint32 c1 = color & 0xff00ff;
	color = color & 0x00ff00;
EOF


$content = <<EOF;
{
	tjs_int s_tmp = *src;
	tjs_uint32 tmp =
		((s_tmp * (c1    & 0xff00ff) >> $shift) & 0xff00ff) + 
		((s_tmp * (color & 0x00ff00) >> $shift) & 0x00ff00);
	s_tmp <<= (8 - $shift);
	s_tmp -= (s_tmp >> 8); /* adjust alpha */
	*dest = TVPAddAlphaBlend_a_ca(*dest, s_tmp, s_tmp ^ 0xff, tmp);
	src++;
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

}

&alpha_color_map_a('', 8);
&alpha_color_map_a('65', 6);


;#-----------------------------------------------------------------

sub alpha_color_map_do
{
	local($namesuffix);
	$namesuffix = $_[0];


print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPApplyColorMap${namesuffix}_do_c, (tjs_uint32 *dest, const tjs_uint8 *src, tjs_int len, tjs_uint32 color, tjs_int opa))
{
	tjs_uint32 d1, d, sopa, addr, destalpha;
	tjs_uint32 c1 = color & 0xff00ff;
	color = color & 0x00ff00;
EOF


$content = <<EOF;
{
	d = *dest;
	addr = ((*src * opa) & 0xff00) + (d>>24);
	destalpha = TVPNegativeMulTable${namesuffix}[addr]<<24;
	sopa = TVPOpacityOnOpacityTable${namesuffix}[addr];
	d1 = d & 0xff00ff;
	d1 = (d1 + ((c1 - d1) * sopa >> 8)) & 0xff00ff;
	d &= 0x00ff00;
	*dest = d1 + ((d + ((color - d) * sopa >> 8)) & 0x00ff00) + destalpha;
	src++;
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

}


&alpha_color_map_do('');
&alpha_color_map_do('65');

;#-----------------------------------------------------------------


sub alpha_color_map_ao
{
	local($namesuffix, $shift);
	$namesuffix = $_[0];
	$shift = $_[1];


print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPApplyColorMap${namesuffix}_ao_c, (tjs_uint32 *dest, const tjs_uint8 *src, tjs_int len, tjs_uint32 color, tjs_int opa))
{
	tjs_uint32 c1 = color & 0xff00ff;
	color = color & 0x00ff00;
EOF


$content = <<EOF;
{
	tjs_int s_tmp = (*src * opa) >> 8;
	tjs_uint32 tmp =
		((s_tmp * (c1    & 0xff00ff) >> $shift) & 0xff00ff) + 
		((s_tmp * (color & 0x00ff00) >> $shift) & 0x00ff00);
	s_tmp <<= (8 - $shift);
	s_tmp -= (s_tmp >> 8); /* adjust alpha */
	*dest = TVPAddAlphaBlend_a_ca(*dest, s_tmp, s_tmp ^ 0xff, tmp);
	src++;
	dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

}

&alpha_color_map_ao('', 8);
&alpha_color_map_ao('65', 6);

;#-----------------------------------------------------------------
;# constant ratio constant color alpha blending
;#-----------------------------------------------------------------


print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPConstColorAlphaBlend_c, (tjs_uint32 *dest, tjs_int len, tjs_uint32 color, tjs_int opa))
{
	/* this function always holds desitination alpha channel */
	tjs_uint32 s1, d;
	s1 = (color & 0xff00ff)*opa ;
	color = (color & 0xff00)*opa ;
	opa = 255 - opa;
EOF


$content = <<EOF;
{
	d = *dest;
	*dest = (d & 0xff000000) + ((((d & 0xff00ff) * opa + s1) >> 8) & 0xff00ff) +
		((((d&0xff00) * opa + color) >> 8) & 0xff00);
	dest ++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------


print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPConstColorAlphaBlend_d_c, (tjs_uint32 *dest, tjs_int len, tjs_uint32 color, tjs_int opa))
{
	tjs_uint32 d1, s1, d, dopa;
	tjs_int alpha;
	s1 = color & 0xff00ff;
	color = color & 0xff00;
EOF

$content = <<EOF;
{
	d = *dest;
	dopa = d>>24;
	alpha = TVPOpacityOnOpacityTable[dopa + (opa<<8)];
	d1 = d & 0xff00ff;
	d1 = ((d1 + ((s1 - d1) * alpha >> 8)) & 0xff00ff) |
		((255-((255-dopa)*(255-opa)>>8)) << 24);
	d &= 0xff00;
	*dest = d1 | ((d + ((color - d) * alpha >> 8)) & 0xff00);
	dest ++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPConstColorAlphaBlend_a_c, (tjs_uint32 *dest, tjs_int len, tjs_uint32 color, tjs_int opa))
{
	tjs_uint32 src = TVPMulColor(color & 0xffffff, opa);
	tjs_uint32 opa_inv = opa ^ 0xff;
EOF

$content = <<EOF;
	dest[{ofs}] = TVPAddAlphaBlend_a_ca(dest[{ofs}], opa, opa_inv, src);
EOF

&loop_unroll_c_2($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------
;# opacity removal
;#-----------------------------------------------------------------

;# ??? where are these used in ?


print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPRemoveConstOpacity_c, (tjs_uint32 *dest, tjs_int len, tjs_int strength))
{
	tjs_uint32 d, d2;

	strength = 255 - strength;

EOF


$content = <<EOF;
	d = dest[{ofs}];;
	dest[{ofs}] = (d & 0xffffff) + ( (((d>>24)*strength) << 16) & 0xff000000);;
EOF

$content2 = <<EOF;
	d2 = dest[{ofs}];;
	dest[{ofs}] = (d2 & 0xffffff) + ( (((d2>>24)*strength) << 16) & 0xff000000);;
EOF


&loop_unroll_c_int_2($content, $content2, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPRemoveOpacity_c, (tjs_uint32 *dest, const tjs_uint8 *src, tjs_int len))
{
	tjs_uint32 d, d2;
EOF


$content = <<EOF;
	d = dest[{ofs}];;
	dest[{ofs}] = (d & 0xffffff) + ( (((d>>24) * (255-src[{ofs}])) << 16) & 0xff000000);;
EOF
$content2 = <<EOF;
	d2 = dest[{ofs}];;
	dest[{ofs}] = (d2 & 0xffffff) + ( (((d2>>24) * (255-src[{ofs}])) << 16) & 0xff000000);;
EOF

&loop_unroll_c_int_2($content, $content2, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPRemoveOpacity_o_c, (tjs_uint32 *dest, const tjs_uint8 *src, tjs_int len, tjs_int strength))
{
	tjs_uint32 d, d2;

	if(strength > 127) strength ++; /* adjust for error */
EOF

$content = <<EOF;
	d = dest[{ofs}];;
	dest[{ofs}] = (d & 0xffffff) + ( (((d>>24) * (65535-src[{ofs}]*strength )) << 8) & 0xff000000);;
EOF
$content2 = <<EOF;
	d2 = dest[{ofs}];;
	dest[{ofs}] = (d2 & 0xffffff) + ( (((d2>>24) * (65535-src[{ofs}]*strength )) << 8) & 0xff000000);;
EOF

&loop_unroll_c_int_2($content, $content2, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPRemoveOpacity65_c, (tjs_uint32 *dest, const tjs_uint8 *src, tjs_int len))
{
	tjs_uint32 d, d2;
EOF


$content = <<EOF;
	d = dest[{ofs}];;
	dest[{ofs}] = (d & 0xffffff) + ( (((d>>24) * (64-src[{ofs}])) << 18) & 0xff000000);;
EOF
$content2 = <<EOF;
	d2 = dest[{ofs}];;
	dest[{ofs}] = (d2 & 0xffffff) + ( (((d2>>24) * (64-src[{ofs}])) << 18) & 0xff000000);;
EOF

&loop_unroll_c_int_2($content, $content2, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPRemoveOpacity65_o_c, (tjs_uint32 *dest, const tjs_uint8 *src, tjs_int len, tjs_int strength))
{
	tjs_uint32 d, d2;

	if(strength > 127) strength ++; /* adjust for error */
EOF

$content = <<EOF;
	d = dest[{ofs}];;
	dest[{ofs}] = (d & 0xffffff) + ( (((d>>24) * (16384-src[{ofs}]*strength )) << 10) & 0xff000000);;
EOF
$content2 = <<EOF;
	d2 = dest[{ofs}];;
	dest[{ofs}] = (d2 & 0xffffff) + ( (((d2>>24) * (16384-src[{ofs}]*strength )) << 10) & 0xff000000);;
EOF

&loop_unroll_c_int_2($content, $content2, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------


print FC <<EOF;
/*not export*/
TVP_GL_FUNC_DECL(void, TVPRemoveAdditiveConstOpacity_c, (tjs_uint32 *dest, tjs_int len, tjs_int strength))
{/*YET NOT IMPLEMENTED*//*MAY LOOSE ADDITIVE STUFF*/
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*not export*/
TVP_GL_FUNC_DECL(void, TVPRemoveAdditiveOpacity_c, (tjs_uint32 *dest, const tjs_uint8 *src, tjs_int len))
{/*YET NOT IMPLEMENTED*//*MAY LOOSE ADDITIVE STUFF*/
}

EOF

;#-----------------------------------------------------------------


print FC <<EOF;
/*not export*/
TVP_GL_FUNC_DECL(void, TVPRemoveAdditiveOpacity_o_c, (tjs_uint32 *dest, const tjs_uint8 *src, tjs_int len, tjs_int strength))
{/*YET NOT IMPLEMENTED*//*MAY LOOSE ADDITIVE STUFF*/
}

EOF


;#-----------------------------------------------------------------

print FC <<EOF;
/*not export*/
TVP_GL_FUNC_DECL(void, TVPRemoveAdditiveOpacity65_c, (tjs_uint32 *dest, const tjs_uint8 *src, tjs_int len))
{/*YET NOT IMPLEMENTED*//*MAY LOOSE ADDITIVE STUFF*/
}

EOF


;#-----------------------------------------------------------------

print FC <<EOF;
/*not export*/
TVP_GL_FUNC_DECL(void, TVPRemoveAdditiveOpacity65_o_c, (tjs_uint32 *dest, const tjs_uint8 *src, tjs_int len, tjs_int strength))
{/*YET NOT IMPLEMENTED*//*MAY LOOSE ADDITIVE STUFF*/
}

EOF



;#-----------------------------------------------------------------
;# pixel addition with saturation
;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPAddBlend_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
register tjs_uint32 tmp;
EOF


$content = <<EOF;
{
tmp = (  ( *src & *dest ) + ( ((*src^*dest)>>1) & 0x7f7f7f7f)  ) & 0x80808080;
tmp = (tmp<<1) - (tmp>>7);
*dest= (*src + *dest - tmp) | tmp;
dest++;
src++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF


;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPAddBlend_HDA_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
register tjs_uint32 tmp;
EOF


$content = <<EOF;
{
tmp = (  ( *src & *dest ) + ( ((*src^*dest)>>1) & 0x7f7f7f7f)  ) & 0x80808080;
tmp = (tmp<<1) - (tmp>>7);
*dest= (((*src + *dest - tmp) | tmp) & 0xffffff) | (*dest & 0xff000000) ;
dest++;
src++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------


print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPAddBlend_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
register tjs_uint32 s;
register tjs_uint32 tmp;
EOF


$content = <<EOF;
{
s = ( ((*src&0x00ff00)  * opa >> 8)&0x00ff00) +
	(( (*src&0xff00ff) * opa >> 8)&0xff00ff);
tmp = (  ( s & *dest ) + ( ((s^*dest)>>1) & 0x7f7f7f7f)  ) & 0x80808080;
src++;
tmp = (tmp<<1) - (tmp>>7);
*dest= (s + *dest - tmp) | tmp;
dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF


;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPAddBlend_HDA_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
register tjs_uint32 s;
register tjs_uint32 tmp;
EOF


$content = <<EOF;
{
s = ( ((*src&0x00ff00)  * opa >> 8)&0x00ff00) +
	(( (*src&0xff00ff) * opa >> 8)&0xff00ff);
tmp = (  ( s & *dest ) + ( ((s^*dest)>>1) & 0x7f7f7f7f)  ) & 0x80808080;
src++;
tmp = (tmp<<1) - (tmp>>7);
*dest= (((s + *dest - tmp) | tmp) & 0xffffff) + (*dest & 0xff000000) ;
dest++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------
;# pixel subtract with saturation
;#-----------------------------------------------------------------

;# thanks Mr. Sugi

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPSubBlend_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
register tjs_uint32 tmp;
EOF


$content = <<EOF;
{
tmp = (  ( *src & *dest ) + ( ((*src ^ *dest)>>1) & 0x7f7f7f7f)  ) & 0x80808080;
tmp = (tmp << 1) - (tmp >> 7);
*dest = (*src + *dest - tmp) & tmp;
dest++;
src++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPSubBlend_HDA_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
register tjs_uint32 tmp, s;
EOF


$content = <<EOF;
{
s = *src | 0xff000000;
tmp = (  ( s & *dest ) + ( ((s ^ *dest)>>1) & 0x7f7f7f7f)  ) & 0x80808080;
tmp = (tmp << 1) - (tmp >> 7);
*dest = (s + *dest - tmp) & tmp;
dest++;
src++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPSubBlend_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
register tjs_uint32 s;
register tjs_uint32 tmp;
EOF


$content = <<EOF;
{
s = ~*src;
s = ~ (( ((s&0x00ff00)  * opa >> 8)&0x00ff00) +
	(( (s&0xff00ff) * opa >> 8)&0xff00ff) );
tmp = (  ( s & *dest ) + ( ((s ^ *dest)>>1) & 0x7f7f7f7f)  ) & 0x80808080;
tmp = (tmp << 1) - (tmp >> 7);
*dest = (s + *dest - tmp) & tmp;
dest++;
src++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPSubBlend_HDA_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
register tjs_uint32 s/*, d*/;
register tjs_uint32 tmp;
EOF


$content = <<EOF;
{
s = ~*src;
s = 0xff000000 | ~ (( ((s&0x00ff00)  * opa >> 8)&0x00ff00) +
	(( (s&0xff00ff) * opa >> 8)&0xff00ff) );
tmp = (  ( s & *dest ) + ( ((s ^ *dest)>>1) & 0x7f7f7f7f)  ) & 0x80808080;
tmp = (tmp << 1) - (tmp >> 7);
*dest = (s + *dest - tmp) & tmp;
dest++;
src++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------
;# pixel multiplactive blend
;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPMulBlend_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
register tjs_uint32 tmp;
EOF


$content = <<EOF;
{
tmp  = (*dest & 0xff) * (*src & 0xff) & 0xff00;
tmp |= ((*dest & 0xff00) >> 8) * (*src & 0xff00) & 0xff0000;
tmp |= ((*dest & 0xff0000) >> 16) * (*src & 0xff0000) & 0xff000000;
tmp >>= 8;
*dest = tmp;
dest++;
src++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPMulBlend_HDA_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
register tjs_uint32 tmp;
EOF


$content = <<EOF;
{
tmp  = (*dest & 0xff) * (*src & 0xff) & 0xff00;
tmp |= ((*dest & 0xff00) >> 8) * (*src & 0xff00) & 0xff0000;
tmp |= ((*dest & 0xff0000) >> 16) * (*src & 0xff0000) & 0xff000000;
tmp >>= 8;
*dest = tmp + (*dest & 0xff000000);
dest++;
src++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------


print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPMulBlend_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
register tjs_uint32 s;
register tjs_uint32 tmp;
EOF


$content = <<EOF;
{
s = ~*src;
s = ~( ( ((s&0x00ff00)  * opa >> 8)&0x00ff00) +
	(( (s&0xff00ff) * opa >> 8)&0xff00ff));
tmp  = (*dest & 0xff) * (s & 0xff) & 0xff00;
tmp |= ((*dest & 0xff00) >> 8) * (s & 0xff00) & 0xff0000;
tmp |= ((*dest & 0xff0000) >> 16) * (s & 0xff0000) & 0xff000000;
tmp >>= 8;
*dest = tmp;
dest++;
src++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------


print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPMulBlend_HDA_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
register tjs_uint32 s;
register tjs_uint32 tmp;
EOF


$content = <<EOF;
{
s = ~*src;
s = ~( ( ((s&0x00ff00)  * opa >> 8)&0x00ff00) +
	(( (s&0xff00ff) * opa >> 8)&0xff00ff));
tmp  = (*dest & 0xff) * (s & 0xff) & 0xff00;
tmp |= ((*dest & 0xff00) >> 8) * (s & 0xff00) & 0xff0000;
tmp |= ((*dest & 0xff0000) >> 16) * (s & 0xff0000) & 0xff000000;
tmp >>= 8;
*dest = tmp + (*dest & 0xff000000);
dest++;
src++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------
;# "color dodge" blend
;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPColorDodgeBlend_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
register tjs_uint32 tmp, tmp2, tmp3;
EOF


$content = <<EOF;
{
tmp2 = ~*src;
tmp = (*dest & 0xff) * TVPRecipTable256[tmp2 & 0xff] >> 8;
tmp3 = (tmp | ((tjs_int32)~(tmp - 0x100) >> 31)) & 0xff;
tmp = ((*dest & 0xff00)>>8) * TVPRecipTable256[(tmp2 & 0xff00)>>8];
tmp3 |= (tmp | ((tjs_int32)~(tmp - 0x10000) >> 31)) & 0xff00;
tmp = ((*dest & 0xff0000)>>16) * TVPRecipTable256[(tmp2 & 0xff0000)>>16];
tmp3 |= ((tmp | ((tjs_int32)~(tmp - 0x10000) >> 31)) & 0xff00 ) << 8;
*dest= tmp3;
dest++;
src++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------


print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPColorDodgeBlend_HDA_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
register tjs_uint32 tmp, tmp2, tmp3;
EOF


$content = <<EOF;
{
tmp2 = ~*src;
tmp = (*dest & 0xff) * TVPRecipTable256[tmp2 & 0xff] >> 8;
tmp3 = (tmp | ((tjs_int32)~(tmp - 0x100) >> 31)) & 0xff;
tmp = ((*dest & 0xff00)>>8) * TVPRecipTable256[(tmp2 & 0xff00)>>8];
tmp3 |= (tmp | ((tjs_int32)~(tmp - 0x10000) >> 31)) & 0xff00;
tmp = ((*dest & 0xff0000)>>16) * TVPRecipTable256[(tmp2 & 0xff0000)>>16];
tmp3 |= ((tmp | ((tjs_int32)~(tmp - 0x10000) >> 31)) & 0xff00 ) << 8;
*dest= tmp3 + (*dest & 0xff000000);
dest++;
src++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPColorDodgeBlend_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
register tjs_uint32 tmp, tmp2, tmp3;
EOF


$content = <<EOF;
{
tmp2 = ~ (( ((*src&0x00ff00)  * opa >> 8)&0x00ff00) +
	(( (*src&0xff00ff) * opa >> 8)&0xff00ff) );
tmp = (*dest & 0xff) * TVPRecipTable256[tmp2 & 0xff] >> 8;
tmp3 = (tmp | ((tjs_int32)~(tmp - 0x100) >> 31)) & 0xff;
tmp = ((*dest & 0xff00)>>8) * TVPRecipTable256[(tmp2 & 0xff00)>>8];
tmp3 |= (tmp | ((tjs_int32)~(tmp - 0x10000) >> 31)) & 0xff00;
tmp = ((*dest & 0xff0000)>>16) * TVPRecipTable256[(tmp2 & 0xff0000)>>16];
tmp3 |= ((tmp | ((tjs_int32)~(tmp - 0x10000) >> 31)) & 0xff00 ) << 8;
*dest= tmp3;
dest++;
src++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPColorDodgeBlend_HDA_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
register tjs_uint32 tmp, tmp2, tmp3;
EOF


$content = <<EOF;
{
tmp2 = ~ (( ((*src&0x00ff00)  * opa >> 8)&0x00ff00) +
	(( (*src&0xff00ff) * opa >> 8)&0xff00ff) );
tmp = (*dest & 0xff) * TVPRecipTable256[tmp2 & 0xff] >> 8;
tmp3 = (tmp | ((tjs_int32)~(tmp - 0x100) >> 31)) & 0xff;
tmp = ((*dest & 0xff00)>>8) * TVPRecipTable256[(tmp2 & 0xff00)>>8];
tmp3 |= (tmp | ((tjs_int32)~(tmp - 0x10000) >> 31)) & 0xff00;
tmp = ((*dest & 0xff0000)>>16) * TVPRecipTable256[(tmp2 & 0xff0000)>>16];
tmp3 |= ((tmp | ((tjs_int32)~(tmp - 0x10000) >> 31)) & 0xff00 ) << 8;
*dest= tmp3 + (*dest & 0xff000000);
dest++;
src++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------
;# darken blend
;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPDarkenBlend_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
register tjs_uint32 tmp, m_src;
EOF


$content = <<EOF;
{
m_src = ~*src;
tmp = ((m_src & *dest) + (((m_src ^ *dest) >> 1) & 0x7f7f7f7f) ) & 0x80808080;
tmp = (tmp << 1) - (tmp >> 7);
*dest ^= (*dest ^ *src) & tmp;
dest++;
src++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPDarkenBlend_HDA_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
register tjs_uint32 tmp, m_src;
EOF


$content = <<EOF;
{
m_src = ~*src;
tmp = ((m_src & *dest) + (((m_src ^ *dest) >> 1) & 0x7f7f7f7f) ) & 0x80808080;
tmp = (tmp << 1) - (tmp >> 7);
*dest ^= ((*dest ^ *src) & tmp) & 0xffffff;
dest++;
src++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPDarkenBlend_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
register tjs_uint32 tmp, m_src, d1;
EOF


$content = <<EOF;
{
m_src = ~*src;
tmp = ((m_src & *dest) + (((m_src ^ *dest) >> 1) & 0x7f7f7f7f) ) & 0x80808080;
tmp = (tmp << 1) - (tmp >> 7);
tmp = *dest ^ ((*dest ^ *src) & tmp);
d1 = *dest & 0xff00ff;
d1 = (d1 + (((tmp & 0xff00ff) - d1) * opa >> 8)) & 0xff00ff;
m_src = *dest & 0xff00;
tmp &= 0xff00;
*dest = d1 + ((m_src + ((tmp - m_src) * opa >> 8)) & 0xff00);
dest++;
src++;

}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPDarkenBlend_HDA_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
register tjs_uint32 tmp, m_src, d1;
EOF


$content = <<EOF;
{
m_src = ~*src;
tmp = ((m_src & *dest) + (((m_src ^ *dest) >> 1) & 0x7f7f7f7f) ) & 0x80808080;
tmp = (tmp << 1) - (tmp >> 7);
tmp = *dest ^ (((*dest ^ *src) & tmp) & 0xffffff);
d1 = *dest & 0xff00ff;
d1 = ((d1 + (((tmp & 0xff00ff) - d1) * opa >> 8)) & 0xff00ff) + (*dest & 0xff000000); /* hda */
m_src = *dest & 0xff00;
tmp &= 0xff00;
*dest = d1 + ((m_src + ((tmp - m_src) * opa >> 8)) & 0xff00);
dest++;
src++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------
;# lighten blend
;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPLightenBlend_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
register tjs_uint32 tmp, m_dest;
EOF


$content = <<EOF;
{
m_dest = ~*dest;
tmp = ((*src & m_dest) + (((*src ^ m_dest) >> 1) & 0x7f7f7f7f) ) & 0x80808080;
tmp = (tmp << 1) - (tmp >> 7);
*dest ^= (*dest ^ *src) & tmp;
dest++;
src++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPLightenBlend_HDA_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
register tjs_uint32 tmp, m_dest;
EOF


$content = <<EOF;
{
m_dest = ~*dest;
tmp = ((*src & m_dest) + (((*src ^ m_dest) >> 1) & 0x7f7f7f7f) ) & 0x80808080;
tmp = (tmp << 1) - (tmp >> 7);
*dest ^= ((*dest ^ *src) & tmp) & 0xffffff;
dest++;
src++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------


print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPLightenBlend_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
register tjs_uint32 tmp, m_dest, d1;
EOF


$content = <<EOF;
{
m_dest = ~*dest;
tmp = ((*src & m_dest) + (((*src ^ m_dest) >> 1) & 0x7f7f7f7f) ) & 0x80808080;
tmp = (tmp << 1) - (tmp >> 7);
tmp = *dest ^ ((*dest ^ *src) & tmp);
d1 = *dest & 0xff00ff;
d1 = (d1 + (((tmp & 0xff00ff) - d1) * opa >> 8)) & 0xff00ff;
m_dest = *dest & 0xff00;
tmp &= 0xff00;
*dest = d1 + ((m_dest + ((tmp - m_dest) * opa >> 8)) & 0xff00);
dest++;
src++;

}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF


;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPLightenBlend_HDA_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
register tjs_uint32 tmp, m_dest, d1;
EOF


$content = <<EOF;
{
m_dest = ~*dest;
tmp = ((*src & m_dest) + (((*src ^ m_dest) >> 1) & 0x7f7f7f7f) ) & 0x80808080;
tmp = (tmp << 1) - (tmp >> 7);
tmp = *dest ^ (((*dest ^ *src) & tmp) & 0xffffff);
d1 = *dest & 0xff00ff;
d1 = ((d1 + (((tmp & 0xff00ff) - d1) * opa >> 8)) & 0xff00ff) + (*dest & 0xff000000); /* hda */
m_dest = *dest & 0xff00;
tmp &= 0xff00;
*dest = d1 + ((m_dest + ((tmp - m_dest) * opa >> 8)) & 0xff00);
dest++;
src++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------
;# pixel screen multiplactive blend
;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPScreenBlend_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
register tjs_uint32 tmp, s, d;
EOF


$content = <<EOF;
{
s = ~*src;
d = ~*dest;
tmp  = (d & 0xff) * (s & 0xff) & 0xff00;
tmp |= ((d & 0xff00) >> 8) * (s & 0xff00) & 0xff0000;
tmp |= ((d & 0xff0000) >> 16) * (s & 0xff0000) & 0xff000000;
tmp >>= 8;
*dest = ~tmp;
dest++;
src++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------


print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPScreenBlend_HDA_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
register tjs_uint32 tmp, s, d;
EOF


$content = <<EOF;
{
s = ~*src;
d = ~*dest;
tmp  = (d & 0xff) * (s & 0xff) & 0xff00;
tmp |= ((d & 0xff00) >> 8) * (s & 0xff00) & 0xff0000;
tmp |= ((d & 0xff0000) >> 16) * (s & 0xff0000) & 0xff000000;
tmp >>= 8;
*dest = ~tmp ^ (d & 0xff000000);
dest++;
src++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPScreenBlend_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
register tjs_uint32 s, d;
register tjs_uint32 tmp;
EOF


$content = <<EOF;
{
d = ~*dest;
s = *src;
s = ~( ( ((s&0x00ff00)  * opa >> 8)&0x00ff00) +
	(( (s&0xff00ff) * opa >> 8)&0xff00ff));
tmp  = (d & 0xff) * (s & 0xff) & 0xff00;
tmp |= ((d & 0xff00) >> 8) * (s & 0xff00) & 0xff0000;
tmp |= ((d & 0xff0000) >> 16) * (s & 0xff0000) & 0xff000000;
tmp >>= 8;
*dest = tmp;
dest++;
src++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------


print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPScreenBlend_HDA_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
register tjs_uint32 s, d;
register tjs_uint32 tmp;
EOF


$content = <<EOF;
{
d = ~*dest;
s = *src;
s = ~( ( ((s&0x00ff00)  * opa >> 8)&0x00ff00) +
	(( (s&0xff00ff) * opa >> 8)&0xff00ff));
tmp  = (d & 0xff) * (s & 0xff) & 0xff00;
tmp |= ((d & 0xff00) >> 8) * (s & 0xff00) & 0xff0000;
tmp |= ((d & 0xff0000) >> 16) * (s & 0xff0000) & 0xff000000;
tmp >>= 8;
*dest = ~tmp ^ (d & 0xff000000);
dest++;
src++;
}
EOF

&loop_unroll_c($content, 'len', 4);

print FC <<EOF;
}

EOF


;#-----------------------------------------------------------------
;# stretch copy
;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPStretchCopy_c, (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep))
{

	destlen -= 3;
	while(destlen > 0)
	{
		dest[0] = src[srcstart >> 16];
		srcstart += srcstep;
		dest[1] = src[srcstart >> 16];
		srcstart += srcstep;
		dest[2] = src[srcstart >> 16];
		srcstart += srcstep;
		dest[3] = src[srcstart >> 16];
		srcstart += srcstep;
		dest += 4;
		destlen -= 4;
	}

	destlen += 3;

	while(destlen > 0)
	{
		dest[0] = src[srcstart >> 16];
		srcstart += srcstep;
		dest ++;
		destlen --;
	}
}

EOF

;#-----------------------------------------------------------------
print FC <<EOF;

/*export*/
TVP_GL_FUNC_DECL(void, TVPInterpStretchCopy_c, (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src1, const tjs_uint32 *src2, tjs_int blend_y, tjs_int srcstart, tjs_int srcstep))
{
	/* stretch copy with bilinear interpolation */
	tjs_int blend_x;
	tjs_int sp;

	blend_y += blend_y >> 7; /* adjust blend ratio */

	destlen -= 1;
	while(destlen > 0)
	{
		blend_x = (srcstart & 0xffff) >> 8;
		sp = srcstart >> 16;
		dest[0] = TVPBlendARGB(
			TVPBlendARGB(src1[sp], src1[sp+1], blend_x),
			TVPBlendARGB(src2[sp], src2[sp+1], blend_x),
				blend_y);
		srcstart += srcstep;

		blend_x = (srcstart & 0xffff) >> 8;
		sp = srcstart >> 16;
		dest[1] = TVPBlendARGB(
			TVPBlendARGB(src1[sp], src1[sp+1], blend_x),
			TVPBlendARGB(src2[sp], src2[sp+1], blend_x),
				blend_y);
		srcstart += srcstep;

		dest += 2;
		destlen -= 2;
	}

	destlen += 1;

	while(destlen > 0)
	{
		blend_x = (srcstart & 0xffff) >> 8;
		sp = srcstart >> 16;
		dest[0] = TVPBlendARGB(
			TVPBlendARGB(src1[sp], src1[sp+1], blend_x),
			TVPBlendARGB(src2[sp], src2[sp+1], blend_x),
				blend_y);
		srcstart += srcstep;
		dest ++;
		destlen --;
	}
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
#define AVG_PACKED(x, y) (((x) & (y)) + ((((x) ^ (y)) & 0xfefefefe) >> 1))

/*export*/
TVP_GL_FUNC_DECL(void, TVPFastLinearInterpH2F_c, (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src))
{
	/* horizontal 2x fast linear interpolation; forward */
	destlen -= 2;
	while(destlen > 0)
	{
		dest[0] = src[0];
		dest[1] = AVG_PACKED(src[0], src[1]);
		dest += 2;
		src ++;
		destlen -= 2;
	}

	destlen += 2;

	while(destlen > 0)
	{
		dest[0] = src[0];
		dest ++;
		destlen --;
	}
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPFastLinearInterpH2B_c, (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src))
{
	/* horizontal 2x fast linear interpolation; backward */
	destlen -= 2;
	while(destlen > 0)
	{
		dest[0] = src[0];
		dest[1] = AVG_PACKED(src[0], src[-1]);
		dest += 2;
		src --;
		destlen -= 2;
	}

	destlen += 2;

	while(destlen > 0)
	{
		dest[0] = src[0];
		dest ++;
		destlen --;
	}
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPFastLinearInterpV2_c, (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src0, const tjs_uint32 *src1))
{
	/* vertical 2x fast linear interpolation */
	destlen -= 3;
	while(destlen > 0)
	{
		dest[0] = AVG_PACKED(src0[0], src1[0]);
		dest[1] = AVG_PACKED(src0[1], src1[1]);
		dest[2] = AVG_PACKED(src0[2], src1[2]);
		dest[3] = AVG_PACKED(src0[3], src1[3]);
		dest += 4;
		src0 += 4;
		src1 += 4;
		destlen -= 4;
	}

	destlen += 3;

	while(destlen > 0)
	{
		dest[0] = AVG_PACKED(src0[0], src1[0]);
		dest ++;
		src0 ++;
		src1 ++;
		destlen --;
	}
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPStretchColorCopy_c, (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep))
{
	/* this performs only color(main) copy */
	destlen -= 3;
	while(destlen > 0)
	{
		dest[0] = (dest[0] & 0xff000000) + (src[srcstart >> 16] & 0xffffff);
		srcstart += srcstep;
		dest[1] = (dest[1] & 0xff000000) + (src[srcstart >> 16] & 0xffffff);
		srcstart += srcstep;
		dest[2] = (dest[2] & 0xff000000) + (src[srcstart >> 16] & 0xffffff);
		srcstart += srcstep;
		dest[3] = (dest[3] & 0xff000000) + (src[srcstart >> 16] & 0xffffff);
		srcstart += srcstep;
		dest += 4;
		destlen -= 4;
	}

	destlen += 3;

	while(destlen > 0)
	{
		dest[0] = (dest[0] & 0xff0000) + (src[srcstart >> 16] & 0xffffff);
		srcstart += srcstep;
		dest ++;
		destlen --;
	}
}

EOF

;#-----------------------------------------------------------------
;# linear transforming copy
;#-----------------------------------------------------------------



print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPLinTransCopy_c, (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch))
{
	/* note that srcpitch unit is in byte */
	destlen -= 3;
	while(destlen > 0)
	{
		dest[0] = *( (const tjs_uint32*)((tjs_uint8*)src + (sy>>16)*srcpitch) + (sx>>16));
		sx += stepx, sy += stepy;
		dest[1] = *( (const tjs_uint32*)((tjs_uint8*)src + (sy>>16)*srcpitch) + (sx>>16));
		sx += stepx, sy += stepy;
		dest[2] = *( (const tjs_uint32*)((tjs_uint8*)src + (sy>>16)*srcpitch) + (sx>>16));
		sx += stepx, sy += stepy;
		dest[3] = *( (const tjs_uint32*)((tjs_uint8*)src + (sy>>16)*srcpitch) + (sx>>16));
		sx += stepx, sy += stepy;

		dest += 4;
		destlen -= 4;
	}

	destlen += 3;

	while(destlen > 0)
	{
		dest[0] = *( (const tjs_uint32*)((tjs_uint8*)src + (sy>>16)*srcpitch) + (sx>>16));
		sx += stepx, sy += stepy;
		dest ++;
		destlen --;
	}
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPInterpLinTransCopy_c, (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch))
{
	/* bilinear interpolation version */
	/* note that srcpitch unit is in byte */
	destlen -= 1;
	while(destlen > 0)
	{
		int blend_x, blend_y;
		const tjs_uint32 *p0, *p1;

		blend_x = (sx & 0xffff) >> 8;
		blend_x += blend_x >> 7;
		blend_y = (sy & 0xffff) >> 8;
		blend_y += blend_y >> 7;
		p0 = (const tjs_uint32*)((const tjs_uint8*)src + ((sy>>16)  )*srcpitch) + (sx>>16);
		p1 = (const tjs_uint32*)((const tjs_uint8*)p0 + srcpitch);
		dest[0] = TVPBlendARGB(
			TVPBlendARGB(p0[0], p0[1], blend_x),
			TVPBlendARGB(p1[0], p1[1], blend_x),
				blend_y);
		sx += stepx, sy += stepy;

		blend_x = (sx & 0xffff) >> 8;
		blend_x += blend_x >> 7;
		blend_y = (sy & 0xffff) >> 8;
		blend_y += blend_y >> 7;
		p0 = (const tjs_uint32*)((const tjs_uint8*)src + ((sy>>16)  )*srcpitch) + (sx>>16);
		p1 = (const tjs_uint32*)((const tjs_uint8*)p0 + srcpitch);
		dest[1] = TVPBlendARGB(
			TVPBlendARGB(p0[0], p0[1], blend_x),
			TVPBlendARGB(p1[0], p1[1], blend_x),
				blend_y);
		sx += stepx, sy += stepy;

		dest += 2;
		destlen -= 2;
	}

	destlen += 1;

	while(destlen > 0)
	{
		int blend_x, blend_y;
		const tjs_uint32 *p0, *p1;

		blend_x = (sx & 0xffff) >> 8;
		blend_x += blend_x >> 7;
		blend_y = (sy & 0xffff) >> 8;
		blend_y += blend_y >> 7;
		p0 = (const tjs_uint32*)((const tjs_uint8*)src + ((sy>>16)  )*srcpitch) + (sx>>16);
		p1 = (const tjs_uint32*)((const tjs_uint8*)p0 + srcpitch);
		dest[0] = TVPBlendARGB(
			TVPBlendARGB(p0[0], p0[1], blend_x),
			TVPBlendARGB(p1[0], p1[1], blend_x),
				blend_y);
		sx += stepx, sy += stepy;

		dest ++;
		destlen --;
	}
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPLinTransColorCopy_c, (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch))
{
	/* note that srcpitch unit is in byte */
	destlen -= 3;
	while(destlen > 0)
	{
		dest[0] = (dest[0] & 0xff000000) + (0x00ffffff & *( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16)*srcpitch) + (sx>>16)));
		sx += stepx, sy += stepy;
		dest[1] = (dest[1] & 0xff000000) + (0x00ffffff & *( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16)*srcpitch) + (sx>>16)));
		sx += stepx, sy += stepy;
		dest[2] = (dest[2] & 0xff000000) + (0x00ffffff & *( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16)*srcpitch) + (sx>>16)));
		sx += stepx, sy += stepy;
		dest[3] = (dest[3] & 0xff000000) + (0x00ffffff & *( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16)*srcpitch) + (sx>>16)));
		sx += stepx, sy += stepy;

		dest += 4;
		destlen -= 4;
	}

	destlen += 3;

	while(destlen > 0)
	{
		dest[0] = (dest[0] & 0xff000000) + (0x00ffffff & *( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16)*srcpitch) + (sx>>16)));
		sx += stepx, sy += stepy;
		dest ++;
		destlen --;
	}
}

EOF



;#-----------------------------------------------------------------
;# make alpha from the color key
;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPMakeAlphaFromKey_c, (tjs_uint32 *dest, tjs_int len, tjs_uint32 key))
{
	tjs_uint32 a, b;
EOF


$content = <<EOF;
	a = dest[{ofs}] & 0xffffff;;
	if(a != key) a |= 0xff000000;;
	dest[{ofs}] = a;;
EOF
$content2 = <<EOF;
	b = dest[{ofs}] & 0xffffff;;
	if(b != key) b |= 0xff000000;;
	dest[{ofs}] = b;;
EOF

&loop_unroll_c_int_2($content, $content2, 'len', 8);

print FC <<EOF;
}

EOF



;#-----------------------------------------------------------------
;# copy the mask only
;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPCopyMask_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
EOF
$content = <<EOF;
		{
			dest[{ofs}] = (dest[{ofs}] & 0xffffff) +
				(src[{ofs}] & 0xff000000);
		}
EOF

print FC <<EOF;
	if(dest < src)
	{
		/* backward */
EOF
&loop_unroll_c_2_backward($content, 'len', 8);
print FC <<EOF;
	}
	else
	{
		/* forward */
EOF
&loop_unroll_c_2($content, 'len', 8);
print FC <<EOF;
	}
}

EOF


;#-----------------------------------------------------------------
;# copy the color (main) only
;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPCopyColor_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
EOF
$content = <<EOF;
{
	dest[{ofs}] = (dest[{ofs}] & 0xff000000) +
		(src[{ofs}] & 0x00ffffff);
}
EOF

print FC <<EOF;
	if(dest < src)
	{
		/* backward */
EOF
&loop_unroll_c_2_backward($content, 'len', 8);
print FC <<EOF;
	}
	else
	{
		/* forward */
EOF
&loop_unroll_c_2($content, 'len', 8);
print FC <<EOF;
	}
}

EOF

;#-----------------------------------------------------------------
;# bind mask image to main image 
;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPBindMaskToMain_c, (tjs_uint32 *main, const tjs_uint8 *mask, tjs_int len))
{
EOF


$content = <<EOF;
{
	main[{ofs}] = (main[{ofs}] & 0xffffff) + (mask[{ofs}] << 24);
}
EOF

&loop_unroll_c_2($content, 'len', 8);

print FC <<EOF;
}

EOF


;#-----------------------------------------------------------------
;# fill ARGB
;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPFillARGB_c, (tjs_uint32 *dest, tjs_int len, tjs_uint32 value))
{
EOF

$content = <<EOF;
{
	dest[{ofs}] = value;
}
EOF

&loop_unroll_c_2($content, 'len', 8);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPFillARGB_NC_c, (tjs_uint32 *dest, tjs_int len, tjs_uint32 value))
{
	/* non-cached version of TVPFillARGB */
	/* this routine written in C has no difference from TVPFillARGB. */ 
EOF

$content = <<EOF;
{
	dest[{ofs}] = value;
}
EOF

&loop_unroll_c_2($content, 'len', 8);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------
;# fill color
;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPFillColor_c, (tjs_uint32 *dest, tjs_int len, tjs_uint32 color))
{
	tjs_uint32 t1, t2;

	color &= 0xffffff;
EOF

$content = <<EOF;
	t1 = dest[{ofs}];;
	t1 &= 0xff000000;;
	t1 += color;;
	dest[{ofs}] = t1;;
EOF
$content2 = <<EOF;
	t2 = dest[{ofs}];;
	t2 &= 0xff000000;;
	t2 += color;;
	dest[{ofs}] = t2;;
EOF

&loop_unroll_c_int_2($content, $content2, 'len', 8);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------
;# fill mask
;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPFillMask_c, (tjs_uint32 *dest, tjs_int len, tjs_uint32 mask))
{
	tjs_uint32 t1, t2;
	mask <<= 24;
EOF

$content = <<EOF;
	t1 = dest[{ofs}];;
	t1 &= 0x00ffffff;;
	t1 += mask;;
	dest[{ofs}] = t1;;
EOF
$content2 = <<EOF;
	t2 = dest[{ofs}];;
	t2 &= 0x00ffffff;;
	t2 += mask;;
	dest[{ofs}] = t2;;
EOF

&loop_unroll_c_int_2($content, $content2, 'len', 8);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------
;# functions for blur operation
;#-----------------------------------------------------------------


print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPAddSubVertSum16_c, (tjs_uint16 *dest, const tjs_uint32 *addline, const tjs_uint32 *subline, tjs_int len))
{
EOF

$content = <<EOF;
{
	tjs_uint32 add, sub;
	add = addline[{ofs}];
	sub = subline[{ofs}];
	dest[{ofs}*4+0] += ((add    ) & 0xff) - ((sub    ) & 0xff);
	dest[{ofs}*4+1] += ((add>>8 ) & 0xff) - ((sub>>8 ) & 0xff);
	dest[{ofs}*4+2] += ((add>>16) & 0xff) - ((sub>>16) & 0xff);
	dest[{ofs}*4+3] += ((add>>24)       ) - ((sub>>24)       );
}
EOF

&loop_unroll_c_2($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPAddSubVertSum16_d_c, (tjs_uint16 *dest, const tjs_uint32 *addline, const tjs_uint32 *subline, tjs_int len))
{
EOF

$content = <<EOF;
{
	tjs_uint32 add, sub;
	tjs_int add_a, sub_a;
	add = addline[{ofs}];
	sub = subline[{ofs}];
	dest[{ofs}*4+3] += (add_a = (add>>24)       ) - (sub_a = (sub>>24)       );
	add_a += add_a >> 7;
	sub_a += sub_a >> 7;
	dest[{ofs}*4+0] += (((add    ) & 0xff) * add_a >> 8) - (((sub    ) & 0xff) * sub_a >> 8);
	dest[{ofs}*4+1] += (((add>>8 ) & 0xff) * add_a >> 8) - (((sub>>8 ) & 0xff) * sub_a >> 8);
	dest[{ofs}*4+2] += (((add>>16) & 0xff) * add_a >> 8) - (((sub>>16) & 0xff) * sub_a >> 8);
}
EOF

&loop_unroll_c_2($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPAddSubVertSum32_c, (tjs_uint32 *dest, const tjs_uint32 *addline, const tjs_uint32 *subline, tjs_int len))
{
EOF

$content = <<EOF;
{
	tjs_uint32 add, sub;
	add = addline[{ofs}];
	sub = subline[{ofs}];
	dest[{ofs}*4+0] += ((add    ) & 0xff) - ((sub    ) & 0xff);
	dest[{ofs}*4+1] += ((add>>8 ) & 0xff) - ((sub>>8 ) & 0xff);
	dest[{ofs}*4+2] += ((add>>16) & 0xff) - ((sub>>16) & 0xff);
	dest[{ofs}*4+3] += ((add>>24)       ) - ((sub>>24)       );
}
EOF

&loop_unroll_c_2($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPAddSubVertSum32_d_c, (tjs_uint32 *dest, const tjs_uint32 *addline, const tjs_uint32 *subline, tjs_int len))
{
EOF

$content = <<EOF;
{
	tjs_uint32 add, sub;
	tjs_int add_a, sub_a;
	add = addline[{ofs}];
	sub = subline[{ofs}];
	dest[{ofs}*4+3] += (add_a = (add>>24)       ) - (sub_a = (sub>>24)       );
	add_a += add_a >> 7;
	sub_a += sub_a >> 7;
	dest[{ofs}*4+0] += (((add    ) & 0xff) * add_a >> 8) - (((sub    ) & 0xff) * sub_a >> 8);
	dest[{ofs}*4+1] += (((add>>8 ) & 0xff) * add_a >> 8) - (((sub>>8 ) & 0xff) * sub_a >> 8);
	dest[{ofs}*4+2] += (((add>>16) & 0xff) * add_a >> 8) - (((sub>>16) & 0xff) * sub_a >> 8);
}
EOF

&loop_unroll_c_2($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPDoBoxBlurAvg16_c, (tjs_uint32 *dest, tjs_uint16 *sum, const tjs_uint16 * add, const tjs_uint16 * sub, tjs_int n, tjs_int len))
{
	tjs_int rcp = (1<<16) / n;
	tjs_int half_n = n >> 1;
EOF

$content = <<EOF;
{
	dest[{ofs}] =
		(((sum[0] + half_n) * rcp >> 16)       )+
		(((sum[1] + half_n) * rcp >> 16) << 8  )+
		(((sum[2] + half_n) * rcp >> 16) << 16 )+
		(((sum[3] + half_n) * rcp >> 16) << 24 );

	sum[0] += add[{ofs}*4+0] - sub[{ofs}*4+0];
	sum[1] += add[{ofs}*4+1] - sub[{ofs}*4+1];
	sum[2] += add[{ofs}*4+2] - sub[{ofs}*4+2];
	sum[3] += add[{ofs}*4+3] - sub[{ofs}*4+3];
}
EOF

&loop_unroll_c_2($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPDoBoxBlurAvg16_d_c, (tjs_uint32 *dest, tjs_uint16 *sum, const tjs_uint16 * add, const tjs_uint16 * sub, tjs_int n, tjs_int len))
{
	tjs_int rcp = (1<<16) / n;
	tjs_int half_n = n >> 1;
EOF

$content = <<EOF;
{
	tjs_int a = ((sum[3] + half_n) * rcp >> 16);
	tjs_uint8 * t = TVPDivTable + (a << 8);
	dest[{ofs}] =
		(t[(sum[0] + half_n) * rcp >> 16]       )+
		(t[(sum[1] + half_n) * rcp >> 16] << 8  )+
		(t[(sum[2] + half_n) * rcp >> 16] << 16 )+
		(a << 24 );

	sum[0] += add[{ofs}*4+0] - sub[{ofs}*4+0];
	sum[1] += add[{ofs}*4+1] - sub[{ofs}*4+1];
	sum[2] += add[{ofs}*4+2] - sub[{ofs}*4+2];
	sum[3] += add[{ofs}*4+3] - sub[{ofs}*4+3];
}
EOF

&loop_unroll_c_2($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPDoBoxBlurAvg32_c, (tjs_uint32 *dest, tjs_uint32 *sum, const tjs_uint32 * add, const tjs_uint32 * sub, tjs_int n, tjs_int len))
{
	/* This function is very slow since using divisiion in loop. Function written in assembly should be used. */
	tjs_int half_n = n >> 1;
EOF

$content = <<EOF;
{
	dest[{ofs}] =
		(((sum[0] + half_n) / n)       )+
		(((sum[1] + half_n) / n) << 8  )+
		(((sum[2] + half_n) / n) << 16 )+
		(((sum[3] + half_n) / n) << 24 );

	sum[0] += add[{ofs}*4+0] - sub[{ofs}*4+0];
	sum[1] += add[{ofs}*4+1] - sub[{ofs}*4+1];
	sum[2] += add[{ofs}*4+2] - sub[{ofs}*4+2];
	sum[3] += add[{ofs}*4+3] - sub[{ofs}*4+3];
}
EOF

&loop_unroll_c_2($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPDoBoxBlurAvg32_d_c, (tjs_uint32 *dest, tjs_uint32 *sum, const tjs_uint32 * add, const tjs_uint32 * sub, tjs_int n, tjs_int len))
{
	/* This function is very slow since using divisiion in loop. Function written in assembly should be used. */
	tjs_int half_n = n >> 1;
EOF

$content = <<EOF;
{
	tjs_int a = ((sum[3] + half_n) / n);
	tjs_uint8 * t = TVPDivTable + (a << 8);
	dest[{ofs}] =
		(t[(sum[0] + half_n) / n]       )+
		(t[(sum[1] + half_n) / n] << 8  )+
		(t[(sum[2] + half_n) / n] << 16 )+
		(a << 24 );

	sum[0] += add[{ofs}*4+0] - sub[{ofs}*4+0];
	sum[1] += add[{ofs}*4+1] - sub[{ofs}*4+1];
	sum[2] += add[{ofs}*4+2] - sub[{ofs}*4+2];
	sum[3] += add[{ofs}*4+3] - sub[{ofs}*4+3];
}
EOF

&loop_unroll_c_2($content, 'len', 4);

print FC <<EOF;
}

EOF


;#-----------------------------------------------------------------
;# UD/LR flip
;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPSwapLine8_c, (tjs_uint8 *line1, tjs_uint8 *line2, tjs_int len))
{
	#define swap_tmp_buf_size 256
	tjs_uint8 swap_tmp_buf[swap_tmp_buf_size];
	while(len)
	{
		tjs_int le = len < swap_tmp_buf_size ? len : swap_tmp_buf_size;
		memcpy(swap_tmp_buf, line1, le);
		memcpy(line1, line2, le);
		memcpy(line2, swap_tmp_buf, le);
		line1 += le;
		line2 += le;
		len -= le;
	}
	#undef swap_tmp_buf_size
}

EOF

;#-----------------------------------------------------------------


print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPSwapLine32_c, (tjs_uint32 *line1, tjs_uint32 *line2, tjs_int len))
{
	tjs_uint32 tmp, tmp2;
EOF

$content = <<EOF;
	tmp = line1[{ofs}];;
	line1[{ofs}] = line2[{ofs}];;
	line2[{ofs}] = tmp;;
EOF

$content2 = <<EOF;
	tmp2 = line1[{ofs}];;
	line1[{ofs}] = line2[{ofs}];;
	line2[{ofs}] = tmp2;;
EOF

&loop_unroll_c_int_2($content, $content2, 'len', 8);


print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------


print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPReverse8_c, (tjs_uint8 *pixels, tjs_int len))
{
	tjs_uint8 *pixels2 = pixels + len -1;
	len/=2;
EOF

$content = <<EOF;
{
	tjs_uint8 tmp = *pixels;
	*pixels = *pixels2;
	*pixels2 = tmp;
	pixels2 --;
	pixels++;
}
EOF

&loop_unroll_c($content, 'len', 4);


print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------


print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPReverse32_c, (tjs_uint32 *pixels, tjs_int len))
{
	tjs_uint32 *pixels2 = pixels + len -1;
	len/=2;
EOF

$content = <<EOF;
{
	tjs_uint32 tmp = *pixels;
	*pixels = *pixels2;
	*pixels2 = tmp;
	pixels2 --;
	pixels++;
}
EOF

&loop_unroll_c($content, 'len', 4);


print FC <<EOF;
}

EOF



;#-----------------------------------------------------------------
;# grayscale conversion
;#-----------------------------------------------------------------


print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPDoGrayScale_c, (tjs_uint32 *dest, tjs_int len))
{
	tjs_uint32 s1, d1, s2, d2;
EOF

$content = <<EOF;
	s1 = dest[{ofs}];;
	d1 = (s1&0xff)*19;;
	d1 += ((s1 >> 8)&0xff)*183;;
	d1 += ((s1 >> 16)&0xff)*54;;
	d1 = (d1 >> 8) * 0x10101 + (s1 & 0xff000000);;
	dest[{ofs}] = d1;;
EOF
$content2 = <<EOF;
	s2 = dest[{ofs}];;
	d2 = (s2&0xff)*19;;
	d2 += ((s2 >> 8)&0xff)*183;;
	d2 += ((s2 >> 16)&0xff)*54;;
	d2 = (d2 >> 8) * 0x10101 + (s2 & 0xff000000);;
	dest[{ofs}] = d2;;
EOF

&loop_unroll_c_int_2($content, $content2, 'len', 4);

print FC <<EOF;
}

EOF


;#-----------------------------------------------------------------
;# Gamma Adjustment
;#-----------------------------------------------------------------

print FH <<EOF;
/*[*/
#pragma pack(push, 4)
typedef struct
{
	tjs_uint8 B[256];
	tjs_uint8 G[256];
	tjs_uint8 R[256];
} tTVPGLGammaAdjustTempData;
#pragma pack(pop)
/*]*/
EOF

print FC <<EOF;


/*export*/
TVP_GL_FUNC_DECL(void, TVPInitGammaAdjustTempData_c, (tTVPGLGammaAdjustTempData *temp, const tTVPGLGammaAdjustData *data))
{
	/* make table */

	double ramp = data->RCeil - data->RFloor;
	double gamp = data->GCeil - data->GFloor;
	double bamp = data->BCeil - data->BFloor;

	double rgamma = 1.0/data->RGamma; /* we assume data.?Gamma is a non-zero value here */
	double ggamma = 1.0/data->GGamma;
	double bgamma = 1.0/data->BGamma;

	int i;
	for(i=0;i<256;i++)
	{
#if 0
		double rate = (double)i/255.0;
		int n;
		n = (int)(pow(rate, rgamma)*ramp+0.5+(double)data->RFloor);
		if(n<0) n=0; else if(n>255) n=255;
		temp->R[i]= n;
		n = (int)(pow(rate, ggamma)*gamp+0.5+(double)data->GFloor);
		if(n<0) n=0; else if(n>255) n=255;
		temp->G[i]= n;
		n = (int)(pow(rate, bgamma)*bamp+0.5+(double)data->BFloor);
		if(n<0) n=0; else if(n>255) n=255;
		temp->B[i]= n;
#else
		// pow(x, y) == exp(y * log(x))
		double rate = log((double)i/255.0);
		int n;
		n = (int)(exp(rate * rgamma)*ramp+0.5+(double)data->RFloor);
		if(n<0) n=0; else if(n>255) n=255;
		temp->R[i]= n;
		n = (int)(exp(rate * ggamma)*gamp+0.5+(double)data->GFloor);
		if(n<0) n=0; else if(n>255) n=255;
		temp->G[i]= n;
		n = (int)(exp(rate * bgamma)*bamp+0.5+(double)data->BFloor);
		if(n<0) n=0; else if(n>255) n=255;
		temp->B[i]= n;
#endif
	}
}

/*export*/
TVP_GL_FUNC_DECL(void, TVPUninitGammaAdjustTempData_c, (tTVPGLGammaAdjustTempData *temp))
{
	/* nothing to do */
}
EOF



print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPAdjustGamma_c, (tjs_uint32 *dest, tjs_int len, tTVPGLGammaAdjustTempData *temp))
{
	tjs_uint32 d1, t1;
EOF

$content = <<EOF;
	d1 = dest[{ofs}];;
	if(d1 > 0x00ffffff)
	{
		/* process only non-fully-transparent pixel */
		t1 = temp->B[d1 & 0xff];;
		d1 >>= 8;;
		t1 += (temp->G[d1 & 0xff]<<8);;
		d1 >>= 8;;
		t1 += (temp->R[d1 & 0xff]<<16);;
		t1 += ((d1 & 0xff00) << 16);;
		dest[{ofs}] = t1;;
	}
EOF

&loop_unroll_c_2($content, 'len', 4);

print FC <<EOF;
}

EOF

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPAdjustGamma_a_c, (tjs_uint32 *dest, tjs_int len, tTVPGLGammaAdjustTempData *temp))
{
	/* gamma adjustment for additive alpha */
EOF

$content = <<EOF;
{
	tjs_uint32 d;
	tjs_int alpha;
	tjs_int alpha_adj;
	tjs_int recip;
	tjs_int t, d_tmp;

	d = dest[{ofs}];

	if(d >= 0xff000000)
	{
		/* completely opaque */
		t = d & 0xff;
		d_tmp =   temp->B[t];
		t = (d>>8) & 0xff;
		d_tmp |=  temp->G[t] << 8;
		t = (d>>16) & 0xff; 
		d_tmp |=  temp->R[t] << 16;
		d_tmp |= 0xff000000;
		dest[{ofs}] = d_tmp;
	}
	else if(d != 0)
	{
		/* not completely transparent */
		alpha = d >> 24;
		alpha_adj = alpha + (alpha >> 7);
		recip = TVPRecipTable256_16[alpha];

		/* B */
		t = d & 0xff;
		if(t > alpha)
			d_tmp = (temp->B[255] * alpha_adj >> 8) + t - alpha;
		else
			d_tmp = temp->B[recip * t >> 8] * alpha_adj >> 8;
		/* G */
		t = (d>>8) & 0xff; 
		if(t > alpha)
			d_tmp |= ((temp->G[255] * alpha_adj >> 8) + t - alpha) << 8;
		else
			d_tmp |= (temp->G[recip * t >> 8] * alpha_adj >> 8) << 8;
		/* R */
		t = (d>>16) & 0xff; 
		if(t > alpha)
			d_tmp |= ((temp->R[255] * alpha_adj >> 8) + t - alpha) << 16;
		else
			d_tmp |= (temp->R[recip * t >> 8] * alpha_adj >> 8) << 16;
		/* A */
		d_tmp |= d & 0xff000000;

		dest[{ofs}] = d_tmp;
	}
}
EOF

&loop_unroll_c_2($content, 'len', 2);

print FC <<EOF;
}

EOF


;#-----------------------------------------------------------------
;# simple blur for character data
;#-----------------------------------------------------------------



print FC <<EOF;

/*export*/
TVP_GL_FUNC_DECL(void, TVPChBlurMulCopy65_c, (tjs_uint8 *dest, const tjs_uint8 *src, tjs_int len, tjs_int level))
{
	tjs_int a, b;
EOF

$content = <<EOF;
	a = (src[{ofs}] * level >> 18);;
	if(a>=64) a = 64;;
	dest[{ofs}] = a;;
EOF
$content2 = <<EOF;
	b = (src[{ofs}] * level >> 18);;
	if(b>=64) b = 64;;
	dest[{ofs}] = b;;
EOF

&loop_unroll_c_int_2($content, $content2, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------


print FC <<EOF;

/*export*/
TVP_GL_FUNC_DECL(void, TVPChBlurAddMulCopy65_c, (tjs_uint8 *dest, const tjs_uint8 *src, tjs_int len, tjs_int level))
{
	tjs_int a, b;
EOF

$content = <<EOF;
	a = dest[{ofs}] +(src[{ofs}] * level >> 18);;
	if(a>=64) a = 64;;
	dest[{ofs}] = a;;
EOF
$content2 = <<EOF;
	b = dest[{ofs}] +(src[{ofs}] * level >> 18);;
	if(b>=64) b = 64;;
	dest[{ofs}] = b;;
EOF

&loop_unroll_c_int_2($content, $content2, 'len', 4);

print FC <<EOF;
}


/* fast_int_hypot from http://demo.and.or.jp/makedemo/effect/math/hypot/fast_hypot.c */
tjs_uint fast_int_hypot(tjs_int lx, tjs_int ly)
{
	tjs_uint len1, len2,t,length;

/*	lx = abs(lx); */
/*	ly = abs(ly); */
	if(lx<0) lx = -lx;
	if(ly<0) ly = -ly;
	/*
		CWD
		XOR EAX,EDX
		SUB EAX,EDX
	*/
	
	if (lx >= ly)
	{
		len1 = lx ; len2 = ly;
	}
	else
	{
		len1 = ly ; len2 = lx;
	}

	t = len2 + (len2 >> 1) ;
	length = len1 - (len1 >> 5) - (len1 >> 7) + (t >> 2) + (t >> 6) ;
	return length;
}



/* simple blur for character data */
/* shuld be more optimized */

/*export*/
TVP_GL_FUNC_DECL(void, TVPChBlurCopy65_c, (tjs_uint8 *dest, tjs_int destpitch, tjs_int destwidth, tjs_int destheight, const tjs_uint8 * src, tjs_int srcpitch, tjs_int srcwidth, tjs_int srcheight, tjs_int blurwidth, tjs_int blurlevel))
{
	tjs_int lvsum, x, y;

	/* clear destination */
	memset(dest, 0, destpitch*destheight);

	/* compute filter level */
	lvsum = 0;
	for(y = -blurwidth; y <= blurwidth; y++)
	{
		for(x = -blurwidth; x <= blurwidth; x++)
		{
			tjs_int len = fast_int_hypot(x, y);
			if(len <= blurwidth)
				lvsum += (blurwidth - len +1);
		}
	}

	if(lvsum) lvsum = (1<<18)/lvsum; else lvsum=(1<<18);

	/* apply */
	for(y = -blurwidth; y <= blurwidth; y++)
	{
		for(x = -blurwidth; x <= blurwidth; x++)
		{
			tjs_int len = fast_int_hypot(x, y);
			if(len <= blurwidth)
			{
				tjs_int sy;

				len = blurwidth - len +1;
				len *= lvsum;
				len *= blurlevel;
				len >>= 8;
				for(sy = 0; sy < srcheight; sy++)
				{
					TVPChBlurAddMulCopy65(dest + (y + sy + blurwidth)*destpitch + x + blurwidth, 
						src + sy * srcpitch, srcwidth, len);
				}
			}
		}
	}
}

/*export*/
TVP_GL_FUNC_DECL(void, TVPChBlurMulCopy_c, (tjs_uint8 *dest, const tjs_uint8 *src, tjs_int len, tjs_int level) )
{
	tjs_int a, b;
	{
		int ___index = 0;
		len -= (4-1);

		while(___index < len)
		{
			a = (src[(___index+(0*2))] * level >> 18);
			b = (src[(___index+(0*2+1))] * level >> 18);
			if(a>=255) a = 255;
			if(b>=255) b = 255;
			dest[(___index+(0*2))] = a;
			dest[(___index+(0*2+1))] = b;
			a = (src[(___index+(1*2))] * level >> 18);
			b = (src[(___index+(1*2+1))] * level >> 18);
			if(a>=255) a = 255;
			if(b>=255) b = 255;
			dest[(___index+(1*2))] = a;
			dest[(___index+(1*2+1))] = b;
			___index += 4;
		}

		len += (4-1);

		while(___index < len)
		{
			a = (src[___index] * level >> 18);;
			if(a>=255) a = 255;
			dest[___index] = a;;
			___index ++;
		}
	}
}

/*export*/
TVP_GL_FUNC_DECL(void, TVPChBlurAddMulCopy_c, (tjs_uint8 *dest, const tjs_uint8 *src, tjs_int len, tjs_int level) )
{
	tjs_int a, b;
	{
		int ___index = 0;
		len -= (4-1);

		while(___index < len)
		{
			a = dest[(___index+(0*2))] +(src[(___index+(0*2))] * level >> 18);
			b = dest[(___index+(0*2+1))] +(src[(___index+(0*2+1))] * level >> 18);
			if(a>=255) a = 255;
			if(b>=255) b = 255;
			dest[(___index+(0*2))] = a;
			dest[(___index+(0*2+1))] = b;
			a = dest[(___index+(1*2))] +(src[(___index+(1*2))] * level >> 18);
			b = dest[(___index+(1*2+1))] +(src[(___index+(1*2+1))] * level >> 18);
			if(a>=255) a = 255;
			if(b>=255) b = 255;
			dest[(___index+(1*2))] = a;
			dest[(___index+(1*2+1))] = b;
			___index += 4;
		}

		len += (4-1);

		while(___index < len)
		{
			a = dest[___index] +(src[___index] * level >> 18);;
			if(a>=255) a = 255;;
			dest[___index] = a;;
			___index ++;
		}
	}
}

/*export*/
TVP_GL_FUNC_DECL(void, TVPChBlurCopy_c, (tjs_uint8 *dest, tjs_int destpitch, tjs_int destwidth, tjs_int destheight, const tjs_uint8 * src, tjs_int srcpitch, tjs_int srcwidth, tjs_int srcheight, tjs_int blurwidth, tjs_int blurlevel) )
{
	tjs_int lvsum, x, y;

	/* clear destination */
	memset(dest, 0, destpitch*destheight);

	/* compute filter level */
	lvsum = 0;
	for(y = -blurwidth; y <= blurwidth; y++)
	{
		for(x = -blurwidth; x <= blurwidth; x++)
		{
			tjs_int len = fast_int_hypot(x, y);
			if(len <= blurwidth)
				lvsum += (blurwidth - len +1);
		}
	}

	if(lvsum) lvsum = (1<<18)/lvsum; else lvsum=(1<<18);

	/* apply */
	for(y = -blurwidth; y <= blurwidth; y++)
	{
		for(x = -blurwidth; x <= blurwidth; x++)
		{
			tjs_int len = fast_int_hypot(x, y);
			if(len <= blurwidth)
			{
				tjs_int sy;

				len = blurwidth - len +1;
				len *= lvsum;
				len *= blurlevel;
				len >>= 8;
				for(sy = 0; sy < srcheight; sy++)
				{
					TVPChBlurAddMulCopy(dest + (y + sy + blurwidth)*destpitch + x + blurwidth, 
						src + sy * srcpitch, srcwidth, len);
				}
			}
		}
	}
}

EOF

;#-----------------------------------------------------------------
;# pixel format conversion
;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPBLExpand1BitTo8BitPal_c, (tjs_uint8 *dest, const tjs_uint8 *buf, tjs_int len, const tjs_uint32 *pal))
{
	tjs_uint8 p[2];
	tjs_uint8 *d=dest, *dlim;
	tjs_uint8 b;

	p[0] = pal[0]&0xff, p[1] = pal[1]&0xff;
	dlim = dest + len-7;
	while(d < dlim)
	{
		b = *(buf++);
		d[0] = p[(tjs_uint)(b&(tjs_uint)0x80)>>7];
		d[1] = p[(tjs_uint)(b&(tjs_uint)0x40)>>6];
		d[2] = p[(tjs_uint)(b&(tjs_uint)0x20)>>5];
		d[3] = p[(tjs_uint)(b&(tjs_uint)0x10)>>4];
		d[4] = p[(tjs_uint)(b&(tjs_uint)0x08)>>3];
		d[5] = p[(tjs_uint)(b&(tjs_uint)0x04)>>2];
		d[6] = p[(tjs_uint)(b&(tjs_uint)0x02)>>1];
		d[7] = p[(tjs_uint)(b&(tjs_uint)0x01)   ];
		d += 8;
	}
	dlim = dest + len;
	b = *buf;
	while(d<dlim)
	{
		*(d++) = (b&0x80) ? p[1] : p[0];
		b<<=1;
	}
}
EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPBLExpand1BitTo8Bit_c, (tjs_uint8 *dest, const tjs_uint8 *buf, tjs_int len))
{
	tjs_uint8 *d=dest, *dlim;
	tjs_uint8 b;

	dlim = dest + len-7;
	while(d < dlim)
	{
		b = *(buf++);
		d[0] = (tjs_uint8)((b&(tjs_uint)0x80)>>7);
		d[1] = (tjs_uint8)((b&(tjs_uint)0x40)>>6);
		d[2] = (tjs_uint8)((b&(tjs_uint)0x20)>>5);
		d[3] = (tjs_uint8)((b&(tjs_uint)0x10)>>4);
		d[4] = (tjs_uint8)((b&(tjs_uint)0x08)>>3);
		d[5] = (tjs_uint8)((b&(tjs_uint)0x04)>>2);
		d[6] = (tjs_uint8)((b&(tjs_uint)0x02)>>1);
		d[7] = (tjs_uint8)((b&(tjs_uint)0x01)   );
		d += 8;
	}
	dlim = dest + len;
	b = *buf;
	while(d<dlim)
	{
		*(d++) = (b&0x80) ? 1 : 0;
		b<<=1;
	}
}
EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPBLExpand1BitTo32BitPal_c, (tjs_uint32 *dest, const tjs_uint8 *buf, tjs_int len, const tjs_uint32 *pal))
{
	tjs_uint32 p[2];
	tjs_uint32 *d=dest, *dlim;
	tjs_uint8 b;

	p[0] = pal[0], p[1] = pal[1];
	dlim = dest + len-7;
	while(d < dlim)
	{
		b = *(buf++);
		d[0] = p[(tjs_uint)(b&(tjs_uint)0x80)>>7];
		d[1] = p[(tjs_uint)(b&(tjs_uint)0x40)>>6];
		d[2] = p[(tjs_uint)(b&(tjs_uint)0x20)>>5];
		d[3] = p[(tjs_uint)(b&(tjs_uint)0x10)>>4];
		d[4] = p[(tjs_uint)(b&(tjs_uint)0x08)>>3];
		d[5] = p[(tjs_uint)(b&(tjs_uint)0x04)>>2];
		d[6] = p[(tjs_uint)(b&(tjs_uint)0x02)>>1];
		d[7] = p[(tjs_uint)(b&(tjs_uint)0x01)   ];
		d += 8;
	}
	dlim = dest + len;
	b = *buf;
	while(d<dlim)
	{
		*(d++) = (b&0x80) ? p[1] : p[0];
		b<<=1;
	}
}
EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPBLExpand4BitTo8BitPal_c, (tjs_uint8 *dest, const tjs_uint8 *buf, tjs_int len, const tjs_uint32 *pal))
{
	tjs_uint8 *d=dest, *dlim;
	tjs_uint8 b;

	dlim = dest + (len & ~1);
	while(d < dlim)
	{
		b = *(buf++);
		d[0] = (tjs_uint8)pal[(b&0xf0)>>4];
		d[1] = (tjs_uint8)pal[b&0x0f];
		d += 2;
	}
	if(len & 1)
	{
		b = *buf;
		if(d<dlim) *d = (tjs_uint8)pal[(b&0xf0)>>4];
	}
}
EOF
;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPBLExpand4BitTo8Bit_c, (tjs_uint8 *dest, const tjs_uint8 *buf, tjs_int len))
{
	tjs_uint8 *d=dest, *dlim;
	tjs_uint8 b;

	dlim = dest + (len & ~1);
	while(d < dlim)
	{
		b = *(buf++);
		d[0] = (tjs_uint8)((b&0xf0)>>4);
		d[1] = (tjs_uint8)(b&0x0f);
		d += 2;
	}
	if(len & 1)
	{
		b = *buf;
		if(d<dlim) *d = (tjs_uint8)((b&0xf0)>>4);
	}
}
EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPBLExpand4BitTo32BitPal_c, (tjs_uint32 *dest, const tjs_uint8 *buf, tjs_int len, const tjs_uint32 *pal))
{
	tjs_uint32 *d=dest, *dlim;
	tjs_uint8 b;

	dlim = dest + (len & ~1);
	while(d < dlim)
	{
		b = *(buf++);
		d[0] = pal[(b&0xf0)>>4];
		d[1] = pal[b&0x0f];
		d += 2;
	}
	if(len & 1)
	{
		b = *buf;
		*d = pal[(b&0xf0)>>4];
	}
}
EOF


;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPBLExpand8BitTo8BitPal_c, (tjs_uint8 *dest, const tjs_uint8 *buf, tjs_int len, const tjs_uint32 *pal))
{
EOF


$content = <<EOF;
{
	dest[{ofs}] = pal[buf[{ofs}]]&0xff;
}
EOF

&loop_unroll_c_2($content, 'len', 4);

print FC <<EOF;
}

EOF


;#-----------------------------------------------------------------


print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPBLExpand8BitTo32BitPal_c, (tjs_uint32 *dest, const tjs_uint8 *buf, tjs_int len, const tjs_uint32 *pal))
{
EOF


$content = <<EOF;
{
	dest[{ofs}] = pal[buf[{ofs}]];
}
EOF

&loop_unroll_c_2($content, 'len', 8);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPExpand8BitTo32BitGray_c, (tjs_uint32 *dest, const tjs_uint8 *buf, tjs_int len))
{
	tjs_uint8 a, b;
EOF

$content = <<EOF;
	a = buf[{ofs}];;
	dest[{ofs}] = 0xff000000 + (a * 0x10101);;
EOF

$content2 = <<EOF;
	b = buf[{ofs}];;
	dest[{ofs}] = 0xff000000 + (b * 0x10101);;
EOF

&loop_unroll_c_int_2($content, $content2, 'len', 8);

print FC <<EOF;
}

EOF


;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPBLConvert15BitTo8Bit_c, (tjs_uint8 *dest, const tjs_uint16 *buf, tjs_int len))
{
EOF


$content = <<EOF;
{
#if TJS_HOST_IS_BIG_ENDIAN
	tjs_uint16 s = *(tjs_uint8*)(buf+{ofs}) << 8 + *((tjs_uint8*)(buf+{ofs})+1);
#else
	tjs_uint16 s = buf[{ofs}];
#endif
	dest[{ofs}] =
		((s&0x7c00)*56+ (s&0x03e0)*(187<<5)+ (s&0x001f)*(21<<10)) >> 15;
}
EOF

&loop_unroll_c_2($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPBLConvert15BitTo32Bit_c, (tjs_uint32 *dest, const tjs_uint16 *buf, tjs_int len))
{
EOF


$content = <<EOF;
{
#if TJS_HOST_IS_BIG_ENDIAN
	tjs_uint16 s = *(tjs_uint8*)(buf+{ofs}) << 8 + *((tjs_uint8*)(buf+{ofs})+1);
#else
	tjs_uint16 s = buf[{ofs}];
#endif
	tjs_int r = s&0x7c00;
	tjs_int g = s&0x03e0;
	tjs_int b = s&0x001f;
	dest[{ofs}] = 0xff000000 +
		(r <<  9) + ((r&0x7000)<<4) +
		(g <<  6) + ((g&0x0380)<<1) +
		(b <<  3) + (b>>2);
}
EOF

&loop_unroll_c_2($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------


print FC <<EOF;
# define compose_grayscale(r,g,b) ((unsigned char)((((tjs_int)(b)*19 + (tjs_int)(g)*183 + (tjs_int)(r)*54)>>8)))
/*export*/
TVP_GL_FUNC_DECL(void, TVPBLConvert24BitTo8Bit_c, (tjs_uint8 *dest, const tjs_uint8 *buf, tjs_int len))
{
	tjs_uint8 *slimglim = dest + len;
	tjs_uint8 *slimglims = slimglim - 3;
	while(dest < slimglims)
	{
		dest[0] = compose_grayscale(buf[2], buf[1], buf[0]);
		dest[1] = compose_grayscale(buf[5], buf[4], buf[3]);
		dest[2] = compose_grayscale(buf[8], buf[7], buf[6]);
		dest[3] = compose_grayscale(buf[11], buf[10], buf[9]);
		dest += 4;
		buf += 12;
	}
	while(dest < slimglim)
	{
		dest[0] = compose_grayscale(buf[2], buf[1], buf[0]);
		dest ++;
		buf += 3;
	}
}

EOF


;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPBLConvert24BitTo32Bit_c, (tjs_uint32 *dest, const tjs_uint8 *buf, tjs_int len))
{
	tjs_uint32 *slimglim = dest + len;
	tjs_uint32 *slimglims = slimglim - 7;
	while(dest < slimglims)
	{
#if TJS_HOST_IS_BIG_ENDIAN
		dest[0] = 0xff000000 + buf[0] + (buf[1]<<8) + (buf[2]<<16);
		dest[1] = 0xff000000 + buf[3] + (buf[4]<<8) + (buf[5]<<16);
		dest[2] = 0xff000000 + buf[6] + (buf[7]<<8) + (buf[8]<<16);
		dest[3] = 0xff000000 + buf[9] + (buf[10]<<8) + (buf[11]<<16);
		dest += 4;
		buf += 12;
		dest[0] = 0xff000000 + buf[0] + (buf[1]<<8) + (buf[2]<<16);
		dest[1] = 0xff000000 + buf[3] + (buf[4]<<8) + (buf[5]<<16);
		dest[2] = 0xff000000 + buf[6] + (buf[7]<<8) + (buf[8]<<16);
		dest[3] = 0xff000000 + buf[9] + (buf[10]<<8) + (buf[11]<<16);
		dest += 4;
		buf += 12;
#else
		tjs_uint32 a = *(tjs_uint32*)buf, b;
		tjs_uint32 c = *(tjs_uint32*)(buf+12), d;
		dest[0] = 0xff000000 + (a & 0x00ffffff);
		dest[4] = 0xff000000 + (c & 0x00ffffff);
		b = *(tjs_uint32*)(buf+4);
		d = *(tjs_uint32*)(buf+16);
		dest[1] = 0xff000000 + ((a >> 24) + ((b & 0xffff)<<8));
		dest[5] = 0xff000000 + ((c >> 24) + ((d & 0xffff)<<8));
		a = *(tjs_uint32*)(buf+8);
		c = *(tjs_uint32*)(buf+20);
		dest[2] = 0xff000000 + ((b >> 16) + ((a & 0xff)<<16));
		dest[6] = 0xff000000 + ((d >> 16) + ((c & 0xff)<<16));
		dest[3] = 0xff000000 + (a >> 8);
		dest[7] = 0xff000000 + (c >> 8);
		dest += 8;
		buf += 24;
#endif
	}
	while(dest < slimglim)
	{
#if TJS_HOST_IS_BIG_ENDIAN
		*(dest++) = 0xff000000 + buf[0] + (buf[1]<<8) + (buf[2]<<16);
#else
		*(dest++) = 0xff000000 + buf[0] + (buf[1]<<8) + (buf[2]<<16);
#endif
		buf += 3;
	}
}
EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPConvert24BitTo32Bit_c, (tjs_uint32 *dest, const tjs_uint8 *buf, tjs_int len))
{
	/* this function does not matter the host endian */
	tjs_uint32 *slimglim = dest + len;
	tjs_uint32 *slimglims = slimglim - 7;
	while(dest < slimglims)
	{
		tjs_uint32 a = *(tjs_uint32*)buf, b;
		tjs_uint32 c = *(tjs_uint32*)(buf+12), d;
		dest[0] = 0xff000000 + (a & 0x00ffffff);
		dest[4] = 0xff000000 + (c & 0x00ffffff);
		b = *(tjs_uint32*)(buf+4);
		d = *(tjs_uint32*)(buf+16);
		dest[1] = 0xff000000 + ((a >> 24) + ((b & 0xffff)<<8));
		dest[5] = 0xff000000 + ((c >> 24) + ((d & 0xffff)<<8));
		a = *(tjs_uint32*)(buf+8);
		c = *(tjs_uint32*)(buf+20);
		dest[2] = 0xff000000 + ((b >> 16) + ((a & 0xff)<<16));
		dest[6] = 0xff000000 + ((d >> 16) + ((c & 0xff)<<16));
		dest[3] = 0xff000000 + (a >> 8);
		dest[7] = 0xff000000 + (c >> 8);
		dest += 8;
		buf += 24;
	}
	while(dest < slimglim)
	{
		*(dest++) = 0xff000000 + buf[0] + (buf[1]<<8) + (buf[2]<<16);
		buf += 3;
	}
}
EOF

;#-----------------------------------------------------------------

print FC <<EOF;
/*export*/
TVP_GL_FUNC_DECL(void, TVPBLConvert32BitTo8Bit_c, (tjs_uint8 *dest, const tjs_uint32 *buf, tjs_int len))
{
EOF


$content = <<EOF;
{
#if TJS_HOST_IS_BIG_ENDIAN
	tjs_uint32 d = buf[{ofs}];
	dest[{ofs}] = compose_grayscale(d&0xff, (d&0xff00)>>8, (d&0xff0000)>>16);
#else
	tjs_uint32 d = buf[{ofs}];
	dest[{ofs}] = compose_grayscale((d&0xff0000)>>16, (d&0xff00)>>8, d&0xff);
#endif
}
EOF

&loop_unroll_c_2($content, 'len', 4);

print FC <<EOF;
}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;

/*export*/
TVP_GL_FUNC_DECL(void, TVPBLConvert32BitTo32Bit_NoneAlpha_c, (tjs_uint32 *dest, const tjs_uint32 *buf, tjs_int len))
{
EOF


$content = <<EOF;
{
#if TJS_HOST_IS_BIG_ENDIAN
	tjs_uint32 d = buf[{ofs}];
	dest[{ofs}] = 0xff000000 + ((d&0xff00)<<8) +  ((d&0xff0000)>>8) + ((d&0xff000000)>>24);
#else
	tjs_uint32 d = buf[{ofs}];
	dest[{ofs}] = d | 0xff000000;
#endif
}
EOF

&loop_unroll_c_2($content, 'len', 4);

print FC <<EOF;

}

EOF

;#-----------------------------------------------------------------


print FC <<EOF;

/*export*/
TVP_GL_FUNC_DECL(void, TVPBLConvert32BitTo32Bit_MulAddAlpha_c, (tjs_uint32 *dest, const tjs_uint32 *buf, tjs_int len))
{
EOF


$content = <<EOF;
{
#if TJS_HOST_IS_BIG_ENDIAN
	tjs_uint32 d = buf[{ofs}];
	dest[{ofs}] = ((d&0xff)<<24) + ((d&0xff00)<<8) +  ((d&0xff0000)>>8) + ((d&0xff000000)>>24);
#else
	tjs_uint32 d = buf[{ofs}];
	dest[{ofs}] = d;
#endif
}
EOF

&loop_unroll_c_2($content, 'len', 4);

print FC <<EOF;

}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;

/*export*/
TVP_GL_FUNC_DECL(void, TVPBLConvert32BitTo32Bit_AddAlpha_c, (tjs_uint32 *dest, const tjs_uint32 *buf, tjs_int len))
{
EOF

$content = <<EOF;
{
#if TJS_HOST_IS_BIG_ENDIAN
	tjs_uint32 d = buf[{ofs}];
	tjs_uint8 *t = TVPDivTable + ((d & 0xff)<<8);
	dest[{ofs}] = ((d&0xff)<<24) + (t[(d&0xff00)>>8]<<16) +  (t[(d&0xff0000)>>16]<<8) + (t[(d&0xff000000)>>24]);
#else
	tjs_uint32 d = buf[{ofs}];
	tjs_uint8 *t = TVPDivTable + ((d>>16) & 0xff00);
	dest[{ofs}] = (d&0xff000000) + (t[(d&0xff0000)>>16]<<16) + (t[(d&0xff00)>>8]<<8) + (t[d&0xff]);
#endif
}
EOF

&loop_unroll_c_2($content, 'len', 4);

print FC <<EOF;

}

EOF

;#-----------------------------------------------------------------
;# conversion of 32bpp->16bpp(565/555) with dithering
;#-----------------------------------------------------------------


print FC <<EOF;

/*export*/
TVP_GL_FUNC_DECL(void, TVPDither32BitTo16Bit565_c, (tjs_uint16 *dest, const tjs_uint32 *src, tjs_int len, tjs_int xofs, tjs_int yofs))
{

tjs_uint8 *line = TVPDitherTable_5_6[yofs & 0x03][0][0];
tjs_int x = (xofs & 0x03) << 9;


EOF

$content = <<EOF;
{
tjs_uint32 v = *src;
*dest = (line[x + ((v >> 16) & 0xff)] << 11)+  (line[x + (v & 0xff)]) +
	(line[x + 256 + ((v >> 8) & 0xff)] << 5);
dest++;
src++;
x+= 0x200;
x &= 0x600;
}
EOF

&loop_unroll_c_2($content, 'len', 4);

print FC <<EOF;

}

EOF

;#-----------------------------------------------------------------

print FC <<EOF;

/*export*/
TVP_GL_FUNC_DECL(void, TVPDither32BitTo16Bit555_c, (tjs_uint16 *dest, const tjs_uint32 *src, tjs_int len, tjs_int xofs, tjs_int yofs))
{

tjs_uint8 *line = TVPDitherTable_5_6[yofs & 0x03][0][0];
tjs_int x = (xofs & 0x03) << 9;


EOF

$content = <<EOF;
{
tjs_uint32 v = *src;
*dest = (line[x + ((v >> 16) & 0xff)] << 10) + (line[x + (v & 0xff)]) +
	(line[x + ((v >> 8) & 0xff)] << 5);
dest++;
src++;
x+= 0x200;
x &= 0x600;
}
EOF

&loop_unroll_c_2($content, 'len', 4);

print FC <<EOF;

}

EOF

;#-----------------------------------------------------------------
;# conversion of 32bpp->8bpp(6*7*6) with dithering
;#-----------------------------------------------------------------


print FC <<EOF;

/*export*/
TVP_GL_FUNC_DECL(void, TVPDither32BitTo8Bit_c, (tjs_uint8 *dest, const tjs_uint32 *src, tjs_int len, tjs_int xofs, tjs_int yofs))
{

tjs_uint8 *line = &(TVPDitherTable_676[0][yofs & 0x03][0][0]);
tjs_int x = (xofs & 0x03) << 8;


EOF

$content = <<EOF;
{
tjs_uint32 v = *src;
*dest = (line[x + ((v >> 16) & 0xff)])+ (line[(256 * 16 * 2) + x + (v & 0xff)]) +
	(line[(16 * 256) + x + ((v >> 8) & 0xff)]);
dest++;
src++;
x += 0x100;
x &= 0x300;
}
EOF

&loop_unroll_c_2($content, 'len', 4);

print FC <<EOF;

}

EOF



;#-----------------------------------------------------------------
;# tlg5 lossless graphics decompressor
;#-----------------------------------------------------------------

print FC <<EOF;


/*export*/
TVP_GL_FUNC_DECL(void, TVPTLG5ComposeColors3To4_c, (tjs_uint8 *outp, const tjs_uint8 *upper, tjs_uint8 * const * buf, tjs_int width))
{
	tjs_int x;
	tjs_uint8 pc[3];
	tjs_uint8 c[3];
	pc[0] = pc[1] = pc[2] = 0;
	for(x = 0; x < width; x++)
	{
		c[0] = buf[0][x];
		c[1] = buf[1][x];
		c[2] = buf[2][x];
		c[0] += c[1]; c[2] += c[1];
		*(tjs_uint32 *)outp =
								((((pc[0] += c[0]) + upper[0]) & 0xff)      ) +
								((((pc[1] += c[1]) + upper[1]) & 0xff) <<  8) +
								((((pc[2] += c[2]) + upper[2]) & 0xff) << 16) +
								0xff000000;
		outp += 4;
		upper += 4;
	}
}

/*export*/
TVP_GL_FUNC_DECL(void, TVPTLG5ComposeColors4To4_c, (tjs_uint8 *outp, const tjs_uint8 *upper, tjs_uint8 * const* buf, tjs_int width))
{
	tjs_int x;
	tjs_uint8 pc[4];
	tjs_uint8 c[4];
	pc[0] = pc[1] = pc[2] = pc[3] = 0;
	for(x = 0; x < width; x++)
	{
		c[0] = buf[0][x];
		c[1] = buf[1][x];
		c[2] = buf[2][x];
		c[3] = buf[3][x];
		c[0] += c[1]; c[2] += c[1];
		*(tjs_uint32 *)outp =
								((((pc[0] += c[0]) + upper[0]) & 0xff)      ) +
								((((pc[1] += c[1]) + upper[1]) & 0xff) <<  8) +
								((((pc[2] += c[2]) + upper[2]) & 0xff) << 16) +
								((((pc[3] += c[3]) + upper[3]) & 0xff) << 24);
		outp += 4;
		upper += 4;
	}
}

/*export*/
TVP_GL_FUNC_DECL(tjs_int, TVPTLG5DecompressSlide_c, (tjs_uint8 *out, const tjs_uint8 *in, tjs_int insize, tjs_uint8 *text, tjs_int initialr))
{
	tjs_int r = initialr;
	tjs_uint flags = 0;
	const tjs_uint8 *inlim = in + insize;
	while(in < inlim)
	{
		if(((flags >>= 1) & 256) == 0)
		{
			flags = 0[in++] | 0xff00;
		}
		if(flags & 1)
		{
			tjs_int mpos = in[0] | ((in[1] & 0xf) << 8);
			tjs_int mlen = (in[1] & 0xf0) >> 4;
			in += 2;
			mlen += 3;
			if(mlen == 18) mlen += 0[in++];

			while(mlen--)
			{
				0[out++] = text[r++] = text[mpos++];
				mpos &= (4096 - 1);
				r &= (4096 - 1);
			}
		}
		else
		{
			unsigned char c = 0[in++];
			0[out++] = c;
			text[r++] = c;
/*			0[out++] = text[r++] = 0[in++];*/
			r &= (4096 - 1);
		}
	}
	return r;
}

EOF


;#-----------------------------------------------------------------
;# tlg6 lossless/near-lossless graphics decompressor
;#-----------------------------------------------------------------

print FC <<EOF;

#if TJS_HOST_IS_BIG_ENDIAN
	#define TVP_TLG6_BYTEOF(a, x) (((tjs_uint8*)(a))[(x)])

	#define TVP_TLG6_FETCH_32BITS(addr) ((tjs_uint32)TVP_TLG6_BYTEOF((addr), 0) +  \\
									((tjs_uint32)TVP_TLG6_BYTEOF((addr), 1) << 8) + \\
									((tjs_uint32)TVP_TLG6_BYTEOF((addr), 2) << 16) + \\
									((tjs_uint32)TVP_TLG6_BYTEOF((addr), 3) << 24) )
#else
	#define TVP_TLG6_FETCH_32BITS(addr) (*(tjs_uint32*)addr)
#endif



/*export*/
TVP_GL_FUNC_DECL(void, TVPTLG6DecodeGolombValuesForFirst_c, (tjs_int8 *pixelbuf, tjs_int pixel_count, tjs_uint8 *bit_pool))
{
	/*
		decode values packed in "bit_pool".
		values are coded using golomb code.

		"ForFirst" function do dword access to pixelbuf,
		clearing with zero except for blue (least siginificant byte).
	*/

	int n = TVP_TLG6_GOLOMB_N_COUNT - 1; /* output counter */
	int a = 0; /* summary of absolute values of errors */

	tjs_int bit_pos = 1;
	tjs_uint8 zero = (*bit_pool & 1)?0:1;

	tjs_int8 * limit = pixelbuf + pixel_count*4;

	while(pixelbuf < limit)
	{
		/* get running count */
		int count;

		{
			tjs_uint32 t = TVP_TLG6_FETCH_32BITS(bit_pool) >> bit_pos;
			tjs_int b = TVPTLG6LeadingZeroTable[t&(TVP_TLG6_LeadingZeroTable_SIZE-1)];
			int bit_count = b;
			while(!b)
			{
				bit_count += TVP_TLG6_LeadingZeroTable_BITS;
				bit_pos += TVP_TLG6_LeadingZeroTable_BITS;
				bit_pool += bit_pos >> 3;
				bit_pos &= 7;
				t = TVP_TLG6_FETCH_32BITS(bit_pool) >> bit_pos;
				b = TVPTLG6LeadingZeroTable[t&(TVP_TLG6_LeadingZeroTable_SIZE-1)];
				bit_count += b;
			}


			bit_pos += b;
			bit_pool += bit_pos >> 3;
			bit_pos &= 7;

			bit_count --;
			count = 1 << bit_count;
			count += ((TVP_TLG6_FETCH_32BITS(bit_pool) >> (bit_pos)) & (count-1));

			bit_pos += bit_count;
			bit_pool += bit_pos >> 3;
			bit_pos &= 7;

		}

		if(zero)
		{
			/* zero values */

			/* fill distination with zero */
			do { *(tjs_uint32*)pixelbuf = 0; pixelbuf+=4; } while(--count);

			zero ^= 1;
		}
		else
		{
			/* non-zero values */

			/* fill distination with glomb code */

			do
			{
				int k = TVPTLG6GolombBitLengthTable[a][n], v, sign;

				tjs_uint32 t = TVP_TLG6_FETCH_32BITS(bit_pool) >> bit_pos;
				tjs_int bit_count;
				tjs_int b;
				if(t)
				{
					b = TVPTLG6LeadingZeroTable[t&(TVP_TLG6_LeadingZeroTable_SIZE-1)];
					bit_count = b;
					while(!b)
					{
						bit_count += TVP_TLG6_LeadingZeroTable_BITS;
						bit_pos += TVP_TLG6_LeadingZeroTable_BITS;
						bit_pool += bit_pos >> 3;
						bit_pos &= 7;
						t = TVP_TLG6_FETCH_32BITS(bit_pool) >> bit_pos;
						b = TVPTLG6LeadingZeroTable[t&(TVP_TLG6_LeadingZeroTable_SIZE-1)];
						bit_count += b;
					}
					bit_count --;
				}
				else
				{
					bit_pool += 5;
					bit_count = bit_pool[-1];
					bit_pos = 0;
					t = TVP_TLG6_FETCH_32BITS(bit_pool);
					b = 0;
				}


				v = (bit_count << k) + ((t >> b) & ((1<<k)-1));
				sign = (v & 1) - 1;
				v >>= 1;
				a += v;
				*(tjs_uint32*)pixelbuf = (unsigned char) ((v ^ sign) + sign + 1);
				pixelbuf += 4;

				bit_pos += b;
				bit_pos += k;
				bit_pool += bit_pos >> 3;
				bit_pos &= 7;

				if (--n < 0) {
					a >>= 1;  n = TVP_TLG6_GOLOMB_N_COUNT - 1;
				}
			} while(--count);
			zero ^= 1;
		}
	}
}

/*export*/
TVP_GL_FUNC_DECL(void, TVPTLG6DecodeGolombValues_c, (tjs_int8 *pixelbuf, tjs_int pixel_count, tjs_uint8 *bit_pool))
{
	/*
		decode values packed in "bit_pool".
		values are coded using golomb code.
	*/

	int n = TVP_TLG6_GOLOMB_N_COUNT - 1; /* output counter */
	int a = 0; /* summary of absolute values of errors */

	tjs_int bit_pos = 1;
	tjs_uint8 zero = (*bit_pool & 1)?0:1;

	tjs_int8 * limit = pixelbuf + pixel_count*4;

	while(pixelbuf < limit)
	{
		/* get running count */
		int count;

		{
			tjs_uint32 t = TVP_TLG6_FETCH_32BITS(bit_pool) >> bit_pos;
			tjs_int b = TVPTLG6LeadingZeroTable[t&(TVP_TLG6_LeadingZeroTable_SIZE-1)];
			int bit_count = b;
			while(!b)
			{
				bit_count += TVP_TLG6_LeadingZeroTable_BITS;
				bit_pos += TVP_TLG6_LeadingZeroTable_BITS;
				bit_pool += bit_pos >> 3;
				bit_pos &= 7;
				t = TVP_TLG6_FETCH_32BITS(bit_pool) >> bit_pos;
				b = TVPTLG6LeadingZeroTable[t&(TVP_TLG6_LeadingZeroTable_SIZE-1)];
				bit_count += b;
			}


			bit_pos += b;
			bit_pool += bit_pos >> 3;
			bit_pos &= 7;

			bit_count --;
			count = 1 << bit_count;
			count += ((TVP_TLG6_FETCH_32BITS(bit_pool) >> (bit_pos)) & (count-1));

			bit_pos += bit_count;
			bit_pool += bit_pos >> 3;
			bit_pos &= 7;

		}

		if(zero)
		{
			/* zero values */

			/* fill distination with zero */
			do { *pixelbuf = 0; pixelbuf+=4; } while(--count);

			zero ^= 1;
		}
		else
		{
			/* non-zero values */

			/* fill distination with glomb code */

			do
			{
				int k = TVPTLG6GolombBitLengthTable[a][n], v, sign;

				tjs_uint32 t = TVP_TLG6_FETCH_32BITS(bit_pool) >> bit_pos;
				tjs_int bit_count;
				tjs_int b;
				if(t)
				{
					b = TVPTLG6LeadingZeroTable[t&(TVP_TLG6_LeadingZeroTable_SIZE-1)];
					bit_count = b;
					while(!b)
					{
						bit_count += TVP_TLG6_LeadingZeroTable_BITS;
						bit_pos += TVP_TLG6_LeadingZeroTable_BITS;
						bit_pool += bit_pos >> 3;
						bit_pos &= 7;
						t = TVP_TLG6_FETCH_32BITS(bit_pool) >> bit_pos;
						b = TVPTLG6LeadingZeroTable[t&(TVP_TLG6_LeadingZeroTable_SIZE-1)];
						bit_count += b;
					}
					bit_count --;
				}
				else
				{
					bit_pool += 5;
					bit_count = bit_pool[-1];
					bit_pos = 0;
					t = TVP_TLG6_FETCH_32BITS(bit_pool);
					b = 0;
				}


				v = (bit_count << k) + ((t >> b) & ((1<<k)-1));
				sign = (v & 1) - 1;
				v >>= 1;
				a += v;
				*pixelbuf = (char) ((v ^ sign) + sign + 1);
				pixelbuf += 4;

				bit_pos += b;
				bit_pos += k;
				bit_pool += bit_pos >> 3;
				bit_pos &= 7;

				if (--n < 0) {
					a >>= 1;  n = TVP_TLG6_GOLOMB_N_COUNT - 1;
				}
			} while(--count);
			zero ^= 1;
		}
	}
}


static TVP_INLINE_FUNC tjs_uint32 make_gt_mask(tjs_uint32 a, tjs_uint32 b){
	tjs_uint32 tmp2 = ~b;
	tjs_uint32 tmp = ((a & tmp2) + (((a ^ tmp2) >> 1) & 0x7f7f7f7f) ) & 0x80808080;
	tmp = ((tmp >> 7) + 0x7f7f7f7f) ^ 0x7f7f7f7f;
	return tmp;
}
static TVP_INLINE_FUNC tjs_uint32 packed_bytes_add(tjs_uint32 a, tjs_uint32 b)
{
	tjs_uint32 tmp = (((a & b)<<1) + ((a ^ b) & 0xfefefefe) ) & 0x01010100;
	return a+b-tmp;
}
static TVP_INLINE_FUNC tjs_uint32 med2(tjs_uint32 a, tjs_uint32 b, tjs_uint32 c){
	/* do Median Edge Detector   thx, Mr. sugi  at    kirikiri.info */
	tjs_uint32 aa_gt_bb = make_gt_mask(a, b);
	tjs_uint32 a_xor_b_and_aa_gt_bb = ((a ^ b) & aa_gt_bb);
	tjs_uint32 aa = a_xor_b_and_aa_gt_bb ^ a;
	tjs_uint32 bb = a_xor_b_and_aa_gt_bb ^ b;
	tjs_uint32 n = make_gt_mask(c, bb);
	tjs_uint32 nn = make_gt_mask(aa, c);
	tjs_uint32 m = ~(n | nn);
	return (n & aa) | (nn & bb) | ((bb & m) - (c & m) + (aa & m));
}
static TVP_INLINE_FUNC tjs_uint32 med(tjs_uint32 a, tjs_uint32 b, tjs_uint32 c, tjs_uint32 v){
	return packed_bytes_add(med2(a, b, c), v);
}

#define TLG6_AVG_PACKED(x, y) ((((x) & (y)) + ((((x) ^ (y)) & 0xfefefefe) >> 1)) +\\
			(((x)^(y))&0x01010101))

static TVP_INLINE_FUNC tjs_uint32 avg(tjs_uint32 a, tjs_uint32 b, tjs_uint32 c, tjs_uint32 v){
	return packed_bytes_add(TLG6_AVG_PACKED(a, b), v);
}

#define TVP_TLG6_DO_CHROMA_DECODE_PROTO(B, G, R, A, POST_INCREMENT) do \\
			{ \\
				tjs_uint32 u = *prevline; \\
				p = med(p, u, up, \\
					(0xff0000 & ((R)<<16)) + (0xff00 & ((G)<<8)) + (0xff & (B)) + ((A) << 24) ); \\
				up = u; \\
				*curline = p; \\
				curline ++; \\
				prevline ++; \\
				POST_INCREMENT \\
			} while(--w);
#define TVP_TLG6_DO_CHROMA_DECODE_PROTO2(B, G, R, A, POST_INCREMENT) do \\
			{ \\
				tjs_uint32 u = *prevline; \\
				p = avg(p, u, up, \\
					(0xff0000 & ((R)<<16)) + (0xff00 & ((G)<<8)) + (0xff & (B)) + ((A) << 24) ); \\
				up = u; \\
				*curline = p; \\
				curline ++; \\
				prevline ++; \\
				POST_INCREMENT \\
			} while(--w);
#define TVP_TLG6_DO_CHROMA_DECODE(N, R, G, B) case (N<<1): \\
	TVP_TLG6_DO_CHROMA_DECODE_PROTO(R, G, B, IA, {in+=step;}) break; \\
	case (N<<1)+1: \\
	TVP_TLG6_DO_CHROMA_DECODE_PROTO2(R, G, B, IA, {in+=step;}) break;

/*export*/
TVP_GL_FUNC_DECL(void, TVPTLG6DecodeLineGeneric_c, (tjs_uint32 *prevline, tjs_uint32 *curline, tjs_int width, tjs_int start_block, tjs_int block_limit, tjs_uint8 *filtertypes, tjs_int skipblockbytes, tjs_uint32 *in, tjs_uint32 initialp, tjs_int oddskip, tjs_int dir))
{
	/*
		chroma/luminosity decoding
		(this does reordering, color correlation filter, MED/AVG  at a time)
	*/
	tjs_uint32 p, up;
	int step, i;

	if(start_block)
	{
		prevline += start_block * TVP_TLG6_W_BLOCK_SIZE;
		curline  += start_block * TVP_TLG6_W_BLOCK_SIZE;
		p  = curline[-1];
		up = prevline[-1];
	}
	else
	{
		p = up = initialp;
	}

	in += skipblockbytes * start_block;
	step = (dir&1)?1:-1;

	for(i = start_block; i < block_limit; i ++)
	{
		int w = width - i*TVP_TLG6_W_BLOCK_SIZE, ww;
		if(w > TVP_TLG6_W_BLOCK_SIZE) w = TVP_TLG6_W_BLOCK_SIZE;
		ww = w;
		if(step==-1) in += ww-1;
		if(i&1) in += oddskip * ww;
		switch(filtertypes[i])
		{
#define IA	(char)((*in>>24)&0xff)
#define IR	(char)((*in>>16)&0xff)
#define IG  (char)((*in>>8 )&0xff)
#define IB  (char)((*in    )&0xff)
		TVP_TLG6_DO_CHROMA_DECODE( 0, IB, IG, IR); 
		TVP_TLG6_DO_CHROMA_DECODE( 1, IB+IG, IG, IR+IG); 
		TVP_TLG6_DO_CHROMA_DECODE( 2, IB, IG+IB, IR+IB+IG); 
		TVP_TLG6_DO_CHROMA_DECODE( 3, IB+IR+IG, IG+IR, IR); 
		TVP_TLG6_DO_CHROMA_DECODE( 4, IB+IR, IG+IB+IR, IR+IB+IR+IG); 
		TVP_TLG6_DO_CHROMA_DECODE( 5, IB+IR, IG+IB+IR, IR); 
		TVP_TLG6_DO_CHROMA_DECODE( 6, IB+IG, IG, IR); 
		TVP_TLG6_DO_CHROMA_DECODE( 7, IB, IG+IB, IR); 
		TVP_TLG6_DO_CHROMA_DECODE( 8, IB, IG, IR+IG); 
		TVP_TLG6_DO_CHROMA_DECODE( 9, IB+IG+IR+IB, IG+IR+IB, IR+IB); 
		TVP_TLG6_DO_CHROMA_DECODE(10, IB+IR, IG+IR, IR); 
		TVP_TLG6_DO_CHROMA_DECODE(11, IB, IG+IB, IR+IB); 
		TVP_TLG6_DO_CHROMA_DECODE(12, IB, IG+IR+IB, IR+IB); 
		TVP_TLG6_DO_CHROMA_DECODE(13, IB+IG, IG+IR+IB+IG, IR+IB+IG); 
		TVP_TLG6_DO_CHROMA_DECODE(14, IB+IG+IR, IG+IR, IR+IB+IG+IR); 
		TVP_TLG6_DO_CHROMA_DECODE(15, IB, IG+(IB<<1), IR+(IB<<1));

		default: return;
		}
		if(step == 1)
			in += skipblockbytes - ww;
		else
			in += skipblockbytes + 1;
		if(i&1) in -= oddskip * ww;
#undef IR
#undef IG
#undef IB
	}
}

/*export*/
TVP_GL_FUNC_DECL(void, TVPTLG6DecodeLine_c, (tjs_uint32 *prevline, tjs_uint32 *curline, tjs_int width, tjs_int block_count, tjs_uint8 *filtertypes, tjs_int skipblockbytes, tjs_uint32 *in, tjs_uint32 initialp, tjs_int oddskip, tjs_int dir))
{
	TVPTLG6DecodeLineGeneric(prevline, curline, width, 0, block_count,
		filtertypes, skipblockbytes, in, initialp, oddskip, dir);
}


EOF

;#-----------------------------------------------------------------
;# process tvpps.c
;#-----------------------------------------------------------------

open PSC, "tvpps.c" or die;

$exportlist = '';
while($line = <PSC>)
{
	if($line =~ /^\#define\s+TVPPS_FUNC_NORM\s+(\w+)/)
	{
		;# normal function
		$exportlist .= "/*export*/\n";
		$exportlist .= "TVP_GL_FUNC_DECL(void, $1, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))\n{}\n";
	}
	elsif($line =~ /^\#define\s+TVPPS_FUNC_O\s+(\w+)/)
	{
		;# with opacity 
		$exportlist .= "/*export*/\n";
		$exportlist .= "TVP_GL_FUNC_DECL(void, $1, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))\n{}\n";
	}
	elsif($line =~ /^\#define\s+TVPPS_FUNC_HDA\s+(\w+)/)
	{
		;# with hda
		$exportlist .= "/*export*/\n";
		$exportlist .= "TVP_GL_FUNC_DECL(void, $1, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))\n{}\n";
	}
	elsif($line =~ /^\#define\s+TVPPS_FUNC_HDA_O\s+(\w+)/)
	{
		;# with hda opacity
		$exportlist .= "/*export*/\n";
		$exportlist .= "TVP_GL_FUNC_DECL(void, $1, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))\n{}\n";
	}

	print FC $line;
}

print FC "\n\n";
print FC "#if 0\n";
print FC "/* dummy definitions to be checked in gengl.pl */\n";
print FC "$exportlist\n";
print FC "#endif\n";
print FC "\n\n";

;#-----------------------------------------------------------------



;#-----------------------------------------------------------------
;# write the footer
;#-----------------------------------------------------------------

;#-----------------------------------------------------------------
close FC;

&get_function_list("tvpgl.c");

open FC, ">>tvpgl.c";

foreach $each (@function_list)
{
	$temp = $each;
	$temp =~ s/TVP_GL_FUNC_DECL\((.*?)\s*\,\s*(.*?)_c\s*\,/TVP_GL_FUNC_PTR_DECL\($1\, $2\, /;
	print FC "$temp;\n";
}


print FC <<EOF;

/* suffix "_c" : function is written in C */

/*export*/
TVP_GL_FUNC_DECL(void, TVPInitTVPGL, ())
{
EOF

foreach $each (@function_list)
{
	$temp = $each;
	$temp =~ /TVP_GL_FUNC_DECL\((.*?)\s*\,\s*(.*?)_c\s*\,/;
	print FC "\t${2} = ${2}_c;\n";
}

print FC <<EOF;

	TVPCreateTable();
}

/*export*/
TVP_GL_FUNC_DECL(void, TVPUninitTVPGL, ())
{
	TVPDestroyTable();
}
/*end of the file*/

EOF
close FC;
;#-----------------------------------------------------------------
print FH "/* begin function list */\n";

foreach $each (@function_list)
{
	$temp = $each;
	$temp =~ s/TVP_GL_FUNC_DECL\((.*?)\s*\,\s*(.*?)_c\s*\,/TVP_GL_FUNC_PTR_EXTERN_DECL\($1\, $2\, /;
	print FH "$temp;\n";
}
print FH "/* end function list */\n";

print FH <<EOF;

TVP_GL_FUNC_EXTERN_DECL(void, TVPInitTVPGL, ());
TVP_GL_FUNC_EXTERN_DECL(void, TVPUninitTVPGL, ());
/*[*/
#ifdef __cplusplus
 }
#endif
/*]*/
/* some utilities */
/*[*/
#define TVP_RGB2COLOR(r,g,b) ((((r)<<16) + ((g)<<8) + (b)) | 0xff000000)
#define TVP_RGBA2COLOR(r,g,b,a) \\
	(((a)<<24) +  (((r)<<16) + ((g)<<8) + (b)))
/*]*/

#endif
/* end of the file */
EOF
;#-----------------------------------------------------------------
<<COMMENT;
open FE, ">tvpgl_exporter.cpp";

print FE "/*\n$gpl\n*/\n";

print FE <<EOF;

/* TVP GL function exporter for plug-ins */
/* this file is always generated by gengl.pl rev. $rev */

#include "tjsCommHead.h"

#include "tvpgl.h"
#include "PluginImpl.h"

void TVPGLExportFunctions()
{
EOF

$count = 0;
foreach $each (@function_list)
{
	$temp = $each;
	$temp =~ /TVP_GL_FUNC_DECL\((.*?)\s*\,\s*(.*?)_c\s*\,/;
	print FE "\tTVPAddExportFunction(TJS_W(\"$2\"),  (void*)$2);\n";
	$count ++;
}


print FE <<EOF;
}
EOF

close FE;

open FE, ">tvpgl_exporter.h";
print FE "/*\n$gpl\n*/\n";

print FE <<EOF;

/* TVP GL function exporter for plug-ins */
/* this file is always generated by gengl.pl rev. $rev */


extern void TVPGLExportFunctions();

EOF
close FE;
COMMENT
;#-----------------------------------------------------------------
