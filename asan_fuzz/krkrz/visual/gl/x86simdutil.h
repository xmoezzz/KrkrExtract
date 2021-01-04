

#ifndef __X86_SIMD_UTIL_H__
#define __X86_SIMD_UTIL_H__

#include <intrin.h>

// SIMD版数学系関数 ( SSE+SSE2使用 ) 4要素一気に計算する
extern __m128 log_ps(__m128 x);
extern __m128 exp_ps(__m128 x);
extern __m128 sin_ps(__m128 x);
extern __m128 cos_ps(__m128 x);
extern void sincos_ps(__m128 x, __m128 *s, __m128 *c);

// SIMD版数学系関数 ( AVX+AVX2使用 ) 8要素一気に計算する
extern __m256 mm256_sin_ps(__m256 x);
extern __m256 mm256_exp_ps(__m256 x);
extern __m256 mm256_cos_ps(__m256 x);

// exp(y*log(x)) for pow(x, y)
inline __m128 pow_ps( __m128 x, __m128 y ) {
	return exp_ps( _mm_mul_ps( y, log_ps( x ) ) );
}

/**
 * 22bit 精度で逆数を求める(4要素版)
 * aの逆数 = 2 * a - rcpa * a * a を用いる
 */
inline __m128 m128_rcp_22bit_ps( const __m128& a ) {
	__m128 xm0 = a;
	__m128 xm1 = _mm_rcp_ps(xm0);
	xm0 = _mm_mul_ps( _mm_mul_ps( xm0, xm1 ), xm1 );
	xm1 = _mm_add_ps( xm1, xm1 );
	return _mm_sub_ps( xm1, xm0 );
}
/**
 * 22bit 精度で逆数を求める(1要素版)
 */
inline __m128 m128_rcp_22bit_ss( const __m128& a ) {
	__m128 xm0 = a;
	__m128 xm1 = _mm_rcp_ss(xm0);
	xm0 = _mm_mul_ss( _mm_mul_ss( xm0, xm1 ), xm1 );
	xm1 = _mm_add_ss( xm1, xm1 );
	return _mm_sub_ss( xm1, xm0 );
}
/**
 * 22bit 精度で逆数を求める(float版)
 */
inline float rcp_sse( float a ) {
	float  ret;
	__m128 xm0 = _mm_set_ss(a);
	_mm_store_ss( &ret, m128_rcp_22bit_ss(xm0) );
	return ret;
}

/**
 * 22bit 精度で逆数を求める(8要素版)
 * aの逆数 = 2 * a - rcpa * a * a を用いる
 */
inline __m256 m256_rcp_22bit_ps( const __m256& a ) {
	__m256 xm0 = a;
	__m256 xm1 = _mm256_rcp_ps(xm0);
	xm0 = _mm256_mul_ps( _mm256_mul_ps( xm0, xm1 ), xm1 );
	xm1 = _mm256_add_ps( xm1, xm1 );
	return _mm256_sub_ps( xm1, xm0 );
}

/**
 * 22bit 精度で逆数を求める(8要素版)
 * aの逆数 = 2 * a - rcpa * a * a を用いる
 * FMA3 を使って、少し速くする 未確認
 */
inline __m256 m256_rcp_22bit_fma_ps( const __m256& a ) {
	__m256 xm0 = a;
	__m256 xm1 = _mm256_rcp_ps(xm0);
	return _mm256_fnmadd_ps( _mm256_mul_ps( xm0, xm1 ), xm1, _mm256_add_ps( xm1, xm1 ) );
}

/**
 * SSEで4要素の合計値を求める
 * 合計値は全要素に入る
 */
inline __m128 m128_hsum_sse1_ps( __m128 sum ) {
	__m128 tmp = sum;
	sum = _mm_shuffle_ps( sum, tmp, _MM_SHUFFLE(1,0,3,2) );
	sum = _mm_add_ps( sum, tmp );
	tmp = sum;
	sum = _mm_shuffle_ps( sum, tmp, _MM_SHUFFLE(2,3,0,1) );
	return _mm_add_ps( sum, tmp );
}
/**
 * SSEで4要素の合計値を求める
 * 合計値は全要素に入る
 * SSE3 だと少ない命令で実行できる
 */
inline __m128 m128_hsum_sse3_ps( __m128 sum ) {
	sum = _mm_hadd_ps( sum, sum );
	return _mm_hadd_ps( sum, sum );
}

/**
 * AVXで8要素の合計値を求める
 * 合計値は全要素に入る
 */
inline __m256 m256_hsum_avx_ps( __m256 sum ) {
	sum = _mm256_hadd_ps( sum, sum );
	sum = _mm256_hadd_ps( sum, sum );
	__m256 rsum = _mm256_permute2f128_ps(sum, sum, 0 << 4 | 1 );
	sum = _mm256_unpacklo_ps( sum, rsum );
	sum = _mm256_hadd_ps( sum, sum );
	return sum;
}
/**
 * AVX2で16要素の合計値を求める
 * 合計値は全要素に入る
 */
/*
inline __m256 m256_hsum_avx_epi16( __m256 sum ) {
	sum = _mm256_hadd_ps( sum, sum );
	sum = _mm256_hadd_ps( sum, sum );
	return _mm256_hadd_ps( sum, sum );
}
*/
#endif
