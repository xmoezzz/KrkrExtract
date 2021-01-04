//---------------------------------------------------------------------------
/*
	Risa [りさ]      alias 吉里吉里3 [kirikiri-3]
	 stands for "Risa Is a Stagecraft Architecture"
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
//! @file
//! @brief CP932(≒SJIS)->UNICODE 変換
//---------------------------------------------------------------------------
// UNICODEとSJIS(cp932)の変換マップについては unicode.org を参照のこと



#ifndef _CP932_UNI_
#define _CP932_UNI_

//---------------------------------------------------------------------------

tjs_size SJISToUnicodeString(const char * in, tjs_char *out);
tjs_size SJISToUnicodeString(const char * in, tjs_char *out, tjs_size limit );
tjs_char SJISToUnicode(tjs_uint sjis);
bool IsSJISLeadByte( tjs_nchar b );

//---------------------------------------------------------------------------

#endif

