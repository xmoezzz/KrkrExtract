#ifndef __INTERPOLATION_FUNCTOR_SSE2_H__
#define __INTERPOLATION_FUNCTOR_SSE2_H__


struct sse2_nearest_functor {
	inline __m128i operator()( const tjs_uint8* s, __m128i mxyi, __m128i mpitch, tjs_int srcpitch ) const {
		mxyi = _mm_srai_epi32( mxyi, 16 );	// 固定小数点から整数へ
		__m128i my = mxyi;
		my = _mm_mul_epu32( my, mpitch );	// sy * srcpitch
		int sy1 = _mm_cvtsi128_si32( my );
		my = _mm_srli_si128( my, 8 );
		int sy2 = _mm_cvtsi128_si32( my );
		mxyi = _mm_srli_epi64( mxyi, 32 );	// 0000 ++sx 0000 sx
		int sx1 = _mm_cvtsi128_si32( mxyi );
		mxyi = _mm_srli_si128( mxyi, 8 );
		int sx2 = _mm_cvtsi128_si32( mxyi );
		__m128i s0 = _mm_cvtsi32_si128( *(const tjs_uint32*)(s + sy1 + sx1*4 ) );	// sy*srcpitch + sx*4
		__m128i s1 = _mm_cvtsi32_si128( *(const tjs_uint32*)(s + sy2 + sx2*4 ) );	// sy*srcpitch + sx*4
		return _mm_unpacklo_epi32( s0, s1 );
	}
	inline tjs_uint32 operator()( const tjs_uint8* s, tjs_int sy, tjs_int sx, tjs_int srcpitch ) const {
		return *(const tjs_uint32*)( s + (sy>>16)*srcpitch + (sx>>16)*4 );
	}
	inline __m128i pack( __m128i s01, __m128i s23 ) const {
		return _mm_unpacklo_epi64( s01, s23 );
	}
};

struct sse2_bilinear_functor {
	const __m128i zero_;
	inline sse2_bilinear_functor() : zero_(_mm_setzero_si128()) {}
	inline __m128i operator()( const tjs_uint8* s, __m128i mxyi, __m128i mpitch, tjs_int srcpitch ) const {
		__m128i mratio = mxyi;
		// 座標計算
		mxyi = _mm_srai_epi32( mxyi, 16 );	// 固定小数点から整数へ
		__m128i my = mxyi;
		my = _mm_mul_epu32( my, mpitch );	// sy * srcpitch
		int sy1 = _mm_cvtsi128_si32( my );
		my = _mm_srli_si128( my, 8 );
		int sy2 = _mm_cvtsi128_si32( my );
		mxyi = _mm_srli_epi64( mxyi, 32 );	// 0000 ++sx 0000 sx
		mxyi = _mm_add_epi32( mxyi, mxyi );	// 0000 ++sx*2 0000 sx*2
		mxyi = _mm_add_epi32( mxyi, mxyi );	// 0000 ++sx*4 0000 sx*4
		int sx1 = _mm_cvtsi128_si32( mxyi );
		mxyi = _mm_srli_si128( mxyi, 8 );
		int sx2 = _mm_cvtsi128_si32( mxyi );
		const tjs_uint8* px1 = s + sy1 + sx1;
		const tjs_uint8* px2 = s + sy2 + sx2;

		// 読み込み
		__m128i mp1 = _mm_cvtsi32_si128( *((const tjs_uint32*)px1) );
		__m128i mp = _mm_cvtsi32_si128( *((const tjs_uint32*)px2) );
		mp1 = _mm_unpacklo_epi32( mp1, mp );
		mp1 = _mm_unpacklo_epi8( mp1, zero_ );
		__m128i mp2 = _mm_cvtsi32_si128( *((const tjs_uint32*)(px1+4)) );
		mp = _mm_cvtsi32_si128( *((const tjs_uint32*)(px2+4)) );
		mp2 = _mm_unpacklo_epi32( mp2, mp );
		mp2 = _mm_unpacklo_epi8( mp2, zero_ );
		__m128i mp3 = _mm_cvtsi32_si128( *((const tjs_uint32*)(px1+srcpitch)) );
		mp = _mm_cvtsi32_si128( *((const tjs_uint32*)(px2+srcpitch)) );
		mp3 = _mm_unpacklo_epi32( mp3, mp );
		mp3 = _mm_unpacklo_epi8( mp3, zero_ );
		__m128i mp4 = _mm_cvtsi32_si128( *((const tjs_uint32*)(px1+srcpitch+4)) );
		mp = _mm_cvtsi32_si128( *((const tjs_uint32*)(px2+srcpitch+4)) );
		mp4 = _mm_unpacklo_epi32( mp4, mp );
		mp4 = _mm_unpacklo_epi8( mp4, zero_ );

		// mratio sx sy sx sy
		// 16bit shuffle で上位 16bit を消す
		mratio = _mm_shufflelo_epi16( mratio, _MM_SHUFFLE( 2, 2, 0, 0 )  );	// sx sy | sx1 sx1 sy1 sy1 = & 0x0000ffffに近い効果
		mratio = _mm_shufflehi_epi16( mratio, _MM_SHUFFLE( 2, 2, 0, 0 )  );	// sx2 sx2 sy2 sy2 sx1 sx1 sy1 sy1
		mratio = _mm_srli_epi16( mratio, 8 );	// ratio >> 8
		__m128i sx_ratio = mratio;
		sx_ratio = _mm_srli_epi16( sx_ratio, 7 );	// ratio >> 7
		sx_ratio = _mm_adds_epu8( sx_ratio, mratio );
		__m128i sy_ratio = sx_ratio;
		sx_ratio = _mm_shuffle_epi32(sx_ratio, _MM_SHUFFLE( 3, 3, 1, 1 )  );	// sx2 sx2 sx2 sx2 sx1 sx1 sx1 sx1
		sy_ratio = _mm_shuffle_epi32(sy_ratio, _MM_SHUFFLE( 2, 2, 0, 0 )  );	// sy2 sy2 sy2 sy2 sy1 sy1 sy1 sy1

		// 上段をsx_ratioでブレンド
		mp2 = _mm_sub_epi16( mp2, mp1 );	// s0: mm2 = s0p1 - s0p0
		mp2 = _mm_mullo_epi16( mp2, sx_ratio );	// s0:
		mp2 = _mm_srli_epi16( mp2, 8 );		// s0: mm2 = (s0p1 - s0p0) * bx
		mp1 = _mm_add_epi8( mp1, mp2 );		// s0: mm1 = s0p0 + (s0p1 - s0p0) * bx = S0
		// 下段をsx_ratioでブレンド
		mp4 = _mm_sub_epi16( mp4, mp3 );	// s1: mm4 = s1p1 - s1p0
		mp4 = _mm_mullo_epi16( mp4, sx_ratio );	// s1:
		mp4 = _mm_srli_epi16( mp4, 8 );		// s1: mm4 = (s1p1 - s1p0) * bx
		mp3 = _mm_add_epi8( mp3, mp4 );		// s1: mm3 = s1p0 + (s1p1 - s1p0) * bx = S1

		// 上下をsy_ratioでブレンド
		mp3 = _mm_sub_epi16( mp3, mp1 );	// s0/s1: mm3 = S1 - S0
		mp3 = _mm_mullo_epi16( mp3, sy_ratio );	// s0/s1:
		mp3 = _mm_srli_epi16( mp3, 8 );		// s0/s1: mm3 = (S1 - S0) * by
		mp1 = _mm_add_epi8( mp1, mp3 );		// s0/s1: mm1 = S0 + (S1 - S0) * by = dst
		
		//mp1 = _mm_packus_epi16( mp1, zero_ );
		//_mm_storel_epi64((__m128i *)d, mp1);
		return mp1;	// packせずに16bit状態のまま返した方が後処理しやすいか？ でも、現行のブレンド処理はpack状態想定しているのでその辺りも対応必要
	}
	inline tjs_uint32 operator()( const tjs_uint8* s, tjs_int sy, tjs_int sx, tjs_int srcpitch ) const {
		const tjs_uint8* px = s + (sy>>16)*srcpitch + (sx>>16)*4;

		// 2pixel 同時に読むとレジスタ節約できるな
		__m128i mp1 = _mm_loadl_epi64( (__m128i const*)px );	// p1 | p2 : s0p0 s0p1
		__m128i mp2 = _mm_loadl_epi64( (__m128i const*)(px+srcpitch) );	// p3 | p4 : s1p0 s1p1
		mp1 = _mm_unpacklo_epi32( mp1, mp2 );	// 1 3 2 4
		mp2 = mp1;
		mp1 = _mm_shuffle_epi32( mp1, _MM_SHUFFLE( 1, 0, 1, 0 )  );	// 1 3 1 3
		mp2 = _mm_shuffle_epi32( mp2, _MM_SHUFFLE( 3, 2, 3, 2 )  );	// 2 4 2 4
		mp1 = _mm_unpacklo_epi8( mp1, zero_ );
		mp2 = _mm_unpacklo_epi8( mp2, zero_ );

		__m128i mtmp = _mm_cvtsi32_si128(sx);
		mtmp = _mm_shufflelo_epi16(mtmp, _MM_SHUFFLE( 0, 0, 0, 0 )  );	// 00000000 sx sx sx sx
		__m128i msxr = _mm_cvtsi32_si128(sy);
		msxr = _mm_shufflelo_epi16(msxr, _MM_SHUFFLE( 0, 0, 0, 0 )  );	// 00000000 sy sy sy sy
		msxr = _mm_unpacklo_epi64( msxr, mtmp );// sx sx sx sx sy sy sy sy
		msxr = _mm_srli_epi16( msxr, 8 );		// sx >> 8, sy >> 8
		mtmp = msxr;
		mtmp = _mm_srli_epi16( mtmp, 7 );	// sx >> 7, sy >> 7
		msxr = _mm_adds_epu8( msxr, mtmp );	// sx + (sx>>7), sy + (sy>>7)
		__m128i msyr = msxr;
		msxr = _mm_shuffle_epi32(msxr, _MM_SHUFFLE( 2, 2, 2, 2 )  );
		// msyr = sx sx sx sx sy sy sy sy, sy は 下位 64bit での演算なので並べ替え必要ない

		// 上下段を同時にsx_ratioでブレンド
		mp2 = _mm_sub_epi16( mp2, mp1 );	// s0: mm2 = s0p1 - s0p0
		mp2 = _mm_mullo_epi16( mp2, msxr );	// s0:
		mp2 = _mm_srli_epi16( mp2, 8 );		// s0: mm2 = (s0p1 - s0p0) * bx
		mp1 = _mm_add_epi8( mp1, mp2 );		// s0: mm1 = s0p0 + (s0p1 - s0p0) * bx = S0
		__m128i mp3 = mp1;
		mp3 = _mm_srli_si128( mp3, 8 );

		// 上下をsy_ratioでブレンド
		mp3 = _mm_sub_epi16( mp3, mp1 );	// s0/s1: mm3 = S1 - S0
		mp3 = _mm_mullo_epi16( mp3, msyr );	// s0/s1:
		mp3 = _mm_srli_epi16( mp3, 8 );		// s0/s1: mm3 = (S1 - S0) * by
		mp1 = _mm_add_epi8( mp1, mp3 );		// s0/s1: mm1 = S0 + (S1 - S0) * by = dst

		mp1 = _mm_packus_epi16( mp1, mp1 );
		return _mm_cvtsi128_si32( mp1 );
	}
	inline __m128i pack( __m128i s01, __m128i s23 ) const {
		return _mm_packus_epi16( s01, s23 );
	}
};

//--------------------------------------------------------------------
// memo
//          s0p0   s0p1
//          s1p0   s1p1
//
//	dst = (s0p0 * (1-bx) + s0p1 * bx ) * (1-by) +
//	      (s1p0 * (1-bx) + s1p1 * bx ) * by
//	dst = (s0p0 - s0p0 * bx + s0p1 * bx ) * (1-by) +
//	      (s1p0 - s1p0 * bx + s1p1 * bx ) * by
//	dst = (s0p0 + s0p1 * bx - s0p0 * bx ) * (1-by) +
//	      (s1p0 + s1p1 * bx - s1p0 * bx ) * by
//	dst = (s0p0 + (s0p1 - s0p0) * bx ) * (1-by) +
//	      (s1p0 + (s1p1 - s1p0) * bx ) * by
//	dst = S0 * (1-by) +
//	      S1 * by
//	dst = S0 - S0 * by + S1 * by
//	dst = S0 + (S1 - S0) * by
struct sse2_bilinear_fixy_functor {
	const __m128i zero_;
	const __m128i mby_;
	inline sse2_bilinear_fixy_functor( tjs_int blend_y ) : zero_(_mm_setzero_si128()),
		mby_(_mm_set1_epi16( (short)(blend_y+(blend_y>>7)) )) {}

	inline tjs_uint32 operator()( const tjs_uint32* src1, const tjs_uint32* src2, tjs_int srcstart ) const {
		tjs_int blend_x = (srcstart & 0xffff) >> 8;
		__m128i mbx = _mm_set1_epi16( (short)blend_x );

		// 2pixel 同時に読むとレジスタ節約できる
		tjs_int sp = srcstart >> 16;
		__m128i mp1 = _mm_loadl_epi64( (__m128i const*)(src1+sp) );	// p1 | p2 : s0p0 s0p1
		__m128i mp2 = _mm_loadl_epi64( (__m128i const*)(src2+sp) );	// p3 | p4 : s1p0 s1p1
		mp1 = _mm_unpacklo_epi32( mp1, mp2 );	// 1 3 2 4
		mp2 = mp1;
		mp1 = _mm_shuffle_epi32( mp1, _MM_SHUFFLE( 1, 0, 1, 0 )  );	// 1 3 1 3
		mp2 = _mm_shuffle_epi32( mp2, _MM_SHUFFLE( 3, 2, 3, 2 )  );	// 2 4 2 4
		mp1 = _mm_unpacklo_epi8( mp1, zero_ );
		mp2 = _mm_unpacklo_epi8( mp2, zero_ );

		// 上下段を同時にmbxでブレンド
		mp2 = _mm_sub_epi16( mp2, mp1 );	// s0: mm2 = s0p1 - s0p0
		mp2 = _mm_mullo_epi16( mp2, mbx );	// s0:
		mp2 = _mm_srli_epi16( mp2, 8 );		// s0: mm2 = (s0p1 - s0p0) * bx
		mp1 = _mm_add_epi8( mp1, mp2 );		// s0: mm1 = s0p0 + (s0p1 - s0p0) * bx = S0
		__m128i mp3 = mp1;
		mp3 = _mm_srli_si128( mp3, 8 );

		// 上下をmbyでブレンド
		mp3 = _mm_sub_epi16( mp3, mp1 );	// s0/s1: mm3 = S1 - S0
		mp3 = _mm_mullo_epi16( mp3, mby_ );	// s0/s1:
		mp3 = _mm_srli_epi16( mp3, 8 );		// s0/s1: mm3 = (S1 - S0) * by
		mp1 = _mm_add_epi8( mp1, mp3 );		// s0/s1: mm1 = S0 + (S1 - S0) * by = dst

		mp1 = _mm_packus_epi16( mp1, mp1 );
		return _mm_cvtsi128_si32( mp1 );
	}

	inline __m128i operator()( const tjs_uint32* src1, const tjs_uint32* src2, __m128i mstart ) const {
		__m128i offset = mstart;
		offset = _mm_srli_epi32( offset, 16 );	// >> 16
		tjs_int o0 = offset.m128i_i32[0];
		tjs_int o1 = offset.m128i_i32[1];
		tjs_int o2 = offset.m128i_i32[2];
		tjs_int o3 = offset.m128i_i32[3];

		__m128i mbx = mstart;
		mbx = _mm_shufflelo_epi16( mbx, _MM_SHUFFLE( 2, 2, 0, 0 )  );	// sx4 sx3 | sx2 sx2 sx1 sx1 = & 0x0000ffffに近い効果
		mbx = _mm_shufflehi_epi16( mbx, _MM_SHUFFLE( 2, 2, 0, 0 )  );	// sx4 sx4 sx3 sx3 sx2 sx2 sx1 sx1
		mbx = _mm_srli_epi16( mbx, 8 );
		__m128i mbx2 = mbx;
		mbx = _mm_unpacklo_epi32( mbx, mbx );	// sx2 sx2 sx2 sx2 sx1 sx1 sx1 sx1
		__m128i r0 = two( src1, src2, o0, o1, mbx );
		mbx2 = _mm_unpackhi_epi32( mbx2, mbx2 );
		__m128i r1 = two( src1, src2, o2, o3, mbx2 );
		return _mm_packus_epi16( r0, r1 );
	}

	inline __m128i two( const tjs_uint32* src1, const tjs_uint32* src2, tjs_int o0, tjs_int o1, __m128i mbx ) const {
		// 読み込み
		__m128i mp1 = _mm_cvtsi32_si128( *(src1+o0) );
		__m128i mp = _mm_cvtsi32_si128( *(src1+o1) );
		mp1 = _mm_unpacklo_epi32( mp1, mp );
		mp1 = _mm_unpacklo_epi8( mp1, zero_ );
		__m128i mp2 = _mm_cvtsi32_si128( *(src1+o0+1) );
		mp = _mm_cvtsi32_si128( *(src1+o1+1) );
		mp2 = _mm_unpacklo_epi32( mp2, mp );
		mp2 = _mm_unpacklo_epi8( mp2, zero_ );
		__m128i mp3 = _mm_cvtsi32_si128( *(src2+o0) );
		mp = _mm_cvtsi32_si128( *(src2+o1) );
		mp3 = _mm_unpacklo_epi32( mp3, mp );
		mp3 = _mm_unpacklo_epi8( mp3, zero_ );
		__m128i mp4 = _mm_cvtsi32_si128( *(src2+o0+1) );
		mp = _mm_cvtsi32_si128( *(src2+o1+1) );
		mp4 = _mm_unpacklo_epi32( mp4, mp );
		mp4 = _mm_unpacklo_epi8( mp4, zero_ );

		// 上段をsx_ratioでブレンド
		mp2 = _mm_sub_epi16( mp2, mp1 );	// s0: mm2 = s0p1 - s0p0
		mp2 = _mm_mullo_epi16( mp2, mbx );	// s0:
		mp2 = _mm_srli_epi16( mp2, 8 );		// s0: mm2 = (s0p1 - s0p0) * bx
		mp1 = _mm_add_epi8( mp1, mp2 );		// s0: mm1 = s0p0 + (s0p1 - s0p0) * bx = S0
		// 下段をsx_ratioでブレンド
		mp4 = _mm_sub_epi16( mp4, mp3 );	// s1: mm4 = s1p1 - s1p0
		mp4 = _mm_mullo_epi16( mp4, mbx );	// s1:
		mp4 = _mm_srli_epi16( mp4, 8 );		// s1: mm4 = (s1p1 - s1p0) * bx
		mp3 = _mm_add_epi8( mp3, mp4 );		// s1: mm3 = s1p0 + (s1p1 - s1p0) * bx = S1
		// 上下をsy_ratioでブレンド
		mp3 = _mm_sub_epi16( mp3, mp1 );	// s0/s1: mm3 = S1 - S0
		mp3 = _mm_mullo_epi16( mp3, mby_ );	// s0/s1:
		mp3 = _mm_srli_epi16( mp3, 8 );		// s0/s1: mm3 = (S1 - S0) * by
		mp1 = _mm_add_epi8( mp1, mp3 );		// s0/s1: mm1 = S0 + (S1 - S0) * by = dst
		return mp1;
	}
};


#endif // __INTERPOLATION_FUNCTOR_SSE2_H__
