

#ifndef __BITMAP_INFOMATION_H__
#define __BITMAP_INFOMATION_H__

class BitmapInfomation {
	BITMAPINFO* BitmapInfo;
	tjs_int BitmapInfoSize;
public:
	BitmapInfomation( tjs_uint width, tjs_uint height, int bpp );
	~BitmapInfomation();

	inline unsigned int GetBPP() const { return BitmapInfo->bmiHeader.biBitCount; }
	inline bool Is32bit() const { return GetBPP() == 32; }
	inline bool Is8bit() const { return GetBPP() == 8; }
	inline int GetWidth() const { return BitmapInfo->bmiHeader.biWidth; }
	inline int GetHeight() const { return BitmapInfo->bmiHeader.biHeight; }
	inline tjs_uint GetImageSize() const { return BitmapInfo->bmiHeader.biSizeImage; }
	inline int GetPitchBytes() const { return GetImageSize()/GetHeight(); }
	BitmapInfomation& operator=(BitmapInfomation& r) {
		memcpy(BitmapInfo, r.BitmapInfo, BitmapInfoSize);
		return *this;
	}

	// 以下、Win32 のみのメソッド
	BITMAPINFO* GetBITMAPINFO() { return BitmapInfo; }
	const BITMAPINFO* GetBITMAPINFO() const { return BitmapInfo; }
};

#endif // __BITMAP_INFOMATION_H__

