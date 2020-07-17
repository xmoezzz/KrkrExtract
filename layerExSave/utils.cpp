#include "ncbind/ncbind.hpp"
#include "utils.hpp"

//----------------------------------------------
// レイヤイメージ操作ユーティリティ

/**
 * レイヤのサイズとバッファを取得する
 */
bool
GetLayerSize(iTJSDispatch2 *lay, long &w, long &h, long &pitch)
{
	// レイヤインスタンス以外ではエラー
	if (!lay || TJS_FAILED(lay->IsInstanceOf(0, 0, 0, TJS_W("Layer"), lay))) return false;

	// レイヤイメージは在るか？
	tTJSVariant val;
	if (TJS_FAILED(lay->PropGet(0, TJS_W("hasImage"), 0, &val, lay)) || (val.AsInteger() == 0)) return false;

	// レイヤサイズを取得
	val.Clear();
	if (TJS_FAILED(lay->PropGet(0, TJS_W("imageWidth"), 0, &val, lay))) return false;
	w = (long)val.AsInteger();

	val.Clear();
	if (TJS_FAILED(lay->PropGet(0, TJS_W("imageHeight"), 0, &val, lay))) return false;
	h = (long)val.AsInteger();

	// ピッチ取得
	val.Clear();
	if (TJS_FAILED(lay->PropGet(0, TJS_W("mainImageBufferPitch"), 0, &val, lay))) return false;
	pitch = (long)val.AsInteger();

	// 正常な値かどうか
	return (w > 0 && h > 0 && pitch != 0);
}

// 読み込み用
bool
GetLayerBufferAndSize(iTJSDispatch2 *lay, long &w, long &h, BufRefT &ptr, long &pitch)
{
	if (!GetLayerSize(lay, w, h, pitch)) return false;

	// バッファ取得
	tTJSVariant val;
	if (TJS_FAILED(lay->PropGet(0, TJS_W("mainImageBuffer"), 0, &val, lay))) return false;
	ptr = reinterpret_cast<BufRefT>(val.AsInteger());
	return  (ptr != 0);
}

// 書き込み用
bool
GetLayerBufferAndSize(iTJSDispatch2 *lay, long &w, long &h, WrtRefT &ptr, long &pitch)
{
	if (!GetLayerSize(lay, w, h, pitch)) return false;

	// バッファ取得
	tTJSVariant val;
	if (TJS_FAILED(lay->PropGet(0, TJS_W("mainImageBufferForWrite"), 0, &val, lay))) return false;
	ptr = reinterpret_cast<WrtRefT>(val.AsInteger());
	return  (ptr != 0);
}

/**
 * 矩形領域の辞書を生成
 */
static void
MakeResult(tTJSVariant *result, long x, long y, long w, long h)
{
	ncbDictionaryAccessor dict;
	dict.SetValue(TJS_W("x"), x);
	dict.SetValue(TJS_W("y"), y);
	dict.SetValue(TJS_W("w"), w);
	dict.SetValue(TJS_W("h"), h);
	*result = dict;
}

/**
 * 不透明チェック関数
 */
static bool
CheckTransp(BufRefT p, long next, long count)
{
	for (; count > 0; count--, p+=next) if (p[3] != 0) return true;
	return false;
}

/**
 * レイヤイメージをクロップ（上下左右の余白透明部分を切り取る）したときのサイズを取得する
 *
 * Layer.getCropRect = function();
 * @return %[ x, y, w, h] 形式の辞書，またはvoid（全部透明のとき）
 */
static tjs_error TJS_INTF_METHOD
GetCropRect(tTJSVariant *result, tjs_int numparams, tTJSVariant **param, iTJSDispatch2 *lay)
{
	// レイヤバッファの取得
	BufRefT p, r = 0;
	long w, h, nl, nc = 4;
	if (!GetLayerBufferAndSize(lay, w, h, r, nl))
		TVPThrowExceptionMessage(TJS_W("Invalid layer image."));

	// 結果領域
	long x1=0, y1=0, x2=w-1, y2=h-1;
	result->Clear();

	for (p=r;             x1 <  w; x1++,p+=nc) if (CheckTransp(p, nl,  h)) break; // 左から透明領域を調べる
	/*                                      */ if (x1 >= w) return TJS_S_OK;      // 全部透明なら void を返す
	for (p=r+x2*nc;       x2 >= 0; x2--,p-=nc) if (CheckTransp(p, nl,  h)) break; // 右から透明領域を調べる
	/*                                      */ long rw = x2 - x1 + 1;             // 左右に挟まれた残りの幅
	for (p=r+x1*nc;       y1 <  h; y1++,p+=nl) if (CheckTransp(p, nc, rw)) break; // 上から透明領域を調べる
	for (p=r+x1*nc+y2*nl; y2 >= 0; y2--,p-=nl) if (CheckTransp(p, nc, rw)) break; // 下から透明領域を調べる

	// 結果を辞書に返す
	MakeResult(result, x1, y1, rw, y2 - y1 + 1);

	return TJS_S_OK;
}

NCB_ATTACH_FUNCTION(getCropRect, Layer, GetCropRect);

/**
 * 不透明チェック関数
 */
static bool
CheckZerop(BufRefT p, long next, long count)
{
	for (; count > 0; count--, p+=next) if (p[3] != 0 || p[2] != 0 || p[1] != 0 || p[0] != 0) return true;
	return false;
}

/**
 * レイヤイメージをクロップ（上下左右の完全透明部分を切り取る）したときのサイズを取得する
 *
 * Layer.getCropRectZero = function();
 * @return %[ x, y, w, h] 形式の辞書，またはvoid（全部透明のとき）
 */
static tjs_error TJS_INTF_METHOD
GetCropRectZero(tTJSVariant *result, tjs_int numparams, tTJSVariant **param, iTJSDispatch2 *lay)
{
	// レイヤバッファの取得
	BufRefT p, r = 0;
	long w, h, nl, nc = 4;
	if (!GetLayerBufferAndSize(lay, w, h, r, nl))
		TVPThrowExceptionMessage(TJS_W("Invalid layer image."));

	// 結果領域
	long x1=0, y1=0, x2=w-1, y2=h-1;
	result->Clear();

	for (p=r;             x1 <  w; x1++,p+=nc) if (CheckZerop(p, nl,  h)) break; // 左から透明領域を調べる
	/*                                      */ if (x1 >= w) return TJS_S_OK;      // 全部透明なら void を返す
	for (p=r+x2*nc;       x2 >= 0; x2--,p-=nc) if (CheckZerop(p, nl,  h)) break; // 右から透明領域を調べる
	/*                                      */ long rw = x2 - x1 + 1;             // 左右に挟まれた残りの幅
	for (p=r+x1*nc;       y1 <  h; y1++,p+=nl) if (CheckZerop(p, nc, rw)) break; // 上から透明領域を調べる
	for (p=r+x1*nc+y2*nl; y2 >= 0; y2--,p-=nl) if (CheckZerop(p, nc, rw)) break; // 下から透明領域を調べる

	// 結果を辞書に返す
	MakeResult(result, x1, y1, rw, y2 - y1 + 1);

	return TJS_S_OK;
}

NCB_ATTACH_FUNCTION(getCropRectZero, Layer, GetCropRectZero);

/**
 * 色比較関数
 */
#define IS_SAME_COLOR(A1,R1,G1,B1, A2,R2,G2,B2) \
	(((A1)==(A2)) && ((A1)==0 || ((R1)==(R2) && (G1)==(G2) && (B1)==(B2))))
//	!((p1[3] != p2[3]) || (p1[3] != 0 && (p1[0] != p2[0] || p1[1] != p2[1] || p1[2] != p2[2])))
//  ((A1 == A2) && (A1 == 0 || (R1==R2 && G1==G2 && B1==B2)))


static bool
CheckDiff(BufRefT p1, long p1n, BufRefT p2, long p2n, long count)
{
	for (; count > 0; count--, p1+=p1n, p2+=p2n)
		if (!IS_SAME_COLOR( p1[3],p1[2],p1[1],p1[0],  p2[3],p2[2],p2[1],p2[0] )) return true;
	return false;
}

/**
 * レイヤの差分領域を取得する
 * 
 * Layer.getDiffRegion = function(base);
 * @param base 差分元となるベース用の画像（インスタンス自身と同じ画像サイズであること）
 * @return %[ x, y, w, h ] 形式の辞書，またはvoid（完全に同じ画像のとき）
 */

static tjs_error TJS_INTF_METHOD
GetDiffRect(tTJSVariant *result, tjs_int numparams, tTJSVariant **param, iTJSDispatch2 *lay)
{
	// 引数の数チェック
	if (numparams < 1) return TJS_E_BADPARAMCOUNT;

	iTJSDispatch2 *base = param[0]->AsObjectNoAddRef();

	// レイヤバッファの取得
	BufRefT fp, tp, fr = 0, tr = 0;
	long w, h, tnl, fw, fh, fnl, nc = 4;
	if (!GetLayerBufferAndSize(lay,   w,  h, tr, tnl) || 
		!GetLayerBufferAndSize(base, fw, fh, fr, fnl))
		TVPThrowExceptionMessage(TJS_W("Invalid layer image."));

	// レイヤのサイズは同じか
	if (w != fw || h != fh)
		TVPThrowExceptionMessage(TJS_W("Different layer size."));

	// 結果領域
	long x1=0, y1=0, x2=w-1, y2=h-1;
	result->Clear();

	for (fp=fr,             tp=tr;              x1 <  w; x1++,fp+=nc, tp+=nc ) if (CheckDiff(fp, fnl, tp, tnl,  h)) break; // 左から透明領域を調べる
	/*                                                                      */ if (x1 >= w) return TJS_S_OK;               // 全部透明なら void を返す
	for (fp=fr+x2*nc,       tp=tr+x2*nc;        x2 >= 0; x2--,fp-=nc, tp-=nc ) if (CheckDiff(fp, fnl, tp, tnl,  h)) break; // 右から透明領域を調べる
	/*                                                                      */ long rw = x2 - x1 + 1;                      // 左右に挟まれた残りの幅
	for (fp=fr+x1*nc,       tp=tr+x1*nc;        y1 <  h; y1++,fp+=fnl,tp+=tnl) if (CheckDiff(fp, nc,  tp, nc,  rw)) break; // 上から透明領域を調べる
	for (fp=fr+x1*nc+y2*fnl,tp=tr+x1*nc+y2*tnl; y2 >= 0; y2--,fp-=fnl,tp-=tnl) if (CheckDiff(fp, nc,  tp, nc,  rw)) break; // 下から透明領域を調べる

	// 結果を辞書に返す
	MakeResult(result, x1, y1, rw, y2 - y1 + 1);

	return TJS_S_OK;
}

NCB_ATTACH_FUNCTION(getDiffRect, Layer, GetDiffRect);

/**
 * レイヤのピクセル比較を行う
 * 
 * Layer.getDiffPixel = function(base, samecol, diffcol);
 * @param base 差分元となるベース用の画像（インスタンス自身と同じ画像サイズであること）
 * @param samecol 同じ場合に塗りつぶす色(0xAARRGGBB)（void・省略なら塗りつぶさない）
 * @param diffcol 違う場合に塗りつぶす色(0xAARRGGBB)（void・省略なら塗りつぶさない）
 * @return 違うピクセルのカウント
 */

static tjs_error TJS_INTF_METHOD
GetDiffPixel(tTJSVariant *result, tjs_int numparams, tTJSVariant **param, iTJSDispatch2 *lay)
{
	DWORD scol = 0, dcol = 0;
	bool sfill = false, dfill = false;
	tTVInteger count = 0;

	// 引数の数チェック
	if (numparams < 1) return TJS_E_BADPARAMCOUNT;

	if (numparams >= 2 && param[1]->Type() != tvtVoid) {
		scol = (DWORD)(param[1]->AsInteger());
		sfill = true;
	}

	if (numparams >= 3 && param[2]->Type() != tvtVoid) {
		dcol = (DWORD)(param[2]->AsInteger());
		dfill = true;
	}

	iTJSDispatch2 *base = param[0]->AsObjectNoAddRef();

	// レイヤバッファの取得
	BufRefT fp, fr = 0;
	WrtRefT tp, tr = 0;
	long w, h, tnl, fw, fh, fnl, nc = 4;
	if (!GetLayerBufferAndSize(lay,   w,  h, tr, tnl) || 
		!GetLayerBufferAndSize(base, fw, fh, fr, fnl))
		TVPThrowExceptionMessage(TJS_W("Invalid layer image."));

	// レイヤのサイズは同じか
	if (w != fw || h != fh)
		TVPThrowExceptionMessage(TJS_W("Different layer size."));

	// 塗りつぶし
	for (long y = 0; (fp=fr, tp=tr, y < h); y++, fr+=fnl, tr+=tnl) {
		for (long x = 0; x < w; x++, fp+=nc, tp+=nc) {
			bool same = IS_SAME_COLOR(fp[3],fp[2],fp[1],fp[0], tp[3],tp[2],tp[1],tp[0]);
			if (      same &&     sfill) *(DWORD*)tp = scol;
			else if (!same) { if (dfill) *(DWORD*)tp = dcol; count++; }
		}
	}
	if (result) *result = count;

	return TJS_S_OK;
}

NCB_ATTACH_FUNCTION(getDiffPixel, Layer, GetDiffPixel);


// 色を加算
static inline void AddColor(DWORD &r, DWORD &g, DWORD &b, BufRefT p) {
	r += p[2], g += p[1], b += p[0];
}

/**
 * レイヤの淵の色を透明部分まで引き伸ばす（縮小時に偽色が出るのを防ぐ）
 * 
 * Layer.oozeColor = function(level, threshold=1);
 * @param level 処理を行う回数。大きいほど引き伸ばし領域が増える
 * @param threshold アルファの閾値(1～255)これより低いピクセルへ引き伸ばす
 * @param fillColor 処理領域以外の塗りつぶし色
 */
static tjs_error TJS_INTF_METHOD
OozeColor(tTJSVariant *result, tjs_int numparams, tTJSVariant **param, iTJSDispatch2 *lay)
{
	// 引数の数チェック
	if (numparams < 1) return TJS_E_BADPARAMCOUNT;
	int level = (int)(param[0]->AsInteger());
	if (level <= 0) 
		TVPThrowExceptionMessage(TJS_W("Invalid level count."));
	unsigned char threshold = (unsigned char)(numparams > 1 ? param[1]->AsInteger() : 1);
	unsigned long fillColor = (unsigned long)(numparams > 2 ? param[2]->AsInteger() : 0);
	unsigned char fillR = (unsigned char)((fillColor >> 16) & 0xff);
	unsigned char fillG = (unsigned char)((fillColor >> 8) & 0xff);
	unsigned char fillB = (unsigned char)((fillColor) & 0xff);
	if (threshold < 1)
		threshold = 1;
	else if (threshold > 255)
		threshold = 255;

	// レイヤバッファの取得
	WrtRefT p, r = 0;
	long x, y, w, h, nl, nc = 4, ow, oh;
	if (!GetLayerBufferAndSize(lay, w, h, r, nl))
		TVPThrowExceptionMessage(TJS_W("Invalid layer image."));

	ow = w+2, oh = h+2; // oozed map のサイズ
	char *o, *otop, *oozed = new char[ow*oh];
	otop = oozed + ow + 1; // oozed map 左上
	ZeroMemory(oozed, ow*oh); // クリア
	try {
		// アルファマップを調べる
		for (y = 0; y < h; y++) {
			o = otop + y*ow;
			p = r    + y*nl;
			for (x = 0; x < w; x++, o++, p+=nc) {
				if (p[3] >= threshold) *o = -1;
				else {
					p[2] = fillR;
					p[1] = fillG;
					p[0] = fillB; // 閾値以下の不透明部分の色を指定色でクリア
				}
			}
		}

		// 引き伸ばし処理
		for (int i = 0; i < level; i++) {
			bool L, R, U, D;
			for (y = 0; y < h; y++) {
				o = otop + y*ow;
				p = r    + y*nl;
				for (x = 0; x < w; x++, p+=nc, o++) {
					// 未処理領域をチェック
					if (!*o) {
						DWORD cr = 0, cg = 0, cb = 0;
						// 上下左右の領域をチェック
						U=o[-ow]<0, D=o[ow]<0, L=o[-1]<0, R=o[1]<0;
						if (U || D || L || R) {
							int cnt = 0;
							if (U) AddColor(cr, cg, cb, p-nl), cnt++;
							if (D) AddColor(cr, cg, cb, p+nl), cnt++;
							if (L) AddColor(cr, cg, cb, p-nc), cnt++;
							if (R) AddColor(cr, cg, cb, p+nc), cnt++;
							p[2] = (unsigned char)(cr / cnt);
							p[1] = (unsigned char)(cg / cnt);
							p[0] = (unsigned char)(cb / cnt);
							*o = 1;
						}
					}
				}
			}
			// 処理済マップの値を再設定
			for (y = 0; y < h; y++) {
				o = otop + y*ow;
				for (x = 0; x < w; x++, o++) if (*o>0) *o=-1;
			}
		}
	} catch (...) {
		delete[] oozed;
		throw;
	}
	delete[] oozed;

	return TJS_S_OK;
}

NCB_ATTACH_FUNCTION(oozeColor, Layer, OozeColor);

/**
 * Layer.copyAlpha = function(src);
 * src の B値を α領域にコピーする
 */
static tjs_error TJS_INTF_METHOD
CopyBlueToAlpha(tTJSVariant *result, tjs_int numparams, tTJSVariant **param, iTJSDispatch2 *lay)
{
	if (numparams < 0) {
		return TJS_E_BADPARAMCOUNT;
	}

	// 読み込みもと
	BufRefT sbuf = 0;
	long sw, sh, spitch;
	if (!GetLayerBufferAndSize(param[0]->AsObjectNoAddRef(), sw, sh, sbuf, spitch)) {
		TVPThrowExceptionMessage(TJS_W("src must be Layer."));
	}
	// 書き込み先
	WrtRefT dbuf = 0;
	long dw, dh, dpitch;
	if (!GetLayerBufferAndSize(lay, dw, dh, dbuf, dpitch)) {
		TVPThrowExceptionMessage(TJS_W("dest must be Layer."));
	}

	// 小さい領域分
	int w = (sw < dw ? sw : dw);
	int h = sh < dh ? sh : dh;
	// コピー
	for (int i=0;i<h;i++) {
		BufRefT p = sbuf;     // B領域
		WrtRefT q = dbuf+3;   // A領域
		for (int j=0;j<w;j++) {
			*q = *p;
			p += 4;
			q += 4;
		}
		sbuf += spitch;
		dbuf += dpitch;
	}
	return TJS_S_OK;
}

NCB_ATTACH_FUNCTION(copyBlueToAlpha, Layer, CopyBlueToAlpha);

/**
 * Layer.isBlank = function(x,y,w,h);
 * 指定領域がブランクデータかどうか確認する
 */
static tjs_error TJS_INTF_METHOD
isBlank(tTJSVariant *result, tjs_int numparams, tTJSVariant **param, iTJSDispatch2 *objthis)
{
	if (numparams < 4) {
		return TJS_E_BADPARAMCOUNT;
	}

	// 読み込みもと
	BufRefT sbuf = 0;
	long sw, sh, spitch;
	if (!GetLayerBufferAndSize(objthis, sw, sh, sbuf, spitch)) {
		TVPThrowExceptionMessage(TJS_W("src must be Layer."));
	}

	tjs_int left   = *param[0];
	tjs_int top    = *param[1];
	tjs_int width  = *param[2];
	tjs_int height = *param[3];
	
	// 範囲チェック
	if (left < 0 || top < 0 || width < 0 || top < 0 ||
		left + width > sw || top + height > sh)
		TVPThrowExceptionMessage(L"invalid layer range");

	// 判定処理
	for (tjs_int y = top; y < top + height; y++) {
		BufRefT buffer = sbuf + left * 4 + spitch * y;
		for (tjs_int x = left; x < left + width; x++, buffer += 4) {
			if (*buffer) {
				if (result) {
					*result = 0;
				}
				return TJS_S_OK;
			}
		}
	}
	
	if (result) {
		*result = 1;
	}
	return TJS_S_OK;
}

NCB_ATTACH_FUNCTION(isBlank, Layer, isBlank);

/**
 * Layer.clearAlpha = function (threthold, fillColor=0)
 * αが指定より小さい部分を完全透明化する
 */
static tjs_error TJS_INTF_METHOD
clearAlpha(tTJSVariant *result, tjs_int numparams, tTJSVariant **param, iTJSDispatch2 *lay)
{
	int threthold = numparams <= 0 ? 0 : *param[0];
	unsigned long fillColor = (unsigned long) ((numparams > 1 ? param[1]->AsInteger() : 0) & 0xffffff);
	
	// 書き込み先
	WrtRefT dbuf = 0;
	long w, h, pitch;
	if (!GetLayerBufferAndSize(lay, w, h, dbuf, pitch)) {
		TVPThrowExceptionMessage(TJS_W("dest must be Layer."));
	}

	// コピー
	for (int i=0;i<h;i++) {
		WrtRefT q = dbuf;   // A領域
		for (int j=0;j<w;j++) {
			if (q[3] <= threthold) {
				*((unsigned long*)q) = fillColor;
			}
			q += 4;
		}
		dbuf += pitch;
	}
	return TJS_S_OK;
}

NCB_ATTACH_FUNCTION(clearAlpha, Layer, clearAlpha);
