//---------------------------------------------------------------------------
/*
	TJS2 Script Engine
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// utility functions
//---------------------------------------------------------------------------
#ifndef tjsUtilsH
#define tjsUtilsH

#include "tjsVariant.h"
#include "tjsString.h"

namespace TJS
{
//---------------------------------------------------------------------------
// tTJSCriticalSection ( implement on each platform for multi-threading support )
//---------------------------------------------------------------------------
#ifdef __WIN32__
#include <Windows.h>
class tTJSCriticalSection
{
	CRITICAL_SECTION CS;
public:
	tTJSCriticalSection() { InitializeCriticalSection(&CS); }
	~tTJSCriticalSection() { DeleteCriticalSection(&CS); }

	void Enter() { EnterCriticalSection(&CS); }
	void Leave() { LeaveCriticalSection(&CS); }
};
#else
class tTJSCriticalSection
{
public:
	tTJSCriticalSection() { ; }
	~tTJSCriticalSection() { ; }

	void Enter() { ; }
	void Leave() { ; }
};
#endif
//---------------------------------------------------------------------------
// interlocked operation ( implement on each platform for multi-threading support )
//---------------------------------------------------------------------------
/*
#ifdef __WIN32__
#include <Windows.h>

// we assume that sizeof(tjs_uint) is 4 on TJS2/win32.

inline tjs_uint TJSInterlockedIncrement(tjs_uint & value)
{
	return InterlockedIncrement((long*)&value);
}

#else

inline

#endif
*/
//---------------------------------------------------------------------------
// tTJSCriticalSectionHolder
//---------------------------------------------------------------------------
class tTJSCriticalSectionHolder
{
	tTJSCriticalSection *Section;
public:
	tTJSCriticalSectionHolder(tTJSCriticalSection &cs)
	{
		Section = &cs;
		Section->Enter();
	}

	~tTJSCriticalSectionHolder()
	{
		Section->Leave();
	}

};
typedef tTJSCriticalSectionHolder tTJSCSH;
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// tTJSAtExit / tTJSAtStart
//---------------------------------------------------------------------------
class tTJSAtExit
{
	void (*Function)();
public:
	tTJSAtExit(void (*func)()) { Function = func; };
	~tTJSAtExit() { Function(); }
};
//---------------------------------------------------------------------------
class tTJSAtStart
{
public:
	tTJSAtStart(void (*func)()) { func(); };
};
//---------------------------------------------------------------------------
class iTJSDispatch2;
extern iTJSDispatch2 * TJSObjectTraceTarget;

#define TJS_DEBUG_REFERENCE_BREAK \
	if(TJSObjectTraceTarget == (iTJSDispatch2*)this) TJSNativeDebuggerBreak()
#define TJS_SET_REFERENCE_BREAK(x) TJSObjectTraceTarget=(x)
//---------------------------------------------------------------------------
TJS_EXP_FUNC_DEF(const tjs_char *, TJSVariantTypeToTypeString, (tTJSVariantType type));
	// convert given variant type to type string ( "void", "int", "object" etc.)

TJS_EXP_FUNC_DEF(tTJSString, TJSVariantToReadableString, (const tTJSVariant &val, tjs_int maxlen = 512));
	// convert given variant to human-readable string
	// ( eg. "(string)\"this is a\\nstring\"" )
TJS_EXP_FUNC_DEF(tTJSString, TJSVariantToExpressionString, (const tTJSVariant &val));
	// convert given variant to string which can be interpret as an expression.
	// this function does not convert objects ( returns empty string )

//---------------------------------------------------------------------------



/*[*/
//---------------------------------------------------------------------------
// tTJSRefHolder : a object holder for classes that has AddRef and Release methods
//---------------------------------------------------------------------------
template <typename T>
class tTJSRefHolder
{
private:
	T *Object;
public:
	tTJSRefHolder(T * ref) { Object = ref; Object->AddRef(); }
	tTJSRefHolder(const tTJSRefHolder<T> &ref)
	{
		Object = ref.Object;
		Object->AddRef();
	}
	~tTJSRefHolder() { Object->Release(); }

	T* GetObject() { Object->AddRef(); return Object; }
	T* GetObjectNoAddRef() { return Object; }

	const tTJSRefHolder & operator = (const tTJSRefHolder & rhs)
	{
		if(rhs.Object != Object)
		{
			Object->Release();
			Object = rhs.Object;
			Object->AddRef();
		}
		return *this;
	}
};



/*]*/

//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// TJSAlignedAlloc : aligned memory allocater
//---------------------------------------------------------------------------
TJS_EXP_FUNC_DEF(void *, TJSAlignedAlloc, (tjs_uint bytes, tjs_uint align_bits));
TJS_EXP_FUNC_DEF(void, TJSAlignedDealloc, (void *ptr));
//---------------------------------------------------------------------------

/*[*/
//---------------------------------------------------------------------------
// floating-point class checker
//---------------------------------------------------------------------------
// constants used in TJSGetFPClass
#define TJS_FC_CLASS_MASK 7
#define TJS_FC_SIGN_MASK 8

#define TJS_FC_CLASS_NORMAL 0
#define TJS_FC_CLASS_NAN 1
#define TJS_FC_CLASS_INF 2

#define TJS_FC_IS_NORMAL(x)  (((x)&TJS_FC_CLASS_MASK) == TJS_FC_CLASS_NORMAL)
#define TJS_FC_IS_NAN(x)  (((x)&TJS_FC_CLASS_MASK) == TJS_FC_CLASS_NAN)
#define TJS_FC_IS_INF(x)  (((x)&TJS_FC_CLASS_MASK) == TJS_FC_CLASS_INF)

#define TJS_FC_IS_NEGATIVE(x) (0!=((x) & TJS_FC_SIGN_MASK))
#define TJS_FC_IS_POSITIVE(x) (!TJS_FC_IS_NEGATIVE(x))


/*]*/
TJS_EXP_FUNC_DEF(tjs_uint32, TJSGetFPClass, (tjs_real r));
//---------------------------------------------------------------------------
}

#endif




