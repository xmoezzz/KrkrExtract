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
#include <intrin.h>
#endif

/*
 * core/utils/cp932_uni.cpp
 * core/utils/uni_cp932.cpp
 * を一緒にリンクしてください。
 * CP932(ShiftJIS) と Unicode 変換に使用しています。
 * Win32 APIの同等の関数は互換性等の問題があることやマルチプラットフォームの足かせとなる
 * ため使用が中止されました。
 */
extern tjs_size SJISToUnicodeString(const char * in, tjs_char *out);
extern tjs_size SJISToUnicodeString(const char * in, tjs_char *out, tjs_size limit );
extern bool IsSJISLeadByte( tjs_nchar b );
extern tjs_uint UnicodeToSJIS(tjs_char in);
extern tjs_size UnicodeToSJISString(const tjs_char *in, tjs_nchar* out );
extern tjs_size UnicodeToSJISString(const tjs_char *in, tjs_nchar* out, tjs_size limit );

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


//---------------------------------------------------------------------------
#define TJS_MB_MAX_CHARLEN 2
//---------------------------------------------------------------------------
size_t TJS_mbstowcs(tjs_char *pwcs, const tjs_nchar *s, size_t n)
{
	if(pwcs && n == 0) return 0;

	if(pwcs)
	{
		size_t count = SJISToUnicodeString( s, pwcs, n );
		if( n > count )
		{
			pwcs[count] = TJS_W('\0');
		}
		return count;
	}
	else
	{	// count length
		return SJISToUnicodeString( s, NULL );
	}
}
//---------------------------------------------------------------------------
size_t TJS_wcstombs(tjs_nchar *s, const tjs_char *pwcs, size_t n)
{
	if(s && !n) return 0;

	if(s)
	{
		tjs_size count = UnicodeToSJISString( pwcs, s, n );
		if( n > count )
		{
			s[count] = '\0';
		}
		return count;
	}
	else
	{
		// Returns the buffer size to store the result
		return UnicodeToSJISString(pwcs,NULL);
	}
}
//---------------------------------------------------------------------------
// 使われていないようなので未確認注意
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
	if( IsSJISLeadByte( *s ) )
	{
		// multi(double) byte character
		if((int)n < TJS_MB_MAX_CHARLEN) return -1;
		tjs_size count = SJISToUnicodeString( s, pwc, 1 );
		if( count <= 0 )
		{
			return -1;
		}
		return TJS_MB_MAX_CHARLEN;
	}
	else
	{
		// single byte character
		return (int)SJISToUnicodeString( s, pwc, 1 );
	}
}
//---------------------------------------------------------------------------
// 使われていないようなので未確認注意
int TJS_wctomb(tjs_nchar *s, tjs_char wc)
{
	if(!s) return 0;
	tjs_uint c = UnicodeToSJIS(wc);
	if( c == 0 ) return -1;
	int size = 0;
	if( c & 0xff00 )
	{
		*s = static_cast<tjs_nchar>((c>>8)&0xff);
		s++;
		size++;
	}
	*s = static_cast<tjs_nchar>( c&0xff );
	size++;
	return size;
}
//---------------------------------------------------------------------------

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
		n = (int)TJS_wcstombs(NULL, wide, 0);

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

#if defined(__WIN32__)
	#if defined(_M_IX86)
		#ifdef __BORLANDC__
				__emit__ (0xcc); // int 3 (Raise debugger breakpoint exception)
		#else
				_asm _emit 0xcc; // int 3 (Raise debugger breakpoint exception)
		#endif
	#else
		__debugbreak();
	#endif
#endif
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// FPU control
//---------------------------------------------------------------------------
#if defined(__WIN32__) && !defined(__GNUC__)
static unsigned int TJSDefaultFPUCW = 0;
static unsigned int TJSNewFPUCW = 0;
static unsigned int TJSDefaultMMCW = 0;
static bool TJSFPUInit = false;
#endif
// FPU例外をマスク
void TJSSetFPUE()
{
#if defined(__WIN32__) && !defined(__GNUC__)
	if(!TJSFPUInit)
	{
		TJSFPUInit = true;
#if defined(_M_X64)
		TJSDefaultMMCW = _MM_GET_EXCEPTION_MASK();
#else
		TJSDefaultFPUCW = _control87(0, 0);

#ifdef _MSC_VER
		TJSNewFPUCW = _control87(MCW_EM, MCW_EM);
#else
		_default87 = TJSNewFPUCW = _control87(MCW_EM, MCW_EM);
#endif	// _MSC_VER
#ifdef TJS_SUPPORT_VCL
		Default8087CW = TJSNewFPUCW;
#endif	// TJS_SUPPORT_VCL
#endif	// _M_X64
	}

#if defined(_M_X64)
	_MM_SET_EXCEPTION_MASK(_MM_MASK_INVALID|_MM_MASK_DIV_ZERO|_MM_MASK_DENORM|_MM_MASK_OVERFLOW|_MM_MASK_UNDERFLOW|_MM_MASK_INEXACT);
#else
//	_fpreset();
	_control87(TJSNewFPUCW, 0xffff);
#endif
#endif	// defined(__WIN32__) && !defined(__GNUC__)

}
// 例外マスクを解除し元に戻す
void TJSRestoreFPUE()
{
	if(!TJSFPUInit) return;
#if defined(__WIN32__) && !defined(__GNUC__)
#if defined(_M_X64)
	_MM_SET_EXCEPTION_MASK(TJSDefaultMMCW);
#else
	_control87(TJSDefaultFPUCW, 0xffff);
#endif
#endif
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
} // namespace TJS



