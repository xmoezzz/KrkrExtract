#ifndef _TextStream_
#define _TextStream_

#include "tjs.h"
#include <Windows.h>

iTJSTextReadStream * TVPCreateTextStreamForRead(const ttstr & name,
	const ttstr & modestr);
iTJSTextReadStream * TVPCreateTextStreamForReadByEncoding(const ttstr & name,
	const ttstr & modestr, const ttstr & encoding);
void TVPSetDefaultReadEncoding(const ttstr& encoding);
const tjs_char* TVPGetDefaultReadEncoding();

#endif
