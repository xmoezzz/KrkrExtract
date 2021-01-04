//---------------------------------------------------------------------------
/*
	Risa [りさ]      alias 吉里吉里3 [kirikiri-3]
	 stands for "Risa Is a Stagecraft Architecture"
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
//! @file
//! @brief UNICODE->CP932(≒SJIS) 変換
//---------------------------------------------------------------------------
// UNICODEとSJIS(cp932)の変換マップについては unicode.org を参照のこと



#ifndef _UNI_CP932_H_
#define _UNI_CP932_H_

//---------------------------------------------------------------------------
tjs_uint UnicodeToSJIS(tjs_char in);
tjs_size UnicodeToSJISString(const tjs_char *in, tjs_nchar* out );
tjs_size UnicodeToSJISString(const tjs_char *in, tjs_nchar* out, tjs_size limit );
//---------------------------------------------------------------------------


#endif
