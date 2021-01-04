
#include "tjsTypes.h"
#include "tvpgl.h"
#include "tvpgl_ia32_intf.h"
#include "simd_def_x86x64.h"
#include "x86simdutil.h"

extern "C" {
extern tjs_uint16 TVPRecipTable256_16[256]; /* 1/x  table  ( 65536 ) multiplied,
	but limitted to 32767 (signed 16bits) */
}

struct sse2_adjust_gamma_a_func {
	const __m128i zero_;
	const tTVPGLGammaAdjustTempData *param_;
	inline sse2_adjust_gamma_a_func(tTVPGLGammaAdjustTempData *p) : zero_(_mm_setzero_si128()), param_(p) {}
	//inline tjs_uint32 operator()( tjs_uint32 dst ) const {
	inline tjs_uint32 operator()( tjs_uint32 dst ) const {
		if( dst >= 0xff000000 ) {
			// completely opaque pixel
			tjs_uint32 ret = param_->B[dst&0xff];		// look table B up  (BB')
			ret |= param_->G[(dst>>8)&0xff] << 8;		// look table G up  (GG')
			ret |= param_->R[(dst>>16)&0xff] << 16;	// look table R up  (RR')
			return 0xff000000|ret;
		} else if( dst != 0 ) {	// premul なので完全透明の時は0になるはず //} else if( dst > 0x00ffffff ) {
			__m128i md = _mm_cvtsi32_si128( dst );
			tjs_uint32 alpha = dst >> 24;
			tjs_uint32 recip = TVPRecipTable256_16[alpha];	// 65536/opacity (rcp)

			md = _mm_unpacklo_epi8( md, zero_ );	// 00 AA 00 RR 00 GG 00 BB
			__m128i maa = _mm_cvtsi32_si128( recip );	// alpha adjust
			__m128i ma = _mm_cvtsi32_si128( alpha );
			maa = _mm_unpacklo_epi16( maa, maa );	// 00 00 rcp rcp
			ma = _mm_unpacklo_epi16( ma, ma );		// 00 00 00 00 00 AA 00 AA
			maa = _mm_unpacklo_epi16( maa, maa );	// rcp rcp rcp rcp
			ma = _mm_unpacklo_epi16( ma, ma );		// 00 AA 00 AA 00 AA 00 AA

			//__m128i msa = _mm_cvtsi32_si128( dst & 0xff000000 );	// extract alpha
			tjs_uint32 sa = dst & 0xff000000;	// extract alpha

			// index = rcp * color >> 8
			maa = _mm_mullo_epi16( maa, md );	// alpha_adj *= dst
			maa = _mm_srli_epi16( maa, 8 );		// alpha_adj = rcp*AA>>8 rcp*RR>>8 rcp*GG>>8 rcp*BB>>8

			// if( dst > alpha ) index = 255
			__m128i md2 = md;
			md2 = _mm_cmpgt_epi16( md2, ma );	// dst > alpha ? 0xffff : 0
			md2 = _mm_srli_epi16( md2, 8 );		// mask >>= 8
			maa = _mm_or_si128( maa, md2 );		// alpha_adj |= mask

			maa = _mm_packus_epi16( maa, zero_ );	// alpha_adj = 00 00 00 00 rcp*AA>>8 rcp*RR>>8 rcp*GG>>8 rcp*BB>>8
			tjs_uint32 idx = _mm_cvtsi128_si32( maa );

			md = _mm_subs_epu8( md, ma );		// dst -= alpha == 0000 RR-AA GG-AA BB-AA (unsigned lower saturated)

			tjs_uint32 tbl = param_->B[idx&0xff];		// look table B up  (BB')
			tbl |= param_->G[(idx>>8)&0xff] << 8;		// look table G up  (GG')
			tbl |= param_->R[(idx>>16)&0xff] << 16;	// look table R up  (RR')

			__m128i adjust = ma;
			adjust = _mm_srli_epi16( adjust, 7 );
			ma = _mm_add_epi16( ma, adjust );	// adjust alpha

			__m128i mdd = _mm_cvtsi32_si128( tbl );	// 00 00 00 00 00 RR' GG' BB'
			mdd = _mm_unpacklo_epi8( mdd, zero_ );	// 00 00 00 RR' 00 GG' 00 BB'
			mdd = _mm_mullo_epi16( mdd, ma );		// color' * alpha
			mdd = _mm_srli_epi16( mdd, 8 );			// 0000 alpha*RR'>>8 alpha*GG'>>8 alpha*BB'>>8
			mdd = _mm_add_epi16( mdd, md );
			mdd = _mm_packus_epi16( mdd, zero_ );
			//mdd = _mm_or_si128( mdd, msa );
			//return _mm_cvtsi128_si32( mdd );
			return _mm_cvtsi128_si32( mdd ) | sa;
		} else {
			return dst;
		}
	}
};

#define TVP_USE_RCP_TABLE
void TVPAdjustGamma_a_sse2_c(tjs_uint32 *dest, tjs_int len, tTVPGLGammaAdjustTempData *param) {
	if( len <= 0 ) return;

	sse2_adjust_gamma_a_func func(param);

	// アライメント処理
	tjs_int count = (tjs_int)((unsigned)dest & 0xF);
	if( count ) {
		count = (16 - count)>>2;
		count = count > len ? len : count;
		tjs_uint32* limit = dest + count;
		while( dest < limit ) {
			*dest = func( *dest );
			dest++;
		}
		len -= count;
	}
	tjs_uint32 rem = (len>>2)<<2;
	tjs_uint32* limit = dest + rem;

	const __m128i alphamask = _mm_set1_epi32(0xff000000);
	const __m128i zero = func.zero_;//_mm_setzero_si128();
	while( dest < limit ) {
		__m128i md0 = _mm_load_si128( (__m128i const*)dest );
		__m128i ma0 = md0;
		ma0 = _mm_and_si128( ma0, alphamask );
		__m128i cmp = ma0;
		cmp = _mm_cmpeq_epi32( cmp, alphamask );	// alpha == 0xff
		int opaque = _mm_movemask_epi8(cmp);
		if( opaque == 0xffff ) {
			// 4pixel完全不透明
			tjs_uint32 dst = dest[0];
			tjs_uint32 ret = param->B[dst&0xff];		// look table B up  (BB')
			ret |= param->G[(dst>>8)&0xff] << 8;		// look table G up  (GG')
			ret |= param->R[(dst>>16)&0xff] << 16;		// look table R up  (RR')
			dest[0] = 0xff000000|ret;
			dst = dest[1];
			ret = param->B[dst&0xff];					// look table B up  (BB')
			ret |= param->G[(dst>>8)&0xff] << 8;		// look table G up  (GG')
			ret |= param->R[(dst>>16)&0xff] << 16;		// look table R up  (RR')
			dest[1] = 0xff000000|ret;
			dst = dest[2];
			ret = param->B[dst&0xff];					// look table B up  (BB')
			ret |= param->G[(dst>>8)&0xff] << 8;		// look table G up  (GG')
			ret |= param->R[(dst>>16)&0xff] << 16;		// look table R up  (RR')
			dest[2] = 0xff000000|ret;
			dst = dest[3];
			ret = param->B[dst&0xff];					// look table B up  (BB')
			ret |= param->G[(dst>>8)&0xff] << 8;		// look table G up  (GG')
			ret |= param->R[(dst>>16)&0xff] << 16;		// look table R up  (RR')
			dest[3] = 0xff000000|ret;
		} else {
			cmp = ma0;
			cmp = _mm_cmpeq_epi32( cmp, zero );	// alpha == 0
			int transparent = _mm_movemask_epi8(cmp);
			if( transparent == 0 && opaque == 0 ) {
				// 4pixelの中に完全不透明/完全透明なものはない
				__m128i msa = ma0;
				ma0 = _mm_srli_epi32( ma0, 24 );	// ma >>= 24

#ifndef TVP_USE_RCP_TABLE
				// 以下、TVPRecipTable256_16 を引くのと同じ。速度的にはわずかに速いが、少し精度が落ちてしまう、なので基本テーブルを使う方にしておく
				__m128 mrcp = _mm_cvtepi32_ps(ma0);
				mrcp = _mm_rcp_ps(mrcp);			// 1 / rcp
				//mrcp = m128_rcp_22bit_ps(mrcp);	// 1 / rcp : 精度上げると少し遅い
				const __m128 val65536 = _mm_set1_ps(65536.0f);
				mrcp = _mm_mul_ps( mrcp, val65536 );	// 65536 / rcp
				__m128i mrcpi0 = _mm_cvtps_epi32(mrcp);

				mrcpi0 = _mm_packs_epi32( mrcpi0, mrcpi0 );	// 0 1 2 3 0 1 2 3 0x7fffでsaturate
				__m128i mask;
#if 0	// 以下の方法でsaturateすると遅いと言うか、_mm_packs_epi32で大幅に省けると気付いた
				const __m128i mmax = _mm_set1_epi32(0x7fff);
				__m128i mask = mrcpi0;
				mask = _mm_cmplt_epi32( mask, mmax );	// rcp < 0x7fff ? 0xffff : 0000
				mrcpi0 = _mm_and_si128( mrcpi0, mask );	// rcp < 0x7fff ? rcp : 0
				mask = _mm_andnot_si128( mask, mmax );	// rcp >= 0x7fff ? 0x7fff : 0
				mrcpi0 = _mm_or_si128( mrcpi0, mask );	// rcp < 0x7fff ? rcp : 0x7fff
				mrcpi0 = _mm_packs_epi32( mrcpi0, mrcpi0 );		// 0 1 2 3 0 1 2 3
#endif

#else
				__m128i mrcpi0 = _mm_set_epi32(TVPRecipTable256_16[ma0.m128i_u32[3]],TVPRecipTable256_16[ma0.m128i_u32[2]],
						TVPRecipTable256_16[ma0.m128i_u32[1]],TVPRecipTable256_16[ma0.m128i_u32[0]]);
				__m128i mask;
				mrcpi0 = _mm_packs_epi32( mrcpi0, mrcpi0 );		// 0 1 2 3 0 1 2 3
#endif

				// rcpとalphaのunpack
				mrcpi0 = _mm_unpacklo_epi16( mrcpi0, mrcpi0 );	// 0 0 1 1 2 2 3 3
				__m128i mrcpi1 = mrcpi0;

				ma0 = _mm_packs_epi32( ma0, ma0 );		// 0 1 2 3 0 1 2 3
				ma0 = _mm_unpacklo_epi16( ma0, ma0 );	// 0 0 1 1 2 2 3 3
				__m128i ma1 = ma0;
				__m128i md1 = md0;

				mrcpi0 = _mm_unpacklo_epi16( mrcpi0, mrcpi0 );	// 0 0 0 0 1 1 1 1
				ma0 = _mm_unpacklo_epi16( ma0, ma0 );	// 0 0 0 0 1 1 1 1
				md0 = _mm_unpacklo_epi8( md0, zero );	// 00 AA 00 RR 00 GG 00 BB

				// index = rcp * color >> 8
				mrcpi0 = _mm_mullo_epi16( mrcpi0, md0 );// rcp *= dst
				mrcpi0 = _mm_srli_epi16( mrcpi0, 8 );	// rcp*AA>>8 rcp*RR>>8 rcp*GG>>8 rcp*BB>>8

				// if( dst > alpha ) index = 255
				mask = md0;
				mask = _mm_cmpgt_epi16( mask, ma0 );		// dst > alpha ? 0xffff : 0
				mask = _mm_srli_epi16( mask, 8 );		// mask >>= 8
				mrcpi0 = _mm_or_si128( mrcpi0, mask );	// alpha_adj |= mask

				md0 = _mm_subs_epu8( md0, ma0 );		// dst -= alpha == 0000 RR-AA GG-AA BB-AA (unsigned lower saturated)
				__m128i madj = ma0;
				madj = _mm_srli_epi16( madj, 7 );
				ma0 = _mm_add_epi16( ma0, madj );	// adjust alpha = alpha + (alpha>>7)
				// md0 と ma0 はテーブル参照後また使われる

				// higher pixel
				mrcpi1 = _mm_unpackhi_epi16( mrcpi1, mrcpi1 );	// 2 2 2 2 3 3 3 3
				ma1 = _mm_unpackhi_epi16( ma1, ma1 );	// 2 2 2 2 3 3 3 3
				md1 = _mm_unpackhi_epi8( md1, zero );	// 00 AA 00 RR 00 GG 00 BB
				// index = rcp * color >> 8
				mrcpi1 = _mm_mullo_epi16( mrcpi1, md1 );	// rcp *= dst
				mrcpi1 = _mm_srli_epi16( mrcpi1, 8 );	// rcp*AA>>8 rcp*RR>>8 rcp*GG>>8 rcp*BB>>8
				// if( dst > alpha ) index = 255
				mask = md1;
				mask = _mm_cmpgt_epi16( mask, ma1 );		// dst > alpha ? 0xffff : 0
				mask = _mm_srli_epi16( mask, 8 );		// mask >>= 8
				mrcpi1 = _mm_or_si128( mrcpi1, mask );	// alpha_adj |= mask

				md1 = _mm_subs_epu8( md1, ma1 );		// dst -= alpha == 0000 RR-AA GG-AA BB-AA (unsigned lower saturated)
				madj = ma1;
				madj = _mm_srli_epi16( madj, 7 );
				ma1 = _mm_add_epi16( ma1, madj );	// adjust alpha = alpha + (alpha>>7)
				// md1 と ma1 はテーブル参照後また使われる

				mrcpi0 = _mm_packus_epi16( mrcpi0, mrcpi1 );	// rcp*AA>>8 rcp*RR>>8 rcp*GG>>8 rcp*BB>>8

				// 4pixel分のテーブル参照
				tjs_uint32 idx = _mm_cvtsi128_si32( mrcpi0 );
				tjs_uint32 col = param->B[idx&0xff];	// look table B up  (BB')
				col |= param->G[(idx>>8)&0xff] << 8;	// look table G up  (GG')
				col |= param->R[(idx>>16)&0xff] << 16;	// look table R up  (RR')
				__m128i mcol0 = _mm_cvtsi32_si128( col );
				mrcpi0 = _mm_shuffle_epi32( mrcpi0, _MM_SHUFFLE( 0, 3, 2, 1 ) );
				idx = _mm_cvtsi128_si32( mrcpi0 );
				col = param->B[idx&0xff];				// look table B up  (BB')
				col |= param->G[(idx>>8)&0xff] << 8;	// look table G up  (GG')
				col |= param->R[(idx>>16)&0xff] << 16;	// look table R up  (RR')
				__m128i mcol2 = _mm_cvtsi32_si128( col );
				mcol0 = _mm_unpacklo_epi32( mcol0, mcol2 );	// 00 00 00 00 00 00 00 00 00 RR' GG' BB' 00 RR' GG' BB'
				mcol0 = _mm_unpacklo_epi8( mcol0, zero );	// 00 00 00 RR' 00 GG' 00 BB'
				mcol0 = _mm_mullo_epi16( mcol0, ma0 );		// color' * alpha
				mcol0 = _mm_srli_epi16( mcol0, 8 );			// 0000 alpha*RR'>>8 alpha*GG'>>8 alpha*BB'>>8
				mcol0 = _mm_add_epi16( mcol0, md0 );		// color' + dst

				mrcpi0 = _mm_shuffle_epi32( mrcpi0, _MM_SHUFFLE( 0, 3, 2, 1 ) );
				idx = _mm_cvtsi128_si32( mrcpi0 );
				col = param->B[idx&0xff];				// look table B up  (BB')
				col |= param->G[(idx>>8)&0xff] << 8;	// look table G up  (GG')
				col |= param->R[(idx>>16)&0xff] << 16;	// look table R up  (RR')
				__m128i mcol1 = _mm_cvtsi32_si128( col );
				mrcpi0 = _mm_shuffle_epi32( mrcpi0, _MM_SHUFFLE( 0, 3, 2, 1 ) );
				idx = _mm_cvtsi128_si32( mrcpi0 );
				col = param->B[idx&0xff];				// look table B up  (BB')
				col |= param->G[(idx>>8)&0xff] << 8;	// look table G up  (GG')
				col |= param->R[(idx>>16)&0xff] << 16;	// look table R up  (RR')
				mcol2 = _mm_cvtsi32_si128( col );
				mcol1 = _mm_unpacklo_epi32( mcol1, mcol2 );	// 00 00 00 00 00 00 00 00 00 RR' GG' BB' 00 RR' GG' BB'
				mcol1 = _mm_unpacklo_epi8( mcol1, zero );	// 00 00 00 RR' 00 GG' 00 BB'
				mcol1 = _mm_mullo_epi16( mcol1, ma1 );		// color' * alpha
				mcol1 = _mm_srli_epi16( mcol1, 8 );			// 0000 alpha*RR'>>8 alpha*GG'>>8 alpha*BB'>>8
				mcol1 = _mm_add_epi16( mcol1, md1 );		// color' + dst

				mcol0 = _mm_packus_epi16( mcol0, mcol1 );
				mcol0 = _mm_or_si128( mcol0, msa );		// | src alpha
				_mm_store_si128( (__m128i*)dest, mcol0 );
			} else if( transparent != 0xffff ) {
				// 混在している場合は下手に分岐するよりも素直に1pixel処理を4回回した方が良さそうな
				dest[0] = func( dest[0] );
				dest[1] = func( dest[1] );
				dest[2] = func( dest[2] );
				dest[3] = func( dest[3] );
			}	// else 4pixel 完全透明の時は何もしない
		}
		dest += 4;
	}

	// 残りの端数処理
	limit += (len-rem);
	while( dest < limit ) {
		*dest = func( *dest );
		dest++;
	}
}

// SSE & SSE2
#if 0
void TVPInitGammaAdjustTempData_sse2_c( tTVPGLGammaAdjustTempData *temp, const tTVPGLGammaAdjustData *data )
{
	/* make table */
	const float ramp = data->RCeil - data->RFloor;
	const float gamp = data->GCeil - data->GFloor;
	const float bamp = data->BCeil - data->BFloor;
	const float rgamma = 1.0f/data->RGamma; /* we assume data.?Gamma is a non-zero value here */
	const float ggamma = 1.0f/data->GGamma;
	const float bgamma = 1.0f/data->BGamma;
	const __m128 amp = _mm_set_ps( 0.0f, bamp, gamp, ramp );
	const __m128 gamma = _mm_set_ps( 0.0f, bgamma, ggamma, rgamma );
	const __m128 floor = _mm_set_ps( 0.0f, data->BFloor, data->GFloor, data->RFloor );
	const __m128 half = _mm_set1_ps(0.5f);
	const __m128 one = _mm_set1_ps(1.0f);
	const __m128 mul = _mm_set1_ps(1.0f/255.0f);
	__m128 inc = _mm_setzero_ps();
	for( int i = 0; i < 256; i++ ) {
		__m128 mrate = inc;
		mrate = _mm_mul_ps( mrate, mul );	// i / 255.0f
		inc = _mm_add_ps( inc, one );
		mrate = pow_ps( mrate, gamma );
		mrate = _mm_mul_ps( mrate, amp );
		mrate = _mm_add_ps( mrate, half );
		mrate = _mm_add_ps( mrate, floor );

		__m128i n = _mm_cvttps_epi32(mrate);
		n = _mm_packs_epi32( n, n );
		n = _mm_packus_epi16( n, n );
		tjs_uint32 col = _mm_cvtsi128_si32(n);
		temp->R[i]= col&0xff; col>>=8;
		temp->G[i]= col&0xff; col>>=8;
		temp->B[i]= col&0xff;
	}
}

void TVPInitGammaAdjustTempData_sse2_c( tTVPGLGammaAdjustTempData *temp, const tTVPGLGammaAdjustData *data )
{
	/* make table */
	const float ramp = data->RCeil - data->RFloor;
	const float gamp = data->GCeil - data->GFloor;
	const float bamp = data->BCeil - data->BFloor;
	const float rgamma = 1.0f/data->RGamma; /* we assume data.?Gamma is a non-zero value here */
	const float ggamma = 1.0f/data->GGamma;
	const float bgamma = 1.0f/data->BGamma;
	const __m128 amp = _mm_set_ps( 0.0f, bamp, gamp, ramp );
	const __m128 gamma = _mm_set_ps( 0.0f, bgamma, ggamma, rgamma );
	const __m128 floor = _mm_set_ps( 0.0f, data->BFloor, data->GFloor, data->RFloor );
	const __m128 half = _mm_set1_ps(0.5f);
	for( int i = 0; i < 256; i++ ) {
		float rate = (float)i/255.0f;
		__m128 mrate = _mm_set1_ps(rate);
		mrate = pow_ps( mrate, gamma );
		mrate = _mm_mul_ps( mrate, amp );
		mrate = _mm_add_ps( mrate, half );
		mrate = _mm_add_ps( mrate, floor );

		__m128i n = _mm_cvttps_epi32(mrate);
		n = _mm_packs_epi32( n, n );
		n = _mm_packus_epi16( n, n );
		tjs_uint32 col = _mm_cvtsi128_si32(n);
		temp->R[i]= col&0xff; col>>=8;
		temp->G[i]= col&0xff; col>>=8;
		temp->B[i]= col&0xff;
	}
}
#endif
// C版と比較して上の2つは3倍程度、下のものは4倍程度と同時演算数に従って速くなっている
// 定数が多いのでレジスタ使いまわしなどで不利かと思ったが気にするほどではなさそう。
void TVPInitGammaAdjustTempData_sse2_c( tTVPGLGammaAdjustTempData *temp, const tTVPGLGammaAdjustData *data )
{
	const __m128 ramp = _mm_set1_ps((float)(data->RCeil - data->RFloor));
	const __m128 gamp = _mm_set1_ps((float)(data->GCeil - data->GFloor));
	const __m128 bamp = _mm_set1_ps((float)(data->BCeil - data->BFloor));
	const __m128 rgamma = _mm_set1_ps(1.0f/data->RGamma);
	const __m128 ggamma = _mm_set1_ps(1.0f/data->GGamma);
	const __m128 bgamma = _mm_set1_ps(1.0f/data->BGamma);
	const __m128 rfloor = _mm_set1_ps((float)data->RFloor);
	const __m128 gfloor = _mm_set1_ps((float)data->GFloor);
	const __m128 bfloor = _mm_set1_ps((float)data->BFloor);
	const __m128 half = _mm_set1_ps(0.5f);
	const __m128 four = _mm_set1_ps(4.0f);
	const __m128 mul = _mm_set1_ps(1.0f/255.0f);
	__m128 inc = _mm_set_ps( 3.0f, 2.0f, 1.0f, 0.0f );
	tjs_uint32* dstr = (tjs_uint32*)temp->R;
	tjs_uint32* dstg = (tjs_uint32*)temp->G;
	tjs_uint32* dstb = (tjs_uint32*)temp->B;
	for( int i = 0; i < 256; i += 4 ) {
		__m128 mrate = inc;
		mrate = _mm_mul_ps( mrate, mul );	// i / 255.0f
		inc = _mm_add_ps( inc, four );
		// red
		__m128 rate = mrate;
		rate = pow_ps( rate, rgamma );		// pow(rate,gamma)
		rate = _mm_mul_ps( rate, ramp );	// pow(rate,gamma)*amp
		rate = _mm_add_ps( rate, half );	// pow(rate,gamma)*amp+0.5
		rate = _mm_add_ps( rate, rfloor );	// pow(rate,gamma)*amp+0.5+floor
		__m128i n = _mm_cvttps_epi32(rate);	// float to int
		n = _mm_packs_epi32( n, n );		// saturate to 16bit
		n = _mm_packus_epi16( n, n );		// saturate to 8bit
		*dstr = _mm_cvtsi128_si32(n);		// to int
		dstr++;
		// green
		rate = mrate;
		rate = pow_ps( rate, ggamma );		// pow(rate,gamma)
		rate = _mm_mul_ps( rate, gamp );	// pow(rate,gamma)*amp
		rate = _mm_add_ps( rate, half );	// pow(rate,gamma)*amp+0.5
		rate = _mm_add_ps( rate, gfloor );	// pow(rate,gamma)*amp+0.5+floor
		n = _mm_cvttps_epi32(rate);
		n = _mm_packs_epi32( n, n );
		n = _mm_packus_epi16( n, n );
		*dstg = _mm_cvtsi128_si32(n);
		dstg++;
		// blue
		rate = mrate;
		rate = pow_ps( rate, bgamma );		// pow(rate,gamma)
		rate = _mm_mul_ps( rate, bamp );	// pow(rate,gamma)*amp
		rate = _mm_add_ps( rate, half );	// pow(rate,gamma)*amp+0.5
		rate = _mm_add_ps( rate, bfloor );	// pow(rate,gamma)*amp+0.5+floor
		n = _mm_cvttps_epi32(rate);
		n = _mm_packs_epi32( n, n );
		n = _mm_packus_epi16( n, n );
		*dstb = _mm_cvtsi128_si32(n);
		dstb++;
	}
}


