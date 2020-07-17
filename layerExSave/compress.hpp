#ifndef _layerexsave_compress_hpp_
#define _layerexsave_compress_hpp_

#include "utils.hpp"

#include <vector>

typedef bool ProgressFunc(int percent, void *userdata);

class CompressBase {
	enum { INITIAL_DATASIZE = 1024*100 };

protected:
	ProgressFunc *progress;
	void         *progressData;

	typedef unsigned char BYTE;
	typedef std::vector<BYTE> DATA;
	DATA data;      //< 格納データ
	ULONG cur;      //< 格納位置
	ULONG size;     //< 格納サイズ
	ULONG dataSize; //< データ領域確保サイズ

public:
	/**
	 * コンストラクタ
	 */
	CompressBase(ProgressFunc *_progress=NULL, void *_progressData=NULL)
		: progress(_progress), progressData(_progressData),
		  cur(0), size(0), dataSize(INITIAL_DATASIZE)
	{
		data.resize(dataSize);
	}
	CompressBase(CompressBase const *ref)
		: progress(ref->progress), progressData(ref->progressData),
		  cur(0), size(0), dataSize(INITIAL_DATASIZE)
	{
		data.resize(dataSize);
	}

	/**
	 * デストラクタ
	 */
	virtual ~CompressBase() {}

	/**
	 * プログレス処理
	 * @return キャンセルされた
	 */
	bool doProgress(int percent) {
		return (progress && progress(percent, progressData));
	}

	/**
	 * サイズ変更
	 * 指定位置がはいるだけのサイズを確保する。
	 * 指定した最大サイズを保持する。
	 * @param サイズ
	 */
	inline void resize(size_t s) {
		if (s > size) {
			size = s;
			if (size > dataSize) {
				dataSize = size * 2;
				data.resize(dataSize);
			}
		}
	}

	/**
	 * 8bit数値の書き出し
	 * @param num 数値
	 */
	template <typename ANYINT>
	inline void writeInt8(ANYINT num) {
		resize(cur + 1);
		data[cur++] = num & 0xff;
	}
	
	/**
	 * 32bit数値の書き出し
	 * @param num 数値
	 */
	template <typename ANYINT>
	inline void writeInt32(ANYINT num) {
		writeInt32(num, cur);
		cur += 4;
	}
	template <typename ANYINT>
	inline void writeBigInt32(ANYINT num) {
		writeBigInt32(num, cur);
		cur += 4;
	}

	/**
	 * 32bit数値の書き出し
	 * @param num 数値
	 */
	template <typename ANYINT>
	inline void writeInt32(ANYINT num, int cur) {
		resize(cur + 4);
		data[cur++] =  num        & 0xff;
		data[cur++] = (num >> 8)  & 0xff;
		data[cur++] = (num >> 16) & 0xff;
		data[cur++] = (num >> 24) & 0xff;
	}

	/**
	 * 32bit数値の書き出し
	 * @param num 数値
	 */
	template <typename ANYINT>
	inline void writeBigInt32(ANYINT num, int cur) {
		resize(cur + 4);
		data[cur++] = (num >> 24) & 0xff;
		data[cur++] = (num >> 16) & 0xff;
		data[cur++] = (num >> 8)  & 0xff;
		data[cur++] =  num        & 0xff;
	}

	/**
	 * バッファの書き出し
	 * @param buf バッファ
	 * @param size 出力バイト数
	 */
	void writeBuffer(const void *buf, int size) {
		resize(cur + size);
		memcpy((void*)&data[cur], buf, size);
		cur += size;
	}

	/**
	 * データをファイルに書き出す
	 * @param out 出力先ストリーム
	 */
	void store(IStream *out) {
		ULONG s;
		out->Write(&data[0], size, &s);
	}

	/**
	 * 圧縮処理
	 * @param width 画像横幅
	 * @param height 画像縦幅
	 * @param buffer 画像バッファ
	 * @param pitch 画像データのピッチ
	 * @param tagsDict タグ情報
	 * @return キャンセルされたら true
	 */
	virtual bool compress(long width, long height, BufRefT buffer, long pitch, iTJSDispatch2 *tagsDict) = 0;

	/**
	 * ファイルに保存する
	 */
	bool save(iTJSDispatch2 *layer, const tjs_char *filename, iTJSDispatch2 *info) {
		// レイヤ画像情報
		BufRefT buffer;
		long width, height, pitch;
		if (!GetLayerBufferAndSize(layer, width, height, buffer, pitch)) {
			ttstr msg = filename;
			msg += L":invalid layer";
			TVPThrowExceptionMessage(msg.c_str());
		}
		bool canceled = compress(width, height, buffer, pitch, info);

		// 圧縮がキャンセルされていなければファイル保存
		if (!canceled) {
			IStream *out = TVPCreateIStream(filename, TJS_BS_WRITE);
			if (!out) {
				ttstr msg = filename;
				msg += L":can't open";
				TVPThrowExceptionMessage(msg.c_str());
			}
			try {
				// 格納
				store(out);
			} catch (...) {
				out->Release();
				throw;
			}
			out->Release();
		}

		return canceled;
	}

};

/**
 * 任意の型で保存
 */
template <class COMPRESS>
struct CompressAndSave {
	typedef COMPRESS CompressClass;
	static bool saveLayerImage(iTJSDispatch2 *layer, const tjs_char *filename, iTJSDispatch2 *info, ProgressFunc *progress=NULL, void *progressData=NULL) {
		CompressClass work(progress, progressData);
		return        work.save(layer, filename, info);
	}
};

#endif
