#ifndef __TARGET_VER_H__
#define __TARGET_VER_H__

// SDKDDKVer.h をインクルードすると、利用できる最も上位の Windows プラットフォームが定義されます。

// 以前の Windows プラットフォーム用にアプリケーションをビルドする場合は、WinSDKVer.h をインクルードし、
// SDKDDKVer.h をインクルードする前に、サポート対象とするプラットフォームを示すように _WIN32_WINNT マクロを設定します。
//#define WINVER 0x0501 // Windows XP
//#define _WIN32_WINNT 0x0501 // Windows XP
// for touch device
#define WINVER 0x0601 // Window 7
#define _WIN32_WINNT 0x0601 // Windows 7
#include <SDKDDKVer.h>

#endif
