//---------------------------------------------------------------------------
/*
	Risa [りさ]		 alias 吉里吉里3 [kirikiri-3]
	 stands for "Risa Is a Stagecraft Architecture"
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
//! @file
//! @brief 実数離散フーリエ変換
//---------------------------------------------------------------------------

#include "tjsCommHead.h"
#if defined(_M_IX86)||defined(_M_X64)
#include "xmmlib.h"

/*
	and

	the original copyright:
*/

/********************************************************************
 *																	*
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.	*
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS		*
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.		*
 *																	*
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2003				*
 * by the XIPHOPHORUS Company http://www.xiph.org/					*
 *																	*
 ********************************************************************/

/*
	Based on blacksword's SSE optimized version of
	Real Discrete FFT package, available at:
	 http://homepage3.nifty.com/blacksword/

	W.Dee: implemented backward transformation of Real Discrete FFT.
*/


//---------------------------------------------------------------------------

#ifdef	_MSC_VER
#pragma warning(push)
#pragma warning(disable : 4700)	// ignore _mm_empty request.
#endif
static inline void cft1st(int n, float * __restrict a, float * __restrict w)
{
	int		j, k1, k2;

	__m128	XMM0, XMM1, XMM2, XMM3, XMM4, XMM5;
//#pragma warning(disable : 592)
	XMM0	 = _mm_loadl_pi(XMM0, (__m64*)(a   ));
	XMM2	 = _mm_loadl_pi(XMM2, (__m64*)(a+ 2));
//#pragma warning(default : 592)
	XMM0	 = _mm_loadh_pi(XMM0, (__m64*)(a+ 4));
	XMM2	 = _mm_loadh_pi(XMM2, (__m64*)(a+ 6));
	XMM1	 = XMM0;
	XMM0	 = _mm_add_ps(XMM0, XMM2);
	XMM1	 = _mm_sub_ps(XMM1, XMM2);
	XMM2	 = XMM0;
	XMM3	 = XMM1;
	XMM0	 = _mm_movelh_ps(XMM0, XMM0);
	XMM2	 = _mm_movehl_ps(XMM2, XMM2);
	XMM1	 = _mm_movelh_ps(XMM1, XMM1);
	XMM3	 = _mm_shuffle_ps(XMM3, XMM3, _MM_SHUFFLE(2,3,2,3));
	XMM2	 = _mm_xor_ps(XMM2, PM128(PCS_RRNN));
	XMM3	 = _mm_xor_ps(XMM3, PM128(PCS_RNNR));
	XMM0	 = _mm_add_ps(XMM0, XMM2);
	XMM1	 = _mm_add_ps(XMM1, XMM3);
	_mm_storel_pi((__m64*)(a   ), XMM0);
	_mm_storeh_pi((__m64*)(a+ 4), XMM0);
	_mm_storel_pi((__m64*)(a+ 2), XMM1);
	_mm_storeh_pi((__m64*)(a+ 6), XMM1);
	XMM0	 = _mm_loadl_pi(XMM0, (__m64*)(a+ 8));
	XMM2	 = _mm_loadl_pi(XMM2, (__m64*)(a+10));
	XMM0	 = _mm_loadh_pi(XMM0, (__m64*)(a+12));
	XMM2	 = _mm_loadh_pi(XMM2, (__m64*)(a+14));
	XMM1	 = XMM0;
	XMM0	 = _mm_add_ps(XMM0, XMM2);
	XMM1	 = _mm_sub_ps(XMM1, XMM2);
	XMM2	 = XMM0;
	XMM3	 = XMM1;
	XMM0	 = _mm_shuffle_ps(XMM0, XMM0, _MM_SHUFFLE(0,3,1,0));
	XMM2	 = _mm_shuffle_ps(XMM2, XMM2, _MM_SHUFFLE(2,1,3,2));
	XMM1	 = _mm_shuffle_ps(XMM1, XMM1, _MM_SHUFFLE(2,3,1,0));
	XMM3	 = _mm_shuffle_ps(XMM3, XMM3, _MM_SHUFFLE(1,0,2,3));
	XMM2	 = _mm_xor_ps(XMM2, PM128(PCS_RRNN));
	XMM3	 = _mm_xor_ps(XMM3, PM128(PCS_RNNR));
	XMM0	 = _mm_add_ps(XMM0, XMM2);
	XMM1	 = _mm_add_ps(XMM1, XMM3);
	_mm_storel_pi((__m64*)(a+ 8), XMM0);
	_mm_storeh_pi((__m64*)(a+12), XMM0);
	XMM2	 = XMM1;
	XMM3	 = _mm_load_ss(w+2);
	XMM1	 = _mm_shuffle_ps(XMM1, XMM1, _MM_SHUFFLE(3,3,0,0));
	XMM2	 = _mm_shuffle_ps(XMM2, XMM2, _MM_SHUFFLE(2,2,1,1));
	XMM3	 = _mm_shuffle_ps(XMM3, XMM3, _MM_SHUFFLE(0,0,0,0));
	XMM2	 = _mm_xor_ps(XMM2, PM128(PCS_NRNR));
	XMM1	 = _mm_add_ps(XMM1, XMM2);
	XMM1	 = _mm_mul_ps(XMM1, XMM3);
	_mm_storel_pi((__m64*)(a+10), XMM1);
	_mm_storeh_pi((__m64*)(a+14), XMM1);
	k1 = 0;
	for (j = 16; j < n; j += 16) {
		k1		+= 2;
		k2		 = 2 * k1;
//#pragma warning(disable : 592)
		XMM4	 = _mm_loadh_pi(XMM4, (__m64*)(w+k1	 ));
		XMM5	 = _mm_loadl_pi(XMM5, (__m64*)(w+k2	 ));
//#pragma warning(default : 592)
		XMM0	 = XMM5;
		XMM1	 = XMM4;
		XMM0	 = _mm_shuffle_ps(XMM0, XMM0, _MM_SHUFFLE(0,1,0,1));
		XMM1	 = _mm_shuffle_ps(XMM1, XMM1, _MM_SHUFFLE(3,3,3,3));
		XMM0	 = _mm_mul_ps(XMM0, XMM1);
		XMM0	 = _mm_add_ps(XMM0, XMM0);
		XMM0	 = _mm_sub_ps(XMM0, XMM5);
		XMM0	 = _mm_xor_ps(XMM0, PM128(PCS_NRNR));
		XMM5	 = _mm_movelh_ps(XMM5, XMM0);

		XMM0	 = _mm_loadl_pi(XMM0, (__m64*)(a+j	 ));
		XMM2	 = _mm_loadl_pi(XMM2, (__m64*)(a+j+ 2));
		XMM0	 = _mm_loadh_pi(XMM0, (__m64*)(a+j+ 4));
		XMM2	 = _mm_loadh_pi(XMM2, (__m64*)(a+j+ 6));
		XMM1	 = XMM0;
		XMM0	 = _mm_add_ps(XMM0, XMM2);
		XMM1	 = _mm_sub_ps(XMM1, XMM2);
		XMM2	 = XMM0;
		XMM3	 = XMM1;
		XMM0	 = _mm_movelh_ps(XMM0, XMM0);
		XMM2	 = _mm_movehl_ps(XMM2, XMM2);
		XMM1	 = _mm_movelh_ps(XMM1, XMM1);
		XMM3	 = _mm_shuffle_ps(XMM3, XMM3, _MM_SHUFFLE(2,3,2,3));
		XMM2	 = _mm_xor_ps(XMM2, PM128(PCS_RRNN));
		XMM3	 = _mm_xor_ps(XMM3, PM128(PCS_RNNR));
		XMM0	 = _mm_add_ps(XMM0, XMM2);
		XMM1	 = _mm_add_ps(XMM1, XMM3);
		_mm_storel_pi((__m64*)(a+j	 ), XMM0);
		XMM2	 = XMM0;
		XMM3	 = XMM4;
		XMM2	 = _mm_shuffle_ps(XMM2, XMM2, _MM_SHUFFLE(2,3,2,3));
		XMM4	 = _mm_shuffle_ps(XMM4, XMM4, _MM_SHUFFLE(2,2,2,2));
		XMM3	 = _mm_shuffle_ps(XMM3, XMM3, _MM_SHUFFLE(3,3,3,3));
		XMM0	 = _mm_mul_ps(XMM0, XMM4);
		XMM2	 = _mm_mul_ps(XMM2, XMM3);
		XMM2	 = _mm_xor_ps(XMM2, PM128(PCS_NRNR));
		XMM0	 = _mm_add_ps(XMM0, XMM2);
		_mm_storeh_pi((__m64*)(a+j+ 4), XMM0);
		XMM4	 = XMM1;
		XMM3	 = XMM5;
		XMM4	 = _mm_shuffle_ps(XMM4, XMM4, _MM_SHUFFLE(2,3,0,1));
		XMM5	 = _mm_shuffle_ps(XMM5, XMM5, _MM_SHUFFLE(2,2,0,0));
		XMM3	 = _mm_shuffle_ps(XMM3, XMM3, _MM_SHUFFLE(3,3,1,1));
		XMM1	 = _mm_mul_ps(XMM1, XMM5);
		XMM4	 = _mm_mul_ps(XMM4, XMM3);
		XMM4	 = _mm_xor_ps(XMM4, PM128(PCS_NRNR));
		XMM1	 = _mm_add_ps(XMM1, XMM4);
		_mm_storel_pi((__m64*)(a+j+ 2), XMM1);
		_mm_storeh_pi((__m64*)(a+j+ 6), XMM1);
		XMM4	 = _mm_loadh_pi(XMM4, (__m64*)(w+k1	 ));
		XMM5	 = _mm_loadl_pi(XMM5, (__m64*)(w+k2+2));
		XMM0	 = XMM5;
		XMM1	 = XMM4;
		XMM0	 = _mm_shuffle_ps(XMM0, XMM0, _MM_SHUFFLE(0,1,0,1));
		XMM1	 = _mm_shuffle_ps(XMM1, XMM1, _MM_SHUFFLE(2,2,2,2));
		XMM0	 = _mm_mul_ps(XMM0, XMM1);
		XMM0	 = _mm_add_ps(XMM0, XMM0);
		XMM0	 = _mm_sub_ps(XMM0, XMM5);
		XMM0	 = _mm_xor_ps(XMM0, PM128(PCS_NRNR));
		XMM5	 = _mm_movelh_ps(XMM5, XMM0);

		XMM0	 = _mm_loadl_pi(XMM0, (__m64*)(a+j+ 8));
		XMM2	 = _mm_loadl_pi(XMM2, (__m64*)(a+j+10));
		XMM0	 = _mm_loadh_pi(XMM0, (__m64*)(a+j+12));
		XMM2	 = _mm_loadh_pi(XMM2, (__m64*)(a+j+14));
		XMM1	 = XMM0;
		XMM0	 = _mm_add_ps(XMM0, XMM2);
		XMM1	 = _mm_sub_ps(XMM1, XMM2);
		XMM2	 = XMM0;
		XMM3	 = XMM1;
		XMM0	 = _mm_movelh_ps(XMM0, XMM0);
		XMM2	 = _mm_movehl_ps(XMM2, XMM2);
		XMM1	 = _mm_movelh_ps(XMM1, XMM1);
		XMM3	 = _mm_shuffle_ps(XMM3, XMM3, _MM_SHUFFLE(2,3,2,3));
		XMM2	 = _mm_xor_ps(XMM2, PM128(PCS_RRNN));
		XMM3	 = _mm_xor_ps(XMM3, PM128(PCS_RNNR));
		XMM0	 = _mm_add_ps(XMM0, XMM2);
		XMM1	 = _mm_add_ps(XMM1, XMM3);
		_mm_storel_pi((__m64*)(a+j+ 8), XMM0);
		XMM2	 = XMM0;
		XMM3	 = XMM4;
		XMM0	 = _mm_shuffle_ps(XMM0, XMM0, _MM_SHUFFLE(2,3,2,3));
		XMM4	 = _mm_shuffle_ps(XMM4, XMM4, _MM_SHUFFLE(2,2,2,2));
		XMM3	 = _mm_shuffle_ps(XMM3, XMM3, _MM_SHUFFLE(3,3,3,3));
		XMM0	 = _mm_mul_ps(XMM0, XMM4);
		XMM2	 = _mm_mul_ps(XMM2, XMM3);
		XMM0	 = _mm_xor_ps(XMM0, PM128(PCS_NRNR));
		XMM0	 = _mm_sub_ps(XMM0, XMM2);
		_mm_storeh_pi((__m64*)(a+j+12), XMM0);
		XMM4	 = XMM1;
		XMM3	 = XMM5;
		XMM4	 = _mm_shuffle_ps(XMM4, XMM4, _MM_SHUFFLE(2,3,0,1));
		XMM5	 = _mm_shuffle_ps(XMM5, XMM5, _MM_SHUFFLE(2,2,0,0));
		XMM3	 = _mm_shuffle_ps(XMM3, XMM3, _MM_SHUFFLE(3,3,1,1));
		XMM1	 = _mm_mul_ps(XMM1, XMM5);
		XMM4	 = _mm_mul_ps(XMM4, XMM3);
		XMM4	 = _mm_xor_ps(XMM4, PM128(PCS_NRNR));
		XMM1	 = _mm_add_ps(XMM1, XMM4);
		_mm_storel_pi((__m64*)(a+j+10), XMM1);
		_mm_storeh_pi((__m64*)(a+j+14), XMM1);
	}
}


static inline void cftmdl(int n, int l, float * __restrict a, float * __restrict w)
{
	int j, j1, j2, j3, k, k1, k2, m, m2;
	float wk1r, wk1i, wk2r, wk2i, wk3r, wk3i;
	__m128	XMM6;
	__m128	pwk1r, pwk1i, pwk2r, pwk2i, pwk3r, pwk3i;

	m = l << 2;
	for (j = 0; j < l; j += 4) {
		__m128	XMM0, XMM1, XMM2, XMM3, XMM4, XMM5;

		j1 = j	+ l;
		j2 = j1 + l;
		j3 = j2 + l;

#if	defined(__GNUC__)
		XMM0	 = _mm_load_ps(a+j );
		XMM2	 = _mm_load_ps(a+j2);
		XMM1	 = XMM0;
		XMM3	 = XMM2;
		XMM0	 = _mm_add_ps(XMM0, PM128(a+j1));
		XMM1	 = _mm_sub_ps(XMM1, PM128(a+j1));
		XMM2	 = _mm_add_ps(XMM2, PM128(a+j3));
		XMM3	 = _mm_sub_ps(XMM3, PM128(a+j3));
#else
		XMM0	 = _mm_load_ps(a+j );
		XMM4	 = _mm_load_ps(a+j1);
		XMM2	 = _mm_load_ps(a+j2);
		XMM5	 = _mm_load_ps(a+j3);
		XMM1	 = XMM0;
		XMM3	 = XMM2;
		XMM0	 = _mm_add_ps(XMM0, XMM4);
		XMM2	 = _mm_add_ps(XMM2, XMM5);
		XMM1	 = _mm_sub_ps(XMM1, XMM4);
		XMM3	 = _mm_sub_ps(XMM3, XMM5);
#endif
		XMM4	 = XMM0;
		XMM5	 = XMM1;
		XMM3	 = _mm_shuffle_ps(XMM3, XMM3, _MM_SHUFFLE(2,3,0,1));
		XMM0	 = _mm_add_ps(XMM0, XMM2);
		XMM3	 = _mm_xor_ps(XMM3, PM128(PCS_NRNR));
		XMM4	 = _mm_sub_ps(XMM4, XMM2);
		XMM1	 = _mm_add_ps(XMM1, XMM3);
		XMM5	 = _mm_sub_ps(XMM5, XMM3);
		_mm_store_ps(a+j , XMM0);
		_mm_store_ps(a+j1, XMM1);
		_mm_store_ps(a+j2, XMM4);
		_mm_store_ps(a+j3, XMM5);
	}
	XMM6	 = _mm_set1_ps(w[2]);
	for (j = m; j < l + m; j += 4) {
		__m128	XMM0, XMM1, XMM2, XMM3, XMM4, XMM5;

		j1 = j	+ l;
		j2 = j1 + l;
		j3 = j2 + l;

#if	defined(__GNUC__)
		XMM0	 = _mm_load_ps(a+j );
		XMM2	 = _mm_load_ps(a+j2);
		XMM1	 = XMM0;
		XMM3	 = XMM2;
		XMM0	 = _mm_add_ps(XMM0, PM128(a+j1));
		XMM1	 = _mm_sub_ps(XMM1, PM128(a+j1));
		XMM2	 = _mm_add_ps(XMM2, PM128(a+j3));
		XMM3	 = _mm_sub_ps(XMM3, PM128(a+j3));
#else
		XMM0	 = _mm_load_ps(a+j );
		XMM4	 = _mm_load_ps(a+j1);
		XMM2	 = _mm_load_ps(a+j2);
		XMM5	 = _mm_load_ps(a+j3);
		XMM1	 = XMM0;
		XMM3	 = XMM2;
		XMM0	 = _mm_add_ps(XMM0, XMM4);
		XMM2	 = _mm_add_ps(XMM2, XMM5);
		XMM1	 = _mm_sub_ps(XMM1, XMM4);
		XMM3	 = _mm_sub_ps(XMM3, XMM5);
#endif

		XMM4	 = XMM0;
		XMM5	 = XMM0;
		XMM4	 = _mm_shuffle_ps(XMM4, XMM2, _MM_SHUFFLE(3,1,2,0));	/* (x2i_1,x2i_0,x0r_1,x0r_0) */
		XMM5	 = _mm_shuffle_ps(XMM5, XMM2, _MM_SHUFFLE(2,0,3,1));	/* (x2r_1,x2r_0,x0i_1,x0i_0) */
		XMM4	 = _mm_shuffle_ps(XMM4, XMM4, _MM_SHUFFLE(1,3,0,2));	/* (x0r_1,x2i_1,x0r_0,x2i_0) */
		XMM5	 = _mm_shuffle_ps(XMM5, XMM5, _MM_SHUFFLE(3,1,2,0));	/* (x2r_1,x0i_1,x2r_0,x0i_0) */
		XMM0	 = _mm_add_ps(XMM0, XMM2);
		XMM4	 = _mm_sub_ps(XMM4, XMM5);
		_mm_store_ps(a+j , XMM0);
		_mm_store_ps(a+j2, XMM4);

		XMM0	 = XMM1;												/* x1  */
		XMM2	 = XMM3;												/* x3  */
		XMM0	 = _mm_shuffle_ps(XMM0, XMM0, _MM_SHUFFLE(2,2,0,0));	/* x1r */
		XMM1	 = _mm_shuffle_ps(XMM1, XMM1, _MM_SHUFFLE(3,3,1,1));	/* x1i */
		XMM2	 = _mm_shuffle_ps(XMM2, XMM2, _MM_SHUFFLE(2,2,0,0));	/* x3r */
		XMM3	 = _mm_shuffle_ps(XMM3, XMM3, _MM_SHUFFLE(3,3,1,1));	/* x3i */
		XMM4	 = XMM0;												/* x1r */
		XMM5	 = XMM1;												/* x1i */
		XMM0	 = _mm_sub_ps(XMM0, XMM3);								/* x1r - x3i */
		XMM1	 = _mm_add_ps(XMM1, XMM2);								/* x1i + x3r */
		XMM2	 = _mm_sub_ps(XMM2, XMM5);								/* x3r - x1i */
		XMM3	 = _mm_add_ps(XMM3, XMM4);								/* x3i + x1r */
		XMM1	 = _mm_xor_ps(XMM1, PM128(PCS_NRNR));
		XMM3	 = _mm_xor_ps(XMM3, PM128(PCS_NRNR));
		XMM0	 = _mm_add_ps(XMM0, XMM1);
		XMM2	 = _mm_add_ps(XMM2, XMM3);
		XMM0	 = _mm_mul_ps(XMM0, XMM6);
		XMM2	 = _mm_mul_ps(XMM2, XMM6);
		_mm_store_ps(a+j1, XMM0);
		_mm_store_ps(a+j3, XMM2);
	}
	k1 = 0;
	m2 = 2 * m;
	for (k = m2; k < n; k += m2) {
		k1 += 2;
		k2 = 2 * k1;
		wk2r = w[k1];
		wk2i = w[k1 + 1];
		wk1r = w[k2];
		wk1i = w[k2 + 1];
		wk3r = wk1r - 2 * wk2i * wk1i;
		wk3i = 2 * wk2i * wk1r - wk1i;
		pwk1r	 = _mm_set1_ps(wk1r);
		pwk1i	 = _mm_set1_ps(wk1i);
		pwk2r	 = _mm_set1_ps(wk2r);
		pwk2i	 = _mm_set1_ps(wk2i);
		pwk3r	 = _mm_set1_ps(wk3r);
		pwk3i	 = _mm_set1_ps(wk3i);
		pwk1i	 = _mm_xor_ps(pwk1i, PM128(PCS_NRNR));
		pwk2i	 = _mm_xor_ps(pwk2i, PM128(PCS_NRNR));
		pwk3i	 = _mm_xor_ps(pwk3i, PM128(PCS_NRNR));
		for (j = k; j < l + k; j += 4) {
			__m128	XMM0, XMM1, XMM2, XMM3, XMM4, XMM5;

			j1 = j	+ l;
			j2 = j1 + l;
			j3 = j2 + l;
#if	defined(__GNUC__)
			XMM0	 = _mm_load_ps(a+j );
			XMM2	 = _mm_load_ps(a+j2);
			XMM1	 = XMM0;
			XMM3	 = XMM2;
			XMM0	 = _mm_add_ps(XMM0, PM128(a+j1));
			XMM1	 = _mm_sub_ps(XMM1, PM128(a+j1));
			XMM2	 = _mm_add_ps(XMM2, PM128(a+j3));
			XMM3	 = _mm_sub_ps(XMM3, PM128(a+j3));
#else
			XMM0	 = _mm_load_ps(a+j );
			XMM4	 = _mm_load_ps(a+j1);
			XMM2	 = _mm_load_ps(a+j2);
			XMM5	 = _mm_load_ps(a+j3);
			XMM1	 = XMM0;
			XMM3	 = XMM2;
			XMM0	 = _mm_add_ps(XMM0, XMM4);
			XMM2	 = _mm_add_ps(XMM2, XMM5);
			XMM1	 = _mm_sub_ps(XMM1, XMM4);
			XMM3	 = _mm_sub_ps(XMM3, XMM5);
#endif

			XMM4	 = XMM0;
			XMM5	 = XMM0;
			XMM6	 = XMM2;
			XMM5	 = _mm_shuffle_ps(XMM5, XMM5, _MM_SHUFFLE(2,3,0,1));
			XMM6	 = _mm_shuffle_ps(XMM6, XMM6, _MM_SHUFFLE(2,3,0,1));
			XMM4	 = _mm_sub_ps(XMM4, XMM2);
			XMM5	 = _mm_sub_ps(XMM5, XMM6);
			XMM4	 = _mm_mul_ps(XMM4, pwk2r);
			XMM5	 = _mm_mul_ps(XMM5, pwk2i);
			XMM0	 = _mm_add_ps(XMM0, XMM2);
			XMM4	 = _mm_add_ps(XMM4, XMM5);
			_mm_store_ps(a+j , XMM0);
			_mm_store_ps(a+j2, XMM4);

			XMM0	 = XMM1;
			XMM5	 = XMM3;
			XMM4	 = XMM1;
			XMM5	 = _mm_shuffle_ps(XMM5, XMM5, _MM_SHUFFLE(2,3,0,1));
			XMM4	 = _mm_shuffle_ps(XMM4, XMM4, _MM_SHUFFLE(2,3,0,1));
			XMM2	 = XMM4;
			XMM5	 = _mm_xor_ps(XMM5, PM128(PCS_NRNR));
			XMM3	 = _mm_xor_ps(XMM3, PM128(PCS_NRNR));
			XMM1	 = _mm_add_ps(XMM1, XMM5);
			XMM4	 = _mm_sub_ps(XMM4, XMM3);
			XMM0	 = _mm_sub_ps(XMM0, XMM5);
			XMM2	 = _mm_add_ps(XMM2, XMM3);
			XMM1	 = _mm_mul_ps(XMM1, pwk1r);
			XMM4	 = _mm_mul_ps(XMM4, pwk1i);
			XMM0	 = _mm_mul_ps(XMM0, pwk3r);
			XMM2	 = _mm_mul_ps(XMM2, pwk3i);
			XMM1	 = _mm_add_ps(XMM1, XMM4);
			XMM0	 = _mm_add_ps(XMM0, XMM2);
			_mm_store_ps(a+j1, XMM1);
			_mm_store_ps(a+j3, XMM0);
		}
		wk1r = w[k2 + 2];
		wk1i = w[k2 + 3];
		wk3r = wk1r - 2 * wk2r * wk1i;
		wk3i = 2 * wk2r * wk1r - wk1i;
		pwk1r	 = _mm_set1_ps(wk1r);
		pwk1i	 = _mm_set1_ps(wk1i);
		pwk2r	 = _mm_set1_ps(wk2r);
		pwk2i	 = _mm_set1_ps(wk2i);
		pwk3r	 = _mm_set1_ps(wk3r);
		pwk3i	 = _mm_set1_ps(wk3i);
		pwk1i	 = _mm_xor_ps(pwk1i, PM128(PCS_NRNR));
		pwk2r	 = _mm_xor_ps(pwk2r, PM128(PCS_NRNR));
		pwk3i	 = _mm_xor_ps(pwk3i, PM128(PCS_NRNR));
		for (j = k + m; j < l + (k + m); j += 4) {
			__m128	XMM0, XMM1, XMM2, XMM3, XMM4, XMM5;

			j1 = j	+ l;
			j2 = j1 + l;
			j3 = j2 + l;

#if	defined(__GNUC__)
			XMM0	 = _mm_load_ps(a+j );
			XMM2	 = _mm_load_ps(a+j2);
			XMM1	 = XMM0;
			XMM3	 = XMM2;
			XMM0	 = _mm_add_ps(XMM0, PM128(a+j1));
			XMM1	 = _mm_sub_ps(XMM1, PM128(a+j1));
			XMM2	 = _mm_add_ps(XMM2, PM128(a+j3));
			XMM3	 = _mm_sub_ps(XMM3, PM128(a+j3));
#else
			XMM0	 = _mm_load_ps(a+j );
			XMM4	 = _mm_load_ps(a+j1);
			XMM2	 = _mm_load_ps(a+j2);
			XMM5	 = _mm_load_ps(a+j3);
			XMM1	 = XMM0;
			XMM3	 = XMM2;
			XMM0	 = _mm_add_ps(XMM0, XMM4);
			XMM2	 = _mm_add_ps(XMM2, XMM5);
			XMM1	 = _mm_sub_ps(XMM1, XMM4);
			XMM3	 = _mm_sub_ps(XMM3, XMM5);
#endif

			XMM4	 = XMM0;
			XMM5	 = XMM0;
			XMM6	 = XMM2;
			XMM5	 = _mm_shuffle_ps(XMM5, XMM5, _MM_SHUFFLE(2,3,0,1));
			XMM6	 = _mm_shuffle_ps(XMM6, XMM6, _MM_SHUFFLE(2,3,0,1));
			XMM4	 = _mm_sub_ps(XMM4, XMM2);
			XMM5	 = _mm_sub_ps(XMM5, XMM6);
			XMM4	 = _mm_mul_ps(XMM4, pwk2i);
			XMM5	 = _mm_mul_ps(XMM5, pwk2r);
			XMM0	 = _mm_add_ps(XMM0, XMM2);
			XMM5	 = _mm_sub_ps(XMM5, XMM4);
			_mm_store_ps(a+j , XMM0);
			_mm_store_ps(a+j2, XMM5);

			XMM0	 = XMM1;
			XMM5	 = XMM3;
			XMM4	 = XMM1;
			XMM5	 = _mm_shuffle_ps(XMM5, XMM5, _MM_SHUFFLE(2,3,0,1));
			XMM4	 = _mm_shuffle_ps(XMM4, XMM4, _MM_SHUFFLE(2,3,0,1));
			XMM2	 = XMM4;
			XMM5	 = _mm_xor_ps(XMM5, PM128(PCS_NRNR));
			XMM3	 = _mm_xor_ps(XMM3, PM128(PCS_NRNR));
			XMM1	 = _mm_add_ps(XMM1, XMM5);
			XMM4	 = _mm_sub_ps(XMM4, XMM3);
			XMM0	 = _mm_sub_ps(XMM0, XMM5);
			XMM2	 = _mm_add_ps(XMM2, XMM3);
			XMM1	 = _mm_mul_ps(XMM1, pwk1r);
			XMM4	 = _mm_mul_ps(XMM4, pwk1i);
			XMM0	 = _mm_mul_ps(XMM0, pwk3r);
			XMM2	 = _mm_mul_ps(XMM2, pwk3i);
			XMM1	 = _mm_add_ps(XMM1, XMM4);
			XMM0	 = _mm_add_ps(XMM0, XMM2);
			_mm_store_ps(a+j1, XMM1);
			_mm_store_ps(a+j3, XMM0);
		}
	}
}


static inline void bitrv2(int n, int * __restrict ip, float * __restrict a)
{
	int j, j1, k, k1, l, m, m2;
	float xr, xi, yr, yi;

	ip[0] = 0;
	l = n;
	m = 1;
	while ((m << 3) < l) {
		l >>= 1;
		for (j = 0; j < m; j++) {
			ip[m + j] = ip[j] + l;
		}
		m <<= 1;
	}
	m2 = 2 * m;
	if ((m << 3) == l) {
		for (k = 0; k < m; k++) {
			for (j = 0; j < k; j++) {
				__m128	X0, Y0, X1, Y1;
				j1 = 2 * j + ip[k];
				k1 = 2 * k + ip[j];
//#pragma warning(disable : 592)
				X0	 = _mm_loadl_pi(X0, (__m64*)(a+j1	  ));
				Y0	 = _mm_loadl_pi(Y0, (__m64*)(a+k1	  ));
				X1	 = _mm_loadl_pi(X1, (__m64*)(a+j1+m2*2));
				Y1	 = _mm_loadl_pi(Y1, (__m64*)(a+k1+m2  ));
//#pragma warning(default : 592)
				X0	 = _mm_loadh_pi(X0, (__m64*)(a+j1+m2  ));
				Y0	 = _mm_loadh_pi(Y0, (__m64*)(a+k1+m2*2));
				X1	 = _mm_loadh_pi(X1, (__m64*)(a+j1+m2*3));
				Y1	 = _mm_loadh_pi(Y1, (__m64*)(a+k1+m2*3));
				_mm_storel_pi((__m64*)(a+k1		), X0);
				_mm_storel_pi((__m64*)(a+j1		), Y0);
				_mm_storel_pi((__m64*)(a+k1+m2	), X1);
				_mm_storel_pi((__m64*)(a+j1+m2*2), Y1);
				_mm_storeh_pi((__m64*)(a+k1+m2*2), X0);
				_mm_storeh_pi((__m64*)(a+j1+m2	), Y0);
				_mm_storeh_pi((__m64*)(a+k1+m2*3), X1);
				_mm_storeh_pi((__m64*)(a+j1+m2*3), Y1);
			}
			j1 = 2 * k + m2 + ip[k];
			k1 = j1 + m2;
			xr = a[j1];
			xi = a[j1 + 1];
			yr = a[k1];
			yi = a[k1 + 1];
			a[j1] = yr;
			a[j1 + 1] = yi;
			a[k1] = xr;
			a[k1 + 1] = xi;
		}
	} else {
		for (k = 1; k < m; k++) {
			for (j = 0; j < k; j++) {
				__m128	X,	Y;
				j1 = 2 * j + ip[k];
				k1 = 2 * k + ip[j];
//#pragma warning(disable : 592)
				X	 = _mm_loadl_pi(X, (__m64*)(a+j1   ));
				Y	 = _mm_loadl_pi(Y, (__m64*)(a+k1   ));
//#pragma warning(default : 592)
				X	 = _mm_loadh_pi(X, (__m64*)(a+j1+m2));
				Y	 = _mm_loadh_pi(Y, (__m64*)(a+k1+m2));
				_mm_storel_pi((__m64*)(a+k1	  ), X);
				_mm_storel_pi((__m64*)(a+j1	  ), Y);
				_mm_storeh_pi((__m64*)(a+k1+m2), X);
				_mm_storeh_pi((__m64*)(a+j1+m2), Y);
			}
		}
	}
}

static inline void cftfsub(int n, float * __restrict a, float * __restrict w)
{
	int j, j1, j2, j3, l;

	l = 2;
	if (n > 8) {
		cft1st(n, a, w);
		l = 8;
		while ((l << 2) < n) {
			cftmdl(n, l, a, w);
			l <<= 2;
		}
	}
	if ((l << 2) == n) {
		for (j = 0; j < l; j += 4) {
			__m128	XMM0, XMM1, XMM2, XMM3, XMM4, XMM5;

			j1 = j	+ l;
			j2 = j1 + l;
			j3 = j2 + l;

#if	defined(__GNUC__)
			XMM0	 = _mm_load_ps(a+j );
			XMM2	 = _mm_load_ps(a+j2);
			XMM1	 = XMM0;
			XMM3	 = XMM2;
			XMM0	 = _mm_add_ps(XMM0, PM128(a+j1));
			XMM1	 = _mm_sub_ps(XMM1, PM128(a+j1));
			XMM2	 = _mm_add_ps(XMM2, PM128(a+j3));
			XMM3	 = _mm_sub_ps(XMM3, PM128(a+j3));
#else
			XMM0	 = _mm_load_ps(a+j );
			XMM4	 = _mm_load_ps(a+j1);
			XMM2	 = _mm_load_ps(a+j2);
			XMM5	 = _mm_load_ps(a+j3);
			XMM1	 = XMM0;
			XMM3	 = XMM2;
			XMM0	 = _mm_add_ps(XMM0, XMM4);
			XMM2	 = _mm_add_ps(XMM2, XMM5);
			XMM1	 = _mm_sub_ps(XMM1, XMM4);
			XMM3	 = _mm_sub_ps(XMM3, XMM5);
#endif
			XMM4	 = XMM0;
			XMM5	 = XMM1;
			XMM3	 = _mm_shuffle_ps(XMM3, XMM3, _MM_SHUFFLE(2,3,0,1));
			XMM0	 = _mm_add_ps(XMM0, XMM2);
			XMM4	 = _mm_sub_ps(XMM4, XMM2);
			XMM3	 = _mm_xor_ps(XMM3, PM128(PCS_NRNR));
			_mm_store_ps(a+j , XMM0);
			_mm_store_ps(a+j2, XMM4);
			XMM1	 = _mm_add_ps(XMM1, XMM3);
			XMM5	 = _mm_sub_ps(XMM5, XMM3);
			_mm_store_ps(a+j1, XMM1);
			_mm_store_ps(a+j3, XMM5);
		}
	} else {
		for (j = 0; j < l; j += 8)
		{
			__m128	XMM0, XMM1, XMM2, XMM3, XMM4, XMM5;
			j1 = j + l;

#if	defined(__GNUC__)
			XMM0	 = _mm_load_ps(a+j	 );
			XMM1	 = _mm_load_ps(a+j+ 4);
			XMM2	 = XMM0;
			XMM3	 = XMM1;
			XMM0	 = _mm_add_ps(XMM0, PM128(a+j1	));
			XMM1	 = _mm_add_ps(XMM1, PM128(a+j1+4));
			XMM2	 = _mm_sub_ps(XMM2, PM128(a+j1	));
			XMM3	 = _mm_sub_ps(XMM3, PM128(a+j1+4));
#else
			XMM0	 = _mm_load_ps(a+j	 );
			XMM4	 = _mm_load_ps(a+j1	 );
			XMM1	 = _mm_load_ps(a+j+ 4);
			XMM5	 = _mm_load_ps(a+j1+4);
			XMM2	 = XMM0;
			XMM3	 = XMM1;
			XMM0	 = _mm_add_ps(XMM0, XMM4);
			XMM1	 = _mm_add_ps(XMM1, XMM5);
			XMM2	 = _mm_sub_ps(XMM2, XMM4);
			XMM3	 = _mm_sub_ps(XMM3, XMM5);
#endif
			_mm_store_ps(a+j   , XMM0);
			_mm_store_ps(a+j +4, XMM1);
			_mm_store_ps(a+j1  , XMM2);
			_mm_store_ps(a+j1+4, XMM3);
		}
	}
}

static inline void rftfsub(int n, float * __restrict a, int nc, float * __restrict c)
{
	int		j, k, kk, ks, m;

	m	 = n >> 1;
	ks	 = 2 * nc / m;
	kk	 = 0;
	j	 = 2;
	{
		float	wkr, wki, xr, xi, yr, yi;
		k	 = n - j;
		kk	+= ks;
		wkr	 = 0.5f - c[nc - kk];
		wki	 = c[kk];
		xr	 = a[j	] - a[k	 ];
		xi	 = a[j+1] + a[k+1];
		yr	 = wkr * xr - wki * xi;
		yi	 = wkr * xi + wki * xr;
		a[j	 ]	-= yr;
		a[j+1]	-= yi;
		a[k	 ]	+= yr;
		a[k+1]	-= yi;
		j	+= 2;
	}
	n	-= 2;
	kk	+= ks;
	for(;j<m;j+=4)
	{
		__m128	XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6;
		k	 = n - j;
		XMM0	 = _mm_load_ss(PFV_0P5);
		XMM1	 = _mm_load_ss(c+kk	  );
//#pragma warning(disable : 592)
		XMM6	 = _mm_loadl_pi(XMM6, (__m64*)(a+k+2));
//#pragma warning(default : 592)
		XMM2	 = XMM0;
		XMM3	 = _mm_load_ss(c+kk+ks);
		XMM4	 = _mm_load_ss(c+nc-kk	 );
		XMM5	 = _mm_load_ss(c+nc-kk-ks);
		XMM6	 = _mm_loadh_pi(XMM6, (__m64*)(a+k	));
		XMM0	 = _mm_sub_ss(XMM0, XMM4);
		XMM2	 = _mm_sub_ss(XMM2, XMM5);
		XMM4	 = XMM6;
		XMM5	 = _mm_load_ps(a+j);
		XMM6	 = _mm_xor_ps(XMM6, PM128(PCS_NRNR));
		XMM0	 = _mm_shuffle_ps(XMM0, XMM2, _MM_SHUFFLE(0,0,0,0));
		XMM1	 = _mm_shuffle_ps(XMM1, XMM3, _MM_SHUFFLE(0,0,0,0));
		XMM6	 = _mm_add_ps(XMM6, XMM5);
		XMM2	 = XMM6;
		XMM2	 = _mm_shuffle_ps(XMM2, XMM2, _MM_SHUFFLE(2,3,0,1));
		XMM6	 = _mm_mul_ps(XMM6, XMM0);
		XMM2	 = _mm_mul_ps(XMM2, XMM1);
		XMM2	 = _mm_xor_ps(XMM2, PM128(PCS_NRNR));
		XMM6	 = _mm_add_ps(XMM6, XMM2);
		XMM0	 = XMM6;
		XMM5	 = _mm_sub_ps(XMM5, XMM6);
		XMM0	 = _mm_xor_ps(XMM0, PM128(PCS_NRNR));
		_mm_store_ps(a+j, XMM5);
		XMM4	 = _mm_sub_ps(XMM4, XMM0);
		_mm_storel_pi((__m64*)(a+k+2), XMM4);
		_mm_storeh_pi((__m64*)(a+k	), XMM4);
		kk	+= ks*2;
	}
}

static inline void cftbsub(int n, float * __restrict a, float * __restrict w)
{
	void cft1st(int n, float * __restrict a, float * __restrict w);
	void cftmdl(int n, int l, float * __restrict a, float * __restrict w);
	int j, j1, j2, j3, l;
	float x0r, x0i;

	l = 2;
	if (n > 8) {
		cft1st(n, a, w);
		l = 8;
		while ((l << 2) < n) {
			cftmdl(n, l, a, w);
			l <<= 2;
		}
	}

	if ((l << 2) == n) {
		for (j = 0; j < l; j += 2) {
			j1 = j	+ l;
			j2 = j1 + l;
			j3 = j2 + l;

			__m128 j2j0, j3j1, x1x0, x3x2;

			j2j0 = _mm_loadl_pi(j2j0, (__m64*)(a+j	));
			j2j0 = _mm_loadh_pi(j2j0, (__m64*)(a+j2 ));
			j3j1 = _mm_loadl_pi(j3j1, (__m64*)(a+j1 ));
			j3j1 = _mm_loadh_pi(j3j1, (__m64*)(a+j3 ));

			x1x0 = _mm_add_ps(
					_mm_xor_ps(_mm_shuffle_ps(j2j0, j2j0, _MM_SHUFFLE(1, 0, 1, 0)), PM128(PCS_RNRN)),
					_mm_xor_ps(_mm_shuffle_ps(j3j1, j3j1, _MM_SHUFFLE(1, 0, 1, 0)), PM128(PCS_NRRN))
					);

			x3x2 = _mm_add_ps(
							   _mm_shuffle_ps(j2j0, j2j0, _MM_SHUFFLE(3, 2, 3, 2)),
					_mm_xor_ps(_mm_shuffle_ps(j3j1, j3j1, _MM_SHUFFLE(3, 2, 3, 2)), PM128(PCS_RRNN))
					);
/*
			x0r =  a[j	   ] + a[j1	   ];
			x0i = -a[j	+ 1] - a[j1 + 1];
			x1r =  a[j	   ] - a[j1	   ];
			x1i = -a[j	+ 1] + a[j1 + 1];

			x2r =  a[j2	   ] + a[j3	   ];
			x2i =  a[j2 + 1] + a[j3 + 1];
			x3r =  a[j2	   ] - a[j3	   ];
			x3i =  a[j2 + 1] - a[j3 + 1];
*/
			__m128 t, m;
			__m128 x3r_x3i_x2i_x2r =
				_mm_shuffle_ps(x3x2, x3x2, _MM_SHUFFLE(2,3,1,0));


			m = _mm_xor_ps(x3r_x3i_x2i_x2r, PM128(PCS_NNNR));
			t = _mm_sub_ps(x1x0, m);
			_mm_storel_pi((__m64*)(a + j ), t);
			_mm_storeh_pi((__m64*)(a + j1), t);
/*
			a[j		] = x0r + x2r;
			a[j	 + 1] = x0i - x2i;
			a[j1	] = x1r - x3i;
			a[j1 + 1] = x1i - x3r;
*/
			m = _mm_xor_ps(x3r_x3i_x2i_x2r, PM128(PCS_NNNR));
			t = _mm_add_ps(x1x0, m);
			_mm_storel_pi((__m64*)(a + j2), t);
			_mm_storeh_pi((__m64*)(a + j3), t);
/*
			a[j2	] = x0r - x2r;
			a[j2 + 1] = x0i + x2i;
			a[j3	] = x1r + x3i;
			a[j3 + 1] = x1i + x3r;
*/
		}
	} else {
		for (j = 0; j < l; j += 2) {
			j1 = j + l;
		/* ... sse code does not work here ... */
		/* might it be faster using fpu code ? */
/*
			__m128 a0;
			__m128 a1;
			__m128 x0i_x0r;
			a0 = _mm_loadl_pi(a0, (__m64*)(a + j  ));
			a1 = _mm_loadl_pi(a1, (__m64*)(a + j1 ));
			a0 = _mm_xor_ps(a0 , PM128(PCS_NNRN));
			a1 = _mm_xor_ps(a1 , PM128(PCS_NNRN));

			x0i_x0r = _mm_sub_ps(a0, a1);
*/

			x0r =  a[j	   ] - (+ a[j1	  ]);
			x0i = -a[j	+ 1] - (- a[j1 + 1]);
/*
			_mm_storel_pi((__m64*)(a + j ), _mm_add_ps(a0, a1));
*/
			a[j		] =	 a[j	 ] + a[j1	 ];
			a[j	 + 1] = -a[j  + 1] - a[j1 + 1];
/*
			_mm_storel_pi((__m64*)(a + j1), x0i_x0r);
*/
			a[j1	] = x0r;
			a[j1 + 1] = x0i;
		}
	}

}
#ifdef	_MSC_VER
#pragma warning(pop)
#endif
static void rftbsub(int n, float * __restrict a, int nc, float * __restrict c)
{
	int j, k, kk, ks, m;
	float wkr, wki, xr, xi, yr, yi;

	a[1] = -a[1];
	m = n >> 1;
	ks = 2 * nc / m;
	kk = 0;
	for (j = 2; j < m; j += 2) {
		k = n - j;
		kk += ks;
		wkr = 0.5f - c[nc - kk];
		wki = c[kk];

		xr = a[j	] - a[k	   ];
		xi = a[j + 1] + a[k + 1];

		yr = wkr * xr + wki * xi;
		yi = wkr * xi - wki * xr;

		a[j	   ]  = -yr + a[j	 ];
		a[j + 1]  =	 yi - a[j + 1];
		a[k	   ]  =	 yr + a[k	 ];
		a[k + 1]  =	 yi - a[k + 1];

	}
	a[m + 1] = -a[m + 1];
}


#include <math.h>

static void makewt(int nw, int * __restrict ip, float * __restrict w)
{
	void bitrv2(int n, int * __restrict ip, float * __restrict a);
	int j, nwh;
	float delta, x, y;

	ip[0] = nw;
	ip[1] = 1;
	if (nw > 2) {
		nwh = nw >> 1;
		delta = (float)(atan(1.0) / nwh);
		w[0] = 1;
		w[1] = 0;
		w[nwh] = cos(delta * nwh);
		w[nwh + 1] = w[nwh];
		if (nwh > 2) {
			for (j = 2; j < nwh; j += 2) {
				x = cos(delta * j);
				y = sin(delta * j);
				w[j] = x;
				w[j + 1] = y;
				w[nw - j] = y;
				w[nw - j + 1] = x;
			}
			bitrv2(nw, ip + 2, w);
		}
	}
}


static void makect(int nc, int *ip, float *c)
{
	int j, nch;
	float delta;

	ip[1] = nc;
	if (nc > 1) {
		nch = nc >> 1;
		delta = (float)(atan(1.0) / nch);
		c[0] = (float)(cos(delta * nch));
		c[nch] = 0.5f * c[0];
		for (j = 1; j < nch; j++) {
			c[j] = (float)(0.5 * cos(delta * j));
			c[nc - j] = (float)(0.5 * sin(delta * j));
		}
	}
}


void rdft_sse(int n, int isgn, float * __restrict a, int * __restrict ip, float * __restrict w)
{
	int nw, nc;
	float xi;

	nw = ip[0];
	if (n > (nw << 2)) {
		nw = n >> 2;
		makewt(nw, ip, w);
	}
	nc = ip[1];
	if (n > (nc << 2)) {
		nc = n >> 2;
		makect(nc, ip, w + nw);
	}
	if (isgn >= 0) {
		if (n > 4) {
			bitrv2(n, ip + 2, a);
			cftfsub(n, a, w);
			rftfsub(n, a, nc, w + nw);
		} else if (n == 4) {
			cftfsub(n, a, w);
		}
		xi = a[0] - a[1];
		a[0] += a[1];
		a[1] = xi;
	} else {
		a[1] = 0.5f * (a[0] - a[1]);
		a[0] -= a[1];
		if (n > 4) {
			rftbsub(n, a, nc, w + nw);
			bitrv2(n, ip + 2, a);
			cftbsub(n, a, w);
		} else if (n == 4) {
			cftfsub(n, a, w);
		}
	}
}


//---------------------------------------------------------------------------
#endif
