//---------------------------------------------------------------------------
/*
	Risa [りさ]      alias 吉里吉里3 [kirikiri-3]
	 stands for "Risa Is a Stagecraft Architecture"
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
//! @file
//! @brief XMMユーティリティ
//---------------------------------------------------------------------------

/*
	and

	the original copyright:
*/



/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2003             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: SSE Function Library
 last mod: $Id: xmmlib.c,v 1.4 2005-07-08 15:00:00+09 blacksword Exp $

 ********************************************************************/

/*
	Based on a file from Ogg Vorbis Optimization Project:
	 http://homepage3.nifty.com/blacksword/
*/

#include "tjsCommHead.h"

#define _USE_MATH_DEFINES
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include "xmmlib.h"
#if defined(_M_IX86)||defined(_M_X64)


//---------------------------------------------------------------------------


_ALIGN16(const tjs_uint32) PCS_NNRN[4]	 = {0x00000000, 0x80000000, 0x00000000, 0x00000000};
_ALIGN16(const tjs_uint32) PCS_NNRR[4]	 = {0x80000000, 0x80000000, 0x00000000, 0x00000000};
_ALIGN16(const tjs_uint32) PCS_NRNN[4]	 = {0x00000000, 0x00000000, 0x80000000, 0x00000000};
_ALIGN16(const tjs_uint32) PCS_NRNR[4]	 = {0x80000000, 0x00000000, 0x80000000, 0x00000000};
_ALIGN16(const tjs_uint32) PCS_NRRN[4]	 = {0x00000000, 0x80000000, 0x80000000, 0x00000000};
_ALIGN16(const tjs_uint32) PCS_NRRR[4]	 = {0x80000000, 0x80000000, 0x80000000, 0x00000000};
_ALIGN16(const tjs_uint32) PCS_RNNN[4]	 = {0x00000000, 0x00000000, 0x00000000, 0x80000000};
_ALIGN16(const tjs_uint32) PCS_RNRN[4]	 = {0x00000000, 0x80000000, 0x00000000, 0x80000000};
_ALIGN16(const tjs_uint32) PCS_RNRR[4]	 = {0x80000000, 0x80000000, 0x00000000, 0x80000000};
_ALIGN16(const tjs_uint32) PCS_RRNN[4]	 = {0x00000000, 0x00000000, 0x80000000, 0x80000000};
_ALIGN16(const tjs_uint32) PCS_RNNR[4]	 = {0x80000000, 0x00000000, 0x00000000, 0x80000000};
_ALIGN16(const tjs_uint32) PCS_RRRR[4]	 = {0x80000000, 0x80000000, 0x80000000, 0x80000000};
_ALIGN16(const tjs_uint32) PCS_NNNR[4]	 = {0x80000000, 0x00000000, 0x00000000, 0x00000000};
_ALIGN16(const tjs_uint32) PABSMASK[4]	 = {0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF};
_ALIGN16(const tjs_uint32) PSTARTEDGEM1[4]	 = {0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
_ALIGN16(const tjs_uint32) PSTARTEDGEM2[4]	 = {0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF};
_ALIGN16(const tjs_uint32) PSTARTEDGEM3[4]	 = {0x00000000, 0x00000000, 0x00000000, 0xFFFFFFFF};
_ALIGN16(const tjs_uint32) PENDEDGEM1[4]	 = {0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000};
_ALIGN16(const tjs_uint32) PENDEDGEM2[4]	 = {0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000};
_ALIGN16(const tjs_uint32) PENDEDGEM3[4]	 = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000};

_ALIGN16(const tjs_uint32) PMASKTABLE[4*16] = {
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0xFFFFFFFF, 0x00000000, 0x00000000,
	0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0xFFFFFFFF, 0x00000000,
	0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000,
	0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000,
	0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0xFFFFFFFF,
	0xFFFFFFFF, 0x00000000, 0x00000000, 0xFFFFFFFF,
	0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF,
	0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF,
	0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF
};

_ALIGN16(const float) PFV_0[4]		 = {0.0f,	0.0f,	0.0f,	0.0f};
_ALIGN16(const float) PFV_1[4]		 = {1.0f,	1.0f,	1.0f,	1.0f};
_ALIGN16(const float) PFV_2[4]		 = {2.0f,	2.0f,	2.0f,	2.0f};
_ALIGN16(const float) PFV_4[4]		 = {4.0f,	4.0f,	4.0f,	4.0f};
_ALIGN16(const float) PFV_8[4]		 = {8.0f,	8.0f,	8.0f,	8.0f};
_ALIGN16(const float) PFV_INIT[4]		 = {0.0f,	1.0f,	2.0f,	3.0f};
_ALIGN16(const float) PFV_0P5[4]		 = {0.5f,	0.5f,	0.5f,	0.5f};

//---------------------------------------------------------------------------

#endif
