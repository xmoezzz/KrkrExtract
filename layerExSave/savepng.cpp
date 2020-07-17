#include "ncbind/ncbind.hpp"
#include "savepng.hpp"

#include "zlib/zlib.h"

#define PNGTYPE_RGBA8888 (0x08060000L)

//---------------------------------------------------------------------------
// 圧縮処理用

class PngChunk : public CompressBase {
	int level;
	enum {
		DEFLATE_INSTEP  = (4096),
		DEFLATE_OUTSTEP = (256*1024)
	};
public:
	PngChunk(CompressBase const *ref)
		: CompressBase(ref), level(Z_DEFAULT_COMPRESSION) { init(); }

	virtual ~PngChunk() {}
	void init() {
		resize(4);
		size = cur = 4;
	}
	void setCompressionLevel(int lv) {
		level = lv;
	}
	void writeChunk(CompressBase *target, const char *chunk) {
		writeInt32(*(DWORD*)chunk, 0);
		unsigned long crc = crc32(0, &data[0], size);

		target->writeBigInt32(size-4);
		target->writeBuffer(&data[0], size);
		target->writeBigInt32(crc);
		init();
	}
	void writeUnitType(ncbPropAccessor &dic, const tjs_char *tag, const tjs_char *oneval) {
		writeInt8(dic.getStrValue(tag) == ttstr(oneval) ? 1 : 0);
	}
	inline void writePixel(BufRefT p) {
		resize(cur + 4);
		data[cur++] = p[2];
		data[cur++] = p[1];
		data[cur++] = p[0];
		data[cur++] = p[3];
	}
	bool deflate() {
		z_stream zs;
		ZeroMemory(&zs, sizeof(zs));
		if (::deflateInit(&zs, level) != Z_OK)
			TVPThrowExceptionMessage(L"deflate initialize");

		unsigned char const * in   = &data[4];
		unsigned long         rest = size-4;
		unsigned long         all  = rest;
		int s = Z_OK, f = Z_NO_FLUSH;

		DATA out;
		bool canceled = false;

		do {
			if (!zs.avail_in) {
				zs.avail_in = DEFLATE_INSTEP;
				zs.next_in  = (Bytef*)in;
				if (zs.avail_in >= rest) {
					zs.avail_in  = rest;
					f = Z_FINISH;
				}
				rest -= zs.avail_in;
				in   += zs.avail_in;
				if (all > 0 && doProgress((all-rest)*100/all)) {
					canceled = true;
					break;
				}
			}
			if (!zs.avail_out) {
				unsigned long cnt = zs.total_out;
				out.resize(out.size() + DEFLATE_OUTSTEP);
				zs.next_out  = (Bytef*)(&out[0]) + cnt;
				zs.avail_out = out.size()        - cnt;
			}

		} while ((s = ::deflate(&zs, f)) == Z_OK);
		::deflateEnd(&zs);
		if (!canceled) {
			doProgress(100);
			if (s == Z_STREAM_END) {
				unsigned long cnt = zs.total_out;
				resize(cnt + 4);
				memcpy((void*)&data[4], &out[0], cnt);
				size = cnt + 4;
			}
		}
		return canceled;
	}
	virtual bool compress(long width, long height, BufRefT buffer, long pitch, iTJSDispatch2 *tagsDict) {
		return false;
	}
};

/**
 * 画像情報の書き出し
 * @param width 画像横幅
 * @param height 画像縦幅
 * @param buffer 画像バッファ
 * @param pitch 画像データのピッチ
 * @param tagsDict タグ情報
 * @return キャンセルされたら true
 */
bool CompressPNG::compress(long width, long height, BufRefT buffer, long pitch, iTJSDispatch2 *tagsDict)
{
	PngChunk chunk(this);
	/**/   compress_first (chunk, width, height, PNGTYPE_RGBA8888);
	/**/   compress_second(chunk, tagsDict);
	return compress_third (chunk, width, height, buffer, pitch);
}

void CompressPNG::compress_first (PngChunk &chunk, long width, long height, long flag)
{
	// PNG signature
	writeBuffer("\x89PNG\x0D\x0A\x1A\x0A", 8);

	// IHDR chunk
	chunk.writeBigInt32(width );
	chunk.writeBigInt32(height);
	chunk.writeBigInt32(flag  );
	chunk.writeInt8(0); // non interlace
	chunk.writeChunk(this, "IHDR");
}
void CompressPNG::compress_second(PngChunk &chunk, iTJSDispatch2 *tagsDict)
{
	// additional chunks
	if (tagsDict == NULL) return;
	ncbPropAccessor dic(tagsDict);

	// pHYs chunk
	if (dic.HasValue(TJS_W("reso_x")) || dic.HasValue(TJS_W("reso_y"))) {
		tjs_int x = dic.getIntValue(TJS_W("reso_x"));
		tjs_int y = dic.getIntValue(TJS_W("reso_y"));
		chunk.writeBigInt32((long)x);
		chunk.writeBigInt32((long)y);
		chunk.writeUnitType(dic, TJS_W("reso_unit"), TJS_W("meter"));
		chunk.writeChunk(this, "pHYs");
	}

	// oFFs chunk
	if (dic.HasValue(TJS_W("offs_x")) || dic.HasValue(TJS_W("offs_y"))) {
		tjs_int x = dic.getIntValue(TJS_W("offs_x"));
		tjs_int y = dic.getIntValue(TJS_W("offs_y"));
		chunk.writeBigInt32((long)x);
		chunk.writeBigInt32((long)y);
		chunk.writeUnitType(dic, TJS_W("offs_unit"), TJS_W("micrometer"));
		chunk.writeChunk(this, "oFFs");
	}

	// vpAg chunk
	if (dic.HasValue(TJS_W("vpag_w")) || dic.HasValue(TJS_W("vpag_h"))) {
		tjs_int w = dic.getIntValue(TJS_W("vpag_w"));
		tjs_int h = dic.getIntValue(TJS_W("vpag_h"));
		chunk.writeBigInt32((long)w);
		chunk.writeBigInt32((long)h);
		chunk.writeUnitType(dic, TJS_W("vpag_unit"), TJS_W("micrometer"));
		chunk.writeChunk(this, "vpAg");
	}

	// compression level
	chunk.setCompressionLevel((int)dic.getIntValue(TJS_W("comp_lv"), Z_DEFAULT_COMPRESSION));
}
bool CompressPNG::compress_third (PngChunk &chunk, long width, long height, BufRefT buffer, long pitch)
{
	// IDAT chunk
	for(long y = 0; y < height; y++) {
		BufRefT p = buffer + pitch * y;
		chunk.writeInt8(0);
		for(long x = 0; x < width; x++, p+=4)
			chunk.writePixel(p);
	}
	bool r = chunk.deflate();
	if (!r) {
		chunk.writeChunk(this, "IDAT");
		chunk.writeChunk(this, "IEND");
	}
	return r;
}

static void b64e(tjs_char *p, unsigned char const *r, long len) {
	tjs_char *b64 = TJS_W("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=");
	long i, stop = len - 3;
	for (i = 0; i <= stop; i+=3) {
		*p++ = b64[ (r[i  ] >> 2) & 0x3F ];
		*p++ = b64[((r[i  ] << 4) & 0x30) | ((r[i+1] >> 4) & 0x0F)];
		*p++ = b64[((r[i+1] << 2) & 0x3C) | ((r[i+2] >> 6) & 0x03)];
		*p++ = b64[ (r[i+2]     ) & 0x3F ];
	}
	switch (len - i) {
	case 2:
		*p++ = b64[ (r[i  ] >> 2) & 0x3F ];
		*p++ = b64[((r[i  ] << 4) & 0x30) | ((r[i+1] >> 4) & 0x0F)];
		*p++ = b64[ (r[i+1] << 2) & 0x3C ];
		*p++ = b64[64];
		*p++ = 0;
		break;
	case 1:
		*p++ = b64[ (r[i  ] >> 2) & 0x3F ];
		*p++ = b64[ (r[i  ] << 4) & 0x30 ];
		*p++ = b64[64];
		*p++ = b64[64];
		*p++ = 0;
		break;
	case 0:
		*p++ = 0;
	}
}

void CompressPNG::encodeToOctet(iTJSDispatch2 *layer, int comp_lv, tTJSVariant &ret)
{
	BufRefT buffer;
	long width, height, pitch;

	ret = TJS_W("");
	if (GetLayerBufferAndSize(layer, width, height, buffer, pitch)) {
		PngChunk chunk(this);
		chunk.setCompressionLevel(comp_lv);

		compress_first (chunk, width, height, PNGTYPE_RGBA8888);
		compress_third (chunk, width, height, buffer, pitch);

		tTJSVariantOctet *oct = TJSAllocVariantOctet(&data[0], size);
		ret = oct;
		oct->Release();
	}
}

//---------------------------------------------------------------------------
// レイヤ拡張
//---------------------------------------------------------------------------

/**
 * PNG 形式での画像の保存。注意点:データの保存が終わるまで処理が帰りません。
 * @param filename ファイル名
 * @param tags タグ情報
 */
static tjs_error TJS_INTF_METHOD saveLayerImagePngFunc(tTJSVariant *result,
														tjs_int numparams,
														tTJSVariant **param,
														iTJSDispatch2 *objthis) {
	if (numparams < 1) return TJS_E_BADPARAMCOUNT;
	CompressAndSave<CompressPNG>::saveLayerImage(
		objthis, // layer
		param[0]->GetString(),  // filename
		numparams > 1 ? param[1]->AsObjectNoAddRef() : NULL // info
		);
	return TJS_S_OK;
}

NCB_ATTACH_FUNCTION(saveLayerImagePng,       Layer, saveLayerImagePngFunc);

/**
 * PNG 形式画像をoctetで返す。注意点:データの保存が終わるまで処理が帰りません。
 * @param compression_level 圧縮率
 */
static tjs_error TJS_INTF_METHOD saveLayerImagePngOctet(tTJSVariant *result,
														tjs_int numparams,
														tTJSVariant **param,
														iTJSDispatch2 *objthis)
{
	int comp_lv = (numparams >= 1) ? (int)param[0]->AsInteger() : 1;
	if (result) {
		CompressPNG png;
		png.encodeToOctet(objthis, comp_lv, *result);
	}
	return TJS_S_OK;
}
NCB_ATTACH_FUNCTION(saveLayerImagePngOctet, Layer, saveLayerImagePngOctet);

