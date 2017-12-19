//---------------------------------------------------------------------------
/*
	TJS2 Script Engine
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// configuration
//---------------------------------------------------------------------------


#include "tjsCommHead.h"


#ifdef __WIN32__
#include <float.h>
#endif

namespace TJS
{

//---------------------------------------------------------------------------
// debug support
//---------------------------------------------------------------------------
#ifdef TJS_DEBUG_PROFILE_TIME
tjs_uint TJSGetTickCount()
{
	return GetTickCount();
}
#endif
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// some wchar_t support functions
//---------------------------------------------------------------------------
tjs_int TJS_atoi(const tjs_char *s)
{
	int r = 0;
	bool sign = false;
	while(*s && *s <= 0x20) s++; // skip spaces
	if(!*s) return 0;
	if(*s == TJS_W('-'))
	{
		sign = true;
		s++;
		while(*s && *s <= 0x20) s++; // skip spaces
		if(!*s) return 0;
	}

	while(*s >= TJS_W('0') && *s <= TJS_W('9'))
	{
		r *= 10;
		r += *s - TJS_W('0');
		s++;
	}
	if(sign) r = -r;
	return r;
}
//---------------------------------------------------------------------------
tjs_char * TJS_int_to_str(tjs_int value, tjs_char *string)
{
	tjs_char *ostring = string;

	if(value<0) *(string++) = TJS_W('-'), value = -value;

	tjs_char buf[40];

	tjs_char *p = buf;

	do
	{
		*(p++) = (value % 10) + TJS_W('0');
		value /= 10;
	} while(value);

	p--;
	while(buf <= p) *(string++) = *(p--);
	*string = 0;

	return ostring;
}
//---------------------------------------------------------------------------
tjs_char * TJS_tTVInt_to_str(tjs_int64 value, tjs_char *string)
{
	if(value == TJS_UI64_VAL(0x8000000000000000))
	{
		// this is a special number which we must avoid normal conversion
		TJS_strcpy(string, TJS_W("-9223372036854775808"));
		return string;
	}

	tjs_char *ostring = string;

	if(value<0) *(string++) = TJS_W('-'), value = -value;

	tjs_char buf[40];

	tjs_char *p = buf;

	do
	{
		*(p++) = (value % 10) + TJS_W('0');
		value /= 10;
	} while(value);

	p--;
	while(buf <= p) *(string++) = *(p--);
	*string = 0;

	return ostring;
}
//---------------------------------------------------------------------------
tjs_int TJS_strnicmp(const tjs_char *s1, const tjs_char *s2,
	size_t maxlen)
{
	while(maxlen--)
	{
		if(*s1 == TJS_W('\0')) return (*s2 == TJS_W('\0')) ? 0 : -1;
		if(*s2 == TJS_W('\0')) return (*s1 == TJS_W('\0')) ? 0 : 1;
		if(*s1 < *s2) return -1;
		if(*s1 > *s2) return 1;
		s1++;
		s2++;
	}

	return 0;
}
//---------------------------------------------------------------------------
tjs_int TJS_stricmp(const tjs_char *s1, const tjs_char *s2)
{
	// we only support basic alphabets
	// fixme: complete alphabets support

	for(;;)
	{
		tjs_char c1 = *s1, c2 = *s2;
		if(c1 >= TJS_W('a') && c1 <= TJS_W('z')) c1 += TJS_W('Z')-TJS_W('z');
		if(c2 >= TJS_W('a') && c2 <= TJS_W('z')) c2 += TJS_W('Z')-TJS_W('z');
		if(c1 == TJS_W('\0')) return (c2 == TJS_W('\0')) ? 0 : -1;
		if(c2 == TJS_W('\0')) return (c1 == TJS_W('\0')) ? 0 : 1;
		if(c1 < c2) return -1;
		if(c1 > c2) return 1;
		s1++;
		s2++;
	}
}
//---------------------------------------------------------------------------
void TJS_strcpy_maxlen(tjs_char *d, const tjs_char *s, size_t len)
{
	tjs_char ch;
	len++;
	while((ch=*s)!=0 && --len) *(d++) = ch, s++;
	*d = 0;
}
//---------------------------------------------------------------------------
void TJS_strcpy(tjs_char *d, const tjs_char *s)
{
	tjs_char ch;
	while((ch=*s)!=0) *(d++) = ch, s++;
	*d = 0;
}
//---------------------------------------------------------------------------
size_t TJS_strlen(const tjs_char *d)
{
	const tjs_char *p = d;
	while(*d) d++;
	return d-p;
}
//---------------------------------------------------------------------------
#if  defined(__GNUC__)
tjs_int TJS_sprintf(tjs_char *s, const tjs_char *format, ...)
{
	tjs_int r;
	va_list param;
	va_start(param, format);
	r = TJS_vsnprintf(s, INT_MAX, format, param);
	va_end(param);
	return r;
}
#endif
//---------------------------------------------------------------------------

void TJS_cdecl TJS_debug_out(const tjs_char *format, ...)
{
	va_list param;
	va_start(param, format);
	TJS_vfprintf(stderr, format, param);
	va_end(param);
}
//---------------------------------------------------------------------------


#ifdef WIN32
//---------------------------------------------------------------------------
// Wide<->narrow conversion functions.
// These functions (Win32 only) are as same as the RTL's, but
// alwayes uses Win32 current code page.
// Code algorithms are taken from borland's RTL source, but totally rewritten
// to avoid copyright problems.
//---------------------------------------------------------------------------
#define TJS_MB_MAX_CHARLEN 2
//---------------------------------------------------------------------------
size_t TJS_mbstowcs(tjs_char *pwcs, const tjs_nchar *s, size_t n)
{
	if(pwcs && n == 0) return 0;

	if(pwcs)
	{
		// Try converting to wide characters. Here assumes pwcs is large enough
		// to store the result.
		int count = MultiByteToWideChar(CP_ACP,
			MB_PRECOMPOSED|MB_ERR_INVALID_CHARS, s, -1, pwcs, n);
		if(count != 0) return count - 1;

		if(GetLastError() != ERROR_INSUFFICIENT_BUFFER) return (size_t) -1;

		// pwcs is not enough to store the result ...

		// count number of source characters to fit the destination buffer
		int charcnt = n;
		const unsigned char *p;
		for(p = (const unsigned char*)s; charcnt-- && *p; p++)
		{
			if(IsDBCSLeadByte(*p)) p++;
		}
		int bytecnt = (int)(p - (const unsigned char *)s);

		count = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, s, bytecnt, pwcs, n);
		if(count == 0) return (size_t) -1;

		return count;
	}
	else
	{
		int count = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED|MB_ERR_INVALID_CHARS,
			s, -1, NULL, 0);
		if(count == 0) return (size_t)-1;
		return count - 1;
	}
}
//---------------------------------------------------------------------------
size_t TJS_wcstombs(tjs_nchar *s, const tjs_char *pwcs, size_t n)
{
	if(s && !n) return 0;

	BOOL useddefault = FALSE;
	if(s)
	{
		// Try converting to multibyte. Here assumes s is large enough to
		// store the result.
		size_t pwcs_len = TJS_strlen(pwcs);
		size_t count = WideCharToMultiByte(CP_ACP, 0,
			pwcs, pwcs_len, s, n, NULL, &useddefault);

		if(count != 0/* && !useddefault*/)
			return count;

		if(/*useddefault || */GetLastError () != ERROR_INSUFFICIENT_BUFFER)
			return (size_t) -1; // may a conversion error

		// Buffer is not enough to store the result ...
		while(count < n)
		{
			char buffer[TJS_MB_MAX_CHARLEN + 1];
			int retval = WideCharToMultiByte(CP_ACP, 0, pwcs, 1, buffer,
				TJS_MB_MAX_CHARLEN, NULL, &useddefault);
			if(retval == 0/* || useddefault*/)
				return (size_t) -1;

			if(count + retval > n)
				return count;

			for(int i = 0; i < retval; i++, count++)
			{
				if((s[count] = buffer[i]) == '\0') return count;
			}

			pwcs ++;
		}

		return count;
	}
	else
	{
		// Returns the buffer size to store the result
		int count = WideCharToMultiByte(CP_ACP, 0,
			pwcs, -1, NULL, 0, NULL, &useddefault);
		if(count == 0/* || useddefault*/) return (size_t) -1;
		return count - 1;
	}
}
//---------------------------------------------------------------------------
int TJS_mbtowc(tjs_char *pwc, const tjs_nchar *s, size_t n)
{
	if(!s || !n) return 0;

	if(*s == 0)
	{
		if(pwc) *pwc = 0;
		return 0;
	}

	/* Borland's RTL seems to assume always MB_MAX_CHARLEN = 2. */
	/* This may true while we use Win32 platforms ... */
	if(IsDBCSLeadByte((BYTE)(*s)))
	{
		// multi(double) byte character
		if((int)n < TJS_MB_MAX_CHARLEN) return -1;
		if(MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED|MB_ERR_INVALID_CHARS,
			s, TJS_MB_MAX_CHARLEN, pwc, pwc ? 1:0) == 0)
		{
			if(s[1] == 0) return -1;
		}
		return TJS_MB_MAX_CHARLEN;
	}
	else
	{
		// single byte character
		if(MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED|MB_ERR_INVALID_CHARS, s,
			1, pwc, pwc ? 1:0) == 0)
			return -1;
		return 1;
	}
}
//---------------------------------------------------------------------------
int TJS_wctomb(tjs_nchar *s, tjs_char wc)
{
	if(!s) return 0;
	BOOL useddefault = FALSE;
	int size = WideCharToMultiByte(CP_ACP, 0, &wc, 1, s,
		TJS_MB_MAX_CHARLEN, NULL, &useddefault);
//	if(useddefault) return -1;
	if(size == 0) return -1;
	return size;
}
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// tTJSNarrowStringHolder
//---------------------------------------------------------------------------
tTJSNarrowStringHolder::tTJSNarrowStringHolder(const wchar_t * wide)
{
	int n;
	if(!wide)
		n = -1;
	else
		n = TJS_wcstombs(NULL, wide, 0);

	if( n == -1 )
	{
		Buf = TJS_N("");
		Allocated = false;
		return;
	}
	Buf = new tjs_nchar[n+1];
	Allocated = true;
	Buf[TJS_wcstombs(Buf, wide, n)] = 0;
}
//---------------------------------------------------------------------------
tTJSNarrowStringHolder::~tTJSNarrowStringHolder()
{
	if(Allocated) delete [] Buf;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// native debugger break point
//---------------------------------------------------------------------------
void TJSNativeDebuggerBreak()
{
	// This function is to be called mostly when the "debugger" TJS statement is
	// executed.
	// Step you debbuger back to the the caller, and continue debugging.
	// Do not use "debugger" statement unless you run the program under the native
	// debugger, or the program may cause an unhandled debugger breakpoint
	// exception.

#if defined(__WIN32__) && defined(_M_IX86)
	#ifdef __BORLANDC__
			__emit__ (0xcc); // int 3 (Raise debugger breakpoint exception)
	#else
			_asm _emit 0xcc; // int 3 (Raise debugger breakpoint exception)
	#endif
#endif
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// FPU control
//---------------------------------------------------------------------------
#if defined(WIN32) && !defined(__GNUC__)
static unsigned int TJSDefaultFPUCW = 0;
static unsigned int TJSNewFPUCW = 0;
static bool TJSFPUInit = false;
#endif
void TJSSetFPUE()
{
#if defined(WIN32) && !defined(__GNUC__)
	if(!TJSFPUInit)
	{
		TJSFPUInit = true;
		TJSDefaultFPUCW = _control87(0, 0);
		
#ifdef _MSC_VER
		TJSNewFPUCW = _control87(MCW_EM, MCW_EM);
#else
		_default87 = TJSNewFPUCW = _control87(MCW_EM, MCW_EM);
#endif
#ifdef TJS_SUPPORT_VCL
		Default8087CW = TJSNewFPUCW;
#endif
	}

//	_fpreset();
	_control87(TJSNewFPUCW, 0xffff);
#endif

}

void TJSRestoreFPUE()
{

#if defined(WIN32) && !defined(__GNUC__)
	if(!TJSFPUInit) return;
	_control87(TJSDefaultFPUCW, 0xffff);
#endif
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
} // namespace TJS



