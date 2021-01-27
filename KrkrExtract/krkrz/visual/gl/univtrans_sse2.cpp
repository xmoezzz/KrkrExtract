
// universal transition blender


#include "simd_def_x86x64.h"
#include "tjsTypes.h"
#include "tvpgl.h"
#include "tvpgl_ia32_intf.h"

// basic algorithm
//  opacity = table[*rule]
//  dest = blend(src1, src2, opacity)
// where blend() is:
//   return src1 * opacity + src2 * (1-opacity)

//--------------------------------------------------------------------

extern "C" {
extern unsigned char TVPOpacityOnOpacityTable[256*256];
extern unsigned char TVPNegativeMulTable[256*256];
}

// ほぼopacity使ったアルファブレンドと同じだけど、opaの与えられ方が違う
struct sse2_univ_trans_blend_func {
	const __m128i zero_;
	const tjs_uint32 *table_;
	inline sse2_univ_trans_blend_func(const tjs_uint32 *t) : zero_( _mm_setzero_si128() ), table_(t) {}
	inline tjs_uint32 operator()( tjs_uint32 s1, tjs_uint32 s2, tjs_uint8 rule) const {
		__m128i ms1 = _mm_cvtsi32_si128( s1 );
		__m128i ms2 = _mm_cvtsi32_si128( s2 );
		__m128i mopa = _mm_cvtsi32_si128( table_[rule] );
		mopa = _mm_unpacklo_epi32( mopa, mopa );	// 00 00 00 00 aa aa aa aa
		ms1 = _mm_unpacklo_epi8( ms1, zero_ );	// 00 s1 00 s1 00 s1 00 s1
		ms2 = _mm_unpacklo_epi8( ms2, zero_ );	// 00 s2 00 s2 00 s2 00 s2
		ms2 = _mm_sub_epi16( ms2, ms1 );		// ms -= md
		ms2 = _mm_mullo_epi16( ms2, mopa );		// ms *= ma
		ms2 = _mm_srli_epi16( ms2, 8 );		// ms >>= 8
		ms1 = _mm_add_epi8( ms1, ms2 );			// md += ms : d + ((s-d)*sopa)>>8
		ms1 = _mm_packus_epi16( ms1, zero_ );	// pack
		return _mm_cvtsi128_si32( ms1 );		// store
	}
	inline __m128i operator()( __m128i ms11, __m128i ms21, const tjs_uint8 *rule ) const {
		__m128i ms12 = ms11;	// src1
		__m128i ms22 = ms21;	// src2
		__m128i mopa = _mm_cvtsi32_si128( table_[rule[0]] );
		__m128i mopa2 = _mm_cvtsi32_si128( table_[rule[1]] );
		mopa = _mm_unpacklo_epi32( mopa, mopa2 );	// 00 00 00 00 a0 a0 a1 a1
		mopa = _mm_unpacklo_epi32( mopa, mopa );	// a0 a0 a0 a0 a1 a1 a1 a1
		ms21 = _mm_unpacklo_epi8( ms21, zero_ );	// 00 s2 00 s2 00 s2 00 s2
		ms11 = _mm_unpacklo_epi8( ms11, zero_ );	// 00 s1 00 s1 00 s1 00 s1
		ms21 = _mm_sub_epi16( ms21, ms11 );		// s2 - s1
		ms21 = _mm_mullo_epi16( ms21, mopa );	// s2 *= table
		ms21 = _mm_srli_epi16( ms21, 8 );		// s2 >>= 8
		ms11 = _mm_add_epi8( ms11, ms21 );		// s1 += s2

		mopa = _mm_cvtsi32_si128( table_[rule[2]] );
		mopa2 = _mm_cvtsi32_si128( table_[rule[3]] );
		mopa = _mm_unpacklo_epi32( mopa, mopa2 );	// 00 00 00 00 a0 a0 a1 a1
		mopa = _mm_unpacklo_epi32( mopa, mopa );	// a0 a0 a0 a0 a1 a1 a1 a1
		ms22 = _mm_unpackhi_epi8( ms22, zero_ );	// 2 unpack
		ms12 = _mm_unpackhi_epi8( ms12, zero_ );	// 2 unpack
		ms22 = _mm_sub_epi16( ms22, ms12 );		// s2 - s1
		ms22 = _mm_mullo_epi16( ms22, mopa );	// s2 *= table
		ms22 = _mm_srli_epi16( ms22, 8 );		// s2 >>= 8
		ms12 = _mm_add_epi8( ms12, ms22 );		// s1 += s2
		return _mm_packus_epi16( ms11, ms12 );
	}
#if 0
	// 2ピクセルずつ処理する
	inline void operator()( tjs_uint32 *d, const tjs_uint32 *s1, const tjs_uint32 *s2, const tjs_uint8 *rule ) const {
		__m128i ms1 = _mm_loadl_epi64( (const __m128i*)s1 );
		__m128i ms2 = _mm_loadl_epi64( (const __m128i*)s2 );
		__m128i mopa = _mm_cvtsi32_si128( table_[rule[0]] );
		__m128i mopa2 = _mm_cvtsi32_si128( table_[rule[1]] );
		mopa = _mm_unpacklo_epi32( mopa, mopa2 );	// 00 00 00 00 a0 a0 a1 a1
		mopa = _mm_unpacklo_epi32( mopa, mopa );	// a0 a0 a0 a0 a1 a1 a1 a1
		ms1 = _mm_unpacklo_epi8( ms1, zero_ );	// 00 s1 00 s1 00 s1 00 s1
		ms2 = _mm_unpacklo_epi8( ms2, zero_ );	// 00 s2 00 s2 00 s2 00 s2
		ms1 = _mm_sub_epi16( ms1, ms2 );		// s1 - s2
		ms1 = _mm_mulhi_epi16( ms1, mopa );		// s1 *= table
		ms1 = _mm_slli_epi16( ms1, 1 );			// s1 <<= 1
		ms2 = _mm_add_epi16( ms2, ms1 );		// s2 += s1
		ms2 = _mm_packus_epi16( ms2, zero_ );	// pack
		_mm_storel_epi64((__m128i*)d, ms2);
	}
#endif
};

struct sse2_univ_trans_d_blend_func {
	const __m128i zero_;
	const tjs_uint32 *table_;
	inline sse2_univ_trans_d_blend_func(const tjs_uint32 *t) : zero_( _mm_setzero_si128() ), table_(t) {}
	inline tjs_uint32 operator()( tjs_uint32 s1, tjs_uint32 s2, tjs_uint8 rule) const {
		tjs_uint32 a1 = s1 >> 24;
		tjs_uint32 a2 = s2 >> 24;
		tjs_int opa = table_[rule];
		tjs_uint32 addr = (a2*opa & 0xff00) + (a1*(256-opa) >> 8);
		tjs_int alpha = TVPOpacityOnOpacityTable[addr];

		__m128i mopa = _mm_cvtsi32_si128( alpha );
		mopa = _mm_unpacklo_epi16( mopa, mopa );	// 00 00 00 00 00 00 aa aa
		mopa = _mm_unpacklo_epi16( mopa, mopa );	// 00 00 00 00 aa aa aa aa
		
		__m128i ms1 = _mm_cvtsi32_si128( s1 );
		__m128i ms2 = _mm_cvtsi32_si128( s2 );
		ms1 = _mm_unpacklo_epi8( ms1, zero_ );	// 00 s1 00 s1 00 s1 00 s1
		ms2 = _mm_unpacklo_epi8( ms2, zero_ );	// 00 s2 00 s2 00 s2 00 s2

		ms2 = _mm_sub_epi16( ms2, ms1 );		// s2 -= s1
		ms2 = _mm_mullo_epi16( ms2, mopa );		// s2 *= opa
		ms2 = _mm_srli_epi16( ms2, 8 );			// s2 >>= 8
		ms1 = _mm_add_epi8( ms1, ms2 );			// s1 += s2
		ms1 = _mm_packus_epi16( ms1, zero_ );

		tjs_uint32 dst_alpha = TVPNegativeMulTable[addr]<<24;
		return (_mm_cvtsi128_si32( ms1 )&0x00ffffff) | dst_alpha;
	}
	inline __m128i operator()( __m128i ms11, __m128i ms21, const tjs_uint8 *rule ) const {
		// calculate addr
		__m128i a1 = ms11;
		a1  = _mm_srli_epi32( a1, 24 );
		__m128i a2 = ms21;
		a2 = _mm_srli_epi32( a2, 24 );
		__m128i mopa = _mm_cvtsi32_si128( table_[rule[0]] );
		__m128i mopa2 = _mm_cvtsi32_si128( table_[rule[1]] );
		mopa = _mm_unpacklo_epi32( mopa, mopa2 );	// 0 0 1 2
		mopa2 = _mm_cvtsi32_si128( table_[rule[2]] );
		__m128i mopa3 = _mm_cvtsi32_si128( table_[rule[3]] );
		mopa2 = _mm_unpacklo_epi32( mopa2, mopa3 );	// 0 0 3 4
		mopa = _mm_unpacklo_epi64( mopa, mopa2 );	// 1 2 3 4

		a2 = _mm_mullo_epi16( a2, mopa );	// a2 * opa
		__m128i mask = _mm_set1_epi32( 0x0000ff00 );
		a2 = _mm_and_si128( a2, mask );
#if 0
		mask = _mm_srli_epi32( mask, 8 );	// 0x00ff
		mopa = _mm_xor_si128( mopa, mask );	// 255-opa
		a1 = _mm_mullo_epi16( a1, mopa );	// a1 * opa
		// 255-opa だと誤差が大きめだけど、速い
#else
		__m128i invopa = _mm_set1_epi32( 256 );
		invopa = _mm_sub_epi16( invopa, mopa );
		a1 = _mm_mullo_epi16( a1, invopa );	// a1 * opa
#endif
		a1 = _mm_srli_epi32( a1, 8 );	// a1 >>= 8
		a1 = _mm_or_si128( a1, a2 );	// addr = (a2*opa & 0xff00) + (a1*(256-opa) >> 8);

		// lower 2pixel
		tjs_uint32 addr = _mm_cvtsi128_si32( a1 );
		tjs_uint32 opa = TVPOpacityOnOpacityTable[addr];
		mopa = _mm_cvtsi32_si128( opa );
		a1 = _mm_shuffle_epi32( a1, _MM_SHUFFLE( 0, 3, 2, 1 ) );
		addr = _mm_cvtsi128_si32( a1 );
		opa = TVPOpacityOnOpacityTable[addr];
		mopa2 = _mm_cvtsi32_si128( opa );
		mopa = _mm_unpacklo_epi16( mopa, mopa2 );	// 0 0 0 0 0 0 1 2
		mopa = _mm_unpacklo_epi16( mopa, mopa );	// 0 0 0 0 1 1 2 2
		mopa = _mm_unpacklo_epi16( mopa, mopa );	// 1 1 1 1 2 2 2 2
		
		__m128i ms12 = ms11;	// src1
		__m128i ms22 = ms21;	// src2
		ms21 = _mm_unpacklo_epi8( ms21, zero_ );	// 00 s2 00 s2 00 s2 00 s2
		ms11 = _mm_unpacklo_epi8( ms11, zero_ );	// 00 s1 00 s1 00 s1 00 s1
		ms21 = _mm_sub_epi16( ms21, ms11 );		// s2 - s1
		ms21 = _mm_mullo_epi16( ms21, mopa );	// s2 *= table
		ms21 = _mm_srli_epi16( ms21, 8 );		// s2 >>= 8
		ms11 = _mm_add_epi8( ms11, ms21 );		// s1 += s2

		// higher 2pixel
		a1 = _mm_shuffle_epi32( a1, _MM_SHUFFLE( 0, 3, 2, 1 ) );
		addr = _mm_cvtsi128_si32( a1 );
		opa = TVPOpacityOnOpacityTable[addr];
		mopa = _mm_cvtsi32_si128( opa );
		a1 = _mm_shuffle_epi32( a1, _MM_SHUFFLE( 0, 3, 2, 1 ) );
		addr = _mm_cvtsi128_si32( a1 );
		opa = TVPOpacityOnOpacityTable[addr];
		mopa2 = _mm_cvtsi32_si128( opa );
		mopa = _mm_unpacklo_epi16( mopa, mopa2 );	// 0 0 0 0 0 0 1 2
		mopa = _mm_unpacklo_epi16( mopa, mopa );	// 0 0 0 0 1 1 2 2
		mopa = _mm_unpacklo_epi16( mopa, mopa );	// 1 1 1 1 2 2 2 2
		
		ms22 = _mm_unpackhi_epi8( ms22, zero_ );	// 00 s2 00 s2 00 s2 00 s2
		ms12 = _mm_unpackhi_epi8( ms12, zero_ );	// 00 s1 00 s1 00 s1 00 s1
		ms22 = _mm_sub_epi16( ms22, ms12 );		// s2 - s1
		ms22 = _mm_mullo_epi16( ms22, mopa );	// s2 *= table
		ms22 = _mm_srli_epi16( ms22, 8 );		// s2 >>= 8
		ms12 = _mm_add_epi8( ms12, ms22 );		// s1 += s2

		ms11 = _mm_packus_epi16( ms11, ms12 );

		// calculate Negative Mul
		a1 = _mm_shuffle_epi32( a1, _MM_SHUFFLE( 0, 3, 2, 1 ) );	// for dopa
		mask = _mm_set1_epi32( 0x0000ffff );
		a1 = _mm_xor_si128( a1, mask );	// (a = 255-a, b = 255-b) : ^=xor
		__m128i mtmp = a1;

		a1 = _mm_slli_epi32( a1, 8 );		// 00ff|ff00	上位 << 8
		mtmp = _mm_slli_epi16( mtmp, 8 );	// 0000|ff00	下位 << 8
		mtmp = _mm_slli_epi32( mtmp, 8 );	// 00ff|0000
		a1 = _mm_mullo_epi16( a1, mtmp );	// 上位で演算、下位部分はごみ
		a1 = _mm_srli_epi32( a1, 16 );		// addr >> 16 | 下位を捨てる
		a1 = _mm_andnot_si128( a1, mask );	// ~addr&0x0000ffff
		a1 = _mm_srli_epi16( a1, 8 );		// addr>>8
		a1 = _mm_slli_epi32( a1, 24 );		// アルファ位置へ

		const __m128i colormask = _mm_set1_epi32(0x00ffffff);
		ms11 = _mm_and_si128( ms11, colormask );
		return _mm_or_si128( ms11, a1 );
	}
};


// あらかじめ 16bit * 2 でパックしておく
void TVPInitUnivTransBlendTable_sse2_c(tjs_uint32 *table, tjs_int phase, tjs_int vague)
{
	tjs_int i = 0;
	tjs_int phasemax = phase;
	phase -= vague;
	for(; i < phase; i++ ) table[i] = (255<<16)|(255);
	if( phasemax > 256 ) phasemax = 256;
	for(;i < phasemax; i++ ) {
		short tmp = (255-(( i - phase )*255 / vague));
		if(tmp<0) tmp = 0;
		if(tmp>255) tmp = 255;
		table[i] = (tmp << 16) | (tmp & 0xffff);
	}
	for(;i < 256; i++ ) table[i] = 0;
}
void TVPInitUnivTransBlendTable_d_sse2_c(tjs_uint32 *table, tjs_int phase, tjs_int vague)
{
	tjs_int i = 0;
	tjs_int phasemax = phase;
	phase -= vague;
	for(; i < phase; i++ ) table[i] = 255;
	if( phasemax > 256 ) phasemax = 256;
	for(;i < phasemax; i++ ) {
		short tmp = (255-(( i - phase )*255 / vague));
		if(tmp<0) tmp = 0;
		if(tmp>255) tmp = 255;
		table[i] = tmp & 0xffff;
	}
	for(;i < 256; i++ ) table[i] = 0;
}

template<typename func_t>
static inline void sse2_univ_trans(tjs_uint32 *dest, const tjs_uint32 *src1, const tjs_uint32 *src2, const tjs_uint8 *rule, const tjs_uint32 *table, tjs_int len) {
	if( len <= 0 ) return;

	func_t func(table);

	tjs_int count = (tjs_int)(((unsigned)dest)&0xF);
	if( count ) {
		count = (16 - count)>>2;
		count = count > len ? len : count;
		tjs_uint32* limit = dest + count;
		while( dest < limit ) {
			*dest = func( *src1, *src2, *rule );
			dest++; src1++; src2++, rule++;
		}
		len -= count;
	}
	tjs_uint32 rem = (len>>2)<<2;
	tjs_uint32* limit = dest + rem;
	if( ((((unsigned)src1)&0xF) == 0) && (((unsigned)src2)&0xF) == 0 ) {
		while( dest < limit ) {
			__m128i ms11 = _mm_load_si128( (__m128i const*)src1 );	// src1
			__m128i ms21 = _mm_load_si128( (__m128i const*)src2 );	// src2
			_mm_store_si128( (__m128i*)dest, func(ms11, ms21, rule) );
			src1 += 4; src2 += 4; rule += 4; dest += 4;
		}
	} else {
		while( dest < limit ) {
			__m128i ms11 = _mm_loadu_si128( (__m128i const*)src1 );	// src1
			__m128i ms21 = _mm_loadu_si128( (__m128i const*)src2 );	// src2
			_mm_store_si128( (__m128i*)dest, func(ms11, ms21, rule) );
			src1 += 4; src2 += 4; rule += 4; dest += 4;
		}
	}
	limit += (len-rem);
	while( dest < limit ) {
		*dest = func( *src1, *src2, *rule );
		dest++; src1++; src2++, rule++;
	}
}

template<typename func_t>
static inline void sse2_univ_trans_switch(tjs_uint32 *dest, const tjs_uint32 *src1, const tjs_uint32 *src2, const tjs_uint8 *rule, const tjs_uint32 *table, tjs_int len, tjs_int src1lv, tjs_int src2lv) {
	if( len <= 0 ) return;

	func_t func(table);
	tjs_int count = (tjs_int)(((unsigned)dest)&0xF);
	if( count ) {
		count = (16 - count)>>2;
		count = count > len ? len : count;
		tjs_uint32* limit = dest + count;
		while( dest < limit ) {
			tjs_int opa = *rule;
			if( opa >= src1lv ) {
				*dest = *src1;
				rule++; src1++; src2++; dest++;
			} else if(opa < src2lv) {
				*dest = *src2;
				rule++; src1++; src2++; dest++;
			} else {
				*dest = func( *src1, *src2, *rule );
				dest++; src1++; src2++, rule++;
			}
		}
		len -= count;
	}
	tjs_uint32 rem = (len>>2)<<2;
	tjs_uint32* limit = dest + rem;
	const __m128i msrc1lv = _mm_set1_epi32(src1lv);
	const __m128i msrc2lv = _mm_set1_epi32(src2lv);
	const __m128i not = _mm_set1_epi32(0xffffffff);
	while( dest < limit ) {
		tjs_uint32 r = *(tjs_uint32*)rule;
		__m128i mr = _mm_cvtsi32_si128( r );
		mr = _mm_unpacklo_epi8( mr, func.zero_ );	// 00 00 00 00 0r 0r 0r 0r
		mr = _mm_unpacklo_epi16( mr, func.zero_ );	// 00 0r 00 0r 00 0r 00 0r
		__m128i opa1 = mr;
		opa1 = _mm_cmplt_epi32( opa1, msrc1lv );	// opa < src1lv ? 0xff : 0
		__m128i md = opa1;
		__m128i ms11 = _mm_loadu_si128( (__m128i const*)src1 );	// src1
		md = _mm_andnot_si128( md, ms11 );	// opa >= src1lv : src1
		if( _mm_movemask_epi8(opa1) != 0 ) {	// opa < src1lv
			__m128i ms21 = _mm_loadu_si128( (__m128i const*)src2 );	// src2
			opa1 = _mm_xor_si128( opa1, not );	// 反転 opa >= src1lv : src1
			__m128i opa2 = mr;
			opa2 = _mm_cmplt_epi32( mr, msrc2lv );	// opa < src2lv ? 0xff : 0
			opa1 = _mm_or_si128( opa1, opa2 );	// (opa >= src1lv) || (opa < src2lv)
			opa2 = _mm_and_si128( opa2, ms21 );
			md = _mm_or_si128( md, opa2 );	// opa < src2lv : src2
			if( _mm_movemask_epi8(opa1) != 0xffff ) {
				__m128i blend = func(ms11, ms21, rule);
				opa1 = _mm_andnot_si128( opa1, blend );	// !((opa >= src1lv) || (opa < src2lv)) : blend
				md = _mm_or_si128( md, opa1 );
			}
		}
		_mm_store_si128( (__m128i*)dest, md );
		src1 += 4; src2 += 4; rule += 4; dest += 4;
	}
	
	limit += (len-rem);
	while( dest < limit ) {
		tjs_int opa = *rule;
		if( opa >= src1lv ) {
			*dest = *src1;
			rule++; src1++; src2++; dest++;
		} else if(opa < src2lv) {
			*dest = *src2;
			rule++; src1++; src2++; dest++;
		} else {
			*dest = func( *src1, *src2, *rule );
			dest++; src1++; src2++, rule++;
		}
	}
}

void TVPUnivTransBlend_sse2_c(tjs_uint32 *dest, const tjs_uint32 *src1, const tjs_uint32 *src2, const tjs_uint8 *rule, const tjs_uint32 *table, tjs_int len)
{
	sse2_univ_trans<sse2_univ_trans_blend_func>(dest,src1,src2,rule,table,len);
}
void TVPUnivTransBlend_switch_sse2_c(tjs_uint32 *dest, const tjs_uint32 *src1, const tjs_uint32 *src2, const tjs_uint8 *rule, const tjs_uint32 *table, tjs_int len, tjs_int src1lv, tjs_int src2lv)
{
	sse2_univ_trans_switch<sse2_univ_trans_blend_func>(dest,src1,src2,rule,table,len,src1lv,src2lv);
}

void TVPUnivTransBlend_d_sse2_c(tjs_uint32 *dest, const tjs_uint32 *src1, const tjs_uint32 *src2, const tjs_uint8 *rule, const tjs_uint32 *table, tjs_int len)
{
	sse2_univ_trans<sse2_univ_trans_d_blend_func>(dest,src1,src2,rule,table,len);
}
void TVPUnivTransBlend_switch_d_sse2_c(tjs_uint32 *dest, const tjs_uint32 *src1, const tjs_uint32 *src2, const tjs_uint8 *rule, const tjs_uint32 *table, tjs_int len, tjs_int src1lv, tjs_int src2lv)
{
	sse2_univ_trans_switch<sse2_univ_trans_d_blend_func>(dest,src1,src2,rule,table,len,src1lv,src2lv);
}

