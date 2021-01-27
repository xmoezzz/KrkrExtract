
#ifndef __BLEND_FUNCTOR_AVX2_H__
#define __BLEND_FUNCTOR_AVX2_H__

#include "x86simdutil.h"

extern "C" {
extern unsigned char TVPOpacityOnOpacityTable[256*256];
extern unsigned char TVPNegativeMulTable[256*256];
};

// ソースのアルファを使う
template<typename blend_func>
struct avx2_variation : public blend_func {
	inline tjs_uint32 operator()( tjs_uint32 d, tjs_uint32 s ) const {
		tjs_uint32 a = (s>>24);
		return blend_func::operator()( d, s, a );
	}
	inline __m256i operator()( __m256i d, __m256i s ) const {
		__m256i a = s;
		a = _mm256_srli_epi32( a, 24 );
		return blend_func::operator()( d, s, a );
	}
};

// ソースのアルファとopacity値を使う
template<typename blend_func>
struct avx2_variation_opa : public blend_func {
	const tjs_int32 opa_;
	const __m256i opa256_;
	inline avx2_variation_opa( tjs_int32 opa ) : opa_(opa), opa256_(_mm256_set1_epi32(opa)) {}
	inline tjs_uint32 operator()( tjs_uint32 d, tjs_uint32 s ) const {
		tjs_uint32 a = (tjs_uint32)( ((tjs_uint64)s*(tjs_uint64)opa_) >> 32 );	// 最適化でうまくmulの上位ビット入るはず
		return blend_func::operator()( d, s, a );
	}
	inline __m256i operator()( __m256i d, __m256i s ) const {
		__m256i a = s;
		a = _mm256_srli_epi32( a, 24 );
		a = _mm256_mullo_epi16( a, opa256_ );
		a = _mm256_srli_epi32( a, 8 );
		return blend_func::operator()( d, s, a );
	}
};

// ソースとデスティネーションのアルファを使う
struct avx2_alpha_blend_d_functor {
	const __m256i m255_;
	const __m256i zero_;
	const __m256i colormask_;
	const __m256 m65535_;
	inline avx2_alpha_blend_d_functor() : m255_(_mm256_set1_epi32(255)), m65535_(_mm256_set1_ps(65535.0f)),
		zero_(_mm256_setzero_si256()), colormask_(_mm256_set1_epi32(0x00ffffff)) {}

	// SSE4.1を使う
	inline tjs_uint32 operator()( tjs_uint32 d, tjs_uint32 s ) const {
		tjs_uint32 addr = ((s >> 16) & 0xff00) + (d>>24);
		tjs_uint32 sopa = TVPOpacityOnOpacityTable[addr];

		__m128i ma = _mm_cvtsi32_si128( sopa );
		ma = _mm_shufflelo_epi16( ma, _MM_SHUFFLE( 0, 0, 0, 0 )  );	// 00oo00oo00oo00oo
		__m128i ms = _mm_cvtsi32_si128( s );
		ms = _mm_cvtepu8_epi16( ms );// 00 ss 00 ss 00 ss 00 ss
		__m128i md = _mm_cvtsi32_si128( d );
		md = _mm_cvtepu8_epi16( md );// 00 dd 00 dd 00 dd 00 dd

		ms = _mm_sub_epi16( ms, md );		// ms -= md
		ms = _mm_mullo_epi16( ms, ma );		// ms *= ma
		md = _mm_slli_epi16( md, 8 );		// md <<= 8
		md = _mm_add_epi16( md, ms );		// md += ms
		md = _mm_srli_epi16( md, 8 );		// ms >>= 8
		md = _mm_packus_epi16( md, md );
		tjs_uint32 ret = _mm_cvtsi128_si32( md );

		addr = TVPNegativeMulTable[addr] << 24;
		return (ret&0x00ffffff) | addr;
	}
	// AVX2 版はテーブルを使わない、最大誤差8になるが許容する
	inline __m256i operator()( __m256i md1, __m256i ms1 ) const {
		__m256i ca = m255_;

		// ca = sa + (da * (1 - sa))
		__m256i da1 = md1;
		__m256i sa1 = ms1;
		da1 = _mm256_srli_epi32(da1,24);	// da
		sa1 = _mm256_srli_epi32(sa1,24);	// sa
		ca = _mm256_sub_epi32(ca,sa1);		// 255 - sa
		ca = _mm256_mullo_epi16(ca,da1);	// (255 - sa)*da
		ca = _mm256_add_epi32(ca,m255_);	// (255 - sa)*da + 255
		ca = _mm256_srli_epi32(ca,8);		// ((255 - sa)*da + 255)>>8
		ca = _mm256_add_epi32(ca,sa1);		// sa + ((255 - sa)*da + 255)>>8
		__m256i dopa = ca;
		dopa = _mm256_slli_epi32(dopa,24);

		// AVXで逆数を求める
		__m256 work = _mm256_cvtepi32_ps(ca);
		work = _mm256_rcp_ps(work);
		work = _mm256_mul_ps(work,m65535_);
		__m256i invca = _mm256_cvtps_epi32(work);

		__m256i ms2 = ms1;
		__m256i md2 = md1;
		ms1 = _mm256_unpacklo_epi8( ms1, zero_ );// 00 ss 00 ss 00 ss 00 ss
		md1 = _mm256_unpacklo_epi8( md1, zero_ );// 00 dd 00 dd 00 dd 00 dd
		ms2 = _mm256_unpackhi_epi8( ms2, zero_ );// 00 ss 00 ss 00 ss 00 ss
		md2 = _mm256_unpackhi_epi8( md2, zero_ );// 00 dd 00 dd 00 dd 00 dd

		// c = (dc * (da * (1 - sa)) + sc * sa) / ca
		// dc*da - dc*da*sa + sc*sa
		// da*(dc - dc*sa) + sc*sa : 出来るだけ精度一杯で計算する。
		sa1 = _mm256_shufflelo_epi16( sa1, _MM_SHUFFLE( 2, 2, 0, 0 )  ); // 0 0 1 1 X 2 X 3
		sa1 = _mm256_shufflehi_epi16( sa1, _MM_SHUFFLE( 2, 2, 0, 0 )  ); // 0 0 1 1 2 2 3 3
		__m256i sa2 = sa1;
		sa1 = _mm256_unpacklo_epi16( sa1, sa1 );	// 0 0 0 0 1 1 1 1
		sa2 = _mm256_unpackhi_epi16( sa2, sa2 );	// 2 2 2 2 3 3 3 3

		__m256i tmp = md1;
		tmp = _mm256_mullo_epi16( tmp, sa1 );	// dc*sa
		md1 = _mm256_slli_epi16( md1, 8 );		// 上位バイトへ移動
		md1 = _mm256_subs_epu16( md1, tmp );	// (dc - dc*sa)
		tmp = md2;
		tmp = _mm256_mullo_epi16( tmp, sa2 );	// dc*sa
		md2 = _mm256_slli_epi16( md2, 8 );		// 上位バイトへ移動
		md2 = _mm256_subs_epu16( md2, tmp );	// (dc - dc*sa)
		
		da1 = _mm256_slli_epi32( da1, 8 );		// 上位バイトへ移動
		da1 = _mm256_shufflelo_epi16( da1, _MM_SHUFFLE( 2, 2, 0, 0 )  ); // 0 0 1 1 X 2 X 3
		da1 = _mm256_shufflehi_epi16( da1, _MM_SHUFFLE( 2, 2, 0, 0 )  ); // 0 0 1 1 2 2 3 3
		__m256i da2 = da1;
		da1 = _mm256_unpacklo_epi16( da1, da1 );	// 0 0 0 0 1 1 1 1
		da2 = _mm256_unpackhi_epi16( da2, da2 );	// 2 2 2 2 3 3 3 3
		md1 = _mm256_mulhi_epu16( md1, da1 );		// da*(dc - dc*sa)
		md2 = _mm256_mulhi_epu16( md2, da2 );		// da*(dc - dc*sa)
		ms1 = _mm256_mullo_epi16( ms1, sa1 );		// sc*sa
		ms2 = _mm256_mullo_epi16( ms2, sa2 );		// sc*sa
		md1 = _mm256_adds_epu16( md1, ms1 );		// da*(dc - dc*sa) + sc*sa
		md2 = _mm256_adds_epu16( md2, ms2 );		// da*(dc - dc*sa) + sc*sa

		invca = _mm256_shufflelo_epi16( invca, _MM_SHUFFLE( 2, 2, 0, 0 )  ); // 0 0 1 1 X 2 X 3
		invca = _mm256_shufflehi_epi16( invca, _MM_SHUFFLE( 2, 2, 0, 0 )  ); // 0 0 1 1 2 2 3 3
		__m256i invca2 = invca;
		invca = _mm256_unpacklo_epi16( invca, invca );		// 0 0 0 0 1 1 1 1
		invca2 = _mm256_unpackhi_epi16( invca2, invca2 );	// 2 2 2 2 3 3 3 3

		md1 = _mm256_mulhi_epu16( md1, invca );		// (dc * (da * (1 - sa)) + sc * sa) / ca
		md2 = _mm256_mulhi_epu16( md2, invca2 );	// (dc * (da * (1 - sa)) + sc * sa) / ca
		md1 = _mm256_packus_epi16( md1, md2 );
		md1 = _mm256_and_si256( md1, colormask_ );
		return _mm256_or_si256( md1, dopa );
	}
};

template<typename blend_func>
struct avx2_variation_hda : public blend_func {
	__m256i alphamask_;
	__m256i colormask_;
	inline avx2_variation_hda() {
		alphamask_ = _mm256_set1_epi32( 0xFF000000 );
		colormask_ = _mm256_set1_epi32( 0x00FFFFFF );
	}
	inline avx2_variation_hda( tjs_int32 opa ) : blend_func(opa) {
		alphamask_ = _mm256_set1_epi32( 0xFF000000 );
		colormask_ = _mm256_set1_epi32( 0x00FFFFFF );
	}
	inline tjs_uint32 operator()( tjs_uint32 d, tjs_uint32 s ) const {
		tjs_uint32 dstalpha = d&0xff000000;
		tjs_uint32 ret = blend_func::operator()( d, s );
		return (ret&0x00ffffff)|dstalpha;
	}
	inline __m256i operator()( __m256i d, __m256i s ) const {
		__m256i dstalpha = _mm256_and_si256( d, alphamask_ );
		__m256i ret = blend_func::operator()( d, s );
		ret = _mm256_and_si256( ret, colormask_ );
		return _mm256_or_si256( ret, dstalpha );
	}
};

struct avx2_alpha_blend_func {
	const __m256i zero_;
	inline avx2_alpha_blend_func() : zero_( _mm256_setzero_si256() ) {}
	inline tjs_uint32 one( __m128i md, __m128i ms, __m128i ma ) const {
		ms = _mm_cvtepu8_epi16( ms );// 00 ss 00 ss 00 ss 00 ss
		md = _mm_cvtepu8_epi16( md );// 00 dd 00 dd 00 dd 00 dd
		ma = _mm_shufflelo_epi16( ma, _MM_SHUFFLE( 0, 0, 0, 0 )  );	// 00aa00aa00aa00aa
		ms = _mm_sub_epi16( ms, md );		// ms -= md
		ms = _mm_mullo_epi16( ms, ma );		// ms *= ma
		ms = _mm_srli_epi16( ms, 8 );		// ms >>= 8
		md = _mm_add_epi8( md, ms );		// md += ms : d + ((s-d)*sopa)>>8
		md = _mm_packus_epi16( md, md );	// pack
		return _mm_cvtsi128_si32( md );		// store
	}
#if 0
	// こっちは遅い。zeroを省けるがそれ以上に遅い。
	inline __m256i operator()( __m256i md, __m256i ms, __m256i ma1 ) const {
		__m128i ms1s = _mm256_extracti128_si256( ms, 0 );
		__m128i md1s = _mm256_extracti128_si256( md, 0 );
		__m256i ms1 = _mm256_cvtepu8_epi16( ms1s );	// 00ss00ss00ss00ss
		__m256i md1 = _mm256_cvtepu8_epi16( md1s );	// 00dd00dd00dd00dd

		ma1 = _mm256_packs_epi32( ma1, ma1 );		// 4 5 6 7 4 5 6 7 | 0 1 2 3 0 1 2 3
		ma1 = _mm256_unpacklo_epi16( ma1, ma1 );	// 4 4 5 5 6 6 7 7 | 0 0 1 1 2 2 3 3
		__m256i ma2 = ma1;
		ma1 = _mm256_permute4x64_epi64( ma1, (1 << 6) | (1 << 4) | (0 << 2) | (0 << 0) );	// 00 11 00 11 | 22 33 22 33
		ma2 = _mm256_permute4x64_epi64( ma2, (3 << 6) | (3 << 4) | (2 << 2) | (2 << 0) );
		ma1 = _mm256_unpacklo_epi16( ma1, ma1 );	// 0000 1111 2222 3333
		ma2 = _mm256_unpacklo_epi16( ma2, ma2 );	// 4444 5555 6666 7777

		ms1 = _mm256_sub_epi16( ms1, md1 );		// s -= d
		ms1 = _mm256_mullo_epi16( ms1, ma1 );	// s *= a
		ms1 = _mm256_srli_epi16( ms1, 8 );		// s >>= 8
		md1 = _mm256_add_epi8( md1, ms1 );		// d += s

		__m128i ms2s = _mm256_extracti128_si256( ms, 1 );
		__m128i md2s = _mm256_extracti128_si256( md, 1 );
		__m256i ms2 = _mm256_cvtepu8_epi16( ms2s );	// 00ss00ss00ss00ss
		__m256i md2 = _mm256_cvtepu8_epi16( md2s );	// 00dd00dd00dd00dd
		ms2 = _mm256_sub_epi16( ms2, md2 );		// s -= d
		ms2 = _mm256_mullo_epi16( ms2, ma2 );	// s *= a
		ms2 = _mm256_srli_epi16( ms2, 8 );		// s >>= 8
		md2 = _mm256_add_epi8( md2, ms2 );		// d += s

		md1 = _mm256_packus_epi16( md1, md2 );	// 0 2 1 3
		return _mm256_permute4x64_epi64( md1, (3 << 6) | (1 << 4) | (2 << 2) | (0 << 0) );
	}
#endif
	// zero を使ってunpack する方が段違いに速い
	inline __m256i operator()( __m256i md, __m256i ms, __m256i ma1 ) const {
		__m256i ms1 = _mm256_unpacklo_epi8( ms, zero_ );
		__m256i md1 = _mm256_unpacklo_epi8( md, zero_ );
		ma1 = _mm256_packs_epi32( ma1, ma1 );			// 4 5 6 7 4 5 6 7 | 0 1 2 3 0 1 2 3
		ma1 = _mm256_unpacklo_epi16( ma1, ma1 );		// 4 4 5 5 6 6 7 7 | 0 0 1 1 2 2 3 3
		__m256i ma2 = _mm256_unpackhi_epi16( ma1, ma1 );// 6 6 6 6 7 7 7 7 | 2 2 2 2 3 3 3 3
		ma1 = _mm256_unpacklo_epi16( ma1, ma1 );		// 4 4 4 4 5 5 5 5 | 0 0 0 0 1 1 1 1
		ms1 = _mm256_sub_epi16( ms1, md1 );		// s -= d
		ms1 = _mm256_mullo_epi16( ms1, ma1 );	// s *= a
		ms1 = _mm256_srli_epi16( ms1, 8 );		// s >>= 8
		md1 = _mm256_add_epi8( md1, ms1 );		// d += s
		__m256i ms2 = _mm256_unpackhi_epi8( ms, zero_ );
		__m256i md2 = _mm256_unpackhi_epi8( md, zero_ );
		ms2 = _mm256_sub_epi16( ms2, md2 );		// s -= d
		ms2 = _mm256_mullo_epi16( ms2, ma2 );	// s *= a
		ms2 = _mm256_srli_epi16( ms2, 8 );		// s >>= 8
		md2 = _mm256_add_epi8( md2, ms2 );		// d += s
		return _mm256_packus_epi16( md1, md2 );	// 4567 0123
	}
};

struct avx2_alpha_blend : public avx2_alpha_blend_func {
	inline tjs_uint32 operator()( tjs_uint32 d, tjs_uint32 s, tjs_uint32 a ) const {
		__m128i ma = _mm_cvtsi32_si128( a );
		__m128i ms = _mm_cvtsi32_si128( s );
		__m128i md = _mm_cvtsi32_si128( d );
		return avx2_alpha_blend_func::one( md, ms, ma );
	}
	inline __m256i operator()( __m256i md1, __m256i ms1, __m256i ma1 ) const {
		return avx2_alpha_blend_func::operator()( md1, ms1, ma1 );
	}
};
// もっともシンプルなコピー dst = src
struct avx2_const_copy_functor {
	inline tjs_uint32 operator()( tjs_uint32 d, tjs_uint32 s ) const { return s; }
	inline __m256i operator()( __m256i md1, __m256i ms1 ) const { return ms1; }
};
// 単純コピーだけど alpha をコピーしない(HDAと同じ)
struct avx2_color_copy_functor {
	const __m256i colormask_;
	const __m256i alphamask_;
	inline avx2_color_copy_functor() : colormask_(_mm256_set1_epi32(0x00ffffff)), alphamask_(_mm256_set1_epi32(0xff000000)) {}
	inline tjs_uint32 operator()( tjs_uint32 d, tjs_uint32 s ) const {
		return (d&0xff000000) + (s&0x00ffffff);
	}
	inline __m256i operator()( __m256i md1, __m256i ms1 ) const {
		ms1 = _mm256_and_si256( ms1, colormask_ );
		md1 = _mm256_and_si256( md1, alphamask_ );
		return _mm256_or_si256( md1, ms1 );
	}
};
// alphaだけコピーする : color_copy の src destを反転しただけ
struct avx2_alpha_copy_functor : public avx2_color_copy_functor {
	inline tjs_uint32 operator()( tjs_uint32 d, tjs_uint32 s ) const {
		return avx2_color_copy_functor::operator()( s, d );
	}
	inline __m256i operator()( __m256i md1, __m256i ms1 ) const {
		return avx2_color_copy_functor::operator()( ms1, md1 );
	}
};
// このままコピーするがアルファを0xffで埋める dst = 0xff000000 | src
struct avx2_color_opaque_functor {
	const __m256i alphamask_;
	inline avx2_color_opaque_functor() : alphamask_(_mm256_set1_epi32(0xff000000)) {}
	inline tjs_uint32 operator()( tjs_uint32 d, tjs_uint32 s ) const { return 0xff000000 | s; }
	inline __m256i operator()( __m256i md1, __m256i ms1 ) const { return _mm256_or_si256( alphamask_, ms1 ); }
};

// 矩形版未実装
struct avx2_alpha_blend_a_functor {
	const __m256i mask_;
	const __m256i zero_;
	inline avx2_alpha_blend_a_functor() : zero_(_mm256_setzero_si256()),
		mask_(_mm256_set_epi32(0x0000ffff,0xffffffff,0x0000ffff,0xffffffff,0x0000ffff,0xffffffff,0x0000ffff,0xffffffff)) {}
	inline tjs_uint32 operator()( tjs_uint32 d, tjs_uint32 s ) const {
		const __m128i mask = _mm256_extracti128_si256( mask_, 0 );
		__m128i ms = _mm_cvtsi32_si128( s );
		__m128i mo = ms;
		mo = _mm_srli_epi32( mo, 24 );			// >> 24
		__m128i msa = mo;
		msa = _mm_slli_epi64( msa, 48 );		// << 48
		mo = _mm_shufflelo_epi16( mo, _MM_SHUFFLE( 0, 0, 0, 0 )  );	// 00oo00oo00oo00oo
		ms = _mm_cvtepu8_epi16( ms );			// 00Sa00Si00Si00Si SSE4.1
		ms = _mm_mullo_epi16( ms, mo );			// src * sopa
		ms = _mm_srli_epi16( ms, 8 );			// src * sopa >> 8
		ms = _mm_and_si128( ms, mask );			// drop alpha
		ms = _mm_or_si128( ms, msa );			// set original alpha

		__m128i md = _mm_cvtsi32_si128( d );
		md = _mm_cvtepu8_epi16( md );		// 00Da00Di00Di00Di SSE4.1
		__m128i md2 = md;
		md2 = _mm_mullo_epi16( md2, mo );	// d * sopa
		md2 = _mm_srli_epi16( md2, 8 );		// 00 SaDa 00 SaDi 00 SaDi 00 SaDi
		md = _mm_sub_epi16( md, md2 );		// d - d*sopa
		md = _mm_add_epi16( md, ms );		// (d-d*sopa) + s
		md = _mm_packus_epi16( md, md );
		return _mm_cvtsi128_si32( md );
	}
	inline __m256i operator()( __m256i md, __m256i ms ) const {
		__m256i mo0 = ms;
		mo0 = _mm256_srli_epi32( mo0, 24 );
		__m256i msa = mo0;
		msa = _mm256_slli_epi32( msa, 24 );		// << 24
		mo0 = _mm256_packs_epi32( mo0, mo0 );		// 0 1 2 3 0 1 2 3
		mo0 = _mm256_unpacklo_epi16( mo0, mo0 );	// 0 0 1 1 2 2 3 3
		__m256i mo1 = mo0;
		mo1 = _mm256_unpacklo_epi16( mo1, mo1 );	// 0 0 0 0 1 1 1 1 o[1]
		mo0 = _mm256_unpackhi_epi16( mo0, mo0 );	// 2 2 2 2 3 3 3 3 o[0]
		
		__m256i ms1 = ms;
		ms = _mm256_unpackhi_epi8( ms, zero_ );
		ms = _mm256_mullo_epi16( ms, mo0 );		// src * sopa
		ms = _mm256_srli_epi16( ms, 8 );			// src * sopa >> 8
		ms = _mm256_and_si256( ms, mask_ );		// drop alpha
		__m256i msa1 = msa;
		msa = _mm256_unpackhi_epi8( msa, zero_ );
		ms = _mm256_or_si256( ms, msa );			// set original alpha
		
		ms1 = _mm256_unpacklo_epi8( ms1, zero_ );
		ms1 = _mm256_srli_epi16( ms1, 8 );			// src * sopa >> 8
		ms1 = _mm256_and_si256( ms1, mask_ );		// drop alpha
		msa1 = _mm256_unpacklo_epi8( msa1, zero_ );
		ms1 = _mm256_or_si256( ms1, msa1 );		// set original alpha

		__m256i md1 = md;
		md = _mm256_unpackhi_epi8( md, zero_ );// 00dd00dd00dd00dd d[0]
		__m256i md02 = md;
		md02 = _mm256_mullo_epi16( md02, mo0 );	// d * sopa | d[0]
		md02 = _mm256_srli_epi16( md02, 8 );	// 00 SaDa 00 SaDi 00 SaDi 00 SaDi | d[0]
		md = _mm256_sub_epi16( md, md02 );		// d - d*sopa | d[0]
		md = _mm256_add_epi16( md, ms );		// d - d*sopa + s | d[0]

		md1 = _mm256_unpacklo_epi8( md1, zero_ );// 00dd00dd00dd00dd d[1]
		__m256i md12 = md1;
		md12 = _mm256_mullo_epi16( md12, mo1 );// d * sopa | d[1]
		md12 = _mm256_srli_epi16( md12, 8 );	// 00 SaDa 00 SaDi 00 SaDi 00 SaDi | d[1]
		md1 = _mm256_sub_epi16( md1, md12 );	// d - d*sopa | d[1]
		md1 = _mm256_add_epi16( md1, ms1 );	// d - d*sopa + s | d[1]

		return _mm256_packus_epi16( md1, md );
	}
};

typedef avx2_variation<avx2_alpha_blend>							avx2_alpha_blend_functor;
typedef avx2_variation_opa<avx2_alpha_blend>						avx2_alpha_blend_o_functor;
typedef avx2_variation_hda<avx2_variation<avx2_alpha_blend> >		avx2_alpha_blend_hda_functor;
typedef avx2_variation_hda<avx2_variation_opa<avx2_alpha_blend> >	avx2_alpha_blend_hda_o_functor;
// avx2_alpha_blend_d_functor
// avx2_alpha_blend_a_functor

struct avx2_premul_alpha_blend_functor {
	const __m256i zero_;
	inline avx2_premul_alpha_blend_functor() : zero_( _mm256_setzero_si256() ) {}
	inline tjs_uint32 operator()( tjs_uint32 d, tjs_uint32 s ) const {
		__m128i ms = _mm_cvtsi32_si128( s );
		tjs_int32 sopa = s >> 24;
		__m128i mo = _mm_cvtsi32_si128( sopa );
		__m128i md = _mm_cvtsi32_si128( d );
		mo = _mm_shufflelo_epi16( mo, _MM_SHUFFLE( 0, 0, 0, 0 )  );	// 0000000000000000 00oo00oo00oo00oo
		md = _mm_cvtepu8_epi16( md );// 00dd00dd00dd00dd
		__m128i md2 = md;
		md = _mm_mullo_epi16( md, mo );		// md * sopa
		md = _mm_srli_epi16( md, 8 );		// md >>= 8
		md2 = _mm_sub_epi16( md2, md );		// d - (d*sopa)>>8
		md2 = _mm_packus_epi16( md2, md2 );// pack
		md2 = _mm_adds_epu8( md2, ms );		// d - ((d*sopa)>>8) + src
		return _mm_cvtsi128_si32( md2 );
	}
	inline __m256i operator()( __m256i d, __m256i s ) const {
		__m256i ma1 = s;
		ma1 = _mm256_srli_epi32( ma1, 24 );		// s >> 24
		ma1 = _mm256_packs_epi32( ma1, ma1 );		// 0 1 2 3 0 1 2 3
		ma1 = _mm256_unpacklo_epi16( ma1, ma1 );	// 0 0 1 1 2 2 3 3
		__m256i ma2 = ma1;
		ma1 = _mm256_unpacklo_epi16( ma1, ma1 );	 // 0 0 0 0 1 1 1 1

		__m256i md2 = d;
		d = _mm256_unpacklo_epi8( d, zero_ );
		__m256i md1 = d;
		d = _mm256_mullo_epi16( d, ma1 );	// md * sopa
		d = _mm256_srli_epi16( d, 8 );		// md >>= 8
		md1 = _mm256_sub_epi16( md1, d );	// d - (d*sopa)>>8

		ma2 = _mm256_unpackhi_epi16( ma2, ma2 );	// 2 2 2 2 3 3 3 3
		md2 = _mm256_unpackhi_epi8( md2, zero_ );
		__m256i md2t = md2;
		md2 = _mm256_mullo_epi16( md2, ma2 );	// md * sopa
		md2 = _mm256_srli_epi16( md2, 8 );		// md >>= 8
		md2t = _mm256_sub_epi16( md2t, md2 );	// d - (d*sopa)>>8

		md1 = _mm256_packus_epi16( md1, md2t );
		return _mm256_adds_epu8( md1, s );		// d - ((d*sopa)>>8) + src
	}
};
//--------------------------------------------------------------------
// di = di - di*a*opa + si*opa
//              ~~~~~Df ~~~~~~ Sf
//           ~~~~~~~~Ds
//      ~~~~~~~~~~~~~Dq
// additive alpha blend with opacity
struct avx2_premul_alpha_blend_o_functor {
	const __m256i zero_;
	const __m256i opa_;
	inline avx2_premul_alpha_blend_o_functor( tjs_int opa ) : zero_( _mm256_setzero_si256() ), opa_(_mm256_set1_epi16((short)opa)) {}
	inline tjs_uint32 operator()( tjs_uint32 d, tjs_uint32 s ) const {
		const __m128i opa = _mm256_extracti128_si256( opa_, 0 );
		__m128i ms = _mm_cvtsi32_si128( s );
		__m128i md = _mm_cvtsi32_si128( d );
		ms = _mm_cvtepu8_epi16( ms );	// 00ss00ss00ss00ss
		md = _mm_cvtepu8_epi16( md );	// 00dd00dd00dd00dd
		ms = _mm_mullo_epi16( ms, opa );	// 00Sf00Sf00Sf00Sf s * opa
		__m128i md2 = md;
		ms = _mm_srli_epi16( ms, 8 );		// s >> 8
		__m128i ms2 = ms;
		ms2 = _mm_srli_epi64( ms2, 48 );		// s >> 48 | sopa
		ms2 = _mm_unpacklo_epi16( ms2, ms2 );
		ms2 = _mm_unpacklo_epi16( ms2, ms2 );// 00Df00Df00Df00Df

		md = _mm_mullo_epi16( md, ms2 );	// 00Ds00Ds00Ds00Ds
		md = _mm_srli_epi16( md, 8 );	// d >> 8
		md2 = _mm_sub_epi16( md2, md );	// 00Dq00Dq00Dq00Dq
		md2 = _mm_add_epi16( md2, ms );	// d + s
		md2 = _mm_packus_epi16( md2, md2 );
		return _mm_cvtsi128_si32( md2 );
	}
	inline __m256i operator()( __m256i d, __m256i s ) const {
		__m256i ms = s;
		__m256i md = d;
		ms = _mm256_unpackhi_epi8( ms, zero_ );
		md = _mm256_unpackhi_epi8( md, zero_ );
		ms = _mm256_mullo_epi16( ms, opa_ );	// 00Sf00Sf00Sf00Sf s * opa
		__m256i md2 = md;
		ms = _mm256_srli_epi16( ms, 8 );		// s >> 8
		__m256i ma = ms;
		ma = _mm256_srli_epi64( ma, 48 );		// s >> 48 : sopa 0 0 0 1 0 0 0 2
		ma = _mm256_shuffle_epi32( ma, _MM_SHUFFLE( 2, 2, 0, 0 ) );	// 0 1 0 1 0 2 0 2
		ma = _mm256_packs_epi32( ma, ma );		// 1 1 2 2 1 1 2 2
		ma = _mm256_unpacklo_epi32( ma, ma );	// 1 1 1 1 2 2 2 2
		// 00Df00Df00Df00Df

		md = _mm256_mullo_epi16( md, ma );	// 00Ds00Ds00Ds00Ds
		md = _mm256_srli_epi16( md, 8 );	// d >> 8
		md2 = _mm256_sub_epi16( md2, md );	// 00Dq00Dq00Dq00Dq
		md2 = _mm256_add_epi16( md2, ms );	// d + s

		s = _mm256_unpacklo_epi8( s, zero_ );
		d = _mm256_unpacklo_epi8( d, zero_ );
		s = _mm256_mullo_epi16( s, opa_ );	// 00Sf00Sf00Sf00Sf s * opa
		__m256i md1 = d;
		s = _mm256_srli_epi16( s, 8 );		// s >> 8
		ma = s;
		ma = _mm256_srli_epi64( ma, 48 );		// s >> 48 | sopa
		ma = _mm256_shuffle_epi32( ma, _MM_SHUFFLE( 2, 2, 0, 0 ) );	// 0 1 0 1 0 2 0 2
		ma = _mm256_packs_epi32( ma, ma );		// 1 1 2 2 1 1 2 2
		ma = _mm256_unpacklo_epi32( ma, ma );	// 1 1 1 1 2 2 2 2
		// 00Df00Df00Df00Df

		d = _mm256_mullo_epi16( d, ma );	// 00Ds00Ds00Ds00Ds
		d = _mm256_srli_epi16( d, 8 );		// d >> 8
		md1 = _mm256_sub_epi16( md1, d );	// 00Dq00Dq00Dq00Dq
		md1 = _mm256_add_epi16( md1, s );	// d + s
		return _mm256_packus_epi16( md1, md2 );
	}
};
/*
	Di = sat(Si, (1-Sa)*Di)
	Da = Sa + Da - SaDa
*/
// additive alpha blend holding destination alpha
struct avx2_premul_alpha_blend_hda_functor {
	const __m256i zero_;
	const __m256i alphamask_;
	const __m256i colormask_;
	inline avx2_premul_alpha_blend_hda_functor() : zero_(_mm256_setzero_si256()),
		alphamask_(_mm256_set_epi32(0x0000ffff,0xffffffff,0x0000ffff,0xffffffff,0x0000ffff,0xffffffff,0x0000ffff,0xffffffff)),
		colormask_(_mm256_set1_epi32(0x00FFFFFF)) {}
	inline tjs_uint32 operator()( tjs_uint32 d, tjs_uint32 s ) const {
		const __m128i alphamask = _mm256_extracti128_si256( alphamask_, 0 );
		const __m128i colormask = _mm256_extracti128_si256( colormask_, 0 );
		__m128i ms = _mm_cvtsi32_si128( s );
		__m128i mo = _mm_cvtsi32_si128( s >> 24 );
		__m128i md = _mm_cvtsi32_si128( d );
		ms = _mm_and_si128( ms, colormask );	// 0000000000ssssss
		md = _mm_cvtepu8_epi16( md );			// 00dd00dd00dd00dd
		mo = _mm_shufflelo_epi16( mo, _MM_SHUFFLE( 0, 0, 0, 0 )  );		// 00oo00oo00oo00oo

		__m128i md2 = md;
		md = _mm_mullo_epi16( md, mo );		// d * opa
		md = _mm_srli_epi16( md, 8 );		// d >> 8
		md = _mm_and_si128( md, alphamask );// d & 0x00ffffff
		md2 = _mm_sub_epi16( md2, md );		// d - d*opa
		md2 = _mm_packus_epi16( md2, md2 );
		md2 = _mm_adds_epu8( md2, ms );		// d + src
		return _mm_cvtsi128_si32( md2 );
	}
	inline __m256i operator()( __m256i md, __m256i s ) const {
		__m256i mo0 = s;
		mo0 = _mm256_srli_epi32( mo0, 24 );
		mo0 = _mm256_packs_epi32( mo0, mo0 );		// 0 1 2 3 0 1 2 3
		mo0 = _mm256_unpacklo_epi16( mo0, mo0 );	// 0 0 1 1 2 2 3 3
		__m256i mo1 = mo0;
		mo1 = _mm256_unpacklo_epi16( mo1, mo1 );	// 0 0 0 0 1 1 1 1 o[1]
		mo0 = _mm256_unpackhi_epi16( mo0, mo0 );	// 2 2 2 2 3 3 3 3 o[0]

		__m256i md0 = md;
		md = _mm256_unpacklo_epi8( md, zero_ );	// 00dd00dd00dd00dd d[1]
		__m256i md12 = md;
		md = _mm256_mullo_epi16( md, mo1 );		// d[1] * o[1]
		md = _mm256_srli_epi16( md, 8 );			// d[1] >> 8
		md = _mm256_and_si256( md, alphamask_);	// d[1] & 0x00ffffff
		md12 = _mm256_sub_epi16( md12, md );		// d[1] - d[1]*opa

		md0 = _mm256_unpackhi_epi8( md0, zero_ );	// 00dd00dd00dd00dd d[0]
		__m256i md02 = md0;
		md0 = _mm256_mullo_epi16( md0, mo0 );		// d[0] * o[0]
		md0 = _mm256_srli_epi16( md0, 8 );			// d[0] >> 8
		md0 = _mm256_and_si256( md0, alphamask_); 	// d[0] & 0x00ffffff
		md02 = _mm256_sub_epi16( md02, md0 );		// d[0] - d[0]*opa
		md02 = _mm256_packus_epi16( md12, md02 );	// pack( d[1], d[0] )

		s = _mm256_and_si256( s, colormask_);		// s & 0x00ffffff00ffffff
		return _mm256_adds_epu8( md02, s );			// d + s
	}
};
// additive alpha blend on additive alpha
struct avx2_premul_alpha_blend_a_functor {
	const __m256i zero_;
	inline avx2_premul_alpha_blend_a_functor() : zero_(_mm256_setzero_si256()) {}
	inline tjs_uint32 operator()( tjs_uint32 d, tjs_uint32 s ) const {
		__m128i ms = _mm_cvtsi32_si128( s );
		__m128i mo = ms;
		mo = _mm_srli_epi64( mo, 24 );			// sopa
		mo = _mm_shufflelo_epi16( mo, _MM_SHUFFLE( 0, 0, 0, 0 )  );	// 00Sa00Sa00Sa00Sa
		ms = _mm_cvtepu8_epi16( ms );	// 00Sa00Si00Si00Si
		__m128i md = _mm_cvtsi32_si128( d );
		md = _mm_cvtepu8_epi16( md );	// 00Da00Di00Di00Di
		__m128i md2 = md;
		md2 = _mm_mullo_epi16( md2, mo );	// d * sopa
		md2 = _mm_srli_epi16( md2, 8 );		// 00 SaDa 00 SaDi 00 SaDi 00 SaDi
		md = _mm_sub_epi16( md, md2 );		// d - d*sopa
		md = _mm_add_epi16( md, ms );		// (d-d*sopa) + s
		md = _mm_packus_epi16( md, md );
		return _mm_cvtsi128_si32( md );
	}
	inline __m256i operator()( __m256i md, __m256i ms ) const {
		__m256i mo0 = ms;
		mo0 = _mm256_srli_epi32( mo0, 24 );
		mo0 = _mm256_packs_epi32( mo0, mo0 );		// 0 1 2 3 0 1 2 3
		mo0 = _mm256_unpacklo_epi16( mo0, mo0 );	// 0 0 1 1 2 2 3 3
		__m256i mo1 = mo0;
		mo1 = _mm256_unpacklo_epi16( mo1, mo1 );	// 0 0 0 0 1 1 1 1 o[1]
		mo0 = _mm256_unpackhi_epi16( mo0, mo0 );	// 2 2 2 2 3 3 3 3 o[0]

		__m256i md1 = md;
		__m256i ms1 = ms;
		md = _mm256_unpackhi_epi8( md, zero_ );// 00dd00dd00dd00dd d[0]
		__m256i md02 = md;
		ms = _mm256_unpackhi_epi8( ms, zero_ );
		md02 = _mm256_mullo_epi16( md02, mo0 );	// d * sopa | d[0]
		md02 = _mm256_srli_epi16( md02, 8 );	// 00 SaDa 00 SaDi 00 SaDi 00 SaDi | d[0]
		md = _mm256_sub_epi16( md, md02 );		// d - d*sopa | d[0]
		md = _mm256_add_epi16( md, ms );		// d - d*sopa + s | d[0]

		md1 = _mm256_unpacklo_epi8( md1, zero_ );// 00dd00dd00dd00dd d[1]
		__m256i md12 = md1;
		ms1 = _mm256_unpacklo_epi8( ms1, zero_ );
		md12 = _mm256_mullo_epi16( md12, mo1 );// d * sopa | d[1]
		md12 = _mm256_srli_epi16( md12, 8 );	// 00 SaDa 00 SaDi 00 SaDi 00 SaDi | d[1]
		md1 = _mm256_sub_epi16( md1, md12 );	// d - d*sopa | d[1]
		md1 = _mm256_add_epi16( md1, ms1 );	// d - d*sopa + s | d[1]

		return _mm256_packus_epi16( md1, md );
	}
};

// opacity値を使う
struct avx2_const_alpha_blend_functor {
	const __m256i opa_;
	const __m256i zero_;
	inline avx2_const_alpha_blend_functor( tjs_int32 opa ) : zero_(_mm256_setzero_si256()), opa_(_mm256_set1_epi16((short)opa)) {}
	inline tjs_uint32 operator()( tjs_uint32 d, tjs_uint32 s ) const {
		const __m128i opa = _mm256_extracti128_si256( opa_, 0 );
		__m128i ms = _mm_cvtsi32_si128( s );
		__m128i md = _mm_cvtsi32_si128( d );
		ms = _mm_cvtepu8_epi16( ms );		// 00 ss 00 ss 00 ss 00 ss
		md = _mm_cvtepu8_epi16( md );		// 00 dd 00 dd 00 dd 00 dd
		ms = _mm_sub_epi16( ms, md );		// ms -= md
		ms = _mm_mullo_epi16( ms, opa );	// ms *= ma
		ms = _mm_srli_epi16( ms, 8 );		// ms >>= 8
		md = _mm_add_epi8( md, ms );		// md += ms : d + ((s-d)*sopa)>>8
		md = _mm_packus_epi16( md, md );	// pack
		return _mm_cvtsi128_si32( md );		// store
	}
	inline __m256i operator()( __m256i md1, __m256i ms1 ) const {
		__m256i ms2 = ms1;
		__m256i md2 = md1;

		ms1 = _mm256_unpacklo_epi8( ms1, zero_ );
		md1 = _mm256_unpacklo_epi8( md1, zero_ );
		ms1 = _mm256_sub_epi16( ms1, md1 );	// s -= d
		ms1 = _mm256_mullo_epi16( ms1, opa_ );	// s *= a
		ms1 = _mm256_srli_epi16( ms1, 8 );		// s >>= 8
		md1 = _mm256_add_epi8( md1, ms1 );		// d += s

		ms2 = _mm256_unpackhi_epi8( ms2, zero_ );
		md2 = _mm256_unpackhi_epi8( md2, zero_ );
		ms2 = _mm256_sub_epi16( ms2, md2 );		// s -= d
		ms2 = _mm256_mullo_epi16( ms2, opa_ );		// s *= a
		ms2 = _mm256_srli_epi16( ms2, 8 );			// s >>= 8
		md2 = _mm256_add_epi8( md2, ms2 );		// d += s
		return _mm256_packus_epi16( md1, md2 );
	}
};
typedef avx2_variation_hda<avx2_const_alpha_blend_functor>	avx2_const_alpha_blend_hda_functor;

// テーブルを使わないように実装したので、要テスト
struct avx2_const_alpha_blend_d_functor {
	const tjs_int32 opa32_;
	const __m256i opa_;
	const __m256i m255_;
	const __m256i zero_;
	const __m256i colormask_;
	const __m256 m65535_;
	inline avx2_const_alpha_blend_d_functor( tjs_int32 opa ) : m255_(_mm256_set1_epi32(255)), m65535_(_mm256_set1_ps(65535.0f)),
		zero_(_mm256_setzero_si256()), colormask_(_mm256_set1_epi32(0x00ffffff)),
		opa32_(opa<<8), opa_(_mm256_set1_epi16((short)opa)) {}

	inline tjs_uint32 operator()( tjs_uint32 d, tjs_uint32 s ) const {
		tjs_uint32 addr = opa32_ + (d>>24);
		tjs_uint32 sopa = TVPOpacityOnOpacityTable[addr];
		__m128i ma = _mm_cvtsi32_si128( sopa );
		ma = _mm_shufflelo_epi16( ma, _MM_SHUFFLE( 0, 0, 0, 0 )  );	// 0000000000000000 00oo00oo00oo00oo
		__m128i ms = _mm_cvtsi32_si128( s );
		__m128i md = _mm_cvtsi32_si128( d );
		ms = _mm_cvtepu8_epi16( ms );		// 00 ss 00 ss 00 ss 00 ss
		md = _mm_cvtepu8_epi16( md );		// 00 dd 00 dd 00 dd 00 dd
		ms = _mm_sub_epi16( ms, md );		// ms -= md
		ms = _mm_mullo_epi16( ms, ma );		// ms *= ma
		ms = _mm_srli_epi16( ms, 8 );		// ms >>= 8
		md = _mm_add_epi8( md, ms );		// md += ms : d + ((s-d)*sopa)>>8
		md = _mm_packus_epi16( md, md );	// pack
		tjs_uint32 ret = _mm_cvtsi128_si32( md );		// store
		addr = TVPNegativeMulTable[addr] << 24;
		return (ret&0x00ffffff) | addr;
	}
	inline __m256i operator()( __m256i md1, __m256i ms1 ) const {
		__m256i ca = m255_;

		// ca = sa + (da * (1 - sa))
		__m256i da1 = md1;
		__m256i sa = opa_;
		da1 = _mm256_srli_epi32(da1,24);	// da
		sa = _mm256_srli_epi32( sa, 16 );	// sa
		ca = _mm256_sub_epi32(ca,sa);		// 255 - sa
		ca = _mm256_mullo_epi16(ca,da1);	// (255 - sa)*da
		ca = _mm256_add_epi32(ca,m255_);	// (255 - sa)*da + 255
		ca = _mm256_srli_epi32(ca,8);		// ((255 - sa)*da + 255)>>8
		ca = _mm256_add_epi32(ca,sa);		// sa + ((255 - sa)*da + 255)>>8
		__m256i dopa = ca;
		dopa = _mm256_slli_epi32(dopa,24);

		// AVXで逆数を求める
		__m256 work = _mm256_cvtepi32_ps(ca);
		work = _mm256_rcp_ps(work);
		work = _mm256_mul_ps(work,m65535_);
		__m256i invca = _mm256_cvtps_epi32(work);

		__m256i ms2 = ms1;
		__m256i md2 = md1;
		ms1 = _mm256_unpacklo_epi8( ms1, zero_ );// 00 ss 00 ss 00 ss 00 ss
		md1 = _mm256_unpacklo_epi8( md1, zero_ );// 00 dd 00 dd 00 dd 00 dd
		ms2 = _mm256_unpackhi_epi8( ms2, zero_ );// 00 ss 00 ss 00 ss 00 ss
		md2 = _mm256_unpackhi_epi8( md2, zero_ );// 00 dd 00 dd 00 dd 00 dd

		// c = (dc * (da * (1 - sa)) + sc * sa) / ca
		// dc*da - dc*da*sa + sc*sa
		// da*(dc - dc*sa) + sc*sa : 出来るだけ精度一杯で計算する。
		__m256i tmp = md1;
		tmp = _mm256_mullo_epi16( tmp, opa_ );	// dc*sa
		md1 = _mm256_slli_epi16( md1, 8 );		// 上位バイトへ移動
		md1 = _mm256_subs_epu16( md1, tmp );	// (dc - dc*sa)
		tmp = md2;
		tmp = _mm256_mullo_epi16( tmp, opa_ );	// dc*sa
		md2 = _mm256_slli_epi16( md2, 8 );		// 上位バイトへ移動
		md2 = _mm256_subs_epu16( md2, tmp );	// (dc - dc*sa)
		
		da1 = _mm256_slli_epi32( da1, 8 );		// 上位バイトへ移動
		da1 = _mm256_shufflelo_epi16( da1, _MM_SHUFFLE( 2, 2, 0, 0 )  ); // 0 0 1 1 X 2 X 3
		da1 = _mm256_shufflehi_epi16( da1, _MM_SHUFFLE( 2, 2, 0, 0 )  ); // 0 0 1 1 2 2 3 3
		__m256i da2 = da1;
		da1 = _mm256_unpacklo_epi16( da1, da1 );	// 0 0 0 0 1 1 1 1
		da2 = _mm256_unpackhi_epi16( da2, da2 );	// 2 2 2 2 3 3 3 3
		md1 = _mm256_mulhi_epu16( md1, da1 );		// da*(dc - dc*sa)
		md2 = _mm256_mulhi_epu16( md2, da2 );		// da*(dc - dc*sa)
		ms1 = _mm256_mullo_epi16( ms1, opa_ );		// sc*sa
		ms2 = _mm256_mullo_epi16( ms2, opa_ );		// sc*sa
		md1 = _mm256_adds_epu16( md1, ms1 );		// da*(dc - dc*sa) + sc*sa
		md2 = _mm256_adds_epu16( md2, ms2 );		// da*(dc - dc*sa) + sc*sa

		invca = _mm256_shufflelo_epi16( invca, _MM_SHUFFLE( 2, 2, 0, 0 )  ); // 0 0 1 1 X 2 X 3
		invca = _mm256_shufflehi_epi16( invca, _MM_SHUFFLE( 2, 2, 0, 0 )  ); // 0 0 1 1 2 2 3 3
		__m256i invca2 = invca;
		invca = _mm256_unpacklo_epi16( invca, invca );		// 0 0 0 0 1 1 1 1
		invca2 = _mm256_unpackhi_epi16( invca2, invca2 );	// 2 2 2 2 3 3 3 3

		md1 = _mm256_mulhi_epu16( md1, invca );		// (dc * (da * (1 - sa)) + sc * sa) / ca
		md2 = _mm256_mulhi_epu16( md2, invca2 );	// (dc * (da * (1 - sa)) + sc * sa) / ca
		md1 = _mm256_packus_epi16( md1, md2 );
		md1 = _mm256_and_si256( md1, colormask_ );
		return _mm256_or_si256( md1, dopa );
	}
};

struct avx2_const_alpha_blend_a_functor {
	const tjs_uint32 opa32_;
	const __m256i opa_;
	const __m256i colormask_;
	const struct avx2_premul_alpha_blend_a_functor blend_;
	inline avx2_const_alpha_blend_a_functor( tjs_int32 opa )
		: colormask_(_mm256_set1_epi32(0x00ffffff)), opa32_(opa<<24), opa_(_mm256_set1_epi32(opa<<24)) {}
	inline tjs_uint32 operator()( tjs_uint32 d, tjs_uint32 s ) const {
		return blend_( d, (s&0x00ffffff)|opa32_ );
	}
	inline __m256i operator()( __m256i md1, __m256i ms1 ) const {
		ms1 = _mm256_and_si256( ms1, colormask_ );
		ms1 = _mm256_or_si256( ms1, opa_ );
		return blend_( md1, ms1 );
	}
};

//																	avx2_const_alpha_blend_functor;
typedef avx2_const_alpha_blend_functor								avx2_const_alpha_blend_sd_functor;
// tjs_uint32 avx2_const_alpha_blend_functor::operator()( tjs_uint32 d, tjs_uint32 s )
// tjs_uint32 avx2_const_alpha_blend_sd_functor::operator()( tjs_uint32 s1, tjs_uint32 s2 )
// と引数は異なるが、処理内容は同じ
// const_alpha_blend は、dest と src1 を共有しているようなもの dest = dest * src
// const_alpha_blend_sd は、dest = src1 * src2

// avx2_const_copy_functor = TVPCopy はない、memcpy になってる
// avx2_color_copy_functor = TVPCopyColor / TVPLinTransColorCopy
// avx2_alpha_copy_functor = TVPCopyMask
// avx2_color_opaque_functor = TVPCopyOpaqueImage
// avx2_const_alpha_blend_functor = TVPConstAlphaBlend
// avx2_const_alpha_blend_hda_functor = TVPConstAlphaBlend_HDA
// avx2_const_alpha_blend_d_functor = TVPConstAlphaBlend_a
// avx2_const_alpha_blend_a_functor = TVPConstAlphaBlend_a

//--------------------------------------------------------------------
// ここまでアルファブレンド
// 加算合成などはps以外はAVX2では対応しない
//--------------------------------------------------------------------

#endif // __BLEND_FUNCTOR_AVX2_H__
