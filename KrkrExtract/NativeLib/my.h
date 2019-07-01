#pragma once

#include "phnt_windows.h"
#include "phnt.h"
#include "Hash.h"
#include <malloc.h>
#include <stdio.h>
#include <wchar.h>
#include <intrin.h>
#include <shlwapi.h>
#include <limits>
#include <intsafe.h>
#include <new>

#define INLINE_ASM __asm

#define MEMORY_PAGE_SIZE (4 * 1024)

#define DEF_VAL(var, value)  var = value

#define TYPE_OF decltype
#define FUNC_POINTER(__func) TYPE_OF(__func)*
#define API_POINTER(__func) TYPE_OF(&__func)
#define PTR_OF(_var) TYPE_OF(_var)*

#define FOR_EACH(_it, _base, _n) for (auto _Count = ( ((_it) = (_base)), (_n)); _Count != 0; ++(_it), --_Count)
#define FOR_EACH_REVERSE(_it, _base, _n) for (auto _Count = ( ((_it) = (_base) + (_n) - 1), (_n)); _Count != 0; --(_it), --_Count)
#define FOR_EACH_ARRAY(_it, _arr) FOR_EACH(_it, _arr, countof(_arr))
#define FOR_EACH_S(_it, _base, _n, _size) for (auto _Count = ( ((_it) = (_base)), (_n)); _Count != 0; ((_it) = PtrAdd(_it, _size)), --_Count)
#define FOR_EACH_X(_it, _base, _n) for (auto _Count = ( ((_it) = (_base)), (_n); _Count != 0; ++(_it), --(_n), --_Count)

#define FOR_EACH_FORWARD(_it, _n) { (_it) += (_n); (_Count) += (_n); }
#define FOR_EACH_BACKWARD(_it, _n) { (_it) -= (_n); (_Count) -= (_n); }



#if !defined(_68K_) && !defined(_MPPC_) && !defined(_X86_) && !defined(_IA64_) && !defined(_AMD64_) && defined(_M_IX86)
#define _X86_
#endif

#if !defined(_68K_) && !defined(_MPPC_) && !defined(_X86_) && !defined(_IA64_) && !defined(_AMD64_) && defined(_M_AMD64)
#define _AMD64_
#endif

#if !defined(_68K_) && !defined(_MPPC_) && !defined(_X86_) && !defined(_IA64_) && !defined(_AMD64_) && defined(_M_M68K)
#define _68K_
#endif

#if !defined(_68K_) && !defined(_MPPC_) && !defined(_X86_) && !defined(_IA64_) && !defined(_AMD64_) && defined(_M_MPPC)
#define _MPPC_
#endif

#if !defined(_68K_) && !defined(_MPPC_) && !defined(_X86_) && !defined(_M_IX86) && !defined(_AMD64_) && defined(_M_IA64)
#if !defined(_IA64_)
#define _IA64_
#endif /* !_IA64_ */
#endif

#ifndef _MAC
#if defined(_68K_) || defined(_MPPC_)
#define _MAC
#endif
#endif /* _MAC */

#if defined(_AMD64_)

#define MY_X64 1
#define ML_AMD64    1

#elif defined(_X86_)

#define ML_X86 1
#define ML_I386 1

#endif



#ifndef ASM
#define ASM __declspec(naked)
#endif /* ASM */

#ifndef NAKED
#define NAKED __declspec(naked)
#endif /* ASM */

#if !defined(INLINE_ASM)
#define INLINE_ASM __asm
#endif /* INLINE_ASM */

#ifndef MY_DLL_EXPORT
#define MY_DLL_EXPORT __declspec(dllexport)
#endif  /* MY_DLL_EXPORT */

#ifdef _MY_NTLIB_DLL_
#define MY_NTLIB_DLL_EXPORT __declspec(dllexport)
#else
#define MY_NTLIB_DLL_EXPORT
#endif  /* _MY_NTLIB_DLL_ */

#ifndef MY_DLL_IMPORT
#define MY_DLL_IMPORT __declspec(dllimport)
#endif /* MY_DLL_IMPORT */

#define SEH_TRY     __try
#define SEH_FINALLY __finally
#define SEH_LEAVE   __leave
#define SEH_EXCEPT  __except

#define DECL_NOVTABLE   __declspec(novtable)


#if !defined(MIN)
#define MIN(a, b) a < b ? a : b
#endif

#if !defined(MAX)
#define MAX(a, b) a > b ? a : b
#endif



template<class T>
inline void SafeReleaseT(T*& Object)
{
	if (Object != NULL)
	{
		Object->Release();
		Object = NULL;
	}
}

template<class T>
inline void SafeDeleteT(T*& Object)
{
	if (Object != NULL)
	{
		delete Object;
		Object = NULL;
	}
}

template<class T>
inline void SafeDeleteArrayT(T*& Object)
{
	if (Object != NULL)
	{
		delete[] Object;
		Object = NULL;
	}
}


namespace PointerOperationType
{
	enum PointerOperationClass
	{
		POINTER_OP_ADD,
		POINTER_OP_SUB,
		POINTER_OP_AND,
		POINTER_OP_OR,
		POINTER_OP_XOR,
		POINTER_OP_MOD,
	};
}


template<PointerOperationType::PointerOperationClass OP, class PtrType1, class PtrType2>
FORCEINLINE
PtrType1 PtrOperator(PtrType1 Ptr1, PtrType2 Ptr2)
{
#pragma warning(push, 0)
	struct
	{
		union
		{
			PtrType1        _Ptr1;
			LARGE_INTEGER   Value1;
		};

		union
		{
			PtrType2        _Ptr2;
			LARGE_INTEGER   Value2;
		};
	} u;

#pragma warning(push)
#pragma warning(disable:4702)

	if (MAX(sizeof(Ptr1), sizeof(Ptr2)) == sizeof(u.Value1.QuadPart))
	{
		u.Value1.QuadPart = 0;
		u.Value2.QuadPart = 0;
	}
	else
	{
		u.Value1.LowPart = NULL;
		u.Value2.LowPart = NULL;
	}

#pragma warning(pop)

	u._Ptr1 = Ptr1;
	u._Ptr2 = Ptr2;

	if (MAX(sizeof(Ptr1), sizeof(Ptr2)) == sizeof(u.Value1.QuadPart))
	{
		switch (OP)
		{
		case PointerOperationType::POINTER_OP_ADD:
			u.Value1.QuadPart += u.Value2.QuadPart;
			break;

		case PointerOperationType::POINTER_OP_SUB:
			u.Value1.QuadPart -= u.Value2.QuadPart;
			break;

		case PointerOperationType::POINTER_OP_AND:
			u.Value1.QuadPart &= u.Value2.QuadPart;
			break;

		case PointerOperationType::POINTER_OP_OR:
			u.Value1.QuadPart |= u.Value2.QuadPart;
			break;

		case PointerOperationType::POINTER_OP_XOR:
			u.Value1.QuadPart ^= u.Value2.QuadPart;
			break;

		case PointerOperationType::POINTER_OP_MOD:
			u.Value1.QuadPart %= u.Value2.QuadPart;
			break;
		}
	}
	else
	{
		switch (OP)
		{
		case PointerOperationType::POINTER_OP_ADD:
			u.Value1.LowPart += u.Value2.LowPart;
			break;

		case PointerOperationType::POINTER_OP_SUB:
			u.Value1.LowPart -= u.Value2.LowPart;
			break;

		case PointerOperationType::POINTER_OP_AND:
			u.Value1.LowPart &= u.Value2.LowPart;
			break;

		case PointerOperationType::POINTER_OP_OR:
			u.Value1.LowPart |= u.Value2.LowPart;
			break;

		case PointerOperationType::POINTER_OP_XOR:
			u.Value1.LowPart ^= u.Value2.LowPart;
			break;

		case PointerOperationType::POINTER_OP_MOD:
			u.Value1.LowPart %= u.Value2.LowPart;
			break;
		}
	}

#pragma warning(pop)

	return u._Ptr1;
}

template<PointerOperationType::PointerOperationClass OP, class PtrType1>
FORCEINLINE
PtrType1 PtrOperator(PtrType1 Ptr1, unsigned short Ptr2)
{
	return PtrOperator<OP>(Ptr1, (ULONG_PTR)Ptr2);
}

template<PointerOperationType::PointerOperationClass OP, class PtrType1>
FORCEINLINE
PtrType1 PtrOperator(PtrType1 Ptr1, short Ptr2)
{
	return PtrOperator<OP>(Ptr1, (ULONG_PTR)Ptr2);
}

template<PointerOperationType::PointerOperationClass OP, class PtrType1>
FORCEINLINE
PtrType1 PtrOperator(PtrType1 Ptr1, unsigned char Ptr2)
{
	return PtrOperator<OP>(Ptr1, (ULONG_PTR)Ptr2);
}

template<PointerOperationType::PointerOperationClass OP, class PtrType1>
FORCEINLINE
PtrType1 PtrOperator(PtrType1 Ptr1, char Ptr2)
{
	return PtrOperator<OP>(Ptr1, (ULONG_PTR)Ptr2);
}

template<class PtrType1, class PtrType2>
FORCEINLINE
ULONG_PTR PtrOffset(PtrType1 Ptr, PtrType2 Offset)
{
#pragma warning(push, 0)
	union
	{
		PtrType1    _Ptr1;
		ULONG_PTR   Value;
	};
#pragma warning(pop)

	_Ptr1 = Ptr;

	return PtrOperator<PointerOperationType::POINTER_OP_SUB>(Value, Offset);
}

template<class PtrType1, class PtrType2>
FORCEINLINE
PtrType1 PtrAdd(PtrType1 Ptr, PtrType2 Offset)
{
	return PtrOperator<PointerOperationType::POINTER_OP_ADD>(Ptr, Offset);
}

template<class PtrType1, class PtrType2>
FORCEINLINE
PtrType1 PtrSub(PtrType1 Ptr, PtrType2 Offset)
{
	return PtrOperator<PointerOperationType::POINTER_OP_SUB>(Ptr, Offset);
}

template<class PtrType1, class PtrType2>
FORCEINLINE
PtrType1 PtrAnd(PtrType1 Ptr, PtrType2 Offset)
{
	return PtrOperator<PointerOperationType::POINTER_OP_AND>(Ptr, Offset);
}

template<class PtrType1, class PtrType2>
FORCEINLINE
PtrType1 PtrOr(PtrType1 Ptr, PtrType2 Offset)
{
	return PtrOperator<PointerOperationType::POINTER_OP_OR>(Ptr, Offset);
}

template<class PtrType1, class PtrType2>
FORCEINLINE
PtrType1 PtrXor(PtrType1 Ptr, PtrType2 Offset)
{
	return PtrOperator<PointerOperationType::POINTER_OP_XOR>(Ptr, Offset);
}

template<class PtrType1, class PtrType2>
FORCEINLINE
PtrType1 PtrMod(PtrType1 Ptr, PtrType2 Offset)
{
	return PtrOperator<PointerOperationType::POINTER_OP_MOD>(Ptr, Offset);
}


#define DR7_LEN_1_BYTE  1
#define DR7_LEN_2_BYTE  2
#define DR7_LEN_4_BYTE  3
#define DR7_RW_EXECUTE  0
#define DR7_RW_WRITE    1
#define DR7_RW_ACCESS   3


#define RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE             (0x00000001)
#define RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING       (0x00000002)
#define RTL_DUPSTR_ADD_NULL                                     RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE
#define RTL_DUPSTR_ALLOC_NULL                                   RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING

#pragma pack(1)

typedef union
{
	ULONG Dr6;
	struct
	{
		BYTE B0 : 1;
		BYTE B1 : 1;
		BYTE B2 : 1;
		BYTE B3 : 1;
		BYTE Reserve1 : 8;
		BYTE Reserve2 : 2;
		BYTE BD : 1;
		BYTE BS : 1;
		BYTE BT : 1;
	};

} DR6_INFO, *PDR6_INFO;

typedef union
{
	ULONG Dr7;
	struct
	{
		BYTE L0 : 1;    // 0x01
		BYTE G0 : 1;    // 0x02
		BYTE L1 : 1;    // 0x03
		BYTE G1 : 1;    // 0x04
		BYTE L2 : 1;    // 0x05
		BYTE G2 : 1;    // 0x06
		BYTE L3 : 1;    // 0x07
		BYTE G3 : 1;    // 0x08
		BYTE LE : 1;    // 0x09
		BYTE GE : 1;    // 0x0A
		BYTE Reserve : 3;    // 0x0B
		BYTE GD : 1;    // 0x0E
		BYTE Reserve2 : 2;    // 0x0F
		BYTE RW0 : 2;    // 0x11
		BYTE LEN0 : 2;    // 0x13
		BYTE RW1 : 2;    // 0x15
		BYTE LEN1 : 2;    // 0x17
		BYTE RW2 : 2;    // 0x19
		BYTE LEN2 : 2;    // 0x1B
		BYTE RW3 : 2;    // 0x1D
		BYTE LEN3 : 2;    // 0x1F
	};
} DR7_INFO, *PDR7_INFO;

typedef struct
{
	ULONG_PTR Rdi;
	ULONG_PTR Rsi;
	ULONG_PTR Rbp;
	ULONG_PTR Rsp;
	ULONG_PTR Rbx;
	ULONG_PTR Rdx;
	ULONG_PTR Rcx;
	ULONG_PTR Rax;

} PUSHAD_REGISTER, *PPUSHAD_REGISTER;


typedef union
{
	struct
	{
		UINT16 w1;
		UINT16 w2;
		UINT16 w3;
		UINT16 w4;
		UINT16 w5;
		UINT16 w6;
		UINT16 w7;
		INT16  w8;
	};

	struct
	{
		UINT32 d1;
		UINT32 d2;
		UINT32 d3;
		INT32  d4;
	};

	struct
	{
		ULARGE_INTEGER q1;
		LARGE_INTEGER  q2;
	};
} MMXRegister;

#pragma pack()

#define RTL_CONST_STRING(_str, _string) \
            (_str).Length = sizeof(_string) - sizeof((_str).Buffer[0]); \
            (_str).MaximumLength = sizeof(_string); \
            (_str).Buffer = (_string);

template<typename StringType>
inline
VOID
RtlInitEmptyString(
	OUT StringType*     String,
	IN  PCWSTR          Buffer = nullptr,
	IN  ULONG_PTR       BufferSize = 0
)
{
	String->Length = 0;
	String->MaximumLength = (USHORT)BufferSize;
	String->Buffer = (PWSTR)Buffer;
}

enum // ECodePage
{
	CP_SHIFTJIS = 932,
	CP_GBK = 936,
	CP_GB2312 = CP_GBK,
	CP_BIG5 = 950,
	CP_UTF16_LE = 1200,
	CP_UTF16_BE = 1201,
};

#define BOM_UTF8        (UInt32)(0xBFBBEF)
#define BOM_UTF16_LE    (UInt16)(0xFEFF)
#define BOM_UTF16_BE    (UInt16)(0xFFFE)

INT32 StringToInt32A(PCCHAR pszString);
INT32 StringToInt32W(PCWCHAR pszString);

inline INT32 StringToInt32(TCHAR* pszString)
{
	if (sizeof(*pszString) == sizeof(CHAR))
		return StringToInt32A((PCHAR)pszString);
	else
		return StringToInt32W((PWCHAR)pszString);
}

INT32 StringToInt32HexW(PCWCHAR pszString);
INT64 StringToInt64HexW(PCWCHAR pszString);
INT64 StringToInt64A(PCHAR pszString);
INT64 StringToInt64W(PCWCHAR pszString);

inline INT64 StringToInt64(TCHAR* pszString)
{
	if (sizeof(TCHAR) == sizeof(CHAR))
		return StringToInt64A((PCHAR)pszString);
	else if (sizeof(TCHAR) == sizeof(WCHAR))
		return StringToInt64W((PWCHAR)pszString);

	return 0;
}

#define __MAKE_WSTRING(str) L##str
#define MAKE_WSTRING(str) __MAKE_WSTRING(str)



#define ASM_UNIQUE() INLINE_ASM mov eax, __LINE__

#define ASM_DUMMY(Bytes) ASM_DUMMY_##Bytes

#define ASM_DUMMY_1 INLINE_ASM nop
#define ASM_DUMMY_2 INLINE_ASM mov eax, eax

// lea eax, [eax+0];
#define ASM_DUMMY_3 INLINE_ASM __emit 0x8D INLINE_ASM __emit 0x40 INLINE_ASM __emit 0x00

// // lea esi, [esi]
#define ASM_DUMMY_4 INLINE_ASM __emit 0x8D \
                    INLINE_ASM __emit 0x74 \
                    INLINE_ASM __emit 0x26 \
                    INLINE_ASM __emit 0x00

#define ASM_DUMMY_5 INLINE_ASM mov eax, 1
#define ASM_DUMMY_6 INLINE_ASM __emit 0x8D INLINE_ASM __emit 0x80 INLINE_ASM __emit 0x00 INLINE_ASM __emit 0x00 INLINE_ASM __emit 0x00 INLINE_ASM __emit 0x00

#define ASM_DUMMY_7 INLINE_ASM __emit 0x8D \
                    INLINE_ASM __emit 0xB4 \
                    INLINE_ASM __emit 0x26 \
                    INLINE_ASM __emit 0x00 \
                    INLINE_ASM __emit 0x00 \
                    INLINE_ASM __emit 0x00 \
                    INLINE_ASM __emit 0x00

#define ASM_DUMMY_AUTO() INLINE_ASM mov eax, 1 INLINE_ASM mov ecx, 1 INLINE_ASM mov edx, 1 ASM_UNIQUE() INLINE_ASM ret

#define LOOP_ALWAYS for (;;)
#define LOOP_FOREVER LOOP_ALWAYS
#define LOOP_ONCE   for (BOOL __condition_ = TRUE; __condition_; __condition_ = FALSE)


#if !defined(BREAK_IF)
#define BREAK_IF(c) if (c) break;
#endif /* BREAK_IF */


#if !defined(CONTINUE_IF)
#define CONTINUE_IF(c) if (c) continue;
#endif /* CONTINUE_IF */

#if !defined(RETURN_IF)
#define RETURN_IF(c, r) if (c) return r
#endif /* RETURN_IF */

#if !defined(countof)
#define countof(x) (sizeof((x)) / sizeof(*(x)))
#endif /* countof */

#if !defined(CONST_STRLEN)
#define CONST_STRLEN(str) (countof(str) - 1)
#define CONST_STRSIZE(str) (CONST_STRLEN(str) * sizeof(str[0]))
#endif // CONST_STRLEN

#if !defined(bitsof)
#define bitsof(x) (sizeof(x) * 8)
#endif /* bitsof */

#define FIELD_BASE(address, type, field) (type *)((ULONG_PTR)address - (ULONG_PTR)&((type *)0)->field)
#define FIELD_TYPE(_Type, _Field)  TYPE_OF(((_Type*)0)->_Field)

#ifndef FIELD_SIZE
#define FIELD_SIZE(type, field) (sizeof(((type *)0)->field))
#endif // FIELD_SIZE

#define SET_FLAG(_V, _F)    ((_V) |= (_F))
#define CLEAR_FLAG(_V, _F)  ((_V) &= ~(_F))
#define FLAG_ON(_V, _F)     (!!((_V) & (_F)))
#define FLAG_OFF(_V, _F)     (!FLAG_ON(_V, _F))

#if !defined(TEST_BIT)
#define TEST_BIT(value, bit) ((value) & (1 << bit))
#endif /* TEST_BIT */

#if !defined(ROUND_DOWN)
#define ROUND_DOWN(Value, Multiple) ((Value) / (Multiple) * (Multiple))
#endif /* ROUND_DOWN */

#if !defined(ROUND_UP)
#define ROUND_UP(Value, Multiple) (ROUND_DOWN((Value) + (Multiple) - 1, (Multiple)))
#endif /* ROUND_UP */

#if !defined(IN_RANGE)
#define IN_RANGE(low, value, high) (((low) <= (value)) && (value) <= (high))
#define IN_RANGEEX(low, value, high) (((low) <= (value)) && (value) < (high))
#endif

#define NOP  0x90
#define CALL 0xE8
#define JUMP 0xE9
#define PUSH 0x68
#define REXW 0x49

template<class FunctionType, class FunctionAddress>
__forceinline
FunctionType GetRoutinePtr(FunctionType, FunctionAddress Address)
{
	union
	{
		FunctionType    _Addr1;
		FunctionAddress _Addr2;
	};

	_Addr2 = Address;

	return _Addr1;
}


#define CHAR_UPPER(ch) (IN_RANGE('a', (ch), 'z') ? ((ch) & (BYTE)0xDF) : ch)
#define CHAR_LOWER(ch) (IN_RANGE('A', (ch), 'Z') ? ((ch) | (BYTE)~0xDF) : ch)

#define _CHAR_UPPER4W(ch) (UINT64)((ch) & 0xFFDFFFDFFFDFFFDF)
#define CHAR_UPPER4W(ch) _CHAR_UPPER4W((UINT64)(ch))
#define CHAR_UPPER3W(ch) (UINT64)(CHAR_UPPER4W(ch) & 0x0000FFFFFFFFFFFF)
#define CHAR_UPPER2W(ch) (UINT64)(CHAR_UPPER4W(ch) & 0x00000000FFFFFFFF)
#define CHAR_UPPER1W(ch) (UINT64)(CHAR_UPPER4W(ch) & 0x000000000000FFFF)

#define _CHAR_UPPER4(ch) (UINT32)((ch) & 0xDFDFDFDF)
#define CHAR_UPPER4(ch) (UINT32)_CHAR_UPPER4((UInt32)(ch))
#define CHAR_UPPER3(ch) (UINT32)(CHAR_UPPER4(ch) & 0x00FFFFFF)
#define CHAR_UPPER2(ch) (UINT32)(CHAR_UPPER4(ch) & 0x0000FFFF)
#define CHAR_UPPER1(ch) (UINT32)(CHAR_UPPER4(ch) & 0x000000FF)
#define CHAR_UPPER8(ch) ((UINT64)(ch) & 0xDFDFDFDFDFDFDFDF)

#define _TAG2(s) ((((s) << 8) | ((s) >> 8)) & 0xFFFF)
#define TAG2(s) _TAG2((UINT16)(s))

#define _TAG3(s) ( \
                (((s) >> 16) & 0xFF)       | \
                (((s)        & 0xFF00))    | \
                (((s) << 16) & 0x00FF0000) \
                )

#define TAG3(s) _TAG3((UINT32)(s))

#define _TAG4(s) ( \
                (((s) >> 24) & 0xFF)       | \
                (((s) >> 8 ) & 0xFF00)     | \
                (((s) << 24) & 0xFF000000) | \
                (((s) << 8 ) & 0x00FF0000) \
                )

#define TAG4(s) _TAG4((UINT32)(s))
#define TAG8(left, right) (((UINT64)TAG4(right) << 32) | TAG4(left))

#define _TAG2W(x) (((x) & 0xFF) << 16 | ((x) & 0xFF00) >> 8)
#define TAG2W(x) (UINT32)_TAG2W((UINT32)(x))

#define _TAG3W(x) (TAG4W(x) >> 16)
#define TAG3W(x) (UINT64)_TAG3W((UINT64)(x))

#define _TAG4W(x) (((UINT64)TAG2W((x) & 0xFFFF) << 32) | ((UINT64)TAG2W((x) >> 16)))
#define TAG4W(x) (UINT64)_TAG4W((UINT64)(x))

#define SWAP2(v) (UINT16)(((UINT32)(v) << 8) | ((UINT16)(v) >> 8))
#define SWAPCHAR(v) ((UINT32)SWAP2(v))

#define LoByte(v)  (UINT8) ((v & 0xFF))
#define HiByte(v)  (UINT8) (((v) >> 8) & 0xFF)
#define LoWord(v)  (UINT16)((v) & 0xFFFF)
#define HiWord(v)  (UINT16)(((v) >> 16) & 0xFFFF)
#define LoDword(v) (UINT32)((v))
#define HiDword(v) (UINT32)(((v) >> 32))

#define MakeLong(l, h)   (LONG)((LONG)(l) | ((LONG)(h) << 16))
#define MakeLong64(l, h) (LONGLONG)((LONGLONG)(l) | (LONGLONG)(h) << 32)

#define MakeDword(l, h) (ULONG)((ULONG)(l) | ((ULONG)(h) << 16))
#define MakeQword(l, h) (ULONGLONG)((ULONGLONG)(l) | (ULONGLONG)(h) << 32)

#if !defined(NtCurrentProcess)
#define NtCurrentProcess() (HANDLE)-1
#define NtCurrentProcess64() (HANDLE64)-1
#endif /* NtCurrentProcess */

#if !defined(NtCurrentThread)
#define NtCurrentThread() (HANDLE)-2
#define NtCurrentThread64() (HANDLE64)-2
#endif /* NtCurrentThread */

#ifndef NT_SUCCESS

__forceinline BOOL IsStatusSuccess(NTSTATUS Status) { return (Status & Status) >= 0; }
//    #define NT_SUCCESS(Status)  (((NTSTATUS)(Status)) >= 0)
#define NT_SUCCESS(Status)  IsStatusSuccess(Status)
#endif /* NT_SUCCESS */

#ifndef NT_FAILED
#define NT_FAILED !NT_SUCCESS
#endif // NT_FAILED

#if !defined(FAIL_RETURN)
#define FAIL_RETURN(Status) { NTSTATUS __Status__;  __Status__ = (Status); if (!NT_SUCCESS(__Status__)) return __Status__; }
#define FAIL_CONTINUE(Status) if (!NT_SUCCESS(Status)) continue
#define FAIL_BREAK(Status) if (!NT_SUCCESS(Status)) break
#define SUCCESS_RETURN(Status) { NTSTATUS __Status__;  __Status__ = (Status); if (NT_SUCCESS(__Status__)) return __Status__; }
#endif  // FAIL_RETURN

#if defined(ForceInline)
#undef ForceInline
#endif

#define ForceInline __forceinline

#if defined(FASTCALL)
#undef FASTCALL
#endif

#define FASTCALL __fastcall



#define IMAGE_INVALID_ORDINAL   ((ULONG_PTR)(~0ull))

#define IMAGE_INVALID_RVA       ((ULONG_PTR)(~0ui64))
#define IMAGE_INVALID_VA        ((PVOID)(~0ui64))
#define IMAGE_INVALID_OFFSET    IMAGE_INVALID_RVA
#define IMAGE_MINIMUM_ADDRESS   (ULONG_PTR)0x10000

#define IMAGE_NTHEADERS(_ImageBase) ((PIMAGE_NT_HEADERS)PtrAdd((_ImageBase), (_ImageBase)->e_lfanew))

#define IMAGE_SECTION_BASE(_Section)    ((_Section)->VirtualAddress)
#define IMAGE_SECTION_SIZE(_Section)    ((_Section)->Misc.VirtualSize)
#define IMAGE_SECTION_TAIL(_Section)    (IMAGE_SECTION_BASE(_Section) + IMAGE_SECTION_SIZE(_Section))
#define IMAGE_SECTION_TAIL_ALIGN(_Section, _SectionAlignment) ROUND_UP(IMAGE_SECTION_TAIL(_Section), _SectionAlignment)

enum
{
	IMAGE_VALID_EXPORT_ADDRESS_TABLE = 0x00000001,
	IMAGE_VALID_IMPORT_ADDRESS_TABLE = 0x00000002,
	IMAGE_VALID_RESOURCE = 0x00000004,
	IMAGE_VALID_RELOC = 0x00000008,
};


#define EXTC_IMPORT extern "C" __declspec(dllimport)

EXTC_IMPORT
BOOL
WINAPI
CreateProcessInternalW(
	HANDLE                  Token,
	PCWSTR                  ApplicationName,
	PWSTR                   CommandLine,
	LPSECURITY_ATTRIBUTES   ProcessAttributes,
	LPSECURITY_ATTRIBUTES   ThreadAttributes,
	BOOL                    InheritHandles,
	DWORD                   CreationFlags,
	LPVOID                  Environment,
	PCWSTR                  CurrentDirectory,
	LPSTARTUPINFOW          StartupInfo,
	LPPROCESS_INFORMATION   ProcessInformation,
	PHANDLE                 NewToken
);

PVOID LookupImportTable(PVOID ImageBase, PCSTR DllName, PCSTR RoutineName);
PVOID LookupImportTable(PVOID ImageBase, PCSTR DllName, ULONG Hash);


BOOL
IsValidImage(
	PVOID       ImageBase,
	ULONG_PTR   DEF_VAL(Flags, 0)
);


PVOID
GetImageBaseAddress(
	PVOID ImageAddress
);

inline PIMAGE_NT_HEADERS ImageNtHeadersFast(PVOID ImageBase, PULONG_PTR NtHeadersVersion = nullptr)
{
	PIMAGE_NT_HEADERS32 NtHeaders32;

	NtHeaders32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)ImageBase + ((PIMAGE_DOS_HEADER)ImageBase)->e_lfanew);

	if (NtHeadersVersion != nullptr) switch (NtHeaders32->OptionalHeader.Magic)
	{
	case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
	case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
		*NtHeadersVersion = NtHeaders32->OptionalHeader.Magic;
		break;

	default:
		return nullptr;
	}

	return (PIMAGE_NT_HEADERS)NtHeaders32;
}

inline PIMAGE_NT_HEADERS ImageNtHeaders(PVOID ImageBase, PULONG_PTR NtHeadersVersion = nullptr)
{
	if (!IsValidImage(ImageBase))
		return nullptr;

	return ImageNtHeadersFast(ImageBase, NtHeadersVersion);
}


typedef const LDR_DATA_TABLE_ENTRY* PCLDR_DATA_TABLE_ENTRY;

typedef LDR_DATA_TABLE_ENTRY LDR_MODULE;
typedef PLDR_DATA_TABLE_ENTRY PLDR_MODULE;
typedef PCLDR_DATA_TABLE_ENTRY PCLDR_MODULE;


ForceInline void* AllocStack(size_t Size)
{
	return _alloca(ROUND_UP(Size, 16));
	return _alloca(ROUND_UP(Size, 16));
}


#define DELAY_ONE_MICROSECOND   (-10)
#define DELAY_ONE_MILLISECOND   (DELAY_ONE_MICROSECOND * 1000)
#define DELAY_ONE_SECOND        (DELAY_ONE_MILLISECOND * 1000)
#define DELAY_QUAD_INFINITE     0x8000000000000000ll

inline PLARGE_INTEGER FormatTimeOut(PLARGE_INTEGER TimeOut, ULONG Milliseconds)
{
	if (Milliseconds == INFINITE)
	{
		//TimeOut->LowPart = 0;
		//TimeOut->HighPart = 0x80000000;
		TimeOut->QuadPart = DELAY_QUAD_INFINITE;
	}
	else
	{
		TimeOut->QuadPart = DELAY_ONE_MILLISECOND * (LONG64)Milliseconds;
	}

	return TimeOut;
}

NTSTATUS
ExceptionBox(
	PCWSTR      Text,
	PCWSTR      Title = nullptr,
	PULONG_PTR  Response = nullptr,
	ULONG_PTR   Type = MB_ICONASTERISK | MB_OK
);

inline LONG_PTR InitNext(const VOID *lpKeyWord, LONG_PTR KeyWordLen, PLONG_PTR piNext)
{
	LONG_PTR i, j;
	const CHAR *pDest;

	pDest = (PCCHAR)lpKeyWord;

	piNext[0] = -1;
	i = 0;
	j = (LONG_PTR)-1;

	while (i < KeyWordLen)
	{
		if (j == -1 || pDest[i] == pDest[j])
		{
			if (pDest[++i] != pDest[++j])
			{
				piNext[i] = j;
			}
			else
			{
				piNext[i] = piNext[j];
			}
		}
		else
		{
			j = piNext[j];
		}
	}

	return KeyWordLen;
}

inline PVOID KMP(const PVOID _Src, LONG_PTR SrcLength, const PVOID _KeyWord, LONG_PTR KeyWordLen)
{
	LONG_PTR i, j;
	PLONG_PTR  Next;//[100];
	PLONG_PTR pNext;
	const CHAR *pSrc, *pKeyWord;

	if (SrcLength == 0)
		return nullptr;

	Next = (PLONG_PTR)AllocStack((KeyWordLen + 1) * sizeof(*pNext));
	pNext = Next;
	pSrc = (const CHAR *)_Src;
	pKeyWord = (const CHAR *)_KeyWord;
	InitNext(_KeyWord, KeyWordLen, pNext);
	i = 0;
	j = 0;
	while (i != SrcLength && j != KeyWordLen)
	{
		if (j == -1 || pSrc[i] == pKeyWord[j])
		{
			++i;
			++j;
		}
		else
		{
			j = pNext[j];
		}
	}

	if (i == SrcLength && j != KeyWordLen)
		return nullptr;

	return (VOID *)(pSrc + i - KeyWordLen);
}


template<class PtrType>
ForceInline
PtrType GetCallDestination(PtrType Buffer)
{
	union
	{
		PtrType     Pointer;
		ULONG_PTR   Value;
	};

	Pointer = Buffer;
	Value = *(PLONG)(Value + 1) + Value + sizeof(ULONG) + 1;

	return Pointer;
}

ULONG_PTR FASTCALL LdeGetOpCodeSize32(PVOID Code, PVOID *OpCodePtr = NULL);
ULONG_PTR FASTCALL LdeGetOpCodeSize64(PVOID Code, PVOID *OpCodePtr = NULL);

ForceInline ULONG_PTR GetOpCodeSize32(PVOID Buffer)
{
	return LdeGetOpCodeSize32(Buffer);
}

ForceInline ULONG_PTR GetOpCodeSize64(PVOID Buffer)
{
	return LdeGetOpCodeSize64(Buffer);
}

PVOID
Nt_LoadLibrary(
	PWSTR ModuleFileName
);

PVOID
FASTCALL
Nt_GetProcAddress(
	PVOID ModuleBase,
	PCSTR ProcedureName
);


ULONG
Nt_GetExeDirectory(
	PWSTR Path,
	ULONG BufferCount
);


PVOID
Nt_GetModuleHandle(
	LPCWSTR lpModuleName
);

NTSTATUS
Nt_CreateThread(
	PVOID                   StartAddress,
	PVOID                   DEF_VAL(StartParameter, NULL),
	BOOL                    DEF_VAL(CreateSuspended, FALSE),
	HANDLE                  DEF_VAL(ProcessHandle, NtCurrentProcess()),
	PHANDLE                 DEF_VAL(ThreadHandle, NULL),
	PCLIENT_ID              DEF_VAL(ClientID, NULL),
	ULONG                   DEF_VAL(StackZeroBits, 0),
	ULONG                   DEF_VAL(StackReserved, 0),
	ULONG                   DEF_VAL(StackCommit, 0),
	PSECURITY_DESCRIPTOR    DEF_VAL(SecurityDescriptor, NULL)
);


NTSTATUS
Nt_CreateProcess(
	HANDLE                  Token,
	PCWSTR                  ApplicationName,
	PWSTR                   DEF_VAL(CommandLine, NULL),
	PCWSTR                  DEF_VAL(CurrentDirectory, NULL),
	ULONG                   DEF_VAL(CreationFlags, 0),
	LPSTARTUPINFOW          DEF_VAL(StartupInfo, NULL),
	LPPROCESS_INFORMATION   DEF_VAL(ProcessInformation, NULL),
	LPSECURITY_ATTRIBUTES   DEF_VAL(ProcessAttributes, NULL),
	LPSECURITY_ATTRIBUTES   DEF_VAL(ThreadAttributes, NULL),
	PVOID                   DEF_VAL(Environment, NULL)
);


ULONG
Nt_WaitForSingleObjectEx(
	IN HANDLE hHandle,
	IN ULONG  Milliseconds,
	IN BOOL   bAlertable
);

inline ULONG_PTR ReadFsPtr(ULONG_PTR Offset)
{
#if defined(_AMD64_)
	return (ULONG_PTR)__readgsqword((ULONG)Offset);
#else // x86
	return (ULONG_PTR)__readfsdword(Offset);
#endif
}


#define TEB_OFFSET FIELD_OFFSET(TEB, NtTib.Self)
#define PEB_OFFSET FIELD_OFFSET(TEB, ProcessEnvironmentBlock)

ForceInline
PPEB Nt_CurrentPeb()
{
	return (PPEB)(ULONG_PTR)ReadFsPtr(PEB_OFFSET);
}


ForceInline PTEB Nt_CurrentTeb()
{
	return (PTEB)ReadFsPtr(TEB_OFFSET);
}

ULONG_PTR   PrintConsoleA(PCSTR Format, ...);
ULONG_PTR   PrintConsoleW(PCWSTR Format, ...);


ForceInline
VOID
SetLastNTError(
	NTSTATUS Status
)
{
	RtlSetLastWin32ErrorAndNtStatusFromNtStatus(Status);
}


LONG_PTR    FASTCALL CmdLineToArgvWorkerA(LPSTR pszCmdLine, LPSTR *pArgv, PLONG_PTR pCmdLineLength);
PSTR*       FASTCALL CmdLineToArgvA(LPSTR pszCmdLine, PLONG_PTR pArgc);
LONG_PTR    FASTCALL CmdLineToArgvWorkerW(LPWSTR pszCmdLine, LPWSTR *pArgv, PLONG_PTR pCmdLineLength);
PWSTR*      FASTCALL CmdLineToArgvW(LPWSTR pszCmdLine, PLONG_PTR pArgc);

inline VOID ReleaseArgv(PVOID Argv)
{
	HeapFree(GetProcessHeap(), 0, Argv);
}

#pragma pack(push, 1)

typedef struct
{
	UINT16 Tag;                   // 0x00
	UINT32 FileSize;             // 0x02
	UINT32 Reserve;              // 0x06
	UINT32 RawOffset;            // 0x0A
	struct
	{
		UINT32 InfoHeaderSize;   // 0x0E
		INT32  Width;            // 0x12
		INT32  Height;           // 0x16
		UINT16 Layer;             // 0x1A
		UINT16 Bit;               // 0x1C
		UINT32 Compressed;       // 0x1E
		UINT32 SizeImage;        // 0x22
		INT32  XPelsPerMeter;     // 0x26
		INT32  YPelsPerMeter;     // 0x2A
		UINT32 ClrUsed;          // 0x2E
		UINT32 ClrImportant;     // 0x32
	} Info;
} IMAGE_BITMAP_HEADER, *PIMAGE_BITMAP_HEADER;

enum
{
	TgaImageRga = 2,
};

typedef struct
{
	UCHAR  ID;
	UCHAR  PaletteType;
	UCHAR  ImageType;
	UINT16 PaletteEntry;
	UINT16 PaletteLength;
	UCHAR  PaletteBitCount;
	UINT16 OriginX;
	UINT16 OriginY;
	UINT16 Width;
	UINT16 Height;
	UCHAR  PixelDepth;
	UCHAR  ImageDescription;

} IMAGE_TGA_HEADER, *PIMAGE_TGA_HEADER;

typedef struct
{
	UCHAR   Width;
	UCHAR   Height;
	UCHAR   NumberOfColorInPalette;
	UCHAR   Reserve;

	union
	{
		struct
		{
			USHORT  Planes;
			USHORT  BitsPerPixel;
		} Ico;

		struct
		{
			USHORT  X;
			USHORT  Y;
		} Cur;
	};

	ULONG ImageSize;
	ULONG ImageOffset;

} IMAGE_ICO_IMAGE_ENTRY, *PIMAGE_ICO_IMAGE_ENTRY;

typedef struct
{
	USHORT Reserve;
	USHORT FileType;   // 1
	USHORT NumberOfImage;

	IMAGE_ICO_IMAGE_ENTRY   Entry[1];

} IMAGE_ICO_HEADER, *PIMAGE_ICO_HEADER;

#pragma pack(pop)



ForceInline LONG GetBitmapStride(LONG Width, LONG BitsPerPixel)
{
	return (Width * BitsPerPixel / 8 + 3) & ~3;
}

inline
BOOL
FASTCALL
InitBitmapHeader(
	IMAGE_BITMAP_HEADER*Header,
	INT32               Width,
	INT32               Height,
	INT32               BitsPerPixel,
	PLONG_PTR           Stride
)
{
	UINT32 LocalStride, PaletteSize;

	if (Header == NULL)
		return FALSE;

	ZeroMemory(Header, sizeof(*Header));

	PaletteSize = BitsPerPixel == 8 ? 256 * 4 : 0;

	Header->RawOffset = sizeof(*Header) + PaletteSize;
	Header->Info.Height = Height;
	Header->Info.Width = Width;
	Header->Tag = TAG2('BM');
	Header->Info.InfoHeaderSize = sizeof(Header->Info);
	Header->Info.Layer = 1;
	Header->Info.Bit = (UINT16)BitsPerPixel;
	//    pHeader->Info.dwClrUsed = 1;
	LocalStride = (Width * BitsPerPixel / 8 + 3) & ~3;
	if (Stride)
		*Stride = LocalStride;

	Header->FileSize = Height * LocalStride + sizeof(*Header) + PaletteSize;

	return TRUE;
}

#define FILE_ATTRIBUTE_VALID_FLAGS          0x00007fb7
#define FILE_ATTRIBUTE_VALID_SET_FLAGS      0x000031a7

#define FILE_OPEN_REMOTE_INSTANCE               0x00000400

#define NFD_NOT_RESOLVE_PATH    (0x00000001u)
#define NFD_EXPAND_ENVVAR       (0x00000002u)

class NtFileDisk
{
protected:
	HANDLE          m_FileHandle;
	LARGE_INTEGER   m_FileSize, m_Position;

public:
	NtFileDisk();
	ForceInline NtFileDisk(const NtFileDisk &file);
	ForceInline operator HANDLE() const;
	NtFileDisk& operator=(const NtFileDisk &file);
	NtFileDisk& operator=(HANDLE Handle);

	~NtFileDisk()
	{
		Close();
	}

protected:
	static
		NTSTATUS
		CreateFileInternal(
			PHANDLE     FileHandle,
			PCWSTR      FileName,
			ULONG_PTR   Flags,
			ULONG_PTR   ShareMode,
			ULONG_PTR   Access,
			ULONG_PTR   CreationDisposition,
			ULONG_PTR   Attributes,
			ULONG_PTR   CreateOptions = 0
		);

	static
		NTSTATUS
		CreateFileInternalWithFullPath(
			PHANDLE         FileHandle,
			PUNICODE_STRING FileName,
			ULONG_PTR           Flags,
			ULONG_PTR           ShareMode,
			ULONG_PTR           Access,
			ULONG_PTR           CreationDisposition,
			ULONG_PTR           Attributes,
			ULONG_PTR           CreateOptions = 0
		);

	static
		NTSTATUS
		GetSizeInternal(
			HANDLE          FileHandle,
			PLARGE_INTEGER  FileSize
		);

	static
		NTSTATUS
		SeekInternal(
			HANDLE          FileHandle,
			LARGE_INTEGER   Offset,
			ULONG_PTR       MoveMethod,
			PLARGE_INTEGER  NewPosition = nullptr
		);

	static
		NTSTATUS
		ReadInternal(
			HANDLE          FileHandle,
			PVOID           Buffer,
			ULONG_PTR           Size,
			PLARGE_INTEGER  pBytesRead = nullptr,
			PLARGE_INTEGER  Offset = nullptr
		);

	static
		NTSTATUS
		WriteInternal(
			HANDLE          FileHandle,
			PVOID           Buffer,
			ULONG_PTR           Size,
			PLARGE_INTEGER  pBytesWritten = nullptr,
			PLARGE_INTEGER  Offset = nullptr
		);

	static
		NTSTATUS
		DeleteInternal(
			HANDLE FileHandle
		);

	static
		NTSTATUS
		SetEndOfFileInternal(
			HANDLE FileHandle,
			LARGE_INTEGER EndPosition
		);

	static
		NTSTATUS
		QuerySymbolicTargetInternal(
			HANDLE                  FileHandle,
			PREPARSE_DATA_BUFFER    ReparseBuffer,
			ULONG_PTR               BufferSize,
			PULONG_PTR              BytesReturned = nullptr
		);

	static
		NTSTATUS
		QuerySymbolicTargetInternal(
			HANDLE      FileHandle,
			PWSTR       PrintNameBuffer,
			PULONG_PTR  PrintNameBufferCount,
			PWSTR       SubstituteNameBuffer = nullptr,
			PULONG_PTR  SubstituteNameBufferCount = nullptr
		);

public:
	NTSTATUS Close();

	NTSTATUS
		Open(
			PCSTR       FileName,
			ULONG_PTR   Flags = 0,
			ULONG_PTR   ShareMode = FILE_SHARE_READ,
			ULONG_PTR   Access = GENERIC_READ,
			ULONG_PTR   CreationDisposition = FILE_OPEN,
			ULONG_PTR   Attributes = FILE_ATTRIBUTE_NORMAL,
			ULONG_PTR   CreateOptions = FILE_OPEN_FOR_BACKUP_INTENT
		)
	{
		NTSTATUS        Status;
		UNICODE_STRING  FileNameU;
		ANSI_STRING     FileNameA;

		RtlInitAnsiString(&FileNameA, (PSTR)FileName);
		RtlAnsiStringToUnicodeString(&FileNameU, &FileNameA, TRUE);

		Status = Open(FileNameU.Buffer, Flags, ShareMode, Access, CreationDisposition, Attributes, CreateOptions);

		RtlFreeUnicodeString(&FileNameU);

		return Status;
	}

	ForceInline
		NTSTATUS
		Open(
			PCWSTR      FileName,
			ULONG_PTR   Flags = 0,
			ULONG_PTR   ShareMode = FILE_SHARE_READ,
			ULONG_PTR   Access = GENERIC_READ,
			ULONG_PTR   CreationDisposition = FILE_OPEN,
			ULONG_PTR   Attributes = FILE_ATTRIBUTE_NORMAL,
			ULONG_PTR   CreateOptions = FILE_OPEN_FOR_BACKUP_INTENT
		)
	{
		return Create(FileName, Flags, ShareMode, Access, CreationDisposition, Attributes, CreateOptions);
	}

	ForceInline
		NTSTATUS
		CreateIfNotExist(
			PCWSTR      FileName,
			ULONG_PTR   Flags = 0,
			ULONG_PTR   ShareMode = FILE_SHARE_READ,
			ULONG_PTR   Access = GENERIC_READ | GENERIC_WRITE,
			ULONG_PTR   CreationDisposition = FILE_OPEN_IF,
			ULONG_PTR   Attributes = FILE_ATTRIBUTE_NORMAL,
			ULONG_PTR   CreateOptions = FILE_OPEN_FOR_BACKUP_INTENT
		)
	{
		return Create(FileName, Flags, ShareMode, Access, CreationDisposition, Attributes, CreateOptions);
	}

	NTSTATUS
		Modify(
			PCWSTR      FileName,
			ULONG_PTR   Flags = 0,
			ULONG_PTR   ShareMode = FILE_SHARE_READ,
			ULONG_PTR   Access = GENERIC_READ | GENERIC_WRITE,
			ULONG_PTR   CreationDisposition = FILE_OPEN_IF,
			ULONG_PTR   Attributes = FILE_ATTRIBUTE_NORMAL,
			ULONG_PTR   CreateOptions = FILE_OPEN_FOR_BACKUP_INTENT
		)
	{
		return Create(FileName, Flags, ShareMode, Access, CreationDisposition, Attributes, CreateOptions);
	}

	NTSTATUS
		Append(
			PCWSTR      FileName,
			ULONG_PTR   Flags = 0,
			ULONG_PTR   ShareMode = FILE_SHARE_READ,
			ULONG_PTR   Access = GENERIC_READ | GENERIC_WRITE,
			ULONG_PTR   CreationDisposition = FILE_OPEN_IF,
			ULONG_PTR   Attributes = FILE_ATTRIBUTE_NORMAL,
			ULONG_PTR   CreateOptions = FILE_OPEN_FOR_BACKUP_INTENT
		)
	{
		NTSTATUS Status;

		Status = Create(FileName, Flags, ShareMode, Access, CreationDisposition, Attributes, CreateOptions);

		if (NT_SUCCESS(Status))
			Status = Seek(0ll, FILE_END);

		return Status;
	}

	NTSTATUS
		Create(
			PCWSTR      FileName,
			ULONG_PTR   Flags = 0,
			ULONG_PTR   ShareMode = FILE_SHARE_READ,
			ULONG_PTR   Access = GENERIC_WRITE,
			ULONG_PTR   CreationDisposition = FILE_OVERWRITE_IF,
			ULONG_PTR   Attributes = FILE_ATTRIBUTE_NORMAL,
			ULONG_PTR   CreateOptions = FILE_OPEN_FOR_BACKUP_INTENT
		);

	ForceInline
		NTSTATUS
		OpenDirectory(
			PCWSTR      FileName,
			ULONG_PTR   Flags = 0,
			ULONG_PTR   ShareMode = FILE_SHARE_READ,
			ULONG_PTR   Access = GENERIC_READ,
			ULONG_PTR   CreationDisposition = FILE_OPEN,
			ULONG_PTR   Attributes = FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_DIRECTORY,
			ULONG_PTR   CreateOptions = FILE_OPEN_FOR_BACKUP_INTENT | FILE_DIRECTORY_FILE
		)
	{
		return CreateDirectory(FileName, Flags, ShareMode, Access, CreationDisposition, Attributes, CreateOptions);
	}

	ForceInline
		NTSTATUS
		CreateDirectory(
			PCWSTR      FileName,
			ULONG_PTR   Flags = 0,
			ULONG_PTR   ShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE,
			ULONG_PTR   Access = GENERIC_ALL,
			ULONG_PTR   CreationDisposition = FILE_CREATE,
			ULONG_PTR   Attributes = FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_DIRECTORY,
			ULONG_PTR   CreateOptions = FILE_OPEN_FOR_BACKUP_INTENT | FILE_DIRECTORY_FILE
		)
	{
		Close();
		return CreateFileInternal(
			&m_FileHandle,
			FileName,
			Flags,
			ShareMode,
			Access,
			CreationDisposition,
			Attributes,
			CreateOptions
		);
	}

	NTSTATUS
		OpenVolume(
			ULONG_PTR   Drive,
			ULONG_PTR   Flags = 0,
			ULONG_PTR   ShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			ULONG_PTR   Access = GENERIC_READ,
			ULONG_PTR   CreationDisposition = FILE_OPEN,
			ULONG_PTR   Attributes = 0,
			ULONG_PTR   CreateOptions = FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
		)
	{
		WCHAR Volume[0x10];

		Volume[0] = '\\';
		Volume[1] = '\\';
		Volume[2] = '.';
		Volume[3] = '\\';
		Volume[4] = (WCHAR)Drive;
		Volume[5] = ':';
		Volume[6] = 0;

		return OpenDirectory(Volume, Flags, ShareMode, Access, CreationDisposition, Attributes, CreateOptions);
	}

	NTSTATUS
		OpenDevice(
			PCWSTR      FileName,
			ULONG_PTR   Flags = 0,
			ULONG_PTR   ShareMode = FILE_SHARE_READ,
			ULONG_PTR   Access = FILE_READ_ATTRIBUTES | SYNCHRONIZE,
			ULONG_PTR   CreationDisposition = FILE_OPEN,
			ULONG_PTR   Attributes = 0,
			ULONG_PTR   CreateOptions = FILE_OPEN_FOR_BACKUP_INTENT
		);

	NTSTATUS
		DeviceIoControl(
			ULONG_PTR   IoControlCode,
			PVOID       InputBuffer,
			ULONG_PTR   InputBufferLength,
			PVOID       OutputBuffer,
			ULONG_PTR   OutputBufferLength,
			PULONG_PTR  BytesReturned = nullptr
		);

	ForceInline
		NTSTATUS
		OpenSymbolicLink(
			PCWSTR      FileName,
			ULONG_PTR   Flags = 0,
			ULONG_PTR   ShareMode = FILE_SHARE_READ,
			ULONG_PTR   Access = GENERIC_READ,
			ULONG_PTR   CreationDisposition = FILE_OPEN,
			ULONG_PTR   Attributes = 0,
			ULONG_PTR   CreateOptions = FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REPARSE_POINT | FILE_SYNCHRONOUS_IO_NONALERT
		)
	{
		return CreateSymbolicLink(FileName, Flags, ShareMode, Access, CreationDisposition, Attributes, CreateOptions);
	}

	ForceInline
		NTSTATUS
		CreateSymbolicLink(
			PCWSTR      FileName,
			ULONG_PTR   Flags = 0,
			ULONG_PTR   ShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE,
			ULONG_PTR   Access = GENERIC_ALL,
			ULONG_PTR   CreationDisposition = FILE_CREATE,
			ULONG_PTR   Attributes = 0,
			ULONG_PTR   CreateOptions = FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REPARSE_POINT | FILE_SYNCHRONOUS_IO_NONALERT
		)
	{
		Close();

		return CreateFileInternal(
			&m_FileHandle,
			FileName,
			Flags,
			ShareMode,
			Access,
			CreationDisposition,
			Attributes,
			CreateOptions
		);
	}

	ForceInline
		NTSTATUS
		QuerySymbolicTarget(
			PREPARSE_DATA_BUFFER    ReparseBuffer,
			ULONG_PTR               BufferSize,
			PULONG_PTR              BytesReturned = nullptr
		)
	{
		return QuerySymbolicTargetInternal(
			m_FileHandle,
			ReparseBuffer,
			BufferSize,
			BytesReturned
		);
	}

	ForceInline
		NTSTATUS
		QuerySymbolicTarget(
			PWSTR                   PrintNameBuffer,
			PULONG_PTR              PrintNameBufferCount,
			PWSTR                   SubstituteNameBuffer = nullptr,
			PULONG_PTR              SubstituteNameBufferCount = nullptr
		)
	{
		return QuerySymbolicTargetInternal(
			m_FileHandle,
			PrintNameBuffer,
			PrintNameBufferCount,
			SubstituteNameBuffer,
			SubstituteNameBufferCount
		);
	}

	NTSTATUS
		GetSize(
			PLARGE_INTEGER FileSize
		);

	NTSTATUS
		GetPosition(
			PLARGE_INTEGER Position
		);

	ForceInline
		NTSTATUS
		Seek(
			LONG64          Offset,
			ULONG_PTR       MoveMethod = FILE_BEGIN,
			PLARGE_INTEGER  NewPosition = nullptr
		)
	{
		LARGE_INTEGER NewOffset;

		NewOffset.QuadPart = Offset;
		return Seek(NewOffset, MoveMethod, NewPosition);
	}

	NTSTATUS
		Seek(
			LARGE_INTEGER   Offset,
			ULONG_PTR       MoveMethod = FILE_BEGIN,
			PLARGE_INTEGER  NewPosition = nullptr
		);

	NTSTATUS
		Read(
			PVOID           Buffer,
			ULONG_PTR       Size = 0,
			PLARGE_INTEGER  BytesRead = nullptr
		);

	NTSTATUS
		Write(
			PVOID           Buffer,
			ULONG_PTR       Size,
			PLARGE_INTEGER  BytesWritten = nullptr
		);

	NTSTATUS
		Print(
			PLARGE_INTEGER  BytesWritten,
			PWSTR           Format,
			...
		);

	NTSTATUS
		Print(
			PLARGE_INTEGER  BytesWritten,
			PSTR            Format,
			...
		);

	NTSTATUS
		MapView(
			PVOID*  BaseAddress,
			HANDLE  ProcessHandle = NtCurrentProcess()
		);

	NTSTATUS
		UnMapView(
			PVOID   BaseAddress,
			HANDLE  ProcessHandle = NtCurrentProcess()
		);

	NTSTATUS SetEndOfFile();
	NTSTATUS SetEndOfFile(LARGE_INTEGER EndPosition);

	NTSTATUS SetEndOfFile(ULONG64 EndPosition)
	{
		LARGE_INTEGER LargeEndPosition;
		LargeEndPosition.QuadPart = EndPosition;
		return SetEndOfFile(LargeEndPosition);
	}

	NTSTATUS Delete()
	{
		return DeleteInternal(m_FileHandle);
	}

	ULONG_PTR GetSize32()
	{
		return (ULONG_PTR)GetSize64();
	}

	ULONG_PTR GetCurrentPos()
	{
		return (ULONG_PTR)GetCurrentPos64();
	}

	ULONG64 GetCurrentPos64()
	{
		return m_Position.QuadPart;
	}

	NTSTATUS Rewind()
	{
		return Seek(0ll, FILE_BEGIN);
	}

	HANDLE GetHandle() const
	{
		return m_FileHandle;
	}

	LONG64 GetSize64()
	{
		return m_FileSize.QuadPart;
	}

	/************************************************************************
	static ver
	************************************************************************/

	ForceInline
		static
		NTSTATUS
		Close(
			HANDLE FileHandle
		)
	{
		return NtClose(FileHandle);
	}

	ForceInline
		static
		NTSTATUS
		Open(
			PHANDLE     FileHandle,
			PCWSTR      FileName,
			ULONG_PTR   Flags = 0,
			ULONG_PTR   ShareMode = FILE_SHARE_READ,
			ULONG_PTR   Access = GENERIC_READ,
			ULONG_PTR   CreationDisposition = FILE_OPEN,
			ULONG_PTR   Attributes = FILE_ATTRIBUTE_NORMAL,
			ULONG_PTR   CreateOptions = FILE_OPEN_FOR_BACKUP_INTENT
		)
	{
		return Create(FileHandle, FileName, Flags, ShareMode, Access, CreationDisposition, Attributes, CreateOptions);
	}

	ForceInline
		static
		NTSTATUS
		Create(
			PHANDLE     FileHandle,
			PCWSTR      FileName,
			ULONG_PTR   Flags = 0,
			ULONG_PTR   ShareMode = FILE_SHARE_READ,
			ULONG_PTR   Access = GENERIC_WRITE,
			ULONG_PTR   CreationDisposition = FILE_OVERWRITE_IF,
			ULONG_PTR   Attributes = FILE_ATTRIBUTE_NORMAL,
			ULONG_PTR   CreateOptions = FILE_OPEN_FOR_BACKUP_INTENT
		)
	{
		return CreateFileInternal(FileHandle, FileName, Flags, ShareMode, Access, CreationDisposition, Attributes, CreateOptions);
	}

	ForceInline
		static
		NTSTATUS
		CreateDirectory(
			PHANDLE     FileHandle,
			PCWSTR      FileName,
			ULONG_PTR   Flags = 0,
			ULONG_PTR   ShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE,
			ULONG_PTR   Access = GENERIC_ALL,
			ULONG_PTR   CreationDisposition = FILE_CREATE,
			ULONG_PTR   Attributes = FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_DIRECTORY,
			ULONG_PTR   CreateOptions = FILE_OPEN_FOR_BACKUP_INTENT | FILE_DIRECTORY_FILE
		)
	{
		return Create(
			FileHandle,
			FileName,
			Flags,
			ShareMode,
			Access,
			CreationDisposition,
			Attributes,
			CreateOptions
		);
	}

	ForceInline
		static
		NTSTATUS
		OpenDirectory(
			PHANDLE     FileHandle,
			PCWSTR      FileName,
			ULONG_PTR   Flags = 0,
			ULONG_PTR   ShareMode = FILE_SHARE_READ,
			ULONG_PTR   Access = GENERIC_READ,
			ULONG_PTR   CreationDisposition = FILE_OPEN,
			ULONG_PTR   Attributes = FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_DIRECTORY,
			ULONG_PTR   CreateOptions = FILE_OPEN_FOR_BACKUP_INTENT | FILE_DIRECTORY_FILE
		)
	{
		return CreateDirectory(FileHandle, FileName, Flags, ShareMode, Access, CreationDisposition, Attributes, CreateOptions);
	}

	ForceInline
		static
		NTSTATUS
		OpenVolume(
			PHANDLE     FileHandle,
			ULONG_PTR   Drive,
			ULONG_PTR   Flags = 0,
			ULONG_PTR   ShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			ULONG_PTR   Access = GENERIC_READ,
			ULONG_PTR   CreationDisposition = FILE_OPEN,
			ULONG_PTR   Attributes = 0,
			ULONG_PTR   CreateOptions = FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
		)
	{
		WCHAR Volume[0x10];

		Volume[0] = '\\';
		Volume[1] = '\\';
		Volume[2] = '.';
		Volume[3] = '\\';
		Volume[4] = (WCHAR)Drive;
		Volume[5] = ':';
		Volume[6] = 0;

		return OpenDirectory(FileHandle, Volume, Flags, ShareMode, Access, CreationDisposition, Attributes, CreateOptions);
	}

	static
		NTSTATUS
		OpenDevice(
			PHANDLE     FileHandle,
			PCWSTR      FileName,
			ULONG_PTR   Flags = 0,
			ULONG_PTR   ShareMode = FILE_SHARE_READ,
			ULONG_PTR   Access = FILE_READ_ATTRIBUTES | SYNCHRONIZE,
			ULONG_PTR   CreationDisposition = FILE_OPEN,
			ULONG_PTR   Attributes = 0,
			ULONG_PTR   CreateOptions = FILE_OPEN_FOR_BACKUP_INTENT
		)
	{
		NTSTATUS        Status;
		UNICODE_STRING  DeviceName;

		RtlInitUnicodeString(&DeviceName, (PWSTR)FileName);
		Status = CreateFileInternalWithFullPath(FileHandle, &DeviceName, Flags, ShareMode, Access, CreationDisposition, Attributes, CreateOptions);
		return Status;
	}

	ForceInline
		static
		NTSTATUS
		OpenSymbolicLink(
			PHANDLE     FileHandle,
			PCWSTR      FileName,
			ULONG_PTR   Flags = 0,
			ULONG_PTR   ShareMode = FILE_SHARE_READ,
			ULONG_PTR   Access = GENERIC_READ,
			ULONG_PTR   CreationDisposition = FILE_OPEN,
			ULONG_PTR   Attributes = 0,
			ULONG_PTR   CreateOptions = FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REPARSE_POINT | FILE_SYNCHRONOUS_IO_NONALERT
		)
	{
		return CreateSymbolicLink(FileHandle, FileName, Flags, ShareMode, Access, CreationDisposition, Attributes, CreateOptions);
	}

	ForceInline
		static
		NTSTATUS
		CreateSymbolicLink(
			PHANDLE     FileHandle,
			PCWSTR      FileName,
			ULONG_PTR   Flags = 0,
			ULONG_PTR   ShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE,
			ULONG_PTR   Access = GENERIC_ALL,
			ULONG_PTR   CreationDisposition = FILE_CREATE,
			ULONG_PTR   Attributes = 0,
			ULONG_PTR   CreateOptions = FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REPARSE_POINT | FILE_SYNCHRONOUS_IO_NONALERT
		)
	{
		return CreateFileInternal(
			FileHandle,
			FileName,
			Flags,
			ShareMode,
			Access,
			CreationDisposition,
			Attributes,
			CreateOptions
		);
	}

	ForceInline
		static
		NTSTATUS
		QuerySymbolicTarget(
			HANDLE                  FileHandle,
			PREPARSE_DATA_BUFFER    ReparseBuffer,
			ULONG_PTR               BufferSize,
			PULONG_PTR              BytesReturned = nullptr
		)
	{
		return QuerySymbolicTargetInternal(
			FileHandle,
			ReparseBuffer,
			BufferSize,
			BytesReturned
		);
	}

	ForceInline
		static
		NTSTATUS
		QuerySymbolicTarget(
			HANDLE                  FileHandle,
			PWSTR                   PrintNameBuffer,
			PULONG_PTR              PrintNameBufferCount,
			PWSTR                   SubstituteNameBuffer = nullptr,
			PULONG_PTR              SubstituteNameBufferCount = nullptr
		)
	{
		return QuerySymbolicTargetInternal(
			FileHandle,
			PrintNameBuffer,
			PrintNameBufferCount,
			SubstituteNameBuffer,
			SubstituteNameBufferCount
		);
	}

	ForceInline
		static
		NTSTATUS
		GetSize(
			HANDLE          FileHandle,
			PLARGE_INTEGER  FileSize
		)
	{
		return GetSizeInternal(FileHandle, FileSize);
	}

	ForceInline
		static
		NTSTATUS
		Read(
			HANDLE          FileHandle,
			PVOID           Buffer,
			ULONG_PTR       Size,
			PLARGE_INTEGER  BytesRead = nullptr,
			PLARGE_INTEGER  Offset = nullptr
		)
	{
		return ReadInternal(FileHandle, Buffer, Size, BytesRead, Offset);
	}

	ForceInline
		static
		NTSTATUS
		Write(
			HANDLE          FileHandle,
			PVOID           Buffer,
			ULONG_PTR       Size,
			PLARGE_INTEGER  BytesWritten = nullptr,
			PLARGE_INTEGER  Offset = nullptr
		)
	{
		return WriteInternal(FileHandle, Buffer, Size, BytesWritten, Offset);
	}

	ForceInline
		static
		NTSTATUS
		Seek(
			HANDLE          FileHandle,
			LONG64          Offset,
			ULONG_PTR       MoveMethod = FILE_BEGIN,
			PLARGE_INTEGER  NewPosition = nullptr
		)
	{
		LARGE_INTEGER NewOffset;

		NewOffset.QuadPart = Offset;
		return Seek(FileHandle, NewOffset, MoveMethod, NewPosition);
	}

	ForceInline
		static
		NTSTATUS
		Seek(
			HANDLE          FileHandle,
			LARGE_INTEGER   Offset,
			ULONG_PTR       MoveMethod = FILE_BEGIN,
			PLARGE_INTEGER  NewPosition = nullptr
		)
	{
		return SeekInternal(FileHandle, Offset, MoveMethod, NewPosition);
	}

	ForceInline
		static
		NTSTATUS
		SetEndOfFile(
			HANDLE FileHandle
		);

	ForceInline
		static
		NTSTATUS
		Delete(
			HANDLE FileHandle
		)
	{
		return DeleteInternal(FileHandle);
	}

	static
		NTSTATUS
		QueryFullNtPath(
			PCWSTR          FileName,
			PUNICODE_STRING NtFilePath,
			ULONG_PTR       Flags = 0
		);


	static
		HANDLE
		SimulateCreateFile(
			PCWSTR lpFileName,
			ULONG dwDesiredAccess,
			ULONG dwShareMode,
			LPSECURITY_ATTRIBUTES lpSecurityAttributes,
			ULONG dwCreationDisposition,
			ULONG dwFlagsAndAttributes,
			HANDLE hTemplateFile
		)
	{
		OBJECT_ATTRIBUTES ObjectAttributes;
		IO_STATUS_BLOCK IoStatusBlock;
		UNICODE_STRING NtPathU;
		HANDLE FileHandle;
		NTSTATUS Status;
		ULONG FileAttributes, Flags = 0;
		PVOID EaBuffer = nullptr;
		ULONG EaLength = 0;

		if (lpFileName == nullptr || lpFileName[0] == 0)
		{
			SetLastNTError(STATUS_OBJECT_PATH_NOT_FOUND);
			return INVALID_HANDLE_VALUE;
		}

		/* validate & translate the creation disposition */
		switch (dwCreationDisposition)
		{
		case CREATE_NEW:
			dwCreationDisposition = FILE_CREATE;
			break;

		case CREATE_ALWAYS:
			dwCreationDisposition = FILE_OVERWRITE_IF;
			break;

		case OPEN_EXISTING:
			dwCreationDisposition = FILE_OPEN;
			break;

		case OPEN_ALWAYS:
			dwCreationDisposition = FILE_OPEN_IF;
			break;

		case TRUNCATE_EXISTING:
			dwCreationDisposition = FILE_OVERWRITE;
			break;

		default:
			SetLastNTError(STATUS_INVALID_PARAMETER);
			return (INVALID_HANDLE_VALUE);
		}

		/* validate & translate the flags */

		/* translate the flags that need no validation */
		if (!(dwFlagsAndAttributes & FILE_FLAG_OVERLAPPED))
		{
			/* yes, nonalert is correct! apc's are not delivered
			while waiting for file io to complete */
			Flags |= FILE_SYNCHRONOUS_IO_NONALERT;
		}

		if (dwFlagsAndAttributes & FILE_FLAG_WRITE_THROUGH)
			Flags |= FILE_WRITE_THROUGH;

		if (dwFlagsAndAttributes & FILE_FLAG_NO_BUFFERING)
			Flags |= FILE_NO_INTERMEDIATE_BUFFERING;

		if (dwFlagsAndAttributes & FILE_FLAG_RANDOM_ACCESS)
			Flags |= FILE_RANDOM_ACCESS;

		if (dwFlagsAndAttributes & FILE_FLAG_SEQUENTIAL_SCAN)
			Flags |= FILE_SEQUENTIAL_ONLY;

		if (dwFlagsAndAttributes & FILE_FLAG_DELETE_ON_CLOSE)
			Flags |= FILE_DELETE_ON_CLOSE;

		if (dwFlagsAndAttributes & FILE_FLAG_BACKUP_SEMANTICS)
		{
			if (dwDesiredAccess & GENERIC_ALL)
				Flags |= FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REMOTE_INSTANCE;
			else
			{
				if (dwDesiredAccess & GENERIC_READ)
					Flags |= FILE_OPEN_FOR_BACKUP_INTENT;

				if (dwDesiredAccess & GENERIC_WRITE)
					Flags |= FILE_OPEN_REMOTE_INSTANCE;
			}
		}
		else
			Flags |= FILE_NON_DIRECTORY_FILE;

		if (dwFlagsAndAttributes & FILE_FLAG_OPEN_REPARSE_POINT)
			Flags |= FILE_OPEN_REPARSE_POINT;

		if (dwFlagsAndAttributes & FILE_FLAG_OPEN_NO_RECALL)
			Flags |= FILE_OPEN_NO_RECALL;

		FileAttributes = (dwFlagsAndAttributes & (FILE_ATTRIBUTE_VALID_FLAGS & ~FILE_ATTRIBUTE_DIRECTORY));

		/* handle may allways be waited on and querying attributes are allways allowed */
		dwDesiredAccess |= SYNCHRONIZE | FILE_READ_ATTRIBUTES;

		/* FILE_FLAG_POSIX_SEMANTICS is handled later */

		/* validate & translate the filename */
		if (!RtlDosPathNameToNtPathName_U(
			(PWSTR)lpFileName,
			&NtPathU,
			nullptr,
			nullptr))
		{
			RtlSetLastWin32Error(ERROR_PATH_NOT_FOUND);
			return INVALID_HANDLE_VALUE;
		}

		if (hTemplateFile != nullptr)
		{
			FILE_EA_INFORMATION EaInformation;

			/* try to get the size of the extended attributes, if we fail just continue
			creating the file without copying the attributes! */
			Status = NtQueryInformationFile(
				hTemplateFile,
				&IoStatusBlock,
				&EaInformation,
				sizeof(FILE_EA_INFORMATION),
				FileEaInformation
			);
			if (NT_SUCCESS(Status) && (EaInformation.EaSize != 0))
			{
				/* there's extended attributes to read, let's give it a try */
				/*
				EaBuffer = RtlAllocateHeap(RtlGetProcessHeap(),
				0,
				EaInformation.EaSize);
				*/
				EaBuffer = AllocStack(EaInformation.EaSize);
				Status = NtQueryEaFile(hTemplateFile,
					&IoStatusBlock,
					EaBuffer,
					EaInformation.EaSize,
					FALSE,
					nullptr,
					0,
					nullptr,
					TRUE
				);

				if (NT_SUCCESS(Status))
				{
					/* we successfully read the extended attributes, break the loop
					and continue */
					EaLength = EaInformation.EaSize;
				}
			}
		}

		/* build the object attributes */
		InitializeObjectAttributes(&ObjectAttributes, &NtPathU, 0, nullptr, nullptr);

		if (lpSecurityAttributes)
		{
			if (lpSecurityAttributes->bInheritHandle)
				ObjectAttributes.Attributes |= OBJ_INHERIT;

			ObjectAttributes.SecurityDescriptor = lpSecurityAttributes->lpSecurityDescriptor;
		}

		if (!(dwFlagsAndAttributes & FILE_FLAG_POSIX_SEMANTICS))
			ObjectAttributes.Attributes |= OBJ_CASE_INSENSITIVE;

		/* perform the call */
		Status = NtCreateFile(&FileHandle,
			dwDesiredAccess,
			&ObjectAttributes,
			&IoStatusBlock,
			nullptr,
			FileAttributes,
			dwShareMode,
			dwCreationDisposition,
			Flags,
			EaBuffer,
			EaLength
		);

		RtlFreeUnicodeString(&NtPathU);

		/* error */
		if (!NT_SUCCESS(Status))
		{
			/* In the case file creation was rejected due to CREATE_NEW flag
			* was specified and file with that name already exists, correct
			* last error is ERROR_FILE_EXISTS and not ERROR_ALREADY_EXISTS.
			* Note: RtlNtStatusToDosError is not the subject to blame here.
			*/
			if (Status == STATUS_OBJECT_NAME_COLLISION &&
				dwCreationDisposition == FILE_CREATE)
			{
				RtlSetLastWin32Error(ERROR_FILE_EXISTS);
			}
			else
			{
				SetLastNTError(Status);
			}

			return INVALID_HANDLE_VALUE;
		}

		/*
		create with OPEN_ALWAYS (FILE_OPEN_IF) returns info = FILE_OPENED or FILE_CREATED
		create with CREATE_ALWAYS (FILE_OVERWRITE_IF) returns info = FILE_OVERWRITTEN or FILE_CREATED
		*/
		if (dwCreationDisposition == FILE_OPEN_IF)
		{
			RtlSetLastWin32Error(IoStatusBlock.Information == FILE_OPENED ? ERROR_ALREADY_EXISTS : 0);
		}
		else if (dwCreationDisposition == FILE_OVERWRITE_IF)
		{
			RtlSetLastWin32Error(IoStatusBlock.Information == FILE_OVERWRITTEN ? ERROR_ALREADY_EXISTS : 0);
		}

		return FileHandle;
	}

};




class NtFileMemory
{
protected:
	PVOID           m_BufferBase, m_BufferCurrent, m_BufferEnd;
	LARGE_INTEGER   m_FileSize;

public:
	NtFileMemory()
	{
		m_BufferBase = nullptr;
		m_BufferCurrent = nullptr;
		m_BufferEnd = nullptr;
	}

	~NtFileMemory()
	{
		Close();
	}

	NTSTATUS Close()
	{
		return ReleaseAll();
	}

	NTSTATUS Open(
		PCWSTR      FileName,
		ULONG_PTR   Flags = 0,
		ULONG_PTR   ShareMode = FILE_SHARE_READ,
		ULONG_PTR   Access = GENERIC_READ,
		ULONG_PTR   CreationDisposition = FILE_OPEN,
		ULONG_PTR   Attributes = FILE_ATTRIBUTE_NORMAL,
		ULONG_PTR   CreateOptions = 0
	)
	{
		NtFileDisk  File;
		ULONG_PTR   Size;
		NTSTATUS    Status;

		ReleaseAll();

		Status = File.Open(FileName, Flags, ShareMode, Access, CreationDisposition, Attributes, CreateOptions);
		if (NT_FAILED(Status))
			return Status;

		Size = File.GetSize32();
		m_BufferBase = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ROUND_UP(Size + sizeof(ULONG_PTR), 16));
		if (m_BufferBase == nullptr)
			return STATUS_NO_MEMORY;

		Status = File.Read(m_BufferBase, Size);
		if (NT_FAILED(Status))
		{
			ReleaseAll();
			return Status;
		}

		*(PULONG)((PBYTE)m_BufferBase + Size) = 0;

		m_FileSize.QuadPart = Size;
		m_BufferCurrent = m_BufferBase;
		m_BufferEnd =  (PBYTE)m_BufferCurrent + Size;

		return Status;
	}

	NTSTATUS Open(PVOID Buffer, ULONG_PTR Size)
	{
		ReleaseAll();

		m_BufferBase = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ROUND_UP(Size + sizeof(ULONG_PTR), 16));
		if (m_BufferBase == nullptr)
			return STATUS_NO_MEMORY;

		CopyMemory(m_BufferBase, Buffer, Size);

		*(PULONG)((PBYTE)m_BufferBase + Size) = 0;

		m_FileSize.QuadPart = Size;
		m_BufferCurrent = m_BufferBase;
		m_BufferEnd = (PBYTE)m_BufferCurrent + Size;

		return STATUS_SUCCESS;
	}

	NTSTATUS GetPosition(PLARGE_INTEGER Position)
	{
		if (Position == nullptr)
			return STATUS_INVALID_PARAMETER;

		Position->QuadPart = (LONGLONG)m_BufferCurrent - (LONGLONG)m_BufferBase;

		return STATUS_SUCCESS;
	}

	ULONG_PTR GetPosition32()
	{
		return (LONGLONG)m_BufferCurrent - (LONGLONG)m_BufferBase;
	}

	ULONG_PTR GetSize32()
	{
		return (ULONG_PTR)GetSize64();
	}

	LONG64 GetSize64()
	{
		return m_FileSize.QuadPart;
	}

	PVOID GetBuffer()
	{
		return m_BufferCurrent;
	}

	operator PVOID()
	{
		return this->GetBuffer();
	}

	operator ULONG_PTR()
	{
		return (ULONG_PTR)this->GetSize64();
	}

	inline
		NTSTATUS
		Seek(
			LONG64          Offset,
			ULONG_PTR       MoveMethod = FILE_BEGIN,
			PLARGE_INTEGER  NewPosition = nullptr
		)
	{
		LARGE_INTEGER NewOffset;

		NewOffset.QuadPart = Offset;
		return Seek(NewOffset, MoveMethod, NewPosition);
	}

	NTSTATUS
		Seek(
			LARGE_INTEGER   Offset,
			ULONG_PTR       MoveMethod = FILE_BEGIN,
			PLARGE_INTEGER  NewPosition = nullptr
		)
	{
		LARGE_INTEGER NewOffset;

		NewOffset.QuadPart = (LONGLONG)m_BufferCurrent - (LONGLONG)m_BufferBase;

		switch (MoveMethod)
		{
		case FILE_CURRENT:
			NewOffset.QuadPart += Offset.QuadPart;
			break;

		case FILE_END:
			NewOffset.QuadPart = m_FileSize.QuadPart + Offset.QuadPart;
			break;

		case FILE_BEGIN:
			NewOffset.QuadPart = Offset.QuadPart;
			break;

		default:
			return STATUS_INVALID_PARAMETER_2;
		}

		if (NewOffset.QuadPart < 0)
			return STATUS_INVALID_PARAMETER_1;

		if (NewOffset.QuadPart > m_FileSize.QuadPart)
		{
			PVOID BufferBase;

			BufferBase = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, m_BufferBase, (SIZE_T)NewOffset.QuadPart);
			if (BufferBase == nullptr)
				return STATUS_NO_MEMORY;

			m_FileSize.QuadPart = NewOffset.QuadPart;
			m_BufferBase = BufferBase;
			m_BufferEnd = (PBYTE)BufferBase + m_FileSize.QuadPart;
		}

		m_BufferCurrent = (PBYTE)m_BufferBase, NewOffset.QuadPart;

		if (NewPosition != nullptr)
			NewPosition->QuadPart = NewOffset.QuadPart;

		return STATUS_SUCCESS;
	}

	NTSTATUS
		Read(
			PVOID           Buffer,
			ULONG_PTR       Size,
			PLARGE_INTEGER  BytesRead = nullptr
		)
	{
		ULONG_PTR SizeRemain;

		SizeRemain = (ULONG_PTR)m_BufferEnd - (ULONG_PTR)m_BufferCurrent;
		Size = MIN(Size, SizeRemain);

		if (Size == 0)
			return STATUS_END_OF_FILE;

		if (Buffer == nullptr)
		{
			if (BytesRead != nullptr)
			{
				BytesRead->QuadPart = SizeRemain;
				return STATUS_SUCCESS;
			}

			return STATUS_INVALID_PARAMETER_1;
		}

		CopyMemory(Buffer, m_BufferCurrent, Size);

		if (BytesRead != nullptr)
			BytesRead->QuadPart = Size;

		m_BufferCurrent = (PBYTE)m_BufferCurrent + Size;

		return STATUS_SUCCESS;
	}

	NTSTATUS
		Write(
			PVOID           Buffer,
			ULONG_PTR       Size,
			PLARGE_INTEGER  pBytesWritten = nullptr
		);

protected:
	NTSTATUS ReleaseAll()
	{
		if (m_BufferBase)
		{
			HeapFree(GetProcessHeap(), 0, m_BufferBase);
			m_BufferBase = NULL;
		}
		m_BufferCurrent = nullptr;
		m_BufferEnd = nullptr;

		return STATUS_SUCCESS;
	}
};




#define FOR_EACH_VEC(_Var, _MlGrowableArray) FOR_EACH(_Var, (_MlGrowableArray).GetData(), (_MlGrowableArray).GetSize())
#define FOR_EACH_VEC_REVERSE(_Var, _MlGrowableArray) FOR_EACH_REVERSE(_Var, (_MlGrowableArray).GetData(), (_MlGrowableArray).GetSize())

template<typename TYPE>
class GrowableArray
{
public:
	static const ULONG_PTR kInvalidIndex = ULONG_PTR_MAX;

protected:
	TYPE* m_Data;       // the actual array of data
	ULONG_PTR m_Size;       // # of elements (upperBound - 1)
	ULONG_PTR m_MaxSize;    // max allocated
	ULONG_PTR PadFor16;

	typedef const TYPE&             CONST_TYPE_REF;
	typedef TYPE&                   TYPE_REF;
	typedef GrowableArray<TYPE>&    SELF_TYPE_REF;

public:
	GrowableArray()
	{
		m_Data = nullptr;
		m_Size = 0;
		m_MaxSize = 0;
	}

	GrowableArray(const GrowableArray<TYPE>& a)
	{
		m_Data = nullptr;
		m_Size = 0;
		m_MaxSize = 0;

		*this = a;
	}

	GrowableArray(const TYPE* buf, ULONG_PTR Length)
	{
		m_Data = nullptr;
		m_Size = 0;
		m_MaxSize = 0;

		this->SetData(buf, Length);
	}

	NTSTATUS SetData(const TYPE* buf, ULONG_PTR Length)
	{
		FAIL_RETURN(this->SetSize(Length));

		CopyMemory(this->GetData(), buf, Length * sizeof(TYPE));
		this->UpdateDataCount(Length);

		return STATUS_SUCCESS;
	}

	~GrowableArray()
	{
		RemoveAll();
	}

	CONST_TYPE_REF operator[](ULONG_PTR Index) const
	{
		return GetAt(Index);
	}

	TYPE_REF operator[](ULONG_PTR Index)
	{
		return GetAt(Index);
	}

	SELF_TYPE_REF operator<<(CONST_TYPE_REF Value)
	{
		this->Add(Value);
		return *this;
	}

	SELF_TYPE_REF operator=(const GrowableArray<TYPE>& a)
	{
		if (this == &a)
			return *this;

		RemoveAll();

		TYPE *Data = a.GetData();

		for (ULONG_PTR Count = a.GetSize(); Count != 0; --Count)
			Add(*Data++);

		return *this;
	}

	ULONG_PTR Increment()
	{
		return ++m_Size;
	}

	ULONG_PTR Decrement()
	{
		return --m_Size;
	}

	ULONG_PTR UpdateDataCount(ULONG_PTR Count)
	{
		m_Size = Count;
		return Count;
	}

	NTSTATUS SetSize(ULONG_PTR NewMaxSize)
	{
		ULONG_PTR OldSize = GetSize();

		if (OldSize > NewMaxSize)
		{
			if (m_Data != nullptr)
			{
				// Removing elements. Call dtor.

				TYPE *Data = GetData() + NewMaxSize;

				for (ULONG_PTR Count = OldSize - NewMaxSize; Count != 0; --Count)
				{
					(*Data).~TYPE();
					++Data;
				}
			}
		}

		// Adjust buffer.  Note that there's no need to check for error
		// since if it happens, nOldSize == nNewMaxSize will be true.)
		NTSTATUS Status = SetSizeInternal(NewMaxSize);

		if (OldSize < NewMaxSize)
		{
			if (m_Data != nullptr)
			{
				// Adding elements. Call ctor.

				TYPE *Data = GetData() + OldSize;

				for (ULONG_PTR Count = NewMaxSize - OldSize; Count != 0; --Count)
				{
					new (Data)TYPE;
					++Data;
				}
			}
		}

		return Status;
	}

	NTSTATUS Add(CONST_TYPE_REF Value)
	{
		TYPE*       Data;
		NTSTATUS    Status;
		ULONG_PTR   NewSize;

		//NewSize = _InterlockedIncrementPtr(&m_Size);
		NewSize = ++m_Size;

		Status = SetSizeInternal(NewSize);
		if (!NT_SUCCESS(Status))
		{
			//_InterlockedDecrementPtr(&m_Size);
			--m_Size;
			return Status;
		}

		Data = &m_Data[NewSize - 1];

		// Construct the new element
		new (Data)TYPE;

		// Assign
		*Data = Value;

		return STATUS_SUCCESS;
	}

	NTSTATUS Insert(ULONG_PTR Index, CONST_TYPE_REF Value)
	{
		TYPE*    Data;
		NTSTATUS Status;

		// Validate index
		if (Index > m_Size)
		{
			return STATUS_INVALID_PARAMETER;
		}

		// Prepare the buffer
		Status = SetSizeInternal(m_Size + 1);
		if (!NT_SUCCESS(Status))
			return Status;

		Data = GetData() + Index;

		// Shift the array
		RtlMoveMemory(Data + 1, Data, sizeof(*Data) * (m_Size - Index));

		// Construct the new element
		new (Data)TYPE;

		// Set the value and increase the size
		*Data = Value;
		++m_Size;

		return STATUS_SUCCESS;
	}

	NTSTATUS SetAt(ULONG_PTR Index, CONST_TYPE_REF Value)
	{
		// Validate arguments
		if (Index < 0 || Index >= m_Size)
			return STATUS_INVALID_PARAMETER;

		m_Data[Index] = Value;
		return STATUS_SUCCESS;
	}

	TYPE_REF GetLast() const
	{
		return m_Data[m_Size - 1];
	}

	TYPE_REF GetAt(ULONG_PTR Index) const
	{
		return m_Data[Index];
	}

	TYPE* GetAtPtr(ULONG_PTR Index) const
	{
		return m_Data == nullptr ? nullptr : Index > m_Size ? nullptr : &m_Data[Index];
	}

	ULONG_PTR GetSize() const
	{
		return m_Size;
	}

	TYPE* GetData() const
	{
		return m_Data;
	}

	BOOL Contains(CONST_TYPE_REF Value)
	{
		return (kInvalidIndex != IndexOf(Value));
	}

	ULONG_PTR IndexOf(CONST_TYPE_REF Value)
	{
		return IndexOf(Value, 0, m_Size);
	}

	ULONG_PTR IndexOf(CONST_TYPE_REF Value, ULONG_PTR Start)
	{
		return IndexOf(Value, Start, m_Size - Start);
	}

	ULONG_PTR IndexOf(CONST_TYPE_REF Value, ULONG_PTR Start, ULONG_PTR NumberOfElements)
	{
		TYPE *Data;

		// Validate arguments
		if (Start >= m_Size || Start + NumberOfElements > m_Size)
			return kInvalidIndex;

		Data = GetData() + Start;

		// Search
		for (ULONG_PTR Count = NumberOfElements - Start; Count != 0; ++Data, --Count)
		{
			if (*Data == Value)
				return Data - GetData();
		}

		// Not found
		return kInvalidIndex;
	}

	ULONG_PTR LastIndexOf(CONST_TYPE_REF Value)
	{
		return LastIndexOf(Value, m_Size - 1, m_Size);
	}

	ULONG_PTR LastIndexOf(CONST_TYPE_REF Value, ULONG_PTR Index)
	{
		return LastIndexOf(Value, Index, Index + 1);
	}

	ULONG_PTR LastIndexOf(CONST_TYPE_REF Value, ULONG_PTR End, ULONG_PTR NumberOfElements)
	{
		// Validate arguments
		if (End < 0 || End >= m_Size || End < NumberOfElements)
		{
			return kInvalidIndex;
		}

		// Search
		TYPE *Data = GetData() + End;

		for (ULONG_PTR Count = End - NumberOfElements; Count != 0; --Data, --Count)
		{
			if (*Data == Value)
				return Data - GetData();
		}

		// Not found
		return kInvalidIndex;
	}

	NTSTATUS Remove(ULONG_PTR Index)
	{
		TYPE *Data;

		if (Index >= m_Size)
			return STATUS_INVALID_PARAMETER;

		// Destruct the element to be removed
		Data = GetData() + Index;
		(*Data).~TYPE();

		// Compact the array and decrease the size
		RtlMoveMemory(Data, Data + 1, sizeof(*Data) * (m_Size - (Index + 1)));
		--m_Size;

		return STATUS_SUCCESS;
	}

	void RemoveAll()
	{
		SetSize(0);
	}

	void Reset()
	{
		SetSize(0);
		//        m_Size = 0;
	}

	TYPE* begin() const
	{
		return GetData();
	}

	TYPE* end() const
	{
		return GetData() + GetSize();
	}

protected:
	NTSTATUS SetSizeInternal(ULONG_PTR NewMaxSize)  // This version doesn't call ctor or dtor.
	{
		if ((NewMaxSize > ULONG_PTR_MAX / sizeof(TYPE)))
			return STATUS_INVALID_PARAMETER;

		if (NewMaxSize == 0)
		{
			// Shrink to 0 size & cleanup
			HeapFree(GetProcessHeap(), 0, m_Data);
			m_Data = nullptr;

			m_MaxSize = 0;
			m_Size = 0;
		}
		else if (m_Data == nullptr || NewMaxSize > m_MaxSize)
		{
			// Grow array
			ULONG_PTR GrowBy = (m_MaxSize == 0) ? 16 : m_MaxSize;

			// Limit nGrowBy to keep m_nMaxSize less than INT_MAX
			if ((ULONG64)m_MaxSize + GrowBy > ULONG_PTR_MAX)
				GrowBy = ULONG_PTR_MAX - m_MaxSize;

			NewMaxSize = MAX(NewMaxSize, m_MaxSize + GrowBy);

			// Verify that (nNewMaxSize * sizeof(TYPE)) is not greater than UINT_MAX or the realloc will overrun
			//            if (sizeof(TYPE) > ULONG_MAX / NewMaxSize)
			//                return STATUS_INVALID_PARAMETER;

			TYPE* DataNew = (TYPE *)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, m_Data, NewMaxSize * sizeof(*DataNew));

			if (DataNew == nullptr)
				return STATUS_NO_MEMORY;

			m_Data = DataNew;
			m_MaxSize = NewMaxSize;
		}

		return STATUS_SUCCESS;
	}
};




FORCEINLINE ULONG_PTR GetOpCodeSize(PVOID Buffer)
{
#if ML_AMD64
	return GetOpCodeSize64(Buffer);
#elif ML_X86
	return GetOpCodeSize32(Buffer);
#endif
}



#define MP_INLINE ForceInline
#define MP_CALL FASTCALL


namespace Mp
{

#if !defined(_AMD64_)
#define TRAMPOLINE_SIZE     0x40
#else
#define TRAMPOLINE_SIZE     0x100
#endif

namespace PatchMemoryTypes
{
enum
{
	MemoryPatch,
	FunctionPatch,
};
};

enum
{
	VirtualAddress = 0x00000001,

	// patch
	BackupData = 0x00000002,
	DataIsBuffer = 0x00000004,

	// function
	DoNotDisassemble = 0x00000002,
	NakedTrampoline = 0x00000004,
	KeepRawTrampoline = 0x00000008,
	ExecuteTrampoline = 0x00000010,

	OpMask = 0xF0000000,
	OpJump = 0x00000000,
	OpCall = 0x10000000,
	OpPush = 0x20000000,
	OpJRax = 0x30000000,
	OpJRcx = 0x40000000,
	OpJRdx = 0x50000000,
	OpJRbx = 0x60000000,
	OpJRbp = 0x70000000,
	OpJRsi = 0x80000000,
	OpJRdi = 0x90000000,
	OpJR10 = 0xA0000000,
};

typedef struct
{

#if !defined(_AMD64_)

	ULONG_PTR EFlags;
	ULONG_PTR Rdi;
	ULONG_PTR Rsi;
	ULONG_PTR Rbp;
	ULONG_PTR Rbx;
	ULONG_PTR Rdx;
	ULONG_PTR Rcx;
	ULONG_PTR Rax;
	ULONG_PTR Rsp;

#else

	ULONG_PTR EFlags;
	ULONG_PTR Rax;
	ULONG_PTR Rcx;
	ULONG_PTR Rdx;
	ULONG_PTR Rbx;
	//ULONG_PTR Rsp;
	ULONG_PTR Rbp;
	ULONG_PTR Rsi;
	ULONG_PTR Rdi;

	ULONG_PTR R8;
	ULONG_PTR R9;
	ULONG_PTR R10;
	ULONG_PTR R11;
	ULONG_PTR R12;
	ULONG_PTR R13;
	ULONG_PTR R14;
	ULONG_PTR R15;

#endif // arch

	ULONG_PTR ReturnAddress;

#if !defined(_AMD64_)

	PVOID GetArgument(LONG_PTR Index)
	{
		return *(PVOID *)(this->Rsp + (Index + 1) * sizeof(this->Rsp));
	}

	template<class T>
	VOID SetArgument(LONG_PTR Index, T Value)
	{
		*(T *)(this->Rsp + (Index + 1) * sizeof(this->Rsp)) = Value;;
	}

#endif

} TRAMPOLINE_NAKED_CONTEXT, *PTRAMPOLINE_NAKED_CONTEXT;

typedef struct
{
	ULONG_PTR PatchType;

	union
	{
		struct
		{
			union
			{
				ULONG Flags;
				struct
				{
					BOOLEAN VirtualAddress : 1;
					BOOLEAN BackupData : 1;
					BOOLEAN DataIsBuffer : 1;
				};
			} Options;

			ULONG64     Data;
			ULONG_PTR   Size;
			ULONG_PTR   Address;
			ULONG64     Backup;

		} Memory;

		struct
		{
			union
			{
				ULONG Flags;
				struct
				{
					BOOLEAN VirtualAddress : 1;
					BOOLEAN DoNotDisassemble : 1;
					BOOLEAN NakedTrampoline : 1;
					BOOLEAN KeepRawTrampoline : 1;
					BOOLEAN ExecuteTrampoline : 1;
				};
			} Options;

			ULONG_PTR   HookOp;
			ULONG_PTR   Source;
			PVOID       Target;
			PVOID*      Trampoline;
			ULONG_PTR   NopBytes;

		} Function;
	};

} PATCH_MEMORY_DATA, *PPATCH_MEMORY_DATA;

typedef struct _TRAMPOLINE_DATA
{
	BYTE                Trampoline[TRAMPOLINE_SIZE];
	BYTE                OriginalCode[TRAMPOLINE_SIZE];
	ULONG               TrampolineSize;
	ULONG               OriginSize;
	PVOID               JumpBackAddress;
	PATCH_MEMORY_DATA   PatchData;

} TRAMPOLINE_DATA, *PTRAMPOLINE_DATA;



template<class TYPE>
MP_INLINE PVOID __AnyToPtr__(const TYPE &Val)
{
	union
	{
		TYPE Val;
		PVOID Ptr;
	} u;

	u.Ptr = nullptr;
	u.Val = Val;

	return u.Ptr;
}

/************************************************************************
MemoryPatch
************************************************************************/

MP_INLINE PATCH_MEMORY_DATA MemoryPatch(ULONG64 Data, ULONG_PTR Size, ULONG_PTR Address, ULONG Flags)
{
	PATCH_MEMORY_DATA PatchData;

	PatchData.PatchType = PatchMemoryTypes::MemoryPatch;
	PatchData.Memory.Options.Flags = Flags;

	PatchData.Memory.Data = Data;
	PatchData.Memory.Size = Size;
	PatchData.Memory.Address = Address;

	PatchData.Memory.Backup = 0;

	return PatchData;
}

MP_INLINE PATCH_MEMORY_DATA MemoryPatchRva(ULONG64 Data, ULONG_PTR Size, ULONG_PTR RVA, ULONG Flags = 0)
{
	return MemoryPatch((ULONG64)Data, Size, (ULONG_PTR)RVA, Flags);
}

MP_INLINE PATCH_MEMORY_DATA MemoryPatchRva(PVOID Data, ULONG_PTR Size, ULONG_PTR RVA, ULONG Flags = 0)
{
	return MemoryPatch((ULONG64)Data, Size, (ULONG_PTR)RVA, Flags | DataIsBuffer);
}

template<class VA_TYPE>
MP_INLINE PATCH_MEMORY_DATA MemoryPatchVa(ULONG64 Data, ULONG_PTR Size, VA_TYPE Address, ULONG Flags = 0)
{
	return MemoryPatch((ULONG64)Data, Size, (ULONG_PTR)__AnyToPtr__(Address), Flags | VirtualAddress);
}

template<class VA_TYPE>
MP_INLINE PATCH_MEMORY_DATA MemoryPatchVa(PVOID Data, ULONG_PTR Size, VA_TYPE Address, ULONG Flags = 0)
{
	return MemoryPatchVa((ULONG64)Data, Size, Address, Flags | VirtualAddress | DataIsBuffer);
}

/************************************************************************
FunctionPatch
************************************************************************/

MP_INLINE PATCH_MEMORY_DATA FunctionPatch(PVOID Source, PVOID Target, PVOID Trampoline, ULONG Flags)
{
	PATCH_MEMORY_DATA PatchData;

	PatchData.PatchType = PatchMemoryTypes::FunctionPatch;
	PatchData.Function.Options.Flags = Flags;
	PatchData.Function.HookOp = Flags & OpMask;

	PatchData.Function.Source = (ULONG_PTR)Source;
	PatchData.Function.Target = Target;
	PatchData.Function.Trampoline = (PVOID *)Trampoline;
	PatchData.Function.NopBytes = 0;

	return PatchData;
}

template<class SOURCE_TYPE, class TARGET_TYPE>
MP_INLINE PATCH_MEMORY_DATA FunctionJumpVa(SOURCE_TYPE Source, TARGET_TYPE Target, PVOID Trampoline = nullptr, ULONG Flags = OpJump)
{
	return FunctionPatch(__AnyToPtr__(Source), __AnyToPtr__((SOURCE_TYPE)Target), Trampoline, Flags | VirtualAddress);
}

template<class TARGET_TYPE>
MP_INLINE PATCH_MEMORY_DATA FunctionJumpRva(ULONG_PTR SourceRva, TARGET_TYPE Target, PVOID Trampoline = nullptr, ULONG Flags = OpJump)
{
	return FunctionPatch((PVOID)SourceRva, __AnyToPtr__(Target), Trampoline, Flags);
}

template<class SOURCE_TYPE, class TARGET_TYPE>
MP_INLINE PATCH_MEMORY_DATA FunctionCallVa(SOURCE_TYPE Source, TARGET_TYPE Target, PVOID Trampoline = nullptr, ULONG Flags = OpCall)
{
	return FunctionPatch(__AnyToPtr__(Source), __AnyToPtr__((SOURCE_TYPE)Target), Trampoline, Flags | VirtualAddress);
}

template<class TARGET_TYPE>
MP_INLINE PATCH_MEMORY_DATA FunctionCallRva(ULONG_PTR SourceRva, TARGET_TYPE Target, PVOID Trampoline = nullptr, ULONG Flags = OpCall)
{
	return FunctionPatch((PVOID)SourceRva, __AnyToPtr__(Target), Trampoline, Flags);
}


NTSTATUS
MP_CALL
PatchMemory(
	PPATCH_MEMORY_DATA  PatchData,
	ULONG_PTR           PatchCount,
	PVOID               BaseAddress = nullptr
);

NTSTATUS
MP_CALL
RestoreMemory(
	PTRAMPOLINE_DATA TrampolineData
);

template<class TRAMPOLINE_PTR> inline NTSTATUS RestoreMemory(TRAMPOLINE_PTR& Trampoline)
{
	NTSTATUS Status;

	if (Trampoline == nullptr)
		return STATUS_SUCCESS;

	Status = RestoreMemory(FIELD_BASE(Trampoline, TRAMPOLINE_DATA, Trampoline));
	if (NT_SUCCESS(Status))
		Trampoline = nullptr;

	return Status;
}

inline NTSTATUS RestoreMemory(PPATCH_MEMORY_DATA PatchData, ULONG_PTR PatchCount)
{
	FOR_EACH(PatchData, PatchData, PatchCount)
	{
		switch (PatchData->PatchType)
		{
		case PatchMemoryTypes::MemoryPatch:
			break;

		case PatchMemoryTypes::FunctionPatch:
			if (PatchData->Function.Trampoline != nullptr)
				RestoreMemory(*PatchData->Function.Trampoline);
			break;
		}
	}

	return STATUS_SUCCESS;
}

};




#define INJECT_THREAD_SUSPENDED     (1u << 0)
#define INJECT_ISSUE_REMOTE_BREAKIN (1u << 1)
#define INJECT_POINT_TO_SHELL_CODE  (1u << 2)
#define INJECT_ISSUE_BREAKIN        (1u << 3)
#define INJECT_PREALLOC_BUFFER      (1u << 4)
#define INJECT_DONT_CHANGE_IP       (1u << 5)
#define INJECT_ISSUE_BREAKIN_ONLY   (1u << 6)

NTSTATUS
InjectDllToRemoteProcess(
	HANDLE          ProcessHandle,
	HANDLE          ThreadHandle,
	PUNICODE_STRING DllFullPath,
	ULONG_PTR       Flags,
	PVOID*          DEF_VAL(InjectBuffer, NULL),
	ULONG_PTR       DEF_VAL(Timeout, 500)
);



inline PLDR_MODULE GetKernel32Ldr()
{
	LDR_MODULE *Ldr, *FirstLdr;

	Ldr = FIELD_BASE(Nt_CurrentPeb()->Ldr->InInitializationOrderModuleList.Flink, LDR_MODULE, InInitializationOrderLinks);
	FirstLdr = Ldr;

	do
	{
		Ldr = FIELD_BASE(Ldr->InInitializationOrderLinks.Flink, LDR_MODULE, InInitializationOrderLinks);
		if (Ldr->BaseDllName.Buffer == NULL)
			continue;

		if (CHAR_UPPER4W(*(PULONG64)(Ldr->BaseDllName.Buffer + 0)) != TAG4W('KERN') ||
			CHAR_UPPER4W(*(PULONG64)(Ldr->BaseDllName.Buffer + 4)) != CHAR_UPPER4W(TAG4W('EL32')) ||
			Ldr->BaseDllName.Buffer[8] != '.')
		{
			continue;
		}

		return Ldr;

	} while (FirstLdr != Ldr);

	return NULL;
}

inline PVOID GetKernel32Handle()
{
	return GetKernel32Ldr()->DllBase;
}


ForceInline PLDR_MODULE GetNtdllLdrModule()
{
	return FIELD_BASE(Nt_CurrentPeb()->Ldr->InInitializationOrderModuleList.Flink, LDR_MODULE, InInitializationOrderLinks);
}

ForceInline PVOID GetNtdllHandle()
{
	return GetNtdllLdrModule()->DllBase;
}



ULONG_PTR
IATLookupRoutineRVAByEntry(
	PVOID ImageBase,
	PVOID RoutineEntry
);

PVOID
IATLookupRoutineByEntryNoFix(
	PVOID ImageBase,
	PVOID RoutineEntry
);

ULONG_PTR
IATLookupRoutineRVAByHashNoFix(
	PVOID       ImageBase,
	ULONG_PTR   Hash
);

PVOID
EATLookupRoutineByHashNoFix(
	PVOID       ImageBase,
	ULONG_PTR   Hash
);

PSTR
EATLookupNameByHashNoFix(
	PVOID       ImageBase,
	ULONG_PTR   Hash
);

PSTR
EATLookupNameByNameNoFix(
	PVOID   ImageBase,
	PSTR    Name
);

PVOID
FASTCALL
EATLookupRoutineByHashPNoFix(
	PVOID       ImageBase,
	ULONG_PTR   RoutineEntry
);

/************************************************************************
x64 ver
************************************************************************/
PVOID
EATLookupRoutineByHashNoFix64(
	PVOID       ImageBase,
	ULONG_PTR   Hash
);

PVOID
EATLookupRoutineByHashPNoFix64(
	PVOID       ImageBase,
	ULONG_PTR   RoutineEntry
);

NTSTATUS
AllocVirtualMemoryEx(
	HANDLE      ProcessHandle,
	PVOID*      BaseAddress,
	ULONG_PTR   Size,
	ULONG       Protect = PAGE_EXECUTE_READWRITE,
	ULONG       AllocationType = MEM_RESERVE | MEM_COMMIT
);


NTSTATUS
ReadMemory(
	HANDLE      ProcessHandle,
	PVOID       BaseAddress,
	PVOID       Buffer,
	ULONG_PTR   Size,
	PULONG_PTR  BytesRead = nullptr
);

NTSTATUS
WriteMemory(
	HANDLE      ProcessHandle,
	PVOID       BaseAddress,
	PVOID       Buffer,
	ULONG_PTR   Size,
	PULONG_PTR  BytesWritten = nullptr
);
