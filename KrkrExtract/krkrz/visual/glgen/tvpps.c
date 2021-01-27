/*
  Photoshop-like layer blender for KIRIKIRI (C-version)
  (c)2004-2005 Kengo Takagi (Kenjo) <kenjo@ceres.dti.ne.jp>
*/


#define TVPPS_REG register
#define TVPPS_USE_OVERLAY_TABLE

/* --------------------------------------------------------------------
  Tables
  These three operations use pow()/divide, so table reference should be
  faster than normal calc.
-------------------------------------------------------------------- */
unsigned char TVPPsTableSoftLight[256][256];
unsigned char TVPPsTableColorDodge[256][256];
unsigned char TVPPsTableColorBurn[256][256];

#ifdef TVPPS_USE_OVERLAY_TABLE
/* only for C version */
unsigned char TVPPsTableOverlay[256][256];
#endif


/* --------------------------------------------------------------------
  Operation defines
-------------------------------------------------------------------- */
#define TVPPS_MAINLOOP \
		if(len > 0) {                                        \
			tjs_int lu_n = (len + (4-1)) / 4;                \
			switch(len % 4) {                                \
			case 0:                                          \
					do {                                     \
						OPERATION1;                          \
			case 3:		OPERATION1;                          \
			case 2:		OPERATION1;                          \
			case 1:		OPERATION1;                          \
					} while(-- lu_n);                        \
			}                                                \
		}

#define TVPPS_ALPHABLEND { \
        TVPPS_REG tjs_uint32 d1 = d&0x00ff00ff, d2 = d&0x0000ff00;                                         \
        s = ((((((s&0x00ff00ff)-d1)*a)>>8)+d1)&0x00ff00ff)|((((((s&0x0000ff00)-d2)*a)>>8)+d2)&0x0000ff00); \
}

#define TVPPS_FADESRC   s = ((((s&0x00ff00ff)*a)>>8)&0x00ff00ff)|((((s&0x0000ff00)*a)>>8)&0x0000ff00);


#define TVPPsOperationAlphaBlend      { \
        TVPPS_ALPHABLEND                                                                                   \
}
#if 0 /* Fade src BEFORE add/sub */
#define TVPPsOperationAddBlend        { \
        TVPPS_REG tjs_uint32 n;                                                                            \
        TVPPS_FADESRC                                                                                      \
        n = (((d&s)<<1)+((d^s)&0x00fefefe))&0x01010100;                                                    \
        n = ((n>>8)+0x007f7f7f)^0x007f7f7f;                                                                \
        s = (d+s-n)|n;                                                                                     \
}
#define TVPPsOperationSubBlend        { \
        TVPPS_REG tjs_uint32 n;                                                                            \
        TVPPS_FADESRC                                                                                      \
        s = ~s;                                                                                            \
        n = (((~d&s)<<1)+((~d^s)&0x00fefefe))&0x01010100;                                                  \
        n = ((n>>8)+0x007f7f7f)^0x007f7f7f;                                                                \
        s = (d|n)-(s|n);                                                                                   \
}
#else  /* Blend src and dst AFTER add/sub */
#define TVPPsOperationAddBlend        { \
        TVPPS_REG tjs_uint32 n;                                                                            \
        n = (((d&s)<<1)+((d^s)&0x00fefefe))&0x01010100;                                                    \
        n = ((n>>8)+0x007f7f7f)^0x007f7f7f;                                                                \
        s = (d+s-n)|n;                                                                                     \
        TVPPS_ALPHABLEND                                                                                   \
}
#define TVPPsOperationSubBlend        { \
        TVPPS_REG tjs_uint32 n;                                                                            \
        s = ~s;                                                                                            \
        n = (((~d&s)<<1)+((~d^s)&0x00fefefe))&0x01010100;                                                  \
        n = ((n>>8)+0x007f7f7f)^0x007f7f7f;                                                                \
        s = (d|n)-(s|n);                                                                                   \
        TVPPS_ALPHABLEND                                                                                   \
}
#endif
#define TVPPsOperationMulBlend        { \
        s = ( ((((d>>16)&0xff)*(s&0x00ff0000))&0xff000000) |                                               \
              ((((d>>8 )&0xff)*(s&0x0000ff00))&0x00ff0000) |                                               \
              ((((d>>0 )&0xff)*(s&0x000000ff))           ) ) >> 8;                                         \
        TVPPS_ALPHABLEND                                                                                   \
}
#define TVPPsOperationScreenBlend     { \
        /* c = ((s+d-(s*d)/255)-d)*a + d = (s-(s*d)/255)*a + d */                                          \
        TVPPS_REG tjs_uint32 sd1, sd2;                                                                     \
        sd1 = ( ((((d>>16)&0xff)*(s&0x00ff0000))&0xff000000) |                                             \
                ((((d>>0 )&0xff)*(s&0x000000ff))           ) ) >> 8;                                       \
        sd2 = ( ((((d>>8 )&0xff)*(s&0x0000ff00))&0x00ff0000) ) >> 8;                                       \
        s = ((((((s&0x00ff00ff)-sd1)*a)>>8)+(d&0x00ff00ff))&0x00ff00ff) |                                  \
            ((((((s&0x0000ff00)-sd2)*a)>>8)+(d&0x0000ff00))&0x0000ff00);                                   \
}
#ifdef TVPPS_USE_OVERLAY_TABLE
#define TVPPsOperationOverlayBlend    { \
        s = (TVPPsTableOverlay[(s>>16)&0xff][(d>>16)&0xff]<<16) |                                          \
            (TVPPsTableOverlay[(s>>8 )&0xff][(d>>8 )&0xff]<<8 ) |                                          \
            (TVPPsTableOverlay[(s>>0 )&0xff][(d>>0 )&0xff]<<0 );                                           \
        TVPPS_ALPHABLEND                                                                                   \
}
#define TVPPsOperationHardLightBlend  { \
        s = (TVPPsTableOverlay[(d>>16)&0xff][(s>>16)&0xff]<<16) |                                          \
            (TVPPsTableOverlay[(d>>8 )&0xff][(s>>8 )&0xff]<<8 ) |                                          \
            (TVPPsTableOverlay[(d>>0 )&0xff][(s>>0 )&0xff]<<0 );                                           \
        TVPPS_ALPHABLEND                                                                                   \
}
#else
#define TVPPsOperationOverlayBlend    { \
        TVPPS_REG tjs_uint32 n = (((d&0x00808080)>>7)+0x007f7f7f)^0x007f7f7f;                              \
        TVPPS_REG tjs_uint32 sa1, sa2, d1 = d&n, s1 = s&n;                                                 \
        /* some tricks to avoid overflow (error between /255 and >>8) */                                   \
        s |= 0x00010101;                                                                                   \
        sa1 = ( ((((d>>16)&0xff)*(s&0x00ff0000))&0xff800000) |                                             \
                ((((d>>0 )&0xff)*(s&0x000000ff))           ) ) >> 7;                                       \
        sa2 = ( ((((d>>8 )&0xff)*(s&0x0000ff00))&0x00ff8000) ) >> 7;                                       \
        s = ((sa1&~n)|(sa2&~n));                                                                           \
        s |= (((s1&0x00fe00fe)+(d1&0x00ff00ff))<<1)-(n&0x00ff00ff)-(sa1&n);                                \
        s |= (((s1&0x0000fe00)+(d1&0x0000ff00))<<1)-(n&0x0000ff00)-(sa2&n);                                \
        TVPPS_ALPHABLEND                                                                                   \
}
#define TVPPsOperationHardLightBlend  { \
        TVPPS_REG tjs_uint32 n = (((s&0x00808080)>>7)+0x007f7f7f)^0x007f7f7f;                              \
        TVPPS_REG tjs_uint32 sa1, sa2, d1 = d&n, s1 = s&n;                                                 \
        /* some tricks to avoid overflow (error between /255 and >>8) */                                   \
        d |= 0x00010101;                                                                                   \
        sa1 = ( ((((d>>16)&0xff)*(s&0x00ff0000))&0xff800000) |                                             \
                ((((d>>0 )&0xff)*(s&0x000000ff))           ) ) >> 7;                                       \
        sa2 = ( ((((d>>8 )&0xff)*(s&0x0000ff00))&0x00ff8000) ) >> 7;                                       \
        s = ((sa1&~n)|(sa2&~n));                                                                           \
        s |= (((s1&0x00ff00ff)+(d1&0x00fe00fe))<<1)-(n&0x00ff00ff)-(sa1&n);                                \
        s |= (((s1&0x0000ff00)+(d1&0x0000fe00))<<1)-(n&0x0000ff00)-(sa2&n);                                \
        TVPPS_ALPHABLEND                                                                                   \
}
#endif
#define TVPPsOperationSoftLightBlend  { \
        s = (TVPPsTableSoftLight[(s>>16)&0xff][(d>>16)&0xff]<<16) |                                        \
            (TVPPsTableSoftLight[(s>>8 )&0xff][(d>>8 )&0xff]<<8 ) |                                        \
            (TVPPsTableSoftLight[(s>>0 )&0xff][(d>>0 )&0xff]<<0 );                                         \
        TVPPS_ALPHABLEND                                                                                   \
}
#define TVPPsOperationColorDodgeBlend { \
        s = (TVPPsTableColorDodge[(s>>16)&0xff][(d>>16)&0xff]<<16) |                                       \
            (TVPPsTableColorDodge[(s>>8 )&0xff][(d>>8 )&0xff]<<8 ) |                                       \
            (TVPPsTableColorDodge[(s>>0 )&0xff][(d>>0 )&0xff]<<0 );                                        \
        TVPPS_ALPHABLEND                                                                                   \
}
#define TVPPsOperationColorDodge5Blend { \
        TVPPS_FADESRC                                                                                      \
        s = (TVPPsTableColorDodge[(s>>16)&0xff][(d>>16)&0xff]<<16) |                                       \
            (TVPPsTableColorDodge[(s>>8 )&0xff][(d>>8 )&0xff]<<8 ) |                                       \
            (TVPPsTableColorDodge[(s>>0 )&0xff][(d>>0 )&0xff]<<0 );                                        \
}
#define TVPPsOperationColorBurnBlend  { \
        s = (TVPPsTableColorBurn[(s>>16)&0xff][(d>>16)&0xff]<<16) |                                        \
            (TVPPsTableColorBurn[(s>>8 )&0xff][(d>>8 )&0xff]<<8 ) |                                        \
            (TVPPsTableColorBurn[(s>>0 )&0xff][(d>>0 )&0xff]<<0 );                                         \
        TVPPS_ALPHABLEND                                                                                   \
}
#define TVPPsOperationLightenBlend    { \
        TVPPS_REG tjs_uint32 n;                                                                            \
        n = (((~d&s)<<1)+((~d^s)&0x00fefefe))&0x01010100;                                                  \
        n = ((n>>8)+0x007f7f7f)^0x007f7f7f;                                                                \
        /* n=mask (d<s:0xff, d>=s:0x00) */                                                                 \
        s = (s&n)|(d&~n);                                                                                  \
        TVPPS_ALPHABLEND                                                                                   \
}
#define TVPPsOperationDarkenBlend     { \
        TVPPS_REG tjs_uint32 n;                                                                            \
        n = (((~d&s)<<1)+((~d^s)&0x00fefefe))&0x01010100;                                                  \
        n = ((n>>8)+0x007f7f7f)^0x007f7f7f;                                                                \
        /* n=mask (d<s:0xff, d>=s:0x00) */                                                                 \
        s = (d&n)|(s&~n);                                                                                  \
        TVPPS_ALPHABLEND                                                                                   \
}
#define TVPPsOperationDiffBlend       { \
        TVPPS_REG tjs_uint32 n;                                                                            \
        n = (((~d&s)<<1)+((~d^s)&0x00fefefe))&0x01010100;                                                  \
        n = ((n>>8)+0x007f7f7f)^0x007f7f7f;                                                                \
        /* n=mask (d<s:0xff, d>=s:0x00) */                                                                 \
        s = ((s&n)-(d&n))|((d&~n)-(s&~n));                                                                 \
        TVPPS_ALPHABLEND     /* Alphablend result & dst */                                                 \
}
#define TVPPsOperationDiff5Blend      { \
        TVPPS_REG tjs_uint32 n;                                                                            \
        TVPPS_FADESRC        /* Fade src first */                                                          \
        n = (((~d&s)<<1)+((~d^s)&0x00fefefe))&0x01010100;                                                  \
        n = ((n>>8)+0x007f7f7f)^0x007f7f7f;                                                                \
        /* n=mask (d<s:0xff, d>=s:0x00) */                                                                 \
        s = ((s&n)-(d&n))|((d&~n)-(s&~n));                                                                 \
}
#define TVPPsOperationExclusionBlend  { \
        /* c = ((s+d-(s*d*2)/255)-d)*a + d = (s-(s*d*2)/255)*a + d */                                      \
        TVPPS_REG tjs_uint32 sd1, sd2;                                                                     \
        sd1 = ( ((((d>>16)&0xff)*((s&0x00ff0000)>>7))&0x01ff0000) |                                        \
                ((((d>>0 )&0xff)*( s&0x000000ff    ))>>7        ) );                                       \
        sd2 = ( ((((d>>8 )&0xff)*(s&0x0000ff00))&0x00ff8000) ) >> 7;                                       \
        s = ((((((s&0x00ff00ff)-sd1)*a)>>8)+(d&0x00ff00ff))&0x00ff00ff) |                                  \
            ((((((s&0x0000ff00)-sd2)*a)>>8)+(d&0x0000ff00))&0x0000ff00);                                   \
}


/* --------------------------------------------------------------------
  Table initialize function
-------------------------------------------------------------------- */
void TVPPsMakeTable(void)
{
	int s, d;
	for (s=0; s<256; s++) {
		for (d=0; d<256; d++) {
			TVPPsTableSoftLight[s][d]  = (s>=128) ?
				( ((unsigned char)(pow(d/255.0, 128.0/s)*255.0)) ) :
				( ((unsigned char)(pow(d/255.0, (1.0-s/255.0)/0.5)*255.0)) );
			TVPPsTableColorDodge[s][d] = ((255-s)<=d) ? (0xff) : ((d*255)/(255-s));
			TVPPsTableColorBurn[s][d]  = (s<=(255-d)) ? (0x00) : (255-((255-d)*255)/s);
#ifdef TVPPS_USE_OVERLAY_TABLE
			TVPPsTableOverlay[s][d]  = (d<128) ? ((s*d*2)/255) : (((s+d)*2)-((s*d*2)/255)-255);
#endif
		}
	}
}


/* --------------------------------------------------------------------
  Function substances
-------------------------------------------------------------------- */
#define TVPPS_FUNC_NORM  TVPPsAlphaBlend_c
#define TVPPS_FUNC_O     TVPPsAlphaBlend_o_c
#define TVPPS_FUNC_HDA   TVPPsAlphaBlend_HDA_c
#define TVPPS_FUNC_HDA_O TVPPsAlphaBlend_HDA_o_c
#define TVPPS_OPERATION  TVPPsOperationAlphaBlend
#include "tvpps.inc"

#define TVPPS_FUNC_NORM  TVPPsAddBlend_c
#define TVPPS_FUNC_O     TVPPsAddBlend_o_c
#define TVPPS_FUNC_HDA   TVPPsAddBlend_HDA_c
#define TVPPS_FUNC_HDA_O TVPPsAddBlend_HDA_o_c
#define TVPPS_OPERATION  TVPPsOperationAddBlend
#include "tvpps.inc"

#define TVPPS_FUNC_NORM  TVPPsSubBlend_c
#define TVPPS_FUNC_O     TVPPsSubBlend_o_c
#define TVPPS_FUNC_HDA   TVPPsSubBlend_HDA_c
#define TVPPS_FUNC_HDA_O TVPPsSubBlend_HDA_o_c
#define TVPPS_OPERATION  TVPPsOperationSubBlend
#include "tvpps.inc"

#define TVPPS_FUNC_NORM  TVPPsMulBlend_c
#define TVPPS_FUNC_O     TVPPsMulBlend_o_c
#define TVPPS_FUNC_HDA   TVPPsMulBlend_HDA_c
#define TVPPS_FUNC_HDA_O TVPPsMulBlend_HDA_o_c
#define TVPPS_OPERATION  TVPPsOperationMulBlend
#include "tvpps.inc"

#define TVPPS_FUNC_NORM  TVPPsScreenBlend_c
#define TVPPS_FUNC_O     TVPPsScreenBlend_o_c
#define TVPPS_FUNC_HDA   TVPPsScreenBlend_HDA_c
#define TVPPS_FUNC_HDA_O TVPPsScreenBlend_HDA_o_c
#define TVPPS_OPERATION  TVPPsOperationScreenBlend
#include "tvpps.inc"

#define TVPPS_FUNC_NORM  TVPPsOverlayBlend_c
#define TVPPS_FUNC_O     TVPPsOverlayBlend_o_c
#define TVPPS_FUNC_HDA   TVPPsOverlayBlend_HDA_c
#define TVPPS_FUNC_HDA_O TVPPsOverlayBlend_HDA_o_c
#define TVPPS_OPERATION  TVPPsOperationOverlayBlend
#include "tvpps.inc"

#define TVPPS_FUNC_NORM  TVPPsHardLightBlend_c
#define TVPPS_FUNC_O     TVPPsHardLightBlend_o_c
#define TVPPS_FUNC_HDA   TVPPsHardLightBlend_HDA_c
#define TVPPS_FUNC_HDA_O TVPPsHardLightBlend_HDA_o_c
#define TVPPS_OPERATION  TVPPsOperationHardLightBlend
#include "tvpps.inc"

#define TVPPS_FUNC_NORM  TVPPsSoftLightBlend_c
#define TVPPS_FUNC_O     TVPPsSoftLightBlend_o_c
#define TVPPS_FUNC_HDA   TVPPsSoftLightBlend_HDA_c
#define TVPPS_FUNC_HDA_O TVPPsSoftLightBlend_HDA_o_c
#define TVPPS_OPERATION  TVPPsOperationSoftLightBlend
#include "tvpps.inc"

#define TVPPS_FUNC_NORM  TVPPsColorDodgeBlend_c
#define TVPPS_FUNC_O     TVPPsColorDodgeBlend_o_c
#define TVPPS_FUNC_HDA   TVPPsColorDodgeBlend_HDA_c
#define TVPPS_FUNC_HDA_O TVPPsColorDodgeBlend_HDA_o_c
#define TVPPS_OPERATION  TVPPsOperationColorDodgeBlend
#include "tvpps.inc"

#define TVPPS_FUNC_NORM  TVPPsColorDodge5Blend_c
#define TVPPS_FUNC_O     TVPPsColorDodge5Blend_o_c
#define TVPPS_FUNC_HDA   TVPPsColorDodge5Blend_HDA_c
#define TVPPS_FUNC_HDA_O TVPPsColorDodge5Blend_HDA_o_c
#define TVPPS_OPERATION  TVPPsOperationColorDodge5Blend
#include "tvpps.inc"

#define TVPPS_FUNC_NORM  TVPPsColorBurnBlend_c
#define TVPPS_FUNC_O     TVPPsColorBurnBlend_o_c
#define TVPPS_FUNC_HDA   TVPPsColorBurnBlend_HDA_c
#define TVPPS_FUNC_HDA_O TVPPsColorBurnBlend_HDA_o_c
#define TVPPS_OPERATION  TVPPsOperationColorBurnBlend
#include "tvpps.inc"

#define TVPPS_FUNC_NORM  TVPPsLightenBlend_c
#define TVPPS_FUNC_O     TVPPsLightenBlend_o_c
#define TVPPS_FUNC_HDA   TVPPsLightenBlend_HDA_c
#define TVPPS_FUNC_HDA_O TVPPsLightenBlend_HDA_o_c
#define TVPPS_OPERATION  TVPPsOperationLightenBlend
#include "tvpps.inc"

#define TVPPS_FUNC_NORM  TVPPsDarkenBlend_c
#define TVPPS_FUNC_O     TVPPsDarkenBlend_o_c
#define TVPPS_FUNC_HDA   TVPPsDarkenBlend_HDA_c
#define TVPPS_FUNC_HDA_O TVPPsDarkenBlend_HDA_o_c
#define TVPPS_OPERATION  TVPPsOperationDarkenBlend
#include "tvpps.inc"

#define TVPPS_FUNC_NORM  TVPPsDiffBlend_c
#define TVPPS_FUNC_O     TVPPsDiffBlend_o_c
#define TVPPS_FUNC_HDA   TVPPsDiffBlend_HDA_c
#define TVPPS_FUNC_HDA_O TVPPsDiffBlend_HDA_o_c
#define TVPPS_OPERATION  TVPPsOperationDiffBlend
#include "tvpps.inc"

#define TVPPS_FUNC_NORM  TVPPsDiff5Blend_c
#define TVPPS_FUNC_O     TVPPsDiff5Blend_o_c
#define TVPPS_FUNC_HDA   TVPPsDiff5Blend_HDA_c
#define TVPPS_FUNC_HDA_O TVPPsDiff5Blend_HDA_o_c
#define TVPPS_OPERATION  TVPPsOperationDiff5Blend
#include "tvpps.inc"

#define TVPPS_FUNC_NORM  TVPPsExclusionBlend_c
#define TVPPS_FUNC_O     TVPPsExclusionBlend_o_c
#define TVPPS_FUNC_HDA   TVPPsExclusionBlend_HDA_c
#define TVPPS_FUNC_HDA_O TVPPsExclusionBlend_HDA_o_c
#define TVPPS_OPERATION  TVPPsOperationExclusionBlend
#include "tvpps.inc"
