
#ifndef __BLEND_PS_FUNCTOR_SSE2_H__
#define __BLEND_PS_FUNCTOR_SSE2_H__

#include "blend_functor_c.h"

template<typename blend_func>
struct sse2_ps_variation_opa : public blend_func {
	const tjs_int32 opa_;
	const __m128i opa128_;
	sse2_ps_variation_opa( tjs_int32 opa ) : opa_(opa), opa128_(_mm_set1_epi32(opa)) {}
	inline tjs_uint32 operator()( tjs_uint32 d, tjs_uint32 s ) const {
		tjs_uint32 a = ((s>>25) * opa_)>>8;	// opa
		return blend_func::operator()( d, s, a );
	}
	inline __m128i operator()( __m128i d, __m128i s ) const {
		__m128i a = s;
		a = _mm_srli_epi32( a, 25 );
		a = _mm_mullo_epi16( a, opa128_ );
		a = _mm_srli_epi32( a, 8 );
		return blend_func::operator()( d, s, a );
	}
};
template<typename blend_func>
struct sse2_ps_variation : public blend_func {
	inline tjs_uint32 operator()( tjs_uint32 d, tjs_uint32 s ) const {
		tjs_uint32 a = (s>>25);
		return blend_func::operator()( d, s, a );
	}
	inline __m128i operator()( __m128i d, __m128i s ) const {
		__m128i a = s;
		a = _mm_srli_epi32( a, 25 );
		return blend_func::operator()( d, s, a );
	}
};
//-------------------------------------
struct sse2_ps_pack_func {
	static inline __m128i pack_alpha( __m128i ma ) {
		ma = _mm_unpacklo_epi16( ma, ma );	// = 0000000000AA00AA
		return _mm_unpacklo_epi32( ma, ma );	// = 00AA00AA00AA00AA
	}
	static inline __m128i pack_alpha1( __m128i ma ) {
		ma = _mm_packs_epi32( ma, ma );			// 0 1 2 3 0 1 2 3
		return _mm_unpacklo_epi16( ma, ma );	// 0 0 1 1 2 2 3 3
	}
	static inline __m128i pack_alpha12( __m128i ma ) {
		return _mm_unpacklo_epi16( ma, ma );	 // 0 0 0 0 1 1 1 1
	}
	static inline __m128i pack_alpha2( __m128i ma ) {
		return _mm_unpackhi_epi16( ma, ma );	// 2 2 2 2 3 3 3 3
	}
};
struct sse2_ps_pack_hda_func {
	static inline __m128i pack_alpha( __m128i ma ) {
		__m128i ma2 = ma;						// = 00000000000000AA
		ma = _mm_unpacklo_epi16( ma, ma );	// = 0000000000AA00AA
		return _mm_unpacklo_epi32( ma, ma2 );// = 000000AA00AA00AA (keep dst alpha)
	}
	static inline __m128i pack_alpha1( __m128i ma ) {
		ma = _mm_packs_epi32( ma, ma );			// 0 1 2 3 0 1 2 3
		return _mm_unpacklo_epi16( ma, ma );	// 0 0 1 1 2 2 3 3
	}
	static inline __m128i pack_alpha12( __m128i ma ) {
		ma = _mm_unpacklo_epi16( ma, ma );	 // 0 0 0 0 1 1 1 1
		return _mm_srli_epi64( ma, 16 );	// 000000aa00aa00aa (keep dst alpha)
	}
	static inline __m128i pack_alpha2( __m128i ma ) {
		ma = _mm_unpackhi_epi16( ma, ma );	// 2 2 2 2 3 3 3 3
		return _mm_srli_epi64( ma, 16 );	// 000000aa00aa00aa (keep dst alpha)
	}
};
//-------------------------------------
struct sse2_ps_nullblend_func {
	// md/ms : 00AA00RR00GG00BB
	inline void blend( __m128i& md, __m128i& ms ) const {}
};
//-------------------------------------
template<typename pack_func, typename blend_func>
struct sse2_ps_alpha_blend {
	const __m128i zero_;
	const blend_func blend_;
	inline sse2_ps_alpha_blend() : zero_( _mm_setzero_si128() ) {}
	inline tjs_uint32 one( __m128i md, __m128i ms, __m128i ma ) const {
		ms = _mm_unpacklo_epi8( ms, zero_ );	// 00AA00RR00GG00BB
		md = _mm_unpacklo_epi8( md, zero_ );	// 00AA00RR00GG00BB
		blend_.blend( md, ms );
		ma = pack_func::pack_alpha( ma );
		ms = _mm_sub_epi16( ms, md );		// src-dst
		ms = _mm_mullo_epi16( ms, ma );	// (src-dst)*a
		ms = _mm_srai_epi16( ms, 7 );	// (((src-dst)*a)>>7)
		md = _mm_add_epi16( md, ms );	// (((src-dst)*a)>>7)+dst
		md = _mm_packus_epi16( md, zero_ );// AARRGGBB
		return _mm_cvtsi128_si32( md );
	}
	inline __m128i operator()( __m128i md1, __m128i ms1, __m128i ma1 ) const {
		__m128i ms2 = ms1;
		__m128i md2 = md1;

		ms1 = _mm_unpacklo_epi8( ms1, zero_ );	// 00AA00RR00GG00BB (src l)
		md1 = _mm_unpacklo_epi8( md1, zero_ );	// 00AA00RR00GG00BB (dst l)
		blend_.blend( md1, ms1 );
		ma1 = pack_func::pack_alpha1( ma1 );
		__m128i ma2 = ma1;
		ma1 = pack_func::pack_alpha12( ma1 );

		ms1 = _mm_sub_epi16( ms1, md1 );	// src-dst (l)
		ms1 = _mm_mullo_epi16( ms1, ma1 );	// (src-dst)*a (l)
		ms1 = _mm_srai_epi16( ms1, 7 );		// ((src-dst)*a)>>7
		md1 = _mm_add_epi16( md1, ms1 );	// (((src-dst)*a)>>7)+dst

		ms2 = _mm_unpackhi_epi8( ms2, zero_ );	// 00AA00RR00GG00BB (src h)
		md2 = _mm_unpackhi_epi8( md2, zero_ );	// 00AA00RR00GG00BB (dst h)
		blend_.blend( md2, ms2 );
		ma2 = pack_func::pack_alpha2( ma2 );

		ms2 = _mm_sub_epi16( ms2, md2 );	// src-dst (h)
		ms2 = _mm_mullo_epi16( ms2, ma2 );	// (src-dst)*a (h)
		ms2 = _mm_srai_epi16( ms2, 7 );		// ((src-dst)*a)>>7
		md2 = _mm_add_epi16( md2, ms2 );	// (((src-dst)*a)>>7)+dst
		return _mm_packus_epi16( md1, md2 );	// AARRGGBBAARRGGBB
	}
};
typedef sse2_ps_alpha_blend<sse2_ps_pack_func,sse2_ps_nullblend_func>		sse2_ps_alpha_blend_func;
typedef sse2_ps_alpha_blend<sse2_ps_pack_hda_func,sse2_ps_nullblend_func>	sse2_ps_alpha_blend_hda_func;
//-------------------------------------

template<typename alpha_blend_base>
struct sse2_ps_alpha_blend_f : public alpha_blend_base {
	inline tjs_uint32 operator()( tjs_uint32 d, tjs_uint32 s, tjs_uint32 a ) const {
		__m128i ma = _mm_cvtsi32_si128( a );
		__m128i ms = _mm_cvtsi32_si128( s );
		__m128i md = _mm_cvtsi32_si128( d );
		return alpha_blend_base::one( md, ms, ma );
	}
	inline __m128i operator()( __m128i md1, __m128i ms1, __m128i ma1 ) const {
		return alpha_blend_base::operator()( md1, ms1, ma1 );
	}
};
typedef sse2_ps_alpha_blend_f<sse2_ps_alpha_blend_func>		sse2_ps_alpha_blend_n;
typedef sse2_ps_alpha_blend_f<sse2_ps_alpha_blend_hda_func>	sse2_ps_alpha_blend_hda;

typedef sse2_ps_variation<sse2_ps_alpha_blend_n>		sse2_ps_alpha_blend_functor;
typedef sse2_ps_variation_opa<sse2_ps_alpha_blend_n>	sse2_ps_alpha_blend_o_functor;
typedef sse2_ps_variation<sse2_ps_alpha_blend_hda>		sse2_ps_alpha_blend_hda_functor;
typedef sse2_ps_variation_opa<sse2_ps_alpha_blend_hda>	sse2_ps_alpha_blend_hda_o_functor;
//-------------------------------------

template<typename alpha_blend_base>	// sse2_ps_alpha_blend_func or sse2_ps_alpha_blend_hda_func
struct sse2_ps_add_blend_func : public alpha_blend_base {
	inline tjs_uint32 operator()( tjs_uint32 d, tjs_uint32 s, tjs_uint32 a ) const {
		__m128i ma = _mm_cvtsi32_si128( a );
		__m128i ms = _mm_cvtsi32_si128( s );
		__m128i md = _mm_cvtsi32_si128( d );
		return one( md, ms, ma );
	}
	inline tjs_uint32 one( __m128i md, __m128i ms, __m128i ma ) const {
		ms = _mm_adds_epu8( ms, md );		// dst+src (saturate)
		return alpha_blend_base::one( md, ms, ma );
	}
	inline __m128i operator()( __m128i md1, __m128i ms1, __m128i ma1 ) const {
		ms1 = _mm_adds_epu8( ms1, md1 );		// dst+src (saturate)
		return alpha_blend_base::operator()( md1, ms1, ma1 );
	}
};
typedef sse2_ps_add_blend_func<sse2_ps_alpha_blend_func>		sse2_ps_add_blend_n;
typedef sse2_ps_add_blend_func<sse2_ps_alpha_blend_hda_func>	sse2_ps_add_blend_hda;

typedef sse2_ps_variation<sse2_ps_add_blend_n>			sse2_ps_add_blend_functor;
typedef sse2_ps_variation_opa<sse2_ps_add_blend_n>		sse2_ps_add_blend_o_functor;
typedef sse2_ps_variation<sse2_ps_add_blend_hda>		sse2_ps_add_blend_hda_functor;
typedef sse2_ps_variation_opa<sse2_ps_add_blend_hda>	sse2_ps_add_blend_hda_o_functor;
//-------------------------------------

template<typename alpha_blend_base>	// sse2_ps_alpha_blend_func or sse2_ps_alpha_blend_hda_func
struct sse2_ps_sub_blend_func : public alpha_blend_base {
	inline tjs_uint32 operator()( tjs_uint32 d, tjs_uint32 s, tjs_uint32 a ) const {
		__m128i ma = _mm_cvtsi32_si128( a );
		__m128i ms = _mm_cvtsi32_si128( s );
		__m128i md = _mm_cvtsi32_si128( d );
		return one( md, ms, ma );
	}
	inline tjs_uint32 one( __m128i md, __m128i ms, __m128i ma ) const {
		__m128i mask_ms = _mm_setzero_si128();
		mask_ms = _mm_cmpeq_epi32( mask_ms, mask_ms );	// = FFFFFFFFFFFFFFFF
		ms = _mm_xor_si128( ms, mask_ms );				// = ~src == 1 - src
		mask_ms = md;
		mask_ms = _mm_subs_epu8( mask_ms, ms );			// dst-src (saturate)
		return alpha_blend_base::one( md, mask_ms, ma );
	}
	inline __m128i operator()( __m128i md1, __m128i ms1, __m128i ma1 ) const {
		__m128i mask_ms = _mm_setzero_si128();
		mask_ms = _mm_cmpeq_epi32( mask_ms, mask_ms );	// = FFFFFFFFFFFFFFFF
		ms1 = _mm_xor_si128( ms1, mask_ms );			// = ~src == 1 - src
		mask_ms = md1;
		mask_ms = _mm_subs_epu8( mask_ms, ms1 );		// dst-src (saturate)
		return alpha_blend_base::operator()( md1, mask_ms, ma1 );
	}
};
typedef sse2_ps_sub_blend_func<sse2_ps_alpha_blend_func>		sse2_ps_sub_blend_n;
typedef sse2_ps_sub_blend_func<sse2_ps_alpha_blend_hda_func>	sse2_ps_sub_blend_hda;

typedef sse2_ps_variation<sse2_ps_sub_blend_n>			sse2_ps_sub_blend_functor;
typedef sse2_ps_variation_opa<sse2_ps_sub_blend_n>		sse2_ps_sub_blend_o_functor;
typedef sse2_ps_variation<sse2_ps_sub_blend_hda>		sse2_ps_sub_blend_hda_functor;
typedef sse2_ps_variation_opa<sse2_ps_sub_blend_hda>	sse2_ps_sub_blend_hda_o_functor;
//-------------------------------------

#define SSE2_PS_BLEND_VARIATION( type )	\
typedef sse2_ps_alpha_blend_f<sse2_ps_alpha_blend<sse2_ps_pack_func,sse2_ps_##type##_blend_func> >		sse2_ps_##type##_blend_n;	\
typedef sse2_ps_alpha_blend_f<sse2_ps_alpha_blend<sse2_ps_pack_hda_func,sse2_ps_##type##_blend_func> >	sse2_ps_##type##_blend_hda;	\
typedef sse2_ps_variation<sse2_ps_##type##_blend_n>			sse2_ps_##type##_blend_functor;			\
typedef sse2_ps_variation_opa<sse2_ps_##type##_blend_n>		sse2_ps_##type##_blend_o_functor;		\
typedef sse2_ps_variation<sse2_ps_##type##_blend_hda>		sse2_ps_##type##_blend_hda_functor;		\
typedef sse2_ps_variation_opa<sse2_ps_##type##_blend_hda>	sse2_ps_##type##_blend_hda_o_functor;

struct sse2_ps_mul_blend_func {
	// md/ms : 00AA00RR00GG00BB | ret ms
	inline void blend( __m128i& md, __m128i& ms ) const {
		ms = _mm_mullo_epi16( ms, md );	// ms = dst*src
		ms = _mm_srli_epi16( ms, 8 );	// ms >>= 8  (/255) XXX penalty
	}
};
SSE2_PS_BLEND_VARIATION( mul )
/*
typedef sse2_ps_alpha_blend<sse2_ps_pack_func,sse2_ps_mul_blend_func>		sse2_ps_mul_blend_n;
typedef sse2_ps_alpha_blend<sse2_ps_pack_hda_func,sse2_ps_mul_blend_func>	sse2_ps_mul_blend_hda;

typedef sse2_ps_variation<sse2_ps_mul_blend_n>			sse2_ps_mul_blend_functor;
typedef sse2_ps_variation_opa<sse2_ps_mul_blend_n>		sse2_ps_mul_blend_o_functor;
typedef sse2_ps_variation<sse2_ps_mul_blend_hda>		sse2_ps_mul_blend_hda_functor;
typedef sse2_ps_variation_opa<sse2_ps_mul_blend_hda>	sse2_ps_mul_blend_hda_o_functor;
*/
//-------------------------------------
#if 0
struct sse2_ps_screen_blend_func {
	inline void blend( __m128i& md, __m128i& ms ) const {
		__m128i ms2 = ms;
		ms = _mm_mullo_epi16( ms, md );	// mm1 = dst*src
		ms = _mm_srli_epi16( ms, 8 );	// mm1 >>= 8 (/255) XXX penalty
		md = ms;
		ms = ms2;
	}
};
SSE2_PS_BLEND_VARIATION( screen )
#endif
template<typename pack_func>
struct sse2_ps_screen_blend {
	const __m128i zero_;
	inline sse2_ps_screen_blend() : zero_( _mm_setzero_si128() ) {}
	inline tjs_uint32 one( __m128i md, __m128i ms, __m128i ma ) const {
		ms = _mm_unpacklo_epi8( ms, zero_ );	// 00AA00RR00GG00BB
		md = _mm_unpacklo_epi8( md, zero_ );	// 00AA00RR00GG00BB

		__m128i ms2 = ms;
		ms = _mm_mullo_epi16( ms, md );	// = dst*src
		ms = _mm_srli_epi16( ms, 8 );	// >>= 8 (/255) XXX penalty

		ma = pack_func::pack_alpha( ma );

		ms2 = _mm_sub_epi16( ms2, ms );		// src-dst
		ms2 = _mm_mullo_epi16( ms2, ma );	// (src-dst)*a
		ms2 = _mm_srai_epi16( ms2, 7 );	// (((src-dst)*a)>>7)
		md = _mm_add_epi16( md, ms2 );	// (((src-dst)*a)>>7)+dst
		md = _mm_packus_epi16( md, zero_ );// AARRGGBB
		return _mm_cvtsi128_si32( md );
	}
	inline __m128i operator()( __m128i md1, __m128i ms1, __m128i ma1 ) const {
		__m128i ms2 = ms1;
		__m128i md2 = md1;

		ms1 = _mm_unpacklo_epi8( ms1, zero_ );	// 00AA00RR00GG00BB (src l)
		md1 = _mm_unpacklo_epi8( md1, zero_ );	// 00AA00RR00GG00BB (dst l)

		__m128i ms12 = ms1;
		ms1 = _mm_mullo_epi16( ms1, md1 );	// = dst*src
		ms1 = _mm_srli_epi16( ms1, 8 );	// >>= 8 (/255) XXX penalty

		ma1 = pack_func::pack_alpha1( ma1 );
		__m128i ma2 = ma1;
		ma1 = pack_func::pack_alpha12( ma1 );

		ms12 = _mm_sub_epi16( ms12, ms1 );	// src-dst (l)
		ms12 = _mm_mullo_epi16( ms12, ma1 );	// (src-dst)*a (l)
		ms12 = _mm_srai_epi16( ms12, 7 );		// ((src-dst)*a)>>7
		md1 = _mm_add_epi16( md1, ms12 );	// (((src-dst)*a)>>7)+dst

		ms2 = _mm_unpackhi_epi8( ms2, zero_ );	// 00AA00RR00GG00BB (src h)
		md2 = _mm_unpackhi_epi8( md2, zero_ );	// 00AA00RR00GG00BB (dst h)
		
		__m128i ms22 = ms2;
		ms2 = _mm_mullo_epi16( ms2, md2 );	// = dst*src
		ms2 = _mm_srli_epi16( ms2, 8 );	// >>= 8 (/255) XXX penalty

		ma2 = pack_func::pack_alpha2( ma2 );

		ms22 = _mm_sub_epi16( ms22, ms2 );	// src-dst (h)
		ms22 = _mm_mullo_epi16( ms22, ma2 );	// (src-dst)*a (h)
		ms22 = _mm_srai_epi16( ms22, 7 );		// ((src-dst)*a)>>7
		md2 = _mm_add_epi16( md2, ms22 );	// (((src-dst)*a)>>7)+dst
		return _mm_packus_epi16( md1, md2 );	// AARRGGBBAARRGGBB
	}
};
typedef sse2_ps_screen_blend<sse2_ps_pack_func>		sse2_ps_screen_blend_func;
typedef sse2_ps_screen_blend<sse2_ps_pack_hda_func>	sse2_ps_screen_blend_hda_func;

typedef sse2_ps_alpha_blend_f<sse2_ps_screen_blend_func>		sse2_ps_screen_blend_n;
typedef sse2_ps_alpha_blend_f<sse2_ps_screen_blend_hda_func>	sse2_ps_screen_blend_hda;

typedef sse2_ps_variation<sse2_ps_screen_blend_n>		sse2_ps_screen_blend_functor;
typedef sse2_ps_variation_opa<sse2_ps_screen_blend_n>	sse2_ps_screen_blend_o_functor;
typedef sse2_ps_variation<sse2_ps_screen_blend_hda>		sse2_ps_screen_blend_hda_functor;
typedef sse2_ps_variation_opa<sse2_ps_screen_blend_hda>	sse2_ps_screen_blend_hda_o_functor;

/*
typedef sse2_ps_alpha_blend<sse2_ps_pack_func,sse2_ps_screen_blend_func>		sse2_ps_screen_blend_n;
typedef sse2_ps_alpha_blend<sse2_ps_pack_hda_func,sse2_ps_screen_blend_func>	sse2_ps_screen_blend_hda;

typedef sse2_ps_variation<sse2_ps_screen_blend_n>			sse2_ps_screen_blend_functor;
typedef sse2_ps_variation_opa<sse2_ps_screen_blend_n>		sse2_ps_screen_blend_o_functor;
typedef sse2_ps_variation<sse2_ps_screen_blend_hda>			sse2_ps_screen_blend_hda_functor;
typedef sse2_ps_variation_opa<sse2_ps_screen_blend_hda>		sse2_ps_screen_blend_hda_o_functor;
*/
//-------------------------------------

struct sse2_ps_overlay_blend_func {
	const __m128i mask_;
	inline sse2_ps_overlay_blend_func() : mask_(_mm_set1_epi16(0x00FF)) {}
	inline void blend( __m128i& md, __m128i& ms ) const {
		__m128i ms2 = ms;	// = src backup
		ms = _mm_mullo_epi16( ms, md );	// = dst*src
		ms2 = _mm_add_epi16( ms2, md );	// = dst+src
		ms = _mm_srli_epi16( ms, 7 );	// = s*d*2/255
		ms2 = _mm_slli_epi16( ms2, 1 );	// *= 2
		//__m128i threshold = _mm_cvtsi32_si128( 0x80808080 );	// = eax(80808080)
		__m128i threshold = _mm_set1_epi16(0x0080);
		ms2 = _mm_sub_epi16( ms2, ms );	// = (d+s)*2-(d*s*2)/255
		//threshold = _mm_unpacklo_epi8( threshold, zero );	// = 0080008000800080
		ms2 = _mm_sub_epi16( ms2, mask_ );	// = (d+s)*2-(d*s*2)/255-255  XXX could be overflow because of >>8 error
		threshold = _mm_cmpgt_epi16( threshold, md );	// = (128>d)?0xffff:0
		ms = _mm_and_si128( ms, threshold );
		threshold = _mm_andnot_si128( threshold, ms2 );
		ms = _mm_or_si128( ms, threshold );
	}
};
SSE2_PS_BLEND_VARIATION( overlay )
/*
typedef sse2_ps_alpha_blend<sse2_ps_pack_func,sse2_ps_overlay_blend_func>		sse2_ps_overlay_blend_n;
typedef sse2_ps_alpha_blend<sse2_ps_pack_hda_func,sse2_ps_overlay_blend_func>	sse2_ps_overlay_blend_hda;

typedef sse2_ps_variation<sse2_ps_overlay_blend_n>			sse2_ps_overlay_blend_functor;
typedef sse2_ps_variation_opa<sse2_ps_overlay_blend_n>		sse2_ps_overlay_blend_o_functor;
typedef sse2_ps_variation<sse2_ps_overlay_blend_hda>		sse2_ps_overlay_blend_hda_functor;
typedef sse2_ps_variation_opa<sse2_ps_overlay_blend_hda>	sse2_ps_overlay_blend_hda_o_functor;
*/
//-------------------------------------

struct sse2_ps_hardlight_blend_func {
	const __m128i mask_;
	inline sse2_ps_hardlight_blend_func() : mask_(_mm_set1_epi16(0x00FF)) {}
	inline void blend( __m128i& md, __m128i& ms ) const {
		//__m128i threshold = _mm_cvtsi32_si128( 0x80808080 );
		//threshold = _mm_unpacklo_epi8( threshold, zero );		// = 0080008000800080
		__m128i threshold = _mm_set1_epi16(0x0080);
		__m128i ms2 = ms;	// = src backup
		threshold = _mm_cmpgt_epi16( threshold, ms2 );	// = (128>s)?0xffff:0
		ms2 = _mm_add_epi16( ms2, md );	// = dst+src
		ms = _mm_mullo_epi16( ms, md );	// = dst*src
		ms2 = _mm_slli_epi16( ms2, 1 );	// *= 2
		ms = _mm_srli_epi16( ms, 7 );	// = s*d*2/255
		ms2 = _mm_sub_epi16( ms2, ms );	// = (d+s-d*s)*2

		ms2 = _mm_sub_epi16( ms2, mask_ );	// = (d+s-d*s)*2-255  XXX could be overflow because of >>8 error
		ms = _mm_and_si128( ms, threshold );
		threshold = _mm_andnot_si128( threshold, ms2 );
		ms = _mm_or_si128( ms, threshold );
	}
};
SSE2_PS_BLEND_VARIATION( hardlight )
/*
typedef sse2_ps_alpha_blend<sse2_ps_pack_func,sse2_ps_hardlight_blend_func>		sse2_ps_hardlight_blend_n;
typedef sse2_ps_alpha_blend<sse2_ps_pack_hda_func,sse2_ps_hardlight_blend_func>	sse2_ps_hardlight_blend_hda;

typedef sse2_ps_variation<sse2_ps_hardlight_blend_n>		sse2_ps_hardlight_blend_functor;
typedef sse2_ps_variation_opa<sse2_ps_hardlight_blend_n>	sse2_ps_hardlight_blend_o_functor;
typedef sse2_ps_variation<sse2_ps_hardlight_blend_hda>		sse2_ps_hardlight_blend_hda_functor;
typedef sse2_ps_variation_opa<sse2_ps_hardlight_blend_hda>	sse2_ps_hardlight_blend_hda_o_functor;
*/
//-------------------------------------

template<typename TTable,typename alpha_blend_base>
struct sse2_ps_table_blend_func : public alpha_blend_base {
	inline tjs_uint32 operator()( tjs_uint32 d, tjs_uint32 s, tjs_uint32 a ) const {
		s = (TTable::TABLE[(s>>16)&0xff][(d>>16)&0xff]<<16) |
			(TTable::TABLE[(s>>8 )&0xff][(d>>8 )&0xff]<<8 ) |
			(TTable::TABLE[(s>>0 )&0xff][(d>>0 )&0xff]<<0 );
		__m128i ms = _mm_cvtsi32_si128( s );
		__m128i ma = _mm_cvtsi32_si128( a );
		__m128i md = _mm_cvtsi32_si128( d );
		return alpha_blend_base::one( md, ms, ma );
	}
	inline __m128i operator()( __m128i md1, __m128i ms1, __m128i ma1 ) const {
		int d = _mm_cvtsi128_si32( md1 );
		int s = _mm_cvtsi128_si32( ms1 );
		s = (TTable::TABLE[(s>>16)&0xff][(d>>16)&0xff]<<16) |
			(TTable::TABLE[(s>>8 )&0xff][(d>>8 )&0xff]<<8 ) |
			(TTable::TABLE[(s>>0 )&0xff][(d>>0 )&0xff]<<0 );
		__m128i sw1 = _mm_cvtsi32_si128(s);

		ms1 = _mm_shuffle_epi32( ms1, _MM_SHUFFLE( 0, 3, 2, 1 ) );
		md1 = _mm_shuffle_epi32( md1, _MM_SHUFFLE( 0, 3, 2, 1 ) );
		d = _mm_cvtsi128_si32( md1 );
		s = _mm_cvtsi128_si32( ms1 );
		s = (TTable::TABLE[(s>>16)&0xff][(d>>16)&0xff]<<16) |
			(TTable::TABLE[(s>>8 )&0xff][(d>>8 )&0xff]<<8 ) |
			(TTable::TABLE[(s>>0 )&0xff][(d>>0 )&0xff]<<0 );
		__m128i sw2 = _mm_cvtsi32_si128(s);
		sw1 = _mm_unpacklo_epi32( sw1, sw2 );

		ms1 = _mm_shuffle_epi32( ms1, _MM_SHUFFLE( 0, 3, 2, 1 ) );
		md1 = _mm_shuffle_epi32( md1, _MM_SHUFFLE( 0, 3, 2, 1 ) );
		d = _mm_cvtsi128_si32( md1 );
		s = _mm_cvtsi128_si32( ms1 );
		s = (TTable::TABLE[(s>>16)&0xff][(d>>16)&0xff]<<16) |
			(TTable::TABLE[(s>>8 )&0xff][(d>>8 )&0xff]<<8 ) |
			(TTable::TABLE[(s>>0 )&0xff][(d>>0 )&0xff]<<0 );
		sw2 = _mm_cvtsi32_si128(s);

		ms1 = _mm_shuffle_epi32( ms1, _MM_SHUFFLE( 0, 3, 2, 1 ) );
		md1 = _mm_shuffle_epi32( md1, _MM_SHUFFLE( 0, 3, 2, 1 ) );
		d = _mm_cvtsi128_si32( md1 );
		s = _mm_cvtsi128_si32( ms1 );
		s = (TTable::TABLE[(s>>16)&0xff][(d>>16)&0xff]<<16) |
			(TTable::TABLE[(s>>8 )&0xff][(d>>8 )&0xff]<<8 ) |
			(TTable::TABLE[(s>>0 )&0xff][(d>>0 )&0xff]<<0 );
		ms1 = _mm_cvtsi32_si128(s);
		sw2 = _mm_unpacklo_epi32( sw2, ms1 );
		sw1 = _mm_unpacklo_epi64( sw1, sw2 );

		md1 = _mm_shuffle_epi32( md1, _MM_SHUFFLE( 0, 3, 2, 1 ) );
		return alpha_blend_base::operator()( md1, sw1, ma1 );
	}
};

typedef sse2_ps_table_blend_func<ps_soft_light_table,sse2_ps_alpha_blend_func>		sse2_ps_softlight_blend_n;
typedef sse2_ps_table_blend_func<ps_soft_light_table,sse2_ps_alpha_blend_hda_func>	sse2_ps_softlight_blend_hda;

typedef sse2_ps_variation<sse2_ps_softlight_blend_n>		sse2_ps_softlight_blend_functor;
typedef sse2_ps_variation_opa<sse2_ps_softlight_blend_n>	sse2_ps_softlight_blend_o_functor;
typedef sse2_ps_variation<sse2_ps_softlight_blend_hda>		sse2_ps_softlight_blend_hda_functor;
typedef sse2_ps_variation_opa<sse2_ps_softlight_blend_hda>	sse2_ps_softlight_blend_hda_o_functor;
//-------------------------------------

typedef sse2_ps_table_blend_func<ps_color_dodge_table,sse2_ps_alpha_blend_func>		sse2_ps_colordodge_blend_n;
typedef sse2_ps_table_blend_func<ps_color_dodge_table,sse2_ps_alpha_blend_hda_func>	sse2_ps_colordodge_blend_hda;

typedef sse2_ps_variation<sse2_ps_colordodge_blend_n>		sse2_ps_colordodge_blend_functor;
typedef sse2_ps_variation_opa<sse2_ps_colordodge_blend_n>	sse2_ps_colordodge_blend_o_functor;
typedef sse2_ps_variation<sse2_ps_colordodge_blend_hda>		sse2_ps_colordodge_blend_hda_functor;
typedef sse2_ps_variation_opa<sse2_ps_colordodge_blend_hda>	sse2_ps_colordodge_blend_hda_o_functor;
//-------------------------------------

typedef sse2_ps_table_blend_func<ps_color_burn_table,sse2_ps_alpha_blend_func>		sse2_ps_colorburn_blend_n;
typedef sse2_ps_table_blend_func<ps_color_burn_table,sse2_ps_alpha_blend_hda_func>	sse2_ps_colorburn_blend_hda;

typedef sse2_ps_variation<sse2_ps_colorburn_blend_n>		sse2_ps_colorburn_blend_functor;
typedef sse2_ps_variation_opa<sse2_ps_colorburn_blend_n>	sse2_ps_colorburn_blend_o_functor;
typedef sse2_ps_variation<sse2_ps_colorburn_blend_hda>		sse2_ps_colorburn_blend_hda_functor;
typedef sse2_ps_variation_opa<sse2_ps_colorburn_blend_hda>	sse2_ps_colorburn_blend_hda_o_functor;
//-------------------------------------

template<typename TTable,typename pack_func>
struct sse2_ps_table_blend_func2 {
	const __m128i zero_;
	inline sse2_ps_table_blend_func2() : zero_( _mm_setzero_si128() ) {}
	inline tjs_uint32 operator()( tjs_uint32 d, tjs_uint32 s, tjs_uint32 a ) const {
		__m128i ma = _mm_cvtsi32_si128( a );
		__m128i ms = _mm_cvtsi32_si128( s );

		ma = pack_func::pack_alpha( ma );
		//ma = _mm_unpacklo_epi16( ma, ma );	// = 0000000000AA00AA
		//ma = _mm_unpacklo_epi32( ma, ma );	// = 00AA00AA00AA00AA
		ms = _mm_unpacklo_epi8( ms, zero_ );	// = 00AA00RR00GG00BB
		ms = _mm_mullo_epi16( ms, ma );	// = src*a
		ms = _mm_srai_epi16( ms, 7 );	// = ((src*a)>>7)
		ms = _mm_packus_epi16( ms, zero_ );	// = AARRGGBB
		s = _mm_cvtsi128_si32( ms );	// = src
		s = (TTable::TABLE[(s>>16)&0xff][(d>>16)&0xff]<<16) |
			(TTable::TABLE[(s>>8 )&0xff][(d>>8 )&0xff]<<8 ) |
			(TTable::TABLE[(s>>0 )&0xff][(d>>0 )&0xff]<<0 );
		return s;
	}
	inline __m128i operator()( __m128i md1, __m128i ms1, __m128i ma1 ) const {
		__m128i ms2 = ms1;
		ma1 = pack_func::pack_alpha1( ma1 );
		__m128i ma2 = ma1;
		ma1 = pack_func::pack_alpha12( ma1 );

		ms1 = _mm_unpacklo_epi8( ms1, zero_ );	// 00AA00RR00GG00BB (src l)
		ms1 = _mm_mullo_epi16( ms1, ma1 );		// (src-dst)*a (l)
		ms1 = _mm_srai_epi16( ms1, 7 );			// ((src-dst)*a)>>7
		ms1 = _mm_packus_epi16( ms1, zero_ );	// = AARRGGBB
		int d = _mm_cvtsi128_si32( md1 );
		int s = _mm_cvtsi128_si32( ms1 );
		s = (TTable::TABLE[(s>>16)&0xff][(d>>16)&0xff]<<16) |
			(TTable::TABLE[(s>>8 )&0xff][(d>>8 )&0xff]<<8 ) |
			(TTable::TABLE[(s>>0 )&0xff][(d>>0 )&0xff]<<0 );
		__m128i sw1 = _mm_cvtsi32_si128(s);

		ms1 = _mm_shuffle_epi32( ms1, _MM_SHUFFLE( 0, 3, 2, 1 ) );
		md1 = _mm_shuffle_epi32( md1, _MM_SHUFFLE( 0, 3, 2, 1 ) );
		d = _mm_cvtsi128_si32( md1 );
		s = _mm_cvtsi128_si32( ms1 );
		s = (TTable::TABLE[(s>>16)&0xff][(d>>16)&0xff]<<16) |
			(TTable::TABLE[(s>>8 )&0xff][(d>>8 )&0xff]<<8 ) |
			(TTable::TABLE[(s>>0 )&0xff][(d>>0 )&0xff]<<0 );
		ms1 = _mm_cvtsi32_si128(s);
		sw1 = _mm_unpacklo_epi32( sw1, ms1 );

		ma2 = pack_func::pack_alpha2( ma2 );
		ms2 = _mm_unpackhi_epi8( ms2, zero_ );	// 00AA00RR00GG00BB (src h)
		ms2 = _mm_mullo_epi16( ms2, ma2 );		// (src-dst)*a (h)
		ms2 = _mm_srai_epi16( ms2, 7 );			// ((src-dst)*a)>>7
		ms2 = _mm_packus_epi16( ms2, zero_ );	// = AARRGGBB
		md1 = _mm_shuffle_epi32( md1, _MM_SHUFFLE( 0, 3, 2, 1 ) );
		s = _mm_cvtsi128_si32( ms2 );
		d = _mm_cvtsi128_si32( md1 );
		s = (TTable::TABLE[(s>>16)&0xff][(d>>16)&0xff]<<16) |
			(TTable::TABLE[(s>>8 )&0xff][(d>>8 )&0xff]<<8 ) |
			(TTable::TABLE[(s>>0 )&0xff][(d>>0 )&0xff]<<0 );
		ms1 = _mm_cvtsi32_si128(s);

		ms2 = _mm_shuffle_epi32( ms2, _MM_SHUFFLE( 0, 3, 2, 1 ) );
		md1 = _mm_shuffle_epi32( md1, _MM_SHUFFLE( 0, 3, 2, 1 ) );
		s = _mm_cvtsi128_si32( ms2 );
		d = _mm_cvtsi128_si32( md1 );
		s = (TTable::TABLE[(s>>16)&0xff][(d>>16)&0xff]<<16) |
			(TTable::TABLE[(s>>8 )&0xff][(d>>8 )&0xff]<<8 ) |
			(TTable::TABLE[(s>>0 )&0xff][(d>>0 )&0xff]<<0 );
		ms2 = _mm_cvtsi32_si128(s);
		ms1 = _mm_unpacklo_epi32( ms1, ms2 );
		return _mm_unpacklo_epi64( sw1, ms1 );
	}
};
template<typename TTable>
struct sse2_ps_table_blend_func2_hda : public sse2_ps_table_blend_func2<TTable,sse2_ps_pack_func> {
	typedef sse2_ps_table_blend_func2<TTable,sse2_ps_pack_func> base;
	const __m128i mask_;
	inline sse2_ps_table_blend_func2_hda() : mask_(_mm_set1_epi32(0xFF000000)) {}
	inline tjs_uint32 operator()( tjs_uint32 d, tjs_uint32 s, tjs_uint32 a ) const {
		return base::operator()( d, s, a ) | (d & 0xff000000);
	}
	inline __m128i operator()( __m128i md1, __m128i ms1, __m128i ma1 ) const {
		__m128i mda = md1;
		mda = _mm_and_si128( mda, mask_ );
		__m128i ret = base::operator()( md1, ms1, ma1 );
		return _mm_or_si128( ret, mda );
	}
};
typedef sse2_ps_table_blend_func2<ps_color_dodge_table,sse2_ps_pack_func>	sse2_ps_colordodge5_blend_n;
typedef sse2_ps_table_blend_func2_hda<ps_color_dodge_table>					sse2_ps_colordodge5_blend_hda;

typedef sse2_ps_variation<sse2_ps_colordodge5_blend_n>			sse2_ps_colordodge5_blend_functor;
typedef sse2_ps_variation_opa<sse2_ps_colordodge5_blend_n>		sse2_ps_colordodge5_blend_o_functor;
typedef sse2_ps_variation<sse2_ps_colordodge5_blend_hda>		sse2_ps_colordodge5_blend_hda_functor;
typedef sse2_ps_variation_opa<sse2_ps_colordodge5_blend_hda>	sse2_ps_colordodge5_blend_hda_o_functor;
//-------------------------------------

struct sse2_ps_lighten_blend_func {
	inline void blend( __m128i& md, __m128i& ms ) const {
		__m128i md2 = md;
		md2 = _mm_subs_epu8( md2, ms );	// = dst-src (saturate)
		ms = _mm_add_epi8( ms, md2 );	// = src+diff = lighten
	}
};
SSE2_PS_BLEND_VARIATION( lighten )
	/*
typedef sse2_ps_alpha_blend<sse2_ps_pack_func,sse2_ps_lighten_blend_func>		sse2_ps_lighten_blend_n;
typedef sse2_ps_alpha_blend<sse2_ps_pack_hda_func,sse2_ps_lighten_blend_func>	sse2_ps_lighten_blend_hda;

typedef sse2_ps_variation<sse2_ps_lighten_blend_n>			sse2_ps_lighten_blend_functor;
typedef sse2_ps_variation_opa<sse2_ps_lighten_blend_n>		sse2_ps_lighten_blend_o_functor;
typedef sse2_ps_variation<sse2_ps_lighten_blend_hda>		sse2_ps_lighten_blend_hda_functor;
typedef sse2_ps_variation_opa<sse2_ps_lighten_blend_hda>	sse2_ps_lighten_blend_hda_o_functor;
*/
//-------------------------------------

struct sse2_ps_darken_blend_func {
	inline void blend( __m128i& md, __m128i& ms ) const {
		__m128i ms2 = ms;
		ms2 = _mm_subs_epu8( ms2, md );	// = src-dst (saturate)
		ms = _mm_sub_epi8( ms, ms2 );	// = darken
	}
};
SSE2_PS_BLEND_VARIATION( darken )
	/*
typedef sse2_ps_alpha_blend<sse2_ps_pack_func,sse2_ps_darken_blend_func>		sse2_ps_darken_blend_n;
typedef sse2_ps_alpha_blend<sse2_ps_pack_hda_func,sse2_ps_darken_blend_func>	sse2_ps_darken_blend_hda;

typedef sse2_ps_variation<sse2_ps_darken_blend_n>		sse2_ps_darken_blend_functor;
typedef sse2_ps_variation_opa<sse2_ps_darken_blend_n>	sse2_ps_darken_blend_o_functor;
typedef sse2_ps_variation<sse2_ps_darken_blend_hda>		sse2_ps_darken_blend_hda_functor;
typedef sse2_ps_variation_opa<sse2_ps_darken_blend_hda>	sse2_ps_darken_blend_hda_o_functor;
*/
//-------------------------------------

template<typename alpha_blend_base>	// sse2_ps_alpha_blend_func or sse2_ps_alpha_blend_hda_func
struct sse2_ps_diff_blend_func : public alpha_blend_base {
	inline tjs_uint32 operator()( tjs_uint32 d, tjs_uint32 s, tjs_uint32 a ) const {
		__m128i ma = _mm_cvtsi32_si128( a );
		__m128i ms = _mm_cvtsi32_si128( s );
		__m128i md = _mm_cvtsi32_si128( d );
		return one( md, ms, ma );
	}
	inline tjs_uint32 one( __m128i md, __m128i ms, __m128i ma ) const {
		__m128i md2 = md;
		md2 = _mm_subs_epu8( md2, ms );	// dst-src (saturate)
		ms = _mm_subs_epu8( ms, md );	// src-dst (saturate)
		md2 = _mm_add_epi8( md2, ms );	// Diff
		return alpha_blend_base::one( md, md2, ma );
	}
	inline __m128i operator()( __m128i md1, __m128i ms1, __m128i ma1 ) const {
		__m128i md2 = md1;
		md2 = _mm_subs_epu8( md2, ms1 );	// dst-src (saturate)
		ms1 = _mm_subs_epu8( ms1, md1 );	// src-dst (saturate)
		md2 = _mm_add_epi8( md2, ms1 );		// Diff
		return alpha_blend_base::operator()( md1, md2, ma1 );
	}
};
typedef sse2_ps_diff_blend_func<sse2_ps_alpha_blend_func>		sse2_ps_diff_blend_n;
typedef sse2_ps_diff_blend_func<sse2_ps_alpha_blend_hda_func>	sse2_ps_diff_blend_hda;

typedef sse2_ps_variation<sse2_ps_diff_blend_n>			sse2_ps_diff_blend_functor;
typedef sse2_ps_variation_opa<sse2_ps_diff_blend_n>		sse2_ps_diff_blend_o_functor;
typedef sse2_ps_variation<sse2_ps_diff_blend_hda>		sse2_ps_diff_blend_hda_functor;
typedef sse2_ps_variation_opa<sse2_ps_diff_blend_hda>	sse2_ps_diff_blend_hda_o_functor;
//-------------------------------------


// Photoshop5 works :
//   1. s = (*src) * alpha
//   2. diff = abs(s-(*dst))
template<typename pack_func>
struct sse2_ps_diff5_blend_func {
	const __m128i zero_;
	inline sse2_ps_diff5_blend_func() : zero_( _mm_setzero_si128() ) {}
	inline tjs_uint32 operator()( tjs_uint32 d, tjs_uint32 s, tjs_uint32 a ) const {
		__m128i ma = _mm_cvtsi32_si128( a );
		__m128i ms = _mm_cvtsi32_si128( s );
		__m128i md = _mm_cvtsi32_si128( d );
		return one( md, ms, ma );
	}
	inline tjs_uint32 one( __m128i md, __m128i ms, __m128i ma ) const {
		ms = _mm_unpacklo_epi8( ms, zero_ );	// = 00AA00RR00GG00BB
		md = _mm_unpacklo_epi8( md, zero_ );	// = 00AA00RR00GG00BB
		ma = pack_func::pack_alpha( ma );

		ms = _mm_mullo_epi16( ms, ma );	// = src*a
		__m128i md2 = md;
		ms = _mm_srai_epi16( ms, 7 );	// = ((src*a)>>7) XXX penalty (mul result)
		md2 = _mm_subs_epu16( md2, ms );	// = dst-src (saturate)
		ms = _mm_subs_epu16( ms, md );	// = src-dst (saturate)
		md2 = _mm_add_epi16( md2, ms );	// = diff
		md2 = _mm_packus_epi16( md2, zero_ );	// = AARRGGBB
		return _mm_cvtsi128_si32( md2 );
	}
	inline __m128i operator()( __m128i md1, __m128i ms1, __m128i ma1 ) const {
		__m128i ms2 = ms1;
		__m128i md2 = md1;

		ms1 = _mm_unpacklo_epi8( ms1, zero_ );	// 00AA00RR00GG00BB (src l)
		md1 = _mm_unpacklo_epi8( md1, zero_ );	// 00AA00RR00GG00BB (dst l)

		ma1 = pack_func::pack_alpha1( ma1 );
		__m128i ma2 = ma1;
		ma1 = pack_func::pack_alpha12( ma1 );

		ms1 = _mm_mullo_epi16( ms1, ma1 );	// = src*a
		__m128i md12 = md1;
		ms1 = _mm_srai_epi16( ms1, 7 );		// = ((src*a)>>7)
		md12 = _mm_subs_epu16( md12, ms1 );	// = dst-src (saturate)
		ms1 = _mm_subs_epu16( ms1, md1 );		// = src-dst (saturate)
		md12 = _mm_add_epi16( md12, ms1 );	// = diff

		ms2 = _mm_unpackhi_epi8( ms2, zero_ );	// 00AA00RR00GG00BB (src h)
		md2 = _mm_unpackhi_epi8( md2, zero_ );	// 00AA00RR00GG00BB (dst h)
		ma2 = pack_func::pack_alpha2( ma2 );

		ms2 = _mm_mullo_epi16( ms2, ma2 );	// = src*a
		__m128i md22 = md2;
		ms2 = _mm_srai_epi16( ms2, 7 );		// = ((src*a)>>7) XXX penalty (mul result)
		md22 = _mm_subs_epu16( md22, ms2 );	// = dst-src (saturate)
		ms2 = _mm_subs_epu16( ms2, md2 );	// = src-dst (saturate)
		md22 = _mm_add_epi16( md22, ms2 );	// = diff
		return _mm_packus_epi16( md12, md22 );// = AARRGGBBAARRGGBB
	}
};
typedef sse2_ps_diff5_blend_func<sse2_ps_pack_func>		sse2_ps_diff5_blend_n;
typedef sse2_ps_diff5_blend_func<sse2_ps_pack_hda_func>	sse2_ps_diff5_blend_hda;

typedef sse2_ps_variation<sse2_ps_diff5_blend_n>		sse2_ps_diff5_blend_functor;
typedef sse2_ps_variation_opa<sse2_ps_diff5_blend_n>	sse2_ps_diff5_blend_o_functor;
typedef sse2_ps_variation<sse2_ps_diff5_blend_hda>		sse2_ps_diff5_blend_hda_functor;
typedef sse2_ps_variation_opa<sse2_ps_diff5_blend_hda>	sse2_ps_diff5_blend_hda_o_functor;
//-------------------------------------

template<typename pack_func>
struct sse2_ps_exclusion_blend_func {
	const __m128i zero_;
	inline sse2_ps_exclusion_blend_func() : zero_( _mm_setzero_si128() ) {}
	inline tjs_uint32 operator()( tjs_uint32 d, tjs_uint32 s, tjs_uint32 a ) const {
		__m128i ma = _mm_cvtsi32_si128( a );
		__m128i ms = _mm_cvtsi32_si128( s );
		__m128i md = _mm_cvtsi32_si128( d );
		return one( md, ms, ma );
	}
	inline tjs_uint32 one( __m128i md, __m128i ms, __m128i ma ) const {
		md = _mm_unpacklo_epi8( md, zero_ );	// = 00AA00RR00GG00BB
		ms = _mm_unpacklo_epi8( ms, zero_ );	// = 00AA00RR00GG00BB

		__m128i ms2 = ms;
		ms = _mm_mullo_epi16( ms, md );	// = dst*src
		ms = _mm_srli_epi16( ms, 7 );	// = s*d*2/255
		ms2 = _mm_sub_epi16( ms2, ms );	// = (d+s)-d*s*2

		ma = pack_func::pack_alpha( ma );

		ms2 = _mm_mullo_epi16( ms2, ma );	// = (src-dst)*a
		ms2 = _mm_srai_epi16( ms2, 7 );		// = (((src-dst)*a)>>7)
		md = _mm_add_epi16( md, ms2 );		// = (((src-dst)*a)>>7)+dst
		md = _mm_packus_epi16( md, zero_ );	// = AARRGGBB
		return _mm_cvtsi128_si32( md );
	}
	inline __m128i operator()( __m128i md1, __m128i ms1, __m128i ma1 ) const {
		__m128i ms2 = ms1;
		__m128i md2 = md1;

		md1 = _mm_unpacklo_epi8( md1, zero_ );	// = 00AA00RR00GG00BB
		ms1 = _mm_unpacklo_epi8( ms1, zero_ );	// = 00AA00RR00GG00BB

		__m128i ms12 = ms1;
		ms1 = _mm_mullo_epi16( ms1, md1 );	// = dst*src
		ms1 = _mm_srli_epi16( ms1, 7 );		// = s*d*2/255
		ms12 = _mm_sub_epi16( ms12, ms1 );	// = (d+s)-d*s*2

		ma1 = pack_func::pack_alpha1( ma1 );
		__m128i ma2 = ma1;
		ma1 = pack_func::pack_alpha12( ma1 );

		ms12 = _mm_mullo_epi16( ms12, ma1 );	// = (src-dst)*a
		ms12 = _mm_srai_epi16( ms12, 7 );	// = (((src-dst)*a)>>7)
		md1 = _mm_add_epi16( md1, ms12 );	// = (((src-dst)*a)>>7)+dst

		md2 = _mm_unpackhi_epi8( md2, zero_ );	// = 00AA00RR00GG00BB
		ms2 = _mm_unpackhi_epi8( ms2, zero_ );	// = 00AA00RR00GG00BB

		__m128i ms22 = ms2;
		ms2 = _mm_mullo_epi16( ms2, md2 );	// = dst*src
		ms2 = _mm_srli_epi16( ms2, 7 );		// = s*d*2/255
		ms22 = _mm_sub_epi16( ms22, ms2 );	// = (d+s)-d*s*2

		ma2 = pack_func::pack_alpha2( ma2 );

		ms22 = _mm_mullo_epi16( ms22, ma2 );	// = (src-dst)*a
		ms22 = _mm_srai_epi16( ms22, 7 );	// = (((src-dst)*a)>>7)
		md2 = _mm_add_epi16( md2, ms22 );	// = (((src-dst)*a)>>7)+dst
		return _mm_packus_epi16( md1, md2 );	// = AARRGGBB
	}
};
typedef sse2_ps_exclusion_blend_func<sse2_ps_pack_func>		sse2_ps_exclusion_blend_n;
typedef sse2_ps_exclusion_blend_func<sse2_ps_pack_hda_func>	sse2_ps_exclusion_blend_hda;

typedef sse2_ps_variation<sse2_ps_exclusion_blend_n>		sse2_ps_exclusion_blend_functor;
typedef sse2_ps_variation_opa<sse2_ps_exclusion_blend_n>	sse2_ps_exclusion_blend_o_functor;
typedef sse2_ps_variation<sse2_ps_exclusion_blend_hda>		sse2_ps_exclusion_blend_hda_functor;
typedef sse2_ps_variation_opa<sse2_ps_exclusion_blend_hda>	sse2_ps_exclusion_blend_hda_o_functor;
//-------------------------------------

#endif // __BLEND_PS_FUNCTOR_SSE2_H__
