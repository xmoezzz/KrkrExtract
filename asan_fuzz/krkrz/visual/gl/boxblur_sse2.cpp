
#include "tjsTypes.h"
#include "tvpgl.h"
#include "tvpgl_ia32_intf.h"
#include "simd_def_x86x64.h"
#include "x86simdutil.h"

void TVPAddSubVertSum16_sse2_c(tjs_uint16 *dest, const tjs_uint32 *addline, const tjs_uint32 *subline, tjs_int len) {
	if( len <= 0 ) return;

	tjs_uint32 rem = (len>>2)<<2;	// 4pixelずつ
	tjs_uint16* limit = dest + rem * 4;
	const __m128i zero = _mm_setzero_si128();
	while( dest < limit ) {
		__m128i md1 = _mm_load_si128( (__m128i const*)dest );
		__m128i madd1 = _mm_loadu_si128( (__m128i const*)addline );
		__m128i msub1 = _mm_loadu_si128( (__m128i const*)subline );
		__m128i md2 = _mm_load_si128( (__m128i const*)(dest+8) );
		__m128i madd2 = madd1;
		__m128i msub2 = msub1;
		madd1 = _mm_unpacklo_epi8( madd1, zero );	// 1 00|A+|00|R+|00|G+|00|B+
		madd2 = _mm_unpackhi_epi8( madd2, zero );	// 2 00|A+|00|R+|00|G+|00|B+
		msub1 = _mm_unpacklo_epi8( msub1, zero );	// 1 00|A-|00|R-|00|G-|00|B-
		msub2 = _mm_unpackhi_epi8( msub2, zero );	// 2 00|A-|00|R-|00|G-|00|B-

		md1 = _mm_add_epi16( md1, madd1 );	// 1
		md2 = _mm_add_epi16( md2, madd2 );	// 2
		md1 = _mm_sub_epi16( md1, msub1 );	// 1
		md2 = _mm_sub_epi16( md2, msub2 );	// 2

		_mm_store_si128( (__m128i*)dest, md1 );
		_mm_store_si128( (__m128i*)(dest+8), md2 );
		dest += 16;
		addline += 4;
		subline += 4;
	}
	limit += (len-rem)*4;
	while( dest < limit ) {
		__m128i md = _mm_loadl_epi64( (__m128i const*)dest );
		__m128i madd = _mm_cvtsi32_si128( *addline );
		__m128i msub = _mm_cvtsi32_si128( *subline );
		madd = _mm_unpacklo_epi8( madd, zero );	// 00|A+|00|R+|00|G+|00|B+
		msub = _mm_unpacklo_epi8( msub, zero );	// 00|A-|00|R-|00|G-|00|B-
		md = _mm_add_epi32( md, madd );
		md = _mm_sub_epi32( md, msub );
		_mm_storel_epi64( (__m128i*)dest, md );
		dest += 4; addline++; subline++;
	}
}
struct sse2_to_premul_alpha_functor {
	const __m128i c_0000ffffffffffff_;
	const __m128i c_0100000000000000_;
	inline sse2_to_premul_alpha_functor()
	: c_0000ffffffffffff_(_mm_set_epi32(0x0000ffff,0xffffffff,0x0000ffff,0xffffffff)),
	  c_0100000000000000_(_mm_set_epi32(0x01000000,0x00000000,0x01000000,0x00000000)) {}
	inline __m128i operator()( __m128i src ) const {
		__m128i tmp = src;
		tmp = _mm_srli_epi16( tmp, 7 );		// >>= 7
		tmp = _mm_add_epi16( tmp, src );	// adjust alpha
		tmp = _mm_srli_epi64( tmp, 48 );	// 000A|000A
		tmp = _mm_shufflelo_epi16( tmp, _MM_SHUFFLE( 0, 0, 0, 0 )  );	// 000A|AAAA
		tmp = _mm_shufflehi_epi16( tmp, _MM_SHUFFLE( 0, 0, 0, 0 )  );	// AAAA|AAAA
		tmp = _mm_and_si128( tmp, c_0000ffffffffffff_ );	// drop alpha area
		tmp = _mm_or_si128( tmp, c_0100000000000000_ );	// 0100|A|A|A
		src = _mm_mullo_epi16( src, tmp );
		return _mm_srli_epi16( src, 8 );
	}
};
/* 以下のようなfunctorを使ってTVPAddSubVertSum16_d_sse2_cを汎用化してもいいが……
struct sse2_to_passthru_functor {
	inline __m128i operator()( __m128i src ) const { return src; }
};
*/
void TVPAddSubVertSum16_d_sse2_c(tjs_uint16 *dest, const tjs_uint32 *addline, const tjs_uint32 *subline, tjs_int len) {
	if( len <= 0 ) return;

	tjs_uint32 rem = (len>>2)<<2;	// 4pixelずつ
	tjs_uint16* limit = dest + rem * 4;
	const __m128i zero = _mm_setzero_si128();
	sse2_to_premul_alpha_functor to_premul;	// 何もしない版のファンクタ作って、共通ソースにできそう。
	while( dest < limit ) {
		__m128i md1 = _mm_load_si128( (__m128i const*)dest );
		__m128i madd1 = _mm_loadu_si128( (__m128i const*)addline );
		__m128i msub1 = _mm_loadu_si128( (__m128i const*)subline );
		__m128i md2 = _mm_load_si128( (__m128i const*)(dest+8) );
		__m128i madd2 = madd1;
		madd1 = _mm_unpacklo_epi8( madd1, zero );	// 1 00|A+|00|R+|00|G+|00|B+
		madd1 = to_premul(madd1);
		md1 = _mm_add_epi16( md1, madd1 );	// 1

		__m128i msub2 = msub1;
		msub1 = _mm_unpacklo_epi8( msub1, zero );	// 1 00|A-|00|R-|00|G-|00|B-
		msub1 = to_premul(msub1);
		md1 = _mm_sub_epi16( md1, msub1 );	// 1
		_mm_store_si128( (__m128i*)dest, md1 );

		madd2 = _mm_unpackhi_epi8( madd2, zero );	// 2 00|A+|00|R+|00|G+|00|B+
		madd2 = to_premul(madd2);
		md2 = _mm_add_epi16( md2, madd2 );	// 2

		msub2 = _mm_unpackhi_epi8( msub2, zero );	// 2 00|A-|00|R-|00|G-|00|B-
		msub2 = to_premul(msub2);
		md2 = _mm_sub_epi16( md2, msub2 );	// 2
		_mm_store_si128( (__m128i*)(dest+8), md2 );

		dest += 16;
		addline += 4;
		subline += 4;
	}
	limit += (len-rem)*4;
	while( dest < limit ) {
		__m128i md = _mm_loadl_epi64( (__m128i const*)dest );
		__m128i madd = _mm_cvtsi32_si128( *addline );
		__m128i msub = _mm_cvtsi32_si128( *subline );
		madd = _mm_unpacklo_epi8( madd, zero );	// 00|A+|00|R+|00|G+|00|B+
		madd = to_premul(madd);
		msub = _mm_unpacklo_epi8( msub, zero );	// 00|A-|00|R-|00|G-|00|B-
		msub = to_premul(msub);
		md = _mm_add_epi32( md, madd );
		md = _mm_sub_epi32( md, msub );
		_mm_storel_epi64( (__m128i*)dest, md );
		dest += 4; addline++; subline++;
	}
}

void TVPAddSubVertSum32_sse2_c(tjs_uint32 *dest, const tjs_uint32 *addline, const tjs_uint32 *subline, tjs_int len) {
	if( len <= 0 ) return;

	tjs_uint32 rem = (len>>2)<<2;	// 4pixelずつ
	tjs_uint32* limit = dest + rem * 4;
	const __m128i zero = _mm_setzero_si128();
	while( dest < limit ) {
		__m128i md = _mm_load_si128( (__m128i const*)dest );
		__m128i madd1 = _mm_loadu_si128( (__m128i const*)addline );
		__m128i msub1 = _mm_loadu_si128( (__m128i const*)subline );
		__m128i madd2 = madd1;
		__m128i msub2 = msub1;
		madd1 = _mm_unpacklo_epi8( madd1, zero );	// 00|A+|00|R+|00|G+|00|B+
		__m128i madd = madd1;
		madd1 = _mm_unpacklo_epi16( madd1, zero );
		msub1 = _mm_unpacklo_epi8( msub1, zero );	// 00|A-|00|R-|00|G-|00|B-
		__m128i msub = msub1;
		msub1 = _mm_unpacklo_epi16( msub1, zero );
		md = _mm_add_epi32( md, madd1 );
		md = _mm_sub_epi32( md, msub1 );
		_mm_store_si128( (__m128i*)dest, md );
		dest += 4;

		md = _mm_load_si128( (__m128i const*)dest );
		madd = _mm_unpackhi_epi16( madd, zero );
		msub = _mm_unpackhi_epi16( msub, zero );
		md = _mm_add_epi32( md, madd );
		md = _mm_sub_epi32( md, msub );
		_mm_store_si128( (__m128i*)dest, md );
		dest += 4;

		md = _mm_load_si128( (__m128i const*)dest );
		madd2 = _mm_unpackhi_epi8( madd2, zero );	// 2 00|A+|00|R+|00|G+|00|B+
		madd = madd2;
		madd2 = _mm_unpacklo_epi16( madd2, zero );
		msub2 = _mm_unpackhi_epi8( msub2, zero );	// 2 00|A-|00|R-|00|G-|00|B-
		msub = msub2;
		msub2 = _mm_unpacklo_epi16( msub2, zero );
		md = _mm_add_epi32( md, madd2 );
		md = _mm_sub_epi32( md, msub2 );
		_mm_store_si128( (__m128i*)dest, md );
		dest += 4;

		md = _mm_load_si128( (__m128i const*)dest );
		madd = _mm_unpackhi_epi16( madd, zero );
		msub = _mm_unpackhi_epi16( msub, zero );
		md = _mm_add_epi32( md, madd );
		md = _mm_sub_epi32( md, msub );
		_mm_store_si128( (__m128i*)dest, md );
		dest += 4;

		addline += 4; subline += 4;
	}
	limit += (len-rem)*4;
	while( dest < limit ) {
		__m128i md = _mm_load_si128( (__m128i const*)dest );
		__m128i madd = _mm_cvtsi32_si128( *addline );
		__m128i msub = _mm_cvtsi32_si128( *subline );
		madd = _mm_unpacklo_epi8( madd, zero );	// 00|A+|00|R+|00|G+|00|B+
		madd = _mm_unpacklo_epi16( madd, zero );
		msub = _mm_unpacklo_epi8( msub, zero );	// 00|A-|00|R-|00|G-|00|B-
		msub = _mm_unpacklo_epi16( msub, zero );
		md = _mm_add_epi32( md, madd );
		md = _mm_sub_epi32( md, msub );
		_mm_store_si128( (__m128i*)dest, md );
		dest += 4; addline++; subline++;
	}
}

void TVPAddSubVertSum32_d_sse2_c(tjs_uint32 *dest, const tjs_uint32 *addline, const tjs_uint32 *subline, tjs_int len) {
	if( len <= 0 ) return;

	tjs_uint32 rem = (len>>2)<<2;	// 4pixelずつ
	tjs_uint32* limit = dest + rem * 4;
	const __m128i zero = _mm_setzero_si128();
	sse2_to_premul_alpha_functor func;
	while( dest < limit ) {
		__m128i md = _mm_load_si128( (__m128i const*)dest );
		__m128i madd1 = _mm_loadu_si128( (__m128i const*)addline );
		__m128i msub1 = _mm_loadu_si128( (__m128i const*)subline );
		__m128i madd2 = madd1;
		__m128i msub2 = msub1;
		madd1 = _mm_unpacklo_epi8( madd1, zero );	// 00|A+|00|R+|00|G+|00|B+
		madd1 = func( madd1 );
		__m128i madd = madd1;
		madd1 = _mm_unpacklo_epi16( madd1, zero );
		msub1 = _mm_unpacklo_epi8( msub1, zero );	// 00|A-|00|R-|00|G-|00|B-
		msub1 = func( msub1 );
		__m128i msub = msub1;
		msub1 = _mm_unpacklo_epi16( msub1, zero );
		md = _mm_add_epi32( md, madd1 );
		md = _mm_sub_epi32( md, msub1 );
		_mm_store_si128( (__m128i*)dest, md );
		dest += 4;

		md = _mm_load_si128( (__m128i const*)dest );
		madd = _mm_unpackhi_epi16( madd, zero );
		msub = _mm_unpackhi_epi16( msub, zero );
		md = _mm_add_epi32( md, madd );
		md = _mm_sub_epi32( md, msub );
		_mm_store_si128( (__m128i*)dest, md );
		dest += 4;

		md = _mm_load_si128( (__m128i const*)dest );
		madd2 = _mm_unpackhi_epi8( madd2, zero );	// 2 00|A+|00|R+|00|G+|00|B+
		madd2 = func( madd2 );
		madd = madd2;
		madd2 = _mm_unpacklo_epi16( madd2, zero );
		msub2 = _mm_unpackhi_epi8( msub2, zero );	// 2 00|A-|00|R-|00|G-|00|B-
		msub2 = func( msub2 );
		msub = msub2;
		msub2 = _mm_unpacklo_epi16( msub2, zero );
		md = _mm_add_epi32( md, madd2 );
		md = _mm_sub_epi32( md, msub2 );
		_mm_store_si128( (__m128i*)dest, md );
		dest += 4;

		md = _mm_load_si128( (__m128i const*)dest );
		madd = _mm_unpackhi_epi16( madd, zero );
		msub = _mm_unpackhi_epi16( msub, zero );
		md = _mm_add_epi32( md, madd );
		md = _mm_sub_epi32( md, msub );
		_mm_store_si128( (__m128i*)dest, md );
		dest += 4;

		addline += 4; subline += 4;
	}
	limit += (len-rem)*4;
	while( dest < limit ) {
		__m128i md = _mm_load_si128( (__m128i const*)dest );
		__m128i madd = _mm_cvtsi32_si128( *addline );
		__m128i msub = _mm_cvtsi32_si128( *subline );
		madd = _mm_unpacklo_epi8( madd, zero );	// 00|A+|00|R+|00|G+|00|B+
		madd = func( madd );
		madd = _mm_unpacklo_epi16( madd, zero );
		msub = _mm_unpacklo_epi8( msub, zero );	// 00|A-|00|R-|00|G-|00|B-
		msub = func( msub );
		msub = _mm_unpacklo_epi16( msub, zero );
		md = _mm_add_epi32( md, madd );
		md = _mm_sub_epi32( md, msub );
		_mm_store_si128( (__m128i*)dest, md );
		dest += 4; addline++; subline++;
	}
}

// _mm_mulhi_epu16 が使えるから、16bitでも問題ない
struct sse2_box_blur_avg_16 {
	__m128i mrcp_;
	inline sse2_box_blur_avg_16( tjs_int n ) {
		mrcp_ = _mm_cvtsi32_si128( (1<<16) / n );
		mrcp_ = _mm_shufflelo_epi16( mrcp_, _MM_SHUFFLE( 0, 0, 0, 0 ) );
		mrcp_ = _mm_shuffle_epi32( mrcp_, _MM_SHUFFLE( 0, 0, 0, 0 ) );
	}
	inline void two( tjs_uint32 *dest, __m128i msum, const __m128i mhalf_n ) const {
		msum = _mm_add_epi16( msum, mhalf_n );		// sum + n/2
		msum = _mm_mulhi_epu16( msum, mrcp_ );		// (sum + n/2) * rcp, rcp は、16bitごとにする
		msum = _mm_packus_epi16( msum, msum );		// A8|R8|G8|B8|A8|R8|G8|B8
		_mm_storel_epi64( (__m128i *)dest, msum );	// 2pixelストア
	}
	inline void one( tjs_uint32 *dest, __m128i msum, const __m128i mhalf_n ) const {
		msum = _mm_add_epi16( msum, mhalf_n );		// sum + n/2
		msum = _mm_mulhi_epu16( msum, mrcp_ );		// (sum + n/2) * rcp, rcp は、16bitごとにする
		msum = _mm_packus_epi16( msum, msum );		// A8|R8|G8|B8|A8|R8|G8|B8
		*dest = _mm_cvtsi128_si32(msum);
	}
};
// SSE使い一部浮動小数点で演算
struct sse2_box_blur_avg_16_d_sse {
	__m128i mrcp_;
	const __m128 f255_;
	const __m128i c_0000ffffffffffff_;
	const __m128i zero_;
	inline sse2_box_blur_avg_16_d_sse( tjs_int n ) : f255_(_mm_set1_ps(255.0f)),
		c_0000ffffffffffff_(_mm_set_epi32(0x0000ffff,0xffffffff,0x0000ffff,0xffffffff)),
		zero_(_mm_setzero_si128()) {
		mrcp_ = _mm_cvtsi32_si128( (1<<16) / n );
		mrcp_ = _mm_shufflelo_epi16( mrcp_, _MM_SHUFFLE( 0, 0, 0, 0 ) );
		mrcp_ = _mm_shuffle_epi32( mrcp_, _MM_SHUFFLE( 0, 0, 0, 0 ) );
	}
	inline void two( tjs_uint32 *dest, __m128i msum0, const __m128i mhalf_n ) const {
		msum0 = _mm_add_epi16( msum0, mhalf_n );		// sum + n/2
		msum0 = _mm_mulhi_epu16( msum0, mrcp_ );		// (sum + n/2) * rcp, rcp は、16bitごとにする

		__m128i msum1 = msum0;
		msum1 = _mm_unpackhi_epi16( msum1, zero_ );	// 16bit -> 32bit
		__m128 fmsum1 = _mm_cvtepi32_ps(msum1);

		__m128i alpha = msum0;
		alpha = _mm_srli_epi64( alpha, 48 );		// color >> 48 = alpha
		__m128 fa = _mm_cvtepi32_ps(alpha);
		fa = _mm_rcp_ps( fa );
		// fa = m128_rcp_22bit_ps( fa ); // 精度上げるのなら
		__m128 fa1 = fa;
		fa1 = _mm_shuffle_ps( fa1, fa1, _MM_SHUFFLE( 2, 2, 2, 2 ) );
		fmsum1 = _mm_mul_ps( fmsum1, f255_ );
		fmsum1 = _mm_mul_ps( fmsum1, fa1 );
		msum1 = _mm_cvttps_epi32( fmsum1 );

		fa = _mm_shuffle_ps( fa, fa, _MM_SHUFFLE( 0, 0, 0, 0 ) );
		msum0 = _mm_unpacklo_epi16( msum0, zero_ );	// 16bit -> 32bit
		__m128 fmsum0 = _mm_cvtepi32_ps(msum0);
		fmsum0 = _mm_mul_ps( fmsum0, f255_ );
		fmsum0 = _mm_mul_ps( fmsum0, fa );
		msum0 = _mm_cvttps_epi32( fmsum0 );

		msum0 = _mm_packs_epi32( msum0, msum1 );
		alpha = _mm_slli_epi64( alpha, 48 );		// alpha << 48
		msum0 = _mm_and_si128( msum0, c_0000ffffffffffff_ );
		msum0 = _mm_or_si128( msum0, alpha );
		msum0 = _mm_packus_epi16( msum0, msum0 );		// A8|R8|G8|B8|A8|R8|G8|B8
		_mm_storel_epi64( (__m128i *)dest, msum0 );	// 2pixelストア
	}
	inline void one( tjs_uint32 *dest, __m128i msum, const __m128i mhalf_n ) const {
		msum = _mm_add_epi16( msum, mhalf_n );		// sum + n/2
		msum = _mm_mulhi_epu16( msum, mrcp_ );		// (sum + n/2) * rcp, rcp は、16bitごとにする
		__m128i alpha = msum;
		alpha = _mm_srli_epi64( alpha, 48 );		// color >> 48 = alpha
		msum = _mm_unpacklo_epi16( msum, zero_ );	// 16bit -> 32bit
		__m128 fmsum = _mm_cvtepi32_ps(msum);
		__m128 fa = _mm_cvtepi32_ps(alpha);
		fa = _mm_rcp_ps( fa );
		// fa = m128_rcp_22bit_ps( fa );
		fa = _mm_shuffle_ps( fa, fa, _MM_SHUFFLE( 0, 0, 0, 0 ) );
		fmsum = _mm_mul_ps( fmsum, f255_ );
		fmsum = _mm_mul_ps( fmsum, fa );
		msum = _mm_cvttps_epi32( fmsum );
		alpha = _mm_slli_epi64( alpha, 48 );		// alpha << 48
		msum = _mm_packs_epi32( msum, msum );
		msum = _mm_and_si128( msum, c_0000ffffffffffff_ );
		msum = _mm_or_si128( msum, alpha );
		msum = _mm_packus_epi16( msum, msum );		// A8|R8|G8|B8|A8|R8|G8|B8
		*dest = _mm_cvtsi128_si32(msum);
	}
};

// 255/alpha を計算してから color にかけているので少し精度低い
struct sse2_box_blur_avg_16_d {
	__m128i mrcp_;
	const __m128 f255_;
	const __m128i c_0100000000000000_;
	inline sse2_box_blur_avg_16_d( tjs_int n ) : f255_(_mm_set1_ps(0x10000)),
	  	c_0100000000000000_(_mm_set_epi32(0x01000000,0x00000000,0x01000000,0x00000000)){
		mrcp_ = _mm_cvtsi32_si128( (1<<16) / n );
		mrcp_ = _mm_shufflelo_epi16( mrcp_, _MM_SHUFFLE( 0, 0, 0, 0 ) );
		mrcp_ = _mm_shuffle_epi32( mrcp_, _MM_SHUFFLE( 0, 0, 0, 0 ) );
	}
	inline void two( tjs_uint32 *dest, __m128i msum, const __m128i mhalf_n ) const {
		msum = _mm_add_epi16( msum, mhalf_n );		// sum + n/2
		msum = _mm_mulhi_epu16( msum, mrcp_ );		// (sum + n/2) * rcp, rcp は、16bitごとにする
		__m128i alpha = msum;
		alpha = _mm_srli_epi64( alpha, 48 );		// color >> 48 = alpha
		__m128 fa = _mm_cvtepi32_ps(alpha);
		fa = _mm_rcp_ps( fa );
		//fa = m128_rcp_22bit_ps( fa );
		fa = _mm_mul_ps( fa, f255_ );
		alpha = _mm_cvttps_epi32( fa );	// color*255/alpha
		alpha = _mm_shufflelo_epi16( alpha, _MM_SHUFFLE( 2, 0, 0, 0 ) );	// 0 A A A
		alpha = _mm_shufflehi_epi16( alpha, _MM_SHUFFLE( 2, 0, 0, 0 ) );	// 0 A A A
		alpha = _mm_or_si128( alpha, c_0100000000000000_ );
		msum = _mm_slli_epi16( msum, 8 );
		msum = _mm_mulhi_epu16( msum, alpha );
		msum = _mm_packus_epi16( msum, msum );		// A8|R8|G8|B8|A8|R8|G8|B8
		_mm_storel_epi64( (__m128i *)dest, msum );	// 2pixelストア
	}
	inline void one( tjs_uint32 *dest, __m128i msum, const __m128i mhalf_n ) const {
		msum = _mm_add_epi16( msum, mhalf_n );		// sum + n/2
		msum = _mm_mulhi_epu16( msum, mrcp_ );		// (sum + n/2) * rcp, rcp は、16bitごとにする
		__m128i alpha = msum;
		alpha = _mm_srli_epi64( alpha, 48 );		// color >> 48 = alpha
		__m128 fa = _mm_cvtepi32_ps(alpha);
		fa = _mm_rcp_ps( fa );
		//fa = m128_rcp_22bit_ps( fa );
		fa = _mm_mul_ps( fa, f255_ );
		alpha = _mm_cvttps_epi32( fa );	// color*255/alpha
		alpha = _mm_shufflelo_epi16( alpha, _MM_SHUFFLE( 2, 0, 0, 0 ) );	// 0 A A A
		alpha = _mm_or_si128( alpha, c_0100000000000000_ );
		msum = _mm_slli_epi16( msum, 8 );
		msum = _mm_mulhi_epu16( msum, alpha );
		msum = _mm_packus_epi16( msum, msum );		// A8|R8|G8|B8|A8|R8|G8|B8
		*dest = _mm_cvtsi128_si32(msum);
	}
};
// テーブルバージョン、かなりの最適化任せ。速度と精度からこれになるか？
struct sse2_box_blur_avg_16_d_table {
	__m128i mrcp_;
	inline sse2_box_blur_avg_16_d_table( tjs_int n ) {
		mrcp_ = _mm_cvtsi32_si128( (1<<16) / n );
		mrcp_ = _mm_shufflelo_epi16( mrcp_, _MM_SHUFFLE( 0, 0, 0, 0 ) );
		mrcp_ = _mm_shuffle_epi32( mrcp_, _MM_SHUFFLE( 0, 0, 0, 0 ) );
	}
	inline void two( tjs_uint32 *dest, __m128i msum, const __m128i mhalf_n ) const {
		msum = _mm_add_epi16( msum, mhalf_n );		// sum + n/2
		msum = _mm_mulhi_epu16( msum, mrcp_ );		// (sum + n/2) * rcp, rcp は、16bitごとにする
		msum = _mm_packus_epi16( msum, msum );		// A8|R8|G8|B8|A8|R8|G8|B8
		unsigned char* lo = &TVPDivTable[msum.m128i_u8[3]<<8];
		unsigned char* hi = &TVPDivTable[msum.m128i_u8[7]<<8];
		dest[0] = (msum.m128i_u8[3]<<24) | (lo[msum.m128i_u8[2]]<<16) | (lo[msum.m128i_u8[1]]<<8) | lo[msum.m128i_u8[0]];
		dest[1] = (msum.m128i_u8[7]<<24) | (hi[msum.m128i_u8[6]]<<16) | (hi[msum.m128i_u8[5]]<<8) | hi[msum.m128i_u8[4]];
	}
	inline void one( tjs_uint32 *dest, __m128i msum, const __m128i mhalf_n ) const {
		msum = _mm_add_epi16( msum, mhalf_n );		// sum + n/2
		msum = _mm_mulhi_epu16( msum, mrcp_ );		// (sum + n/2) * rcp, rcp は、16bitごとにする
		msum = _mm_packus_epi16( msum, msum );		// A8|R8|G8|B8|A8|R8|G8|B8
		unsigned char* lo = &TVPDivTable[msum.m128i_u8[3]<<8];
		dest[0] = (msum.m128i_u8[3]<<24) | (lo[msum.m128i_u8[2]]<<16) | (lo[msum.m128i_u8[1]]<<8) | lo[msum.m128i_u8[0]];
	}
};
template<typename avg_func_t>
inline void sse2_box_blur_avg16(tjs_uint32 *dest, tjs_uint16 *sum, const tjs_uint16 * add, const tjs_uint16 * sub, tjs_int n, tjs_int len) {
	if( len <= 0 ) return;

	__m128i mhalf_n = _mm_cvtsi32_si128( n >> 1 );	// n / 2
	mhalf_n = _mm_shufflelo_epi16( mhalf_n, _MM_SHUFFLE( 0, 0, 0, 0 )  );
	mhalf_n = _mm_shuffle_epi32( mhalf_n, _MM_SHUFFLE( 0, 0, 0, 0 ) );
	__m128i msum = _mm_loadl_epi64((__m128i const*)sum);			// A16R16G16B16

	__m128i tmp = msum;
	tmp = _mm_slli_si128( tmp, 8 );		// << 64 下位pixelを上位へ
	msum = _mm_or_si128( msum, tmp );	// 上位と下位を同じに

	// 1pixel処理
	avg_func_t func( n );
	func.one( dest, msum, mhalf_n );
	dest++;
	len--;

	__m128i madd = _mm_loadu_si128( (__m128i const*)add );
	tmp = madd;
	tmp = _mm_slli_si128( tmp, 8 );		// << 64 下位pixelを上位へ
	msum = _mm_add_epi16( msum, tmp );	// sum += add まず、下位分を上位で加算
	__m128i msub = _mm_loadu_si128( (__m128i const*)sub );
	tmp = msub;
	tmp = _mm_slli_si128( tmp, 8 );		// << 64 下位pixelを上位へ
	msum = _mm_sub_epi16( msum, tmp );	// sum -= sub 下位分を上位で減算
	// 2pixel 分 (下位は1進み、上位は2進む)
	msum = _mm_add_epi16( msum, madd );	// sum += add
	msum = _mm_sub_epi16( msum, msub );	// sum -= sub
	add += 8;
	sub += 8;

	tjs_uint32 rem = (len>>1)<<1;
	tjs_uint32* limit = dest + rem;
	while( dest < limit ) {
		func.two( dest, msum, mhalf_n );
		dest += 2;

		// 下位は、以前の上位分加算されていないので、その分加算減算
		madd = _mm_srli_si128( madd, 8 );	// << 64 上位pixelを下位へ
		msub = _mm_srli_si128( msub, 8 );	// << 64 上位pixelを下位へ
		msum = _mm_add_epi16( msum, madd );	// sum += add
		msum = _mm_sub_epi16( msum, msub );	// sum -= sub

		// 次の 2px 加算減算
		madd = _mm_loadu_si128( (__m128i const*)add );
		tmp = madd;
		tmp = _mm_slli_si128( tmp, 8 );	// << 64 下位pixelを上位へ
		msum = _mm_add_epi16( msum, tmp );	// sum += add まず、下位を加算
		msub = _mm_loadu_si128( (__m128i const*)sub );
		tmp = msub;
		tmp = _mm_slli_si128( tmp, 8 );	// << 64 下位pixelを上位へ
		msum = _mm_sub_epi16( msum, tmp );	// sum -= sub 下位を減算

		msum = _mm_add_epi16( msum, madd );	// sum += add
		msum = _mm_sub_epi16( msum, msub );	// sum -= sub
		// madd, msub は次の処理で使う
		add += 8;
		sub += 8;
	}
	// 2pixel ずつ処理なので、余りは1か0
	if( (len-rem) > 0 ) {
		func.one( dest, msum, mhalf_n );
		msum = _mm_srli_si128( msum, 8 );		// 上位が次になっているので下位へ
	}
	_mm_storel_epi64( (__m128i *)sum, msum );	// msumを書き戻し
}

void TVPDoBoxBlurAvg16_sse2_c(tjs_uint32 *dest, tjs_uint16 *sum, const tjs_uint16 * add, const tjs_uint16 * sub, tjs_int n, tjs_int len) {
	sse2_box_blur_avg16<sse2_box_blur_avg_16>( dest, sum, add, sub, n, len );
}
void TVPDoBoxBlurAvg16_d_sse2_c_1(tjs_uint32 *dest, tjs_uint16 *sum, const tjs_uint16 * add, const tjs_uint16 * sub, tjs_int n, tjs_int len) {
	sse2_box_blur_avg16<sse2_box_blur_avg_16_d_sse>( dest, sum, add, sub, n, len );
}
void TVPDoBoxBlurAvg16_d_sse2_c_2(tjs_uint32 *dest, tjs_uint16 *sum, const tjs_uint16 * add, const tjs_uint16 * sub, tjs_int n, tjs_int len) {
	sse2_box_blur_avg16<sse2_box_blur_avg_16_d>( dest, sum, add, sub, n, len );
}
void TVPDoBoxBlurAvg16_d_sse2_c_3(tjs_uint32 *dest, tjs_uint16 *sum, const tjs_uint16 * add, const tjs_uint16 * sub, tjs_int n, tjs_int len) {
	sse2_box_blur_avg16<sse2_box_blur_avg_16_d_table>( dest, sum, add, sub, n, len );
}
// 速度的には 2 < 1 << 3  となる
void TVPDoBoxBlurAvg16_d_sse2_c(tjs_uint32 *dest, tjs_uint16 *sum, const tjs_uint16 * add, const tjs_uint16 * sub, tjs_int n, tjs_int len) {
	//sse2_box_blur_avg16<sse2_box_blur_avg_16_d_sse>( dest, sum, add, sub, n, len );
	sse2_box_blur_avg16<sse2_box_blur_avg_16_d>( dest, sum, add, sub, n, len );
	//sse2_box_blur_avg16<sse2_box_blur_avg_16_d_table>( dest, sum, add, sub, n, len );
}

struct sse2_box_blur_avg_32 {
	__m128i mrcp_;
	const __m128i zero_;
	inline sse2_box_blur_avg_32( tjs_int n ) : zero_(_mm_setzero_si128()) {
		mrcp_ = _mm_cvtsi32_si128( (tjs_uint32)( (1ULL<<32) / n ) );
		mrcp_ = _mm_unpacklo_epi64( mrcp_, mrcp_ );
	}
	inline void operator()( tjs_uint32 *dest, __m128i tmp, const __m128i mhalf_n ) const {
		tmp = _mm_add_epi32( tmp, mhalf_n );		// sum + n/2
		__m128i tmp2 = tmp;
		tmp = _mm_unpacklo_epi32( tmp, zero_ );	// 0000|Gs+HalfN|0000|Bs+HalfN 32bit -> 64bit
		tmp2 = _mm_unpackhi_epi32( tmp2, zero_ );// 0000|As+HalfN|0000|Rs+HalfN 32bit -> 64bit
		tmp = _mm_mul_epu32( tmp, mrcp_ );		// (sum + n/2) * rcp
		tmp2 = _mm_mul_epu32( tmp2, mrcp_ );	// (sum + n/2) * rcp
		tmp = _mm_srli_epi64( tmp, 32 );		// packしてからやると桁足りない
		tmp2 = _mm_srli_epi64( tmp2, 32 );
		// pack
		tmp = _mm_packs_epi32( tmp, tmp2 );		// ARGB (32bit)
		tmp = _mm_packs_epi32( tmp, zero_ );	// ARGB (16bit)
		tmp = _mm_packus_epi16( tmp, zero_ );	// A8|R8|G8|B8
		*dest = _mm_cvtsi128_si32(tmp);
	}
};
// SSE 使う。浮動小数点なので 23bit 超えると精度が落ちる
// SSE2 版と比較して速いかは……
struct sse2_box_blur_avg_32f {
	const __m128 mrcp_;
	inline sse2_box_blur_avg_32f( tjs_int n ) : mrcp_(_mm_set1_ps(1.0f/n)) {}
	inline void operator()( tjs_uint32 *dest, __m128i tmp, const __m128i mhalf_n ) const {
		tmp = _mm_add_epi32( tmp, mhalf_n );	// sum + n/2
		__m128 tmp2 = _mm_cvtepi32_ps( tmp );	// int -> float
		tmp2 = _mm_mul_ps( tmp2, mrcp_ );		// (sum + n/2) * rcp
		tmp = _mm_cvtps_epi32(tmp2);			// float -> int
		tmp = _mm_packs_epi32( tmp, tmp );	// ARGB (16bit)
		tmp = _mm_packus_epi16( tmp, tmp );	// A8|R8|G8|B8
		*dest = _mm_cvtsi128_si32(tmp);
	}
};
// 精度落ちるかもしれないが、途中の計算がSSEで出来てテーブル参照の手間がない
struct sse2_box_blur_avg_32_d {
	const __m128 mrcp_;
	const __m128 f255_;
	const __m128 one_;
	inline sse2_box_blur_avg_32_d( tjs_int n )
	: mrcp_(_mm_set1_ps(1.0f/n)),
	  f255_(_mm_set_ps( 1.0f, 255.0f, 255.0f, 255.0f )),
	  one_(_mm_set1_ps(1.0f)) {}
	inline void operator()( tjs_uint32 *dest, __m128i tmp, const __m128i mhalf_n ) const {
		tmp = _mm_add_epi32( tmp, mhalf_n );	// sum + n/2
		__m128 tmp2 = _mm_cvtepi32_ps( tmp );	// int -> float
		tmp2 = _mm_mul_ps( tmp2, mrcp_ );		// (sum + n/2) * rcp

		__m128 alpha = tmp2;
		alpha = _mm_shuffle_ps( alpha, one_,  _MM_SHUFFLE( 0, 0, 3, 3 ) );	// 1.0 1.0 a a
		//alpha = _mm_rcp_ss( alpha );			// 1/alpha
		alpha = m128_rcp_22bit_ss( alpha );		// 1/alpha : 精度上げるのなら
		tmp2 = _mm_mul_ps( tmp2, f255_ );		// sum*1.0 sum*255 sum*255 sum*255
		alpha = _mm_shuffle_ps( alpha, alpha, _MM_SHUFFLE( 3, 0, 0, 0 ) );	// 1.0 1/a 1/a 1/a
		tmp2 = _mm_mul_ps( tmp2, alpha );		// sum*1.0 sum*255/alpha sum*255/alpha sum*255/alpha
		tmp = _mm_cvtps_epi32(tmp2);			// float -> int
		tmp = _mm_packs_epi32( tmp, tmp );		// ARGB (16bit)
		tmp = _mm_packus_epi16( tmp, tmp );		// A8|R8|G8|B8
		*dest = _mm_cvtsi128_si32(tmp);
	}
};
template<typename func_t>
void sse2_box_blur_avg32(tjs_uint32 *dest, tjs_uint32 *sum, const tjs_uint32 * add, const tjs_uint32 * sub, tjs_int n, tjs_int len) {
	if( len <= 0 ) return;

	func_t func( n );
	tjs_uint32* limit = dest + len;
	__m128i msum = _mm_loadu_si128((__m128i const*)sum);		// A32R32G32B32
	__m128i mhalf_n = _mm_cvtsi32_si128( n >> 1 );	// n / 2
	mhalf_n = _mm_shuffle_epi32( mhalf_n, _MM_SHUFFLE( 0, 0, 0, 0 ) );
	while( dest < limit ) {
		__m128i tmp = msum;
		func( dest, msum, mhalf_n );

		__m128i madd = _mm_loadu_si128( (__m128i const*)add );
		__m128i msub = _mm_loadu_si128( (__m128i const*)sub );
		msum = _mm_add_epi32( msum, madd );	// sum += add
		msum = _mm_sub_epi32( msum, msub );	// sum -= sub
		dest++; add += 4; sub += 4;
	}
	_mm_storeu_si128( (__m128i *)sum, msum );	// msumを書き戻し
}
void TVPDoBoxBlurAvg32_sse2_c(tjs_uint32 *dest, tjs_uint32 *sum, const tjs_uint32 * add, const tjs_uint32 * sub, tjs_int n, tjs_int len) {
	sse2_box_blur_avg32<sse2_box_blur_avg_32>( dest, sum, add, sub, n, len );
}
void TVPDoBoxBlurAvg32_sse2_c_2(tjs_uint32 *dest, tjs_uint32 *sum, const tjs_uint32 * add, const tjs_uint32 * sub, tjs_int n, tjs_int len) {
	sse2_box_blur_avg32<sse2_box_blur_avg_32f>( dest, sum, add, sub, n, len );
}
void TVPDoBoxBlurAvg32_d_sse2_c(tjs_uint32 *dest, tjs_uint32 *sum, const tjs_uint32 * add, const tjs_uint32 * sub, tjs_int n, tjs_int len) {
	sse2_box_blur_avg32<sse2_box_blur_avg_32_d>( dest, sum, add, sub, n, len );
}

// SSE4.1 なら _mm_mullo_epi32 が使えて、4つ同時に mull 出来るんだが
// _mm_ cvtepu8_epi32 も使える
// _mm_mul_epu32 で 64bit * 2 で mul できる
// _mm_unpacklo_epi32
