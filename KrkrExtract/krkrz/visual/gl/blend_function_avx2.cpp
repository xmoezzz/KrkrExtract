


#include "tjsTypes.h"
#include "tvpgl.h"
#include "tvpgl_ia32_intf.h"
#include "simd_def_x86x64.h"

#include "blend_functor_avx2.h"
//#include "blend_ps_functor_avx2.h"
//#include "interpolation_functor_avx2.h"


extern "C" {
extern tjs_uint32 TVPCPUType;
extern unsigned char TVPDivTable[256*256];
extern unsigned char TVPOpacityOnOpacityTable[256*256];
extern unsigned char TVPNegativeMulTable[256*256];
extern unsigned char TVPOpacityOnOpacityTable65[65*256];
extern unsigned char TVPNegativeMulTable65[65*256];
extern unsigned char TVPDitherTable_5_6[8][4][2][256];
extern unsigned char TVPDitherTable_676[3][4][4][256];
extern unsigned char TVP252DitherPalette[3][256];
extern tjs_uint32 TVPRecipTable256[256];
extern tjs_uint16 TVPRecipTable256_16[256];
}
// AVX2 世代になるとアライメントはあまり気にしなくても速度差少ない、アライメント揃える処理のオーバーヘッドの方が大きいことも
// SSE2 でも、Core i系のNehalem以降は同じ状況だが、それより前のことも考慮してアライメント揃える処理を入れている
template<typename functor>
static inline void blend_func_avx2( tjs_uint32 * __restrict dest, const tjs_uint32 * __restrict src, tjs_int len, const functor& func ) {
	if( len <= 0 ) return;

	tjs_uint32 rem = (len>>3)<<3;
	tjs_uint32* limit = dest + rem;
	while( dest < limit ) {
		__m256i md = _mm256_loadu_si256( (__m256i const*)dest );
		__m256i ms = _mm256_loadu_si256( (__m256i const*)src );
		_mm256_storeu_si256( (__m256i*)dest, func( md, ms ) );
		dest+=8; src+=8;
	}
	limit += (len-rem);
	while( dest < limit ) {
		*dest = func( *dest, *src );
		dest++; src++;
	}
}
template<typename functor>
static void copy_func_avx2( tjs_uint32 * __restrict dest, const tjs_uint32 * __restrict src, tjs_int len ) {
	functor func;
	blend_func_avx2<functor>( dest, src, len, func );
}

// src と dest が重複している可能性のあるもの
template<typename functor>
static inline void overlap_blend_func_avx2( tjs_uint32 * dest, const tjs_uint32 * src, tjs_int len, const functor& func ) {
	if( len <= 0 ) return;
	
	const tjs_uint32 *src_end = src + len;
	if( dest > src && dest < src_end ) {
		// backward オーバーラップするので後ろからコピー
		tjs_int remain = (len>>3)<<3;	// 8未満の端数カット
		len--;
		while( len >= remain ) {
			dest[len] = func( dest[len], src[len] );
			len--;
		}
		while( len >= 0 ) {
			// 8ピクセルずつコピー
			__m256i md = _mm256_loadu_si256( (__m256i const*)&(dest[len-7]) );
			__m256i ms = _mm256_loadu_si256( (__m256i const*)&(src[len-7]) );
			md = func( md, ms );
			_mm256_storeu_si256( (__m256i*)&(dest[len-7]), md );
			len -= 8;
		}
	} else {
		// forward
		blend_func_avx2<functor>( dest, src, len, func );
	}
}
template<typename functor>
static void overlap_copy_func_avx2( tjs_uint32 * __restrict dest, const tjs_uint32 * __restrict src, tjs_int len ) {
	functor func;
	overlap_blend_func_avx2<functor>( dest, src, len, func );
}
// dest = src1 * src2 となっているもの
template<typename functor>
static inline void sd_blend_func_avx2( tjs_uint32 *dest, const tjs_uint32 *src1, const tjs_uint32 *src2, tjs_int len, const functor& func ) {
	if( len <= 0 ) return;

	tjs_uint32 rem = (len>>3)<<3;
	tjs_uint32* limit = dest + rem;
	while( dest < limit ) {
		__m256i ms1 = _mm256_loadu_si256( (__m256i const*)src1 );
		__m256i ms2 = _mm256_loadu_si256( (__m256i const*)src2 );
		_mm256_storeu_si256( (__m256i*)dest, func( ms1, ms2 ) );
		dest+=8; src1+=8; src2+=8;
	}
	limit += (len-rem);
	while( dest < limit ) {
		*dest = func( *src1, *src2 );
		dest++; src1++; src2++;
	}
}

// 完全透明ではコピーせず、完全不透明はそのままコピーする
template<typename functor>
static void blend_src_branch_func_avx2( tjs_uint32 * __restrict dest, const tjs_uint32 * __restrict src, tjs_int len, const functor& func ) {
	if( len <= 0 ) return;

	tjs_uint32 rem = (len>>3)<<3;
	tjs_uint32* limit = dest + rem;
	const __m256i alphamask = _mm256_set1_epi32(0xff000000);
	while( dest < limit ) {
		__m256i ms = _mm256_loadu_si256( (__m256i const*)src );
		if( _mm256_testc_si256( ms, alphamask ) ) {	// totally opaque
			_mm256_storeu_si256( (__m256i*)dest, ms );
		} else if( !_mm256_testz_si256( ms, alphamask ) ) {	// alpha != 0
			__m256i md = _mm256_loadu_si256( (__m256i const*)dest );
			_mm256_storeu_si256( (__m256i*)dest, func( md, ms ) );
		}
		dest+=8; src+=8;
	}
	limit += (len-rem);
	while( dest < limit ) {
		*dest = func( *dest, *src );
		dest++; src++;
	}
}

template<typename functor>
static void copy_src_branch_func_avx2( tjs_uint32 * __restrict dest, const tjs_uint32 * __restrict src, tjs_int len ) {
	functor func;
	blend_src_branch_func_avx2<functor>( dest, src, len, func );
}

#define DEFINE_BLEND_FUNCTION_MIN_VARIATION( NAME, FUNC ) \
static void TVP##NAME##_avx2_c( tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len ) {				\
	copy_func_avx2<avx2_##FUNC##_functor>( dest, src, len );											\
}																										\
static void TVP##NAME##_HDA_avx2_c( tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len ) {			\
	copy_func_avx2<avx2_##FUNC##_hda_functor>( dest, src, len );										\
}																										\
static void TVP##NAME##_o_avx2_c( tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa ) {	\
	avx2_##FUNC##_o_functor func(opa);																	\
	blend_func_avx2( dest, src, len, func );															\
}																										\
static void TVP##NAME##_HDA_o_avx2_c( tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa ) {	\
	avx2_##FUNC##_hda_o_functor func(opa);																\
	blend_func_avx2( dest, src, len, func );															\
}

#define DEFINE_BLEND_FUNCTION_MIN3_VARIATION( NAME, FUNC ) \
static void TVP##NAME##_avx2_c( tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len ) {				\
	copy_func_avx2<avx2_##FUNC##_functor>( dest, src, len );											\
}																										\
static void TVP##NAME##_HDA_avx2_c( tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len ) {			\
	copy_func_avx2<avx2_##FUNC##_hda_functor>( dest, src, len );										\
}																										\
static void TVP##NAME##_o_avx2_c( tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa ) {	\
	avx2_##FUNC##_o_functor func(opa);																	\
	blend_func_avx2( dest, src, len, func );															\
}

#define DEFINE_BLEND_FUNCTION_MIN2_VARIATION( NAME, FUNC ) \
static void TVP##NAME##_avx2_c( tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len ) {				\
	copy_func_avx2<avx2_##FUNC##_functor>( dest, src, len );											\
}																										\
static void TVP##NAME##_HDA_avx2_c( tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len ) {			\
	copy_func_avx2<avx2_##FUNC##_hda_functor>( dest, src, len );										\
}

// AlphaBlendはソースが完全透明/不透明で分岐する特殊版を使うので、個別に書く
static void TVPAlphaBlend_avx2_c( tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len ) {
	copy_src_branch_func_avx2<avx2_alpha_blend_functor>( dest, src, len );
}
static void TVPAlphaBlend_HDA_avx2_c( tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len ) {
	copy_func_avx2<avx2_alpha_blend_hda_functor>( dest, src, len );
}
static void TVPAlphaBlend_o_avx2_c( tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa ) {
	avx2_alpha_blend_o_functor func(opa);
	blend_func_avx2( dest, src, len, func );
}
static void TVPAlphaBlend_HDA_o_avx2_c( tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa ) {
	avx2_alpha_blend_hda_o_functor func(opa);
	blend_func_avx2( dest, src, len, func );
}
static void TVPAlphaBlend_d_avx2_c( tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len ) {
	copy_src_branch_func_avx2<avx2_alpha_blend_d_functor>( dest, src, len );
}
static void TVPConstAlphaBlend_SD_avx2_c(tjs_uint32 *dest, const tjs_uint32 *src1, const tjs_uint32 *src2, tjs_int len, tjs_int opa){
	avx2_const_alpha_blend_functor func(opa);
	sd_blend_func_avx2( dest, src1, src2, len, func );
}
static void TVPCopyColor_avx2_c(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len) {
	overlap_copy_func_avx2<avx2_color_copy_functor>( dest, src, len );
}
static void TVPCopyMask_avx2_c(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len) {
	overlap_copy_func_avx2<avx2_alpha_copy_functor>( dest, src, len );
}
static void TVPCopyOpaqueImage_avx2_c(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len) {
	copy_func_avx2<avx2_color_opaque_functor>( dest, src, len );
}

static void TVPConstAlphaBlend_avx2_c(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa) {
	avx2_const_alpha_blend_functor func(opa);
	blend_func_avx2( dest, src, len, func );
}
static void TVPConstAlphaBlend_HDA_avx2_c(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa) {
	avx2_const_alpha_blend_hda_functor func(opa);
	blend_func_avx2( dest, src, len, func );
}
static void TVPConstAlphaBlend_d_avx2_c(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa) {
	avx2_const_alpha_blend_d_functor func(opa);
	blend_func_avx2( dest, src, len, func );
}
static void TVPConstAlphaBlend_a_avx2_c(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa) {
	avx2_const_alpha_blend_a_functor func(opa);
	blend_func_avx2( dest, src, len, func );
}

static void TVPAdditiveAlphaBlend_avx2_c(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len){
	copy_func_avx2<avx2_premul_alpha_blend_functor>( dest, src, len );
}
static void TVPAdditiveAlphaBlend_o_avx2_c(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa){
	avx2_premul_alpha_blend_o_functor func(opa);
	blend_func_avx2( dest, src, len, func );
}
static void TVPAdditiveAlphaBlend_HDA_avx2_c(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len){
	copy_func_avx2<avx2_premul_alpha_blend_hda_functor>( dest, src, len );
}
static void TVPAdditiveAlphaBlend_a_avx2_c(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len){
	copy_func_avx2<avx2_premul_alpha_blend_a_functor>( dest, src, len );
}
/*
DEFINE_BLEND_FUNCTION_MIN_VARIATION( PsAlphaBlend, ps_alpha_blend )
DEFINE_BLEND_FUNCTION_MIN_VARIATION( PsAddBlend, ps_add_blend )
DEFINE_BLEND_FUNCTION_MIN_VARIATION( PsSubBlend, ps_sub_blend )
DEFINE_BLEND_FUNCTION_MIN_VARIATION( PsMulBlend, ps_mul_blend )
DEFINE_BLEND_FUNCTION_MIN_VARIATION( PsScreenBlend, ps_screen_blend )
DEFINE_BLEND_FUNCTION_MIN_VARIATION( PsOverlayBlend, ps_overlay_blend )
DEFINE_BLEND_FUNCTION_MIN_VARIATION( PsHardLightBlend, ps_hardlight_blend )
DEFINE_BLEND_FUNCTION_MIN_VARIATION( PsSoftLightBlend, ps_softlight_blend )
DEFINE_BLEND_FUNCTION_MIN_VARIATION( PsColorDodgeBlend, ps_colordodge_blend )
DEFINE_BLEND_FUNCTION_MIN_VARIATION( PsColorBurnBlend, ps_colorburn_blend )
DEFINE_BLEND_FUNCTION_MIN_VARIATION( PsColorDodge5Blend, ps_colordodge5_blend )
DEFINE_BLEND_FUNCTION_MIN_VARIATION( PsLightenBlend, ps_lighten_blend )
DEFINE_BLEND_FUNCTION_MIN_VARIATION( PsDarkenBlend, ps_darken_blend )
DEFINE_BLEND_FUNCTION_MIN_VARIATION( PsDiffBlend, ps_diff_blend )
DEFINE_BLEND_FUNCTION_MIN_VARIATION( PsDiff5Blend, ps_diff5_blend )
DEFINE_BLEND_FUNCTION_MIN_VARIATION( PsExclusionBlend, ps_exclusion_blend )
*/
extern void TVPInitializeResampleAVX2();
void TVPGL_AVX2_Init() {
	if( TVPCPUType & TVP_CPU_HAS_AVX2 ) {
		TVPAdditiveAlphaBlend = TVPAdditiveAlphaBlend_avx2_c;
		TVPAdditiveAlphaBlend_o = TVPAdditiveAlphaBlend_o_avx2_c;
		TVPAdditiveAlphaBlend_HDA = TVPAdditiveAlphaBlend_HDA_avx2_c;
		TVPAdditiveAlphaBlend_a = TVPAdditiveAlphaBlend_a_avx2_c;
		// TVPAdditiveAlphaBlend_ao

		TVPAlphaBlend =  TVPAlphaBlend_avx2_c;
		TVPAlphaBlend_o =  TVPAlphaBlend_o_avx2_c;
		TVPAlphaBlend_HDA =  TVPAlphaBlend_HDA_avx2_c;
		TVPAlphaBlend_d =  TVPAlphaBlend_d_avx2_c;
		// TVPAlphaBlend_a
		// TVPAlphaBlend_do
		// TVPAlphaBlend_ao
		TVPConstAlphaBlend =  TVPConstAlphaBlend_avx2_c;
		TVPConstAlphaBlend_HDA = TVPConstAlphaBlend_HDA_avx2_c;
		TVPConstAlphaBlend_d = TVPConstAlphaBlend_d_avx2_c;
		TVPConstAlphaBlend_a = TVPConstAlphaBlend_a_avx2_c;

		TVPConstAlphaBlend_SD =  TVPConstAlphaBlend_SD_avx2_c;
		TVPConstAlphaBlend_SD_a = TVPConstAlphaBlend_SD_avx2_c;

		TVPCopyColor = TVPCopyColor_avx2_c;
		TVPCopyMask = TVPCopyMask_avx2_c;
		TVPCopyOpaqueImage = TVPCopyOpaqueImage_avx2_c;

#if 0
		TVPPsAlphaBlend =  TVPPsAlphaBlend_avx2_c;
		TVPPsAlphaBlend_o =  TVPPsAlphaBlend_o_avx2_c;
		TVPPsAlphaBlend_HDA =  TVPPsAlphaBlend_HDA_avx2_c;
		TVPPsAlphaBlend_HDA_o =  TVPPsAlphaBlend_HDA_o_avx2_c;
		TVPPsAddBlend =  TVPPsAddBlend_avx2_c;
		TVPPsAddBlend_o =  TVPPsAddBlend_o_avx2_c;
		TVPPsAddBlend_HDA =  TVPPsAddBlend_HDA_avx2_c;
		TVPPsAddBlend_HDA_o =  TVPPsAddBlend_HDA_o_avx2_c;	
		TVPPsSubBlend =  TVPPsSubBlend_avx2_c;
		TVPPsSubBlend_o =  TVPPsSubBlend_o_avx2_c;
		TVPPsSubBlend_HDA =  TVPPsSubBlend_HDA_avx2_c;
		TVPPsSubBlend_HDA_o =  TVPPsSubBlend_HDA_o_avx2_c;
		TVPPsMulBlend =  TVPPsMulBlend_avx2_c;
		TVPPsMulBlend_o =  TVPPsMulBlend_o_avx2_c;
		TVPPsMulBlend_HDA =  TVPPsMulBlend_HDA_avx2_c;
		TVPPsMulBlend_HDA_o =  TVPPsMulBlend_HDA_o_avx2_c;
		TVPPsScreenBlend =  TVPPsScreenBlend_avx2_c;
		TVPPsScreenBlend_o =  TVPPsScreenBlend_o_avx2_c;
		TVPPsScreenBlend_HDA =  TVPPsScreenBlend_HDA_avx2_c;
		TVPPsScreenBlend_HDA_o =  TVPPsScreenBlend_HDA_o_avx2_c;
		TVPPsOverlayBlend =  TVPPsOverlayBlend_avx2_c;
		TVPPsOverlayBlend_o =  TVPPsOverlayBlend_o_avx2_c;
		TVPPsOverlayBlend_HDA =  TVPPsOverlayBlend_HDA_avx2_c;
		TVPPsOverlayBlend_HDA_o =  TVPPsOverlayBlend_HDA_o_avx2_c;
		TVPPsHardLightBlend =  TVPPsHardLightBlend_avx2_c;
		TVPPsHardLightBlend_o =  TVPPsHardLightBlend_o_avx2_c;
		TVPPsHardLightBlend_HDA =  TVPPsHardLightBlend_HDA_avx2_c;
		TVPPsHardLightBlend_HDA_o =  TVPPsHardLightBlend_HDA_o_avx2_c;
		TVPPsSoftLightBlend =  TVPPsSoftLightBlend_avx2_c;
		TVPPsSoftLightBlend_o =  TVPPsSoftLightBlend_o_avx2_c;
		TVPPsSoftLightBlend_HDA =  TVPPsSoftLightBlend_HDA_avx2_c;
		TVPPsSoftLightBlend_HDA_o =  TVPPsSoftLightBlend_HDA_o_avx2_c;
		TVPPsColorDodgeBlend =  TVPPsColorDodgeBlend_avx2_c;
		TVPPsColorDodgeBlend_o =  TVPPsColorDodgeBlend_o_avx2_c;
		TVPPsColorDodgeBlend_HDA =  TVPPsColorDodgeBlend_HDA_avx2_c;
		TVPPsColorDodgeBlend_HDA_o =  TVPPsColorDodgeBlend_HDA_o_avx2_c;
		TVPPsColorDodge5Blend =  TVPPsColorDodge5Blend_avx2_c;
		TVPPsColorDodge5Blend_o =  TVPPsColorDodge5Blend_o_avx2_c;
		TVPPsColorDodge5Blend_HDA =  TVPPsColorDodge5Blend_HDA_avx2_c;
		TVPPsColorDodge5Blend_HDA_o =  TVPPsColorDodge5Blend_HDA_o_avx2_c;
		TVPPsColorBurnBlend =  TVPPsColorBurnBlend_avx2_c;
		TVPPsColorBurnBlend_o =  TVPPsColorBurnBlend_o_avx2_c;
		TVPPsColorBurnBlend_HDA =  TVPPsColorBurnBlend_HDA_avx2_c;
		TVPPsColorBurnBlend_HDA_o =  TVPPsColorBurnBlend_HDA_o_avx2_c;
		TVPPsLightenBlend =  TVPPsLightenBlend_avx2_c;
		TVPPsLightenBlend_o =  TVPPsLightenBlend_o_avx2_c;
		TVPPsLightenBlend_HDA =  TVPPsLightenBlend_HDA_avx2_c;
		TVPPsLightenBlend_HDA_o =  TVPPsLightenBlend_HDA_o_avx2_c;
		TVPPsDarkenBlend =  TVPPsDarkenBlend_avx2_c;
		TVPPsDarkenBlend_o =  TVPPsDarkenBlend_o_avx2_c;
		TVPPsDarkenBlend_HDA =  TVPPsDarkenBlend_HDA_avx2_c;
		TVPPsDarkenBlend_HDA_o =  TVPPsDarkenBlend_HDA_o_avx2_c;
		TVPPsDiffBlend =  TVPPsDiffBlend_avx2_c;
		TVPPsDiffBlend_o =  TVPPsDiffBlend_o_avx2_c;
		TVPPsDiffBlend_HDA =  TVPPsDiffBlend_HDA_avx2_c;
		TVPPsDiffBlend_HDA_o =  TVPPsDiffBlend_HDA_o_avx2_c;
		TVPPsDiff5Blend =  TVPPsDiff5Blend_avx2_c;
		TVPPsDiff5Blend_o =  TVPPsDiff5Blend_o_avx2_c;
		TVPPsDiff5Blend_HDA =  TVPPsDiff5Blend_HDA_avx2_c;
		TVPPsDiff5Blend_HDA_o =  TVPPsDiff5Blend_HDA_o_avx2_c;
		TVPPsExclusionBlend =  TVPPsExclusionBlend_avx2_c;
		TVPPsExclusionBlend_o =  TVPPsExclusionBlend_o_avx2_c;
		TVPPsExclusionBlend_HDA =  TVPPsExclusionBlend_HDA_avx2_c;
		TVPPsExclusionBlend_HDA_o =  TVPPsExclusionBlend_HDA_o_avx2_c;
		TVPUnivTransBlend = TVPUnivTransBlend_avx2_c;
		TVPUnivTransBlend_a = TVPUnivTransBlend_avx2_c;
		TVPUnivTransBlend_d = TVPUnivTransBlend_d_avx2_c;
		TVPUnivTransBlend_switch = TVPUnivTransBlend_switch_avx2_c;
		TVPUnivTransBlend_switch_a = TVPUnivTransBlend_switch_avx2_c;
		TVPUnivTransBlend_switch_d = TVPUnivTransBlend_switch_d_avx2_c;

		if( TVPCPUType & TVP_CPU_HAS_AVX ) {
			TVPInitGammaAdjustTempData = TVPInitGammaAdjustTempData_avx2_c;
		}
		TVPAdjustGamma_a = TVPAdjustGamma_a_avx2_c;

		// アフィン変換用
		TVPLinTransAlphaBlend = TVPLinTransAlphaBlend_avx2_c;
		TVPLinTransAlphaBlend_HDA = TVPLinTransAlphaBlend_HDA_avx2_c;
		TVPLinTransAlphaBlend_o = TVPLinTransAlphaBlend_o_avx2_c;
		TVPLinTransAlphaBlend_HDA_o = TVPLinTransAlphaBlend_HDA_o_avx2_c;
		TVPLinTransAlphaBlend_d = TVPLinTransAlphaBlend_d_avx2_c;
		TVPLinTransAlphaBlend_a = TVPLinTransAlphaBlend_a_avx2_c;
		TVPLinTransAdditiveAlphaBlend = TVPLinTransAdditiveAlphaBlend_avx2_c;
		TVPLinTransAdditiveAlphaBlend_HDA = TVPLinTransAdditiveAlphaBlend_HDA_avx2_c;
		TVPLinTransAdditiveAlphaBlend_o = TVPLinTransAdditiveAlphaBlend_o_avx2_c;
		TVPLinTransAdditiveAlphaBlend_a = TVPLinTransAdditiveAlphaBlend_a_avx2_c;
		TVPLinTransCopyOpaqueImage = TVPLinTransCopyOpaqueImage_avx2_c;
		TVPLinTransCopy = TVPLinTransCopy_avx2_c;
		TVPLinTransColorCopy = TVPLinTransColorCopy_avx2_c;
		TVPLinTransConstAlphaBlend = TVPLinTransConstAlphaBlend_avx2_c;
		TVPLinTransConstAlphaBlend_HDA = TVPLinTransConstAlphaBlend_HDA_avx2_c;
		TVPLinTransConstAlphaBlend_d = TVPLinTransConstAlphaBlend_d_avx2_c;
		TVPLinTransConstAlphaBlend_a = TVPLinTransConstAlphaBlend_a_avx2_c;
		TVPInterpLinTransCopy = TVPInterpLinTransCopy_avx2_c;
		TVPInterpLinTransConstAlphaBlend = TVPInterpLinTransConstAlphaBlend_avx2_c;
#endif
		TVPInitializeResampleAVX2();
	}
}

