//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Transition handler mamagement & default transition handlers
//---------------------------------------------------------------------------
#ifndef TransIntfH
#define TransIntfH
//---------------------------------------------------------------------------

#include "LayerBitmapIntf.h"
#include "transhandler.h"


//---------------------------------------------------------------------------
// iTVPSimpleOptionProvider implementation
//---------------------------------------------------------------------------
class tTVPSimpleOptionProvider : public iTVPSimpleOptionProvider
{
	tjs_uint RefCount;
	tTJSVariantClosure Object;
	ttstr String;

public:
	tTVPSimpleOptionProvider(tTJSVariantClosure object);
	~tTVPSimpleOptionProvider();

	tjs_error TJS_INTF_METHOD AddRef();
	tjs_error TJS_INTF_METHOD Release();

	tjs_error TJS_INTF_METHOD GetAsNumber(
			/*in*/const tjs_char *name, /*out*/tjs_int64 *value);
	tjs_error TJS_INTF_METHOD GetAsString(
			/*in*/const tjs_char *name, /*out*/const tjs_char **out);

	tjs_error TJS_INTF_METHOD GetValue(
			/*in*/const tjs_char *name, /*out*/tTJSVariant *dest);

	tjs_error TJS_INTF_METHOD Reserved2() { return TJS_E_NOTIMPL; }

	tjs_error TJS_INTF_METHOD GetDispatchObject(iTJSDispatch2 **dsp);
};
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// iTVPSimpleImageProvider implementation
//---------------------------------------------------------------------------
class tTVPSimpleImageProvider : public iTVPSimpleImageProvider
{
public:
	tjs_error TJS_INTF_METHOD LoadImage(
			/*in*/const tjs_char *name, /*in*/tjs_int bpp,
			/*in*/tjs_uint32 key, 
			/*in*/tjs_uint w,
			/*in*/tjs_uint h,
			/*out*/iTVPScanLineProvider ** scpro);
};
//---------------------------------------------------------------------------
extern tTVPSimpleImageProvider TVPSimpleImageProvider;
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
TJS_EXP_FUNC_DEF(iTVPScanLineProvider *, TVPSLPLoadImage, (const ttstr &name, tjs_int bpp,
	tjs_uint32 key, tjs_uint w, tjs_uint h));
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// iTVPScanLineProvider implementation for image provider ( holds tTVPBaseBitmap )
//---------------------------------------------------------------------------
// provides layer scanline
class tTVPScanLineProviderForBaseBitmap : public iTVPScanLineProvider
{
	tjs_uint RefCount;
	bool Own;
	tTVPBaseBitmap *Bitmap;

public:
	tTVPScanLineProviderForBaseBitmap(tTVPBaseBitmap *bmp, bool own = false);
	~tTVPScanLineProviderForBaseBitmap();

	void Attach(tTVPBaseBitmap *bmp); // attach bitmap


	tjs_error TJS_INTF_METHOD AddRef();
	tjs_error TJS_INTF_METHOD Release();

	tjs_error TJS_INTF_METHOD GetWidth(/*in*/tjs_int *width);
	tjs_error TJS_INTF_METHOD GetHeight(/*in*/tjs_int *height);
	tjs_error TJS_INTF_METHOD GetPixelFormat(/*out*/tjs_int *bpp);
	tjs_error TJS_INTF_METHOD GetPitchBytes(/*out*/tjs_int *pitch);
	tjs_error TJS_INTF_METHOD GetScanLine(/*in*/tjs_int line,
			/*out*/const void ** scanline);
	tjs_error TJS_INTF_METHOD GetScanLineForWrite(/*in*/tjs_int line,
			/*out*/void ** scanline);
};
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// handler management functions
//---------------------------------------------------------------------------
TJS_EXP_FUNC_DEF(void, TVPAddTransHandlerProvider,(iTVPTransHandlerProvider *pro));
TJS_EXP_FUNC_DEF(void, TVPRemoveTransHandlerProvider, (iTVPTransHandlerProvider *pro));
iTVPTransHandlerProvider * TVPFindTransHandlerProvider(const ttstr &name);
//---------------------------------------------------------------------------


/*[*/
//---------------------------------------------------------------------------
// scroll transition handler
//---------------------------------------------------------------------------
enum tTVPScrollTransFrom
{
	sttLeft, sttTop, sttRight, sttBottom
};
enum tTVPScrollTransStay
{
	ststNoStay, ststStayDest, ststStaySrc
};
/*]*/
//---------------------------------------------------------------------------


#endif
