#include "ncbind.h"
#include "savetlg5.h"

#include "slide.h"
#define BLOCK_HEIGHT 4
//---------------------------------------------------------------------------

bool CompressTLG5::main(long width, long height, BufRefT buffer, long pitch) {

	bool canceled = false;

	// ARGB 固定
	int colors = 4;

	// header
	{
		writeBuffer("TLG5.0\x00raw\x1a\x00", 11);
		writeInt8(colors);
		writeInt32(width);
		writeInt32(height);
		writeInt32(BLOCK_HEIGHT);
	}

	int blockcount = (int)((height - 1) / BLOCK_HEIGHT) + 1;

	// buffers/compressors
	SlideCompressor * compressor = NULL;
	unsigned char *cmpinbuf[4];
	unsigned char *cmpoutbuf[4];
	for (int i = 0; i < colors; i++) {
		cmpinbuf[i] = cmpoutbuf[i] = NULL;
	}
	long written[4];
	int *blocksizes;

	// allocate buffers/compressors
	compressor = new SlideCompressor();
	for (int i = 0; i < colors; i++)	{
		cmpinbuf[i] = new unsigned char[width * BLOCK_HEIGHT];
		cmpoutbuf[i] = new unsigned char[width * BLOCK_HEIGHT * 9 / 4];
		written[i] = 0;
	}
	blocksizes = new int[blockcount];

	// ブロックサイズの位置を記録
	ULONG blocksizepos = cur;

	cur += blockcount * 4;

		//
	int block = 0;
	for (int blk_y = 0; blk_y < height; blk_y += BLOCK_HEIGHT, block++) {
		if (doProgress(blk_y * 100 / height)) {
			canceled = true;
			break;
		}
		int ylim = blk_y + BLOCK_HEIGHT;
		if (ylim > height) ylim = height;

		int inp = 0;

		for (int y = blk_y; y < ylim; y++) {
			// retrieve scan lines
			const unsigned char * upper;
			if (y != 0) {
				upper = (const unsigned char *)buffer;
				buffer += pitch;
			}
			else {
				upper = NULL;
			}
			const unsigned char * current;
			current = (const unsigned char *)buffer;

			// prepare buffer
			int prevcl[4];
			int val[4];

			for (int c = 0; c < colors; c++) prevcl[c] = 0;

			for (int x = 0; x < width; x++) {
				for (int c = 0; c < colors; c++) {
					int cl;
					if (upper)
						cl = 0[current++] - 0[upper++];
					else
						cl = 0[current++];
					val[c] = cl - prevcl[c];
					prevcl[c] = cl;
				}
				// composite colors
				switch (colors){
					case 1:
					cmpinbuf[0][inp] = val[0];
					break;
				case 3:
					cmpinbuf[0][inp] = val[0] - val[1];
					cmpinbuf[1][inp] = val[1];
					cmpinbuf[2][inp] = val[2] - val[1];
					break;
				case 4:
					cmpinbuf[0][inp] = val[0] - val[1];
					cmpinbuf[1][inp] = val[1];
					cmpinbuf[2][inp] = val[2] - val[1];
					cmpinbuf[3][inp] = val[3];
					break;
				}
				inp++;
			}
		}

		// compress buffer and write to the file

		// LZSS
		int blocksize = 0;
		for (int c = 0; c < colors; c++) {
			long wrote = 0;
			compressor->Store();
			compressor->Encode(cmpinbuf[c], inp,
				cmpoutbuf[c], wrote);
			if (wrote < inp)	{
				writeInt8(0x00);
				writeInt32(wrote);
				writeBuffer((const char *)cmpoutbuf[c], wrote);
				blocksize += wrote + 4 + 1;
			}
			else {
				compressor->Restore();
				writeInt8(0x01);
				writeInt32(inp);
				writeBuffer((const char *)cmpinbuf[c], inp);
				blocksize += inp + 4 + 1;
			}
			written[c] += wrote;
		}

		blocksizes[block] = blocksize;
	}

	if (!canceled) {
		// ブロックサイズ格納
		for (int i = 0; i < blockcount; i++) {
			writeInt32(blocksizes[i], blocksizepos);
			blocksizepos += 4;
		}
	}

	for (int i = 0; i < colors; i++) {
		if (cmpinbuf[i]) delete[] cmpinbuf[i];
		if (cmpoutbuf[i]) delete[] cmpoutbuf[i];
	}
	if (compressor) delete compressor;
	if (blocksizes) delete[] blocksizes;

	doProgress(100);

	return canceled;
}

/**
* 画像情報の書き出し
* @param width 画像横幅
* @param height 画像縦幅
* @param buffer 画像バッファ
* @param pitch 画像データのピッチ
* @param tagsDict タグ情報
* @return キャンセルされたら true
*/
bool CompressTLG5::compress(long width, long height, BufRefT buffer, long pitch, iTJSDispatch2 *tagsDict) {

	bool canceled = false;

	// 取得
	ttstr tags;
	if (tagsDict) {
		/**
		* タグ展開用
		*/
		class TagsCaller : public tTJSDispatch /** EnumMembers 用 */ {
		protected:
			ttstr *store;
		public:
			TagsCaller(ttstr *store) : store(store) {};
			virtual tjs_error TJS_INTF_METHOD FuncCall( // function invocation
				tjs_uint32 flag,			// calling flag
				const tjs_char * membername,// member name ( NULL for a default member )
				tjs_uint32 *hint,			// hint for the member name (in/out)
				tTJSVariant *result,		// result
				tjs_int numparams,			// number of parameters
				tTJSVariant **param,		// parameters
				iTJSDispatch2 *objthis		// object as "this"
				) {
				if (numparams > 1) {
					tTVInteger flag = param[1]->AsInteger();
					if (!(flag & TJS_HIDDENMEMBER)) {
						ttstr name = *param[0];
						ttstr value = *param[2];
						*store += ttstr(name.GetNarrowStrLen()) + ":" + name + "=" + ttstr(value.GetNarrowStrLen()) + ":" + value + ",";
					}
				}
				if (result) {
					*result = true;
				}
				return TJS_S_OK;
			}
		} *caller = new TagsCaller(&tags);
		tTJSVariantClosure closure(caller);
		tagsDict->EnumMembers(TJS_IGNOREPROP, &closure, tagsDict);
		caller->Release();
	}

	ULONG tagslen = tags.GetNarrowStrLen();
	if (tagslen > 0) {
		// write TLG0.0 Structured Data Stream header
		writeBuffer("TLG0.0\x00sds\x1a\x00", 11);
		ULONG rawlenpos = cur;
		cur += 4;
		// write raw TLG stream
		if (!(canceled = main(width, height, buffer, pitch))) {
			// write raw data size
			writeInt32(cur - rawlenpos - 4, rawlenpos);
			// write "tags" chunk name
			writeBuffer("tags", 4);
			// write chunk size
			writeInt32(tagslen);
			// write chunk data
			resize(cur + tagslen);
			tags.ToNarrowStr((tjs_nchar*)&data[cur], tagslen);
			cur += tagslen;
		}
	}
	else {
		// write raw TLG stream
		canceled = main(width, height, buffer, pitch);
	}
	return canceled;
}


//---------------------------------------------------------------------------
// レイヤ拡張
//---------------------------------------------------------------------------

/**
* TLG5 形式での画像の保存。注意点:データの保存が終わるまで処理が帰りません。
* @param filename ファイル名
* @param tags タグ情報
*/
static tjs_error TJS_INTF_METHOD saveLayerImageTlg5Func(tTJSVariant *result,
	tjs_int numparams,
	tTJSVariant **param,
	iTJSDispatch2 *objthis) {
	if (numparams < 1) return TJS_E_BADPARAMCOUNT;
	CompressAndSave<CompressTLG5>::saveLayerImage(
		objthis, // layer
		param[0]->GetString(),  // filename
		numparams > 1 ? param[1]->AsObjectNoAddRef() : NULL // info
		);
	return TJS_S_OK;
}

NCB_ATTACH_FUNCTION(saveLayerImageTlg5, Layer, saveLayerImageTlg5Func);

