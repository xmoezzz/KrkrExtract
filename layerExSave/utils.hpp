#ifndef _layerexsave_utils_hpp_
#define _layerexsave_utils_hpp_

// バッファ参照用の型
typedef unsigned char const *BufRefT;
typedef unsigned char       *WrtRefT;

bool GetLayerSize(iTJSDispatch2 *lay, long &w, long &h, long &pitch);
bool GetLayerBufferAndSize(iTJSDispatch2 *lay, long &w, long &h, BufRefT &ptr, long &pitch);
bool GetLayerBufferAndSize(iTJSDispatch2 *lay, long &w, long &h, WrtRefT &ptr, long &pitch);

#endif
