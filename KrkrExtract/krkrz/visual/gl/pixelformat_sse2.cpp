

#include "simd_def_x86x64.h"
#include "tjsTypes.h"
#include "tvpgl.h"
#include "tvpgl_ia32_intf.h"

void TVPConvert24BitTo32Bit_sse2_c(tjs_uint32 *dest, const tjs_uint8 *buf, tjs_int len) {
	const __m128i alphamask( _mm_set1_epi32( 0xff000000 ) );
	const __m128i lomask( _mm_cvtsi32_si128( 0xffffffff ) );
	__m128i himask = lomask;
	himask = _mm_slli_epi64( himask, 32 );

	tjs_uint32 rem = (len>>4)<<4;
	tjs_uint32* limit = dest + rem;
	while( dest < limit ) {
		// b5 r4 g4 b4 | r3 g3 b3 r2 | g2 b2 r1 g1 | b1 r0 g0 b0 : m0
		__m128i m0 = _mm_loadu_si128((__m128i const*)(buf+0));
		__m128i md0 = m0;
		__m128i md1 = m0;
		md0 = _mm_and_si128( md0, lomask );	// 00 00 00 00 b1 r0 g0 b0
		md1 = _mm_slli_epi64( md1, 8 );	// << 1*8
		// r4 g4 b4 r3 | g3 b3 r2 00 | b2 r1 g1 b1 | r0 g0 b0 00
		md1 = _mm_and_si128( md1, himask );	// b2 r1 g1 b1 00 00 00 00
		md0 = _mm_or_si128( md0, md1 );		// b2 r1 g1 b1 b1 r0 g0 b0
		__m128i md2 = m0;
		md2 = _mm_srli_si128( md2, 5 );	// >> 5*8
		// 00 00 00 00 | 00 b5 r4 g4 | b4 r3 g3 b3 | r2 g2 b2 r1 : m0
		md1 = md2;
		md2 = _mm_and_si128( md2, himask );	// b4 r3 g3 b3 00 00 00 00
		md1 = _mm_srli_epi32( md1, 8 );	// >> 1*8 : 00 r2 g2 b2
		md1 = _mm_and_si128( md1, lomask );
		md1 = _mm_or_si128( md1, md2 );	// b4 r3 g3 b3 00 r2 g2 b2
		md0 = _mm_unpacklo_epi64( md0, md1 );
		md0 = _mm_or_si128( md0, alphamask );
		_mm_storeu_si128((__m128i *)&dest[0], md0 );	// 0 - 3
		// gA bA r9 g9 | b9 r8 g8 b8 | r7 g7 b7 r6 | g6 b6 r5 g5 : m1
		__m128i m1 = _mm_loadu_si128((__m128i const*)(buf+16));
		m0 = _mm_srli_si128( m0, 12 );	// >> 12*8 : b5 r4 g4 b4
		md1 = m0;
		md0 = m1;
		md0 = _mm_slli_si128( md0, 5 );	// << 5*8
		// r8 g8 b8 r7 | g7 b7 r6 g6 | b6 r5 g5 00 | 00 00 00 00
		md1 = _mm_slli_epi64( md1, 8 ); // 00 00 00 b5 | r4 g4 b4 00
		md0 = _mm_or_si128( md0, md1 );
		md0 = _mm_and_si128( md0, himask );
		md0 = _mm_or_si128( md0, m0 );	// b6 r5 g5 b5 | b5 r4 g4 b4
		md2 = m1;
		md1 = m1;
		md2 = _mm_srli_epi64( md2, 16 );	// 00 00 r7 g7 | b7 r6 g6 b6
		md2 = _mm_and_si128( md2, lomask );
		md1 = _mm_srli_epi64( md1, 8 );	// 00 r7 g7 b7 | r6 g6 b6 r5
		md1 = _mm_and_si128( md1, himask );
		md1 = _mm_or_si128( md1, md2 );
		md0 = _mm_unpacklo_epi64( md0, md1 );
		md0 = _mm_or_si128( md0, alphamask );
		_mm_storeu_si128((__m128i *)&dest[4], md0 );	// 4 - 7
		m1 = _mm_srli_si128( m1, 8 );	// >> 8*8 gA bA r9 g9 | b9 r8 g8 b8
		md0 = m1;
		md1 = m1;
		md0 = _mm_slli_epi64( md0, 8 );	// << 8 bA r9 g9 b9 | r8 g8 b8 00
		md0 = _mm_and_si128( md0, himask );
		md1 = _mm_and_si128( md1, lomask );
		md0 = _mm_or_si128( md0, md1 );	// bA r9 g9 b9 | b9 r8 g8 b8
		// rF gF bF rE | gE bE rD gD | bD rC gC bC | rB gB bB bA : m2
		__m128i m2 = _mm_loadu_si128((__m128i const*)(buf+32));
		m1 = _mm_srli_epi64( m1, 48 );	// >> 48 00 00 00 00 | 00 00 gA bA
		md1 = m2;
		md2 = m2;
		md1 = _mm_slli_si128( md1, 2 );	// << 2*8
		md1 = _mm_or_si128( md1, m1 );
		md1 = _mm_and_si128( md1, lomask );
		md2 = _mm_slli_epi64( md2, 24 );	// << 24
		md2 = _mm_and_si128( md2, himask );
		md1 = _mm_or_si128( md1, md2 );	// bC rB gB bB | bB bA gA bA
		md0 = _mm_unpacklo_epi64( md0, md1 );
		md0 = _mm_or_si128( md0, alphamask );
		_mm_storeu_si128((__m128i *)&dest[8], md0 );	// 8 - B
		md0 = m2;
		md0 = _mm_srli_si128( md0, 4 );	// >> 4*8
		md1 = md0;
		md1 = _mm_slli_epi64( md1, 8 );	// << 8
		md1 = _mm_and_si128( md1, himask );
		md0 = _mm_and_si128( md0, lomask );
		md0 = _mm_or_si128( md0, md1 );	// bE rD gD bD | bD rC gC bC
		md2 = m2;
		md2 = _mm_srli_si128( md2, 10 );	// >> 10*8
		m2 = _mm_srli_si128( m2, 9 );	// >> 10*8
		m2 = _mm_and_si128( m2, himask );
		md2 = _mm_and_si128( md2, lomask );
		md2 = _mm_or_si128( md2, m2 );
		md0 = _mm_unpacklo_epi64( md0, md2 );
		md0 = _mm_or_si128( md0, alphamask );
		_mm_storeu_si128((__m128i *)&dest[12], md0 );	// C - F
		buf += 48; dest += 16;
	}
	limit += (len-rem);
	while( dest < limit ) {
		*dest = 0xff000000 | (buf[2]<<16) | (buf[1]<<8) | buf[0];
		buf+=3; dest++;
	}
}


// SSSE3
void TVPConvert24BitTo32Bit_ssse3_c(tjs_uint32 *dest, const tjs_uint8 *buf, tjs_int len) {
	const __m128i alphamask( _mm_set1_epi32( 0xff000000 ) );
	__m128i mask;
	mask.m128i_u8[0] = 0x00;
	mask.m128i_u8[1] = 0x01;
	mask.m128i_u8[2] = 0x02;
	mask.m128i_u8[3] = 0x80;
	mask.m128i_u8[4] = 0x03;
	mask.m128i_u8[5] = 0x04;
	mask.m128i_u8[6] = 0x05;
	mask.m128i_u8[7] = 0x80;
	mask.m128i_u8[8] = 0x06;
	mask.m128i_u8[9] = 0x07;
	mask.m128i_u8[10] = 0x08;
	mask.m128i_u8[11] = 0x80;
	mask.m128i_u8[12] = 0x09;
	mask.m128i_u8[13] = 0x0A;
	mask.m128i_u8[14] = 0x0B;
	mask.m128i_u8[15] = 0x80;

	// 16単位
	tjs_uint32 rem = (len>>4)<<4;
	tjs_uint32* limit = dest + rem;
	while( dest < limit ) {
		// b5 r4 g4 b4 | r3 g3 b3 r2 | g2 b2 r1 g1 | b1 r0 g0 b0 : m0
		// gA bA r9 g9 | b9 r8 g8 b8 | r7 g7 b7 r6 | g6 b6 r5 g5 : m1
		// rF gF bF rE | gE bE rD gD | bD rC gC bC | rB gB bB bA : m2
		__m128i m0 = _mm_loadu_si128((__m128i const*)(buf+0));
		__m128i m1 = _mm_loadu_si128((__m128i const*)(buf+16));
		__m128i m2 = _mm_loadu_si128((__m128i const*)(buf+32));
		__m128i md0 = m0;
		md0 = _mm_shuffle_epi8( md0, mask );
		md0 = _mm_or_si128( md0, alphamask );
		_mm_storeu_si128((__m128i *)&dest[0], md0 );	// 0 - 3
		m0 = _mm_srli_si128( m0, 12 );	// >> 12*8
		__m128i md1 = m1;
		md1 = _mm_slli_si128( md1, 4 );	// << 4*8
		md1 = _mm_or_si128( md1, m0 );
		md1 = _mm_shuffle_epi8( md1, mask );
		md1 = _mm_or_si128( md1, alphamask );
		_mm_storeu_si128((__m128i *)&dest[4], md1 );	// 4 - 7
		m1 = _mm_srli_si128( m1, 8 );	// >> 8*8
		__m128i md2 = m2;
		md2 = _mm_slli_si128( md2, 8 );	// << 8*8
		md2 = _mm_or_si128( md2, m1 );
		md2 = _mm_shuffle_epi8( md2, mask );
		md2 = _mm_or_si128( md2, alphamask );
		_mm_storeu_si128((__m128i *)&dest[8], md2 );	// 8 - B
		m2 = _mm_srli_si128( m2, 4 );	// >> 4*8
		m2 = _mm_shuffle_epi8( m2, mask );
		m2 = _mm_or_si128( m2, alphamask );
		_mm_storeu_si128((__m128i *)&dest[12], m2 );	// A - F
		buf += 48; dest += 16;
	}
	limit += (len-rem);
	while( dest < limit ) {
		*dest = 0xff000000 | (buf[2]<<16) | (buf[1]<<8) | buf[0];
		buf+=3; dest++;
	}
}

