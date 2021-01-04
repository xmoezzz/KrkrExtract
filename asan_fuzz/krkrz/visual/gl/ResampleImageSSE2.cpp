

#define _USE_MATH_DEFINES

#include "tjsCommHead.h"
#include "LayerBitmapIntf.h"
#include "LayerBitmapImpl.h"
#include "ThreadIntf.h"

#include <float.h>
#include <math.h>
#include <cmath>
#include <vector>

#include <xmmintrin.h> // SSE
#include <emmintrin.h> // SSE2

#include "x86simdutil.h"
#include "aligned_allocator.h"

#include "WeightFunctorSSE.h"
#include "ResampleImageInternal.h"

static __m128 M128_PS_STEP( _mm_set_ps(3.0f,2.0f,1.0f,0.0f) );
static __m128 M128_PS_4_0( _mm_set1_ps( 4.0f ) );
static __m128 M128_PS_FIXED15( _mm_set1_ps( (float)(1<<15) ) );
static __m128i M128_U32_FIXED_ROUND( (_mm_set1_epi32(0x00200020)) );
static __m128i M128_U32_FIXED_COLOR_MASK( (_mm_set1_epi32(0x00ff00ff)) );
static __m128i M128_U32_FIXED_COLOR_MASK8( (_mm_set1_epi32(0x000000ff)) );
static __m128 M128_EPSILON( _mm_set1_ps( FLT_EPSILON ) );
static __m128 M128_ABS_MASK( _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff)) );

static bool InitializedResampleSSE2 = false;
void TVPInitializeResampleSSE2() {
	if( !InitializedResampleSSE2) {
		M128_PS_STEP = ( _mm_set_ps(3.0f,2.0f,1.0f,0.0f) );
		M128_PS_4_0 = ( _mm_set1_ps( 4.0f ) );
		M128_PS_FIXED15 = ( _mm_set1_ps( (float)(1<<15) ) );
		M128_U32_FIXED_ROUND = ( (_mm_set1_epi32(0x00200020)) );
		M128_U32_FIXED_COLOR_MASK = ( (_mm_set1_epi32(0x00ff00ff)) );
		M128_U32_FIXED_COLOR_MASK8 = ( (_mm_set1_epi32(0x000000ff)) );
		M128_EPSILON = ( _mm_set1_ps( FLT_EPSILON ) );
		M128_ABS_MASK = ( _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff)) );
		InitializedResampleSSE2 = true;
	}
}

void TJS_USERENTRY ResamplerSSE2FixFunc( void* p );
void TJS_USERENTRY ResamplerSSE2Func( void* p );

template<typename TWeight>
struct AxisParamSSE2 {
	std::vector<int> start_;	// 開始インデックス
	std::vector<int> length_;	// 各要素長さ
	std::vector<int> length_min_;	// 各要素長さ, アライメント化されていない最小長さ
	std::vector<TWeight,aligned_allocator<TWeight> > weight_;

	/**
	 * はみ出している部分をカットする
	 */
	static inline void calculateEdge( float* weight, int& len, int leftedge, int rightedge ) {
		// 左端or右端の時、はみ出す分のウェイトを端に加算する
		if( leftedge ) {
			// 左端からはみ出す分を加算
			int i = 1;
			for( ; i <= leftedge; i++ ) {
				weight[0] += weight[i];
			}
			// 加算した分を移動
			for( int j = 1; i < len; i++, j++ ) {
				weight[j] = weight[i];
			}
			// はみ出した分の長さをカット
			len -= leftedge;
		}
		if( rightedge ) {
			// 右端からはみ出す分を加算
			int i = len - rightedge;
			int r = i - 1;
			for( ; i < len; i++ ) {
				weight[r] += weight[i];
			}
			// はみ出した分の長さをカット
			len -= rightedge;
		}
	}
	// 合計値を求める
	static inline __m128 sumWeight( float* weight, int len4 ) {
		float* w = weight;
		__m128 sum = _mm_setzero_ps();
		for( int i = 0; i < len4; i+=4 ) {
			__m128 weight4 = _mm_load_ps( w );
			sum = _mm_add_ps( sum, weight4 );
			w += 4;
		}
		return m128_hsum_sse1_ps(sum);
	}
	static inline void normalizeAndFixed( float* weight, tjs_uint32*& output, int& len, int len4, bool strip ) {
		// 合計値を求める
		__m128 sum = sumWeight( weight, len4 );

		// EPSILON より小さい場合は 0 を設定
		const __m128 one = M128_PS_FIXED15; // 符号付なので。あと正規化されているから、最大値は1になる
		__m128 onemask = _mm_cmpgt_ps( sum, M128_EPSILON ); // sum > FLT_EPSILON ? 0xffffffff : 0; _mm_cmpgt_ps
		__m128 rcp = m128_rcp_22bit_ps( sum );
		rcp = _mm_mul_ps( rcp, one );	// 先にシフト分も掛けておく
		rcp = _mm_and_ps( rcp, onemask );
		float* w = weight;
		// 正規化と固定小数点化
		for( int i = 0; i < len4; i+=4 ) {
			__m128 weight4 = _mm_load_ps( w ); w += 4;
			weight4 = _mm_mul_ps( weight4, rcp );

			// 固定小数点化
			__m128i fix = _mm_cvtps_epi32( weight4 );
			fix = _mm_packs_epi32( fix, fix );		// 16bit化 [01 02 03 04 01 02 03 04]
			fix = _mm_unpacklo_epi16( fix, fix );	// 01 01 02 02 03 03 04 04
			_mm_storeu_si128( (__m128i*)output, fix );	// tjs_uint32 に short*2 で同じ値を格納する
			output += 4;
		}
		if( strip ) {
			output -= len4-len;
		}
	}
	static inline void calculateWeight( float* weight, tjs_uint32*& output, int& len, int leftedge, int rightedge, bool strip=false ) {
		// len にははみ出した分も含まれているので、まずはその部分をカットする
		calculateEdge( weight, len, leftedge, rightedge );

		// 4 の倍数化
		int len4 = ((len+3)>>2)<<2;

		// ダミー部分を0に設定
		for( int i = len; i < len4; i++ ) weight[i] = 0.0f;

		// 正規化と固定小数点化
		normalizeAndFixed( weight, output, len, len4, strip );
	}
	static inline void normalize( float* weight, float*& output, int& len, int len4, bool strip ) {
		// 合計値を求める
		__m128 sum = sumWeight( weight, len4 );

		// EPSILON より小さい場合は 0 を設定
		__m128 onemask = _mm_cmpgt_ps( sum, M128_EPSILON ); // sum > FLT_EPSILON ? 0xffffffff : 0; _mm_cmpgt_ps
		__m128 rcp = m128_rcp_22bit_ps( sum );
		rcp = _mm_and_ps( rcp, onemask );
		float* w = weight;
		// 正規化
		for( int i = 0; i < len4; i+=4 ) {
			__m128 weight4 = _mm_load_ps( w ); w += 4;
			weight4 = _mm_mul_ps( weight4, rcp );
			_mm_storeu_ps( (float*)output, weight4 );
			output += 4;
		}
		if( strip ) {
			output -= len4-len;
		}
	}
	static inline void calculateWeight( float* weight, float*& output, int& len, int leftedge, int rightedge, bool strip=false ) {
		// len にははみ出した分も含まれているので、まずはその部分をカットする
		calculateEdge( weight, len, leftedge, rightedge );

		// 4 の倍数化
		int len4 = ((len+3)>>2)<<2;

		// ダミー部分を0に設定
		for( int i = len; i < len4; i++ ) weight[i] = 0.0f;

		// 正規化と固定小数点化
		normalize( weight, output, len, len4, strip );
	}

	template<typename TWeightFunc>
	void calculateAxis( int srcstart, int srcend, int srclength, int dstlength, float tap, bool strip, TWeightFunc& func) {
		start_.clear();
		start_.reserve( dstlength );
		length_.clear();
		length_.reserve( dstlength );
		length_min_.clear();
		length_min_.reserve( dstlength );
		// まずは距離を計算
		// left/right判定も外に出すと少しだけ速くなるとは思ったが遅くなった、inline化されないのかもしれない
		if( srclength <= dstlength ) { // 拡大
			float rangex = tap;
			int maxrange = ((((int)rangex*2+2)+3)>>2)<<2;
			std::vector<float,aligned_allocator<float> > work( maxrange, 0.0f );
			float* weight = &work[0];
			int length = (((dstlength * maxrange + dstlength)+3)>>2)<<2;
#ifdef _DEBUG
			weight_.resize( length );
#else
			weight_.reserve( length );
#endif
			const __m128 delta4 = M128_PS_4_0;
			const __m128 deltafirst = M128_PS_STEP;
			const __m128 absmask = M128_ABS_MASK;
			TWeight* output = &weight_[0];
			for( int x = 0; x < dstlength; x++ ) {
				float cx = (x+0.5f)*(float)srclength/(float)dstlength + srcstart;
				int left = (int)std::floor(cx-rangex);
				int right = (int)std::floor(cx+rangex);
				int start = left;
				int leftedge = 0;
				if( left < srcstart ) {
					leftedge = srcstart - left;
					start = srcstart;
				}
				int rightedge = 0;
				if( right >= srcend ) {
					rightedge = right - srcend;
				}
				start_.push_back( start );
				int len = right - left;
				__m128 dist4 = _mm_set1_ps((float)left+0.5f-cx);
				int len4 = ((len+3)>>2)<<2;	// 4 の倍数化
				float* w = weight;
				// まずは最初の要素のみ処理する
				dist4 = _mm_add_ps( dist4, deltafirst );
				_mm_store_ps( w, func( _mm_and_ps( dist4, absmask ) ) );	// 絶対値+weight計算
				w += 4;
				for( int sx = 4; sx < len4; sx+=4 ) {
					dist4 = _mm_add_ps( dist4, delta4 );	// 4つずつスライド
					_mm_store_ps( w, func( _mm_and_ps( dist4, absmask ) ) );	// 絶対値+weight計算
					w += 4;
				}
				calculateWeight( weight, output, len, leftedge, rightedge, strip );
				len4 = ((len+3)>>2)<<2;
				if( strip ) {
					length_.push_back( len );
					length_min_.push_back( len );
				} else {
					length_.push_back( len4 );
					length_min_.push_back( len );
				}
			}
		} else { // 縮小
			float rangex = tap*(float)srclength/(float)dstlength;
			int maxrange = ((((int)rangex*2+2)+3)>>2)<<2;
			std::vector<float,aligned_allocator<float> > work( maxrange, 0.0f );
			float* weight = &work[0];
			int length = (((srclength * maxrange + srclength)+3)>>2)<<2;
#ifdef _DEBUG
			weight_.resize( length );
#else
			weight_.reserve( length );
#endif
			TWeight* output = &weight_[0];
			const float delta = (float)dstlength/(float)srclength; // 転送先座標での位置増分

			__m128 delta4 = _mm_set1_ps(delta);
			__m128 deltafirst = M128_PS_STEP;
			const __m128 absmask = M128_ABS_MASK;
			deltafirst = _mm_mul_ps( deltafirst, delta4 );	// 0 1 2 3 と順に加算されるようにする
			// 4倍する
			delta4 = _mm_add_ps( delta4, delta4 );
			delta4 = _mm_add_ps( delta4, delta4 );
			for( int x = 0; x < dstlength; x++ ) {
				float cx = (x+0.5f)*(float)srclength/(float)dstlength + srcstart;
				int left = (int)std::floor(cx-rangex);
				int right = (int)std::floor(cx+rangex);
				int start = left;
				int leftedge = 0;
				if( left < srcstart ) {
					leftedge = srcstart - left;
					start = srcstart;
				}
				int rightedge = 0;
				if( right >= srcend ) {
					rightedge = right - srcend;
				}
				start_.push_back( start );
				// 転送先座標での位置
				int len = right-left;
				float dx = (left+0.5f-cx) * delta;
				__m128 dist4 = _mm_set1_ps(dx);
				int len4 = ((len+3)>>2)<<2;	// 4 の倍数化
				float* w = weight;
				// まずは最初の要素のみ処理する
				dist4 = _mm_add_ps( dist4, deltafirst );
				_mm_store_ps( w, func( _mm_and_ps( dist4, absmask ) ) );	// 絶対値+weight計算
				w += 4;
				for( int sx = 4; sx < len4; sx+=4 ) {
					dist4 = _mm_add_ps( dist4, delta4 );	// 4つずつスライド
					_mm_store_ps( w, func( _mm_and_ps( dist4, absmask ) ) );	// 絶対値+weight計算
					w += 4;
				}
				calculateWeight( weight, output, len, leftedge, rightedge, strip );
				len4 = ((len+3)>>2)<<2;
				if( strip ) {
					length_.push_back( len );
					length_min_.push_back( len );
				} else {
					length_.push_back( len4 );
					length_min_.push_back( len );
				}
			}
		}
	}
	// 合計値を求める
	static inline __m128 sumWeightUnalign( float* weight, int len4 ) {
		float* w = weight;
		__m128 sum = _mm_setzero_ps();
		for( int i = 0; i < len4; i+=4 ) {
			__m128 weight4 = _mm_loadu_ps( w );
			sum = _mm_add_ps( sum, weight4 );
			w += 4;
		}
		return m128_hsum_sse1_ps(sum);
	}
	// 正規化
	void normalizeAreaAvg( float* wstart, float* dweight, tjs_uint size, bool strip ) {
		const int count = (const int)length_.size();
		int dwindex = 0;
		const __m128 epsilon = M128_EPSILON;
		for( int i = 0; i < count; i++ ) {
			float* dw = dweight;
			int len = length_[i];
			float* w = wstart;
			int len4 = ((len+3)>>2)<<2;	// 4 の倍数化
			int idx = 0;
			for( ; idx < len; idx++ ) {
				*dw = *w;
				dw++;
				w++;
			}
			wstart = w;
			w = dweight;
			// アライメント
			for( ; idx < len4; idx++ ) {
				*dw = 0.0f;
				dw++;
			}
			dweight = dw;

			// 合計値を求める
			__m128 sum;
			if( strip ) {
				sum = sumWeightUnalign( w, len4 );
			} else {
				sum = sumWeight( w, len4 );
			}

			// EPSILON より小さい場合は 0 を設定
			__m128 onemask = _mm_cmpgt_ps( sum, epsilon ); // sum > FLT_EPSILON ? 0xffffffff : 0; _mm_cmpgt_ps
			__m128 rcp = m128_rcp_22bit_ps( sum );
			rcp = _mm_and_ps( rcp, onemask );
			// 正規化
			for( int j = 0; j < len4; j += 4 ) {
				__m128 weight4 = _mm_loadu_ps( w );
				weight4 = _mm_mul_ps( weight4, rcp );
				_mm_storeu_ps( (float*)w, weight4 );
				w += 4;
			}
			if( strip ) {
				dweight -= len4-len;
				length_min_.push_back( len );
			} else {
				length_[i] = len4;
				length_min_.push_back( len );
			}
		}
	}
	void normalizeAreaAvg( float* wstart, tjs_uint32* dweight, tjs_uint size,  bool strip ) {
		const int count = (const int)length_.size();
#ifdef _DEBUG
		std::vector<float,aligned_allocator<float> > work(size);
#else
		std::vector<float,aligned_allocator<float> > work;
		work.reserve( size );
#endif
		int dwindex = 0;
		const __m128 one = M128_PS_FIXED15; // 符号付なので。あと正規化されているから、最大値は1になる
		const __m128 epsilon = M128_EPSILON;
		for( int i = 0; i < count; i++ ) {
			float* dw = &work[0];
			int len = length_[i];
			float* w = wstart;
			int len4 = ((len+3)>>2)<<2;	// 4 の倍数化
			int idx = 0;
			for( ; idx < len; idx++ ) {
				*dw = *w;
				dw++;
				w++;
			}
			wstart = w;
			w = &work[0];
			// アライメント
			for( ; idx < len4; idx++ ) {
				*dw = 0.0f;
				dw++;
			}

			// 合計値を求める
			__m128 sum = sumWeight( w, len4 );

			// EPSILON より小さい場合は 0 を設定
			__m128 onemask = _mm_cmpgt_ps( sum, epsilon ); // sum > FLT_EPSILON ? 0xffffffff : 0; _mm_cmpgt_ps
			__m128 rcp = m128_rcp_22bit_ps( sum );
			rcp = _mm_mul_ps( rcp, one );	// 先にシフト分も掛けておく
			rcp = _mm_and_ps( rcp, onemask );
			// 正規化
			for( int j = 0; j < len4; j += 4 ) {
				__m128 weight4 = _mm_load_ps( w ); w += 4;
				weight4 = _mm_mul_ps( weight4, rcp );
				// 固定小数点化
				__m128i fix = _mm_cvtps_epi32( weight4 );
				fix = _mm_packs_epi32( fix, fix );		// 16bit化 [01 02 03 04 01 02 03 04]
				fix = _mm_unpacklo_epi16( fix, fix );	// 01 01 02 02 03 03 04 04
				_mm_storeu_si128( (__m128i*)dweight, fix );	// tjs_uint32 に short*2 で同じ値を格納する
				dweight += 4;
			}
			if( strip ) {
				dweight -= len4-len;
				length_min_.push_back( len );
			} else {
				length_[i] = len4;
				length_min_.push_back( len );
			}
		}
	}
	void calculateAxisAreaAvg( int srcstart, int srcend, int srclength, int dstlength, bool strip ) {
		if( dstlength <= srclength ) { // 縮小のみ
			std::vector<float> weight;
			TVPCalculateAxisAreaAvg( srcstart, srcend, srclength, dstlength, start_, length_, weight );
			// 実際のサイズを求める
			int maxsize = 0;
			if( strip == false ) {
				int count = (int)length_.size();
				for( int i = 0; i < count; i++ ) {
					int len = length_[i];
					maxsize += ((len+3)>>2)<<2;	// 4 の倍数化
				}
			} else {
				maxsize = (int)weight.size();
			}
#ifdef _DEBUG
			weight_.resize( maxsize+3 );
#else
			weight_.reserve( maxsize+3 );
#endif
			normalizeAreaAvg( &weight[0], &weight_[0], maxsize+3, strip );
		}
	}
	
};

class ResamplerSSE2Fix {
	AxisParamSSE2<tjs_uint32> paramx_;
	AxisParamSSE2<tjs_uint32> paramy_;

public:
	/**
	 * マルチスレッド化用
	 */
	struct ThreadParameterHV {
		ResamplerSSE2Fix* sampler_;
		int start_;
		int end_;
		int alingnwidth_;

		const tjs_uint32* wstarty_;
		const tTVPBaseBitmap* src_;
		const tTVPRect* srcrect_;
		tTVPBaseBitmap* dest_;
		const tTVPRect* destrect_;

		const tTVPResampleClipping* clip_;
		const tTVPImageCopyFuncBase* blendfunc_;
	};
	/**
	 * 横方向の処理を後にした場合の実装
	 */
	inline void samplingHorizontal( tjs_uint32* dstbits, const int offsetx, const int dstwidth, const tjs_uint32* srcbits ) {
		const tjs_uint32* weightx = &paramx_.weight_[0];
		// まずoffset分をスキップ
		for( int x = 0; x < offsetx; x++ ) {
			weightx += paramx_.length_[x];
		}
		const tjs_uint32* src = srcbits;
		const __m128i cmask = M128_U32_FIXED_COLOR_MASK;
		const __m128i fixround = M128_U32_FIXED_ROUND;
		for( int x = offsetx; x < dstwidth; x++ ) {
			const int left = paramx_.start_[x];
			int right = left + paramx_.length_[x];
			__m128i color_lo = _mm_setzero_si128();
			__m128i color_hi = _mm_setzero_si128();
			// 4ピクセルずつ処理する
			for( int sx = left; sx < right; sx+=4 ) {
				__m128i col4 = _mm_loadu_si128( (const __m128i*)&src[sx] ); // 4ピクセル読み込み
				__m128i weight4 = _mm_loadu_si128( (const __m128i*)weightx ); // ウェイト(固定少数)4つ(16bitで8)読み込み 0 1 2 3
				weightx += 4;

				__m128i col = _mm_and_si128( col4, cmask );	// 00 RR 00 BB & 0x00ff00ff
				col = _mm_slli_epi16( col, 7 );	// << 7
				col = _mm_mulhi_epi16( col, weight4 );
				color_lo = _mm_adds_epi16( color_lo, col );

				col = _mm_srli_epi16( col4, 8 );	// 00 AA 00 GG
				col = _mm_slli_epi16( col, 7 );	// << 7
				col = _mm_mulhi_epi16( col, weight4 );
				color_hi = _mm_adds_epi16( color_hi, col );
			}
			{	// SSE - 水平加算
				__m128i sumlo = color_lo;
				color_lo = _mm_shuffle_epi32( color_lo, _MM_SHUFFLE(1,0,3,2) ); // 0 1 2 3 + 1 0 3 2
				sumlo = _mm_adds_epi16( sumlo, color_lo );
				color_lo = _mm_shuffle_epi32( sumlo, _MM_SHUFFLE(2,3,0,1) ); // 3 2 1 0
				sumlo = _mm_adds_epi16( sumlo, color_lo );
				sumlo = _mm_adds_epi16( sumlo, fixround );
				sumlo = _mm_srai_epi16( sumlo, 6 ); // 固定小数点から整数化 - << 15, << 7, >> 16 = 6

				__m128i sumhi = color_hi;
				color_hi = _mm_shuffle_epi32( color_hi, _MM_SHUFFLE(1,0,3,2) ); // 0 1 2 3 + 1 0 3 2
				sumhi = _mm_adds_epi16( sumhi, color_hi );
				color_hi = _mm_shuffle_epi32( sumhi, _MM_SHUFFLE(2,3,0,1) ); // 3 2 1 0
				sumhi = _mm_adds_epi16( sumhi, color_hi );
				sumhi = _mm_adds_epi16( sumhi, fixround );
				sumhi = _mm_srai_epi16( sumhi, 6 ); // 固定小数点から整数化

				sumlo = _mm_unpacklo_epi16( sumlo, sumhi );
				sumlo = _mm_packus_epi16( sumlo, sumlo );
				*dstbits = _mm_cvtsi128_si32( sumlo );
			}
			dstbits++;
		}
	}
	/** */
	inline void samplingVertical( int y, tjs_uint32* dstbits, int dstheight, int srcwidth, const tTVPBaseBitmap *src, const tTVPRect &srcrect, const tjs_uint32*& wstarty ) {
		const int top = paramy_.start_[y];
		const int len = paramy_.length_min_[y];
		const int bottom = top + len;
		const tjs_uint32* weighty = wstarty;
		const __m128i cmask = M128_U32_FIXED_COLOR_MASK;
		const __m128i fixround = M128_U32_FIXED_ROUND;
		const tjs_uint32* srctop = (const tjs_uint32*)src->GetScanLine(top) + srcrect.left;
		tjs_int stride = src->GetPitchBytes()/(int)sizeof(tjs_uint32);
		for( int x = 0; x < srcwidth; x+=4 ) {
			weighty = wstarty;
			__m128i color_lo = _mm_setzero_si128();
			__m128i color_hi = _mm_setzero_si128();
			const tjs_uint32* srcbits = &srctop[x];
			for( int sy = top; sy < bottom; sy++ ) {
				__m128i col4 = _mm_loadu_si128( (const __m128i*)srcbits ); // 4列読み込み
				srcbits += stride;
				__m128i weight4 = _mm_set1_epi32( (int)*weighty ); // weight は、同じ値を設定
				weighty++;

				__m128i col = _mm_and_si128( col4, cmask );	// 00 RR 00 BB
				col = _mm_slli_epi16( col, 7 );	// << 7
				col = _mm_mulhi_epi16( col, weight4 );
				color_lo = _mm_adds_epi16( color_lo, col );

				col = _mm_srli_epi16( col4, 8 );	// 00 AA 00 GG
				col = _mm_slli_epi16( col, 7 );	// << 7
				col = _mm_mulhi_epi16( col, weight4 );
				color_hi = _mm_adds_epi16( color_hi, col );
			}
			{
				color_lo = _mm_adds_epi16( color_lo, fixround );
				color_hi = _mm_adds_epi16( color_hi, fixround );
				color_lo = _mm_srai_epi16( color_lo, 6 ); // 固定小数点から整数化 - << 15, << 7, >> 16 = 6
				color_hi = _mm_srai_epi16( color_hi, 6 ); // 固定小数点から整数化
				__m128i lo = _mm_unpacklo_epi16( color_lo, color_hi );
				__m128i hi = _mm_unpackhi_epi16( color_lo, color_hi );
				color_lo = _mm_packus_epi16( lo, hi );
				_mm_store_si128( (__m128i *)&dstbits[x], color_lo );
			}
		}
		wstarty = weighty;
	}

	void ResampleImage( const tTVPResampleClipping &clip, const tTVPImageCopyFuncBase* blendfunc, tTVPBaseBitmap *dest, const tTVPRect &destrect, const tTVPBaseBitmap *src, const tTVPRect &srcrect ) {
		const int srcwidth = srcrect.get_width();
		const int dstheight = destrect.get_height();
		const int alingnwidth = (((srcwidth+3)>>2)<<2) + 3;
#ifdef _DEBUG
		std::vector<tjs_uint32,aligned_allocator<tjs_uint32> > work( alingnwidth );
#else
		std::vector<tjs_uint32,aligned_allocator<tjs_uint32> > work;
		work.reserve( alingnwidth );
#endif
		const tjs_uint32* wstarty = &paramy_.weight_[0];
		// クリッピング部分スキップ
		for( int y = 0; y < clip.offsety_; y++ ) {
			wstarty += paramy_.length_[y];
		}
		tjs_uint32* workbits = &work[0];
		tjs_int dststride = dest->GetPitchBytes()/(int)sizeof(tjs_uint32);
		tjs_uint32* dstbits = (tjs_uint32*)dest->GetScanLineForWrite(clip.dst_top_) + clip.dst_left_;
		if( blendfunc == NULL ) {
			for( int y = clip.offsety_; y < clip.height_; y++ ) {
				samplingVertical( y, workbits, dstheight, srcwidth, src, srcrect, wstarty );
				samplingHorizontal( dstbits, clip.offsetx_, clip.width_, workbits );
				dstbits += dststride;
			}
		} else {	// 単純コピー以外は、一度テンポラリに書き出してから合成する
#ifdef _DEBUG
			std::vector<tjs_uint32> dstwork(clip.getDestWidth()+3);
#else
			std::vector<tjs_uint32> dstwork;
			dstwork.reserve( clip.getDestWidth()+3 );
#endif
			tjs_uint32* midbits = &dstwork[0];	// 途中処理用バッファ
			for( int y = clip.offsety_; y < clip.height_; y++ ) {
				samplingVertical( y, workbits, dstheight, srcwidth, src, srcrect, wstarty );
				samplingHorizontal( midbits, clip.offsetx_, clip.width_, workbits ); // 一時バッファにまずコピー, 範囲外は処理しない
				(*blendfunc)( dstbits, midbits, clip.getDestWidth() );
				dstbits += dststride;
			}
		}
	}
	void ResampleImageMT( const tTVPResampleClipping &clip, const tTVPImageCopyFuncBase* blendfunc, tTVPBaseBitmap *dest, const tTVPRect &destrect, const tTVPBaseBitmap *src, const tTVPRect &srcrect, tjs_int threadNum ) {
		const int srcwidth = srcrect.get_width();
		const int alingnwidth = ((srcwidth+3)>>2)<<2;
		const tjs_uint32* wstarty = &paramy_.weight_[0];
		// クリッピング部分スキップ
		for( int y = 0; y < clip.offsety_; y++ ) {
			wstarty += paramy_.length_[y];
		}
		int offset = clip.offsety_;
		const int height = clip.getDestHeight();

		TVPBeginThreadTask(threadNum);
		std::vector<ThreadParameterHV> params(threadNum);
		for( int i = 0; i < threadNum; i++ ) {
			ThreadParameterHV* param = &params[i];
			param->sampler_ = this;
			param->start_ = height * i / threadNum + offset;
			param->end_ = height * (i + 1) / threadNum + offset;
			param->alingnwidth_ = alingnwidth;
			param->wstarty_ = wstarty;
			param->src_ = src;
			param->srcrect_ = &srcrect;
			param->dest_ = dest;
			param->destrect_ = &destrect;
			param->clip_ = &clip;
			param->blendfunc_ = blendfunc;
			int top = param->start_;
			int bottom = param->end_;
			TVPExecThreadTask(&ResamplerSSE2FixFunc, TVP_THREAD_PARAM(param));
			if( i < (threadNum-1) ) {
				for( int y = top; y < bottom; y++ ) {
					int len = paramy_.length_[y];
					wstarty += len;
				}
			}
		}
		TVPEndThreadTask();
	}
public:
	template<typename TWeightFunc>
	void Resample( const tTVPResampleClipping &clip, const tTVPImageCopyFuncBase* blendfunc, tTVPBaseBitmap *dest, const tTVPRect &destrect, const tTVPBaseBitmap *src, const tTVPRect &srcrect, float tap, TWeightFunc& func ) {
		const int srcwidth = srcrect.get_width();
		const int srcheight = srcrect.get_height();
		const int dstwidth = destrect.get_width();
		const int dstheight = destrect.get_height();

		paramx_.calculateAxis( 0, srcwidth, srcwidth, dstwidth, tap, false, func );
		paramy_.calculateAxis( srcrect.top, srcrect.bottom, srcheight, dstheight, tap, true, func );
		ResampleImage( clip, blendfunc, dest, destrect, src, srcrect );
	}
	template<typename TWeightFunc>
	void ResampleMT( const tTVPResampleClipping &clip, const tTVPImageCopyFuncBase* blendfunc, tTVPBaseBitmap *dest, const tTVPRect &destrect, const tTVPBaseBitmap *src, const tTVPRect &srcrect, float tap, TWeightFunc& func ) {
		const int srcwidth = srcrect.get_width();
		const int srcheight = srcrect.get_height();
		const int dstwidth = destrect.get_width();
		const int dstheight = destrect.get_height();
		int maxwidth = srcwidth > dstwidth ? srcwidth : dstwidth;
		int maxheight = srcheight > dstheight ? srcheight : dstheight;
		int threadNum = 1;
		int pixelNum = maxwidth*(int)tap*maxheight + maxheight*(int)tap*maxwidth;
		if( pixelNum >= 50 * 500 ) {
			threadNum = TVPGetThreadNum();
		}
		if( threadNum == 1 ) { // 面積が少なくスレッドが1の時はそのまま実行
			Resample( clip, blendfunc, dest, destrect, src, srcrect, tap, func );
			return;
		}

		paramx_.calculateAxis( 0, srcwidth, srcwidth, dstwidth, tap, false, func );
		paramy_.calculateAxis( srcrect.top, srcrect.bottom, srcheight, dstheight, tap, true, func );
		ResampleImageMT( clip, blendfunc, dest, destrect, src, srcrect, threadNum );
	}
	void ResampleAreaAvg( const tTVPResampleClipping &clip, const tTVPImageCopyFuncBase* blendfunc, tTVPBaseBitmap *dest, const tTVPRect &destrect, const tTVPBaseBitmap *src, const tTVPRect &srcrect ) {
		const int srcwidth = srcrect.get_width();
		const int srcheight = srcrect.get_height();
		const int dstwidth = destrect.get_width();
		const int dstheight = destrect.get_height();
		if( dstwidth > srcwidth || dstheight > srcheight ) return;

		paramx_.calculateAxisAreaAvg( 0, srcwidth, srcwidth, dstwidth, false );
		paramy_.calculateAxisAreaAvg( srcrect.top, srcrect.bottom, srcheight, dstheight, true );
		ResampleImage( clip, blendfunc, dest, destrect, src, srcrect );
	}
	void ResampleAreaAvgMT( const tTVPResampleClipping &clip, const tTVPImageCopyFuncBase* blendfunc, tTVPBaseBitmap *dest, const tTVPRect &destrect, const tTVPBaseBitmap *src, const tTVPRect &srcrect ) {
		const int srcwidth = srcrect.get_width();
		const int srcheight = srcrect.get_height();
		const int dstwidth = destrect.get_width();
		const int dstheight = destrect.get_height();
		if( dstwidth > srcwidth || dstheight > srcheight ) return;

		int maxwidth = srcwidth > dstwidth ? srcwidth : dstwidth;
		int maxheight = srcheight > dstheight ? srcheight : dstheight;
		int threadNum = 1;
		int pixelNum = maxwidth*maxheight;
		if( pixelNum >= 50 * 500 ) {
			threadNum = TVPGetThreadNum();
		}
		if( threadNum == 1 ) { // 面積が少なくスレッドが1の時はそのまま実行
			ResampleAreaAvg( clip, blendfunc, dest, destrect, src, srcrect );
			return;
		}

		paramx_.calculateAxisAreaAvg( 0, srcwidth, srcwidth, dstwidth, false );
		paramy_.calculateAxisAreaAvg( srcrect.top, srcrect.bottom, srcheight, dstheight, true );
		ResampleImageMT( clip, blendfunc, dest, destrect, src, srcrect, threadNum );
	}
};

void TJS_USERENTRY ResamplerSSE2FixFunc( void* p ) {
	ResamplerSSE2Fix::ThreadParameterHV* param = (ResamplerSSE2Fix::ThreadParameterHV*)p;
	const int alingnwidth = param->alingnwidth_;
#ifdef _DEBUG
	std::vector<tjs_uint32,aligned_allocator<tjs_uint32> > work(alingnwidth);
#else
	std::vector<tjs_uint32,aligned_allocator<tjs_uint32> > work;
	work.reserve( alingnwidth );
#endif

	tTVPBaseBitmap* dest = param->dest_;
	const tTVPRect& destrect = *param->destrect_;
	const tTVPBaseBitmap* src = param->src_;
	const tTVPRect& srcrect = *param->srcrect_;

	const int srcwidth = srcrect.get_width();
	const int dstwidth = destrect.get_width();
	const int dstheight = destrect.get_height();
	const tjs_uint32* wstarty = param->wstarty_;
	tjs_uint32* workbits = &work[0];
	tjs_int dststride = dest->GetPitchBytes()/(int)sizeof(tjs_uint32);
	tjs_uint32* dstbits = (tjs_uint32*)dest->GetScanLineForWrite(param->start_+destrect.top) + param->clip_->dst_left_;
	if( param->blendfunc_ == NULL ) {
		for( int y = param->start_; y < param->end_; y++ ) {
			param->sampler_->samplingVertical( y, workbits, dstheight, srcwidth, src, srcrect, wstarty );
			param->sampler_->samplingHorizontal( dstbits, param->clip_->offsetx_, param->clip_->width_, workbits );
			dstbits += dststride;
		}
	} else {	// 単純コピー以外
#ifdef _DEBUG
		std::vector<tjs_uint32> dstwork(param->clip_->getDestWidth()+3);
#else
		std::vector<tjs_uint32> dstwork;
		dstwork.reserve( param->clip_->getDestWidth()+3 );
#endif
		tjs_uint32* midbits = &dstwork[0];	// 途中処理用バッファ
		for( int y = param->start_; y < param->end_; y++ ) {
			param->sampler_->samplingVertical( y, workbits, dstheight, srcwidth, src, srcrect, wstarty );
			param->sampler_->samplingHorizontal( midbits, param->clip_->offsetx_, param->clip_->width_, workbits ); // 一時バッファにまずコピー, 範囲外は処理しない
			(*param->blendfunc_)( dstbits, midbits, param->clip_->getDestWidth() );
			dstbits += dststride;
		}
	}
}

class ResamplerSSE2 {
	AxisParamSSE2<float> paramx_;
	AxisParamSSE2<float> paramy_;

public:
	/** マルチスレッド化用 */
	struct ThreadParameterHV {
		ResamplerSSE2* sampler_;
		int start_;
		int end_;
		int alingnwidth_;
		
		const float* wstarty_;
		const tTVPBaseBitmap* src_;
		const tTVPRect* srcrect_;
		tTVPBaseBitmap* dest_;
		const tTVPRect* destrect_;

		const tTVPResampleClipping* clip_;
		const tTVPImageCopyFuncBase* blendfunc_;
	};
	/**
	 * 横方向の処理 (後に処理)
	 */
	inline void samplingHorizontal( tjs_uint32* dstbits, const int offsetx, const int dstwidth, const tjs_uint32* srcbits ) {
		const __m128i cmask = M128_U32_FIXED_COLOR_MASK8;	// 8bit化するためのマスク
		const float* weightx = &paramx_.weight_[0];
		// まずoffset分をスキップ
		for( int x = 0; x < offsetx; x++ ) {
			weightx += paramx_.length_[x];
		}
		const tjs_uint32* src = srcbits;
		const __m128i zero = _mm_setzero_si128();
		for( int x = offsetx; x < dstwidth; x++ ) {
			const int left = paramx_.start_[x];
			int right = left + paramx_.length_[x];
			__m128 color_elm = _mm_setzero_ps();
			// 4ピクセルずつ処理する
			for( int sx = left; sx < right; sx+=4 ) {
				__m128i col4 = _mm_loadu_si128( (const __m128i*)&src[sx] ); // 4ピクセル読み込み
				__m128 weight4 = _mm_loadu_ps( (const float*)weightx ); // ウェイト4つ
				weightx += 4;

				// a r g b | a r g b と 2つずつ処理するから、weight もその形にインターリーブ
				__m128i collo = _mm_unpacklo_epi8( col4, zero );		// 00 01 00 02 00 03 0 04 00 05 00 06...
				__m128i col = _mm_unpacklo_epi16( collo, zero );		// 00 00 00 01 00 00 00 02...
				__m128 colf = _mm_cvtepi32_ps( col );
				__m128 wlo = _mm_unpacklo_ps( weight4, weight4 );
				__m128 w = _mm_unpacklo_ps( wlo, wlo );	// 00 00 00 00 04 04 04 04
				colf = _mm_mul_ps( colf, w );
				color_elm = _mm_add_ps( color_elm, colf );
				
				col = _mm_unpackhi_epi16( collo, zero );		// 00 00 00 01 00 00 00 02...
				colf = _mm_cvtepi32_ps( col );				// int to float
				w = _mm_unpackhi_ps( wlo, wlo );
				colf = _mm_mul_ps( colf, w );
				color_elm = _mm_add_ps( color_elm, colf );
				
				__m128i colhi = _mm_unpackhi_epi8( col4, zero );	// 00 01 00 02 00 03 0 04 00 05 00 06...
				col = _mm_unpacklo_epi16( colhi, zero );			// 00 00 00 01 00 00 00 02...
				colf = _mm_cvtepi32_ps( col );					// int to float
				__m128 whi = _mm_unpackhi_ps( weight4, weight4 );
				w = _mm_unpacklo_ps( whi, whi );
				colf = _mm_mul_ps( colf, w );
				color_elm = _mm_add_ps( color_elm, colf );
				
				col = _mm_unpackhi_epi16( colhi, zero );		// 00 00 00 01 00 00 00 02...
				colf = _mm_cvtepi32_ps( col );				// int to float
				w = _mm_unpackhi_ps( whi, whi );
				colf = _mm_mul_ps( colf, w );
				color_elm = _mm_add_ps( color_elm, colf );
			}
			{	// SSE
				__m128i color = _mm_cvtps_epi32( color_elm );
				color = _mm_packs_epi32( color, color );
				color = _mm_packus_epi16( color, color );
				*dstbits = _mm_cvtsi128_si32( color );
			}
			dstbits++;
		}
	}
	/**
	 * 縦方向処理
	 */
	inline void samplingVertical( int y, tjs_uint32* dstbits, int dstheight, int srcwidth, const tTVPBaseBitmap *src, const tTVPRect &srcrect, const float*& wstarty ) {
		const int top = paramy_.start_[y];
		const int len = paramy_.length_min_[y];
		const int bottom = top + len;
		const float* weighty = wstarty;
		const __m128i cmask = M128_U32_FIXED_COLOR_MASK8;
		const tjs_uint32* srctop = (const tjs_uint32*)src->GetScanLine(top) + srcrect.left;
		tjs_int stride = src->GetPitchBytes()/(int)sizeof(tjs_uint32);
		for( int x = 0; x < srcwidth; x+=4 ) {
			weighty = wstarty;
			__m128 color_a = _mm_setzero_ps();
			__m128 color_r = _mm_setzero_ps();
			__m128 color_g = _mm_setzero_ps();
			__m128 color_b = _mm_setzero_ps();
			const tjs_uint32* srcbits = &srctop[x];
			for( int sy = top; sy < bottom; sy++ ) {
				__m128i col4 = _mm_loadu_si128( (const __m128i*)srcbits ); // 8列読み込み
				srcbits += stride;
				__m128 weight4 = _mm_set1_ps( *weighty ); // weight は、同じ値を設定
				weighty++;
				
				__m128i c = _mm_srli_epi32( col4, 24 );
				__m128 cf = _mm_cvtepi32_ps(c);
				cf = _mm_mul_ps( cf, weight4 );
				color_a = _mm_add_ps( color_a, cf );

				c = _mm_srli_epi32( col4, 16 );
				c = _mm_and_si128( c, cmask );
				cf = _mm_cvtepi32_ps(c);
				cf = _mm_mul_ps( cf, weight4 );
				color_r = _mm_add_ps( color_r, cf );
				
				c = _mm_srli_epi32( col4, 8 );
				c = _mm_and_si128( c, cmask );
				cf = _mm_cvtepi32_ps(c);
				cf = _mm_mul_ps( cf, weight4 );
				color_g = _mm_add_ps( color_g, cf );

				c = _mm_and_si128( col4, cmask );
				cf = _mm_cvtepi32_ps(c);
				cf = _mm_mul_ps( cf, weight4 );
				color_b = _mm_add_ps( color_b, cf );
			}
			{
				__m128i a = _mm_cvtps_epi32( color_a );
				__m128i r = _mm_cvtps_epi32( color_r );
				__m128i g = _mm_cvtps_epi32( color_g );
				__m128i b = _mm_cvtps_epi32( color_b );
				// インターリーブ
				__m128i arl = _mm_unpacklo_epi32( r, a );
				__m128i arh = _mm_unpackhi_epi32( r, a );
				arl = _mm_packs_epi32( arl, arh );	// a r a r a r ar
				__m128i gbl = _mm_unpacklo_epi32( b, g );
				__m128i gbh = _mm_unpackhi_epi32( b, g );
				gbl = _mm_packs_epi32( gbl, gbh );	// g b g b g b g b
				__m128i l = _mm_unpacklo_epi32( gbl, arl );
				__m128i h = _mm_unpackhi_epi32( gbl, arl );
				l = _mm_packus_epi16( l, h );
				_mm_store_si128( (__m128i *)&dstbits[x], l );
			}
		}
		wstarty = weighty;
	}
	
	void ResampleImage( const tTVPResampleClipping &clip, const tTVPImageCopyFuncBase* blendfunc, tTVPBaseBitmap *dest, const tTVPRect &destrect, const tTVPBaseBitmap *src, const tTVPRect &srcrect ) {
		const int srcwidth = srcrect.get_width();
		const int dstheight = destrect.get_height();
		const int alingnwidth = (((srcwidth+3)>>2)<<2) + 3;
#ifdef _DEBUG
		std::vector<tjs_uint32,aligned_allocator<tjs_uint32> > work( alingnwidth );
#else
		std::vector<tjs_uint32,aligned_allocator<tjs_uint32> > work;
		work.reserve( alingnwidth );
#endif
		const float* wstarty = &paramy_.weight_[0];
		// クリッピング部分スキップ
		for( int y = 0; y < clip.offsety_; y++ ) {
			wstarty += paramy_.length_[y];
		}
		tjs_uint32* workbits = &work[0];
		tjs_int dststride = dest->GetPitchBytes()/(int)sizeof(tjs_uint32);
		tjs_uint32* dstbits = (tjs_uint32*)dest->GetScanLineForWrite(clip.dst_top_) + clip.dst_left_;
		if( blendfunc == NULL ) {
			for( int y = clip.offsety_; y < clip.height_; y++ ) {
				samplingVertical( y, workbits, dstheight, srcwidth, src, srcrect, wstarty );
				samplingHorizontal( dstbits, clip.offsetx_, clip.width_, workbits );
				dstbits += dststride;
			}
		} else {	// 単純コピー以外は、一度テンポラリに書き出してから合成する
#ifdef _DEBUG
			std::vector<tjs_uint32> dstwork(clip.getDestWidth()+3);
#else
			std::vector<tjs_uint32> dstwork;
			dstwork.reserve( clip.getDestWidth()+3 );
#endif
			tjs_uint32* midbits = &dstwork[0];	// 途中処理用バッファ
			for( int y = clip.offsety_; y < clip.height_; y++ ) {
				samplingVertical( y, workbits, dstheight, srcwidth, src, srcrect, wstarty );
				samplingHorizontal( midbits, clip.offsetx_, clip.width_, workbits ); // 一時バッファにまずコピー, 範囲外は処理しない
				(*blendfunc)( dstbits, midbits, clip.getDestWidth() );
				dstbits += dststride;
			}
		}
	}
	void ResampleImageMT( const tTVPResampleClipping &clip, const tTVPImageCopyFuncBase* blendfunc, tTVPBaseBitmap *dest, const tTVPRect &destrect, const tTVPBaseBitmap *src, const tTVPRect &srcrect, tjs_int threadNum ) {
		const int srcwidth = srcrect.get_width();
		const int alingnwidth = ((srcwidth+3)>>2)<<2;
		const float* wstarty = &paramy_.weight_[0];
		// クリッピング部分スキップ
		for( int y = 0; y < clip.offsety_; y++ ) {
			wstarty += paramy_.length_[y];
		}
		int offset = clip.offsety_;
		const int height = clip.getDestHeight();

		TVPBeginThreadTask(threadNum);
		std::vector<ThreadParameterHV> params(threadNum);
		for( int i = 0; i < threadNum; i++ ) {
			ThreadParameterHV* param = &params[i];
			param->sampler_ = this;
			param->start_ = height * i / threadNum + offset;
			param->end_ = height * (i + 1) / threadNum + offset;
			param->alingnwidth_ = alingnwidth;
			param->wstarty_ = wstarty;
			param->src_ = src;
			param->srcrect_ = &srcrect;
			param->dest_ = dest;
			param->destrect_ = &destrect;
			param->clip_ = &clip;
			param->blendfunc_ = blendfunc;
			int top = param->start_;
			int bottom = param->end_;
			TVPExecThreadTask(&ResamplerSSE2Func, TVP_THREAD_PARAM(param));
			if( i < (threadNum-1) ) {
				for( int y = top; y < bottom; y++ ) {
					int len = paramy_.length_[y];
					wstarty += len;
				}
			}
		}
		TVPEndThreadTask();
	}
public:
	/** 4ラインずつ処理する */
	template<typename TWeightFunc>
	void Resample( const tTVPResampleClipping &clip, const tTVPImageCopyFuncBase* blendfunc, tTVPBaseBitmap *dest, const tTVPRect &destrect, const tTVPBaseBitmap *src, const tTVPRect &srcrect, float tap, TWeightFunc& func ) {
		const int srcwidth = srcrect.get_width();
		const int srcheight = srcrect.get_height();
		const int dstwidth = destrect.get_width();
		const int dstheight = destrect.get_height();
		paramx_.calculateAxis( 0, srcwidth, srcwidth, dstwidth, tap, false, func );
		paramy_.calculateAxis( srcrect.top, srcrect.bottom, srcheight, dstheight, tap, true, func );
		ResampleImage( clip, blendfunc, dest, destrect, src, srcrect );
	}
	template<typename TWeightFunc>
	void ResampleMT( const tTVPResampleClipping &clip, const tTVPImageCopyFuncBase* blendfunc, tTVPBaseBitmap *dest, const tTVPRect &destrect, const tTVPBaseBitmap *src, const tTVPRect &srcrect, float tap, TWeightFunc& func ) {
		const int srcwidth = srcrect.get_width();
		const int srcheight = srcrect.get_height();
		const int dstwidth = destrect.get_width();
		const int dstheight = destrect.get_height();
		int maxwidth = srcwidth > dstwidth ? srcwidth : dstwidth;
		int maxheight = srcheight > dstheight ? srcheight : dstheight;
		int threadNum = 1;
		int pixelNum = maxwidth*(int)tap*maxheight + maxheight*(int)tap*maxwidth;
		if( pixelNum >= 50 * 500 ) {
			threadNum = TVPGetThreadNum();
		}
		if( threadNum == 1 ) { // 面積が少なくスレッドが1の時はそのまま実行
			Resample( clip, blendfunc, dest, destrect, src, srcrect, tap, func );
			return;
		}
		paramx_.calculateAxis( 0, srcwidth, srcwidth, dstwidth, tap, false, func );
		paramy_.calculateAxis( srcrect.top, srcrect.bottom, srcheight, dstheight, tap, true, func );
		ResampleImageMT( clip, blendfunc, dest, destrect, src, srcrect, threadNum );
	}
	
	void ResampleAreaAvg( const tTVPResampleClipping &clip, const tTVPImageCopyFuncBase* blendfunc, tTVPBaseBitmap *dest, const tTVPRect &destrect, const tTVPBaseBitmap *src, const tTVPRect &srcrect ) {
		const int srcwidth = srcrect.get_width();
		const int srcheight = srcrect.get_height();
		const int dstwidth = destrect.get_width();
		const int dstheight = destrect.get_height();
		if( dstwidth > srcwidth || dstheight > srcheight ) return;
		paramx_.calculateAxisAreaAvg( 0, srcwidth, srcwidth, dstwidth, false );
		paramy_.calculateAxisAreaAvg( srcrect.top, srcrect.bottom, srcheight, dstheight, true );
		ResampleImage( clip, blendfunc, dest, destrect, src, srcrect );
	}
	void ResampleAreaAvgMT( const tTVPResampleClipping &clip, const tTVPImageCopyFuncBase* blendfunc, tTVPBaseBitmap *dest, const tTVPRect &destrect, const tTVPBaseBitmap *src, const tTVPRect &srcrect ) {
		const int srcwidth = srcrect.get_width();
		const int srcheight = srcrect.get_height();
		const int dstwidth = destrect.get_width();
		const int dstheight = destrect.get_height();
		if( dstwidth > srcwidth || dstheight > srcheight ) return;
		int maxwidth = srcwidth > dstwidth ? srcwidth : dstwidth;
		int maxheight = srcheight > dstheight ? srcheight : dstheight;
		int threadNum = 1;
		int pixelNum = maxwidth*maxheight;
		if( pixelNum >= 50 * 500 ) {
			threadNum = TVPGetThreadNum();
		}
		if( threadNum == 1 ) { // 面積が少なくスレッドが1の時はそのまま実行
			ResampleAreaAvg( clip, blendfunc, dest, destrect, src, srcrect );
			return;
		}
		paramx_.calculateAxisAreaAvg( 0, srcwidth, srcwidth, dstwidth, false );
		paramy_.calculateAxisAreaAvg( srcrect.top, srcrect.bottom, srcheight, dstheight, true );
		ResampleImageMT( clip, blendfunc, dest, destrect, src, srcrect, threadNum );
	}
};

void TJS_USERENTRY ResamplerSSE2Func( void* p ) {
	ResamplerSSE2::ThreadParameterHV* param = (ResamplerSSE2::ThreadParameterHV*)p;
	const int alingnwidth = param->alingnwidth_;
#ifdef _DEBUG
	std::vector<tjs_uint32,aligned_allocator<tjs_uint32> > work(alingnwidth);
#else
	std::vector<tjs_uint32,aligned_allocator<tjs_uint32> > work;
	work.reserve( alingnwidth );
#endif

	tTVPBaseBitmap* dest = param->dest_;
	const tTVPRect& destrect = *param->destrect_;
	const tTVPBaseBitmap* src = param->src_;
	const tTVPRect& srcrect = *param->srcrect_;

	const int srcwidth = srcrect.get_width();
	const int dstwidth = destrect.get_width();
	const int dstheight = destrect.get_height();
	const float* wstarty = param->wstarty_;
	tjs_uint32* workbits = &work[0];
	tjs_int dststride = dest->GetPitchBytes()/(int)sizeof(tjs_uint32);
	tjs_uint32* dstbits = (tjs_uint32*)dest->GetScanLineForWrite(param->start_+destrect.top) + param->clip_->dst_left_;
	if( param->blendfunc_ == NULL ) {
		for( int y = param->start_; y < param->end_; y++ ) {
			param->sampler_->samplingVertical( y, workbits, dstheight, srcwidth, src, srcrect, wstarty );
			param->sampler_->samplingHorizontal( dstbits, param->clip_->offsetx_, param->clip_->width_, workbits );
			dstbits += dststride;
		}
	} else {	// 単純コピー以外
#ifdef _DEBUG
		std::vector<tjs_uint32> dstwork(param->clip_->getDestWidth()+3);
#else
		std::vector<tjs_uint32> dstwork;
		dstwork.reserve( param->clip_->getDestWidth()+3 );
#endif
		tjs_uint32* midbits = &dstwork[0];	// 途中処理用バッファ
		for( int y = param->start_; y < param->end_; y++ ) {
			param->sampler_->samplingVertical( y, workbits, dstheight, srcwidth, src, srcrect, wstarty );
			param->sampler_->samplingHorizontal( midbits, param->clip_->offsetx_, param->clip_->width_, workbits ); // 一時バッファにまずコピー, 範囲外は処理しない
			(*param->blendfunc_)( dstbits, midbits, param->clip_->getDestWidth() );
			dstbits += dststride;
		}
	}
}

void TVPBicubicResampleSSE2Fix( const tTVPResampleClipping &clip, const tTVPImageCopyFuncBase* blendfunc, tTVPBaseBitmap *dest, const tTVPRect &destrect, const tTVPBaseBitmap *src, const tTVPRect &srcrect, float sharpness ) {
	BicubicWeightSSE weightfunc(sharpness);
	ResamplerSSE2Fix sampler;
	sampler.ResampleMT( clip, blendfunc, dest, destrect, src, srcrect, BicubicWeightSSE::RANGE, weightfunc );
}
void TVPBicubicResampleSSE2( const tTVPResampleClipping &clip, const tTVPImageCopyFuncBase* blendfunc, tTVPBaseBitmap *dest, const tTVPRect &destrect, const tTVPBaseBitmap *src, const tTVPRect &srcrect, float sharpness ) {
	BicubicWeightSSE weightfunc(sharpness);
	ResamplerSSE2 sampler;
	sampler.ResampleMT( clip, blendfunc, dest, destrect, src, srcrect, BicubicWeightSSE::RANGE, weightfunc );
}

void TVPAreaAvgResampleSSE2Fix( const tTVPResampleClipping &clip, const tTVPImageCopyFuncBase* blendfunc, tTVPBaseBitmap *dest, const tTVPRect &destrect, const tTVPBaseBitmap *src, const tTVPRect &srcrect ) {
	ResamplerSSE2Fix sampler;
	sampler.ResampleAreaAvgMT( clip, blendfunc, dest, destrect, src, srcrect );
}
void TVPAreaAvgResampleSSE2( const tTVPResampleClipping &clip, const tTVPImageCopyFuncBase* blendfunc, tTVPBaseBitmap *dest, const tTVPRect &destrect, const tTVPBaseBitmap *src, const tTVPRect &srcrect ) {
	ResamplerSSE2 sampler;
	sampler.ResampleAreaAvgMT( clip, blendfunc, dest, destrect, src, srcrect );
}

template<typename TWeightFunc>
void TVPWeightResampleSSE2Fix( const tTVPResampleClipping &clip, const tTVPImageCopyFuncBase* blendfunc, tTVPBaseBitmap *dest, const tTVPRect &destrect, const tTVPBaseBitmap *src, const tTVPRect &srcrect ) {
	TWeightFunc weightfunc;
	ResamplerSSE2Fix sampler;
	sampler.ResampleMT( clip, blendfunc, dest, destrect, src, srcrect, TWeightFunc::RANGE, weightfunc );
}

template<typename TWeightFunc>
void TVPWeightResampleSSE2( const tTVPResampleClipping &clip, const tTVPImageCopyFuncBase* blendfunc, tTVPBaseBitmap *dest, const tTVPRect &destrect, const tTVPBaseBitmap *src, const tTVPRect &srcrect ) {
	TWeightFunc weightfunc;
	ResamplerSSE2 sampler;
	sampler.ResampleMT( clip, blendfunc, dest, destrect, src, srcrect, TWeightFunc::RANGE, weightfunc );
}
/**
 * 拡大縮小する SSE2 版
 * @param dest : 書き込み先画像
 * @param destrect : 書き込み先矩形
 * @param src : 読み込み元画像
 * @param srcrect : 読み込み元矩形
 * @param type : 拡大縮小フィルタタイプ
 * @param typeopt : 拡大縮小フィルタタイプオプション
 * @param method : ブレンド方法
 * @param opa : 不透明度
 * @param hda : 書き込み先アルファ保持
 */
void TVPResampleImageSSE2( const tTVPResampleClipping &clip, const tTVPImageCopyFuncBase* blendfunc,
	tTVPBaseBitmap *dest, const tTVPRect &destrect, const tTVPBaseBitmap *src, const tTVPRect &srcrect,
	tTVPBBStretchType type, tjs_real typeopt ) {
	switch( type ) {
	case stLinear:
		TVPWeightResampleSSE2<BilinearWeightSSE>(clip, blendfunc, dest, destrect, src, srcrect );
		break;
	case stCubic:
		TVPBicubicResampleSSE2(clip, blendfunc, dest, destrect, src, srcrect, (float)typeopt );
		break;
	case stSemiFastLinear:
		TVPWeightResampleSSE2Fix<BilinearWeightSSE>(clip, blendfunc, dest, destrect, src, srcrect );
		break;
	case stFastCubic:
		TVPBicubicResampleSSE2Fix(clip, blendfunc, dest, destrect, src, srcrect, (float)typeopt );
		break;
	case stLanczos2:
		TVPWeightResampleSSE2<LanczosWeightSSE<2> >(clip, blendfunc, dest, destrect, src, srcrect );
		break;
	case stFastLanczos2:
		TVPWeightResampleSSE2Fix<LanczosWeightSSE<2> >(clip, blendfunc, dest, destrect, src, srcrect );
		break;
	case stLanczos3:
		TVPWeightResampleSSE2<LanczosWeightSSE<3> >(clip, blendfunc, dest, destrect, src, srcrect );
		break;
	case stFastLanczos3:
		TVPWeightResampleSSE2Fix<LanczosWeightSSE<3> >(clip, blendfunc, dest, destrect, src, srcrect );
		break;
	case stSpline16:
		TVPWeightResampleSSE2<Spline16WeightSSE>(clip, blendfunc, dest, destrect, src, srcrect );
		break;
	case stFastSpline16:
		TVPWeightResampleSSE2Fix<Spline16WeightSSE>(clip, blendfunc, dest, destrect, src, srcrect );
		break;
	case stSpline36:
		TVPWeightResampleSSE2<Spline36WeightSSE>(clip, blendfunc, dest, destrect, src, srcrect );
		break;
	case stFastSpline36:
		TVPWeightResampleSSE2Fix<Spline36WeightSSE>(clip, blendfunc, dest, destrect, src, srcrect );
		break;
	case stAreaAvg:
		TVPAreaAvgResampleSSE2(clip, blendfunc, dest, destrect, src, srcrect );
		break;
	case stFastAreaAvg:
		TVPAreaAvgResampleSSE2Fix(clip, blendfunc, dest, destrect, src, srcrect );
		break;
	case stGaussian:
		TVPWeightResampleSSE2<GaussianWeightSSE>(clip, blendfunc, dest, destrect, src, srcrect );
		break;
	case stFastGaussian:
		TVPWeightResampleSSE2Fix<GaussianWeightSSE>(clip, blendfunc, dest, destrect, src, srcrect );
		break;
	case stBlackmanSinc:
		TVPWeightResampleSSE2<BlackmanSincWeightSSE>(clip, blendfunc, dest, destrect, src, srcrect );
		break;
	case stFastBlackmanSinc:
		TVPWeightResampleSSE2Fix<BlackmanSincWeightSSE>(clip, blendfunc, dest, destrect, src, srcrect );
		break;
	default:
		throw L"Not supported yet.";
		break;
	}
}

