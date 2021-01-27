
#include "tjsTypes.h"
#include "tvpgl.h"
#include "tvpgl_ia32_intf.h"
#include "simd_def_x86x64.h"

#if 0	// 偏りが多い場合は速いが、通常ケースでは遅い
// MMX + SSE
tjs_int TVPTLG5DecompressSlide_sse_c( tjs_uint8 *out, const tjs_uint8 *in, tjs_int insize, tjs_uint8 *text, tjs_int initialr ) {
	tjs_int r = initialr;
	tjs_uint flags = 0;
	const tjs_uint8 *inlim = in + insize;
	const __m64 mask = _mm_set1_pi32(0xffffffff);
	while(in < inlim ) {
		if(((flags >>= 1) & 256) == 0) {
			flags = in[0];
			in++;
			if( flags == 0 && in < (inlim-8) ) {	// copy 8byte
				__m64 c = *(__m64 const*)in;
				*(__m64 *)out = c;
				*(__m64 *)&text[r] = c;
				r += 8;
				if( r > 4095 ) {
					r &= 0x0FFF;
					c = _mm_srli_pi64( c, (8 - r)*8 );
					__m64 t = *(__m64 const*)text;
					__m64 m = mask;
					m = _mm_srli_pi64( m, (8 - r)*8 );
					_mm_maskmove_si64( c, m, (char*)text );
				}
				in += 8;
				out += 8;
				continue;
			} else {
				flags |= 0xff00;
			}
		}
		if(flags & 1) {
			tjs_int mpos = in[0] | ((in[1] & 0xf) << 8);
			tjs_int mlen = (in[1] & 0xf0) >> 4;
			in += 2;
			mlen += 3;
			if(mlen == 18) {
				mlen += in[0];
				in++;
			}
			while( mlen ) {
				__m64 c = *(__m64 const*)&text[mpos];
				*(__m64 *)out = c;
				if( mlen < 8 ) {
					if( (4096-mpos) < mlen ) {
						tjs_int mrem = (4096-mpos);
						out += mrem;
						mlen -= mrem;
						__m64 m = mask;
						m = _mm_srli_pi64( m, (8-mrem)*8 );
						_mm_maskmove_si64( c, m, (char*)&text[r] );
						r += mrem;
						if( r > 4095 ) {
							r &= 0x0fff;
							c = _mm_srli_pi64( c, (mrem-r)*8 );
							m = mask;
							m = _mm_srli_pi64( m, (8-r)*8 );
							_mm_maskmove_si64( c, m, (char*)text );
						}
						mpos = 0;
					} else {
						out += mlen;
						__m64 m = mask;
						m = _mm_srli_pi64( m, (8-mlen)*8 );
						_mm_maskmove_si64( c, m, (char*)&text[r] );
						r += mlen;
						if( r > 4095 ) {
							r &= 0x0fff;
							c = _mm_srli_pi64( c, (mlen-r)*8 );
							m = mask;
							m = _mm_srli_pi64( m, (8-r)*8 );
							_mm_maskmove_si64( c, m, (char*)text );
						}
						mpos += mlen;
						mpos &= 0x0fff;
						mlen = 0;
					}
				} else if( (4096-mpos) < 8 ) {
					tjs_int mrem = (4096-mpos);
					out += mrem;
					mlen -= mrem;
					__m64 m = mask;
					m = _mm_srli_pi64( m, (8-mrem)*8 );
					_mm_maskmove_si64( c, m, (char*)&text[r] );
					r += mrem;
					if( r > 4095 ) {
						r &= 0x0fff;
						c = _mm_srli_pi64( c, (mrem-r)*8 );
						m = mask;
						m = _mm_srli_pi64( m, (8-r)*8 );
						_mm_maskmove_si64( c, m, (char*)text );
					}
					mpos = 0;
				} else {
					out += 8;
					mlen -= 8;
					*(__m64*)&text[r] = c;
					r += 8;
					if( r > 4095 ) {
						r &= 0x0fff;
						c = _mm_srli_pi64( c, (8-r)*8 );
						__m64 m = mask;
						m = _mm_srli_pi64( m, (8-r)*8 );
						_mm_maskmove_si64( c, m, (char*)text );
					}
					mpos += 8;
					mpos &= 0x0fff;
				}
			}
		} else {
			unsigned char c = in[0]; in++;
			out[0] = c; out++;
			text[r++] = c;
			r &= (4096 - 1);
		}
	}
	_mm_empty();
	return r;
}
#endif

tjs_int TVPTLG5DecompressSlide_sse2_c( tjs_uint8 *out, const tjs_uint8 *in, tjs_int insize, tjs_uint8 *text, tjs_int initialr ) {
	tjs_int r = initialr;
	tjs_uint flags = 0;
	const tjs_uint8 *inlim = in + insize;
	// text と out、in は、+16余分に確保して、はみ出してもいいようにしておく
	//const __m128i mask = _mm_set1_epi32(0xffffffff);
	const __m128i mask = _mm_set_epi32(0,0,0xffffffff,0xffffffff);
	while(in < inlim ) {
		if(((flags >>= 1) & 256) == 0) {
			flags = in[0] | 0xff00;
			in++;
#if 1
			if( flags == 0xff00 && r < (4096-8) && in < (inlim-8)  ) {	// copy 8byte
				__m128i c = _mm_loadl_epi64( (__m128i const*)in );
				_mm_storel_epi64( (__m128i *)out, c );
				_mm_storel_epi64( (__m128i *)&text[r], c );	// 末尾はみ出すのを気にせずコピー
				r += 8;
#if 0
				if( r > 4095 ) {
					r &= 0x0FFF;
					c = _mm_srli_epi64( c, (8 - r)*8 );
					__m128i t = _mm_loadl_epi64( (__m128i const*)text );
					__m128i m = mask;
					m = _mm_srli_epi64( m, (8 - r)*8 );
					_mm_maskmoveu_si128( c, m, (char*)text );
				}
#endif
				in += 8;
				out += 8;
				flags = 0;
				continue;
			}
#endif
		}
		if(flags & 1) {
			tjs_int mpos = in[0] | ((in[1] & 0xf) << 8);
			tjs_int mlen = (in[1] & 0xf0) >> 4;
			in += 2;
			mlen += 3;
			if(mlen == 18) {
				mlen += in[0]; in++;
				// 数バイトの細切れだと遅いと思うけど、そうじゃなければ速いかな？
				// 8バイト単位 128bitシフトはimmでないといけないので、64bitごとに処理
				// 頑張れば16バイト単位でも行けそうだけど、分岐が多くなりそうなので
				if( (mpos+mlen) < 4096 && (r+mlen) < 4096 ) {
					// 末尾気にせずコピーしていい時は16byte単位でまとめてコピー
					tjs_int count = mlen >> 4;
					while( count-- ) {
						__m128i c = _mm_loadu_si128( (__m128i const*)&text[mpos] );
						_mm_storeu_si128( (__m128i *)out, c );
						_mm_storeu_si128( (__m128i *)&text[r], c );
						mpos += 16; r += 16; out += 16;
					}
					mlen &= 0x0f;	// 余り
					while(mlen--) {
						out[0] = text[r++] = text[mpos++]; out++;
					}
					continue;
				}
#if 0
				while(mlen--) {
					out[0] = text[r++] = text[mpos++]; out++;
					mpos &= 0x0fff;
					r &= 0x0fff;
				}
#else
				while( mlen ) {
					__m128i c = _mm_loadl_epi64( (__m128i const*)&text[mpos] );
					_mm_storeu_si128( (__m128i *)out, c );
					if( mlen < 8 ) {
						if( (4096-mpos) < mlen ) {
							tjs_int mrem = (4096-mpos);
							out += mrem;
							mlen -= mrem;
							__m128i m = mask;
							m = _mm_srli_epi64( m, (8-mrem)*8 );
							_mm_maskmoveu_si128( c, m, (char*)&text[r] );
							r += mrem;
							if( r > 4095 ) {
								r &= 0x0fff;
								c = _mm_srli_epi64( c, (mrem-r)*8 );
								m = mask;
								m = _mm_srli_epi64( m, (8-r)*8 );
								_mm_maskmoveu_si128( c, m, (char*)text );
							}
							mpos = 0;
						} else {
							out += mlen;
							__m128i m = mask;
							m = _mm_srli_epi64( m, (8-mlen)*8 );
							_mm_maskmoveu_si128( c, m, (char*)&text[r] );
							r += mlen;
							if( r > 4095 ) {
								r &= 0x0fff;
								c = _mm_srli_epi64( c, (mlen-r)*8 );
								m = mask;
								m = _mm_srli_epi64( m, (8-r)*8 );
								_mm_maskmoveu_si128( c, m, (char*)text );
							}
							mpos += mlen;
							mpos &= 0x0fff;
							mlen = 0;
						}
					} else if( (4096-mpos) < 8 ) {
						tjs_int mrem = (4096-mpos);
						out += mrem;
						mlen -= mrem;
						__m128i m = mask;
						m = _mm_srli_epi64( m, (8-mrem)*8 );
						_mm_maskmoveu_si128( c, m, (char*)&text[r] );
						r += mrem;
						if( r > 4095 ) {
							r &= 0x0fff;
							c = _mm_srli_epi64( c, (mrem-r)*8 );
							m = mask;
							m = _mm_srli_epi64( m, (8-r)*8 );
							_mm_maskmoveu_si128( c, m, (char*)text );
						}
						mpos = 0;
					} else {
						out += 8;
						mlen -= 8;
						_mm_storel_epi64( (__m128i *)&text[r], c );
						r += 8;
						if( r > 4095 ) {
							r &= 0x0fff;
							c = _mm_srli_epi64( c, (8-r)*8 );
							__m128i m = mask;
							m = _mm_srli_epi64( m, (8-r)*8 );
							_mm_maskmoveu_si128( c, m, (char*)text );
						}
						mpos += 8;
						mpos &= 0x0fff;
					}
				}
#endif
			} else {
				while(mlen--) {
					out[0] = text[r++] = text[mpos++]; out++;
					mpos &= 0x0fff;
					r &= 0x0fff;
				}
			}
		} else {
			unsigned char c = in[0]; in++;
			out[0] = c; out++;
			text[r++] = c;
			r &= 0x0fff;
		}
	}
	return r;
}

void TVPTLG5ComposeColors3To4_sse2_c(tjs_uint8 *outp, const tjs_uint8 *upper, tjs_uint8 * const * buf, tjs_int width) {
	tjs_int len = (width>>2)<<2;
	// 4つずつ処理
	tjs_uint32* b0 = (tjs_uint32*)buf[0];
	tjs_uint32* b1 = (tjs_uint32*)buf[1];
	tjs_uint32* b2 = (tjs_uint32*)buf[2];
	__m128i pc = _mm_setzero_si128();
	const __m128i opa(_mm_set1_epi32( 0xff000000 ));
	const __m128i zero(_mm_setzero_si128());
	tjs_int x = 0;
	for( ; x < len; x+=4 ) {
		__m128i c0 = _mm_cvtsi32_si128( *b0 );
		__m128i c1 = _mm_cvtsi32_si128( *b1 );
		__m128i c2 = _mm_cvtsi32_si128( *b2 );
		c0 = _mm_unpacklo_epi8( c0, zero );
		c0 = _mm_unpacklo_epi16( c0, zero );
		c2 = _mm_unpacklo_epi8( c2, zero );
		__m128i tmp = zero;
		tmp = _mm_unpacklo_epi16( tmp, c2 );
		c0 = _mm_or_si128( c0, tmp );		// 0 X 2 X
		c1 = _mm_unpacklo_epi8( c1, c1 );	// XXXXXXXX 0 0 1 1 2 2 3 3
		c1 = _mm_unpacklo_epi16( c1, c1 );	// 0000 1111 2222 3333
		c0 = _mm_add_epi8( c0, c1 );
		pc = _mm_add_epi8( pc, c0 );
		tmp = pc;
		c0 = _mm_srli_si128( c0, 4 );
		pc = _mm_add_epi8( pc, c0 );
		tmp = _mm_unpacklo_epi32( tmp, pc );
		c0 = _mm_srli_si128( c0, 4 );
		pc = _mm_add_epi8( pc, c0 );
		__m128i tmp2 = pc;
		c0 = _mm_srli_si128( c0, 4 );
		pc = _mm_add_epi8( pc, c0 );
		tmp2 = _mm_unpacklo_epi32( tmp2, pc );
		tmp = _mm_unpacklo_epi64( tmp, tmp2 );
		__m128i mup = _mm_loadu_si128( (__m128i const*)upper );
		tmp = _mm_add_epi8( tmp, mup );
		tmp = _mm_or_si128( tmp, opa );
		_mm_storeu_si128( (__m128i*)outp, tmp );
		b0++; b1++; b2++; outp += 4*4; upper += 4*4;
	}
	tjs_uint8* bb0 = (tjs_uint8*)b0;
	tjs_uint8* bb1 = (tjs_uint8*)b1;
	tjs_uint8* bb2 = (tjs_uint8*)b2;
	for( ; x < width; x++ ) {
		__m128i c0 = _mm_cvtsi32_si128( *bb0 );
		__m128i c1 = _mm_cvtsi32_si128( (*bb1)*0x00010101 );
		__m128i c2 = _mm_cvtsi32_si128( *bb2 << 16 );
		c0 = _mm_or_si128( c0, c2 );
		c0 = _mm_add_epi8( c0, c1 );
		pc = _mm_add_epi8( pc, c0 );
		__m128i mup = _mm_cvtsi32_si128(*(tjs_uint32*)upper);
		__m128i tmp = pc;
		tmp = _mm_add_epi8( tmp, mup );
		tmp = _mm_or_si128( tmp, opa );
		*(tjs_uint32*)outp = _mm_cvtsi128_si32(tmp);
		bb0++; bb1++; bb2++;
		outp += 4; upper += 4;
	}
}
void TVPTLG5ComposeColors4To4_sse2_c(tjs_uint8 *outp, const tjs_uint8 *upper, tjs_uint8 * const * buf, tjs_int width) {
	tjs_int len = (width>>2)<<2;
	// 4つずつ処理
	tjs_uint32* b0 = (tjs_uint32*)buf[0];
	tjs_uint32* b1 = (tjs_uint32*)buf[1];
	tjs_uint32* b2 = (tjs_uint32*)buf[2];
	tjs_uint32* b3 = (tjs_uint32*)buf[3];
	__m128i pc = _mm_setzero_si128();
	const __m128i zero(_mm_setzero_si128());
	tjs_int x = 0;
	for( ; x < len; x+=4 ) {
		__m128i c0 = _mm_cvtsi32_si128( *b0 );
		__m128i c1 = _mm_cvtsi32_si128( *b1 );
		__m128i c2 = _mm_cvtsi32_si128( *b2 );
		__m128i c3 = _mm_cvtsi32_si128( *b3 );
		c0 = _mm_unpacklo_epi8( c0, zero );
		c0 = _mm_unpacklo_epi16( c0, zero );
		c2 = _mm_unpacklo_epi8( c2, zero );
		__m128i tmp = zero;
		tmp = _mm_unpacklo_epi16( tmp, c2 );
		c3 = _mm_unpacklo_epi8( c3, zero );
		c3 = _mm_unpacklo_epi16( c3, zero );
		c3 = _mm_slli_epi32( c3, 24 );
		c0 = _mm_or_si128( c0, tmp );		// 0 X 2 X
		c0 = _mm_or_si128( c0, c3 );		// 0 X 2 3
		c1 = _mm_unpacklo_epi8( c1, c1 );	// XXXXXXXX 0 0 1 1 2 2 3 3
		c1 = _mm_unpacklo_epi16( c1, c1 );	// 0000 1111 2222 3333
		c1 = _mm_srli_epi32( c1, 8 );		// X000 X111 X222 X333
		c0 = _mm_add_epi8( c0, c1 );
		pc = _mm_add_epi8( pc, c0 );
		tmp = pc;
		c0 = _mm_srli_si128( c0, 4 );
		pc = _mm_add_epi8( pc, c0 );
		tmp = _mm_unpacklo_epi32( tmp, pc );
		c0 = _mm_srli_si128( c0, 4 );
		pc = _mm_add_epi8( pc, c0 );
		__m128i tmp2 = pc;
		c0 = _mm_srli_si128( c0, 4 );
		pc = _mm_add_epi8( pc, c0 );
		tmp2 = _mm_unpacklo_epi32( tmp2, pc );
		tmp = _mm_unpacklo_epi64( tmp, tmp2 );
		__m128i mup = _mm_loadu_si128( (__m128i const*)upper );
		tmp = _mm_add_epi8( tmp, mup );
		_mm_storeu_si128( (__m128i*)outp, tmp );
		b0++; b1++; b2++; b3++; outp += 4*4; upper += 4*4;
	}
	tjs_uint8* bb0 = (tjs_uint8*)b0;
	tjs_uint8* bb1 = (tjs_uint8*)b1;
	tjs_uint8* bb2 = (tjs_uint8*)b2;
	tjs_uint8* bb3 = (tjs_uint8*)b3;
	for( ; x < width; x++ ) {
		__m128i c0 = _mm_cvtsi32_si128( *bb0 );
		__m128i c1 = _mm_cvtsi32_si128( (*bb1)*0x00010101 );
		__m128i c2 = _mm_cvtsi32_si128( *bb2 << 16 );
		__m128i c3 = _mm_cvtsi32_si128( *bb3 << 24 );
		c0 = _mm_or_si128( c0, c2 );
		c0 = _mm_or_si128( c0, c3 );
		c0 = _mm_add_epi8( c0, c1 );
		pc = _mm_add_epi8( pc, c0 );
		__m128i mup = _mm_cvtsi32_si128(*(tjs_uint32*)upper);
		__m128i tmp = pc;
		tmp = _mm_add_epi8( tmp, mup );
		*(tjs_uint32*)outp = _mm_cvtsi128_si32(tmp);
		bb0++; bb1++; bb2++; bb3++;
		outp += 4; upper += 4;
	}
}

// TLG6 chroma/luminosity decoder
// This does reordering, color correlation filter, MED/AVG
#define TVP_TLG6_W_BLOCK_SIZE		8

static const __m128i g_mask( _mm_set1_epi32( 0x0000ff00 ) );
static const __m128i b_mask( _mm_set1_epi32( 0x000000ff ) );
static const __m128i r_mask( _mm_set1_epi32( 0x00ff0000 ) );
static const __m128i a_mask( _mm_set1_epi32( 0xff000000 ) );
static const __m128i g_d_mask( _mm_set1_epi32( 0x0000fe00 ) );
static const __m128i r_d_mask( _mm_set1_epi32( 0x00fe0000 ) );
static const __m128i b_d_mask( _mm_set1_epi32( 0x000000fe ) );
static const __m128i avg_mask_fe( _mm_set1_epi32( 0xfefefefe ) );
static const __m128i avg_mask_01( _mm_set1_epi32( 0x01010101 ) );

// ( 0, IB, IG, IR)
struct filter_insts_0_sse2 {
	inline __m128i operator()( __m128i a ) const {
		return a;
	}
};
// ( 1, IB+IG, IG, IR+IG)
struct filter_insts_1_sse2 {
	inline __m128i operator()( __m128i a ) const {
		__m128i b = a;
		__m128i c = a;
		b = _mm_and_si128( b, g_mask );
		c = _mm_and_si128( c, g_mask );
		b = _mm_slli_epi32( b, 8 );	// g << 8
		c = _mm_srli_epi32( c, 8 );	// g >> 8
		a = _mm_add_epi8( a, b );	// r+g
		a = _mm_add_epi8( a, c );	// b+g
		return a;
	}
};
// ( 2, IB, IG+IB, IR+IB+IG)
struct filter_insts_2_sse2 {
	inline __m128i operator()( __m128i a ) const {
		__m128i b = a;
		b = _mm_slli_epi32( b, 8 );		// << 8
		b = _mm_and_si128( b, g_mask );	// & 0x0000ff00
		a = _mm_add_epi8( a, b );		// g+b
		b = a;
		b = _mm_slli_epi32( b, 8 );		// << 8
		b = _mm_and_si128( b, r_mask );	// & 0x00ff0000
		a = _mm_add_epi8( a, b );		// r+g+b
		return a;
	}
};
// ( 3, IB+IR+IG, IG+IR, IR)
struct filter_insts_3_sse2 {
	inline __m128i operator()( __m128i a ) const {
		__m128i b = a;
		b = _mm_srli_epi32( b, 8 );		// >> 8
		b = _mm_and_si128( b, g_mask );	// & 0x0000ff00
		a = _mm_add_epi8( a, b );		// g+r
		b = a;
		b = _mm_srli_epi32( b, 8 );		// >> 8
		b = _mm_and_si128( b, b_mask );	// & 0x000000ff
		a = _mm_add_epi8( a, b );		// b+g+r
		return a;
	}
};
// ( 4, IB+IR, IG+IB+IR, IR+IB+IR+IG)
struct filter_insts_4_sse2 {
	inline __m128i operator()( __m128i a ) const {
		__m128i b = a;
		b = _mm_srli_epi32( b, 16 );	// >> 16
		b = _mm_and_si128( b, b_mask );	// & 0x000000ff
		a = _mm_add_epi8( a, b );		// b+r
		b = a;
		b = _mm_slli_epi32( b, 8 );		// << 8
		b = _mm_and_si128( b, g_mask );	// & 0x0000ff00
		a = _mm_add_epi8( a, b );		// g+b+r
		b = a;
		b = _mm_slli_epi32( b, 8 );		// << 8
		b = _mm_and_si128( b, r_mask );	// & 0x00ff0000
		a = _mm_add_epi8( a, b );		// r+g+b+r
		return a;
	}
};
// ( 5, IB+IR, IG+IB+IR, IR)
struct filter_insts_5_sse2 {
	inline __m128i operator()( __m128i a ) const {
		__m128i b = a;
		b = _mm_srli_epi32( b, 16 );
		b = _mm_and_si128( b, b_mask );
		a = _mm_add_epi8( a, b );
		b = a;
		b = _mm_slli_epi32( b, 8 );
		b = _mm_and_si128( b, g_mask );
		a = _mm_add_epi8( a, b );
		return a;
	}
};
// ( 6, IB+IG, IG, IR)
struct filter_insts_6_sse2 {
	inline __m128i operator()( __m128i a ) const {
		__m128i b = a;
		b = _mm_srli_epi32( b, 8 );
		b = _mm_and_si128( b, b_mask );
		a = _mm_add_epi8( a, b );
		return a;
	}
};
// ( 7, IB, IG+IB, IR)
struct filter_insts_7_sse2 {
	inline __m128i operator()( __m128i a ) const {
		__m128i b = a;
		b = _mm_slli_epi32( b, 8 );
		b = _mm_and_si128( b, g_mask );
		a = _mm_add_epi8( a, b );
		return a;
	}
};
// ( 8, IB, IG, IR+IG)
struct filter_insts_8_sse2 {
	inline __m128i operator()( __m128i a ) const {
		__m128i b = a;
		b = _mm_slli_epi32( b, 8 );
		b = _mm_and_si128( b, r_mask );
		a = _mm_add_epi8( a, b );
		return a;
	}
};
// ( 9, IB+IG+IR+IB, IG+IR+IB, IR+IB)
struct filter_insts_9_sse2 {
	inline __m128i operator()( __m128i a ) const {
		__m128i b = a;
		b = _mm_slli_epi32( b, 16 );
		b = _mm_and_si128( b, r_mask );
		a = _mm_add_epi8( a, b );
		b = a;
		b = _mm_srli_epi32( b, 8 );
		b = _mm_and_si128( b, g_mask );
		a = _mm_add_epi8( a, b );
		b = a;
		b = _mm_srli_epi32( b, 8 );
		b = _mm_and_si128( b, b_mask );
		a = _mm_add_epi8( a, b );
		return a;
	}
};
// (10, IB+IR, IG+IR, IR)
struct filter_insts_10_sse2 {
	inline __m128i operator()( __m128i a ) const {
		__m128i b = a;
		__m128i c = a;
		b = _mm_srli_epi32( b, 8 );
		c = _mm_srli_epi32( c, 16 );
		b = _mm_and_si128( b, g_mask );
		c = _mm_and_si128( c, b_mask );
		a = _mm_add_epi8( a, b );
		a = _mm_add_epi8( a, c );
		return a;
	}
};
// (11, IB, IG+IB, IR+IB)
struct filter_insts_11_sse2 {
	inline __m128i operator()( __m128i a ) const {
		__m128i b = a;
		__m128i c = a;
		b = _mm_slli_epi32( b, 8 );
		c = _mm_slli_epi32( c, 16 );
		b = _mm_and_si128( b, g_mask );
		c = _mm_and_si128( c, r_mask );
		a = _mm_add_epi8( a, b );
		a = _mm_add_epi8( a, c );
		return a;
	}
};
// (12, IB, IG+IR+IB, IR+IB)
struct filter_insts_12_sse2 {
	inline __m128i operator()( __m128i a ) const {
		__m128i b = a;
		b = _mm_slli_epi32( b, 16 );
		b = _mm_and_si128( b, r_mask );
		a = _mm_add_epi8( a, b );
		b = a;
		b = _mm_srli_epi32( b, 8 );
		b = _mm_and_si128( b, g_mask );
		a = _mm_add_epi8( a, b );
		return a;
	}
};
// (13, IB+IG, IG+IR+IB+IG, IR+IB+IG)
struct filter_insts_13_sse2 {
	inline __m128i operator()( __m128i a ) const {
		__m128i b = a;
		b = _mm_srli_epi32( b, 8 );
		b = _mm_and_si128( b, b_mask );
		a = _mm_add_epi8( a, b );
		b = a;
		b = _mm_slli_epi32( b, 16 );
		b = _mm_and_si128( b, r_mask );
		a = _mm_add_epi8( a, b );
		b = a;
		b = _mm_srli_epi32( b, 8 );
		b = _mm_and_si128( b, g_mask );
		a = _mm_add_epi8( a, b );
		return a;
	}
};
// (14, IB+IG+IR, IG+IR, IR+IB+IG+IR)
struct filter_insts_14_sse2 {
	inline __m128i operator()( __m128i a ) const {
		__m128i b = a;
		b = _mm_srli_epi32( b, 8 );
		b = _mm_and_si128( b, g_mask );
		a = _mm_add_epi8( a, b );
		b = a;
		b = _mm_srli_epi32( b, 8 );
		b = _mm_and_si128( b, b_mask );
		a = _mm_add_epi8( a, b );
		b = a;
		b = _mm_slli_epi32( b, 16 );
		b = _mm_and_si128( b, r_mask );
		a = _mm_add_epi8( a, b );
		return a;
	}
};
// (15, IB, IG+(IB<<1), IR+(IB<<1))
struct filter_insts_15_sse2 {
	inline __m128i operator()( __m128i a ) const {
		__m128i b = a;
		__m128i c = a;
		b = _mm_slli_epi32( b, 8+1 );		// b <<= (8+1)
		c = _mm_slli_epi32( c, 16+1 );		// c <<= (16+1)
		b = _mm_and_si128( b, g_d_mask );	// b &= 0x0000fe00
		c = _mm_and_si128( c, r_d_mask );	// c != 0x00fe0000
		a = _mm_add_epi8( a, b );		// a += b;
		a = _mm_add_epi8( a, c );		// a += c;
		return a;
	}
};
#if 1
// v でエラーが出るので、cを参照渡しとしておく、インライン化されて影響はないはず
static inline __m128i do_med_sse2( __m128i a, __m128i b, const __m128i& c, __m128i v ) {
	__m128i a2 = a;
	a = _mm_max_epu8( a, b );	// = max_a_b
	b = _mm_min_epu8( b, a2 );	// = min_a_b
	v = _mm_add_epi8( v, a );
	a = _mm_min_epu8( a, c );	// = max_a_b < c ? max_a_b : c
	v = _mm_add_epi8( v, b );
	a = _mm_max_epu8( a, b );	// = min_a_b < a ? a : min_a_b
	return _mm_sub_epi8( v, a );
}
#else
#define do_med( a, b, c, v ) \
	_mm_setzero_si128();{__m128i a2 = a; \
	a = _mm_max_epu8( a, b ); \
	b = _mm_min_epu8( b, a2 ); \
	v = _mm_add_epi8( v, a ); \
	a = _mm_min_epu8( a, c ); \
	v = _mm_add_epi8( v, b ); \
	a = _mm_max_epu8( a, b ); \
	p = _mm_sub_epi8( v, a );}
#endif
// v += max(a,b);
// v += max(a,b) < c ? max(a,b) : c
// v -= min(a,b) < a ? a : min(a,b)

// v += max(p,u);
// v += max(p,u) < up ? max(p,u) : up
// v -= min(p,u) < p ? p : min(p,u)

struct filter_forward_input_sse2 {
	inline __m128i first(tjs_uint32 *in) const {
		return _mm_loadu_si128( (__m128i const*)&in[0] );
	}
	inline __m128i second(tjs_uint32 *in) const {
		return _mm_loadu_si128( (__m128i const*)&in[4] );
	}
};
struct filter_backward_input_sse2 {
	inline __m128i first(tjs_uint32 *in) const {
		__m128i minput = _mm_loadu_si128( (__m128i const*)&in[4] );
		return _mm_shuffle_epi32( minput, _MM_SHUFFLE( 0, 1, 2, 3 ) );	// 逆転
	}
	inline __m128i second(tjs_uint32 *in) const {
		__m128i minput = _mm_loadu_si128( (__m128i const*)&in[0] );
		return _mm_shuffle_epi32( minput, _MM_SHUFFLE( 0, 1, 2, 3 ) );	// 逆転
	}
};

template<typename tfilter, typename tinput>
static inline void do_filter_med_sse2( tjs_uint32& inp, tjs_uint32& inup, tjs_uint32 *in, tjs_uint32 *prevline, tjs_uint32 *curline ) {
	tfilter filter;
	tinput input;
	__m128i p = _mm_cvtsi32_si128( inp );
	__m128i up = _mm_cvtsi32_si128( inup );

	__m128i minput = input.first( in );
	__m128i u = _mm_loadu_si128( (__m128i const*)&prevline[0] );
	minput = filter( minput );
	p = do_med_sse2( p, u, up, minput );
	up = u;
	curline[0] = _mm_cvtsi128_si32( p );

	minput = _mm_srli_si128( minput, 4 );	// >> 32
	u = _mm_srli_si128( u, 4 );	// >> 32
	p = do_med_sse2( p, u, up, minput );
	up = u;
	curline[1] = _mm_cvtsi128_si32( p );

	minput = _mm_srli_si128( minput, 4 );	// >> 32
	u = _mm_srli_si128( u, 4 );	// >> 32
	p = do_med_sse2( p, u, up, minput );
	up = u;
	curline[2] = _mm_cvtsi128_si32( p );

	minput = _mm_srli_si128( minput, 4 );	// >> 32
	u = _mm_srli_si128( u, 4 );	// >> 32
	p = do_med_sse2( p, u, up, minput );
	up = u;
	curline[3] = _mm_cvtsi128_si32( p );

	minput = input.second( in );
	u = _mm_loadu_si128( (__m128i const*)&prevline[4] );
	minput = filter( minput );
	p = do_med_sse2( p, u, up, minput );
	up = u;
	curline[4] = _mm_cvtsi128_si32( p );

	minput = _mm_srli_si128( minput, 4 );	// >> 32
	u = _mm_srli_si128( u, 4 );	// >> 32
	p = do_med_sse2( p, u, up, minput );
	up = u;
	curline[5] = _mm_cvtsi128_si32( p );

	minput = _mm_srli_si128( minput, 4 );	// >> 32
	u = _mm_srli_si128( u, 4 );	// >> 32
	p = do_med_sse2( p, u, up, minput );
	up = u;
	curline[6] = _mm_cvtsi128_si32( p );

	minput = _mm_srli_si128( minput, 4 );	// >> 32
	u = _mm_srli_si128( u, 4 );	// >> 32
	p = do_med_sse2( p, u, up, minput );
	curline[7] = _mm_cvtsi128_si32( p );

	inp = _mm_cvtsi128_si32( p );
	inup =  _mm_cvtsi128_si32( u );
}

// c = up : は使ってないので、要らない
// p, u, up, in
static inline __m128i do_avg_sse2( __m128i a, __m128i b, /*__m128i c, */__m128i v ) {
	a = _mm_avg_epu8( a, b );
	return _mm_add_epi8( a, v );
}
// TVP_TLG6_W_BLOCK_SIZE == 8
// SSE2 なら 2回なので、アンロールしてしまっていいかも
template<typename tfilter, typename tinput>
inline void do_filter_avg_sse2( tjs_uint32& inp, tjs_uint32& up, tjs_uint32 *in, tjs_uint32 *prevline, tjs_uint32 *curline ) {
	tfilter filter;
	tinput input;
	__m128i p = _mm_cvtsi32_si128( inp );
	__m128i minput = input.first( in );
	__m128i u = _mm_loadu_si128( (__m128i const*)&prevline[0] );
	minput = filter( minput );
	p = do_avg_sse2( p, u, minput );
	curline[0] = _mm_cvtsi128_si32( p );

	minput = _mm_srli_si128( minput, 4 );	// >> 32
	u = _mm_srli_si128( u, 4 );	// >> 32
	p = do_avg_sse2( p, u, minput );
	curline[1] = _mm_cvtsi128_si32( p );

	minput = _mm_srli_si128( minput, 4 );	// >> 32
	u = _mm_srli_si128( u, 4 );	// >> 32
	p = do_avg_sse2( p, u, minput );
	curline[2] = _mm_cvtsi128_si32( p );

	minput = _mm_srli_si128( minput, 4 );	// >> 32
	u = _mm_srli_si128( u, 4 );	// >> 32
	p = do_avg_sse2( p, u, minput );
	curline[3] = _mm_cvtsi128_si32( p );

	minput = input.second( in );
	u = _mm_loadu_si128( (__m128i const*)&prevline[4] );
	minput = filter( minput );
	p = do_avg_sse2( p, u, minput );
	curline[4] = _mm_cvtsi128_si32( p );

	minput = _mm_srli_si128( minput, 4 );	// >> 32
	u = _mm_srli_si128( u, 4 );	// >> 32
	p = do_avg_sse2( p, u, minput );
	curline[5] = _mm_cvtsi128_si32( p );

	minput = _mm_srli_si128( minput, 4 );	// >> 32
	u = _mm_srli_si128( u, 4 );	// >> 32
	p = do_avg_sse2( p, u, minput );
	curline[6] = _mm_cvtsi128_si32( p );

	minput = _mm_srli_si128( minput, 4 );	// >> 32
	u = _mm_srli_si128( u, 4 );	// >> 32
	p = do_avg_sse2( p, u, minput );
	curline[7] = _mm_cvtsi128_si32( p );

	inp = _mm_cvtsi128_si32( p );
	up =  _mm_cvtsi128_si32( u );
}
#if 0 // MMX(SSE) 使う版遅い
static inline __m64 do_med_sse( __m64 a, __m64 b, const __m64& c, __m64 v ) {
	__m64 a2 = a;
	a = _mm_max_pu8( a, b );	// = max_a_b
	b = _mm_min_pu8( b, a2 );	// = min_a_b
	v = _mm_add_pi8( v, a );
	a = _mm_min_pu8( a, c );	// = max_a_b < c ? max_a_b : c
	v = _mm_add_pi8( v, b );
	a = _mm_max_pu8( a, b );	// = min_a_b < a ? a : min_a_b
	return _mm_sub_pi8( v, a );
}
static inline __m64 do_avg_sse( __m64 a, __m64 b, __m64 v ) {
	a = _mm_avg_pu8( a, b );
	return _mm_add_pi8( a, v );
}
template<typename tfilter, typename tinput>
static inline void do_filter_med_sse2_sse( tjs_uint32& inp, tjs_uint32& inup, tjs_uint32 *in, tjs_uint32 *prevline, tjs_uint32 *curline ) {
	tfilter filter;
	tinput input;
	__m64 p = _mm_cvtsi32_si64(inp);
	__m64 up = _mm_cvtsi32_si64(inup);

	__m128i minput = input.first( in );
	minput = filter( minput );
	__m64 u = _mm_cvtsi32_si64( prevline[0] );
	__m64 i = _mm_movepi64_pi64(minput);
	p = do_med_sse( p, u, up, i );
	up = u;
	curline[0] = _mm_cvtsi64_si32( p );

	i = _mm_srli_pi64( i, 32 );
	u = _mm_cvtsi32_si64( prevline[1] );
	p = do_med_sse( p, u, up, i );
	up = u;
	curline[1] = _mm_cvtsi64_si32( p );

	minput = _mm_srli_si128( minput, 8 );	// >> 64
	i = _mm_movepi64_pi64(minput);
	u = _mm_cvtsi32_si64( prevline[2] );
	p = do_med_sse( p, u, up, i );
	up = u;
	curline[2] = _mm_cvtsi64_si32( p );
	
	i = _mm_srli_pi64( i, 32 );	// >> 32
	u = _mm_cvtsi32_si64( prevline[3] );
	p = do_med_sse( p, u, up, i );
	up = u;
	curline[3] = _mm_cvtsi64_si32( p );

	minput = input.second( in );
	minput = filter( minput );
	u = _mm_cvtsi32_si64( prevline[4] );
	i = _mm_movepi64_pi64(minput);
	p = do_med_sse( p, u, up, i );
	up = u;
	curline[4] = _mm_cvtsi64_si32( p );
	
	i = _mm_srli_pi64( i, 32 );
	u = _mm_cvtsi32_si64( prevline[5] );
	p = do_med_sse( p, u, up, i );
	up = u;
	curline[5] = _mm_cvtsi64_si32( p );

	minput = _mm_srli_si128( minput, 8 );	// >> 64
	i = _mm_movepi64_pi64(minput);
	u = _mm_cvtsi32_si64( prevline[6] );
	p = do_med_sse( p, u, up, i );
	up = u;
	curline[6] = _mm_cvtsi64_si32( p );
	
	i = _mm_srli_pi64( i, 32 );	// >> 32
	u = _mm_cvtsi32_si64( prevline[7] );
	p = do_med_sse( p, u, up, i );
	up = u;
	curline[7] = _mm_cvtsi64_si32( p );

	inp = _mm_cvtsi64_si32( p );
	inup =  _mm_cvtsi64_si32( u );
	_mm_empty();
}

template<typename tfilter, typename tinput>
static inline void do_filter_avg_sse2_sse( tjs_uint32& inp, tjs_uint32& inup, tjs_uint32 *in, tjs_uint32 *prevline, tjs_uint32 *curline ) {
	tfilter filter;
	tinput input;
	__m64 p = _mm_cvtsi32_si64(inp);
	__m128i minput = input.first( in );
	minput = filter( minput );
	__m64 u = _mm_cvtsi32_si64( prevline[0] );
	__m64 i = _mm_movepi64_pi64(minput);
	p = do_avg_sse( p, u, i );
	curline[0] = _mm_cvtsi64_si32( p );

	i = _mm_srli_pi64( i, 32 );
	u = _mm_cvtsi32_si64( prevline[1] );
	p = do_avg_sse( p, u, i );
	curline[1] = _mm_cvtsi64_si32( p );

	minput = _mm_srli_si128( minput, 8 );	// >> 64
	i = _mm_movepi64_pi64(minput);
	u = _mm_cvtsi32_si64( prevline[2] );
	p = do_avg_sse( p, u, i );
	curline[2] = _mm_cvtsi64_si32( p );
	
	i = _mm_srli_pi64( i, 32 );	// >> 32
	u = _mm_cvtsi32_si64( prevline[3] );
	p = do_avg_sse( p, u, i );
	curline[3] = _mm_cvtsi64_si32( p );

	minput = input.second( in );
	minput = filter( minput );
	u = _mm_cvtsi32_si64( prevline[4] );
	i = _mm_movepi64_pi64(minput);
	p = do_avg_sse( p, u, i );
	curline[4] = _mm_cvtsi64_si32( p );
	
	i = _mm_srli_pi64( i, 32 );
	u = _mm_cvtsi32_si64( prevline[5] );
	p = do_avg_sse( p, u, i );
	curline[5] = _mm_cvtsi64_si32( p );

	minput = _mm_srli_si128( minput, 8 );	// >> 64
	i = _mm_movepi64_pi64(minput);
	u = _mm_cvtsi32_si64( prevline[6] );
	p = do_avg_sse( p, u, i );
	curline[6] = _mm_cvtsi64_si32( p );
	
	i = _mm_srli_pi64( i, 32 );	// >> 32
	u = _mm_cvtsi32_si64( prevline[7] );
	p = do_avg_sse( p, u, i );
	curline[7] = _mm_cvtsi64_si32( p );

	inp = _mm_cvtsi64_si32( p );
	inup =  _mm_cvtsi64_si32( u );
	_mm_empty();
}
#endif

/*
	chroma/luminosity decoding
	(this does reordering, color correlation filter, MED/AVG  at a time)
*/
void TVPTLG6DecodeLineGeneric_sse2_c(tjs_uint32 *prevline, tjs_uint32 *curline, tjs_int width, tjs_int start_block, tjs_int block_limit, tjs_uint8 *filtertypes, tjs_int skipblockbytes, tjs_uint32 *in, tjs_uint32 initialp, tjs_int oddskip, tjs_int dir) {
	tjs_uint32 p, up;
	if(start_block) {
		prevline += start_block * TVP_TLG6_W_BLOCK_SIZE;
		curline  += start_block * TVP_TLG6_W_BLOCK_SIZE;
		p  = curline[-1];
		up = prevline[-1];
	} else {
		p = up = initialp;
	}
	oddskip *= TVP_TLG6_W_BLOCK_SIZE;	// oddskip * 8
	if( dir & 1 ) {
		// forward
		skipblockbytes -= TVP_TLG6_W_BLOCK_SIZE;
		in += skipblockbytes * start_block;
		in += oddskip;
		for( int i = start_block; i < block_limit; i ++) {
			if( i&1 ) {
				in += oddskip;
			} else {
				in -= oddskip;
			}
			switch(filtertypes[i]) {
#define TVP_TLG6_DO_CHROMA_DECODE_FORWARD( N )	\
	case (N<<1)+0: do_filter_med_sse2<filter_insts_##N##_sse2,filter_forward_input_sse2>( p, up, in, prevline, curline ); prevline+=8; curline+=8; in+=8; break;\
	case (N<<1)+1: do_filter_avg_sse2<filter_insts_##N##_sse2,filter_forward_input_sse2>( p, up, in, prevline, curline ); prevline+=8; curline+=8; in+=8; break;
				TVP_TLG6_DO_CHROMA_DECODE_FORWARD( 0);
				TVP_TLG6_DO_CHROMA_DECODE_FORWARD( 1);
				TVP_TLG6_DO_CHROMA_DECODE_FORWARD( 2);
				TVP_TLG6_DO_CHROMA_DECODE_FORWARD( 3);
				TVP_TLG6_DO_CHROMA_DECODE_FORWARD( 4);
				TVP_TLG6_DO_CHROMA_DECODE_FORWARD( 5);
				TVP_TLG6_DO_CHROMA_DECODE_FORWARD( 6);
				TVP_TLG6_DO_CHROMA_DECODE_FORWARD( 7);
				TVP_TLG6_DO_CHROMA_DECODE_FORWARD( 8);
				TVP_TLG6_DO_CHROMA_DECODE_FORWARD( 9);
				TVP_TLG6_DO_CHROMA_DECODE_FORWARD(10);
				TVP_TLG6_DO_CHROMA_DECODE_FORWARD(11);
				TVP_TLG6_DO_CHROMA_DECODE_FORWARD(12);
				TVP_TLG6_DO_CHROMA_DECODE_FORWARD(13);
				TVP_TLG6_DO_CHROMA_DECODE_FORWARD(14);
				TVP_TLG6_DO_CHROMA_DECODE_FORWARD(15);
#undef TVP_TLG6_DO_CHROMA_DECODE_FORWARD
			}
			in += skipblockbytes;
		}
	} else {
		// backward
		skipblockbytes += TVP_TLG6_W_BLOCK_SIZE;
		in += skipblockbytes * start_block;
		in += oddskip;
		//in += (TVP_TLG6_W_BLOCK_SIZE - 1);
		in += TVP_TLG6_W_BLOCK_SIZE;
		for( int i = start_block; i < block_limit; i ++) {
			if( i&1 ) {
				in += oddskip;
			} else{
				in -= oddskip;
			}
			switch(filtertypes[i]) {
#define TVP_TLG6_DO_CHROMA_DECODE_BACKWARD( N )	\
	case (N<<1)+0: in-=8; do_filter_med_sse2<filter_insts_##N##_sse2,filter_backward_input_sse2>( p, up, in, prevline, curline ); prevline+=8; curline+=8; break;\
	case (N<<1)+1: in-=8; do_filter_avg_sse2<filter_insts_##N##_sse2,filter_backward_input_sse2>( p, up, in, prevline, curline ); prevline+=8; curline+=8; break;
				TVP_TLG6_DO_CHROMA_DECODE_BACKWARD( 0);
				TVP_TLG6_DO_CHROMA_DECODE_BACKWARD( 1);
				TVP_TLG6_DO_CHROMA_DECODE_BACKWARD( 2);
				TVP_TLG6_DO_CHROMA_DECODE_BACKWARD( 3);
				TVP_TLG6_DO_CHROMA_DECODE_BACKWARD( 4);
				TVP_TLG6_DO_CHROMA_DECODE_BACKWARD( 5);
				TVP_TLG6_DO_CHROMA_DECODE_BACKWARD( 6);
				TVP_TLG6_DO_CHROMA_DECODE_BACKWARD( 7);
				TVP_TLG6_DO_CHROMA_DECODE_BACKWARD( 8);
				TVP_TLG6_DO_CHROMA_DECODE_BACKWARD( 9);
				TVP_TLG6_DO_CHROMA_DECODE_BACKWARD(10);
				TVP_TLG6_DO_CHROMA_DECODE_BACKWARD(11);
				TVP_TLG6_DO_CHROMA_DECODE_BACKWARD(12);
				TVP_TLG6_DO_CHROMA_DECODE_BACKWARD(13);
				TVP_TLG6_DO_CHROMA_DECODE_BACKWARD(14);
				TVP_TLG6_DO_CHROMA_DECODE_BACKWARD(15);
#undef TVP_TLG6_DO_CHROMA_DECODE_BACKWARD
			}
			in += skipblockbytes;
		}
	}
}
void TVPTLG6DecodeLine_sse2_c(tjs_uint32 *prevline, tjs_uint32 *curline, tjs_int width, tjs_int block_count, tjs_uint8 *filtertypes, tjs_int skipblockbytes, tjs_uint32 *in, tjs_uint32 initialp, tjs_int oddskip, tjs_int dir) {
	TVPTLG6DecodeLineGeneric_sse2_c(prevline, curline, width, 0, block_count,
		filtertypes, skipblockbytes, in, initialp, oddskip, dir);
}
#if 0 // SSE2/SSE/MMX を使うバージョン
void TVPTLG6DecodeLineGeneric_sse2_sse_c(tjs_uint32 *prevline, tjs_uint32 *curline, tjs_int width, tjs_int start_block, tjs_int block_limit, tjs_uint8 *filtertypes, tjs_int skipblockbytes, tjs_uint32 *in, tjs_uint32 initialp, tjs_int oddskip, tjs_int dir) {
	tjs_uint32 p, up;
	if(start_block) {
		prevline += start_block * TVP_TLG6_W_BLOCK_SIZE;
		curline  += start_block * TVP_TLG6_W_BLOCK_SIZE;
		p  = curline[-1];
		up = prevline[-1];
	} else {
		p = up = initialp;
	}
	oddskip *= TVP_TLG6_W_BLOCK_SIZE;	// oddskip * 8
	if( dir & 1 ) {
		// forward
		skipblockbytes -= TVP_TLG6_W_BLOCK_SIZE;
		in += skipblockbytes * start_block;
		in += oddskip;
		for( int i = start_block; i < block_limit; i ++) {
			if( i&1 ) {
				in += oddskip;
			} else {
				in -= oddskip;
			}
			switch(filtertypes[i]) {
#define TVP_TLG6_DO_CHROMA_DECODE_FORWARD( N )	\
	case (N<<1)+0: do_filter_med_sse2_sse<filter_insts_##N##_sse2,filter_forward_input_sse2>( p, up, in, prevline, curline ); prevline+=8; curline+=8; in+=8; break;\
	case (N<<1)+1: do_filter_avg_sse2_sse<filter_insts_##N##_sse2,filter_forward_input_sse2>( p, up, in, prevline, curline ); prevline+=8; curline+=8; in+=8; break;
				TVP_TLG6_DO_CHROMA_DECODE_FORWARD( 0);
				TVP_TLG6_DO_CHROMA_DECODE_FORWARD( 1);
				TVP_TLG6_DO_CHROMA_DECODE_FORWARD( 2);
				TVP_TLG6_DO_CHROMA_DECODE_FORWARD( 3);
				TVP_TLG6_DO_CHROMA_DECODE_FORWARD( 4);
				TVP_TLG6_DO_CHROMA_DECODE_FORWARD( 5);
				TVP_TLG6_DO_CHROMA_DECODE_FORWARD( 6);
				TVP_TLG6_DO_CHROMA_DECODE_FORWARD( 7);
				TVP_TLG6_DO_CHROMA_DECODE_FORWARD( 8);
				TVP_TLG6_DO_CHROMA_DECODE_FORWARD( 9);
				TVP_TLG6_DO_CHROMA_DECODE_FORWARD(10);
				TVP_TLG6_DO_CHROMA_DECODE_FORWARD(11);
				TVP_TLG6_DO_CHROMA_DECODE_FORWARD(12);
				TVP_TLG6_DO_CHROMA_DECODE_FORWARD(13);
				TVP_TLG6_DO_CHROMA_DECODE_FORWARD(14);
				TVP_TLG6_DO_CHROMA_DECODE_FORWARD(15);
#undef TVP_TLG6_DO_CHROMA_DECODE_FORWARD
			}
			in += skipblockbytes;
		}
	} else {
		// backward
		skipblockbytes += TVP_TLG6_W_BLOCK_SIZE;
		in += skipblockbytes * start_block;
		in += oddskip;
		//in += (TVP_TLG6_W_BLOCK_SIZE - 1);
		in += TVP_TLG6_W_BLOCK_SIZE;
		for( int i = start_block; i < block_limit; i ++) {
			if( i&1 ) {
				in += oddskip;
			} else{
				in -= oddskip;
			}
			switch(filtertypes[i]) {
#define TVP_TLG6_DO_CHROMA_DECODE_BACKWARD( N )	\
	case (N<<1)+0: in-=8; do_filter_med_sse2_sse<filter_insts_##N##_sse2,filter_backward_input_sse2>( p, up, in, prevline, curline ); prevline+=8; curline+=8; break;\
	case (N<<1)+1: in-=8; do_filter_avg_sse2_sse<filter_insts_##N##_sse2,filter_backward_input_sse2>( p, up, in, prevline, curline ); prevline+=8; curline+=8; break;
				TVP_TLG6_DO_CHROMA_DECODE_BACKWARD( 0);
				TVP_TLG6_DO_CHROMA_DECODE_BACKWARD( 1);
				TVP_TLG6_DO_CHROMA_DECODE_BACKWARD( 2);
				TVP_TLG6_DO_CHROMA_DECODE_BACKWARD( 3);
				TVP_TLG6_DO_CHROMA_DECODE_BACKWARD( 4);
				TVP_TLG6_DO_CHROMA_DECODE_BACKWARD( 5);
				TVP_TLG6_DO_CHROMA_DECODE_BACKWARD( 6);
				TVP_TLG6_DO_CHROMA_DECODE_BACKWARD( 7);
				TVP_TLG6_DO_CHROMA_DECODE_BACKWARD( 8);
				TVP_TLG6_DO_CHROMA_DECODE_BACKWARD( 9);
				TVP_TLG6_DO_CHROMA_DECODE_BACKWARD(10);
				TVP_TLG6_DO_CHROMA_DECODE_BACKWARD(11);
				TVP_TLG6_DO_CHROMA_DECODE_BACKWARD(12);
				TVP_TLG6_DO_CHROMA_DECODE_BACKWARD(13);
				TVP_TLG6_DO_CHROMA_DECODE_BACKWARD(14);
				TVP_TLG6_DO_CHROMA_DECODE_BACKWARD(15);
#undef TVP_TLG6_DO_CHROMA_DECODE_BACKWARD
			}
			in += skipblockbytes;
		}
	}
	_mm_empty();
}
void TVPTLG6DecodeLine_sse2_sse_c(tjs_uint32 *prevline, tjs_uint32 *curline, tjs_int width, tjs_int block_count, tjs_uint8 *filtertypes, tjs_int skipblockbytes, tjs_uint32 *in, tjs_uint32 initialp, tjs_int oddskip, tjs_int dir) {
	TVPTLG6DecodeLineGeneric_sse2_sse_c(prevline, curline, width, 0, block_count,
		filtertypes, skipblockbytes, in, initialp, oddskip, dir);
}
#endif
//#define LZSS_TEST
#ifdef LZSS_TEST
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include "tjsCommHead.h"
#include "tjsUtils.h"

extern "C" void TVPTLG6DecodeLine_c(tjs_uint32 *prevline, tjs_uint32 *curline, tjs_int width, tjs_int block_count, tjs_uint8 *filtertypes, tjs_int skipblockbytes, tjs_uint32 *in, tjs_uint32 initialp, tjs_int oddskip, tjs_int dir);
// C版と比較すると4倍以上だけど、MMX2(SSE)に比較すると少し遅いか……
void TVPTLG6DecodeLine_test(tjs_uint32 *prevline, tjs_uint32 *curline, tjs_int width, tjs_int block_count, tjs_uint8 *filtertypes, tjs_int skipblockbytes, tjs_uint32 *in, tjs_uint32 initialp, tjs_int oddskip, tjs_int dir) {
	tjs_uint32 *curline0 = (tjs_uint32*)TJSAlignedAlloc(block_count*TVP_TLG6_W_BLOCK_SIZE*sizeof(tjs_uint32), 4);
	tjs_uint32 *curline1 = (tjs_uint32*)TJSAlignedAlloc(block_count*TVP_TLG6_W_BLOCK_SIZE*sizeof(tjs_uint32), 4);
	memcpy( curline0, curline, block_count*TVP_TLG6_W_BLOCK_SIZE*sizeof(tjs_uint32) );
	memcpy( curline1, curline, block_count*TVP_TLG6_W_BLOCK_SIZE*sizeof(tjs_uint32) );
	TVPTLG6DecodeLine_sse_a( prevline, curline, width, block_count, filtertypes, skipblockbytes, in, initialp, oddskip, dir);
#if 1
	tjs_uint32 aux;
	unsigned __int64 start = __rdtscp(&aux);
	TVPTLG6DecodeLine_sse2_c( prevline, curline0, width, block_count, filtertypes, skipblockbytes, in, initialp, oddskip, dir);
	//TVPTLG6DecodeLine_sse2_sse_c( prevline, curline0, width, block_count, filtertypes, skipblockbytes, in, initialp, oddskip, dir);
	unsigned __int64 end0 = __rdtscp(&aux);
	TVPTLG6DecodeLine_sse_a( prevline, curline1, width, block_count, filtertypes, skipblockbytes, in, initialp, oddskip, dir);
	//TVPTLG6DecodeLine_c( prevline, curline1, width, block_count, filtertypes, skipblockbytes, in, initialp, oddskip, dir);
	unsigned __int64 end1 = __rdtscp(&aux);
	{
		unsigned __int64 func_b_total = end0-start;
		unsigned __int64 func_a_total = end1-end0;
		wchar_t buff[128];
		wsprintf( buff, L"TLG6 SSE2 %I64d, ASM %I64d, rate %I64d\n", func_b_total, func_a_total, (func_b_total)*100/(func_a_total) );
		OutputDebugString( buff );
	}
#ifdef _DEBUG
	for( int i = 0; i < block_count*TVP_TLG6_W_BLOCK_SIZE; i++ ) {
		if( curline0[i] != curline1[i] ) {
			wchar_t buff[128];
			wsprintf( buff, L"LZSS text type : %d, index : %d, 0x%08x, 0x%08x\n", filtertypes[i/TVP_TLG6_W_BLOCK_SIZE], i, curline0[i], curline1[i] );
			OutputDebugString( buff );
		}
	}
#endif
#endif
	TJSAlignedDealloc( curline0 );
	TJSAlignedDealloc( curline1 );
}

extern "C" tjs_int TVPTLG5DecompressSlide_c(tjs_uint8 *out, const tjs_uint8 *in, tjs_int insize, tjs_uint8 *text, tjs_int initialr);
tjs_int TVPTLG5DecompressSlide_test( tjs_uint8 *out, const tjs_uint8 *in, tjs_int insize, tjs_uint8 *text, tjs_int initialr ) {
	tjs_uint8* text0 = (tjs_uint8*)TJSAlignedAlloc(4096+16, 4);
	tjs_uint8* text1 = (tjs_uint8*)TJSAlignedAlloc(4096+16, 4);
	tjs_uint8* out0 = (tjs_uint8*)TJSAlignedAlloc(2048 * 2048 + 10+16, 4);
	tjs_uint8* out1 = (tjs_uint8*)TJSAlignedAlloc(2048 * 2048 + 10+16, 4);
	memcpy( text0, text, 4096 );
	memcpy( text1, text, 4096 );
	tjs_int ret = TVPTLG5DecompressSlide_a( out, in, insize, text, initialr );

	tjs_uint32 aux;
	unsigned __int64 start = __rdtscp(&aux);
	tjs_int r0 = TVPTLG5DecompressSlide_sse2_c( out0, in, insize, text0, initialr );
	//tjs_int r0 = TVPTLG5DecompressSlide_sse_c( out0, in, insize, text0, initialr );
	unsigned __int64 end0 = __rdtscp(&aux);
	//tjs_int r1 = TVPTLG5DecompressSlide_a( out1, in, insize, text1, initialr );
	tjs_int r1 = TVPTLG5DecompressSlide_c( out1, in, insize, text1, initialr );
	unsigned __int64 end1 = __rdtscp(&aux);
	{
		unsigned __int64 func_b_total = end0-start;
		unsigned __int64 func_a_total = end1-end0;
		wchar_t buff[128];
		wsprintf( buff, L"LZSS SSE2 %I64d, ASM %I64d, rate %I64d\n", func_b_total, func_a_total, (func_b_total)*100/(func_a_total) );
		OutputDebugString( buff );
	}

	if( r0 != r1 ) {
		wchar_t buff[128];
		wsprintf( buff, L"LZSS ret, %d, %d\n", r0, r1 );
		OutputDebugString( buff );
	}
	for( int i = 0; i < 4096; i++ ) {
		if( text0[i] != text1[i] ) {
			wchar_t buff[128];
			wsprintf( buff, L"LZSS text index : %d, %02x, %02x\n", i, text0[i], text1[i] );
			OutputDebugString( buff );
		}
	}
	for( int i = 0; i < 1920*4; i++ ) {
		if( out0[i] != out1[i] ) {
			wchar_t buff[128];
			wsprintf( buff, L"LZSS output index : %d, %02x, %02x\n", i, out0[i], out1[i] );
			OutputDebugString( buff );
		}
	}

	TJSAlignedDealloc( text0 );
	TJSAlignedDealloc( text1 );
	TJSAlignedDealloc( out0 );
	TJSAlignedDealloc( out1 );
	return ret;
}
// tjs_int TVPTLG5DecompressSlide_sse2_c( tjs_uint8 *out, const tjs_uint8 *in, tjs_int insize, tjs_uint8 *text, tjs_int initialr );
// tjs_int TVPTLG5DecompressSlide_mmx_a( tjs_uint8 *out, const tjs_uint8 *in, tjs_int insize, tjs_uint8 *text, tjs_int initialr );

// void TVPTLG5ComposeColors3To4_sse2_c(tjs_uint8 *outp, const tjs_uint8 *upper, tjs_uint8 * const * buf, tjs_int width)
// TVPTLG5ComposeColors4To4_sse2_c(tjs_uint8 *outp, const tjs_uint8 *upper, tjs_uint8 * const * buf, tjs_int width)
void TVPTLG5ComposeColors3To4_test(tjs_uint8 *outp, const tjs_uint8 *upper, tjs_uint8 * const * buf, tjs_int width) {
	tjs_uint8 *outbuf0[4];
	tjs_uint8 *outbuf1[4];
	for(tjs_int i = 0; i < 3; i++) {
		outbuf0[i] = (tjs_uint8*)TJSAlignedAlloc(16 * width + 10, 4);
		outbuf1[i] = (tjs_uint8*)TJSAlignedAlloc(16 * width + 10, 4);
		memcpy( outbuf0[i], buf[i], width );
		memcpy( outbuf1[i], buf[i], width );
	}
	tjs_uint8 *upper0 = (tjs_uint8*)TJSAlignedAlloc(width*4, 4);
	tjs_uint8 *upper1 = (tjs_uint8*)TJSAlignedAlloc(width*4, 4);
	memcpy( upper0, upper, width*4 );
	memcpy( upper1, upper, width*4 );
	tjs_uint8 *outp0 = (tjs_uint8*)TJSAlignedAlloc(width*4, 4);
	tjs_uint8 *outp1 = (tjs_uint8*)TJSAlignedAlloc(width*4, 4);
	TVPTLG5ComposeColors3To4_mmx_a( outp, upper, buf, width );

	tjs_uint32 aux;
	unsigned __int64 start = __rdtscp(&aux);
	TVPTLG5ComposeColors3To4_sse2_c( outp0, upper0, outbuf0, width );
	unsigned __int64 end0 = __rdtscp(&aux);
	TVPTLG5ComposeColors3To4_mmx_a( outp1, upper1, outbuf1, width );
	unsigned __int64 end1 = __rdtscp(&aux);
	{
		unsigned __int64 func_b_total = end0-start;
		unsigned __int64 func_a_total = end1-end0;
		wchar_t buff[128];
		wsprintf( buff, L"ComposeColors3To4 SSE2 %I64d, ASM %I64d, rate %I64d\n", func_b_total, func_a_total, (func_b_total)*100/(func_a_total) );
		OutputDebugString( buff );
	}
	for( int i = 0; i < width; i++ ) {
		if( outp0[i*4+0] != outp1[i*4+0] ||
			outp0[i*4+1] != outp1[i*4+1] ||
			outp0[i*4+2] != outp1[i*4+2] ||
			outp0[i*4+3] != outp1[i*4+3]) {
			wchar_t buff[128];
			wsprintf( buff, L"LZSS color index : %d, 0x%02x%02x%02x%02x, 0x%02x%02x%02x%02x\n", i,
				outp0[i*4+3], outp0[i*4+2], outp0[i*4+1], outp0[i*4+0],
				outp1[i*4+3], outp1[i*4+2], outp1[i*4+1], outp1[i*4+0] );
			OutputDebugString( buff );
		}
	}

	for(tjs_int i = 0; i < 3; i++) {
		TJSAlignedDealloc( outbuf0[i] );
		TJSAlignedDealloc( outbuf1[i] );
	}
	TJSAlignedDealloc( upper0 );
	TJSAlignedDealloc( upper1 );
}
void TVPTLG5ComposeColors4To4_test(tjs_uint8 *outp, const tjs_uint8 *upper, tjs_uint8 * const * buf, tjs_int width) {
	tjs_uint8 *outbuf0[4];
	tjs_uint8 *outbuf1[4];
	for(tjs_int i = 0; i < 4; i++) {
		outbuf0[i] = (tjs_uint8*)TJSAlignedAlloc(16 * width + 10, 4);
		outbuf1[i] = (tjs_uint8*)TJSAlignedAlloc(16 * width + 10, 4);
		memcpy( outbuf0[i], buf[i], width );
		memcpy( outbuf1[i], buf[i], width );
	}
	tjs_uint8 *upper0 = (tjs_uint8*)TJSAlignedAlloc(width*4, 4);
	tjs_uint8 *upper1 = (tjs_uint8*)TJSAlignedAlloc(width*4, 4);
	memcpy( upper0, upper, width*4 );
	memcpy( upper1, upper, width*4 );
	tjs_uint8 *outp0 = (tjs_uint8*)TJSAlignedAlloc(width*4, 4);
	tjs_uint8 *outp1 = (tjs_uint8*)TJSAlignedAlloc(width*4, 4);
	TVPTLG5ComposeColors4To4_mmx_a( outp, upper, buf, width );
	memset( outp0, 0xdd, width*4 );
	memset( outp1, 0xdd, width*4 );

	tjs_uint32 aux;
	unsigned __int64 start = __rdtscp(&aux);
	TVPTLG5ComposeColors4To4_sse2_c( outp0, upper0, outbuf0, width );
	unsigned __int64 end0 = __rdtscp(&aux);
	TVPTLG5ComposeColors4To4_mmx_a( outp1, upper1, outbuf1, width );
	unsigned __int64 end1 = __rdtscp(&aux);
	{
		unsigned __int64 func_b_total = end0-start;
		unsigned __int64 func_a_total = end1-end0;
		wchar_t buff[128];
		wsprintf( buff, L"ComposeColors4To4 SSE2 %I64d, ASM %I64d, rate %I64d\n", func_b_total, func_a_total, (func_b_total)*100/(func_a_total) );
		OutputDebugString( buff );
	}
	for( int i = 0; i < width; i++ ) {
		if( outp0[i*4+0] != outp1[i*4+0] ||
			outp0[i*4+1] != outp1[i*4+1] ||
			outp0[i*4+2] != outp1[i*4+2] ||
			outp0[i*4+3] != outp1[i*4+3]) {
			wchar_t buff[128];
			wsprintf( buff, L"LZSS color index : %d, 0x%02x%02x%02x%02x, 0x%02x%02x%02x%02x\n", i,
				outp0[i*4+3], outp0[i*4+2], outp0[i*4+1], outp0[i*4+0],
				outp1[i*4+3], outp1[i*4+2], outp1[i*4+1], outp1[i*4+0] );
			OutputDebugString( buff );
		}
	}
	for(tjs_int i = 0; i < 4; i++) {
		TJSAlignedDealloc( outbuf0[i] );
		TJSAlignedDealloc( outbuf1[i] );
	}
	TJSAlignedDealloc( upper0 );
	TJSAlignedDealloc( upper1 );
	TJSAlignedDealloc( outp0 );
	TJSAlignedDealloc( outp1 );
}
/*
tlg6_golomb は、MMX 使っているが、一時変数として使われているのとプリフェッチのみ。SSE2 は意味なさげ
tlg6_chroma は、MMX(SSE)が使われている
*/
#endif
