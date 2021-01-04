/******************************************************************************/
/**
 * 各種フィルターのウェイトを計算する AVX 版
 * ----------------------------------------------------------------------------
 * 	Copyright (C) T.Imoto <http://www.kaede-software.com>
 * ----------------------------------------------------------------------------
 * @author		T.Imoto
 * @date		2014/04/03
 * @note
 *****************************************************************************/


#ifndef __WEIGHT_FUNCTOR_AVX_H__
#define __WEIGHT_FUNCTOR_AVX_H__

#include "WeightFunctor.h"

/**
 * バイリニア
 */
struct BilinearWeightAVX : public BilinearWeight {
	const __m256 absmask;
	const __m256 one;
	inline BilinearWeightAVX() : absmask( _mm256_castsi256_ps(_mm256_set1_epi32(0x7fffffff)) ), one(_mm256_set1_ps(1.0f)){}
	inline __m256 operator() ( __m256 distance ) const {
		__m256 x = _mm256_and_ps( distance, absmask );	// 絶対値
		// 1.0f 以上は0を設定するためのマスク
		__m256 zeromask = _mm256_cmp_ps( x, one, _CMP_LT_OS ); // x < 1.0f
		x = _mm256_sub_ps( one, x ); // 1.0f - x
		return _mm256_and_ps( x, zeromask );	// x >= 1.0f は0へ
	}
};
/**
 * バイキュービック
 * SIMD 化したけど、load多用しているので重いかもしれない
 */
struct BicubicWeightAVX : public BicubicWeight {
	const __m256 M256_P0;
	const __m256 M256_P1;
	const __m256 M256_P2;
	const __m256 M256_P3;
	const __m256 M256_P4;
	const __m256 M256_COEFF;
	const __m256 absmask;
	const __m256 one;
	const __m256 two;

	/**
	 * @param c : シャープさ。小さくなるにしたがって強くなる
	 */
	BicubicWeightAVX( float c = -1 ) : BicubicWeight(c),
		M256_P0( _mm256_set1_ps( c + 3.0f ) ),
		M256_P1( _mm256_set1_ps( c + 2.0f ) ),
		M256_P2( _mm256_set1_ps( -c*4.0f ) ),
		M256_P3( _mm256_set1_ps( c*8.0f ) ),
		M256_P4( _mm256_set1_ps( c*5.0f ) ),
		M256_COEFF( _mm256_set1_ps( c ) ),
		absmask( _mm256_castsi256_ps(_mm256_set1_epi32(0x7fffffff)) ),
		one(_mm256_set1_ps(1.0f)),
		two(_mm256_set1_ps(2.0f))
	{}


	// 素直に実装すると遅いかも
	inline __m256 operator() ( __m256 distance ) const {
		__m256 x = _mm256_and_ps( distance, absmask );	// 絶対値
		__m256 x2 = _mm256_mul_ps(x, x);
		__m256 x3 = _mm256_mul_ps(x, x2);

		// 1.0以下の時の値
		__m256 t2 = _mm256_mul_ps( M256_P0, x2 );
		__m256 t3 = _mm256_mul_ps( M256_P1, x3 );
		__m256 onev = _mm256_add_ps( _mm256_sub_ps( one, t2 ), t3 );

		// 2.0以下の時の値
		t2 = _mm256_mul_ps( M256_P3, x );
		t3 = _mm256_mul_ps( M256_P4, x2 );
		__m256 t4 = _mm256_mul_ps( M256_COEFF, x3 );
		__m256 twov = _mm256_add_ps( _mm256_sub_ps( _mm256_add_ps( M256_P2, t2), t3 ), t4 );

		__m256 onemask = _mm256_cmp_ps( x, one, _CMP_LE_OS ); // x <= 1.0f
		__m256 twomask = _mm256_cmp_ps( x, two, _CMP_LE_OS ); // x <= 2.0f
		twomask = _mm256_andnot_ps( onemask, twomask );	// x > 1.0f && x <= 2.0f
		onev = _mm256_and_ps( onev, onemask );	// 1.0以下の時の値
		twov = _mm256_and_ps( twov, twomask );	// 1.0より大きく、2.0以下の時の値
		return _mm256_or_ps( onev, twov );	// 2.0以下の値を or して、それ以外は0に
	}
};
/**
 * Lanczos
 */
template<int TTap>
struct LanczosWeightAVX : public LanczosWeight<TTap> {
	const __m256 M256_TAP;
	const __m256 M256_PI;
	const __m256 M256_EPSILON;
	const __m256 M256_1_0;
	LanczosWeightAVX() : M256_TAP(_mm256_set1_ps((float)TTap)), M256_PI(_mm256_set1_ps((float)M_PI)),
		M256_EPSILON(_mm256_set1_ps(FLT_EPSILON)), M256_1_0(_mm256_set1_ps(1.0f)) {}
	inline __m256 operator() ( __m256 dist ) const {
		__m256 pidist = _mm256_mul_ps( dist, M256_PI );	// M_PI * dist
		__m256 result = mm256_sin_ps( pidist ); // std::sin(M_PI*phase)
		__m256 tap = M256_TAP;
		__m256 rtap = m256_rcp_22bit_ps( tap );
		result = _mm256_mul_ps( result, mm256_sin_ps( _mm256_mul_ps( pidist, rtap ) ) ); // std::sin(M_PI*phase)*std::sin(M_PI*phase/TTap)
		pidist = _mm256_mul_ps( pidist, pidist );
		pidist = _mm256_mul_ps( pidist, rtap );// M_PI*M_PI*phase*phase/TTap
		pidist = m256_rcp_22bit_ps( pidist );
		result = _mm256_mul_ps( result, pidist );	// std::sin(M_PI*phase)*std::sin(M_PI*phase/TTap)/(M_PI*M_PI*phase*phase/TTap)
		
		// EPSILON より小さい場合は、1を設定
		__m256 onemask = _mm256_cmp_ps( dist, M256_EPSILON, _CMP_LT_OS );
		result = _mm256_or_ps( _mm256_andnot_ps( onemask, result ), _mm256_and_ps( M256_1_0, onemask ) );
		// TAP 以上はゼロを設定
		__m256 zeromask = _mm256_cmp_ps( dist, tap, _CMP_GE_OS );
		result = _mm256_or_ps( _mm256_andnot_ps( zeromask, result ), _mm256_and_ps( _mm256_setzero_ps(), zeromask ) );
		return result;
	}
};

/**
 * Spline16 用ウェイト関数
 */
struct Spline16WeightAVX : public Spline16Weight {
	const __m256 one;
	const __m256 two;
	const __m256 absmask;
	const __m256 M256_9_0__5_0;
	const __m256 M256_1_0__5_0;
	const __m256 M256_1_0__3_0;
	const __m256 M256_46_0__15_0;
	const __m256 M256_8_0__5_0;
	inline Spline16WeightAVX() : absmask(_mm256_castsi256_ps(_mm256_set1_epi32(0x7fffffff))), one(_mm256_set1_ps(1.0f)), two(_mm256_set1_ps(2.0f)),
		M256_9_0__5_0(_mm256_set1_ps(9.0f/5.0f)), M256_1_0__5_0(_mm256_set1_ps(1.0f/ 5.0f)),
		M256_1_0__3_0(_mm256_set1_ps(1.0f/3.0f)), M256_46_0__15_0(_mm256_set1_ps(46.0f/15.0f)),
		M256_8_0__5_0(_mm256_set1_ps(8.0f/5.0f))
	{}
	inline __m256 operator() ( __m256 distance ) const {
		__m256 x = _mm256_and_ps( distance, absmask );	// 絶対値
		__m256 x2 = _mm256_mul_ps( x, x );
		__m256 x3 = _mm256_mul_ps( x, x2 );

		// 1以下の時
		__m256 t2_95 = M256_9_0__5_0;
		__m256 t2 = _mm256_mul_ps( x2, t2_95 );	// x*x*9/5
		__m256 t3 = _mm256_mul_ps( x3, M256_1_0__5_0 );	// x*1/5
		__m256 onev = _mm256_add_ps( _mm256_sub_ps( _mm256_sub_ps( x3, t2 ), t3 ), one );

		// 2以下の時
		__m256 t1 = _mm256_mul_ps( x3, M256_1_0__3_0 );	// x*x*x*1/3
		t2 = _mm256_mul_ps( x2, t2_95 );	// x*x*9/5
		t3 = _mm256_mul_ps( x, M256_46_0__15_0 );	// x*46.0f/15.0f
		__m256 twov = _mm256_add_ps( _mm256_sub_ps( _mm256_sub_ps( t2, t1 ), t3 ), M256_8_0__5_0 );

		// 結果を合成
		__m256 onemask = _mm256_cmp_ps( x, one, _CMP_LE_OS ); // x <= 1.0f
		__m256 twomask = _mm256_cmp_ps( x, two, _CMP_LE_OS ); // x <= 2.0f
		twomask = _mm256_andnot_ps( onemask, twomask );	// x > 1.0f && x <= 2.0f
		onev = _mm256_and_ps( onev, onemask );	// 1.0以下の時の値
		twov = _mm256_and_ps( twov, twomask );	// 1.0より大きく、2.0以下の時の値
		return _mm256_or_ps( onev, twov );	// 2.0以下の値を or して、それ以外は0に
	}
};

/**
 * Spline36 用ウェイト関数
 */
struct Spline36WeightAVX : public Spline36Weight {
	const __m256 one;
	const __m256 two;
	const __m256 three;
	const __m256 absmask;
	const __m256 M256_13_0__11_0;
	const __m256 M256_453_0__209_0;
	const __m256 M256_3_0__209_0;
	const __m256 M256_6_0__11_0;
	const __m256 M256_612_0__209_0;
	const __m256 M256_1038_0__209_0;
	const __m256 M256_540_0__209_0;
	const __m256 M256_1_0__11_0;
	const __m256 M256_159_0__209_0;
	const __m256 M256_434_0__209_0;
	const __m256 M256_384_0__209_0;
	inline Spline36WeightAVX() : absmask(_mm256_castsi256_ps(_mm256_set1_epi32(0x7fffffff))),
		one(_mm256_set1_ps(1.0f)), two(_mm256_set1_ps(2.0f)), three(_mm256_set1_ps(3.0f)),
		M256_13_0__11_0(_mm256_set1_ps(13.0f/11.0f)), 
		M256_453_0__209_0(_mm256_set1_ps(453.0f/209.0f)), 
		M256_3_0__209_0(_mm256_set1_ps(3.0f/209.0f)), 
		M256_6_0__11_0(_mm256_set1_ps(6.0f/11.0f)), 
		M256_612_0__209_0(_mm256_set1_ps(612.0f/209.0f)), 
		M256_1038_0__209_0(_mm256_set1_ps(1038.0f/209.0f)), 
		M256_540_0__209_0(_mm256_set1_ps(540.0f/209.0f)), 
		M256_1_0__11_0(_mm256_set1_ps(1.0f/11.0f)), 
		M256_159_0__209_0(_mm256_set1_ps(159.0f/209.0f)), 
		M256_434_0__209_0(_mm256_set1_ps(434.0f/209.0f)), 
		M256_384_0__209_0(_mm256_set1_ps(384.0f/209.0f)) {}
	inline __m256 operator() ( __m256 distance ) const {
		__m256 x = _mm256_and_ps( distance, absmask );	// 絶対値
		__m256 x2 = _mm256_mul_ps( x, x );
		__m256 x3 = _mm256_mul_ps( x, x2 );

		// 1.0以下の時
		__m256 t1 = _mm256_mul_ps( x3, M256_13_0__11_0 );
		__m256 t2 = _mm256_mul_ps( x2, M256_453_0__209_0 );
		__m256 t3 = _mm256_mul_ps( x, M256_3_0__209_0 );
		__m256 onev = _mm256_add_ps( _mm256_sub_ps( _mm256_sub_ps( t1, t2 ), t3 ), one );

		// 2.0以下の時
		t1 = _mm256_mul_ps( x3, M256_6_0__11_0 );
		t2 = _mm256_mul_ps( x2, M256_612_0__209_0 );
		t3 = _mm256_mul_ps( x, M256_1038_0__209_0 );
		__m256 twov = _mm256_add_ps( _mm256_sub_ps( _mm256_sub_ps( t2, t1 ), t3 ), M256_540_0__209_0 );

		// 3.0以下の時
		t1 = _mm256_mul_ps( x3, M256_1_0__11_0 );
		t2 = _mm256_mul_ps( x2, M256_159_0__209_0 );
		t3 = _mm256_mul_ps( x, M256_434_0__209_0 );
		__m256 thrv = _mm256_sub_ps( _mm256_add_ps( _mm256_sub_ps( t1, t2 ), t3 ), M256_384_0__209_0 );

		// 結果を合成
		__m256 onemask = _mm256_cmp_ps( x, one, _CMP_LE_OS ); // x <= 1.0f
		__m256 twomask = _mm256_cmp_ps( x, two, _CMP_LE_OS ); // x <= 2.0f
		__m256 thrmask = _mm256_cmp_ps( x, three, _CMP_LE_OS ); // x <= 3.0f
		thrmask = _mm256_andnot_ps( twomask, thrmask );	// x > 2.0f && x <= 3.0f
		twomask = _mm256_andnot_ps( onemask, twomask );	// x > 1.0f && x <= 2.0f
		onev = _mm256_and_ps( onev, onemask );	// 1.0以下の時の値
		twov = _mm256_and_ps( twov, twomask );	// 1.0より大きく、2.0以下の時の値
		thrv = _mm256_and_ps( thrv, thrmask );	// 2.0より大きく、3.0以下の時の値
		return _mm256_or_ps( _mm256_or_ps( onev, twov ), thrv );
	}
};

/**
 * ガウス関数
 */
struct GaussianWeightAVX : public GaussianWeight {
	const __m256 M256_SQ2PI;
	const __m256 M256_M2_0;
	inline GaussianWeightAVX() : M256_SQ2PI(_mm256_set1_ps(std::sqrt( 2.0f/(float)M_PI ))), M256_M2_0(_mm256_set1_ps(-2.0f)) {}
	inline __m256 operator() ( __m256 x ) const {
		__m256 x2 = _mm256_mul_ps( x, x );
		x2 = _mm256_mul_ps( x2, M256_M2_0 );
		return _mm256_mul_ps( mm256_exp_ps( x2 ), M256_SQ2PI );
	}
};

/**
 * Blackman-Sinc 関数
 */
struct BlackmanSincWeightAVX : public BlackmanSincWeight {
	const __m256 absmask;
	const __m256 M256_COEFF;
	const __m256 M256_EPSILON;
	const __m256 M256_1_0;
	const __m256 M256_PI;
	const __m256 M256_0_5;
	const __m256 M256_2_0;
	const __m256 M256_0_08;
	const __m256 M256_0_42;
	inline BlackmanSincWeightAVX( float c = RANGE ) : BlackmanSincWeight(c), absmask(_mm256_castsi256_ps(_mm256_set1_epi32(0x7fffffff))),
		M256_COEFF(_mm256_set1_ps(1.0f/c)),
		M256_EPSILON(_mm256_set1_ps(FLT_EPSILON)),
		M256_1_0(_mm256_set1_ps(1.0f)),
		M256_PI(_mm256_set1_ps((float)M_PI)),
		M256_0_5(_mm256_set1_ps(0.5f)),
		M256_2_0(_mm256_set1_ps(2.0f)),
		M256_0_08(_mm256_set1_ps(0.08f)),
		M256_0_42(_mm256_set1_ps(0.42f)) {}
	inline __m256 operator() ( __m256 distance ) const {
		__m256 x = _mm256_and_ps( distance, absmask );	// 絶対値
		__m256 pix = _mm256_mul_ps( x, M256_PI );	// PI * x

		__m256 coeff = M256_COEFF;
		coeff = _mm256_mul_ps( coeff, pix );	// coeff * PI * x

		__m256 t2 = mm256_cos_ps( coeff );	// cos(coeff * PI * x)
		t2 = _mm256_mul_ps( t2, M256_0_5 );	// 0.5 * cos(coeff * PI * x)

		__m256 t3 = _mm256_mul_ps( coeff, M256_2_0 );	// 2.0 * coeff * PI * x
		t3 = mm256_cos_ps( t3 );	// cos(2.0 * coeff * PI * x)
		t3 = _mm256_mul_ps( t3, M256_0_08 );	// 0.08 * cos(2.0 * coeff * PI * x)

		// 0.42 + 0.5 * cos(PI * x * coeff) + 0.08 * cos(2.0 * PI * x * coeff)
		__m256 result = _mm256_add_ps( _mm256_add_ps( t2, t3 ), M256_0_42 );	// + 0.42

		__m256 sincpix = mm256_sin_ps( pix );	// sin(PI * x)
		pix  = m256_rcp_22bit_ps( pix );	// 1.0 / (PI * x)
		sincpix = _mm256_mul_ps( sincpix, pix );	// sin(PI * x) / (PI * x)
		result = _mm256_mul_ps( result, sincpix );	// result

		__m256 one = M256_1_0;
		__m256 onemask = _mm256_cmp_ps( x, M256_EPSILON, _CMP_LT_OS );	
		one = _mm256_and_ps( one, onemask );	// x < epsilon ? 1.0
		onemask = _mm256_andnot_ps( onemask, result );	// x >= epsilon ? result
		return _mm256_or_ps( one, onemask );
	}
};

#endif // __WEIGHT_FUNCTOR_AVX_H__
