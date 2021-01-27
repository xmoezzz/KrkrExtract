/******************************************************************************/
/**
 * 各種フィルターのウェイトを計算する SSE 版
 * ----------------------------------------------------------------------------
 * 	Copyright (C) T.Imoto <http://www.kaede-software.com>
 * ----------------------------------------------------------------------------
 * @author		T.Imoto
 * @date		2014/04/05
 * @note
 *****************************************************************************/


#ifndef __WEIGHT_FUNCTOR_SSE_H__
#define __WEIGHT_FUNCTOR_SSE_H__

#include "WeightFunctor.h"

/**
 * バイリニア
 */
struct BilinearWeightSSE : public BilinearWeight {
	const __m128 absmask;
	const __m128 one;
	inline BilinearWeightSSE() : absmask( _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff)) ), one(_mm_set1_ps(1.0f)){}
	inline __m128 operator() ( __m128 distance ) const {
		__m128 x = _mm_and_ps( distance, absmask );	// 絶対値
		// 1.0f 以上は0を設定するためのマスク
		//__m128 zeromask = _mm_cmp_ps( x, one, _CMP_LT_OS ); // x < 1.0f
		__m128 zeromask = _mm_cmplt_ps( x, one ); // x < 1.0f
		x = _mm_sub_ps( one, x ); // 1.0f - x
		return _mm_and_ps( x, zeromask );	// x >= 1.0f は0へ
	}
};
/**
 * バイキュービック
 */
struct BicubicWeightSSE : public BicubicWeight {
	const __m128 M128_P0;
	const __m128 M128_P1;
	const __m128 M128_P2;
	const __m128 M128_P3;
	const __m128 M128_P4;
	const __m128 M128_COEFF;
	const __m128 absmask;
	const __m128 one;
	const __m128 two;

	/**
	 * @param c : シャープさ。小さくなるにしたがって強くなる
	 */
	inline BicubicWeightSSE( float c = -1 ) : BicubicWeight(c),
		M128_P0( _mm_set1_ps( c + 3.0f ) ),
		M128_P1( _mm_set1_ps( c + 2.0f ) ),
		M128_P2( _mm_set1_ps( -c*4.0f ) ),
		M128_P3( _mm_set1_ps( c*8.0f ) ),
		M128_P4( _mm_set1_ps( c*5.0f ) ),
		M128_COEFF( _mm_set1_ps( c ) ),
		absmask( _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff)) ),
		one(_mm_set1_ps(1.0f)),
		two(_mm_set1_ps(2.0f))
	{}

	// 素直に実装すると遅いかも
	inline __m128 operator() ( __m128 distance ) const {
		__m128 x = _mm_and_ps( distance, absmask );	// 絶対値
		__m128 x2 = _mm_mul_ps(x, x);
		__m128 x3 = _mm_mul_ps(x, x2);

		// 1.0以下の時の値
		__m128 t2 = _mm_mul_ps( M128_P0, x2 );
		__m128 t3 = _mm_mul_ps( M128_P1, x3 );
		__m128 onev = _mm_add_ps( _mm_sub_ps( one, t2 ), t3 );

		// 2.0以下の時の値
		t2 = _mm_mul_ps( M128_P3, x );
		t3 = _mm_mul_ps( M128_P4, x2 );
		__m128 t4 = _mm_mul_ps( M128_COEFF, x3 );
		__m128 twov = _mm_add_ps( _mm_sub_ps( _mm_add_ps( M128_P2, t2 ), t3 ), t4 );

		//__m128 onemask = _mm_cmp_ps( x, one, _CMP_LE_OS ); // x <= 1.0f
		__m128 onemask = _mm_cmple_ps( x, one ); // x <= 1.0f
		//__m128 twomask = _mm_cmp_ps( x, two, _CMP_LE_OS ); // x <= 2.0f
		__m128 twomask = _mm_cmple_ps( x, two ); // x <= 2.0f
		twomask = _mm_andnot_ps( onemask, twomask );	// x > 1.0f && x <= 2.0f
		onev = _mm_and_ps( onev, onemask );	// 1.0以下の時の値
		twov = _mm_and_ps( twov, twomask );	// 1.0より大きく、2.0以下の時の値
		return _mm_or_ps( onev, twov );	// 2.0以下の値を or して、それ以外は0に
	}
};
/**
 * Lanczos
 */
template<int TTap>
struct LanczosWeightSSE : public LanczosWeight<TTap> {
	const __m128 M128_TAP;
	const __m128 M128_PI;
	const __m128 M128_EPSILON;
	const __m128 M128_1_0;
	inline LanczosWeightSSE() : M128_TAP(_mm_set1_ps((float)TTap)), M128_PI(_mm_set1_ps((float)M_PI)),
		M128_EPSILON(_mm_set1_ps(FLT_EPSILON)), M128_1_0(_mm_set1_ps(1.0f)) {}
	inline __m128 operator() ( __m128 dist ) const {
		__m128 pidist = _mm_mul_ps( dist, M128_PI );	// M_PI * dist
		__m128 result = sin_ps( pidist ); // std::sin(M_PI*phase)
		__m128 tap = M128_TAP;
		__m128 rtap = m128_rcp_22bit_ps( tap );
		result = _mm_mul_ps( result, sin_ps( _mm_mul_ps( pidist, rtap ) ) ); // std::sin(M_PI*phase)*std::sin(M_PI*phase/TTap)
		pidist = _mm_mul_ps( pidist, pidist );
		pidist = _mm_mul_ps( pidist, rtap );// M_PI*M_PI*phase*phase/TTap
		pidist = m128_rcp_22bit_ps( pidist );
		result = _mm_mul_ps( result, pidist );	// std::sin(M_PI*phase)*std::sin(M_PI*phase/TTap)/(M_PI*M_PI*phase*phase/TTap)
		
		// EPSILON より小さい場合は、1を設定
		__m128 onemask = _mm_cmplt_ps( dist, M128_EPSILON );
		result = _mm_or_ps( _mm_andnot_ps( onemask, result ), _mm_and_ps( M128_1_0, onemask ) );
		// TAP 以上はゼロを設定
		__m128 zeromask = _mm_cmpge_ps( dist, tap );
		result = _mm_or_ps( _mm_andnot_ps( zeromask, result ), _mm_and_ps( _mm_setzero_ps(), zeromask ) );
		return result;
	}
};

/**
 * Spline16 用ウェイト関数
 */
struct Spline16WeightSSE : public Spline16Weight {
	const __m128 one;
	const __m128 two;
	const __m128 absmask;
	const __m128 M128_9_0__5_0;
	const __m128 M128_1_0__5_0;
	const __m128 M128_1_0__3_0;
	const __m128 M128_46_0__15_0;
	const __m128 M128_8_0__5_0;
	inline Spline16WeightSSE() : absmask(_mm_castsi128_ps(_mm_set1_epi32(0x7fffffff))), one(_mm_set1_ps(1.0f)), two(_mm_set1_ps(2.0f)),
		M128_9_0__5_0(_mm_set1_ps(9.0f/5.0f)), M128_1_0__5_0(_mm_set1_ps(1.0f/ 5.0f)),
		M128_1_0__3_0(_mm_set1_ps(1.0f/3.0f)), M128_46_0__15_0(_mm_set1_ps(46.0f/15.0f)),
		M128_8_0__5_0(_mm_set1_ps(8.0f/5.0f))
	{}
	inline __m128 operator() ( __m128 distance ) const {
		__m128 x = _mm_and_ps( distance, absmask );	// 絶対値
		__m128 x2 = _mm_mul_ps( x, x );
		__m128 x3 = _mm_mul_ps( x, x2 );

		// 1以下の時
		const __m128 t2_95 = M128_9_0__5_0;
		__m128 t2 = _mm_mul_ps( x2, t2_95 );	// x*x*9/5
		__m128 t3 = _mm_mul_ps( x3, M128_1_0__5_0 );	// x*1/5
		__m128 onev = _mm_add_ps( _mm_sub_ps( _mm_sub_ps( x3, t2 ), t3 ), one );

		// 2以下の時
		__m128 t1 = _mm_mul_ps( x3, M128_1_0__3_0 );	// x*x*x*1/3
		t2 = _mm_mul_ps( x2, t2_95 );	// x*x*9/5
		t3 = _mm_mul_ps( x, M128_46_0__15_0 );	// x*46.0f/15.0f
		__m128 twov = _mm_add_ps( _mm_sub_ps( _mm_sub_ps( t2, t1 ), t3 ), M128_8_0__5_0 );

		// 結果を合成
		__m128 onemask = _mm_cmple_ps( x, one ); // x <= 1.0f
		__m128 twomask = _mm_cmple_ps( x, two );
		twomask = _mm_andnot_ps( onemask, twomask );	// x > 1.0f && x <= 2.0f
		onev = _mm_and_ps( onev, onemask );	// 1.0以下の時の値
		twov = _mm_and_ps( twov, twomask );	// 1.0より大きく、2.0以下の時の値
		return _mm_or_ps( onev, twov );	// 2.0以下の値を or して、それ以外は0に
	}
};

/**
 * Spline36 用ウェイト関数
 */
struct Spline36WeightSSE : public Spline36Weight {
	const __m128 one;
	const __m128 two;
	const __m128 three;
	const __m128 absmask;
	const __m128 M128_13_0__11_0;
	const __m128 M128_453_0__209_0;
	const __m128 M128_3_0__209_0;
	const __m128 M128_6_0__11_0;
	const __m128 M128_612_0__209_0;
	const __m128 M128_1038_0__209_0;
	const __m128 M128_540_0__209_0;
	const __m128 M128_1_0__11_0;
	const __m128 M128_159_0__209_0;
	const __m128 M128_434_0__209_0;
	const __m128 M128_384_0__209_0;
	inline Spline36WeightSSE() : absmask(_mm_castsi128_ps(_mm_set1_epi32(0x7fffffff))),
		one(_mm_set1_ps(1.0f)), two(_mm_set1_ps(2.0f)), three(_mm_set1_ps(3.0f)),
		M128_13_0__11_0(_mm_set1_ps(13.0f/11.0f)), 
		M128_453_0__209_0(_mm_set1_ps(453.0f/209.0f)), 
		M128_3_0__209_0(_mm_set1_ps(3.0f/209.0f)), 
		M128_6_0__11_0(_mm_set1_ps(6.0f/11.0f)), 
		M128_612_0__209_0(_mm_set1_ps(612.0f/209.0f)), 
		M128_1038_0__209_0(_mm_set1_ps(1038.0f/209.0f)), 
		M128_540_0__209_0(_mm_set1_ps(540.0f/209.0f)), 
		M128_1_0__11_0(_mm_set1_ps(1.0f/11.0f)), 
		M128_159_0__209_0(_mm_set1_ps(159.0f/209.0f)), 
		M128_434_0__209_0(_mm_set1_ps(434.0f/209.0f)), 
		M128_384_0__209_0(_mm_set1_ps(384.0f/209.0f)) {}
	inline __m128 operator() ( __m128 distance ) const {
		__m128 x = _mm_and_ps( distance, absmask );	// 絶対値
		__m128 x2 = _mm_mul_ps( x, x );
		__m128 x3 = _mm_mul_ps( x, x2 );

		// 1.0以下の時
		__m128 t1 = _mm_mul_ps( x3, M128_13_0__11_0 );
		__m128 t2 = _mm_mul_ps( x2, M128_453_0__209_0 );
		__m128 t3 = _mm_mul_ps( x, M128_3_0__209_0 );
		__m128 onev = _mm_add_ps( _mm_sub_ps( _mm_sub_ps( t1, t2 ), t3 ), one );

		// 2.0以下の時
		t1 = _mm_mul_ps( x3, M128_6_0__11_0 );
		t2 = _mm_mul_ps( x2, M128_612_0__209_0 );
		t3 = _mm_mul_ps( x, M128_1038_0__209_0 );
		__m128 twov = _mm_add_ps( _mm_sub_ps( _mm_sub_ps( t2, t1 ), t3 ), M128_540_0__209_0 );

		// 3.0以下の時
		t1 = _mm_mul_ps( x3, M128_1_0__11_0 );
		t2 = _mm_mul_ps( x2, M128_159_0__209_0 );
		t3 = _mm_mul_ps( x, M128_434_0__209_0 );
		__m128 thrv = _mm_sub_ps( _mm_add_ps( _mm_sub_ps( t1, t2 ), t3 ), M128_384_0__209_0 );

		// 結果を合成
		__m128 onemask = _mm_cmple_ps( x, one ); // x <= 1.0f
		__m128 twomask = _mm_cmple_ps( x, two ); // x <= 2.0f
		__m128 thrmask = _mm_cmple_ps( x, three ); // x <= 3.0f
		thrmask = _mm_andnot_ps( twomask, thrmask );	// x > 2.0f && x <= 3.0f
		twomask = _mm_andnot_ps( onemask, twomask );	// x > 1.0f && x <= 2.0f
		onev = _mm_and_ps( onev, onemask );	// 1.0以下の時の値
		twov = _mm_and_ps( twov, twomask );	// 1.0より大きく、2.0以下の時の値
		thrv = _mm_and_ps( thrv, thrmask );	// 2.0より大きく、3.0以下の時の値
		return _mm_or_ps( _mm_or_ps( onev, twov ), thrv );
	}
};

/**
 * ガウス関数
 */
struct GaussianWeightSSE : public GaussianWeight {
	const __m128 M128_SQ2PI;
	const __m128 M128_M2_0;
	inline GaussianWeightSSE() : M128_SQ2PI(_mm_set1_ps(std::sqrt( 2.0f/(float)M_PI ))), M128_M2_0(_mm_set1_ps(-2.0f)) {}
	inline __m128 operator() ( __m128 x ) const {
		__m128 x2 = _mm_mul_ps( x, x );
		x2 = _mm_mul_ps( x2, M128_M2_0 );
		return _mm_mul_ps( exp_ps( x2 ), M128_SQ2PI );
	}
};

/**
 * Blackman-Sinc 関数
 */
struct BlackmanSincWeightSSE : public BlackmanSincWeight {
	const __m128 absmask;
	const __m128 M128_COEFF;
	const __m128 M128_EPSILON;
	const __m128 M128_1_0;
	const __m128 M128_PI;
	const __m128 M128_0_5;
	const __m128 M128_2_0;
	const __m128 M128_0_08;
	const __m128 M128_0_42;
	inline BlackmanSincWeightSSE( float c = RANGE ) : BlackmanSincWeight(c), absmask(_mm_castsi128_ps(_mm_set1_epi32(0x7fffffff))),
		M128_COEFF(_mm_set1_ps(1.0f/c)),
		M128_EPSILON(_mm_set1_ps(FLT_EPSILON)),
		M128_1_0(_mm_set1_ps(1.0f)),
		M128_PI(_mm_set1_ps((float)M_PI)),
		M128_0_5(_mm_set1_ps(0.5f)),
		M128_2_0(_mm_set1_ps(2.0f)),
		M128_0_08(_mm_set1_ps(0.08f)),
		M128_0_42(_mm_set1_ps(0.42f)) {}
	inline __m128 operator() ( __m128 distance ) const {
		__m128 x = _mm_and_ps( distance, absmask );	// 絶対値
		__m128 pix = _mm_mul_ps( x, M128_PI );	// PI * x

		__m128 coeff = M128_COEFF;
		coeff = _mm_mul_ps( coeff, pix );	// coeff * PI * x

		__m128 t2 = cos_ps( coeff );	// cos(coeff * PI * x)
		t2 = _mm_mul_ps( t2, M128_0_5 );	// 0.5 * cos(coeff * PI * x)

		__m128 t3 = _mm_mul_ps( coeff, M128_2_0 );	// 2.0 * coeff * PI * x
		t3 = cos_ps( t3 );	// cos(2.0 * coeff * PI * x)
		t3 = _mm_mul_ps( t3, M128_0_08 );	// 0.08 * cos(2.0 * coeff * PI * x)

		// 0.42 + 0.5 * cos(PI * x * coeff) + 0.08 * cos(2.0 * PI * x * coeff)
		__m128 result = _mm_add_ps( _mm_add_ps( t2, t3 ), M128_0_42 );	// + 0.42

		__m128 sincpix = sin_ps( pix );	// sin(PI * x)
		pix  = m128_rcp_22bit_ps( pix );	// 1.0 / (PI * x)
		sincpix = _mm_mul_ps( sincpix, pix );	// sin(PI * x) / (PI * x)
		result = _mm_mul_ps( result, sincpix );	// result

		__m128 one = M128_1_0;
		__m128 onemask = _mm_cmplt_ps( x, M128_EPSILON );
		one = _mm_and_ps( one, onemask );	// x < epsilon ? 1.0
		onemask = _mm_andnot_ps( onemask, result );	// x >= epsilon ? result
		return _mm_or_ps( one, onemask );
	}
};

#endif // __WEIGHT_FUNCTOR_SSE_H__
