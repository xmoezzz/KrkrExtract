//---------------------------------------------------------------------------
/*
	TJS2 Script Engine
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// configuration
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

#ifndef tjsConfigH
#define tjsConfigH

namespace TJS
{
//---------------------------------------------------------------------------


/*
	many settings can be changed here.

	tjsCommHead.h includes most common headers that will be needed to
	compile the entire TJS program.

	configuration about Critical Section for multithreading support is there in
	tjsUtils.cpp/h.
*/

// TODO: autoconf integration

#include "tjsTypes.h"
#include "stdarg.h"


// #define TJS_NO_AVOID_ISWDIGIT
// #define TJS_SUPPORT_VCL
// #define TJS_MSG_EMPTY
// #define TJS_DEBUG_TRACE
// #define TJS_JP_LOCALIZED
// #define TJS_TEXT_OUT_CRLF
// #define TJS_WITH_IS_NOT_RESERVED_WORD

TJS_EXP_FUNC_DEF(tjs_int, TJS_atoi, (const tjs_char *s));
TJS_EXP_FUNC_DEF(tjs_char *, TJS_int_to_str, (tjs_int value, tjs_char *string));
TJS_EXP_FUNC_DEF(tjs_char *, TJS_tTVInt_to_str, (tjs_int64 value, tjs_char *string));
TJS_EXP_FUNC_DEF(tjs_int, TJS_strnicmp, (const tjs_char *s1, const tjs_char *s2, size_t maxlen));
TJS_EXP_FUNC_DEF(tjs_int, TJS_stricmp, (const tjs_char *s1, const tjs_char *s2));
TJS_EXP_FUNC_DEF(void, TJS_strcpy_maxlen, (tjs_char *d, const tjs_char *s, size_t len));
TJS_EXP_FUNC_DEF(void, TJS_strcpy, (tjs_char *d, const tjs_char *s));
TJS_EXP_FUNC_DEF(size_t, TJS_strlen, (const tjs_char *d));

#define TJS_strcmp			wcscmp
#define TJS_strncmp			wcsncmp
#define TJS_strncpy			wcsncpy
#define TJS_strcat			wcscat
#define TJS_strstr			wcsstr
#define TJS_strchr			wcschr
#define TJS_malloc			malloc
#define TJS_free			free
#define TJS_realloc			realloc
#define TJS_nsprintf		sprintf
#define TJS_nstrcpy			strcpy
#define TJS_nstrcat			strcat
#define TJS_nstrlen			strlen
#define TJS_nstrstr			strstr
#define TJS_strftime		wcsftime
#define TJS_vfprintf		vfwprintf
#define TJS_octetcpy		memcpy
#define TJS_octetcmp		memcmp
#define TJS_strtod			wcstod


#define TJS_strncpy_s		wcsncpy_s

#if defined(__GNUC__)
	#define TJS_cdecl
	#define TJS_vsnprintf		vswprintf
	extern tjs_int TJS_sprintf(tjs_char *s, const tjs_char *format, ...);
	#define TJS_timezone timezone
	#define TJS_rand rand
	#define TJS_RAND_MAX RAND_MAX
	#define TJS_mbstowcs mbstowcs
	#define TJS_wcstombs wcstombs
	#define TJS_mbtowc   mbtowc
	#define TJS_wctomb   wctomb
	#define TJS_snprintf wsnprintf
#elif WIN32
	#define TJS_cdecl __cdecl
#ifdef _MSC_VER
	#define TJS_vsnprintf		_vsnwprintf
	#define TJS_rand rand
	#define TJS_RAND_MAX RAND_MAX
#else
	#define TJS_vsnprintf		vsnwprintf
	#define TJS_rand _lrand
	#define TJS_RAND_MAX LRAND_MAX
#endif
	#define TJS_snprintf		swprintf_s
	#define TJS_sprintf			swprintf
	#define TJS_timezone _timezone
	extern size_t TJS_mbstowcs(tjs_char *pwcs, const tjs_nchar *s, size_t n);
	extern size_t TJS_wcstombs(tjs_nchar *s, const tjs_char *pwcs, size_t n);
	extern int TJS_mbtowc(tjs_char *pwc, const tjs_nchar *s, size_t n);
	extern int TJS_wctomb(tjs_nchar *s, tjs_char wc);
#endif


#define TJS_narrowtowidelen(X) TJS_mbstowcs(NULL, (X),0) // narrow->wide (if) converted length
#define TJS_narrowtowide TJS_mbstowcs

void TJS_debug_out(const tjs_char *format, ...);

#ifdef TJS_DEBUG_TRACE
#define TJS_D(x)	TJS_debug_out x;
#define TJS_F_TRACE(x) tTJSFuncTrace ___trace(TJS_W(x));
#else
#define TJS_D(x)
#define TJS_F_TRACE(x)
#endif


#ifdef _MSC_VER
#define __STR2__(x) #x
#define __STR1__(x) __STR2__(x)
#define __LOC__ __FILE__ "("__STR1__(__LINE__)") : Warning Msg: "
#endif

extern void TJSNativeDebuggerBreak();


extern void TJSSetFPUE();
extern void TJSRestoreFPUE();


//---------------------------------------------------------------------------
// elapsed time profiler
//---------------------------------------------------------------------------
#ifdef TJS_DEBUG_PROFILE_TIME
extern tjs_uint TJSGetTickCount();
class tTJSTimeProfiler
{
	tjs_uint & timevar;
	tjs_uint start;
public:
	tTJSTimeProfiler(tjs_uint & tv) : timevar(tv)
	{
		start = TJSGetTickCount();
	}

	~tTJSTimeProfiler()
	{
		timevar += TJSGetTickCount() - start;
	}
};
#endif
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// function tracer
//---------------------------------------------------------------------------
class tTJSFuncTrace
{
	tjs_char *funcname;
public:
	tTJSFuncTrace(tjs_char *p)
	{
		funcname = p;
		TJS_debug_out(TJS_W("enter: %ls\n"), funcname);
	}
	~tTJSFuncTrace()
	{
		TJS_debug_out(TJS_W("exit: %ls\n"), funcname);
	}
};
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// tTJSNarrowStringHolder : converts wide -> narrow, and holds it until be destroyed
//---------------------------------------------------------------------------
struct tTJSNarrowStringHolder
{
	bool Allocated;
	tjs_nchar *Buf;
public:
	tTJSNarrowStringHolder(const wchar_t *wide);

	~tTJSNarrowStringHolder(void);

	operator const tjs_nchar *()
	{
		return Buf;
	}
};
//---------------------------------------------------------------------------

}


#endif

