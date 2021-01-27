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


#if !defined(USE_NT_VER)
#define USE_NT_VER 1
#endif


#define ML_DISABLE_THIRD_LIB_UCL 1
#define ML_USER_MODE  1

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_SWPRINTFS
#define _CRT_NONSTDC_NO_WARNINGS

#ifndef _WINSOCKAPI_
#include <ws2spi.h>
#endif

#include <intsafe.h>


#if defined(__INTEL_COMPILER)
#define ML_COMPILER_INTEL __INTEL_COMPILER
#define ML_COMPILER_MSC 1700
#define MY_COMPILER_MSC ML_COMPILER_MSC

#elif defined(_MSC_VER)
#define ML_COMPILER_MSC _MSC_VER
#define MY_COMPILER_MSC ML_COMPILER_MSC

#endif // compiler

#if ML_COMPILER_MSC >= 1500
#define MY_MSC_1500 1
#define SUPPORT_VA_ARGS_MACRO 1
#endif  // MY_COMPILER_MSC >= 1500

#if ML_COMPILER_MSC >= 1700
#define ML_SUPPORT_LAMBDA 1
#endif // ML_SUPPORT_LAMBDA

#if defined(UNICODE) || defined(_UNICODE)
#define MY_UNICODE_ENABLE 1
#else
#define MY_UNICODE_ENABLE 0
#endif // set unicode flag

#if !defined(WINCE)
#define MY_OS_WIN32 1
#elif defined(_WIN32_WCE)
#define MY_OS_WINCE 1
#endif // MY_OS


#if MY_OS_WIN32

#if !defined(_WIN32_WINNT)
#define _WIN32_WINNT 0x0503
#endif /* _WIN32_WINNT */

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

#elif defined(_M_ARM)

define ML_ARM 1

#elif defined(_ARM64_)

#define ML_ARM64 1

#elif defined(_IA64_)

#define ML_IA64 1

#elif defined(_M_ALPHA)

#define ML_ALPHA 1

#elif defined(_M_MIPS)

#define ML_MIPS 1

#elif defined(_M_PPC)

#define ML_PPC 1

#endif
#endif // MY_OS_WIN32


/************************************************************************/
/*  compiler settings                                                   */
/************************************************************************/
#pragma warning(disable:4530 4100 4101 4731 4740 4146 4214 4201 4750 4457 4458 4477 4459 4456)

#if MY_MSC_1500
#pragma warning(suppress:4530)
#endif  // MY_MSC_1500

#if defined(ML_COMPILER_INTEL)
#pragma warning(disable:7 157 170 181 186 869 1899 2557)
#endif

#if MY_COMPILER_MSC < 1300
#define for if(1) for
#endif /* fix vc6 for bug */

#ifndef CPP_DEFINED
#if defined(__cplusplus)
#define CPP_DEFINED 1
#else
#define CPP_DEFINED 0
#endif
#endif // CPP_DEFINED

#if defined(__cplusplus_cli)
#define CPP_CLI_DEFINED 1
#else
#define CPP_CLI_DEFINED 0
#endif // cpp cli

#if !ML_DISABLE_THIRD_LIB

#define INCLUDE_LIB(_lib) comment(lib, #_lib)

#else

#define INCLUDE_LIB(_lib) once

#endif // ML_DISABLE_THIRD_LIB

#define USE_STACK_FRAME_POINTER   "y", off
#define UNUSE_STACK_FRAME_POINTER "y", on


#if !defined(typedef_s)
#define typedef_s(s, d) __if_not_exists(d) { typedef s d; }

#if SUPPORT_VA_ARGS_MACRO
#define typedef_s2(s, ...) __if_not_exists(d) { typedef s __VA_ARGS__; }
#endif  // SUPPORT_VA_ARGS_MACRO

#endif /* typedef_s */

#ifndef EXTCPP
#if CPP_DEFINED
#define EXTCPP extern "C++"
#else
#define EXTCPP
#endif
#endif /* EXTCPP */

#ifndef NOVTABLE
#define NOVTABLE __declspec(novtable)
#endif /* NOVTABLE */

#if ML_AMD64

#ifndef ASM
#define ASM
#endif /* ASM */

#ifndef NAKED
#define NAKED
#endif /* ASM */

#if !defined(INLINE_ASM)
#if defined(ML_COMPILER_INTEL)
#define INLINE_ASM __asm
#else
#define INLINE_ASM
#endif
#endif /* INLINE_ASM */

#else // x86

#ifndef ASM
#define ASM __declspec(naked)
#endif /* ASM */

#ifndef NAKED
#define NAKED __declspec(naked)
#endif /* ASM */

#if !defined(INLINE_ASM)
#define INLINE_ASM __asm
#endif /* INLINE_ASM */

#endif // x64 x86

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

#if !defined(MY_CRT_IMPORT)
#if defined(_DLL)
#define MY_IMPORT MY_DLL_IMPORT
#else
#define MY_IMPORT
#endif
#endif /* MY_CRT_IMPORT */

#define MY_LIB_NAME MY_LIB

#if  CPP_DEFINED
#define _ML_C_HEAD_     extern "C" {
#define _ML_C_TAIL_     }
#define _ML_CPP_HEAD_     extern "C++" {
#define _ML_CPP_TAIL_     }
#define ML_NAMESPACE_BEGIN(name) namespace name {
#define ML_NAMESPACE_END }
#define ML_NAMESPACE_END_(name) }
#define DEFAULT_VALUE(type, var, value) type var = value
#else
#define _ML_C_HEAD_
#define _ML_C_TAIL_
#define _ML_CPP_HEAD_
#define _ML_CPP_TAIL_
#define ML_NAMESPACE_BEGIN(name)
#define ML_NAMESPACE_END
#define DEFAULT_VALUE(type, var, value) type var
#endif /* _ML_C_HEAD_ */

#define ML_NAMESPACE ML_NAMESPACE_BEGIN(ml)

#define MY_NAMESPACE_BEGIN  ML_NAMESPACE_BEGIN
#define MY_NAMESPACE_END    ML_NAMESPACE_END

#ifndef ForceInline
#if (MY_COMPILER_MSC >= 1200)
#define ForceInline __forceinline
#else
#define ForceInline __inline
#endif
#endif /* ForceInline */

#ifndef NoInline
#if (MY_COMPILER_MSC >= 1300)
#define NoInline __declspec(noinline)
#else
#define NoInline
#endif
#endif /* NoInline */

#if (defined(_M_IX86) || defined(_M_IA64) || defined(_M_AMD64)) && !defined(MIDL_PASS)
#define DECL_IMPORT __declspec(dllimport)
#define DECL_EXPORT __declspec(dllexport)
#else
#define DECL_IMPORT
#define DECL_EXPORT
#endif

#if !defined(DISABLE_SEH)

#define SEH_TRY     __try
#define SEH_FINALLY __finally
#define SEH_LEAVE   __leave

#if SUPPORT_VA_ARGS_MACRO
#define SEH_EXCEPT(...)  __except(__VA_ARGS__)
#else
#define SEH_EXCEPT  __except
#endif  // SUPPORT_VA_ARGS_MACRO

#else

#define SEH_TRY
#define SEH_FINALLY
#define SEH_LEAVE

#if SUPPORT_VA_ARGS_MACRO
#define SEH_EXCEPT(...) for (int __c_false = 0; __c_false; )
#else
#define SEH_EXCEPT()
#endif  // SUPPORT_VA_ARGS_MACRO

#endif // DISABLE_SEH

#ifndef EXTC
#if CPP_DEFINED
#define EXTC extern "C"
#define EXTC_IMPORT EXTC __declspec(dllimport)
#define EXTC_EXPORT EXTC DECL_EXPORT
#else
#define EXTC
#define EXTC_IMPORT __declspec(dllimport)
#endif
#endif /* EXTC */

#ifndef DECL_NORETURN
#if (MY_COMPILER_MSC >= 1200) && !defined(MIDL_PASS)
#define DECL_NORETURN   __declspec(noreturn)
#else
#define DECL_NORETURN
#endif
#endif

#ifndef DECL_NOTHROW
#if (MY_COMPILER_MSC >= 1200) && !defined(MIDL_PASS)
#define DECL_NOTHROW   __declspec(nothrow)
#else
#define DECL_NOTHROW
#endif
#endif

#ifndef DECL_ALIGN
#if (MY_COMPILER_MSC >= 1300) && !defined(MIDL_PASS)
#define DECL_ALIGN(x)   __declspec(align(x))
#else
#define DECL_ALIGN(x)
#endif
#endif

#ifndef SYSTEM_CACHE_ALIGNMENT_SIZE
#if defined(_AMD64_) || defined(_X86_)
#define SYSTEM_CACHE_ALIGNMENT_SIZE 64
#else
#define SYSTEM_CACHE_ALIGNMENT_SIZE 128
#endif
#endif

#ifndef DECL_CACHEALIGN
#define DECL_CACHEALIGN DECL_ALIGN(SYSTEM_CACHE_ALIGNMENT_SIZE)
#endif

#ifndef DECL_UUID
#if (MY_COMPILER_MSC >= 1100) && CPP_DEFINED
#define DECL_UUID(x)    __declspec(uuid(x))
#else
#define DECL_UUID(x)
#endif
#endif

#ifndef DECL_NOVTABLE
#if (MY_COMPILER_MSC >= 1100) && CPP_DEFINED
#define DECL_NOVTABLE   __declspec(novtable)
#else
#define DECL_NOVTABLE
#endif
#endif

#ifndef DECL_SELECTANY
#if (MY_COMPILER_MSC >= 1100)
#define DECL_SELECTANY  __declspec(selectany)
#else
#define DECL_SELECTANY
#endif
#endif

#ifndef NOP_FUNCTION
#if (MY_COMPILER_MSC >= 1210)
#define NOP_FUNCTION __noop
#else
#define NOP_FUNCTION (void)0
#endif
#endif

#ifndef DECL_ADDRSAFE
#if (MY_COMPILER_MSC >= 1200) && (defined(_M_ALPHA) || defined(_M_AXP64))
#define DECL_ADDRSAFE  __declspec(address_safe)
#else
#define DECL_ADDRSAFE
#endif
#endif

#ifndef DECL_DEPRECATED
#if (MY_COMPILER_MSC >= 1300) && !defined(MIDL_PASS)
#define DECL_DEPRECATED   __declspec(deprecated)
#define DEPRECATE_SUPPORTED
#else
#define DECL_DEPRECATED
#undef  DEPRECATE_SUPPORTED
#endif
#endif

#if !defined(DECL_THREAD) && (defined(MY_COMPILER_MSC) || defined(__INTEL_COMPILER))
#define DECL_THREAD __declspec(thread)
#endif // DECL_THREAD

#ifndef UNUSED_VARIABLE
#define UNUSED_VARIABLE(Var) __if_exists(Var) { (Var); }
#endif // UNUSED_VARIABLE

#ifndef True
#define True 1l
#endif /* True */

#ifndef False
#define False 0l
#endif /* False */

#ifndef NULL
#define NULL 0l
#endif /* NULL */

#ifdef CDECL
#undef CDECL
#endif /* CDECL */
#define CDECL __cdecl

#ifdef STDCALL
#undef STDCALL
#endif /* STDCALL */
#define STDCALL __stdcall

#ifdef FASTCALL
#undef FASTCALL
#endif /* STDCALL */

#if ML_AMD64 || CPP_CLI_DEFINED
#define FASTCALL
#else
#define FASTCALL __fastcall
#endif

#ifdef THISCALL
#undef THISCALL
#endif

#ifdef ML_COMPILER_MSC
#define THISCALL __thiscall
#else
#define THISCALL
#endif

#ifndef OPTIONAL
#define OPTIONAL
#endif /* OPTIONAL */

#ifndef RESERVED
#define RESERVED
#endif /* RESERVED */

#if ML_USER_MODE

#if !defined(ARGUMENT_PRESENT)
#define ARGUMENT_PRESENT(ArgumentPointer) ((CHAR *)((ULONG_PTR)(ArgumentPointer)) != (CHAR *)(NULL) )
#define ARGUMENT_NOT_PRESENT(ArgumentPointer) (!ARGUMENT_PRESENT(ArgumentPointer))
#endif

#endif


#if ML_COMPILER_MSC

#define PROPERTY(type, name) \
    __declspec(property(put = __property_set_##name, get = __property_get_##name ) ) type name; \
    typedef type __property_type_##name

#define READONLY_PROPERTY(type, name) \
    __declspec(property(get = __property_get_##name) ) type name; \
    typedef type __property_type_##name; \
    GET(name)

#define WRITEONLY_PROPERTY(type, name) \
    __declspec(property(put = __property_set_##name) ) type name; \
    typedef type property__tmp_type_##name; \
    SET(name)

#define GET(name) __property_type_##name __property_get_##name()
#define SET(name) void __property_set_##name(const __property_type_##name & value)

#endif // msc


#if !defined(INTEL_STATIC)
#if defined(__INTEL_COMPILER)
#define INTEL_STATIC static
#else
#define INTEL_STATIC
#endif
#endif /* INTEL_STATIC */

#if !defined(MY_W64)
#if !defined(__midl) && (defined(_X86_) || defined(_M_IX86)) && (_MSC_VER >= 1300)
#define MY_W64 __w64
#else
#define MY_W64
#endif
#endif



#if !defined(_WCHAR_T_DEFINED)
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif

#if !CPP_DEFINED

#define nullptr NULL

#endif // cpp

typedef float               Float, *PFloat, *LPFloat;
typedef double              Double, *PDouble, *LPDouble;

typedef char                Char, s8, Int8, *ps8, *PInt8, *PChar;
typedef const char         *PCChar;
typedef wchar_t             WChar, *PWChar;
typedef const wchar_t      *PCWChar;
typedef unsigned char       u8, UChar, UInt8, UInt24, Byte, *pu8, *PByte, *PUInt8, *PUChar;
typedef short               Short, s16, Int16, *ps16, *PInt16, *PShort, *LPShort;
typedef unsigned short      UShort, Word, u16, UInt16, *pu16, *PUInt16, *PWord, *LPWord, *PUShort, *LPUShort;
typedef long                Long, *PLong, *LPLong;
typedef long long           Long64, LongLong, *PLong64, *PLongLong;
typedef unsigned long       Dword, ULong, *PULong, *LPULong, *PDword, *LPDword;
typedef unsigned long long  ULong64, ULongLong, *PULong64, *PULongLong;
typedef void                Void, *PVoid, *LPVoid;
typedef const void         *LPCVoid, *PCVoid;

typedef int                 Bool, Int, s32, Int32, *PInt, *ps32, *PInt32;
typedef unsigned int        UInt, u32, UInt32, *PUInt, *pu32, *PUInt32;

typedef __int64             s64, Int64, *ps64, *PInt64;
typedef unsigned __int64    u64, UInt64, *pu64, *PUInt64;


#pragma pack(push, 1)

typedef struct
{
	UShort Offset : 12;
	UShort Type : 4;

} IMAGE_RELOCATION_ADDRESS_ENTRY, *PIMAGE_RELOCATION_ADDRESS_ENTRY;

typedef struct
{
	ULong   VirtualAddress;
	ULong   SizeOfBlock;
	IMAGE_RELOCATION_ADDRESS_ENTRY TypeOffset[1];

} *PIMAGE_BASE_RELOCATION2;

#pragma pack(pop)


#if defined(MY_X64)
typedef __int64 Int_Ptr, *PInt_Ptr;
typedef unsigned __int64 UInt_Ptr, *PUInt_Ptr;

typedef __int64 Long_Ptr, *PLong_Ptr, LongPtr, *PLongPtr;
typedef unsigned __int64 ULong_Ptr, *PULong_Ptr, ULongPtr, *PULongPtr;
#else
typedef int MY_W64 Int_Ptr, *PInt_Ptr;
typedef unsigned int MY_W64 UInt_Ptr, *PUInt_Ptr;

typedef long MY_W64 Long_Ptr, *PLong_Ptr, LongPtr, *PLongPtr;
typedef unsigned long MY_W64 ULong_Ptr, *PULong_Ptr, ULongPtr, *PULongPtr;
#endif

typedef ULong_Ptr SizeT, *PSizeT;
typedef Long_Ptr  SSizeT, *PSSizeT;

#define MAX_SHORT  (Short) (0x7FFF)
#define MAX_USHORT (UShort)(0xFFFF)
#define MAX_INT    (Int)   (0x7FFFFFFF)
#define MAX_UINT   (UInt)  (0xFFFFFFFF)
#define MAX_INT64  (Int64) (0x7FFFFFFFFFFFFFFF)
#define MAX_UINT64 (UInt64)(0xFFFFFFFFFFFFFFFF)
#define MAX_NTPATH  0x220


__if_not_exists(_PVFV)
{
	typedef void (CDECL *_PVFV)(void);
	typedef int  (CDECL *_PIFV)(void);
	typedef void (CDECL *_PVFI)(int);
}

EXTC _PVFV __xi_a, __xi_z, __xc_a, __xc_z;

inline void my_initterm(_PVFV *pfbegin, _PVFV *pfend)
{
	/*
	* walk the table of function pointers from the bottom up, until
	* the end is encountered.  Do not skip the first entry.  The initial
	* value of pfbegin points to the first valid entry.  Do not try to
	* execute what pfend points to.  Only entries before pfend are valid.
	*/
	while (pfbegin < pfend)
	{
		/*
		* if current table entry is non-NULL, call thru it.
		*/
		if (*pfbegin != NULL)
			(**pfbegin)();
		++pfbegin;
	}
}


#define DR7_LEN_1_BYTE  0
#define DR7_LEN_2_BYTE  1
#define DR7_LEN_4_BYTE  3
#define DR7_LEN_8_BYTE  2
#define DR7_RW_EXECUTE  0
#define DR7_RW_WRITE    1
#define DR7_RW_ACCESS   3

#pragma pack(1)

typedef union
{
	ULong Dr6;
	struct
	{
		Byte B0 : 1;
		Byte B1 : 1;
		Byte B2 : 1;
		Byte B3 : 1;
		Byte Reserve1 : 8;
		Byte Reserve2 : 2;
		Byte BD : 1;
		Byte BS : 1;
		Byte BT : 1;
	};

} DR6_INFO, *PDR6_INFO;

typedef union
{
	ULong Dr7;
	struct
	{
		Byte L0 : 1;    // 0x01
		Byte G0 : 1;    // 0x02
		Byte L1 : 1;    // 0x03
		Byte G1 : 1;    // 0x04
		Byte L2 : 1;    // 0x05
		Byte G2 : 1;    // 0x06
		Byte L3 : 1;    // 0x07
		Byte G3 : 1;    // 0x08
		Byte LE : 1;    // 0x09
		Byte GE : 1;    // 0x0A
		Byte Reserve : 3;    // 0x0B
		Byte GD : 1;    // 0x0E
		Byte Reserve2 : 2;    // 0x0F
		Byte RW0 : 2;    // 0x11
		Byte LEN0 : 2;    // 0x13
		Byte RW1 : 2;    // 0x15
		Byte LEN1 : 2;    // 0x17
		Byte RW2 : 2;    // 0x19
		Byte LEN2 : 2;    // 0x1B
		Byte RW3 : 2;    // 0x1D
		Byte LEN3 : 2;    // 0x1F
	};
} DR7_INFO, *PDR7_INFO;

typedef struct
{
	ULong_Ptr Rdi;
	ULong_Ptr Rsi;
	ULong_Ptr Rbp;
	ULong_Ptr Rsp;
	ULong_Ptr Rbx;
	ULong_Ptr Rdx;
	ULong_Ptr Rcx;
	ULong_Ptr Rax;

} PUSHAD_REGISTER, *PPUSHAD_REGISTER;

typedef union
{
	struct
	{
		ULong LowPart;
		Long  HighPart;
	};
	Long64 QuadPart;
} Large_Integer, *PLarge_Integer, *LPLarge_Integer;

typedef union
{
	struct
	{
		ULong LowPart;
		ULong HighPart;
	};
	ULong64 QuadPart;
} ULarge_Integer, *PULarge_Integer, *LPULarge_Integer;

typedef union
{
	struct
	{
		UInt16 w1;
		UInt16 w2;
		UInt16 w3;
		UInt16 w4;
		UInt16 w5;
		UInt16 w6;
		UInt16 w7;
		Int16  w8;
	};

	struct
	{
		UInt32 d1;
		UInt32 d2;
		UInt32 d3;
		Int32  d4;
	};

	struct
	{
		ULarge_Integer q1;
		Large_Integer  q2;
	};
} MMXRegister;

typedef union
{
	LPVoid  lpBuffer;
	PByte   pbBuffer;
	PUInt16 pBuffer16;
	PUInt32 pBuffer32;
	PUInt64 pBuffer64;
	PLarge_Integer  pLi;
	PULarge_Integer pULi;
} MultiTypePointer;

#pragma pack()

#if !CPP_DEFINED
typedef unsigned char bool, *pbool;
#define inline __inline
#else
typedef bool *pbool;
#endif /* __cplusplus */

#if MY_UNICODE_ENABLE
typedef WChar TChar;
#else
typedef Char TChar;
#endif /* UNICODE */

typedef TChar *PTChar;
typedef const TChar *PCTChar;

#if MY_OS_WIN32 && !ML_AMD64

ForceInline Void FASTCALL DebugBreakPoint()
{
	__debugbreak();
}

ForceInline Int CDECL _purecall2()
{
	DebugBreakPoint();
	return 0;
}

ForceInline int CDECL _purecall()
{
	DebugBreakPoint();
	return 0;
}

#endif

// override global operator new/delete

#if !defined(MYAPI)
#define MYAPI(funcname) funcname
#endif

#if ML_KERNEL_MODE
/*
inline PVOID _KiNewAllocate(HANDLE HeapBase, ULONG Flags, SIZE_T Bytes)
{
ULONG_PTR PoolType = (ULONG_PTR)HeapBase;
return AllocateMemory(Bytes, PoolType);
}

BOOLEAN _KiDelete(HANDLE HeapBase, ULONG Flags, LPVOID Memory)
{
FreeMemory(Memory);
}
*/
#define NEW_ALLOC_API(PoolType, Flags, Bytes) AllocateMemory(Bytes, (POOL_TYPE)(ULONG_PTR)(PoolType))
#define NEW_FREE_API(HeapBase, Flags, Memory) FreeMemory(Memory)

#else // r3

#if USE_CRT_ALLOC
#define MY_OPNEW_ALLOC(n) malloc(n)
#define MY_OPNEW_FREE(p)  free(p)
#else
#define MY_OPNEW_ALLOC(n) HeapAlloc(GetProcessHeap(), 0, n)
#define MY_OPNEW_FREE(p)  HeapFree(GetProcessHeap(), 0, p)
#endif

#if USE_NT_VER
#define NEW_ALLOC_API   RtlAllocateHeap
#define NEW_FREE_API    RtlFreeHeap
#else
#define NEW_ALLOC_API   HeapAlloc
#define NEW_FREE_API    HeapFree
#endif // USE_NT_VER

#endif // r0

#ifndef SAFE_DELETE
#define SAFE_DELETE(p) if (p) \
                           { \
                               delete (p); \
                               (p) = NULL; \
                           }
#endif // SAFE_DELETE

#ifndef SAFE_FREE
#define SAFE_FREE(p) if (p) \
                           { \
                               MY_OPNEW_FREE(p); \
                               (p) = NULL; \
                           }
#endif // SAFE_FREE


// && (defined(_INC_MALLOC) || defined(_WINDOWS_))

#define OVERLOAD_OP_NEW  \
    inline Void* CDECL operator new[](size_t size)\
    {\
        return size ? MY_OPNEW_ALLOC(size) : NULL;\
    }\
    \
    inline Void* CDECL operator new(size_t size)\
    {\
        return (Void *)(new char[size]);\
    }\
    \
    inline Void CDECL operator delete(Void *p)\
    {\
        if (p != NULL) \
            MY_OPNEW_FREE(p);\
    }\
    \
    inline Void CDECL operator delete[](Void *p)\
    {\
        delete(p);\
    }

#define OVERLOAD_CPP_NEW_WITH_HEAP(NewHeap) \
    inline PVoid CDECL operator new[](size_t Size) \
    { \
        return NEW_ALLOC_API((NewHeap), HEAP_ZERO_MEMORY, Size); \
    } \
    inline PVoid CDECL operator new(size_t Size) \
    { \
        return operator new[](Size); \
    } \
    inline Void CDECL operator delete(PVoid pBuffer) \
    { \
        if (pBuffer != NULL) \
            NEW_FREE_API((NewHeap), 0, pBuffer); \
    } \
    inline Void CDECL operator delete[](PVoid pBuffer) \
    { \
        delete pBuffer; \
    }

#define OVERLOAD_CPP_METHOD_NEW_WITH_HEAP(NewHeap) \
    inline PVoid CDECL operator new[](size_t Size) \
    { \
        return NEW_ALLOC_API((NewHeap), 0, Size); \
    } \
    inline PVoid CDECL operator new(size_t Size) \
    { \
        return operator new[](Size); \
    } \
    inline Void CDECL operator delete(PVoid pBuffer) \
    { \
        if (pBuffer != NULL) \
            NEW_FREE_API((NewHeap), 0, pBuffer); \
    } \
    inline Void CDECL operator delete[](PVoid pBuffer) \
    { \
        delete pBuffer; \
    }


#define ML_OVERLOAD_NEW \
            inline PVoid CDECL operator new[](size_t Size) \
            { \
                return AllocateMemoryP(Size); \
            } \
            inline PVoid CDECL operator new(size_t Size) \
            { \
                return operator new[](Size); \
            } \
            inline Void CDECL operator delete(PVoid Buffer) \
            { \
                if (Buffer != NULL) \
                    FreeMemoryP(Buffer); \
            } \
            inline Void CDECL operator delete(PVoid Buffer, UInt_Ptr) \
            { \
                if (Buffer != NULL) \
                    FreeMemoryP(Buffer); \
            } \
            inline Void CDECL operator delete[](PVoid Buffer) \
            { \
                delete Buffer; \
            } \
            inline Void CDECL operator delete[](PVoid Buffer, UInt_Ptr) \
            { \
                delete Buffer; \
            }




#if ML_DEBUG_KERNEL

#if ML_X86

#define DEBUG_BREAK_POINT     DbgBreakPoint

#elif ML_AMD64

#define DEBUG_BREAK_POINT     DbgBreakPoint

#endif // arch

#if SUPPORT_VA_ARGS_MACRO
#define DebugPrint(...)         DbgPrint(__VA_ARGS__)
#else
#define DebugPrint  DbgPrint
#endif

#else

#define DEBUG_BREAK_POINT()

#if SUPPORT_VA_ARGS_MACRO
#define DebugPrint(...)
#else
#define DebugPrint()
#endif

#endif /* ML_DEBUG_KERNEL */

#define BP DEBUG_BREAK_POINT

#define RTL_CONST_STRING(_str, _string) \
            (_str).Length = sizeof(_string) - sizeof((_str).Buffer[0]); \
            (_str).MaximumLength = sizeof(_string); \
            (_str).Buffer = (_string);

enum ECodePage
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

#if !defined(_M_IA64)
#define MEMORY_PAGE_SIZE (4 * 1024)
#else
#define MEMORY_PAGE_SIZE (8 * 1024)
#endif

#define NO_BREAK


#define LOOP_ALWAYS for (;;)
#define LOOP_FOREVER LOOP_ALWAYS
#define LOOP_ONCE   for (Bool __condition_ = True; __condition_; __condition_ = False)

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
#define FLAG_ON(_V, _F)     (!!(BOOL)((_V) & (_F)))
#define FLAG_OFF(_V, _F)     (!(BOOL)FLAG_ON(_V, _F))

#if !defined(TEST_BIT)
#define TEST_BIT(value, bit) ((value) & (1 << bit))
#endif /* TEST_BIT */

#if !defined(TEST_BITS)
#define TEST_BITS(value, bits) (Bool)(!!((value) & (bits)))
#endif /* TEST_BITS */

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

#if !defined(MEMORY_PAGE_ADDRESS)
#define MEMORY_PAGE_ADDRESS(Address) (ROUND_DOWN((ULongPtr)(Address), MEMORY_PAGE_SIZE))
#endif /* MEMORY_PAGE_ADDRESS */

#if defined(DEFAULT_VALUE)
#undef DEFAULT_VALUE
#endif // DEFAULT_VALUE

#if CPP_DEFINED
#define DEFAULT_VALUE(type, var, value) type var = value
#define DEF_VAL(var, value)             var = value
#else
#define DEFAULT_VALUE(type, var, value) type var
#define DEF_VAL(var, value)             var
#endif //CPP_DEFINED

#if defined(__INTEL_COMPILER)
#pragma warning(disable:1479)
#endif

#define MAKE_UNIQUE_NAME2(n) _##n
#define MAKE_UNIQUE_NAME(n) MAKE_UNIQUE_NAME2(n)
#define DUMMY_STRUCT(n) DECL_ALIGN(1) struct { char _[n]; } MAKE_UNIQUE_NAME(__LINE__)
// #define DUMMY_STRUCT(size) DECL_ALIGN(1) struct { Byte __dummy[size]; }


#define MAKE_NAME_PREFIX_(prefix, n) prefix##n
#define MAKE_NAME_PREFIX(prefix, n) MAKE_NAME_PREFIX_(prefix, n)

#define GetStructMemberOffset(type, member_begin, member_end) \
    (PByte)&(*(type*)0).member_end - (PByte)&(*(type*)0).member_begin


template<class FunctionType, class FunctionAddress>
ForceInline
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

#if SUPPORT_VA_ARGS_MACRO

#define CallFuncPtr(RoutineType, RoutineAddr, ...) ((TYPE_OF(RoutineType)*)(RoutineAddr))(__VA_ARGS__)

#endif // SUPPORT_VA_ARGS_MACRO



#define ML_MAX(a, b) ((a) < (b) ? (b) : (a))
#define ML_MIN(a, b) ((a) < (b) ? (a) : (b))

#define MY_MAX ML_MAX
#define MY_MIN ML_MIN


#define CHAR_UPPER(ch) (IN_RANGE('a', (ch), 'z') ? ((ch) & (Byte)0xDF) : ch)
#define CHAR_LOWER(ch) (IN_RANGE('A', (ch), 'Z') ? ((ch) | (Byte)~0xDF) : ch)

#define _CHAR_UPPER4W(ch) (UInt64)((ch) & 0xFFDFFFDFFFDFFFDF)
#define CHAR_UPPER4W(ch) _CHAR_UPPER4W((UInt64)(ch))
#define CHAR_UPPER3W(ch) (UInt64)(CHAR_UPPER4W(ch) & 0x0000FFFFFFFFFFFF)
#define CHAR_UPPER2W(ch) (UInt64)(CHAR_UPPER4W(ch) & 0x00000000FFFFFFFF)
#define CHAR_UPPER1W(ch) (UInt64)(CHAR_UPPER4W(ch) & 0x000000000000FFFF)

#define _CHAR_UPPER4(ch) (UInt32)((ch) & 0xDFDFDFDF)
#define CHAR_UPPER4(ch) (UInt32)_CHAR_UPPER4((UInt32)(ch))
#define CHAR_UPPER3(ch) (UInt32)(CHAR_UPPER4(ch) & 0x00FFFFFF)
#define CHAR_UPPER2(ch) (UInt32)(CHAR_UPPER4(ch) & 0x0000FFFF)
#define CHAR_UPPER1(ch) (UInt32)(CHAR_UPPER4(ch) & 0x000000FF)
#define CHAR_UPPER8(ch) ((UInt64)(ch) & 0xDFDFDFDFDFDFDFDF)

#define _TAG2(s) ((((s) << 8) | ((s) >> 8)) & 0xFFFF)
#define TAG2(s) _TAG2((u16)(s))

#define _TAG3(s) ( \
                (((s) >> 16) & 0xFF)       | \
                (((s)        & 0xFF00))    | \
                (((s) << 16) & 0x00FF0000) \
                )
#define TAG3(s) _TAG3((u32)(s))

#define _TAG4(s) ( \
                (((s) >> 24) & 0xFF)       | \
                (((s) >> 8 ) & 0xFF00)     | \
                (((s) << 24) & 0xFF000000) | \
                (((s) << 8 ) & 0x00FF0000) \
                )
#define TAG4(s) _TAG4((u32)(s))

#define TAG8(left, right) (((UInt64)TAG4(right) << 32) | TAG4(left))

#define _TAG2W(x) (((x) & 0xFF) << 16 | ((x) & 0xFF00) >> 8)
#define TAG2W(x) (UInt32)_TAG2W((UInt32)(x))

#define _TAG3W(x) (TAG4W(x) >> 16)
#define TAG3W(x) (UInt64)_TAG3W((UInt64)(x))

#define _TAG4W(x) (((UInt64)TAG2W((x) & 0xFFFF) << 32) | ((UInt64)TAG2W((x) >> 16)))
#define TAG4W(x) (UInt64)_TAG4W((UInt64)(x))

#pragma warning(disable:4310)
#define SWAP2(v) (u16)(((u32)(v) << 8) | ((u16)(v) >> 8))
#define SWAPCHAR(v) ((u32)SWAP2(v))

#define LoByte(v)  (u8) ((v & 0xFF))
#define HiByte(v)  (u8) (((v) >> 8) & 0xFF)
#define LoWord(v)  (u16)((v) & 0xFFFF)
#define HiWord(v)  (u16)(((v) >> 16) & 0xFFFF)
#define LoDword(v) (u32)((v))
#define HiDword(v) (u32)(((v) >> 32))

#define MakeLong(l, h)   (long)((s32)(l) | ((s32)(h) << 16))
#define MakeLong64(l, h) (s64)((s64)(l) | (s64)(h) << 32)

#define MakeDword(l, h) (u32)((u32)(l) | ((u32)(h) << 16))
#define MakeQword(l, h) (u64)((u64)(l) | (u64)(h) << 32)

#define MAKEINTRESA(i) ((PChar)(Word)(i))
#define MAKEINTRESW(i) ((PWChar)(Word)(i))

#define STRTOULONG(x) (ULong_Ptr)(x)

#define ML_IP_ADDRESS(a1, a2, a3, a4) ((a1) | ((a2) << 8) | ((a3) << 16) | ((a4) << 24))
#define ML_PORT(_port) SWAP2(_port)

#define FOR_EACH(_it, _base, _n) for (auto _Count = ( ((_it) = (_base)), (_n)); _Count != 0; ++(_it), --_Count)
#define FOR_EACH_REVERSE(_it, _base, _n) for (auto _Count = ( ((_it) = (_base) + (_n) - 1), (_n)); _Count != 0; --(_it), --_Count)
#define FOR_EACH_ARRAY(_it, _arr) FOR_EACH(_it, _arr, countof(_arr))
#define FOR_EACH_S(_it, _base, _n, _size) for (auto _Count = ( ((_it) = (_base)), (_n)); _Count != 0; ((_it) = PtrAdd(_it, _size)), --_Count)
#define FOR_EACH_X(_it, _base, _n) for (auto _Count = ( ((_it) = (_base)), (_n); _Count != 0; ++(_it), --(_n), --_Count)

#define FOR_EACH_FORWARD(_it, _n) { (_it) += (_n); (_Count) += (_n); }
#define FOR_EACH_BACKWARD(_it, _n) { (_it) -= (_n); (_Count) -= (_n); }

#if MY_UNICODE_ENABLE
#define MAKEINTRES MAKEINTRESW
#else
#define MAKEINTRES MAKEINTRESA
#endif /* UNICODE */


#define DEFINED_HRESULT(failed, facility, code) \
            ((HRESULT)(((failed) << 31) | ((facility) << 16) | (code)))

#define SET_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTC const GUID DECLSPEC_SELECTANY name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

#define DECL_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTC const GUID FAR name


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

/************************************************************************
bsearch
************************************************************************/

#define BinarySearchM(ValuePtr, Value, Context) [&] (auto ValuePtr, auto Value, auto Context)

template<class ArgType, class ValueType, class RoutineContext, class CompareRoutine>
ArgType
BinarySearch(
	ArgType             First,
	ArgType             Last,
	const ValueType&    Value,
	CompareRoutine      CompareFunction,
	RoutineContext      Context = NULL
)
{
	ULONG_PTR   Left, Right, Middle;
	LONG_PTR    Result;

	if (First == Last)
		return CompareFunction(&First[0], Value, Context) == 0 ? First : NULL;

	Left = 0;
	Right = Last - First - 1;

	Result = CompareFunction(&First[0], Value, Context);
	if (Result > 0)
		return NULL;

	if (Result == 0)
		return First;

	Result = CompareFunction(&Last[-1], Value, Context);
	if (Result < 0)
		return NULL;

	if (Result == 0)
		return &Last[-1];

	while (Left < Right)
	{
		Middle = (Right - Left) / 2 + Left;

		Result = CompareFunction(&First[Middle], Value, Context);
		if (Result == 0)
			return &First[Middle];

		if (Result < 0)
		{
			Left = Middle + 1;
		}
		else
		{
			Right = Middle - 1;
		}
	}

	Result = CompareFunction(&First[Left], Value, Context);

	return Result == 0 ? &First[Left] : NULL;
}


#define TYPE_OF decltype

#define FUNC_POINTER(__func) TYPE_OF(__func)*
#define API_POINTER(__func) TYPE_OF(&__func)
#define PTR_OF(_var) TYPE_OF(_var)*


/************************************************************************
scope exit
************************************************************************/
#pragma warning(disable:4512)

template<typename Lambda>
struct LambdaWrapper
{
	const Lambda &fn;

	ForceInline LambdaWrapper(const Lambda &fn) : fn(fn)
	{
		;
	}

	ForceInline void operator()()
	{
		fn();
	}

	ForceInline ~LambdaWrapper()
	{
		fn();
	}
};

#pragma warning(default:4512)

template<typename ScopeExitRoutine>
ForceInline
LambdaWrapper<ScopeExitRoutine>
CreateScopeExitObject(
	const ScopeExitRoutine &ScopeExitFunction
)
{
	return LambdaWrapper<ScopeExitRoutine>(ScopeExitFunction);
}

#define SCOPE_EXIT auto MAKE_NAME_PREFIX(scope_exit_obj_, __LINE__) = CreateScopeExitObject([&] ()
#define SCOPE_EXIT_END )



template<class InIterator, class Function>
ForceInline Function ForEachZ(InIterator _Count, Function _Func)
{
	for (; _Count; --_Count)
		_Func(_Count);

	return (_Func);
}

template<class InIterator, class Function>
ForceInline Function ForEachR(InIterator _First, InIterator _Last, Function _Func)
{
	for (; _First != _Last; --_First)
		_Func(_First);

	return (_Func);
}

template<class InIterator, class Function>
ForceInline Function ForEach(InIterator _First, InIterator _Last, Function _Func)
{
	for (; _First != _Last; ++_First)
		_Func(_First);

	return (_Func);
}

template<ULong_Ptr StringLength>
struct HashStringConst
{
	template<class StringType> static ULong Hash(StringType String, ULong Initial = 0)
	{
		Initial = _rotl(Initial, 0xD) ^ (Byte)(String[0]);
		return HashStringConst<StringLength - 1>::Hash(++String, Initial);
	}
};

template<>
struct HashStringConst<0>
{
	template<class StringType> static ULong Hash(StringType String, ULong Initial = 0)
	{
		return Initial;
	}
};

#define CONST_STRHASH(_str) HashStringConst<CONST_STRLEN((_str))>::Hash(_str)



#if ML_KERNEL_MODE

//    #include <ntddk.h>

#else

//    #include <winsock2.h>
#include <Windows.h>
#include <Wininet.h>

#pragma warning(disable:4005)

#include <ntstatus.h>

#pragma warning(default:4005)

#endif // MY_NT_DDK

typedef void * POINTER_64 HANDLE64;
typedef HANDLE64 *PHANDLE64;

#pragma comment(lib, "Shlwapi.lib")

#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <direct.h>

#include <conio.h>
#include <stdlib.h>
#include <malloc.h>
#include <time.h>
#include <signal.h>
#include <locale.h>
#include <process.h>

#include <tchar.h>
#include <string.h>

#include <math.h>
#include <float.h>

#include <tchar.h>

#include <Shlwapi.h>


#pragma warning(disable:4127)
#pragma warning(disable:4047)
#pragma warning(disable:4706)


_ML_C_HEAD_

// #include <emmintrin.h>
#if !defined(ML_ARM64)
#include <xmmintrin.h>
#endif

#include <crtversion.h>

#if _VC_CRT_BUILD_VERSION >= 23026
#include <intrin.h>
#endif


/************************************************************************
interlocked
************************************************************************/

long            __cdecl _InterlockedIncrement(long volatile * lpAddend);
long            __cdecl _InterlockedDecrement(long volatile * lpAddend);
long            __cdecl _InterlockedCompareExchange(long volatile * Destination, long Exchange, long Comperand);
long            __cdecl _InterlockedExchangeAdd(long volatile * Addend, long Value);
long            __cdecl _InterlockedExchange(volatile long * Target, long Value);
char            __cdecl _InterlockedExchange8(char volatile *Destination, char ExChange);

long            _InterlockedAnd(long volatile *Destination, long Value);
long            _InterlockedOr(long volatile *Destination, long Value);
long            _InterlockedXor(long volatile *Destination, long Value);

#if ML_X86 || ML_AMD64
long            __cdecl _InterlockedIncrement_acq(long * lpAddend);
long            __cdecl _InterlockedIncrement_rel(long * lpAddend);
#endif

short           _InterlockedIncrement16(short volatile * lpAddend);

#if ML_X86 || ML_AMD64
short           __cdecl _InterlockedIncrement16_acq(short * lpAddend);
short           __cdecl _InterlockedIncrement16_rel(short * lpAddend);
#endif

__int64         _InterlockedIncrement64(volatile __int64 * lpAddend);
__int64         __cdecl _InterlockedIncrement64_acq(volatile __int64 * lpAddend);
__int64         __cdecl _InterlockedIncrement64_rel(volatile __int64 * lpAddend);

#if ML_X86 || ML_AMD64
long            __cdecl _InterlockedDecrement_acq(long * lpAddend);
#endif

short           _InterlockedDecrement16(short volatile * lpAddend);

#if ML_X86 || ML_AMD64
short           __cdecl _InterlockedDecrement16_acq(short * lpAddend);
short           __cdecl _InterlockedDecrement16_rel(short * lpAddend);
#endif

__int64         _InterlockedDecrement64(volatile __int64 * lpAddend);

#if ML_X86 || ML_AMD64
__int64         __cdecl _InterlockedDecrement64_acq(__int64 * lpAddend);
__int64         __cdecl _InterlockedDecrement64_rel(__int64 * lpAddend);
#endif

#if ML_X86 || ML_AMD64
long            __cdecl _InterlockedExchange_acq(long * Target, long Value);
#endif

__int64         _InterlockedExchange64(volatile __int64 * Target, __int64 Value);

#if ML_X86 || ML_AMD64
__int64         __cdecl _InterlockedExchange64_acq(__int64 * Target, __int64 Value);
#endif

long            __cdecl _InterlockedCompareExchange_acq(long volatile * Destination, long Exchange, long Comperand);
long            __cdecl _InterlockedCompareExchange_rel(long volatile * Destination, long Exchange, long Comperand);
short                   _InterlockedCompareExchange16(short volatile * Destination, short Exchange, short Comperand);
short           __cdecl _InterlockedCompareExchange16_acq(short volatile * Destination, short Exchange, short Comperand);
short           __cdecl _InterlockedCompareExchange16_rel(short volatile * Destination, short Exchange, short Comperand);
__int64         __cdecl _InterlockedCompareExchange64(__int64 volatile * Destination, __int64 Exchange, __int64 Comperand);
__int64         __cdecl _InterlockedCompareExchange64_acq(__int64 volatile * Destination, __int64 Exchange, __int64 Comperand);
__int64         __cdecl _InterlockedCompareExchange64_rel(__int64 volatile * Destination, __int64 Exchange, __int64 Comperand);
unsigned char   __cdecl _InterlockedCompareExchange128(__int64 volatile * Destination, __int64 ExchangeHigh, __int64 ExchangeLow, __int64 * Comparand);
unsigned char           _BitScanForward(unsigned long *Index, unsigned long Mask);


void _ReadBarrier(void);
void _WriteBarrier(void);
void _ReadWriteBarrier(void);


/************************************************************************
cpuid
************************************************************************/
void __cpuid(int CPUInfo[4], int InfoType);
void __cpuidex(int CPUInfo[4], int InfoType, int ECXValue);


/************************************************************************
dr
************************************************************************/
UInt_Ptr __readdr(unsigned int DebugRegister);

#if ML_AMD64
void __writedr(unsigned DebugRegister, unsigned __int64 DebugValue);
#elif ML_X86
void __writedr(unsigned int DebugRegister, unsigned int DebugValue);
#endif

/************************************************************************
crx
************************************************************************/

#if ML_AMD64

void __writecr0(unsigned __int64 Data);
void writecr3(unsigned __int64 Data);
void writecr4(unsigned __int64 Data);
void writecr8(unsigned __int64 Data);

#elif ML_X86
void __writecr0(unsigned Data);
void writecr3(unsigned Data);
void writecr4(unsigned Data);
void writecr8(unsigned Data);
#endif

ULongPtr __readcr0(void);


/************************************************************************
msr
************************************************************************/

#pragma warning(disable : 5033)
unsigned __int64 __readmsr(unsigned long register);
void __writemsr(unsigned long Register, unsigned __int64 Value);

#pragma warning(default : 5033)


/************************************************************************
vmx
************************************************************************/

void __vmx_off();


/************************************************************************
idt
************************************************************************/

void __lidt(void *);


#if ML_AMD64

unsigned char __vmx_on(unsigned __int64 *VmsSupportPhysicalAddress);
unsigned char __vmx_vmread(size_t Field, size_t *FieldValue);
unsigned char __vmx_vmwrite(size_t Field, size_t FieldValue);
unsigned char __vmx_vmclear(unsigned __int64 *VmcsPhysicalAddress);
unsigned char __vmx_vmlaunch(void);
unsigned char __vmx_vmptrld(unsigned __int64 *VmcsPhysicalAddress);
void          __vmx_vmptrst(unsigned __int64 *VmcsPhysicalAddress);

#endif

#if !ML_AMD64

#define _InterlockedCompareExchangePointer(Target, Exchange, Comperand) (PVOID)_InterlockedCompareExchange((long *)Target, (long)Exchange, (long)Comperand)

#ifndef _InterlockedExchangePointer
#define _InterlockedExchangePointer(Target, Value) (PVOID)_InterlockedExchange((long *)Target, (long)Value)
#endif

#if ML_USER_MODE

#ifndef _WIN32_WINNT_WIN8
#if   (_WIN32_WINNT <= _WIN32_WINNT_WINXP)

inline long long _InterlockedExchange64(long long volatile *Target, long long Value)
{
	long long Old;

	do
	{
		Old = *Target;
	} while (_InterlockedCompareExchange64(Target, Value, Old) != Old);

	return Old;
}

#endif
#endif

#endif // r3

#else

#define _InterlockedCompareExchangePointer(Target, Exchange, Comperand) (PVOID)_InterlockedCompareExchange64((__int64 *)Target, (__int64)Exchange, (__int64)Comperand)
void * _InterlockedExchangePointer(void * volatile * Target, void * Value);
void * _InterlockedExchangePointer_acq(void * volatile * Target, void * Value);

#endif

#if MY_OS_WIN32
#define Interlocked_Increment _InterlockedIncrement
#define Interlocked_Decrement _InterlockedDecrement
#else
#define Interlocked_Increment InterlockedIncrement
#define Interlocked_Decrement InterlockedDecrement
#endif // MY_OS_WIN32

ForceInline LongPtr _InterlockedIncrementPtr(PVoid Addend)
{
#if ML_AMD64
	return _InterlockedIncrement64((PLongPtr)Addend);
#else
	return _InterlockedIncrement((PLongPtr)Addend);
#endif
}

ForceInline LongPtr _InterlockedDecrementPtr(PVoid Addend)
{
#if ML_AMD64
	return _InterlockedDecrement64((PLongPtr)Addend);
#else
	return _InterlockedDecrement((PLongPtr)Addend);
#endif
}

ForceInline BOOLEAN MlBitScanForwardPtr(PULONG Index, ULONG_PTR Mask)
{
#if ML_AMD64

	return _BitScanForward64(Index, Mask);

#else

	return _BitScanForward(Index, Mask);

#endif
}


_ML_C_TAIL_


_ML_C_HEAD_

#define CopyStruct(dest, src, size) \
    { \
        typedef DECL_ALIGN(1) struct { ::Byte b[size]; } _SDummy; \
        *(_SDummy *)(dest) = *(_SDummy *)(src); \
    }

ForceInline void* AllocStack(size_t Size)
{
#if _DEBUG
	return malloc(ROUND_UP(Size, 16));
#else
	return _alloca(ROUND_UP(Size, 16));
#endif
}


_ML_C_TAIL_



#pragma warning(default:4047)
#pragma warning(default:4127)
#pragma warning(default:4706)



_ML_C_HEAD_

#pragma pack(1)

#define SHA224_DIGEST_SIZE ( 224 / 8)
#define SHA256_DIGEST_SIZE ( 256 / 8)
#define SHA384_DIGEST_SIZE ( 384 / 8)
#define SHA512_DIGEST_SIZE ( 512 / 8)

#define SHA256_BLOCK_SIZE  ( 512 / 8)
#define SHA512_BLOCK_SIZE  (1024 / 8)
#define SHA384_BLOCK_SIZE  SHA512_BLOCK_SIZE
#define SHA224_BLOCK_SIZE  SHA256_BLOCK_SIZE

typedef struct
{
	ULONG   tot_len;
	ULONG   len;
	BYTE    block[2 * SHA256_BLOCK_SIZE];
	ULONG   h[8];
} sha256_ctx;

#pragma pack()

VOID STDCALL sha256_init(sha256_ctx *ctx);
VOID STDCALL sha256_update(sha256_ctx *ctx, PVOID message, ULONG len);
VOID STDCALL sha256_final(sha256_ctx *ctx, PVOID digest);
VOID STDCALL sha256(PVOID message, ULONG len, PVOID digest);

inline VOID sha256to128(PVOID message, ULONG len, PVOID digest)
{
	ULONG Hash[8];

	sha256(message, len, Hash);
	((PULONG)digest)[0] = Hash[0] ^ Hash[4];
	((PULONG)digest)[1] = Hash[1] ^ Hash[5];
	((PULONG)digest)[2] = Hash[2] ^ Hash[6];
	((PULONG)digest)[3] = Hash[3] ^ Hash[7];
}

#if !defined(ReadTimeStampCounter)

#if ML_X86 || ML_AMD64
ForceInline UInt64 ReadTimeStampCounter()
{
	__asm rdtsc;
}
#endif

#endif // ReadTimeStampCounter

ForceInline UInt64 GetRandom64()
{
	return ReadTimeStampCounter();
}

ForceInline UInt32 GetRandom32()
{
	return (UInt32)GetRandom64();
}


_ML_C_TAIL_

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
ForceInline
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

	if (MY_MAX(sizeof(Ptr1), sizeof(Ptr2)) == sizeof(u.Value1.QuadPart))
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

	if (MY_MAX(sizeof(Ptr1), sizeof(Ptr2)) == sizeof(u.Value1.QuadPart))
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
ForceInline
PtrType1 PtrOperator(PtrType1 Ptr1, unsigned short Ptr2)
{
	return PtrOperator<OP>(Ptr1, (ULONG_PTR)Ptr2);
}

template<PointerOperationType::PointerOperationClass OP, class PtrType1>
ForceInline
PtrType1 PtrOperator(PtrType1 Ptr1, short Ptr2)
{
	return PtrOperator<OP>(Ptr1, (ULONG_PTR)Ptr2);
}

template<PointerOperationType::PointerOperationClass OP, class PtrType1>
ForceInline
PtrType1 PtrOperator(PtrType1 Ptr1, unsigned char Ptr2)
{
	return PtrOperator<OP>(Ptr1, (ULONG_PTR)Ptr2);
}

template<PointerOperationType::PointerOperationClass OP, class PtrType1>
ForceInline
PtrType1 PtrOperator(PtrType1 Ptr1, char Ptr2)
{
	return PtrOperator<OP>(Ptr1, (ULONG_PTR)Ptr2);
}

template<class PtrType1, class PtrType2>
ForceInline
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
ForceInline
PtrType1 PtrAdd(PtrType1 Ptr, PtrType2 Offset)
{
	return PtrOperator<PointerOperationType::POINTER_OP_ADD>(Ptr, Offset);
}

template<class PtrType1, class PtrType2>
ForceInline
PtrType1 PtrSub(PtrType1 Ptr, PtrType2 Offset)
{
	return PtrOperator<PointerOperationType::POINTER_OP_SUB>(Ptr, Offset);
}

template<class PtrType1, class PtrType2>
ForceInline
PtrType1 PtrAnd(PtrType1 Ptr, PtrType2 Offset)
{
	return PtrOperator<PointerOperationType::POINTER_OP_AND>(Ptr, Offset);
}

template<class PtrType1, class PtrType2>
ForceInline
PtrType1 PtrOr(PtrType1 Ptr, PtrType2 Offset)
{
	return PtrOperator<PointerOperationType::POINTER_OP_OR>(Ptr, Offset);
}

template<class PtrType1, class PtrType2>
ForceInline
PtrType1 PtrXor(PtrType1 Ptr, PtrType2 Offset)
{
	return PtrOperator<PointerOperationType::POINTER_OP_XOR>(Ptr, Offset);
}

template<class PtrType1, class PtrType2>
ForceInline
PtrType1 PtrMod(PtrType1 Ptr, PtrType2 Offset)
{
	return PtrOperator<PointerOperationType::POINTER_OP_MOD>(Ptr, Offset);
}

template<class T> ForceInline Void FASTCALL Swap(T &t1, T &t2)
{
	T t;
	t = t1;
	t1 = t2;
	t2 = t;
}

ForceInline UInt32 GetRandom32Range(UInt32 Min, UInt32 Max)
{
	UInt32 Result = GetRandom32();

	if (Max < Min)
		Swap(Min, Max);

	return (Result % (Max + 1 - Min) + Min);
}


template<typename T> T Bswap(T);

#define DECL_BSWAP_2(type) template<> inline type Bswap(type v) { return _byteswap_ushort(v); }
#define DECL_BSWAP_4(type) template<> inline type Bswap(type v) { return _byteswap_ulong(v); }
#define DECL_BSWAP_8(type) template<> inline type Bswap(type v) { return _byteswap_uint64(v); }

DECL_BSWAP_2(Int16);
DECL_BSWAP_2(UInt16);
DECL_BSWAP_2(WChar);

DECL_BSWAP_4(Int32);
DECL_BSWAP_4(UInt32);

DECL_BSWAP_8(Int64);
DECL_BSWAP_8(UInt64);

DECL_BSWAP_4(Long);
DECL_BSWAP_4(ULong);


_ML_C_HEAD_

#if ML_X86

ForceInline UInt32 FASTCALL GetEfFlags()
{
	INLINE_ASM
	{
		pushfd;
		pop eax;
	}
}

ForceInline UInt32 FASTCALL SetEfFlags(UInt32 EfFlags)
{
	INLINE_ASM
	{
		push EfFlags;
		popfd;
	}
}

ForceInline Void SetDirectionFlag()
{
	INLINE_ASM std;
}

ForceInline Void ClearDirectionFlag()
{
	INLINE_ASM cld;
}

inline Bool IsCPUIDValid()
{
	__asm
	{
		pushfd;
		pop eax;
		or eax, 1 << 21;
		push eax;
		popfd;
		pushfd;
		pop eax;
		shr eax, 21;
		and eax, 1;
	}
}

#endif // x86

_ML_C_TAIL_


#pragma warning(disable:4103)

#if ML_AMD64
#pragma pack(push, 8)
#else
#pragma pack(push, 4)
#endif

#if ML_KERNEL_MODE


#define INFINITE ULONG_PTR_MAX

typedef USHORT ATOM;
typedef PVOID PRTL_CRITICAL_SECTION;

// typedef PROCESSINFOCLASS    PROCESS_INFORMATION_CLASS;
// typedef THREADINFOCLASS     THREAD_INFORMATION_CLASS;

typedef struct _KSERVICE_TABLE_DESCRIPTOR
{
	PULONG_PTR  Base;       // ssdt
	PULONG_PTR  Count;      // callout count of each system service
	ULONG_PTR   Limit;      // count of system call
	PUCHAR      Number;     // Size of arguments

} KSERVICE_TABLE_DESCRIPTOR, *PKSERVICE_TABLE_DESCRIPTOR;

///////////////////////////// apc /////////////////////////////////////

typedef enum _KAPC_ENVIRONMENT
{
	OriginalApcEnvironment,
	AttachedApcEnvironment,
	CurrentApcEnvironment,
	InsertApcEnvironment

} KAPC_ENVIRONMENT;

#else // r3


__if_not_exists(NTSTATUS)
{
	typedef LONG NTSTATUS;
}


char _RTL_CONSTANT_STRING_type_check(const void *s);
#define _RTL_CONSTANT_STRING_remove_const_macro(s) (s)

#ifndef RTL_CONSTANT_STRING
#define RTL_CONSTANT_STRING(s) \
						{ \
    sizeof( s ) - sizeof( (s)[0] ), \
    sizeof( s ) / sizeof(_RTL_CONSTANT_STRING_type_check(s)), \
    _RTL_CONSTANT_STRING_remove_const_macro((PWSTR)s) \
						}
#endif

/************************************************************************/
/* others                                                               */
/************************************************************************/

#if !defined(NtCurrentProcess)
#define NtCurrentProcess() (HANDLE)-1
#define NtCurrentProcess64() (HANDLE64)-1
#endif /* NtCurrentProcess */

#if !defined(NtCurrentThread)
#define NtCurrentThread() (HANDLE)-2
#define NtCurrentThread64() (HANDLE64)-2
#endif /* NtCurrentThread */

#ifndef NT_SUCCESS

ForceInline Bool IsStatusSuccess(NTSTATUS Status) { return (Status & Status) >= 0; }
//    #define NT_SUCCESS(Status)  (((NTSTATUS)(Status)) >= 0)
#define NT_SUCCESS(Status)  IsStatusSuccess(Status)
#endif /* NT_SUCCESS */

#if !defined(RT_MESSAGETABLE)
#define RT_MESSAGETABLE 0x0B
#endif




	/************************************************************************
	object types
	************************************************************************/


#undef InitializeObjectAttributes
inline VOID InitializeObjectAttributes(POBJECT_ATTRIBUTES p, PUNICODE_STRING n, ULONG a, HANDLE r, PVOID s)
{
	p->Length = sizeof(*p);
	p->RootDirectory = r;
	p->Attributes = a;
	p->ObjectName = n;
	p->SecurityDescriptor = s;
	p->SecurityQualityOfService = NULL;

	*(volatile PUNICODE_STRING *)&n = n;
}

#ifndef InitializeObjectAttributes
#define __InitializeObjectAttributes_unused( p, n, a, r, s ) { \
        (p)->Length = sizeof( OBJECT_ATTRIBUTES );          \
        (p)->RootDirectory = r;                             \
        (p)->Attributes = a;                                \
        (p)->ObjectName = n;                                \
        (p)->SecurityDescriptor = s;                        \
        (p)->SecurityQualityOfService = NULL;               \
						    }
#endif


/************************************************************************
object information
************************************************************************/

//
// Object Manager Object Type Specific Access Rights.
//


#define OBJECT_TYPE_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | 0x1)


/************************************************************************/
/* file                                                                 */
/************************************************************************/

#define FSRTL_FAT_LEGAL         0x01
#define FSRTL_HPFS_LEGAL        0x02
#define FSRTL_NTFS_LEGAL        0x04
#define FSRTL_WILD_CHARACTER    0x08
#define FSRTL_OLE_LEGAL         0x10
#define FSRTL_NTFS_STREAM_LEGAL (FSRTL_NTFS_LEGAL | FSRTL_OLE_LEGAL)

#define FsRtlIsUnicodeCharacterWild(C) (                                \
      (((C) >= 0x40) ? FALSE : FLAG_ON( LEGAL_ANSI_CHARACTER_ARRAY[(C)], \
                                       FSRTL_WILD_CHARACTER ) )         \
)

#define ANSI_DOS_STAR   ('<')
#define ANSI_DOS_QM     ('>')
#define ANSI_DOS_DOT    ('"')

#define DOS_STAR        (L'<')
#define DOS_QM          (L'>')
#define DOS_DOT         (L'"')

#endif // ML_KERNEL_MODE


#pragma warning(disable:4103)

//#pragma warning(disable:4201)

_ML_C_HEAD_

#if !defined(FAIL_RETURN)
#define FAIL_RETURN(Status) { NTSTATUS __Status__;  __Status__ = (Status); if (!NT_SUCCESS(__Status__)) return __Status__; }
#define FAIL_CONTINUE(Status) if (!NT_SUCCESS(Status)) continue
#define FAIL_BREAK(Status) if (!NT_SUCCESS(Status)) break
#define SUCCESS_RETURN(Status) { NTSTATUS __Status__;  __Status__ = (Status); if (NT_SUCCESS(__Status__)) return __Status__; }
#endif  // FAIL_RETURN

#ifndef NT_FAILED
#define NT_FAILED !NT_SUCCESS
#endif // NT_FAILED


#ifndef FACILITY_NTWIN32
#define FACILITY_NTWIN32                 0x7
#endif // FACILITY_NTWIN32

#define DEFINE_NTSTATUS(Severity, Code) \
            (((ULONG)(Severity) << 30) | (1 << 29) | (FACILITY_NTWIN32 << 16) | (USHORT)(Code))






/************************************************************************
afd
************************************************************************/

#if ML_USER_MODE

typedef struct _MDL
{
	struct _MDL *Next;
	SHORT Size;
	SHORT MdlFlags;

	struct _EPROCESS *Process;
	PVOID MappedSystemVa;   /* see creators for field size annotations. */
	PVOID StartVa;   /* see creators for validity; could be address 0.  */
	ULONG ByteCount;
	ULONG ByteOffset;

} MDL, *PMDL;

#endif // r3

typedef struct _AFD_MAPBUF
{
	PVOID BufferAddress;
	PMDL  Mdl;

} AFD_MAPBUF, *PAFD_MAPBUF;

typedef struct _AFD_WSABUF
{
	ULONG  Length;
	PCHAR  Buffer;

} AFD_WSABUF, *PAFD_WSABUF;

typedef struct  _AFD_RECV_INFO
{
	PAFD_WSABUF    BufferArray;
	ULONG          BufferCount;
	ULONG          AfdFlags;
	ULONG          TdiFlags;

} AFD_RECV_INFO, *PAFD_RECV_INFO;

typedef struct _AFD_RECV_INFO_UDP
{
	PAFD_WSABUF     BufferArray;
	ULONG           BufferCount;
	ULONG           AfdFlags;
	ULONG           TdiFlags;
	PVOID           Address;
	PLONG           AddressLength;

} AFD_RECV_INFO_UDP, *PAFD_RECV_INFO_UDP;

typedef struct  _AFD_SEND_INFO
{
	PAFD_WSABUF BufferArray;
	ULONG       BufferCount;
	ULONG       AfdFlags;
	ULONG       TdiFlags;

} AFD_SEND_INFO, *PAFD_SEND_INFO;

typedef struct _AFD_SEND_INFO_UDP
{
	PAFD_WSABUF BufferArray;
	ULONG       BufferCount;
	ULONG       AfdFlags;
	ULONG       Padding[9];
	ULONG       SizeOfRemoteAddress;
	PVOID       RemoteAddress;

} AFD_SEND_INFO_UDP, *PAFD_SEND_INFO_UDP;

/* IOCTL Generation */
#define FSCTL_AFD_BASE                      FILE_DEVICE_NETWORK
#define _AFD_CONTROL_CODE(Operation, Method) ((FSCTL_AFD_BASE)<<12 | (Operation<<2) | Method)

	/* AFD Commands */
#define AFD_BIND                        0
#define AFD_CONNECT                     1
#define AFD_START_LISTEN                2
#define AFD_WAIT_FOR_LISTEN             3
#define AFD_ACCEPT                      4
#define AFD_RECV                        5
#define AFD_RECV_DATAGRAM               6
#define AFD_SEND                        7
#define AFD_SEND_DATAGRAM               8
#define AFD_SELECT                      9
#define AFD_DISCONNECT                  10
#define AFD_GET_SOCK_NAME               11
#define AFD_GET_PEER_NAME               12
#define AFD_GET_TDI_HANDLES             13
#define AFD_SET_INFO                    14
#define AFD_GET_CONTEXT_SIZE            15
#define AFD_GET_CONTEXT                 16
#define AFD_SET_CONTEXT                 17
#define AFD_SET_CONNECT_DATA            18
#define AFD_SET_CONNECT_OPTIONS         19
#define AFD_SET_DISCONNECT_DATA         20
#define AFD_SET_DISCONNECT_OPTIONS      21
#define AFD_GET_CONNECT_DATA            22
#define AFD_GET_CONNECT_OPTIONS         23
#define AFD_GET_DISCONNECT_DATA         24
#define AFD_GET_DISCONNECT_OPTIONS      25
#define AFD_SET_CONNECT_DATA_SIZE       26
#define AFD_SET_CONNECT_OPTIONS_SIZE    27
#define AFD_SET_DISCONNECT_DATA_SIZE    28
#define AFD_SET_DISCONNECT_OPTIONS_SIZE 29
#define AFD_GET_INFO                    30
#define AFD_EVENT_SELECT                33
#define AFD_ENUM_NETWORK_EVENTS         34
#define AFD_DEFER_ACCEPT                35
#define AFD_GET_PENDING_CONNECT_DATA    41
#define AFD_VALIDATE_GROUP              42

	/* AFD IOCTLs */

#define IOCTL_AFD_BIND                          _AFD_CONTROL_CODE(AFD_BIND,                         METHOD_NEITHER)     //  0x12003
#define IOCTL_AFD_CONNECT                       _AFD_CONTROL_CODE(AFD_CONNECT,                      METHOD_NEITHER)     //  0x12007
#define IOCTL_AFD_START_LISTEN                  _AFD_CONTROL_CODE(AFD_START_LISTEN,                 METHOD_NEITHER)     //  0x1200B
#define IOCTL_AFD_WAIT_FOR_LISTEN               _AFD_CONTROL_CODE(AFD_WAIT_FOR_LISTEN,              METHOD_BUFFERED)    //  0x1200C
#define IOCTL_AFD_ACCEPT                        _AFD_CONTROL_CODE(AFD_ACCEPT,                       METHOD_BUFFERED)    //  0x12010
#define IOCTL_AFD_RECV                          _AFD_CONTROL_CODE(AFD_RECV,                         METHOD_NEITHER)     //  0x12017
#define IOCTL_AFD_RECV_DATAGRAM                 _AFD_CONTROL_CODE(AFD_RECV_DATAGRAM,                METHOD_NEITHER)     //  0x1201B
#define IOCTL_AFD_SEND                          _AFD_CONTROL_CODE(AFD_SEND,                         METHOD_NEITHER)     //  0x1201F
#define IOCTL_AFD_SEND_DATAGRAM                 _AFD_CONTROL_CODE(AFD_SEND_DATAGRAM,                METHOD_NEITHER)     //  0x12023
#define IOCTL_AFD_SELECT                        _AFD_CONTROL_CODE(AFD_SELECT,                       METHOD_BUFFERED)    //  0x12024
#define IOCTL_AFD_DISCONNECT                    _AFD_CONTROL_CODE(AFD_DISCONNECT,                   METHOD_NEITHER)     //  0x1202B
#define IOCTL_AFD_GET_SOCK_NAME                 _AFD_CONTROL_CODE(AFD_GET_SOCK_NAME,                METHOD_NEITHER)     //  0x1202F
#define IOCTL_AFD_GET_PEER_NAME                 _AFD_CONTROL_CODE(AFD_GET_PEER_NAME,                METHOD_NEITHER)     //  0x12033
#define IOCTL_AFD_GET_TDI_HANDLES               _AFD_CONTROL_CODE(AFD_GET_TDI_HANDLES,              METHOD_NEITHER)     //  0x12037
#define IOCTL_AFD_SET_INFO                      _AFD_CONTROL_CODE(AFD_SET_INFO,                     METHOD_NEITHER)     //  0x1203B
#define IOCTL_AFD_GET_CONTEXT_SIZE              _AFD_CONTROL_CODE(AFD_GET_CONTEXT_SIZE,             METHOD_NEITHER)     //  0x1203F
#define IOCTL_AFD_GET_CONTEXT                   _AFD_CONTROL_CODE(AFD_GET_CONTEXT,                  METHOD_NEITHER)     //  0x12043
#define IOCTL_AFD_SET_CONTEXT                   _AFD_CONTROL_CODE(AFD_SET_CONTEXT,                  METHOD_NEITHER)     //  0x12047
#define IOCTL_AFD_SET_CONNECT_DATA              _AFD_CONTROL_CODE(AFD_SET_CONNECT_DATA,             METHOD_NEITHER)     //  0x1204B
#define IOCTL_AFD_SET_CONNECT_OPTIONS           _AFD_CONTROL_CODE(AFD_SET_CONNECT_OPTIONS,          METHOD_NEITHER)     //  0x1204F
#define IOCTL_AFD_SET_DISCONNECT_DATA           _AFD_CONTROL_CODE(AFD_SET_DISCONNECT_DATA,          METHOD_NEITHER)     //  0x12053
#define IOCTL_AFD_SET_DISCONNECT_OPTIONS        _AFD_CONTROL_CODE(AFD_SET_DISCONNECT_OPTIONS,       METHOD_NEITHER)     //  0x12057
#define IOCTL_AFD_GET_CONNECT_DATA              _AFD_CONTROL_CODE(AFD_GET_CONNECT_DATA,             METHOD_NEITHER)     //  0x1205B
#define IOCTL_AFD_GET_CONNECT_OPTIONS           _AFD_CONTROL_CODE(AFD_GET_CONNECT_OPTIONS,          METHOD_NEITHER)     //  0x1205F
#define IOCTL_AFD_GET_DISCONNECT_DATA           _AFD_CONTROL_CODE(AFD_GET_DISCONNECT_DATA,          METHOD_NEITHER)     //  0x12063
#define IOCTL_AFD_GET_DISCONNECT_OPTIONS        _AFD_CONTROL_CODE(AFD_GET_DISCONNECT_OPTIONS,       METHOD_NEITHER)     //  0x12067
#define IOCTL_AFD_SET_CONNECT_DATA_SIZE         _AFD_CONTROL_CODE(AFD_SET_CONNECT_DATA_SIZE,        METHOD_NEITHER)     //  0x1206B
#define IOCTL_AFD_SET_CONNECT_OPTIONS_SIZE      _AFD_CONTROL_CODE(AFD_SET_CONNECT_OPTIONS_SIZE,     METHOD_NEITHER)     //  0x1206F
#define IOCTL_AFD_SET_DISCONNECT_DATA_SIZE      _AFD_CONTROL_CODE(AFD_SET_DISCONNECT_DATA_SIZE,     METHOD_NEITHER)     //  0x12073
#define IOCTL_AFD_SET_DISCONNECT_OPTIONS_SIZE   _AFD_CONTROL_CODE(AFD_SET_DISCONNECT_OPTIONS_SIZE,  METHOD_NEITHER)     //  0x12077
#define IOCTL_AFD_GET_INFO                      _AFD_CONTROL_CODE(AFD_GET_INFO,                     METHOD_NEITHER)     //  0x1207B
#define IOCTL_AFD_EVENT_SELECT                  _AFD_CONTROL_CODE(AFD_EVENT_SELECT,                 METHOD_NEITHER)     //  0x12087
#define IOCTL_AFD_DEFER_ACCEPT                  _AFD_CONTROL_CODE(AFD_DEFER_ACCEPT,                 METHOD_NEITHER)     //  0x1208F
#define IOCTL_AFD_GET_PENDING_CONNECT_DATA      _AFD_CONTROL_CODE(AFD_GET_PENDING_CONNECT_DATA,     METHOD_NEITHER)     //  0x120A7
#define IOCTL_AFD_ENUM_NETWORK_EVENTS           _AFD_CONTROL_CODE(AFD_ENUM_NETWORK_EVENTS,          METHOD_NEITHER)     //  0x1208B
#define IOCTL_AFD_VALIDATE_GROUP                _AFD_CONTROL_CODE(AFD_VALIDATE_GROUP,               METHOD_NEITHER)     //  0x120AB

	/************************************************************************
	iocp api
	************************************************************************/


/************************************************************************
shim engine
************************************************************************/
typedef enum
{
	SE_PROCESS_ATTACH = 1,
	SE_PROCESS_DETACH = 2,
	SE_DLL_LOAD = 3,
} SHIM_NOTIFY_REASON;

typedef struct
{
	PCSTR   ModuleName;
	PCSTR   FunctionName;
	PVOID   NewRoutine;
	PVOID   Unknown[3];
} SHIM_HOOK_API_INFO, *PSHIM_HOOK_API_INFO;


/************************************************************************
misc
************************************************************************/

#define DELAY_ONE_MICROSECOND   (-10)
#define DELAY_ONE_MILLISECOND   (DELAY_ONE_MICROSECOND * 1000)
#define DELAY_ONE_SECOND        (DELAY_ONE_MILLISECOND * 1000)
#define DELAY_QUAD_INFINITE     0x8000000000000000ll


_ML_C_TAIL_


#pragma warning(default:4103)

//#pragma warning(default:4201)


#define NATIVE_API  EXTC_IMPORT
#define NTKRNLAPI   EXTC_IMPORT

ForceInline NTSTATUS ML_NTSTATUS_FROM_WIN32(long x) { return x <= 0 ? (NTSTATUS)x : (NTSTATUS)(((x) & 0x0000FFFF) | (FACILITY_NTWIN32 << 16) | ERROR_SEVERITY_ERROR); }


#if ML_KERNEL_MODE

typedef struct _KTRAP_FRAME32
{
	/* 0x000 */ ULONG                          DbgEbp;
	/* 0x004 */ ULONG                          DbgEip;
	/* 0x008 */ ULONG                          DbgArgMark;
	/* 0x00C */ ULONG                          DbgArgPointer;
	/* 0x010 */ USHORT                         TempSegCs;
	/* 0x012 */ UCHAR                          Logging;
	/* 0x013 */ UCHAR                          Reserved;
	/* 0x014 */ ULONG                          TempEsp;
	/* 0x018 */ ULONG                          Dr0;
	/* 0x01C */ ULONG                          Dr1;
	/* 0x020 */ ULONG                          Dr2;
	/* 0x024 */ ULONG                          Dr3;
	/* 0x028 */ ULONG                          Dr6;
	/* 0x02C */ ULONG                          Dr7;
	/* 0x030 */ ULONG                          SegGs;
	/* 0x034 */ ULONG                          SegEs;
	/* 0x038 */ ULONG                          SegDs;
	/* 0x03C */ ULONG                          Edx;
	/* 0x040 */ ULONG                          Ecx;
	/* 0x044 */ ULONG                          Eax;
	/* 0x048 */ ULONG                          PreviousPreviousMode;
	/* 0x04C */ PEXCEPTION_REGISTRATION_RECORD ExceptionList;
	/* 0x050 */ ULONG                          SegFs;
	/* 0x054 */ ULONG                          Edi;
	/* 0x058 */ ULONG                          Esi;
	/* 0x05C */ ULONG                          Ebx;
	/* 0x060 */ ULONG                          Ebp;
	/* 0x064 */ ULONG                          ErrCode;
	/* 0x068 */ ULONG                          Eip;
	/* 0x06C */ ULONG                          SegCs;
	/* 0x070 */ ULONG                          EFlags;
	/* 0x074 */ ULONG                          HardwareEsp;
	/* 0x078 */ ULONG                          HardwareSegSs;
	/* 0x07C */ ULONG                          V86Es;
	/* 0x080 */ ULONG                          V86Ds;
	/* 0x084 */ ULONG                          V86Fs;
	/* 0x088 */ ULONG                          V86Gs;

} KTRAP_FRAME32, *PKTRAP_FRAME32;

#if ML_AMD64
#else // 32

typedef PKTRAP_FRAME32 PKTRAP_FRAME;

struct KEXCEPTION_FRAME;

typedef KEXCEPTION_FRAME *PKEXCEPTION_FRAME;

#endif // x86

VOID
NTAPI
KiDispatchException(
	IN      PEXCEPTION_RECORD    ExceptionRecord,
	IN      PKEXCEPTION_FRAME    ExceptionFrame,
	IN OUT  PKTRAP_FRAME         TrapFrame,
	IN      KPROCESSOR_MODE      PreviousMode,
	IN      BOOLEAN              FirstChance
);

#else // r3

NATIVE_API
VOID
NTAPI
KiUserExceptionDispatcher(
	PEXCEPTION_RECORD   ExceptionRecord,
	PCONTEXT            Context
);


#endif // !ML_KERNEL_MODE



#define HARDERROR_PARAMETERS_FLAGSPOS   4
#define HARDERROR_FLAGS_DEFDESKTOPONLY  0x00020000

#define MAXIMUM_HARDERROR_PARAMETERS    5
#define HARDERROR_OVERRIDE_ERRORMODE    0x10000000

NATIVE_API
NTSTATUS
NTAPI
NtRaiseHardError(
	IN  NTSTATUS    ErrorStatus,
	IN  ULONG       NumberOfParameters,
	IN  ULONG       UnicodeStringParameterMask,
	IN  PULONG_PTR  Parameters,
	IN  ULONG       ValidResponseOptions,
	OUT PULONG      Response
);

NATIVE_API
NTSTATUS
NTAPI
ZwRaiseHardError(
	IN  NTSTATUS    ErrorStatus,
	IN  ULONG       NumberOfParameters,
	IN  ULONG       UnicodeStringParameterMask,
	IN  PULONG_PTR  Parameters,
	IN  ULONG       ValidResponseOptions,
	OUT PULONG      Response
);

NATIVE_API
NTSTATUS
NTAPI
NtRaiseException(
	IN  PEXCEPTION_RECORD   ExceptionRecord,
	IN  PCONTEXT            ContextRecord,
	IN  BOOLEAN             FirstChance
);

NATIVE_API
NTSTATUS
NTAPI
ZwRaiseException(
	IN  PEXCEPTION_RECORD   ExceptionRecord,
	IN  PCONTEXT            ContextRecord,
	IN  BOOLEAN             FirstChance
);

NATIVE_API
NTSTATUS
NTAPI
ExRaiseHardError(
	IN  NTSTATUS    ErrorStatus,
	IN  ULONG       NumberOfParameters,
	IN  ULONG       UnicodeStringParameterMask,
	IN  PULONG_PTR  Parameters,
	IN  ULONG       ValidResponseOptions,
	OUT PULONG      Response
);


#ifndef FILE_BEGIN
#define FILE_BEGIN      0
#define FILE_CURRENT    1
#define FILE_END        2
#endif /* FILE_BEGIN */

#if !defined(INVALID_FILE_ATTRIBUTES)
#define INVALID_FILE_ATTRIBUTES ((ULONG)-1)
#endif // INVALID_FILE_ATTRIBUTES

#if !defined(FSCTL_GET_REPARSE_POINT)

#include <WinIoctl.h>

#endif // FSCTL_GET_REPARSE_POINT

typedef struct _FILE_NAME_INFORMATION2
{
	ULONG FileNameLength;
	WCHAR FileName[MAX_NTPATH];

} FILE_NAME_INFORMATION2, *PFILE_NAME_INFORMATION2;

#if !defined(_NTIFS_)

#define SYMLINK_FLAG_RELATIVE   1

#endif // _NTIFS_



#if ML_USER_MODE


// Named pipe values

// NamedPipeType for NtCreateNamedPipeFile
#define FILE_PIPE_BYTE_STREAM_TYPE 0x00000000
#define FILE_PIPE_MESSAGE_TYPE 0x00000001

	// CompletionMode for NtCreateNamedPipeFile
#define FILE_PIPE_QUEUE_OPERATION 0x00000000
#define FILE_PIPE_COMPLETE_OPERATION 0x00000001

	// ReadMode for NtCreateNamedPipeFile
#define FILE_PIPE_BYTE_STREAM_MODE 0x00000000
#define FILE_PIPE_MESSAGE_MODE 0x00000001

	// NamedPipeConfiguration for NtQueryInformationFile
#define FILE_PIPE_INBOUND 0x00000000
#define FILE_PIPE_OUTBOUND 0x00000001
#define FILE_PIPE_FULL_DUPLEX 0x00000002

	// NamedPipeState for NtQueryInformationFile
#define FILE_PIPE_DISCONNECTED_STATE 0x00000001
#define FILE_PIPE_LISTENING_STATE 0x00000002
#define FILE_PIPE_CONNECTED_STATE 0x00000003
#define FILE_PIPE_CLOSING_STATE 0x00000004

	// NamedPipeEnd for NtQueryInformationFile
#define FILE_PIPE_CLIENT_END 0x00000000
#define FILE_PIPE_SERVER_END 0x00000001

#endif // r3 only





#if defined(USE_LPC6432)

#define LPC_CLIENT_ID CLIENT_ID64
#define LPC_SIZE_T ULONGLONG
#define LPC_PVOID ULONGLONG
#define LPC_HANDLE ULONGLONG

typedef CLIENT_ID64 LPC_CLIENT_ID;
typedef ULONGLONG   LPC_SIZE_T, LPC_SIZE;
typedef ULONGLONG   LPC_PVOID;
typedef ULONGLONG   LPC_HANDLE;

#else

#define LPC_CLIENT_ID CLIENT_ID
#define LPC_SIZE_T SIZE_T
#define LPC_PVOID PVOID
#define LPC_HANDLE HANDLE

typedef CLIENT_ID   LPC_CLIENT_ID;
typedef ULONG_PTR   LPC_SIZE_T, LPC_SIZE;
typedef PVOID       LPC_PVOID;
typedef HANDLE      LPC_HANDLE;

#endif


typedef NTSTATUS LPC_STATUS;


	// begin_ntddk begin_wdm
#ifdef _WIN64
#define PORT_MAXIMUM_MESSAGE_LENGTH 512
#else
#define PORT_MAXIMUM_MESSAGE_LENGTH 256
#endif

#define LPC_MAX_CONNECTION_INFO_SIZE (16 * sizeof(ULONG_PTR))


#define PORT_CREATE_MAXIMUM_MESSAGE_LENGTH          0x148
#define PORT_CREATE_MAXIMUM_CONNECTION_INFO_SIZE    0x104

	// end_ntddk end_wdm

#if defined(USE_LPC6432)
#undef PORT_MAXIMUM_MESSAGE_LENGTH
#define PORT_MAXIMUM_MESSAGE_LENGTH 512
#endif


#ifndef InitializePortHeader

#define InitializeServerView(__ServerView, __ViewSize, __ViewBase) { \
            (__ServerView)->Length = sizeof(*(__ServerView)); \
            (__ServerView)->ViewSize = (__ViewSize); \
            (__ServerView)->ViewBase = (__ViewBase); }

#define InitializeClientView(__ClientView, __SectionHandle, __SectionOffset, __ViewSize, __ViewBase, __ViewRemoteBase) { \
            (__ClientView)->Length = sizeof(*(__ClientView)); \
            (__ClientView)->SectionHandle = (__SectionHandle); \
            (__ClientView)->SectionOffset = (__SectionOffset); \
            (__ClientView)->ViewSize = (__ViewSize); \
            (__ClientView)->ViewBase = (__ViewBase); \
            (__ClientView)->ViewRemoteBase = (__ViewRemoteBase); }

#define InitializeMessageHeader(ph, l, t) {                        \
    (ph)->TotalLength    = (USHORT) (l);                            \
    (ph)->DataLength    = (USHORT) (l - sizeof(PORT_MESSAGE));    \
    (ph)->Type            = (USHORT) (t);                            \
    (ph)->VirtualRangesOffset = 0;                                \
						}

#define InitializeMessageData(msag, view, command, dataaddr, datalen) {        \
    InitializeMessageHeader(&((msag)->Header), 256, LPC_NEW_MESSAGE)        \
    (msag)->Command        = command;                                            \
    (msag)->DataLength  = datalen;                                            \
    if ((datalen) < 224)                                                    \
						    {                                                                        \
        CopyMemory((PVOID)(msag)->MessageText,                                    \
               (PVOID)(dataaddr),                                            \
               (ULONG)(datalen));                                            \
						    }                                                                        \
    else                                                                    \
						    {                                                                        \
    CopyMemory((PVOID)(view)->ViewBase,                                        \
               (PVOID)(dataaddr),                                            \
               (ULONG)(datalen));                                            \
						    }                                                                        \
						}

#endif  // InitializePortHeader


NTSTATUS
Nt_AllocateMemory(
	HANDLE      ProcessHandle,
	PVOID*      BaseAddress,
	ULONG_PTR   Size,
	ULONG       DEF_VAL(Protect, PAGE_EXECUTE_READWRITE),
	ULONG       DEF_VAL(AllocationType, MEM_RESERVE | MEM_COMMIT)
);

NTSTATUS
Nt_FreeMemory(
	HANDLE  ProcessHandle,
	PVOID   BaseAddress
);


struct LDR_DATA_TABLE_ENTRY_CXX : public LDR_DATA_TABLE_ENTRY
{
	ForceInline _LDR_DATA_TABLE_ENTRY* NextLoadOrder()
	{
		return FIELD_BASE(this->InLoadOrderLinks.Flink, _LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
	}

	ForceInline _LDR_DATA_TABLE_ENTRY* NextMemoryOrder()
	{
		return FIELD_BASE(this->InMemoryOrderLinks.Flink, _LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
	}

	ForceInline _LDR_DATA_TABLE_ENTRY* NextInitializationOrder()
	{
		return FIELD_BASE(this->InInitializationOrderLinks.Flink, _LDR_DATA_TABLE_ENTRY, InInitializationOrderLinks);
	}

	ForceInline _LDR_DATA_TABLE_ENTRY* PrevLoadOrder()
	{
		return FIELD_BASE(this->InLoadOrderLinks.Blink, _LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
	}

	ForceInline _LDR_DATA_TABLE_ENTRY* PrevMemoryOrder()
	{
		return FIELD_BASE(this->InMemoryOrderLinks.Blink, _LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
	}

	ForceInline _LDR_DATA_TABLE_ENTRY* PrevInitializationOrder()
	{
		return FIELD_BASE(this->InInitializationOrderLinks.Blink, _LDR_DATA_TABLE_ENTRY, InInitializationOrderLinks);
	}
};


typedef const LDR_DATA_TABLE_ENTRY* PCLDR_DATA_TABLE_ENTRY;

typedef LDR_DATA_TABLE_ENTRY LDR_MODULE;
typedef PLDR_DATA_TABLE_ENTRY PLDR_MODULE;
typedef PCLDR_DATA_TABLE_ENTRY PCLDR_MODULE;


typedef struct OBJECT_TYPE_INFORMATION2 : public OBJECT_TYPE_INFORMATION
{
	OBJECT_TYPE_INFORMATION Type;
	WCHAR TypeNameBuffer[MAX_NTPATH];

} OBJECT_TYPE_INFORMATION2, *POBJECT_TYPE_INFORMATION2;


typedef struct
{
	UNICODE_STRING  Name;
	WCHAR           Buffer[MAX_NTPATH];

} OBJECT_NAME_INFORMATION2, *POBJECT_NAME_INFORMATION2;

typedef OBJECT_NAME_INFORMATION MEMORY_MAPPED_FILENAME_INFORMATION, *PMEMORY_MAPPED_FILENAME_INFORMATION;
typedef OBJECT_NAME_INFORMATION2 MEMORY_MAPPED_FILENAME_INFORMATION2;


/************************************************************************
ml C interface
************************************************************************/
PIMAGE_RESOURCE_DATA_ENTRY
Nt_FindResource(
	PVOID   Module,
	PCWSTR  Name,
	PCWSTR  Type
);

PVOID
Nt_LoadResource(
	PVOID                       Module,
	PIMAGE_RESOURCE_DATA_ENTRY  ResourceDataEntry,
	PULONG                      DEF_VAL(Size, NULL)
);


typedef
SHIM_HOOK_API_INFO*
(STDCALL
	*GetHookAPIsRoutine)(
		IN  PCWSTR ShimCommandLine,
		IN  PCWSTR CompatibilityFixName,
		OUT PULONG HookApiCount
		);

typedef
VOID
(STDCALL
	*NotifyShimsRoutine)(
		IN SHIM_NOTIFY_REASON   Reason,
		IN LDR_MODULE           *LoadedDll
		);

typedef struct
{
	WCHAR ShimEngineFullPath[MAX_PATH];
	ULONG SizeOfData;
	ULONG Magic;        // 0xAC0DEDAB

} SHIM_ENGINE_DATA, *PSHIM_ENGINE_DATA;

/*++

if return success, load LayerDll

--*/

typedef
NTSTATUS
(STDCALL
	*SE_InitializeEngine)(
		PUNICODE_STRING     LayerDll,
		PUNICODE_STRING     ExeFullPath,
		PSHIM_ENGINE_DATA   ShimData
		);


/*++

call before invoke dll EntryPoint
return value seems be ignored
must export

--*/

typedef
BOOLEAN
(STDCALL
	*SE_InstallBeforeInitRoutine)(
		PUNICODE_STRING     ModuleLoading,
		PSHIM_ENGINE_DATA   ShimData
		);


/*++

call after invoke dll EntryPoint
return FALSE will be unloaded
must export

--*/

typedef
BOOLEAN
(STDCALL
	*SE_InstallAfterInitRoutine)(
		PUNICODE_STRING     ModuleLoading,
		PSHIM_ENGINE_DATA   ShimData
		);


typedef
VOID
(STDCALL
	*SE_ShimDllLoaded)(
		PVOID DllHandle
		);

/*++

invoke before LdrpCallInitRoutine
must export

--*/

typedef
VOID
(STDCALL
	*SE_DllLoadedRoutine)(
		PLDR_DATA_TABLE_ENTRY DllLoaded
		);

/*++

invoke before NtUnmapViewOfSection and after decrement LoadCount
must export

--*/

typedef
VOID
(STDCALL
	*SE_DllUnloadedRoutine)(
		PLDR_DATA_TABLE_ENTRY DllUnLoading
		);

/*++

must export

--*/

typedef
VOID
(STDCALL
	*SE_ProcessDyingRoutine)(
		VOID
		);

typedef
VOID
(STDCALL
	*SE_LdrEntryRemovedRoutine)(
		PLDR_DATA_TABLE_ENTRY EntryRemoving
		);

/*++

call when first LdrGetProcedureAddressEx invoked

--*/

typedef
VOID
(STDCALL
	*SE_GetProcAddressLoadRoutine)(
		PLDR_DATA_TABLE_ENTRY LdrEntryLinks
		);

NATIVE_API
BOOL
NTAPI
LdrInitShimEngineDynamic(
	PVOID ShimEngineModule
);


/************************************************************************/
/* KMP                                                                  */
/************************************************************************/
inline Long_Ptr InitNext(const Void *lpKeyWord, Long_Ptr KeyWordLen, PLong_Ptr piNext)
{
	Long_Ptr i, j;
	const Char *pDest;

	pDest = (const Char *)lpKeyWord;

	piNext[0] = -1;
	i = 0;
	j = (Long_Ptr)-1;

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

inline PVoid KMP(PCVoid _Src, Long_Ptr SrcLength, PCVoid _KeyWord, Long_Ptr KeyWordLen)
{
	Long_Ptr i, j;
	PLong_Ptr  Next;//[100];
	PLong_Ptr pNext;
	const Char *pSrc, *pKeyWord;

	if (SrcLength == 0)
		return nullptr;

	Next = (PLong_Ptr)AllocStack((KeyWordLen + 1) * sizeof(*pNext));
	pNext = Next;
	pSrc = (const Char *)_Src;
	pKeyWord = (const Char *)_KeyWord;
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

	return (Void *)(pSrc + i - KeyWordLen);
}


#include <stdarg.h>

#pragma warning(disable:4127)


PChar                StringLowerA(PChar pszString, ULong DEF_VAL(Length, -1));
PWChar               StringLowerW(PWChar pszString, ULong DEF_VAL(Length, -1));
PChar                StringUpperA(PChar pszString, ULong DEF_VAL(Length, -1));
PWChar               StringUpperW(PWChar pszString, ULong DEF_VAL(Length, -1));
Long_Ptr    FASTCALL StrLengthA(PCChar pString);
Long_Ptr    FASTCALL SSE2_StrLengthA(PCChar pString);
Long_Ptr    FASTCALL StrLengthW(PCWChar pString);
PChar       FASTCALL StrCopyA(PChar DestinationString, PCChar SourceString);
PWChar      FASTCALL StrCopyW(PWChar DestinationString, PCWChar SourceString);
PChar       FASTCALL StrConcatenateA(PChar DestinationString, PCChar SourceString);
PWChar      FASTCALL StrConcatenateW(PWChar DestinationString, PCWChar SourceString);
Long_Ptr    FASTCALL StrNCompareA(PCChar pString1, PCChar pString2, SizeT LengthToCompare);

PChar   FASTCALL findexta(PCChar lpFileName);
PWChar  FASTCALL findextw(PCWChar lpFileName);
PChar   FASTCALL findnamea(PCChar lpFileName);
PWChar  FASTCALL findnamew(PCWChar lpFileName, SizeT DEF_VAL(Length, -1));
PWChar           StrFindCharW(PCWChar lpString, WChar CharMatch);
PWChar           StrFindLastCharW(PCWChar lpString, WChar CharMatch);
PWChar           PeekLineW(PVoid pBuffer, PVoid pEndOfBuffer, PVoid pOutBuffer, PSizeT pcbOutBuffer);

inline PTChar StrCopy(LPVoid DestinationString, LPCVoid SourceString)
{
	return sizeof(*(PTChar)SourceString) == sizeof(Char) ? (PTChar)StrCopyA((PChar)DestinationString, (PCChar)SourceString) : (PTChar)StrCopyW((PWChar)DestinationString, (PCWChar)SourceString);
}

inline PTChar findext(LPVoid lpFileName)
{
	return sizeof(*(PTChar)lpFileName) == sizeof(Char) ? (PTChar)findexta((PCChar)lpFileName) : (PTChar)findextw((PCWChar)lpFileName);
}

inline PTChar findname(PCTChar lpFileName)
{
	return sizeof(*(PTChar)lpFileName) == 1 ? (PTChar)findnamea((PCChar)lpFileName) : (PTChar)findnamew((PCWChar)lpFileName, ULONG_PTR_MAX);
}

inline PChar FASTCALL rmexta(PChar pszPath)
{
	*findexta(pszPath) = 0;
	return pszPath;
}

inline PWChar FASTCALL rmextw(PWChar pszPath)
{
	*findextw(pszPath) = 0;
	return pszPath;
}

inline PTChar FASTCALL rmext(PTChar pszPath)
{
	if (sizeof(*pszPath) == sizeof(Char))
		return (PTChar)rmexta((PChar)pszPath);
	else
		return (PTChar)rmextw((PWChar)pszPath);
}

inline Void FASTCALL chext(Char *szFileName, Char *pExt)
{
	StrCopyA(findexta(szFileName), pExt);
}

inline Void FASTCALL chextw(WChar *szFileName, WChar *pExt)
{
	StrCopyW(findextw(szFileName), pExt);
}

inline PChar FASTCALL rmname(PChar pszPath)
{
	*findnamea(pszPath) = 0;
	return pszPath;
}

inline PWChar FASTCALL rmnamew(PWChar pszPath)
{
	*findnamew(pszPath, ULONG_PTR_MAX) = 0;
	return pszPath;
}

inline Long_Ptr FASTCALL StrLength(PCTChar pString)
{
	if (sizeof(TChar) == sizeof(Char))
		return StrLengthA((PCChar)pString);
	else
		return StrLengthW((PCWChar)pString);
}

enum
{
	StrCmp_Keep,
	StrCmp_ToLower,
	StrCmp_ToUpper,
};

Long_Ptr FASTCALL StrNICompareA(PCChar pString1, PCChar pString2, SizeT LengthToCompare, LongPtr DEF_VAL(CaseConvertTo, StrCmp_Keep));

inline Long_Ptr FASTCALL StrICompareA(PCChar pString1, PCChar pString2, LongPtr DEF_VAL(CaseConvertTo, StrCmp_Keep))
{
	return StrNICompareA(pString1, pString2, (SizeT)~0ull, CaseConvertTo);
}

Long_Ptr FASTCALL StrNICompareW(PCWChar pString1, PCWChar pString2, SizeT LengthToCompare, LongPtr DEF_VAL(CaseConvertTo, StrCmp_Keep));

inline Long_Ptr FASTCALL StrICompareW(PCWChar pString1, PCWChar pString2, LongPtr DEF_VAL(CaseConvertTo, StrCmp_Keep))
{
	return StrNICompareW(pString1, pString2, (SizeT)-1, CaseConvertTo);
}

inline Long_Ptr FASTCALL StrNICompare(PCTChar pString1, PCTChar pString2, SizeT LengthToCompare, LongPtr DEF_VAL(CaseConvertTo, StrCmp_Keep))
{
	if (sizeof(*pString1) == sizeof(Char))
	{
		return StrNICompareA((PCChar)pString1, (PCChar)pString2, LengthToCompare, CaseConvertTo);
	}
	else if (sizeof(*pString1) == sizeof(WChar))
	{
		return StrNICompareW((PCWChar)pString1, (PCWChar)pString2, LengthToCompare, CaseConvertTo);
	}

	return 0;
}

inline Long_Ptr FASTCALL StrICompare(PCTChar pString1, PCTChar pString2)
{
	return StrNICompare(pString1, pString2, (SizeT)-1, StrCmp_Keep);
}

inline Long_Ptr FASTCALL StrCompareA(PCChar pString1, PCChar pString2)
{
	return StrNCompareA(pString1, pString2, (SizeT)-1);
}

Long_Ptr FASTCALL StrNCompareW(PCWChar pString1, PCWChar pString2, SizeT LengthToCompare);

inline Long_Ptr FASTCALL StrCompareW(PCWChar pString1, PCWChar pString2)
{
	return StrNCompareW(pString1, pString2, (SizeT)-1);
}

inline Long_Ptr FASTCALL StrNCompare(PCTChar pString1, PCTChar pString2, SizeT LengthToCompare)
{
	if (sizeof(*pString1) == sizeof(Char))
	{
		return StrNCompareA((PCChar)pString1, (PCChar)pString2, LengthToCompare);
	}
	else if (sizeof(*pString1) == sizeof(WChar))
	{
		return StrNCompareW((PCWChar)pString1, (PCWChar)pString2, LengthToCompare);
	}

	return 0;
}

inline Long_Ptr FASTCALL StrCompare(PCTChar pString1, PCTChar pString2)
{
	return StrNCompare(pString1, pString2, (SizeT)-1);
}

inline PChar SubStrA(const Char *_Src, const Char *_Sub)
{
	return (Char *)KMP(_Src, StrLengthA(_Src), _Sub, StrLengthA(_Sub));
}

inline PWChar SubStrW(const WChar *_Src, const WChar *_Sub)
{
	return (PWChar)KMP(_Src, StrLengthW(_Src) * sizeof(WChar), _Sub, StrLengthW(_Sub) * sizeof(WChar));
}



Int32 StringToInt32A(PCChar pszString);
Int32 StringToInt32W(PCWChar pszString);

inline Int32 StringToInt32(PCTChar pszString)
{
	if (sizeof(*pszString) == sizeof(Char))
		return StringToInt32A((PChar)pszString);
	else
		return StringToInt32W((PWChar)pszString);
}

Int32 StringToInt32HexW(PCWChar pszString);
Int64 StringToInt64HexW(PCWChar pszString);
Int64 StringToInt64A(PChar pszString);
Int64 StringToInt64W(PCWChar pszString);

inline Int64 StringToInt64(PTChar pszString)
{
	if (sizeof(TChar) == sizeof(Char))
		return StringToInt64A((PChar)pszString);
	else if (sizeof(TChar) == sizeof(WChar))
		return StringToInt64W((PWChar)pszString);

	return 0;
}

Bool IsShiftJISChar(SizeT uChar);
Bool IsShiftJISString(PCChar pString, SizeT Length);

inline int GetFormatedSize(PCWSTR Format, ...)
{
	return _vscwprintf(Format, (va_list)((PVOID *)&Format + 1));
}

Int FormatStringA(PChar  pszBuffer, PCChar  pszFormat, ...);
Int FormatStringW(PWChar pszBuffer, PCWChar pszFormat, ...);
Int FormatStringvA(PChar  pszBuffer, PCChar  pszFormat, va_list args);
Int FormatStringvW(PWChar pszBuffer, PCWChar pszFormat, va_list args);
Int FormatStringvnA(PChar  pszBuffer, UInt    cchLimitIn, PCChar  pszFormat, va_list args);
Int FormatStringvnW(PWChar pszBuffer, UInt    cchLimitIn, PCWChar pszFormat, va_list args);

// reactos\dll\win32\shlwapi\wsprintf.c

#define FMTSTR_FLAG_LEFTALIGN   0x00000001  /* Align output on the left ('-' prefix) */
#define FMTSTR_FLAG_PREFIX_HEX  0x00000002  /* Prefix hex with 0x ('#' prefix) */
#define FMTSTR_FLAG_ZEROPAD     0x00000004  /* Pad with zeros ('0' prefix) */
#define FMTSTR_FLAG_LONG        0x00000008  /* Long arg ('l' prefix) */
#define FMTSTR_FLAG_SHORT       0x00000010  /* Short arg ('h' prefix) */
#define FMTSTR_FLAG_UPPER_HEX   0x00000020  /* Upper-case hex ('X' specifier) */
#define FMTSTR_FLAG_WIDE        0x00000040  /* Wide arg ('w' prefix) */
#define FMTSTR_FLAG_FORCESIGNED 0x00000080  /* Force signed ('+' prefix) */
#define FMTSTR_FLAG_I64         0x00000100  /* 64 bit version ("%I64" prefix) */

typedef enum
{
	FMTSTR_FORMAT_UNKNOWN,
	FMTSTR_FORMAT_CHAR,
	FMTSTR_FORMAT_WCHAR,
	FMTSTR_FORMAT_STRING,
	FMTSTR_FORMAT_WSTRING,
	FMTSTR_FORMAT_SIGNED,
	FMTSTR_FORMAT_UNSIGNED,
	FMTSTR_FORMAT_HEXA,
	FMTSTR_FORMAT_FLOAT,

#if MY_OS_WIN32
	FMTSTR_FORMAT_BINARY,
#endif
} FMTSTR_TYPE;

typedef struct
{
	UInt        flags;
	UInt        width;
	UInt        precision;
	FMTSTR_TYPE type;
} FMTSTR_FORMAT;

typedef union
{
	WChar   wchar_view;
	Char    char_view;
	PCChar  lpcstr_view;
	PCWChar lpcwstr_view;
	Int     int_view;
	Int64   int64_view;
} FMTSTR_DATA;

Bool
StrMatchExpression(
	IN PWChar   Expression,
	IN PWChar   Name,
	IN Bool     DEF_VAL(IgnoreCase, TRUE),
	IN PWChar   DEF_VAL(UpcaseTable, NULL) OPTIONAL
);


#pragma warning(default:4127)


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


inline
NTSTATUS
RtlSetUnicodeString(
	PUNICODE_STRING UnicodeString,
	PCWSTR          String,
	ULONG_PTR       DEF_VAL(Length, -1)
)
{
	if (UnicodeString == nullptr)
		return STATUS_INVALID_PARAMETER;

	if (String == nullptr)
	{
		RtlFreeUnicodeString(UnicodeString);
		return STATUS_SUCCESS;
	}

	if (UnicodeString->Buffer == nullptr)
	{
		return RtlCreateUnicodeString(UnicodeString, String) ? STATUS_SUCCESS : STATUS_NO_MEMORY;
	}

	if (String == nullptr)
	{
		((PWSTR)UnicodeString->Buffer)[0] = 0;
		UnicodeString->Length = 0;

		return STATUS_SUCCESS;
	}

	if (Length == -1)
		Length = StrLengthW(String) * sizeof(WCHAR);

	if (Length < UnicodeString->MaximumLength)
	{
		UnicodeString->Length = (USHORT)Length;
		CopyMemory((PWSTR)UnicodeString->Buffer, String, Length);
		((PWSTR)UnicodeString->Buffer)[Length / sizeof(WCHAR)] = 0;

		return STATUS_SUCCESS;
	}

	RtlFreeUnicodeString(UnicodeString);
	return RtlCreateUnicodeString(UnicodeString, String) ? STATUS_SUCCESS : STATUS_NO_MEMORY;
}


#define WIN32K_API NATIVE_API

#if ML_KERNEL_MODE

DECLARE_HANDLE(HWND);

#endif // r0

#if !ML_KERNEL_MODE

/************************************************************************
user32 undoc api
************************************************************************/

#define RST_DONTATTACHQUEUE     0x00000001
#define RST_DONTJOURNALATTACH   0x00000002

WIN32K_API
VOID
NTAPI
RegisterSystemThread(
	ULONG Flags,
	ULONG Reserve       // must be zero
);

#define SNDMSG_FLAG_ANSICALL    0x00000001

inline
LRESULT
NTAPI
SendMessageWorker(
	HWND    Window,
	UINT    Message,
	WPARAM  wParam,
	LPARAM  lParam,
	ULONG   Flags
)
{
	return 0;
}

inline
HWND
NTAPI
NtUserCreateWindowEx_Win7(
	ULONG                   ExStyle,    // 0x80000000 == CreateWindowExA
	PLARGE_UNICODE_STRING   ClassName,
	PLARGE_UNICODE_STRING   ClassVersion,
	PLARGE_UNICODE_STRING   WindowName,
	ULONG                   Style,
	LONG                    X,
	LONG                    Y,
	LONG                    Width,
	LONG                    Height,
	HWND                    ParentWnd,
	HMENU                   Menu,
	PVOID                   Instance,
	LPVOID                  Param,
	ULONG                   ShowMode,
	ULONG                   Unknown
)
{
	return 0;
}

inline
HWND
NTAPI
NtUserCreateWindowEx_Win8(
	ULONG                   ExStyle,    // 0x80000000 == CreateWindowExA
	PLARGE_UNICODE_STRING   ClassName,
	PLARGE_UNICODE_STRING   ClassVersion,
	PLARGE_UNICODE_STRING   WindowName,
	ULONG                   Style,
	LONG                    X,
	LONG                    Y,
	LONG                    Width,
	LONG                    Height,
	HWND                    ParentWnd,
	HMENU                   Menu,
	PVOID                   Instance,
	LPVOID                  Param,
	ULONG                   ShowMode,
	ULONG                   Unknown,
	ULONG                   Unknown2
)
{
	return 0;
}

inline
LRESULT
NTAPI
NtUserMessageCall(
	IN HWND         hWnd,
	IN UINT         Message,
	IN WPARAM       wParam,
	IN LPARAM       lParam,
	IN ULONG_PTR    xParam,
	IN DWORD        xpfnProc,
	IN BOOL         Ansi
)
{
	return 0;
}

inline
BOOL
NTAPI
NtUserDefSetText(
	HWND                    hWnd,
	PLARGE_UNICODE_STRING   Text
)
{
	return 0;
}

#endif // !ML_KERNEL_MODE

typedef enum _WINDOWINFOCLASS
{
	WindowProcess = 0,
	WindowThread = 2,
	WindowIsHung = 5,

} WINDOWINFOCLASS;

WIN32K_API
HANDLE
NTAPI
NtUserQueryWindow(
	IN HWND             hWnd,
	IN WINDOWINFOCLASS  WindowInfo
);



// Resources


NATIVE_API
USHORT
NTAPI
RtlCaptureStackBackTrace(
	IN  ULONG   FramesToSkip,
	IN  ULONG   FramesToCapture,
	OUT PVOID*  BackTrace,
	OUT PULONG  BackTraceHash OPTIONAL
);



_ML_C_HEAD_

inline
ULONG
NTAPI
NtGdiGetGlyphOutline(
	IN  HDC             hDc,
	IN  WCHAR           UnicodeChar,
	IN  ULONG           Format,
	OUT LPGLYPHMETRICS  GlyphMetrics,
	IN  ULONG           BufferSize,
	OUT PVOID           Buffer,
	IN  LPMAT2          Mat2,
	IN  BOOL            IgnoreRotation
)
{
	return 0;
}

inline
HFONT
NTAPI
NtGdiHfontCreate(
	IN  PENUMLOGFONTEXDVW   EnumLogFont,
	IN  ULONG               SizeOfEnumLogFont,
	IN  LONG                LogFontType,
	IN  LONG                Unknown,
	IN  PVOID               FreeListLocalFont
)
{
	return 0;
}


#if ML_USER_MODE

typedef struct
{
	ULONG SizeOfSelf;
	ULONG Unknown1;
	ULONG Unknown2;
	ULONG FontHandle;
	ULONG FontCount;
	ULONG Unknown5;

} FONT_REALIZATION_INFO, *PFONT_REALIZATION_INFO;

NATIVE_API
BOOL
NTAPI
GetFontRealizationInfo(
	HDC DC,
	PFONT_REALIZATION_INFO RealizationInfo
);

typedef struct
{
	ULONG Reserved[4];
	WCHAR FontFile[MAX_NTPATH];

} FONT_FILE_INFO, *PFONT_FILE_INFO;

NATIVE_API
BOOL
NTAPI
GetFontFileInfo(
	ULONG           FontHandle,
	ULONG           FontIndex,
	PFONT_FILE_INFO FontFileInfo,
	ULONG           InfoLength,
	PULONG          ReturnedLength
);

#endif // r3

_ML_C_TAIL_


#if ML_KERNEL_MODE

#define ES_SYSTEM_REQUIRED      ((ULONG)0x00000001)
#define ES_DISPLAY_REQUIRED     ((ULONG)0x00000002)
#define ES_USER_PRESENT         ((ULONG)0x00000004)
#define ES_AWAYMODE_REQUIRED    ((ULONG)0x00000004)
#define ES_CONTINUOUS           ((ULONG)0x80000000)

typedef ULONG EXECUTION_STATE;

#endif // r0

// Font Management Services


#if ML_USER_MODE

DECLARE_HANDLE(FMS_ENUMERATOR);

typedef FMS_ENUMERATOR *PFMS_ENUMERATOR;


ML_NAMESPACE_BEGIN(FmsFilterType);

enum
{
	FaceName = 0x02,
	CharSet = 0x20,
};

ML_NAMESPACE_END_(FmsFilterType);

typedef struct
{
	ULONG   Unknown1;
	ULONG   FilterType;

	union
	{
		WCHAR   FaceName[1000];
		BYTE    Charset;
	};

} FMS_FILTER_DATA, *PFMS_FILTER_DATA;

typedef struct
{
	TEXTMETRICW TextMetric;
	ULONG       Unknown[10];

} FMS_TEXTMETRIC, *PFMS_TEXTMETRIC;

ML_NAMESPACE_BEGIN(FmsFontStyle);

enum
{
	Latin1 = 0,
	Latin2 = 1,
	Cyrillic = 2,
	Thai = 3,
	Jisjapan = 4,
	Chinesesimp = 5,
	Latin2_2 = 6,
};

ML_NAMESPACE_END_(FmsFontStyle);

ML_NAMESPACE_BEGIN(FmsPropertyType);

enum
{
	FontTypeName = 0x00,
	FontStyle = 0x01,
	FaceNameLocale = 0x02,
	FaceNameAnsi = 0x03,
	FullNameLocale = 0x04,
	FullNameAnsi = 0x05,
	FontWeightName = 0x06,
	FontStyleNameLocale = 0x09,
	FontStyleNameAnsi = 0x0A,
	Company = 0x13,
	FontFileName = 0x17,
	FontDirectory = 0x18,
	ScriptName = 0x1F,
	FullDescription = 0x16,
	SymbolInfo = 0x25,

	PropertyMax = 0x25,
};

ML_NAMESPACE_END_(FmsPropertyType);


ML_NAMESPACE_BEGIN(FmsInitializeFlags);

enum
{
	NoSimulations = 0x00000001,
	NoVertFonts = 0x00000004,
};

ML_NAMESPACE_END_(FmsInitializeFlags);

NATIVE_API
NTSTATUS
NTAPI
FmsInitializeEnumerator(
	IN  PFMS_ENUMERATOR Enumerator,
	IN  ULONG           Flags = FmsInitializeFlags::NoSimulations | FmsInitializeFlags::NoVertFonts
);

NATIVE_API
NTSTATUS
NTAPI
FmsResetEnumerator(
	IN FMS_ENUMERATOR Enumerator
);

NATIVE_API
NTSTATUS
NTAPI
FmsFreeEnumerator(
	IN OUT PFMS_ENUMERATOR Enumerator
);

NATIVE_API
NTSTATUS
NTAPI
FmsSetDefaultFilter(
	IN  FMS_ENUMERATOR Enumerator
);

NATIVE_API
NTSTATUS
NTAPI
FmsSetFilter(
	IN  FMS_ENUMERATOR      Enumerator,
	IN  PFMS_FILTER_DATA    FilterData,
	IN  ULONG               NumberOfFilterData
);

NATIVE_API
NTSTATUS
NTAPI
FmsAddFilter(
	IN  FMS_ENUMERATOR      Enumerator,
	IN  PFMS_FILTER_DATA    FilterData,
	IN  ULONG               NumberOfFilterData
);

NATIVE_API
NTSTATUS
NTAPI
FmsGetFilteredPropertyList(
	IN      FMS_ENUMERATOR  Enumerator,
	IN      ULONG           PropertyType,
	OUT     PULONG          NumberofProperty,
	IN OUT  PULONG          PropertySize,
	OUT     PVOID           Property    OPTIONAL
);

NATIVE_API
NTSTATUS
NTAPI
FmsGetFilteredFontList(
	IN  FMS_ENUMERATOR  Enumerator,
	OUT PULONG          NumberOfFonts,
	OUT PULONG          FontIdList OPTIONAL
);

NATIVE_API
NTSTATUS
NTAPI
FmsGetBestMatchInFamily(
	IN  FMS_ENUMERATOR  Enumerator,
	IN  ULONG           ReservedZero,
	IN  PCWSTR          FaceName,
	OUT PULONG          FontId
);

NATIVE_API
NTSTATUS
NTAPI
FmsGetGDILogFont(
	IN  FMS_ENUMERATOR      Enumerator,
	IN  ULONG               FontId,
	IN  BOOLEAN             WhatTrue,
	OUT LPENUMLOGFONTEXW    EnumLogFontEx,
	OUT PFMS_TEXTMETRIC     TextMetric OPTIONAL
);

NATIVE_API
NTSTATUS
NTAPI
FmsGetGdiLogicalFont(
	IN  FMS_ENUMERATOR      Enumerator,
	IN  ULONG               FontId,
	IN  BOOLEAN             WhatTrue,
	OUT LPENUMLOGFONTEXW    EnumLogFontEx,
	OUT PFMS_TEXTMETRIC     FmsTextMetric  OPTIONAL,
	OUT PULONG              SomeBuffer  OPTIONAL
);

NATIVE_API
NTSTATUS
NTAPI
FmsMapGdiLogicalFont(
	IN  FMS_ENUMERATOR      Enumerator,
	IN  ULONG               FontId,
	OUT LPENUMLOGFONTEXW    EnumLogFontEx
);

NATIVE_API
NTSTATUS
NTAPI
FmsGetFontProperty(
	IN      FMS_ENUMERATOR  Enumerator,
	IN      ULONG           FontId,
	IN      ULONG           PropertyType,
	IN OUT  PULONG          PropertySize,
	IN OUT  PVOID           PropertyBuffer
);

#endif // r3



#if !MY_NT_DDK


_ML_C_HEAD_

EXTC_IMPORT HANDLE WINAPI GetConsoleInputWaitHandle();

EXTC_IMPORT
BOOL
WINAPI
CreateProcessInternalA(
	HANDLE                  Token,
	LPCSTR                  ApplicationName,
	LPSTR                   CommandLine,
	LPSECURITY_ATTRIBUTES   ProcessAttributes,
	LPSECURITY_ATTRIBUTES   ThreadAttributes,
	BOOL                    InheritHandles,
	DWORD                   CreationFlags,
	LPVOID                  Environment,
	LPCSTR                  CurrentDirectory,
	LPSTARTUPINFOA          StartupInfo,
	LPPROCESS_INFORMATION   ProcessInformation,
	PHANDLE                 NewToken
);


typedef struct
{
	HANDLE                  Token;
	PCWSTR                  ApplicationName;
	PCWSTR                  CommandLine;
	LPSECURITY_ATTRIBUTES   ProcessAttributes;
	LPSECURITY_ATTRIBUTES   ThreadAttributes;
	BOOL                    InheritHandles;
	DWORD                   CreationFlags;
	LPVOID                  Environment;
	PCWSTR                  CurrentDirectory;
	LPSTARTUPINFOW          StartupInfo;
	LPPROCESS_INFORMATION   ProcessInformation;
	PHANDLE                 NewToken;

} CREATE_PROCESS_INTERNAL_PARAM, *PCREATE_PROCESS_INTERNAL_PARAM;

typedef BOOL(WINAPI *CreateProcessInternal2Routine)(CREATE_PROCESS_INTERNAL_PARAM Parameter);


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

EXTC_IMPORT
INT
WINAPI
MessageBoxTimeoutA(
	IN HWND     hWnd,
	IN LPCSTR   Text,
	IN LPCSTR   Caption,
	IN UINT     Type,
	IN WORD     LanguageId,
	IN DWORD    Milliseconds
);

EXTC_IMPORT
INT
STDCALL
MessageBoxTimeoutW(
	IN HWND     hWnd,
	IN LPCWSTR  Text,
	IN LPCWSTR  Caption,
	IN UINT     Type,
	IN WORD     anguageId,
	IN DWORD    Milliseconds
);

#if MY_UNICODE_ENABLE
#define MessageBoxTimeout MessageBoxTimeoutW
#else
#define MessageBoxTimeout MessageBoxTimeoutA
#endif

_ML_C_TAIL_

#endif // MY_NT_DDK

_ML_C_HEAD_


/************************************************************************
strings
************************************************************************/

NTSTATUS
FASTCALL
Nt_AnsiToUnicode(
	PWSTR       UnicodeBuffer,
	ULONG_PTR   BufferCount,
	PCSTR       AnsiString,
	LONG_PTR    DEF_VAL(AnsiLength, -1),
	PULONG_PTR  DEF_VAL(BytesInUnicode, NULL)
);

NTSTATUS
Nt_UnicodeToAnsi(
	PCHAR       AnsiBuffer,
	ULONG_PTR   BufferCount,
	LPCWSTR     UnicodeString,
	LONG_PTR    DEF_VAL(UnicodeLength, -1),
	PULONG_PTR  DEF_VAL(BytesInAnsi, NULL)
);

NTSTATUS
Nt_AnsiToUnicodeString(
	PUNICODE_STRING Unicode,
	PCHAR           AnsiString,
	LONG_PTR        DEF_VAL(AnsiLength, -1),
	BOOL            DEF_VAL(AllocateDestinationString, TRUE)
);

NTSTATUS
Nt_UnicodeToAnsiString(
	PANSI_STRING    Ansi,
	LPCWSTR         UnicodeString,
	LONG_PTR        DEF_VAL(UnicodeLength, -1),
	BOOL            DEF_VAL(AllocateDestinationString, TRUE)
);

BOOL Nt_IsPathExists(LPCWSTR pszPath);

LDR_MODULE*
Nt_FindLdrModuleByName(
	PUNICODE_STRING ModuleName
);

LDR_MODULE*
Nt_FindLdrModuleByHandle(
	PVOID BaseAddress
);

PTEB_ACTIVE_FRAME
Nt_FindThreadFrameByContext(
	ULONG_PTR Context
);

ForceInline
ANSI_STRING
CharToAnsiString(
	PSTR  Buffer,
	ULONG Length
)
{
	ANSI_STRING Ansi;

	Ansi.Buffer = Buffer;
	Ansi.Length = (USHORT)(Length * sizeof(*Buffer));
	Ansi.MaximumLength = Ansi.Length;

	return Ansi;
}

ForceInline
UNICODE_STRING
WCharToUnicodeString(
	PCWSTR Buffer,
	ULONG Length
)
{
	UNICODE_STRING Unicode;

	Unicode.Buffer = (PWSTR)Buffer;
	Unicode.Length = (USHORT)(Length * sizeof(*Buffer));
	Unicode.MaximumLength = Unicode.Length;

	return Unicode;
}

ForceInline
UNICODE_STRING64
WCharToUnicodeString64(
	PWSTR Buffer,
	ULONG Length
)
{
	UNICODE_STRING64 Unicode;

	Unicode.Buffer = (ULONGLONG)Buffer;
	Unicode.Length = (USHORT)(Length * sizeof(*Buffer));
	Unicode.MaximumLength = Unicode.Length;

	return Unicode;
}

#pragma warning(disable:4238)

#define EMPTYUS         WCharToUnicodeString((PWSTR)NULL, 0)
#define USTR(_str)      WCharToUnicodeString(_str, CONST_STRLEN((_str)))
#define IUSTR(x)        {CONST_STRLEN(x) * sizeof(x[0]), CONST_STRLEN(x) * sizeof(x[0]), x}
#define VUSTR(_str)      ((*(volatile UNICODE_STRING *)&WCharToUnicodeString(_str, CONST_STRLEN((_str)))))
#define PUSTR(_str)     (PUNICODE_STRING)&VUSTR(_str)
#define USTR64(_str)    WCharToUnicodeString64(_str, CONST_STRLEN((_str)))
#define ASTR(_str)      CharToAnsiString(_str, CONST_STRLEN((_str)))

#define WCS2US          USTR
#define CS2AS           ASTR

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

unsigned __int64 __readfsqword(unsigned long Offset);

inline ULONG_PTR ReadFsPtr(ULONG_PTR Offset)
{
#if ML_AMD64
	return (ULONG_PTR)__readgsqword((ULONG)Offset);
#elif ML_X86
	return (ULONG_PTR)__readfsdword(Offset);
#elif ML_ARM64
	return __getReg(18) + Offset;
#elif ML_ARM
	return _MoveFromCoprocessor(15, 0, 13, 0, 2) + Offset;
#elif ML_IA64
	return (size_t)_rdteb() + Offset;
#elif ML_ALPHA
	return (size_t)_rdteb() + Offset;
#elif ML_MIPS
	return (*(size_t*)(0x7ffff030)) + Offset;
#elif ML_PPC
	return __gregister_get(13) + Offset;
#else
	#error "Unsupported arch"
#endif
}

#pragma warning(disable:4733)

inline VOID WriteFsPtr(ULONG_PTR Offset, ULONG_PTR Data)
{
#if ML_AMD64
	__writegsqword((ULONG)Offset, Data);
#elif ML_X86
	__writefsdword(Offset, Data);
#elif ML_ARM
	*(PULONG_PTR)(_MoveFromCoprocessor(15, 0, 13, 0, 2) + Offset) = Data;
#elif ML_ARM64
	*(PULONG_PTR)(__getReg(18) + Offset) = Data;
#elif ML_IA64
	*(PULONG_PTR)((size_t)_rdteb() + Offset) = Data;
#elif ML_ALPHA
	*(PULONG_PTR)((size_t)_rdteb() + Offset) = Data;
#elif ML_MIPS
	*(PULONG_PTR)(*(size_t*)(0x7ffff030) + Offset) = Data;
#elif ML_PPC
	*(PULONG_PTR)(__gregister_get(13) + Offset) = Data;
#else
	#error "Unsupported arch"
#endif
}

#pragma warning(default:4733)

#define TEB_OFFSET FIELD_OFFSET(TEB, NtTib.Self)
#define PEB_OFFSET FIELD_OFFSET(TEB, ProcessEnvironmentBlock)

ForceInline PTEB Nt_CurrentTeb()
{
#if ML_AMD64
	return (PTEB)ReadFsPtr(TEB_OFFSET);
#else

#ifdef NtCurrentTeb
	return (PTEB)NtCurrentTeb();
#else
	return (PTEB)ReadFsPtr(TEB_OFFSET);
#endif // no macro

#endif
}

#if ML_KERNEL_MODE
inline
#else
ForceInline
#endif
PPEB Nt_CurrentPeb()
{

#if ML_KERNEL_MODE

	NTSTATUS    Status;
	PROCESS_BASIC_INFORMATION BasicInfo;

	Status = ZwQueryInformationProcess(NtCurrentProcess(), ProcessBasicInformation, &BasicInfo, sizeof(BasicInfo), NULL);
	if (!NT_SUCCESS(Status))
		return NULL;

	return (PPEB_BASE)BasicInfo.PebBaseAddress;

#else // r3

	return (PPEB)(ULONG_PTR)ReadFsPtr(PEB_OFFSET);

#endif // rx
}

ForceInline HANDLE RtlGetProcessHeap()
{
	return Nt_CurrentTeb()->ProcessEnvironmentBlock->ProcessHeap;
}

ForceInline PUSHORT Nt_GetDefaultCodePageBase()
{
	return (PUSHORT)Nt_CurrentPeb()->AnsiCodePageData;
}

inline LDR_MODULE* GetKernel32Ldr()
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

ForceInline LDR_MODULE* GetNtdllLdrModule()
{
	return FIELD_BASE(Nt_CurrentPeb()->Ldr->InInitializationOrderModuleList.Flink, LDR_MODULE, InInitializationOrderLinks);
}

ForceInline PVOID GetNtdllHandle()
{
	return GetNtdllLdrModule()->DllBase;
}

inline HANDLE Nt_GetCurrentDirectoryHandle()
{
	return Nt_CurrentPeb()->ProcessParameters->CurrentDirectory.Handle;
}

/************************************************************************
common file io
************************************************************************/

NTSTATUS
Nt_DeleteFile(
	PCWSTR FileName
);

/************************************************************************
common path
************************************************************************/

ULONG_PTR
Nt_QueryDosDevice(
	PCWSTR      DeviceName,
	PWSTR       TargetPath,
	ULONG_PTR   PathSize
);


/************************************************************************
common query api
************************************************************************/

#define FOR_EACH_PROCESS_INFO(_Info) for (ULONG_PTR NextOffset = ULONG_PTR_MAX; NextOffset != 0; NextOffset = (_Info)->NextEntryOffset, (_Info) = PtrAdd((_Info), NextOffset))
#define FOR_EACH_PROCESS_THREAD(_Process, _Thread) for (ULONG_PTR ThreadCount = (((_Thread) = (_Process)->Threads), (_Process)->NumberOfThreads); ThreadCount; ++(_Thread), --ThreadCount)

POBJECT_TYPES_INFORMATION       QuerySystemObjectTypes();
PSYSTEM_HANDLE_INFORMATION_EX   QuerySystemHandles();

BOOL ReleaseSystemInformation(PVOID Processes);

HANDLE QueryCsrssProcessId(ULONG_PTR DEF_VAL(Session, 1));

/************************************************************************
common callback
************************************************************************/

typedef
VOID
(NTAPI
	*PCREATE_PROCESS_NOTIFY)(
		PVOID       Context,
		ULONG_PTR   ProcessId,
		BOOL        Create
		);

/************************************************************************
common process
************************************************************************/

NTSTATUS
Nt_GetProcessUserName(
	PUNICODE_STRING UserName,
	HANDLE          DEF_VAL(Process, NtCurrentProcess())
);

/************************************************************************
common token
************************************************************************/

NTSTATUS
Nt_GetTokenInfo(
	HANDLE                  TokenHandle,
	TOKEN_INFORMATION_CLASS TokenInformationClass,
	PVOID                   Information,
	PULONG_PTR              DEF_VAL(Size, NULL)
);

VOID
ReleaseTokenInfo(
	PVOID TokenInfo
);

NTSTATUS
Nt_SetProcessThreadToken(
	HANDLE Process,
	HANDLE Thread,
	HANDLE Token
);

ULONG_PTR
Nt_GetSessionId(
	HANDLE DEF_VAL(Process, NtCurrentProcess())
);

NTSTATUS
Nt_AdjustPrivilege(
	ULONG_PTR   Privilege,
	BOOL        DEF_VAL(Enable, TRUE),
	BOOL        DEF_VAL(CurrentThread, FALSE)
);

#if ML_KERNEL_MODE

LDR_MODULE*
LookupPsLoadedModuleList(
	LDR_MODULE  *LdrModule,
	PVOID       CallDriverEntry
);

NTSTATUS
QuerySystemModuleByHandle(
	PVOID                           ImageBase,
	PRTL_PROCESS_MODULE_INFORMATION Module
);

NTSTATUS
QueryModuleNameByHandle(
	PVOID           ImageBase,
	PUNICODE_STRING ModuleName
);

NTSTATUS
Nt_WaitForDebugEvent(
	PDBGUI_WAIT_STATE_CHANGE WaitState,
	ULONG Timeout,
	HANDLE DebugObject
);

NTSTATUS
KiQueueUserApc(
	PETHREAD            Thread,
	PKNORMAL_ROUTINE    ApcRoutine,
	PVOID               DEF_VAL(ApcRoutineContext, NULL),
	PVOID               DEF_VAL(Argument1, NULL),
	PVOID               DEF_VAL(Argument2, NULL)
);

#else   // r3

/************************************************************************
user mode
************************************************************************/

/************************************************************************
debug api
************************************************************************/

NTSTATUS Nt_DebugActiveProcess(ULONG_PTR ProcessId);
NTSTATUS Nt_DebugActiveProcessStop(ULONG_PTR ProcessId);

NTSTATUS
Nt_WaitForDebugEvent(
	PDBGUI_WAIT_STATE_CHANGE WaitState,
	ULONG Timeout,
	HANDLE DEF_VAL(DebugObject, DbgUiGetThreadDebugObject())
);


/************************************************************************
process api
************************************************************************/

BOOL
Nt_IsWow64Process(
	HANDLE DEF_VAL(Process, NtCurrentProcess())
);

ULONG Nt_GetErrorMode();
ULONG Nt_SetErrorMode(ULONG Mode);

/************************************************************************
memory
************************************************************************/

NTSTATUS
Nt_ProtectMemory(
	HANDLE      ProcessHandle,
	PVOID       BaseAddress,
	ULONG_PTR   Size,
	ULONG       NewProtect,
	PULONG      OldProtect
);

NTSTATUS
Nt_ReadMemory(
	HANDLE      ProcessHandle,
	PVOID       BaseAddress,
	PVOID       Buffer,
	ULONG_PTR   Size,
	PULONG_PTR  DEF_VAL(BytesRead, NULL)
);

NTSTATUS
Nt_WriteMemory(
	HANDLE      ProcessHandle,
	PVOID       BaseAddress,
	PVOID       Buffer,
	ULONG_PTR   Size,
	PULONG_PTR  DEF_VAL(BytesWritten, NULL)
);

NTSTATUS
Nt_WriteProtectMemory(
	HANDLE      ProcessHandle,
	PVOID       BaseAddress,
	PVOID       Buffer,
	ULONG_PTR   Size,
	PULONG_PTR  DEF_VAL(BytesWritten, NULL)
);


/************************************************************************
file io
************************************************************************/

NTSTATUS
Nt_MoveFile(
	PCWSTR ExistingFileName,
	PCWSTR NewFileName
);


NTSTATUS
Nt_CopyFile(
	PCWSTR  ExistingFileName,
	PCWSTR  NewFileName,
	BOOL    DEF_VAL(FailIfExists, FALSE)
);

ULONG_PTR
Nt_GetDosPathFromNtDeviceName(
	PWSTR           DosPath,
	ULONG_PTR       DosPathLength,
	PUNICODE_STRING NtDeviceName
);

NTSTATUS
Nt_GetDosLetterFromVolumeDeviceName(
	PUNICODE_STRING DosLetter,
	PUNICODE_STRING NtDeviceName
);

NTSTATUS
Nt_GetDosPathFromHandle(
	PUNICODE_STRING DosPath,
	HANDLE          FileHandle
);

NTSTATUS
Nt_NtPathNameToDosPathName(
	PUNICODE_STRING DosPath,
	PUNICODE_STRING NtPath
);

ULONG
Nt_GetFileAttributes(
	LPCWSTR FileName
);

PVOID
Nt_GetModuleHandle(
	LPCWSTR lpModuleName
);

ULONG_PTR
Nt_GetModulePath(
	PVOID       ModuleBase,
	PWSTR       Path,
	ULONG_PTR   BufferCount
);

ULONG_PTR
Nt_GetModuleFileName(
	PVOID       ModuleBase,
	LPWSTR      lpFilename,
	ULONG_PTR   nSize
);

BOOL Nt_SetExeDirectoryAsCurrent();

BOOL
Nt_FindNextFile(
	HANDLE              FindFileHandle,
	PWIN32_FIND_DATAW   FindFileData
);

BOOL
Nt_FindClose(
	HANDLE FindFileHandle
);

HANDLE
Nt_FindFirstFile(
	LPCWSTR             FileName,
	PWIN32_FIND_DATAW   FindFileData
);

/************************************************************************
registry api
************************************************************************/

NTSTATUS
Nt_OpenPredefinedKeyHandle(
	PHANDLE     KeyHandle,
	HKEY        PredefinedKey,
	ACCESS_MASK DEF_VAL(DesiredAccess, KEY_ALL_ACCESS)
);

NTSTATUS
Nt_RegGetValue(
	HANDLE                      hKey,
	PCWSTR                      SubKey,
	PCWSTR                      ValueName,
	KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
	PVOID                       KeyValueInformation,
	ULONG                       Length,
	PULONG                      DEF_VAL(ResultLength, NULL),
	ULONG                       DEF_VAL(Flags, 0)
);

NTSTATUS
Nt_RegSetValue(
	HANDLE  hKey,
	PCWSTR  SubKey,
	PCWSTR  ValueName,
	ULONG   ValueType,
	LPCVOID ValueData,
	DWORD   ValueDataLength
);

NTSTATUS
Nt_RegDeleteValue(
	HANDLE  hKey,
	PCWSTR  SubKey,
	PCWSTR  ValueName
);


/************************************************************************
dll api
************************************************************************/

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

/************************************************************************
process api
************************************************************************/

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

NTSTATUS
Nt_CreateProcess2(
	CreateProcessInternal2Routine   Routine,
	HANDLE                          Token,
	PCWSTR                          ApplicationName,
	PWSTR                           DEF_VAL(CommandLine, NULL),
	PCWSTR                          DEF_VAL(CurrentDirectory, NULL),
	ULONG                           DEF_VAL(CreationFlags, 0),
	LPSTARTUPINFOW                  DEF_VAL(StartupInfo, NULL),
	LPPROCESS_INFORMATION           DEF_VAL(ProcessInformation, NULL),
	LPSECURITY_ATTRIBUTES           DEF_VAL(ProcessAttributes, NULL),
	LPSECURITY_ATTRIBUTES           DEF_VAL(ThreadAttributes, NULL),
	PVOID                           DEF_VAL(Environment, NULL)
);

ULONG
Nt_WaitForSingleObjectEx(
	IN HANDLE hHandle,
	IN ULONG  Milliseconds,
	IN BOOL   bAlertable
);

VOID MessageBoxErrorW(HWND hWnd, NTSTATUS Status, LPWSTR lpCaption, UINT uType);
VOID PrintError(NTSTATUS Status);
VOID Nt_Sleep(ULONG_PTR Milliseconds, BOOL DEF_VAL(Alertable, FALSE));
BOOL Nt_TerminateThread(IN HANDLE hThread, IN ULONG dwExitCode);

ForceInline PWCHAR Nt_GetCommandLine()
{
	return (PWSTR)Nt_CurrentPeb()->ProcessParameters->CommandLine.Buffer;
}

ForceInline PVOID Nt_GetExeModuleHandle()
{
	return (PVOID)Nt_CurrentPeb()->ImageBaseAddress;
}

ForceInline ULONG_PTR Nt_GetCurrentProcessId()
{
	return (ULONG_PTR)Nt_CurrentTeb()->ClientId.UniqueProcess;
}

ForceInline ULONG_PTR Nt_GetCurrentThreadId()
{
	return (ULONG_PTR)Nt_CurrentTeb()->ClientId.UniqueThread;
}

ULONG
Nt_GetSystemDirectory64(
	PWSTR Buffer,
	ULONG BufferCount
);

ULONG
Nt_GetWindowsDirectory(
	PWSTR Buffer,
	ULONG BufferCount
);

ULONG
Nt_GetSystemDirectory(
	PWSTR Buffer,
	ULONG BufferCount
);

ULONG
Nt_GetExeDirectory(
	PWSTR Path,
	ULONG BufferCount
);

inline
ULONG
Nt_GetCurrentDirectory(
	ULONG BufferCount,
	PWSTR Buffer
)
{
	return RtlGetCurrentDirectory_U(BufferCount * sizeof(WCHAR), Buffer) / sizeof(WCHAR);
}

inline
BOOL
Nt_SetCurrentDirectory(
	PCWSTR PathName
)
{
	UNICODE_STRING Path;

	RtlInitUnicodeString(&Path, PathName);
	return NT_SUCCESS(RtlSetCurrentDirectory_U(&Path));
}

VOID
Nt_ExitProcess(
	ULONG ExitCode
);

inline
VOID
SetLastNTError(
	NTSTATUS Status
)
{
	RtlSetLastWin32ErrorAndNtStatusFromNtStatus(Status);
}

ForceInline
VOID
BaseSetLastNTError(
	NTSTATUS Status
)
{
	SetLastNTError(Status);
}

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
Nt_SetThreadPriority(
	HANDLE  ThreadHandle,
	LONG    Priority
);


PLIST_ENTRY
ExInterlockedInsertHeadList(
	PLIST_ENTRY         ListHead,
	PLIST_ENTRY         Entry,
	PCRITICAL_SECTION   Lock
);

PLIST_ENTRY
ExInterlockedInsertTailList(
	PLIST_ENTRY         ListHead,
	PLIST_ENTRY         Entry,
	PCRITICAL_SECTION   Lock
);

PLIST_ENTRY
ExInterlockedRemoveHeadList(
	PLIST_ENTRY         ListHead,
	PCRITICAL_SECTION   Lock
);

PSINGLE_LIST_ENTRY
ExInterlockedPopEntryList(
	PSINGLE_LIST_ENTRY  ListHead,
	PCRITICAL_SECTION   Lock
);

PSINGLE_LIST_ENTRY
ExInterlockedPushEntryList(
	PSINGLE_LIST_ENTRY  ListHead,
	PSINGLE_LIST_ENTRY  Entry,
	PCRITICAL_SECTION   Lock
);

VOID
Nt_InitializeSListHead(
	PSLIST_HEADER SListHead
);

#endif  // MY_NT_DDK

_ML_C_TAIL_



#include <new>

#define MM_NOT_FREE     0x80000000

class __declspec(novtable) MemoryAllocator
{
protected:

#if ML_KERNEL_MODE

public:
	static const ULONG m_kMemTag = TAG4('MLKN');

#else

	HANDLE       m_hHeap;
	static HANDLE       m_hHeapGlobal;
	static ULONG_PTR    m_ObjectCount;

#endif  // MY_NT_DDK

protected:

#if !ML_KERNEL_MODE

	NoInline static HANDLE CreateGlobalHeapInternal(DWORD flOptions = 0, SIZE_T dwInitialSize = 0, SIZE_T dwMaximumSize = 0)
	{
		if (m_hHeapGlobal == NULL)
			m_hHeapGlobal = CreateHeapInternal(flOptions, dwInitialSize, dwMaximumSize);

		return m_hHeapGlobal;
	}

	ForceInline static ULONG_PTR AddRef()
	{
		return ++m_ObjectCount;
		//return Interlocked_Increment((PLong)&m_ObjectCount);
	}

	ForceInline static ULONG_PTR Release()
	{
		return --m_ObjectCount;
		//return Interlocked_Decrement((PLong)&m_ObjectCount);
	}

	NoInline Bool DestroyGlobal()
	{
		if (m_hHeap == NULL)
			return False;

		return DestroyGlobalHeap();
	}

	ForceInline Bool IsHeapPrivate()
	{
		return m_hHeap != NULL && m_hHeap != m_hHeapGlobal;
	}

#endif  // MY_NT_DDK

public:
	NoInline MemoryAllocator(HANDLE hHeap = NULL)
	{
#if ML_KERNEL_MODE
		UNREFERENCED_PARAMETER(hHeap);
#else
		if (hHeap != NULL)
		{
			//            m_hHeapPrivate = hHeap;
		}
		else
		{
			if (AddRef() == 1)
			{
				hHeap = CreateGlobalHeapInternal();
				if (hHeap == NULL)
					Release();
			}
			else
			{
				hHeap = m_hHeapGlobal;
			}

			//            m_hHeapPrivate = NULL;
		}

		m_hHeap = hHeap;

#endif  // MY_NT_DDK
	}

	MemoryAllocator(const MemoryAllocator &mem)
	{
		*this = mem;
	}

	NoInline ~MemoryAllocator()
	{
#if !ML_KERNEL_MODE

		if (IsHeapPrivate())
		{
			if (IsNotProcessHeap())
				DestroyHeapInternal(m_hHeap);
		}
		else
		{
			DestroyGlobal();
		}

#endif  // MY_NT_DDK
	}

#if !ML_KERNEL_MODE

	HANDLE GetHeap() const
	{
		return m_hHeap;
	}

	static HANDLE CreateGlobalHeap(ULONG Options = 0)
	{
		HANDLE hHeap = CreateGlobalHeapInternal(Options);
		if (hHeap != NULL)
			AddRef();
		return hHeap;
	}

	static Bool DestroyGlobalHeap()
	{
		if (m_hHeapGlobal != NULL && Release() == 0)
		{
			if (DestroyHeapInternal(m_hHeapGlobal))
			{
				m_hHeapGlobal = NULL;
				return True;
			}
		}

		return False;
	}

	static HANDLE GetGlobalHeap()
	{
		if (m_hHeapGlobal == NULL)
			m_hHeapGlobal = MemoryAllocator::CreateGlobalHeap();

		return m_hHeapGlobal;
	}

	static PVOID GetAddressOfGlobalHeap()
	{
		return &m_hHeapGlobal;
	}

	static PVOID AllocateMemory(ULONG_PTR Size, ULONG Flags = 0)
	{
		return AllocateHeapInternal(GetGlobalHeap(), Flags, Size);
	}

	static PVOID ReAllocateMemory(PVOID Memory, ULONG_PTR Size, ULONG Flags = 0)
	{
		PVOID Block = ReAllocateHeapInternal(GetGlobalHeap(), Flags, Memory, Size);

		if (Block == NULL && Memory != NULL)
		{
			FreeMemory(Memory);
		}

		return Block;
	}

	static BOOL FreeMemory(PVOID Memory, ULONG Flags = 0)
	{
		return FreeHeapInternal(GetGlobalHeap(), Flags, Memory);
	}

	HANDLE CreateHeap(DWORD flOptions = 0, SIZE_T dwInitialSize = 0, SIZE_T dwMaximumSize = 0)
	{
		if (IsHeapPrivate())
			DestroyHeapInternal(m_hHeap);
		else
			DestroyGlobal();

		m_hHeap = CreateHeapInternal(flOptions, dwInitialSize, dwMaximumSize);

		return m_hHeap;
	}

	Bool DestroyHeap()
	{
		Bool Result = True;

		if (IsHeapPrivate() && IsNotProcessHeap())
		{
			Result = DestroyHeapInternal(m_hHeap);
			if (Result)
				m_hHeap = NULL;
		}

		return Result;
	}

#else // r0

	static POOL_TYPE GetGlobalHeap()
	{
		return NonPagedPool;
	}

	static PVOID AllocateMemory(ULONG_PTR Size, POOL_TYPE PoolType = NonPagedPool, ULONG Tag = m_kMemTag)
	{
		return AllocateHeapInternal(Size, PoolType, Tag);
	}

	static BOOL FreeMemory(PVOID Memory, ULONG Tag = m_kMemTag)
	{
		return FreeHeapInternal(Memory, Tag);
	}

#endif  // ML_KERNEL_MODE

#if ML_KERNEL_MODE

	PVoid Alloc(SizeT Size, POOL_TYPE PoolType = NonPagedPool, ULONG Tag = m_kMemTag)
	{
		return AllocateHeapInternal(Size, PoolType, Tag);
	}

	Bool Free(PVoid pBuffer, ULONG Tag = m_kMemTag)
	{
		return FreeHeapInternal(pBuffer, Tag);
	}

	Bool SafeFree(PVoid pBuffer, ULONG Tag = m_kMemTag)
	{
		LPVoid **pt = (LPVoid **)pBuffer;
		if (*pt == NULL)
			return False;

		FreeHeapInternal(*pt, Tag);
		*pt = NULL;

		return True;
	}

#else

	PVoid Alloc(SizeT Size, ULONG_PTR Flags = 0)
	{
		return AllocateHeapInternal(m_hHeap, Flags, Size);
	}

	PVoid ReAlloc(PVoid pBuffer, SizeT Size, ULONG_PTR Flags = 0)
	{
		PVoid pRealloc;

		if (pBuffer == NULL)
			return Alloc(Size, Flags);

		pRealloc = ReAllocateHeapInternal(m_hHeap, Flags, pBuffer, Size);
		if (!FLAG_ON(Flags, MM_NOT_FREE))
		{
			if (pRealloc == NULL)
			{
				Free(pBuffer);
			}
		}

		return pRealloc;
	}

	Bool Free(PVoid pBuffer, ULONG_PTR Flags = 0)
	{
		return pBuffer == NULL ? False : FreeHeapInternal(m_hHeap, Flags, pBuffer);
	}

	Bool SafeFree(LPVoid pBuffer, ULONG_PTR Flags = 0)
	{
		LPVoid **pt = (LPVoid **)pBuffer;
		if (*pt == NULL)
			return False;

		Bool Result = FreeHeapInternal(m_hHeap, Flags, *pt);
		if (Result)
			*pt = NULL;

		return Result;
	}

#endif

private:

#if !ML_KERNEL_MODE

#if USE_NT_VER

	BOOL IsNotProcessHeap()
	{
		return m_hHeap != Nt_CurrentPeb()->ProcessHeap;
	}

	static HANDLE CreateHeapInternal(ULONG Flags = 0, SIZE_T CommitSize = 0, SIZE_T ReserveSize = 0)
	{
		if (Flags == 0)
			Flags = HEAP_GROWABLE;
		return RtlCreateHeap(Flags, NULL, ReserveSize, CommitSize, NULL, NULL);
	}

	static BOOL DestroyHeapInternal(HANDLE hHeap)
	{
		return !RtlDestroyHeap(hHeap);
	}

#else

	BOOL IsNotProcessHeap()
	{
		return m_hHeap != GetProcessHeap();
	}

	static HANDLE CreateHeapInternal(ULONG Flags = 0, SIZE_T CommitSize = 0, SIZE_T ReserveSize = 0)
	{
		return HeapCreate(Flags, CommitSize, ReserveSize);
	}

	static BOOL DestroyHeapInternal(HANDLE hHeap)
	{
		return HeapDestroy(hHeap);
	}

#endif

#endif  // MY_NT_DDK

	static LONG_PTR ModifyAllocCount(LONG_PTR Increment)
	{
		static LONG_PTR AllocCount;

		AllocCount += Increment;
		return AllocCount;
	}

#if ML_KERNEL_MODE

	static PVOID AllocateHeapInternal(SIZE_T Size, POOL_TYPE PoolType = NonPagedPool, ULONG Tag = m_kMemTag)
	{
		PVOID Memory = ExAllocatePoolWithTag(PoolType, Size, Tag);

#if ML_MEMORY_DEBUG

		ModifyAllocCount(Memory != nullptr);

#endif

		return Memory;
	}

	static BOOL FreeHeapInternal(PVOID Memory, ULONG Tag = m_kMemTag)
	{
		if (Memory == NULL)
			return TRUE;

#if ML_MEMORY_DEBUG

		ModifyAllocCount(-1);

#endif

		ExFreePoolWithTag(Memory, Tag);

		return TRUE;
	}

#elif USE_NT_VER

	static PVOID AllocateHeapInternal(HANDLE Heap, ULONG_PTR Flags, SIZE_T Size)
	{
		PVOID Memory = RtlAllocateHeap(Heap, (ULONG)Flags, Size);

#if ML_MEMORY_DEBUG

		ModifyAllocCount(Memory != nullptr);

#endif

		return Memory;
	}

	static PVOID ReAllocateHeapInternal(HANDLE Heap, ULONG_PTR Flags, PVOID Memory, SIZE_T Size)
	{
		return Memory == NULL ? AllocateHeapInternal(Heap, Flags, Size) : RtlReAllocateHeap(Heap, (ULONG)Flags, Memory, Size);
	}

	static BOOL FreeHeapInternal(HANDLE Heap, ULONG_PTR Flags, PVOID Memory)
	{
		if (Memory == NULL)
			return TRUE;

#if ML_MEMORY_DEBUG

		ModifyAllocCount(-1);

#endif

		return RtlFreeHeap(Heap, (ULONG)Flags, Memory);
	}

#elif 0
	static PVOID AllocateHeapInternal(HANDLE hHeap, ULONG Flags, SIZE_T Size)
	{
		return HeapAlloc(hHeap, Flags, Size);
	}

	static PVOID ReAllocateHeapInternal(HANDLE hHeap, ULONG Flags, PVOID pvMemory, SIZE_T Size)
	{
		return pvMemory == NULL ? AllocateHeapInternal(hHeap, Flags, Size) : HeapReAlloc(hHeap, Flags, pvMemory, Size);
	}

	static BOOL FreeHeapInternal(HANDLE hHeap, ULONG Flags, PVOID pvMemory)
	{
		if (pvMemory == NULL)
			return TRUE;

		return HeapFree(hHeap, Flags, pvMemory);
	}

#endif  // MY_NT_DDK
};

#if !ML_KERNEL_MODE

DECL_SELECTANY HANDLE       MemoryAllocator::m_hHeapGlobal = NULL;
DECL_SELECTANY ULONG_PTR    MemoryAllocator::m_ObjectCount = 0;

#endif // ML_KERNEL_MODE

// typedef MemoryAllocator CMem;

#if ML_KERNEL_MODE

template<class Object, POOL_TYPE PoolType = NonPagedPool>
class FixedMemoryBlock
{
protected:
	union
	{
		NPAGED_LOOKASIDE_LIST   NonPaged;
		PAGED_LOOKASIDE_LIST    Paged;
	} LookasideList;

public:
	static const ULONG_PTR BlockSize = sizeof(Object);

public:
	NTSTATUS Initialize(
		PALLOCATE_FUNCTION  Allocate = NULL,
		PFREE_FUNCTION      Free = NULL,
		ULONG               Flags = 0,
		ULONG               Tag = MemoryAllocator::m_kMemTag,
		USHORT              Depth = 0
	)
	{
		switch (PoolType)
		{
		case NonPagedPool:
			ExInitializeNPagedLookasideList(&LookasideList.NonPaged, Allocate, Free, Flags, BlockSize, Tag, Depth);
			break;

		case PagedPool:
			ExInitializePagedLookasideList(&LookasideList.Paged, Allocate, Free, Flags, BlockSize, Tag, Depth);
			break;

		default:
			return STATUS_INVALID_PARAMETER;
		}

		return STATUS_SUCCESS;
	}

	NTSTATUS UnInitialize()
	{
		switch (PoolType)
		{
		case NonPagedPool:
			ExDeleteNPagedLookasideList(&LookasideList.NonPaged);
			break;

		case PagedPool:
			ExDeletePagedLookasideList(&LookasideList.Paged);
			break;
		}

		return STATUS_SUCCESS;
	}

	Object* Allocate()
	{
		PVOID Block;

		switch (PoolType)
		{
		case NonPagedPool:
			Block = ExAllocateFromNPagedLookasideList(&LookasideList.NonPaged);
			break;

		case PagedPool:
			Block = ExAllocateFromPagedLookasideList(&LookasideList.Paged);
			break;

		default:
			Block = NULL;
			break;
		}

		if (Block != NULL)
			new (Block)Object;

		return (Object *)Block;
	}

	VOID Free(PVOID Block)
	{
		if (Block == NULL)
			return;

		switch (PoolType)
		{
		case NonPagedPool:
			((Object *)Block)->~Object();
			ExFreeToNPagedLookasideList(&LookasideList.NonPaged, Block);
			break;

		case PagedPool:
			((Object *)Block)->~Object();
			ExFreeToPagedLookasideList(&LookasideList.Paged, Block);
			break;
		}
	}
};

#endif // r0


_ML_C_HEAD_

BOOL
FreeMemoryP(
	PVOID Memory,
	ULONG DEF_VAL(Flags, 0)
);

#if ML_KERNEL_MODE

PVOID
AllocateMemoryP(
	ULONG_PTR Size,
	POOL_TYPE DEF_VAL(PoolType, NonPagedPool)
);

PVOID
AllocateMemory(
	ULONG_PTR Size,
	POOL_TYPE DEF_VAL(PoolType, NonPagedPool)
);

BOOL
FreeMemory(
	PVOID Memory,
	ULONG Flags = 0
);

#else // user mode

PVOID
AllocateMemoryP(
	ULONG_PTR   Size,
	ULONG       DEF_VAL(Flags, 0)
);

PVOID
ReAllocateMemoryP(
	PVOID       Memory,
	ULONG_PTR   Size,
	ULONG       DEF_VAL(Flags, 0)
);

PVOID
AllocateMemory(
	ULONG_PTR   Size,
	ULONG       DEF_VAL(Flags, 0)
);

PVOID
ReAllocateMemory(
	PVOID       Memory,
	ULONG_PTR   Size,
	ULONG       DEF_VAL(Flags, 0)
);

BOOL
FreeMemory(
	PVOID Memory,
	ULONG DEF_VAL(Flags, 0)
);

PVOID
AllocateVirtualMemory(
	ULONG_PTR   Size,
	ULONG       DEF_VAL(Protect, PAGE_EXECUTE_READWRITE),
	HANDLE      DEF_VAL(ProcessHandle, NtCurrentProcess())
);

BOOL
FreeVirtualMemory(
	PVOID   Memory,
	HANDLE  DEF_VAL(ProcessHandle, NtCurrentProcess())
);

#endif // ML_KERNEL_MODE

_ML_C_TAIL_

#pragma warning(disable:4127)


/************************************************************************
misc
************************************************************************/

ForceInline ULONG HashAPI(PCChar pszName)
{
	ULONG Hash = 0;

	while (*(PByte)pszName)
	{
		Hash = _rotl(Hash, 0x0D) ^ *(PByte)pszName++;
	}

	return Hash;
}

ForceInline ULONG HashAPILower(PCChar pszName)
{
	ULONG Hash = 0;

	while (*(PByte)pszName)
	{
		BYTE ch = *(PByte)pszName++;
		Hash = _rotl(Hash, 0x0D) ^ CHAR_LOWER(ch);
	}

	return Hash;
}

ForceInline ULONG HashAPI2(PCChar pszName)
{
	ULONG Hash = 0;

	while (*(PByte)pszName)
	{
		Hash = _rotr(Hash, 9) + (ULONG)(*pszName++) + 0x9E370001;
	}

	return Hash;
}

ForceInline ULONG HashAPIUpper(PCChar pszName)
{
	ULONG ch, Hash = 0;

	while (*(PByte)pszName)
	{
		ch = *(PByte)pszName++;
		Hash = _rotl(Hash, 0x0D) ^ CHAR_UPPER(ch);
	}

	return Hash;
}

ForceInline ULONG HashAPI2Upper(PCChar pszName)
{
	ULONG ch, Hash = 0;

	while (*(PByte)pszName)
	{
		ch = *(PByte)pszName++;
		Hash = _rotr(Hash, 9) + CHAR_UPPER(ch) + 0x9E370001;
	}

	return Hash;
}

inline ULONG HashUnicodeUpper(PCWSTR Unicode, ULONG DEF_VAL(Hash, 0))
{
	WCHAR ch;

	do
	{
		ch = *Unicode++;
		Hash = Hash * 0x1003F + CHAR_UPPER(ch);
	} while (ch != 0);

	return Hash;
}

inline ULONG HashUnicodeLower(PCWSTR Unicode, ULONG DEF_VAL(Hash, 0))
{
	ULONG ch;

	ch = *Unicode++;

	while (ch)
	{
		Hash = Hash * 0x1003F + CHAR_LOWER(ch);
		ch = *Unicode++;
	}

	return Hash;
}

inline ULONG HashAnsiLower(PCSTR Ansi, ULONG DEF_VAL(Hash, 0))
{
	ULONG ch;

	ch = *(PByte)Ansi++;

	while (ch)
	{
		Hash = Hash * 0x1003F + CHAR_LOWER(ch);
		ch = *(PByte)Ansi++;
	}

	return Hash;
}




#if ML_KERNEL_MODE

NTSTATUS
ProbeForReadSafe(
	PVOID   Address,
	SIZE_T  Length,
	ULONG   Alignment
);

NTSTATUS
ProbeForWriteSafe(
	PVOID   Address,
	SIZE_T  Length,
	ULONG   Alignment
);

NTSTATUS
MmProbeAndLockPagesSafe(
	PMDL            MemoryDescriptorList,
	KPROCESSOR_MODE AccessMode,
	LOCK_OPERATION  Operation
);

#else   // else if !MY_NT_DDK

/************************************************************************
user mode
************************************************************************/

NTSTATUS CreateMiniDump(PEXCEPTION_POINTERS ExceptionPointers);

/************************************************************************
directory
************************************************************************/

BOOL    IsPathExistsW(LPCWSTR Path);
BOOL    IsPathExistsA(LPCSTR Path);
BOOL    CreateDirectoryRecursiveA(LPCSTR PathName);
BOOL    CreateDirectoryRecursiveW(LPCWSTR PathName);
ULONG   GetExeDirectoryW(LPWSTR Path, ULONG BufferCount);
ULONG   GetExeDirectoryA(LPSTR Path, ULONG BufferCount);


enum
{
	EDF_SUBDIR = 0x00000001,
	EDF_BEFORE = 0x00000002,
	EDF_AFTER = 0x00000004,
	EDF_PROCDIR = 0x00000008,
};

typedef
LONG
(STDCALL *EnumDirectoryFilesCallBackRoutine)(
	PVOID               Buffer,
	PWIN32_FIND_DATAW   FindData,
	ULONG_PTR           Context
	);

#define EnumFilesM(Buffer, FindData, Context) [] (PVOID Buffer, PWIN32_FIND_DATAW FindData, ULONG_PTR Context) -> LONG

typedef EnumDirectoryFilesCallBackRoutine FEnumDirectoryFilesCallBack;

typedef struct
{
	ULONG                               Flags;
	HANDLE                              hHeap;
	PVOID                               lpBuffer;
	PVOID                               lpOutBuffer;
	LPCWSTR                             pszFilter;
	ULONG                               ElemSize;
	LARGE_INTEGER                       ElemCount;
	LARGE_INTEGER                       MaxCount;
	EnumDirectoryFilesCallBackRoutine   CallBack;
	ULONG_PTR                           Context;
	WIN32_FIND_DATAW                    wfd;
	WCHAR                               SymbolicLinkPath[MAX_NTPATH];
} ENUM_DIRECTORY_INFO;

BOOL EnumDirectoryFilesWorker(LPWSTR lpPath, ENUM_DIRECTORY_INFO *pFindInfo);
BOOL EnumDirectoryFilesFree(PVOID lpBuffer);

BOOL
EnumDirectoryFiles(
	PVOID                              *lpFilesBuffer,
	LPCWSTR                             pszFilter,
	ULONG                               ElemSize,
	LPCWSTR                             pszPath,
	PLARGE_INTEGER                      pElemCount,
	EnumDirectoryFilesCallBackRoutine   CallBack,
	ULONG_PTR                           Context,
	ULONG                               Flags = 0
);

/************************************************************************
command line
************************************************************************/

typedef struct
{
	int newmode;
} __my_startupinfo;

EXTC_IMPORT void CDECL __getmainargs(int *argc, char ***argv, char ***envp, int, __my_startupinfo *);
EXTC_IMPORT void CDECL __wgetmainargs(int *argc, wchar_t ***argv, wchar_t ***envp, int, __my_startupinfo *);

#ifndef UNICODE
#define getmainargs __getmainargs
#else
#define getmainargs __wgetmainargs
#endif /* UNICODE */

/*
argret = __wgetmainargs(&argc, &argv, &envp,
_dowildcard, &startinfo);
#else
argret = __getmainargs(&argc, &argv, &envp,
_dowildcard, &startinfo);
*/

#if USE_NT_VER

#define getargsW(pargc, pargv) (*(pargv)) = CmdLineToArgvW(Nt_GetCommandLine(), (pargc))

#else // !USE_NT_VER

#define getargsA(pargc, pargv) \
						            { \
                Char **__envp__;int __dowildcard = 0;__my_startupinfo _my_startupinfo;\
                *(pargc) = 0; \
                __getmainargs((int *)(pargc), (pargv), (&__envp__), __dowildcard, &_my_startupinfo); \
						            }

#define getargsW(pargc, pargv) \
						            { \
                WChar **__envp__;int __dowildcard = 0;__my_startupinfo _my_startupinfo; \
                *(pargc) = 0; \
                __wgetmainargs((int *)(pargc), (pargv), (&__envp__), __dowildcard, &_my_startupinfo); \
						            }

#endif  // USE_NT_VER

#if MY_UNICODE_ENABLE
#define getargs getargsW
#else
#define getargs getargsA
#endif

#if defined(_MT) && !defined(_DLL)
#undef getargs
#undef getargsA
#undef getargsW
#define getargs(a, b)
#define getargsA getargs
#define getargsW getargs
#endif

LONG_PTR    FASTCALL CmdLineToArgvWorkerA(LPSTR pszCmdLine, LPSTR *pArgv, PLONG_PTR pCmdLineLength);
PSTR*       FASTCALL CmdLineToArgvA(LPSTR pszCmdLine, PLONG_PTR pArgc);
LONG_PTR    FASTCALL CmdLineToArgvWorkerW(LPWSTR pszCmdLine, LPWSTR *pArgv, PLONG_PTR pCmdLineLength);
PWSTR*      FASTCALL CmdLineToArgvW(LPWSTR pszCmdLine, PLONG_PTR pArgc);

inline VOID ReleaseArgv(PVOID Argv)
{
	FreeMemory(Argv, 0);
}

ForceInline PTChar* FASTCALL CmdLineToArgv(PTChar pszCmdLine, PLONG_PTR pArgc)
{
	if (sizeof(*pszCmdLine) == sizeof(Char))
		return (PTChar *)CmdLineToArgvA((LPSTR)pszCmdLine, pArgc);
	else
		return (PTChar *)CmdLineToArgvW((LPWSTR)pszCmdLine, pArgc);
}

#endif  // MY_NT_DDK


#pragma warning(default:4127)


#if !defined(FILE_DEVICE_CONSOLE)
#define FILE_DEVICE_CONSOLE             0x00000050
#endif

_ML_C_HEAD_

CHAR        ConsoleReadChar();
VOID        ClearConsoleScreen();
VOID        PauseConsole(PCWSTR DEF_VAL(PauseText, nullptr));
ULONG_PTR   PrintConsoleA(PCSTR Format, ...);
ULONG_PTR   PrintConsoleW(PCWSTR Format, ...);
ULONG_PTR   PrintConsole(PCWSTR Format, ...);

_ML_C_TAIL_


#if MY_OS_WIN32

EXTC_IMPORT int STDCALL UCIDecode(const void *src, int srclen, void** dst, int* stride, int* w, int* h, int* bpp);
EXTC_IMPORT int STDCALL UCIFree(void* p);

#endif


#if MY_OS_WIN32

#define RGBA_GetRValue(color) ((Byte)((color) >> 0))
#define RGBA_GetGValue(color) ((Byte)((color) >> 8))
#define RGBA_GetBValue(color) ((Byte)((color) >> 16))
#define RGBA_GetAValue(color) ((Byte)((color) >> 24))
#define RGBA(r, g, b, a) (UInt32)(((u32)(u8)(r)) | (((u32)(u8)(g) << 8)) | (((u32)(u8)(b) << 16)) | (((u32)(u8)(a) << 24)))

#endif // MY_OS_WIN32

#pragma pack(push, 1)

typedef struct
{
	UInt16 Tag;                   // 0x00
	UInt32 FileSize;             // 0x02
	UInt32 Reserve;              // 0x06
	UInt32 RawOffset;            // 0x0A
	struct
	{
		UInt32 InfoHeaderSize;   // 0x0E
		Int32  Width;            // 0x12
		Int32  Height;           // 0x16
		UInt16 Layer;             // 0x1A
		UInt16 Bit;               // 0x1C
		UInt32 Compressed;       // 0x1E
		UInt32 SizeImage;        // 0x22
		Int32  XPelsPerMeter;     // 0x26
		Int32  YPelsPerMeter;     // 0x2A
		UInt32 ClrUsed;          // 0x2E
		UInt32 ClrImportant;     // 0x32
	} Info;
} IMAGE_BITMAP_HEADER, *PIMAGE_BITMAP_HEADER;

enum
{
	TgaImageRga = 2,
};

typedef struct
{
	UChar  ID;
	UChar  PaletteType;
	UChar  ImageType;
	UInt16 PaletteEntry;
	UInt16 PaletteLength;
	UChar  PaletteBitCount;
	UInt16 OriginX;
	UInt16 OriginY;
	UInt16 Width;
	UInt16 Height;
	UChar  PixelDepth;
	UChar  ImageDescription;

} IMAGE_TGA_HEADER, *PIMAGE_TGA_HEADER;

typedef struct
{
	union
	{
		PVoid lpBuffer;
		PByte  pbBuffer;
	};
	Int32   Width;
	Int32   Height;
	Int32   BitsPerPixel;
	Int32   Stride;
	PVoid   ExtraInfo;
	UInt32  ExtraInfoSize;

} UCI_INFO;

#define UCI_META_INFO_MAGIC TAG4('META')

typedef struct
{
	UInt32 Magic;   // TAG4('META')
	UInt32 cbSize;
	Int32  Width;
	Int32  Height;
	Int32  BitsPerPixel;
} UCI_META_INFO;

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

ForceInline Long GetBitmapStride(Long Width, Long BitsPerPixel)
{
	return (Width * BitsPerPixel / 8 + 3) & ~3;
}

inline
Bool
FASTCALL
InitBitmapHeader(
	IMAGE_BITMAP_HEADER*Header,
	Int32               Width,
	Int32               Height,
	Int32               BitsPerPixel,
	PLongPtr            Stride
)
{
	UInt32 LocalStride, PaletteSize;

	if (Header == NULL)
		return False;

	ZeroMemory(Header, sizeof(*Header));

	PaletteSize = BitsPerPixel == 8 ? 256 * 4 : 0;

	Header->RawOffset = sizeof(*Header) + PaletteSize;
	Header->Info.Height = Height;
	Header->Info.Width = Width;
	Header->Tag = TAG2('BM');
	Header->Info.InfoHeaderSize = sizeof(Header->Info);
	Header->Info.Layer = 1;
	Header->Info.Bit = (UInt16)BitsPerPixel;
	//    pHeader->Info.dwClrUsed = 1;
	LocalStride = (Width * BitsPerPixel / 8 + 3) & ~3;
	if (Stride)
		*Stride = LocalStride;

	Header->FileSize = Height * LocalStride + sizeof(*Header) + PaletteSize;

	return True;
}

#if MY_OS_WIN32

inline PVoid SkipUCIStream(PVoid Data, Int32 DataSize, PUInt32 ExtraInfoSize)
{
	PByte Buffer = (PByte)Data;
	UInt32 StreamCount;

	switch (*(PUInt32)Buffer)
	{
	case TAG4('UCI3'):
	case TAG4('UCI '):
	case TAG4('UCI@'): StreamCount = 1; break;
	case TAG4('UCI4'):
	case TAG4('UCI!'):
	case TAG4('UCIA'): StreamCount = 2; break;
	case TAG4('UCIT'): StreamCount = 3; break;
	case TAG4('UCIQ'): StreamCount = 4; break;
	default: return NULL;
	}

	Buffer += 0xC;
	DataSize -= 0xC;
	while (StreamCount--)
	{
		UInt32 Size = *(PUInt32)Buffer + 4;
		Buffer += Size;
		DataSize -= Size;
	}

	if (DataSize <= 0)
	{
		if (ExtraInfoSize)
			*ExtraInfoSize = 0;
		return NULL;
	}

	if (ExtraInfoSize)
		*ExtraInfoSize = DataSize;

	return Buffer;
}

inline Int32 UCIDecodeEx(PVoid Data, Int32 DataSize, UCI_INFO *UCIInfo, Bool GetExtraInfo)
{
	Int32 ret;

	if (UCIInfo == NULL)
		return -1;

	ret = UCIDecode(Data, DataSize, &UCIInfo->lpBuffer, &UCIInfo->Stride, &UCIInfo->Width, &UCIInfo->Height, &UCIInfo->BitsPerPixel);
	if (ret < 0)
		return ret;

	if (GetExtraInfo)
	{
		UCIInfo->ExtraInfo = SkipUCIStream(Data, DataSize, &UCIInfo->ExtraInfoSize);
	}
	else
	{
		UCIInfo->ExtraInfo = NULL;
		UCIInfo->ExtraInfoSize = 0;
	}

	return ret;
}

inline Void UCIFreeEx(UCI_INFO *UCIInfo)
{
	if (UCIInfo == NULL)
		return;

	UCIFree(UCIInfo->lpBuffer);
}

#endif // MY_OS_WIN32

inline
UInt32
ConvertRawTo8Bit(
	UCI_INFO   *UciInfo,
	Int32       Width,
	Int32       Height,
	Int32       BitsPerPixel,
	PVoid       pvBuffer,
	UInt32      BufferSize
)
{
	PByte pbSrc, pbDest;
	LongPtr BytesPerPixel, Stride;
	IMAGE_BITMAP_HEADER *pBmp;

	UNREFERENCED_PARAMETER(Width);
	UNREFERENCED_PARAMETER(Height);
	UNREFERENCED_PARAMETER(BitsPerPixel);

	if (BufferSize < sizeof(*pBmp))
		return 0;

	pBmp = (IMAGE_BITMAP_HEADER *)pvBuffer;
	InitBitmapHeader(pBmp, Width, Height, BitsPerPixel, &Stride);
	if (pBmp->FileSize > BufferSize)
		return 0;

	pbDest = (PByte)(pBmp + 1);
	for (ULong Color = 0, Count = 256; Count; Color += 0x00010101, --Count)
	{
		*(PULong)pbDest = Color;
		pbDest += 4;
	}

	BytesPerPixel = UciInfo->BitsPerPixel / 8;
	pbSrc = UciInfo->pbBuffer + (UciInfo->Height - 1) * UciInfo->Stride;
	pbDest = (PByte)pBmp + pBmp->RawOffset;
	for (ULong Height = UciInfo->Height;;)
	{
		PByte p1, p2;

		p1 = pbSrc;
		p2 = pbDest;
		for (ULong Width = UciInfo->Width;;)
		{
			ULong R, G, B, Color = *(PULong)p1;

			// Y = 0.299R + 0.587G + 0.114B

			/************************************************************************
			full range������������õľ������, JPEGӦҲ�õ��������

			Y = 0.11448 * B + 0.58661 * G + 0.29891 * R
			U = 0.50000 * B - 0.33126 * G - 0.16874 * R + 128
			V =-0.08131 * B - 0.41869 * G + 0.50000 * R + 128

			�Ż���Ĺ�ʽ����:

			Y = (120041*B + 615105*G + 313430*R + 0x007FFFF) >> 20
			U = (524288*B - 347351*G - 176937*R + 0x807FFFF) >> 20
			V = (-85260*B - 439028*G + 524288*R + 0x807FFFF) >> 20
			************************************************************************/

			R = RGBA_GetRValue(Color);
			G = RGBA_GetGValue(Color);
			B = RGBA_GetBValue(Color);
			*p2 = (BYTE)((R * 313430 + G * 615105 + B * 120041 + 0x007FFFF) >> 20);
			++p2;
			p1 += BytesPerPixel;

			if (--Width == 0)
				break;
		}

		pbSrc -= UciInfo->Stride;
		pbDest += Stride;

		if (--Height == 0)
			break;
	}

	return pBmp->FileSize;
}

inline
UInt32
ConvertRawTo24Or32(
	UCI_INFO   *pUciInfo,
	Int32       Width,
	Int32       Height,
	Int32       BitsPerPixel,
	PVoid       pvBuffer,
	UInt32      BufferSize
)
{
	PByte pbSrc, pbDest;
	LongPtr Stride;
	IMAGE_BITMAP_HEADER *pBmp;

	if (BufferSize < sizeof(*pBmp))
		return 0;

	pBmp = (IMAGE_BITMAP_HEADER *)pvBuffer;
	InitBitmapHeader(pBmp, Width, Height, BitsPerPixel, &Stride);
	if (pBmp->FileSize > BufferSize)
		return 0;

	pbSrc = pUciInfo->pbBuffer;
	pbDest = (PByte)pBmp;
	pbDest += (Height - 1) * Stride + sizeof(*pBmp);

	for (ULong Height = pUciInfo->Height; Height; --Height)
	{
		CopyMemory(pbDest, pbSrc, Stride);
		pbDest -= Stride;
		pbSrc += pUciInfo->Stride;
	}

	return pBmp->FileSize;
}

inline
UInt32
ConvertRawToBitMap(
	UCI_INFO   *pUciInfo,
	UInt32      Width,
	UInt32      Height,
	UInt32      BitsPerPixel,
	PVoid       pvBuffer,
	UInt32      BufferSize
)
{
	if (BitsPerPixel == 8)
	{
		return ConvertRawTo8Bit(pUciInfo, Width, Height, BitsPerPixel, pvBuffer, BufferSize);
	}
	else
	{
		switch (BitsPerPixel)
		{
		case 24:
		case 32:
			return ConvertRawTo24Or32(pUciInfo, Width, Height, BitsPerPixel, pvBuffer, BufferSize);
		}
	}

	return 0;
}



typedef struct
{
	UInt32 RIFF;                    // TAG4('RIFF')
	UInt32 Size;
	UInt32 WAVE;                    // TAG4('WAVE')
	UInt32 fmt;                     // TAG4('fmt ')
	UInt32 FormatLength;
	UInt16 FormatTag;
	UInt16 Channels;
	UInt32 SamplesPerSec;
	UInt32 AvgBytesPerSec;
	UInt16 BlockAlign;
	UInt16 BitsPerSample;
	UInt32 data;                    // TAG4('data')
	UInt32 DataSize;

} AUDIO_WAVE_HEADER, *PAUDIO_WAVE_HEADER;


#include <MMSystem.h>

inline
Bool
FASTCALL
InitWaveHeader(
	AUDIO_WAVE_HEADER  *Header,
	const WAVEFORMATEX *WaveFormat,
	UInt32              WaveLength
)
{
	if (Header == NULL || WaveFormat == NULL)
		return False;

	Header->RIFF = TAG4('RIFF');
	Header->fmt = TAG4('fmt ');
	Header->WAVE = TAG4('WAVE');
	Header->data = TAG4('data');
	Header->FormatLength = 16;

	Header->FormatTag = WaveFormat->wFormatTag;
	Header->Channels = WaveFormat->nChannels;
	Header->SamplesPerSec = WaveFormat->nSamplesPerSec;
	Header->AvgBytesPerSec = WaveFormat->nAvgBytesPerSec;
	Header->BlockAlign = WaveFormat->nBlockAlign;
	Header->BitsPerSample = WaveFormat->wBitsPerSample;
	Header->DataSize = WaveLength;
	Header->Size = WaveLength + sizeof(*Header) - 8;

	return True;
}


inline
Bool
FASTCALL
InitWaveHeader(
	AUDIO_WAVE_HEADER  *Header,
	UInt16              Channels,
	UInt32              SampleRate,
	UInt16              BitsPerSample,
	UInt32              WaveLength
)
{
	if (Header == NULL)
		return False;

	Header->RIFF = TAG4('RIFF');
	Header->fmt = TAG4('fmt ');
	Header->WAVE = TAG4('WAVE');
	Header->data = TAG4('data');
	Header->FormatTag = WAVE_FORMAT_PCM;
	Header->FormatLength = 16;
	Header->Channels = Channels;
	Header->BitsPerSample = BitsPerSample;
	Header->BlockAlign = Header->Channels * BitsPerSample / 8;
	Header->SamplesPerSec = SampleRate;
	Header->AvgBytesPerSec = Header->BlockAlign * Header->SamplesPerSec;
	Header->DataSize = WaveLength;
	Header->Size = WaveLength + sizeof(*Header) - 8;

	return True;
}





_ML_C_HEAD_

#define SDBGMEF_IGNORE_ENVIRONMENT  1

#define SDB_DATABASE_MAIN           0x80000000
#define SDB_DATABASE_SHIM           0x00010000
#define SDB_DATABASE_MSI            0x00020000
#define SDB_DATABASE_DRIVERS        0x00040000
#define SDB_DATABASE_DETAILS        0x00080000
#define SDB_DATABASE_SP_DETAILS     0x00100000
#define SDB_DATABASE_RESOURCE       0x00200000
#define SDB_DATABASE_TYPE_MASK      0xF02F0000

#define SDB_DATABASE_MAIN_SHIM              (SDB_DATABASE_SHIM          | SDB_DATABASE_MSI | SDB_DATABASE_MAIN)
#define SDB_DATABASE_MAIN_MSI               (SDB_DATABASE_MSI           | SDB_DATABASE_MAIN)
#define SDB_DATABASE_MAIN_DRIVERS           (SDB_DATABASE_DRIVERS       | SDB_DATABASE_MAIN)
#define SDB_DATABASE_MAIN_DETAILS           (SDB_DATABASE_DETAILS       | SDB_DATABASE_MAIN)
#define SDB_DATABASE_MAIN_SP_DETAILS        (SDB_DATABASE_SP_DETAILS    | SDB_DATABASE_MAIN)
#define SDB_DATABASE_MAIN_RESOURCE          (SDB_DATABASE_RESOURCE      | SDB_DATABASE_MAIN)

#define SDB_TAG_TYPE_NULL       0x1000
#define SDB_TAG_TYPE_BYTE       0x2000
#define SDB_TAG_TYPE_WORD       0x3000
#define SDB_TAG_TYPE_DWORD      0x4000
#define SDB_TAG_TYPE_QWORD      0x5000
#define SDB_TAG_TYPE_STRINGREF  0x6000
#define SDB_TAG_TYPE_LIST       0x7000
#define SDB_TAG_TYPE_STRING     0x8000
#define SDB_TAG_TYPE_BINARY     0x9000

// It seems that this type is "masked"
#define SDB_TAG_TYPE_MASK       0xF000

		// http://msdn2.microsoft.com/en-us/library/bb432487

#define SDB_TAG_DATABASE                    (0x0001 | SDB_TAG_TYPE_LIST)
#define SDB_TAG_LIBRARY                     (0x0002 | SDB_TAG_TYPE_LIST)
#define SDB_TAG_INEXCLUDE                   (0x0003 | SDB_TAG_TYPE_LIST)
#define SDB_TAG_SHIM                        (0x0004 | SDB_TAG_TYPE_LIST)
#define SDB_TAG_PATCH                       (0x0005 | SDB_TAG_TYPE_LIST)
#define SDB_TAG_APP                         (0x0006 | SDB_TAG_TYPE_LIST)
#define SDB_TAG_EXE                         (0x0007 | SDB_TAG_TYPE_LIST)
#define SDB_TAG_MATCHING_FILE               (0x0008 | SDB_TAG_TYPE_LIST)
#define SDB_TAG_SHIM_REF                    (0x0009 | SDB_TAG_TYPE_LIST)
#define SDB_TAG_PATCH_REF                   (0x000A | SDB_TAG_TYPE_LIST)
#define SDB_TAG_LAYER                       (0x000B | SDB_TAG_TYPE_LIST)
#define SDB_TAG_FILE                        (0x000C | SDB_TAG_TYPE_LIST)
#define SDB_TAG_APPHELP                     (0x000D | SDB_TAG_TYPE_LIST)
#define SDB_TAG_LINK                        (0x000E | SDB_TAG_TYPE_LIST)
#define SDB_TAG_DATA                        (0x000F | SDB_TAG_TYPE_LIST)
#define SDB_TAG_MSI_TRANSFORM               (0x0010 | SDB_TAG_TYPE_LIST)
#define SDB_TAG_MSI_TRANSFORM_REF           (0x0011 | SDB_TAG_TYPE_LIST)
#define SDB_TAG_MSI_PACKAGE                 (0x0012 | SDB_TAG_TYPE_LIST)
#define SDB_TAG_FLAG                        (0x0013 | SDB_TAG_TYPE_LIST)
#define SDB_TAG_MSI_CUSTOM_ACTION           (0x0014 | SDB_TAG_TYPE_LIST)
#define SDB_TAG_FLAG_REF                    (0x0015 | SDB_TAG_TYPE_LIST)
#define SDB_TAG_ACTION                      (0x0016 | SDB_TAG_TYPE_LIST)
#define SDB_TAG_LOOKUP                      (0x0017 | SDB_TAG_TYPE_LIST)
#define SDB_TAG_STRINGTABLE                 (0x0801 | SDB_TAG_TYPE_LIST)
#define SDB_TAG_INDEXES                     (0x0802 | SDB_TAG_TYPE_LIST)
#define SDB_TAG_INDEX                       (0x0803 | SDB_TAG_TYPE_LIST)

#define SDB_TAG_NAME                        (0x0001 | SDB_TAG_TYPE_STRINGREF)
#define SDB_TAG_DESCRIPTION                 (0x0002 | SDB_TAG_TYPE_STRINGREF)
#define SDB_TAG_MODULE                      (0x0003 | SDB_TAG_TYPE_STRINGREF)
#define SDB_TAG_API                         (0x0004 | SDB_TAG_TYPE_STRINGREF)
#define SDB_TAG_VENDOR                      (0x0005 | SDB_TAG_TYPE_STRINGREF)
#define SDB_TAG_APP_NAME                    (0x0006 | SDB_TAG_TYPE_STRINGREF)
#define SDB_TAG_COMMAND_LINE                (0x0008 | SDB_TAG_TYPE_STRINGREF)
#define SDB_TAG_COMPANY_NAME                (0x0009 | SDB_TAG_TYPE_STRINGREF)
#define SDB_TAG_DLLFILE                     (0x000A | SDB_TAG_TYPE_STRINGREF)
#define SDB_TAG_WILDCARD_NAME               (0x000B | SDB_TAG_TYPE_STRINGREF)
#define SDB_TAG_PRODUCT_NAME                (0x0010 | SDB_TAG_TYPE_STRINGREF)
#define SDB_TAG_PRODUCT_VERSION             (0x0011 | SDB_TAG_TYPE_STRINGREF)
#define SDB_TAG_FILE_DESCRIPTION            (0x0012 | SDB_TAG_TYPE_STRINGREF)
#define SDB_TAG_FILE_VERSION                (0x0013 | SDB_TAG_TYPE_STRINGREF)
#define SDB_TAG_ORIGINAL_FILENAME           (0x0014 | SDB_TAG_TYPE_STRINGREF)
#define SDB_TAG_INTERNAL_NAME               (0x0015 | SDB_TAG_TYPE_STRINGREF)
#define SDB_TAG_LEGAL_COPYRIGHT             (0x0016 | SDB_TAG_TYPE_STRINGREF)
#define SDB_TAG_16BIT_DESCRIPTION           (0x0017 | SDB_TAG_TYPE_STRINGREF)
#define SDB_TAG_APPHELP_DETAILS             (0x0018 | SDB_TAG_TYPE_STRINGREF)
#define SDB_TAG_LINK_URL                    (0x0019 | SDB_TAG_TYPE_STRINGREF)
#define SDB_TAG_LINK_TEXT                   (0x001A | SDB_TAG_TYPE_STRINGREF)
#define SDB_TAG_APPHELP_TITLE               (0x001B | SDB_TAG_TYPE_STRINGREF)
#define SDB_TAG_APPHELP_CONTACT             (0x001C | SDB_TAG_TYPE_STRINGREF)
#define SDB_TAG_SXS_MANIFEST                (0x001D | SDB_TAG_TYPE_STRINGREF)
#define SDB_TAG_DATA_STRING                 (0x001E | SDB_TAG_TYPE_STRINGREF)
#define SDB_TAG_MSI_TRANSFORM_FILE          (0x001F | SDB_TAG_TYPE_STRINGREF)
#define SDB_TAG_16BIT_MODULE_NAME           (0x0020 | SDB_TAG_TYPE_STRINGREF)
#define SDB_TAG_LAYER_DISPLAYNAME           (0x0021 | SDB_TAG_TYPE_STRINGREF)
#define SDB_TAG_COMPILER_VERSION            (0x0022 | SDB_TAG_TYPE_STRINGREF)
#define SDB_TAG_ACTION_TYPE                 (0x0023 | SDB_TAG_TYPE_STRINGREF)
#define SDB_TAG_EXPORT_NAME                 (0x0024 | SDB_TAG_TYPE_STRINGREF)

#define SDB_TAG_SIZE                        (0x0001 | SDB_TAG_TYPE_DWORD)
#define SDB_TAG_OFFSET                      (0x0002 | SDB_TAG_TYPE_DWORD)
#define SDB_TAG_CHECKSUM                    (0x0003 | SDB_TAG_TYPE_DWORD)
#define SDB_TAG_SHIM_TAGID                  (0x0004 | SDB_TAG_TYPE_DWORD)
#define SDB_TAG_PATCH_TAGID                 (0x0005 | SDB_TAG_TYPE_DWORD)
#define SDB_TAG_MODULE_TYPE                 (0x0006 | SDB_TAG_TYPE_DWORD)
#define SDB_TAG_VERDATEHI                   (0x0007 | SDB_TAG_TYPE_DWORD)
#define SDB_TAG_VERDATELO                   (0x0008 | SDB_TAG_TYPE_DWORD)
#define SDB_TAG_VERFILEOS                   (0x0009 | SDB_TAG_TYPE_DWORD)
#define SDB_TAG_VERFILETYPE                 (0x000A | SDB_TAG_TYPE_DWORD)
#define SDB_TAG_PE_CHECKSUM                 (0x000B | SDB_TAG_TYPE_DWORD)
#define SDB_TAG_PREVOSMAJORVER              (0x000C | SDB_TAG_TYPE_DWORD)
#define SDB_TAG_PREVOSMINORVER              (0x000D | SDB_TAG_TYPE_DWORD)
#define SDB_TAG_PREVOSPLATFORMID            (0x000E | SDB_TAG_TYPE_DWORD)
#define SDB_TAG_PREVOSBUILDNO               (0x000F | SDB_TAG_TYPE_DWORD)
#define SDB_TAG_PROBLEMSEVERITY             (0x0010 | SDB_TAG_TYPE_DWORD)
#define SDB_TAG_LANGID                      (0x0011 | SDB_TAG_TYPE_DWORD)
#define SDB_TAG_VER_LANGUAGE                (0x0012 | SDB_TAG_TYPE_DWORD)
#define SDB_TAG_ENGINE                      (0x0014 | SDB_TAG_TYPE_DWORD)
#define SDB_TAG_HTMLHELPID                  (0x0015 | SDB_TAG_TYPE_DWORD)
#define SDB_TAG_INDEX_FLAGS                 (0x0016 | SDB_TAG_TYPE_DWORD)
#define SDB_TAG_FLAGS                       (0x0017 | SDB_TAG_TYPE_DWORD)
#define SDB_TAG_DATA_VALUETYPE              (0x0018 | SDB_TAG_TYPE_DWORD)
#define SDB_TAG_DATA_DWORD                  (0x0019 | SDB_TAG_TYPE_DWORD)
#define SDB_TAG_LAYER_TAGID                 (0x001A | SDB_TAG_TYPE_DWORD)
#define SDB_TAG_MSI_TRANSFORM_TAGID         (0x001B | SDB_TAG_TYPE_DWORD)
#define SDB_TAG_LINKER_VERSION              (0x001C | SDB_TAG_TYPE_DWORD)
#define SDB_TAG_LINK_DATE                   (0x001D | SDB_TAG_TYPE_DWORD)
#define SDB_TAG_UPTO_LINK_DATE              (0x001E | SDB_TAG_TYPE_DWORD)
#define SDB_TAG_OS_SERVICE_PACK             (0x001F | SDB_TAG_TYPE_DWORD)
#define SDB_TAG_FLAG_TAGID                  (0x0020 | SDB_TAG_TYPE_DWORD)
#define SDB_TAG_RUNTIME_PLATFORM            (0x0021 | SDB_TAG_TYPE_DWORD)
#define SDB_TAG_OS_SKU                      (0x0022 | SDB_TAG_TYPE_DWORD)
#define SDB_TAG_OS_PLATFORM                 (0x0023 | SDB_TAG_TYPE_DWORD)
#define SDB_TAG_APP_NAME_RC_ID              (0x0024 | SDB_TAG_TYPE_DWORD)
#define SDB_TAG_VENDOR_NAME_RC_ID           (0x0025 | SDB_TAG_TYPE_DWORD)
#define SDB_TAG_SUMMARY_MSG_RC_ID           (0x0026 | SDB_TAG_TYPE_DWORD)
#define SDB_TAG_VISTA_SKU                   (0x0027 | SDB_TAG_TYPE_DWORD)
#define SDB_TAG_DESCRIPTION_RC_ID           (0x0028 | SDB_TAG_TYPE_DWORD)
#define SDB_TAG_PARAMETER1_RC_ID            (0x0029 | SDB_TAG_TYPE_DWORD)
#define SDB_TAG_TAGID                       (0x0801 | SDB_TAG_TYPE_DWORD)

#define SDB_TAG_STRINGTABLE_ITEM            (0x0801 | SDB_TAG_TYPE_STRING)

#define SDB_TAG_INCLUDE                     (0x0001 | SDB_TAG_TYPE_NULL)
#define SDB_TAG_GENERAL                     (0x0002 | SDB_TAG_TYPE_NULL)
#define SDB_TAG_MATCH_LOGIC_NOT             (0x0003 | SDB_TAG_TYPE_NULL)
#define SDB_TAG_APPLY_ALL_SHIMS             (0x0004 | SDB_TAG_TYPE_NULL)
#define SDB_TAG_USE_SERVICE_PACK_FILES      (0x0005 | SDB_TAG_TYPE_NULL)
#define SDB_TAG_MITIGATION_OS               (0x0006 | SDB_TAG_TYPE_NULL)
#define SDB_TAG_BLOCK_UPGRADE               (0x0007 | SDB_TAG_TYPE_NULL)
#define SDB_TAG_INCLUDEEXCLUDEDLL           (0x0008 | SDB_TAG_TYPE_NULL)

#define SDB_TAG_TIME                        (0x0001 | SDB_TAG_TYPE_QWORD)
#define SDB_TAG_BIN_FILE_VERSION            (0x0002 | SDB_TAG_TYPE_QWORD)
#define SDB_TAG_BIN_PRODUCT_VERSION         (0x0003 | SDB_TAG_TYPE_QWORD)
#define SDB_TAG_MODTIME                     (0x0004 | SDB_TAG_TYPE_QWORD)
#define SDB_TAG_FLAG_MASK_KERNEL            (0x0005 | SDB_TAG_TYPE_QWORD)
#define SDB_TAG_UPTO_BIN_PRODUCT_VERSION    (0x0006 | SDB_TAG_TYPE_QWORD)
#define SDB_TAG_DATA_QWORD                  (0x0007 | SDB_TAG_TYPE_QWORD)
#define SDB_TAG_FLAG_MASK_USER              (0x0008 | SDB_TAG_TYPE_QWORD)
#define SDB_TAG_FLAGS_NTVDM1                (0x0009 | SDB_TAG_TYPE_QWORD)
#define SDB_TAG_FLAGS_NTVDM2                (0x000A | SDB_TAG_TYPE_QWORD)
#define SDB_TAG_FLAGS_NTVDM3                (0x000B | SDB_TAG_TYPE_QWORD)
#define SDB_TAG_FLAG_MASK_SHELL             (0x000C | SDB_TAG_TYPE_QWORD)
#define SDB_TAG_UPTO_BIN_FILE_VERSION       (0x000D | SDB_TAG_TYPE_QWORD)
#define SDB_TAG_FLAG_MASK_FUSION            (0x000E | SDB_TAG_TYPE_QWORD)
#define SDB_TAG_FLAG_PROCESSPARAM           (0x000F | SDB_TAG_TYPE_QWORD)
#define SDB_TAG_FLAG_LUA                    (0x0010 | SDB_TAG_TYPE_QWORD)
#define SDB_TAG_FLAG_INSTALL                (0x0011 | SDB_TAG_TYPE_QWORD)

#define SDB_TAG_PATCH_BITS                  (0x0002 | SDB_TAG_TYPE_BINARY)
#define SDB_TAG_FILE_BITS                   (0x0003 | SDB_TAG_TYPE_BINARY)
#define SDB_TAG_EXE_ID                      (0x0004 | SDB_TAG_TYPE_BINARY)
#define SDB_TAG_DATA_BITS                   (0x0005 | SDB_TAG_TYPE_BINARY)
#define SDB_TAG_MSI_PACKAGE_ID              (0x0006 | SDB_TAG_TYPE_BINARY)
#define SDB_TAG_DATABASE_ID                 (0x0007 | SDB_TAG_TYPE_BINARY)
#define SDB_TAG_INDEX_BITS                  (0x0801 | SDB_TAG_TYPE_BINARY)

#define SDB_TAG_MATCH_MODE                  (0x0001 | SDB_TAG_TYPE_WORD)
#define SDB_TAG_TAG                         (0x0801 | SDB_TAG_TYPE_WORD)
#define SDB_TAG_INDEX_TAG                   (0x0802 | SDB_TAG_TYPE_WORD)
#define SDB_TAG_INDEX_KEY                   (0x0803 | SDB_TAG_TYPE_WORD)

	typedef enum
{
	DOS_PATH,
	NT_PATH,

} PATH_TYPE;


#define SDB_TAGID_NULL (0)
#define SDB_TAGID_ROOT (0)

typedef ULONG TAGID;

typedef ULONG TAGREF, INDEXID, TAG;
typedef PVOID HSDB;

#pragma pack(1)

typedef struct tagAPPHELP_DATA
{
	ULONG  Flags;
	ULONG  Severity;
	ULONG  HTMLHelpID;
	LPWSTR AppName;
	TAGREF Exe;
	LPWSTR URL;
	LPWSTR Link;
	LPWSTR AppTitle;
	LPWSTR Contact;
	LPWSTR Details;
	ULONG  Data;
	BOOL   SPEntry;

} APPHELP_DATA, *PAPPHELP_DATA;

typedef struct tagATTRINFO
{
	TAG   AttrID;
	ULONG Flags;

	union
	{
		ULONGLONG   Attr64;
		ULONG       Attr32;
		PWCHAR      AttrString;
	};
} ATTRINFO, *PATTRINFO;

typedef struct _FIND_INFO
{
	TAGID     tiIndex;
	TAGID     tiCurrent;
	TAGID     tiEndIndex;
	TAG       tName;
	ULONG     dwIndexRec;
	ULONG     dwFlags;
	ULONGLONG ullKey;
	union
	{
		LPCWSTR szName;
		ULONG   dwName;
		GUID    *pguidName;
	};
} FIND_INFO, *PFIND_INFO;

#define SDB_MAX_EXES    16
#define SDB_MAX_LAYERS  8
#define SDB_MAX_SDBS    16

// Flags

#define SHIMREG_DISABLE_SHIM    (0x00000001u)
#define SHIMREG_DISABLE_APPHELP (0x00000002u)
#define SHIMREG_APPHELP_NOUI    (0x00000004u)
#define SHIMREG_APPHELP_CANCEL  (0x10000000u)
#define SHIMREG_DISABLE_SXS     (0x00000010u)
#define SHIMREG_DISABLE_LAYER   (0x00000020u)
#define SHIMREG_DISABLE_DRIVER  (0x00000040u)

typedef struct tagSDBQUERYRESULT
{
	TAGREF atrExes[SDB_MAX_EXES];
	ULONG  adwExeFlags[SDB_MAX_EXES];
	TAGREF atrLayers[SDB_MAX_LAYERS];
	ULONG  dwLayerFlags;
	TAGREF trApphelp;
	ULONG  dwExeCount;
	ULONG  dwLayerCount;
	GUID   guidID;
	ULONG  dwFlags;
	ULONG  dwCustomSDBMap;
	GUID   rgGuidDB[SDB_MAX_SDBS];
} SDBQUERYRESULT, *PSDBQUERYRESULT;

typedef struct      // 0x1E8, win7 ultimate SP1 x86
{
	HANDLE  DataBaseFileHandle;
	PVOID   MappedBase;
	ULONG   Unknown;
	ULONG   FileSize;
	BYTE    DataBaseId[0x14];

	HANDLE  SectionHandle;  // 0x1B8

} DB, *PDB;

typedef struct
{
	ULONG   Flags;
	GUID    Guid;
} SDB_DATABASE_INFORMATION, *PSDB_DATABASE_INFORMATION;

#pragma pack()

EXTC_IMPORT BOOL WINAPI BaseFlushAppcompatCache();

EXTC_IMPORT
VOID
WINAPI
SdbCloseDatabase(
	IN  PDB Db
);

EXTC_IMPORT
VOID
WINAPI
SdbCloseDatabaseWrite(
	IN  PDB Db
);

EXTC_IMPORT
BOOL
WINAPI
SdbCommitIndexes(
	IN  PDB Db
);

EXTC_IMPORT
PDB
WINAPI
SdbCreateDatabase(
	IN  PCWSTR      DataBase,
	IN  PATH_TYPE   Type
);

EXTC_IMPORT
BOOL
WINAPI
SdbDeclareIndex(
	IN  PDB pdb,
	IN  TAG tWhich,
	IN  TAG tKey,
	IN  ULONG dwEntries,
	IN  BOOL bUniqueKey,
	OUT INDEXID *piiIndex
);

EXTC_IMPORT
TAGID
WINAPI
SdbBeginWriteListTag(
	IN PDB      Db,
	IN TAGID    ListTag
);

EXTC_IMPORT
BOOL
WINAPI
SdbEndWriteListTag(
	IN OUT  PDB     Db,
	IN      TAGID   ListTag
);

EXTC_IMPORT
TAGID
WINAPI
SdbFindFirstDWORDIndexedTag(
	IN  PDB pdb,
	IN  TAG tWhich,
	IN  TAG tKey,
	IN  ULONG dwName,
	OUT FIND_INFO *pFindInfo
);

EXTC_IMPORT
TAGID
WINAPI
SdbFindFirstTag(
	IN  PDB pdb,
	IN  TAGID tiParent,
	IN  TAG tTag
);

EXTC_IMPORT
TAGID
WINAPI
SdbFindFirstNamedTag(
	IN  PDB     Db,
	IN  TAGID   TagId,
	IN  TAGID   Parent,
	IN  TAG     Tag
);

EXTC_IMPORT
TAGREF
WINAPI
SdbFindFirstTagRef(
	IN  PDB     Db,
	IN  TAGREF  TagRef,
	IN  TAG     Tag
);

EXTC_IMPORT
TAGID
WINAPI
SdbFindNextTag(
	IN  PDB pdb,
	IN  TAGID tiParent,
	IN  TAGID tiPrev
);

EXTC_IMPORT
BOOL
WINAPI
SdbFormatAttribute(
	IN  PATTRINFO pAttrInfo,
	OUT LPWSTR pchBuffer,
	IN  ULONG dwBufferSize
);

EXTC_IMPORT
BOOL
WINAPI
SdbFreeFileAttributes(
	IN  PATTRINFO pFileAttributes
);

EXTC_IMPORT
VOID
WINAPI
SdbGetAppPatchDir(
	IN  HSDB hSDB OPTIONAL,
	OUT LPWSTR szAppPatchPath,
	IN  ULONG cchSize
);

EXTC_IMPORT
PVOID
WINAPI
SdbGetBinaryTagData(
	IN  PDB     Db,
	IN  TAGID   Which
);

EXTC_IMPORT
BOOL
WINAPI
SdbGetDatabaseID(
	IN  PDB     Db,
	OUT GUID   *Guid
);

EXTC_IMPORT
BOOL
WINAPI
SdbGetDatabaseGUID(
	IN  PDB     Db,
	OUT GUID   *Guid
);

EXTC_IMPORT
BOOL
WINAPI
SdbGetDatabaseInformation(
	IN  PDB                         Db,
	OUT PSDB_DATABASE_INFORMATION   DatabaseInformation
);

EXTC_IMPORT
BOOL
WINAPI
SdbGetFileAttributes(
	IN  LPCWSTR lpwszFileName,
	OUT PATTRINFO *ppAttrInfo,
	OUT LPDWORD lpdwAttrCount
);

EXTC_IMPORT
TAGID
WINAPI
SdbGetFirstChild(
	IN  PDB     Db,
	IN  TAGID   ParentTagId
);

EXTC_IMPORT
TAGID
WINAPI
SdbGetIndex(
	IN  PDB pdb,
	IN  TAG tWhich,
	IN  TAG tKey,
	OUT LPDWORD lpdwFlags OPTIONAL
);

EXTC_IMPORT
BOOL
WINAPI
SdbGetMatchingExe(
	IN  HSDB    hSDB OPTIONAL,
	IN  LPCWSTR szPath,
	IN  LPCWSTR szModuleName OPTIONAL,
	IN  LPCWSTR pszEnvironment OPTIONAL,
	IN  ULONG dwFlags,
	OUT PSDBQUERYRESULT pQueryResult
);

EXTC_IMPORT
TAGID
WINAPI
SdbGetNextChild(
	IN  PDB pdb,
	IN  TAGID tiParent,
	IN  TAGID tiPrev
);

EXTC_IMPORT
PWSTR
WINAPI
SdbGetStringTagPtr(
	IN  PDB pdb,
	IN  TAGID WhichTagId
);

EXTC_IMPORT
TAG
WINAPI
SdbGetTagFromTagID(
	IN  PDB pdb,
	IN  TAGID tiWhich
);

EXTC_IMPORT
HSDB
WINAPI
SdbInitDatabase(
	IN  ULONG   Flags,
	IN  PCWSTR  DatabasePath
);

EXTC_IMPORT
BOOL
WINAPI
SdbIsStandardDatabase(
	IN  GUID GuidDB
);

EXTC_IMPORT
ULONGLONG
WINAPI
SdbMakeIndexKeyFromString(
	IN  LPCWSTR pwszKey
);

EXTC_IMPORT
PDB
WINAPI
SdbOpenApphelpDetailsDatabase(
	IN OUT  LPCWSTR pwsDetailsDatabasePath
);

EXTC_IMPORT
HMODULE
WINAPI
SdbOpenApphelpResourceFile(
	IN  LPCWSTR pwszACResourceFile OPTIONAL
);

EXTC_IMPORT
PDB
WINAPI
SdbOpenDatabase(
	IN  LPCWSTR pwszPath,
	IN  PATH_TYPE eType
);

EXTC_IMPORT
ULONG
WINAPI
SdbQueryDataExTagID(
	IN      PDB pdb,
	IN      TAGID tiExe,
	IN      LPCWSTR lpszDataName OPTIONAL,
	OUT     LPDWORD lpdwDataType OPTIONAL,
	OUT     LPVOID lpBuffer OPTIONAL,
	IN OUT  LPDWORD lpcbBufferSize OPTIONAL,
	OUT     TAGID *ptiData OPTIONAL
);

EXTC_IMPORT
BOOL
WINAPI
SdbReadApphelpDetailsData(
	IN  PDB pdb,
	OUT PAPPHELP_DATA pData
);

EXTC_IMPORT
BOOL
WINAPI
SdbReadBinaryTag(
	IN  PDB     Db,
	IN  TAGID   WhichTagId,
	OUT PBYTE   Buffer,
	IN  ULONG   BufferSize
);

EXTC_IMPORT
ULONG
WINAPI
SdbReadDWORDTag(
	IN  PDB pdb,
	IN  TAGID tiWhich,
	IN  ULONG dwDefault
);

EXTC_IMPORT
ULONGLONG
WINAPI
SdbReadQWORDTag(
	IN  PDB pdb,
	IN  TAGID tiWhich,
	IN  ULONGLONG qwDefault
);

EXTC_IMPORT
BOOL
WINAPI
SdbReadStringTag(
	IN  PDB pdb,
	IN  TAGID tiWhich,
	OUT LPWSTR pwszBuffer,
	IN  ULONG cchBufferSize
);

EXTC_IMPORT
BOOL
WINAPI
SdbRegisterDatabaseEx(
	IN  LPCWSTR pszDatabasePath,
	IN  ULONG dwDatabaseType,
	IN  PULONGLONG pTimeStamp OPTIONAL
);

EXTC_IMPORT
VOID
WINAPI
SdbReleaseDatabase(
	IN  HSDB hSDB
);

EXTC_IMPORT
VOID
WINAPI
SdbReleaseMatchingExe(
	IN  HSDB hSDB,
	IN  TAGREF trExe
);

EXTC_IMPORT
BOOL
WINAPI
SdbStartIndexing(
	IN  PDB pdb,
	IN  INDEXID iiWhich
);

EXTC_IMPORT
BOOL
WINAPI
SdbStopIndexing(
	IN  PDB pdb,
	IN  INDEXID WhichIndexId
);

EXTC_IMPORT
BOOL
WINAPI
SdbTagIDToTagRef(
	IN  HSDB    hSDB,
	IN  PDB     Db,
	IN  TAGID   TagId,
	OUT TAGREF *TagRef
);

EXTC_IMPORT
BOOL
WINAPI
SdbTagRefToTagID(
	IN  HSDB    hSDB,
	IN  TAGREF  WhichTagRef,
	OUT PDB    *Db,
	OUT TAGID  *WhichTagId
);

EXTC_IMPORT
LPCWSTR
WINAPI
SdbTagToString(
	IN  TAG tag
);

EXTC_IMPORT
BOOL
WINAPI
SdbUnregisterDatabase(
	IN  GUID *pguidDB
);

EXTC_IMPORT
BOOL
WINAPI
SdbWriteBinaryTag(
	IN  PDB     Db,
	IN  TAG     Tag,
	IN  PVOID   Buffer,
	IN  ULONG   Size
);

EXTC_IMPORT
BOOL
WINAPI
SdbWriteBinaryTagFromFile(
	IN  PDB     Db,
	IN  TAG     Tag,
	IN  PCWSTR  FileName
);

EXTC_IMPORT
BOOL
WINAPI
SdbWriteDWORDTag(
	IN  PDB     Db,
	IN  TAG     Tag,
	IN  ULONG   Data
);

EXTC_IMPORT
BOOL
WINAPI
SdbWriteNULLTag(
	IN  PDB Db,
	IN  TAG Tag
);

EXTC_IMPORT
BOOL
WINAPI
SdbWriteQWORDTag(
	IN  PDB     Db,
	IN  TAG     Tag,
	IN  ULONG64 Data
);

EXTC_IMPORT
BOOL
WINAPI
SdbWriteStringTag(
	IN  PDB     Db,
	IN  TAG     Tag,
	IN  PCWSTR  Data
);

EXTC_IMPORT
BOOL
WINAPI
SdbWriteWORDTag(
	IN  PDB     Db,
	IN  TAG     Tag,
	IN  USHORT  Data
);

EXTC_IMPORT
BOOL
WINAPI
ShimFlushCache(
	IN OUT  HWND hwnd,
	IN OUT  HINSTANCE hInstance,
	IN OUT  LPCSTR lpszCmdLine,
	IN      int nCmdShow
);

_ML_C_TAIL_


_ML_C_HEAD_

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

PVOID
GetImageBaseAddress(
	PVOID ImageAddress
);

BOOL
ValidateDataDirectory(
	PIMAGE_DATA_DIRECTORY   DataDirectory,
	ULONG_PTR               SizeOfImage
);

BOOL
IsValidImage(
	PVOID       ImageBase,
	ULONG_PTR   DEF_VAL(Flags, 0)
);

#if !defined(_MY_STATIC_LIB_)

EXTC IMAGE_DOS_HEADER __ImageBase;

#endif // _MY_STATIC_LIB_


#pragma pack(push, 1)

typedef struct
{
	BYTE    Width;          // Width, in pixels, of the image
	BYTE    Height;         // Height, in pixels, of the image
	BYTE    ColorCount;     // Number of colors in image (0 if >=8bpp)
	BYTE    Reserved;       // Reserved
	WORD    Planes;         // Color Planes
	WORD    BitCount;       // Bits per pixel
	DWORD   BytesInRes;     // how many bytes in this resource?
	WORD    ID;             // the ID

} GROUP_ICON_ENTRY, *PGROUP_ICON_ENTRY;

typedef struct
{
	WORD                Reserved;   // Reserved (must be 0)
	WORD                Type;       // Resource type (1 for icons)
	WORD                Count;      // How many images?
	GROUP_ICON_ENTRY    Entries[1]; // The entries for each image

} GROUP_ICON, *PGROUP_ICON;

#pragma pack(pop)

typedef struct _RELOCATE_ADDRESS_INFO
{
	PVOID AddressToRelocate;
	PVOID NewAddress;
	BOOL(*CanRelocate)(struct _RELOCATE_ADDRESS_INFO *Address, PVOID PointerToAddress);
	PVOID Context;

} RELOCATE_ADDRESS_INFO, *PRELOCATE_ADDRESS_INFO;

NTSTATUS
RelocateAddress(
	LDR_MODULE              *ImageModule,
	PRELOCATE_ADDRESS_INFO  Addresses,
	ULONG                   Count
);

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

inline PVOID IATLookupRoutineByEntry(PVOID ImageBase, PVOID RoutineEntry)
{
	return IATLookupRoutineByEntryNoFix(GetImageBaseAddress(ImageBase), RoutineEntry);
}

inline ULONG_PTR IATLookupRoutineRVAByEntry(PVOID ImageBase, PVOID RoutineEntry)
{
	ImageBase = GetImageBaseAddress(ImageBase);
	if (ImageBase == nullptr)
		return IMAGE_INVALID_RVA;

	RoutineEntry = IATLookupRoutineByEntryNoFix(ImageBase, RoutineEntry);
	if (RoutineEntry == nullptr)
		return IMAGE_INVALID_RVA;

	return (ULONG_PTR)RoutineEntry - (ULONG_PTR)(ImageBase);
}

inline PVOID IATLookupRoutineByHash(PVOID ImageBase, ULONG_PTR Hash)
{
	ImageBase = GetImageBaseAddress(ImageBase);
	if (ImageBase == nullptr)
		return nullptr;

	Hash = IATLookupRoutineRVAByHashNoFix(ImageBase, Hash);

	return Hash == IMAGE_INVALID_RVA ? (PVOID)Hash : PtrAdd(ImageBase, Hash);
}

#define LOAD_MEM_DLL_INFO_MAGIC  TAG4('LMDI')

#define LMD_REMOVE_PE_HEADER        0x00000001
#define LMD_REMOVE_IAT              0x00000002
#define LMD_REMOVE_EAT              0x00000004
#define LMD_REMOVE_RES              0x00000008
#define LMD_IGNORE_IAT_DLL_MISSING  0x00000010
#define LMD_MAPPED_DLL              0x10000000

NTSTATUS
LoadDllFromMemory(
	PVOID           DllBuffer,
	ULONG           DllBufferSize,
	PUNICODE_STRING ModuleFileName,
	PVOID*          ModuleHandle = nullptr,
	ULONG           Flags = 0
);

NTSTATUS
UnmapDllSection(
	PVOID BaseAddress
);

_ML_C_TAIL_


ULONG_PTR GetOpCodeSize_2(PVOID Buffer);
ULONG_PTR GetOpCodeSize32_2(PVOID Buffer);
ULONG_PTR GetOpCodeSize64_2(PVOID Buffer);

PVOID LookupImportTable(PVOID ImageBase, PCSTR DllName, PCSTR RoutineName);
PVOID LookupImportTable(PVOID ImageBase, PCSTR DllName, ULONG Hash);

PVOID LookupExportTable(PVOID ImageBase, ULONG Hash);
PVOID LookupExportTable(PVOID ImageBase, PCSTR RoutineName);

inline PIMAGE_NT_HEADERS ImageNtHeadersFast(PVOID ImageBase, PULONG_PTR NtHeadersVersion = nullptr)
{
	PIMAGE_NT_HEADERS32 NtHeaders32;

	NtHeaders32 = (PIMAGE_NT_HEADERS32)PtrAdd(ImageBase, ((PIMAGE_DOS_HEADER)ImageBase)->e_lfanew);

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

inline PIMAGE_NT_HEADERS ImageFileNtHeaders(PVOID ImageBase, PULONG_PTR NtHeadersVersion = nullptr)
{
	return ImageNtHeadersFast(ImageBase, NtHeadersVersion);
}


inline ULONG_PTR ImageGetSizeOfImage(PVOID ImageBase)
{
	ULONG_PTR Version;
	PIMAGE_NT_HEADERS NtHeaders;

	NtHeaders = ImageNtHeaders(ImageBase, &Version);

	switch (Version)
	{
	case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
		return ((PIMAGE_NT_HEADERS32)NtHeaders)->OptionalHeader.SizeOfImage;

	case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
		return ((PIMAGE_NT_HEADERS32)NtHeaders)->OptionalHeader.SizeOfImage;
	}

	return 0;
}

typedef
NTSTATUS
(*WalkRelocTableCallback)(
	PVOID                           ImageBase,
	PIMAGE_BASE_RELOCATION2         RelocationEntry,
	PIMAGE_RELOCATION_ADDRESS_ENTRY Offset,
	PVOID                           Context
	);

#define WalkRelocCallbackM(ImageBase, RelocationEntry, Offset, Context) [&] (PVOID ImageBase, PIMAGE_BASE_RELOCATION2 RelocationEntry, PIMAGE_RELOCATION_ADDRESS_ENTRY Offset, PVOID Context) -> NTSTATUS

template<class CallbackRoutine>
inline NTSTATUS WalkRelocTableT(PVOID ImageBase, CallbackRoutine Callback, PVOID Context = nullptr)
{
	PIMAGE_DOS_HEADER           DosHeader;
	PIMAGE_NT_HEADERS32         NtHeaders32;
	PIMAGE_NT_HEADERS64         NtHeaders64;
	PIMAGE_DATA_DIRECTORY       RelocDirectory;
	PIMAGE_BASE_RELOCATION2     Relocation, RelocationEnd;
	ULONG_PTR                   SizeOfImage;

	DosHeader = (PIMAGE_DOS_HEADER)ImageBase;
	NtHeaders32 = (PIMAGE_NT_HEADERS32)PtrAdd(DosHeader, DosHeader->e_lfanew);
	NtHeaders64 = nullptr;

	switch (NtHeaders32->OptionalHeader.Magic)
	{
	case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
		SizeOfImage = NtHeaders32->OptionalHeader.SizeOfImage;
		RelocDirectory = &NtHeaders32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
		break;

	case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
		NtHeaders64 = (PIMAGE_NT_HEADERS64)NtHeaders32;
		SizeOfImage = NtHeaders64->OptionalHeader.SizeOfImage;
		RelocDirectory = &NtHeaders64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
		break;

	default:
		return STATUS_INVALID_IMAGE_FORMAT;
	}

	if (!ValidateDataDirectory(RelocDirectory, NtHeaders32->OptionalHeader.SizeOfImage))
		return STATUS_INVALID_IMAGE_FORMAT;

	Relocation = (PIMAGE_BASE_RELOCATION2)PtrAdd(ImageBase, RelocDirectory->VirtualAddress);
	RelocationEnd = PtrAdd(Relocation, RelocDirectory->Size);

	while (Relocation < RelocationEnd)
	{
		ULONG_PTR   SizeOfBlock;
		PVOID       RelocateBase;
		NTSTATUS    Status;
		PIMAGE_RELOCATION_ADDRESS_ENTRY TypeOffset;

		TypeOffset = Relocation->TypeOffset;
		SizeOfBlock = Relocation->SizeOfBlock;
		RelocateBase = PtrAdd(ImageBase, Relocation->VirtualAddress);

		if (SizeOfBlock < sizeof(*Relocation) - sizeof(Relocation->TypeOffset) ||
			SizeOfBlock >= SizeOfImage ||
			PtrAnd(RelocateBase, 0xFFF) != 0)
		{
			return STATUS_ILLEGAL_DLL_RELOCATION;
		}

		SizeOfBlock -= sizeof(*Relocation) - sizeof(Relocation->TypeOffset);

		for (; SizeOfBlock > 0; ++TypeOffset, SizeOfBlock -= sizeof(*TypeOffset))
		{
			Status = Callback(ImageBase, Relocation, TypeOffset, Context);
			if (!NT_SUCCESS(Status))
				return Status;
		}

		Relocation = PtrAdd(Relocation, Relocation->SizeOfBlock);
	}

	return STATUS_SUCCESS;
}

ForceInline NTSTATUS WalkRelocTableInternal(PVOID ImageBase, WalkRelocTableCallback Callback, PVOID Context)
{
	return WalkRelocTableT(ImageBase, Callback, Context);
}

template<class CallbackRoutine, class CallbackContext>
ForceInline NTSTATUS WalkRelocTable(PVOID ImageBase, CallbackRoutine Callback, CallbackContext Context)
{
	return WalkRelocTableInternal(ImageBase, (WalkRelocTableCallback)Callback, (PVOID)Context);
}

typedef struct
{
	PVOID                       ImageBase;
	PIMAGE_IMPORT_DESCRIPTOR    ImportDescriptor;

	union
	{
		PIMAGE_THUNK_DATA       ThunkData;
		PIMAGE_THUNK_DATA32     ThunkData32;
		PIMAGE_THUNK_DATA64     ThunkData64;
	};

	PCSTR                       DllName;
	PCSTR                       FunctionName;
	ULONG_PTR                   Ordinal;
	PVOID                       Context;

} WALK_IMPORT_TABLE_DATA, *PWALK_IMPORT_TABLE_DATA;

typedef NTSTATUS(*WalkImportTableCallback)(PWALK_IMPORT_TABLE_DATA Data);

#define WalkIATCallbackM(Data) [&] (PWALK_IMPORT_TABLE_DATA Data) -> NTSTATUS

typedef struct
{
	PVOID                       ImageBase;
	PIMAGE_IMPORT_DESCRIPTOR    ImportDescriptor;
	PVOID                       EndOfImage;
	PVOID                       EndOfTable;
	ULONG_PTR                   SizeOfImage;
	ULONG_PTR                   SizeOfTable;
	ULONG_PTR                   NtHeadersVersion;

} WALK_IMPORT_TABLE_INTERNAL_DATA, *PWALK_IMPORT_TABLE_INTERNAL_DATA;

template<class CallbackRoutine, class CallbackContext, class PIMAGE_THUNK_DATA_TYPE>
inline NTSTATUS WalkImportTableInternal(PWALK_IMPORT_TABLE_INTERNAL_DATA InternalData, CallbackRoutine Callback, CallbackContext Context)
{
	NTSTATUS                    Status;
	PIMAGE_THUNK_DATA_TYPE      OriginalThunk, FirstThunk;
	PIMAGE_IMPORT_DESCRIPTOR    ImportDescriptor;

	ImportDescriptor = InternalData->ImportDescriptor;

	for (; ImportDescriptor->Name != NULL && ImportDescriptor->FirstThunk != NULL; ++ImportDescriptor)
	{
		LONG_PTR DllName;

		if (ImportDescriptor->FirstThunk > InternalData->SizeOfImage)
			continue;

		if (*(PULONG_PTR)PtrAdd(InternalData->ImageBase, ImportDescriptor->FirstThunk) == NULL)
			continue;

		OriginalThunk = (PIMAGE_THUNK_DATA_TYPE)InternalData->ImageBase;
		if (ImportDescriptor->OriginalFirstThunk != NULL)
		{
			OriginalThunk = PtrAdd(OriginalThunk, ImportDescriptor->OriginalFirstThunk);
		}
		else
		{
			OriginalThunk = PtrAdd(OriginalThunk, ImportDescriptor->FirstThunk);
		}

		if (OriginalThunk >= InternalData->EndOfImage)
			continue;

		DllName = PtrAdd((LONG_PTR)InternalData->ImageBase, ImportDescriptor->Name);
		if ((PVOID)DllName >= InternalData->EndOfImage)
			continue;

		FirstThunk = (PIMAGE_THUNK_DATA_TYPE)PtrAdd(InternalData->ImageBase, ImportDescriptor->FirstThunk);
		while (OriginalThunk->u1.AddressOfData != NULL)
		{
			LONG_PTR    FunctionName;
			ULONG_PTR   Ordinal;

			FunctionName = (LONG_PTR)OriginalThunk->u1.AddressOfData;
			if (FunctionName < 0)
			{
				Ordinal = (USHORT)FunctionName;
				FunctionName = NULL;
			}
			else
			{
				Ordinal = IMAGE_INVALID_ORDINAL;
				FunctionName += (LONG_PTR)PtrAdd(InternalData->ImageBase, 2);
			}

			WALK_IMPORT_TABLE_DATA Data;

			Data.ImageBase = InternalData->ImageBase;
			Data.ImportDescriptor = ImportDescriptor;
			Data.ThunkData = (PIMAGE_THUNK_DATA)FirstThunk;
			Data.DllName = (PCSTR)DllName;
			Data.Ordinal = Ordinal;
			Data.FunctionName = (PCSTR)FunctionName;
			Data.Context = (PVOID)(ULONG_PTR)Context;

			Status = Callback(&Data);
			if (Status == STATUS_VALIDATE_CONTINUE)
				break;

			FAIL_RETURN(Status);

			++OriginalThunk;
			++FirstThunk;
		}
	}

	return STATUS_SUCCESS;
}

template<class CallbackRoutine, class CallbackContext>
inline NTSTATUS WalkImportTableT(PVOID ImageBase, CallbackRoutine Callback, CallbackContext Context = nullptr)
{
	ULONG_PTR                   NtHeadersVersion;
	NTSTATUS                    Status;
	PIMAGE_NT_HEADERS32         NtHeaders32;
	PIMAGE_NT_HEADERS64         NtHeaders64;

	WALK_IMPORT_TABLE_INTERNAL_DATA InternalData;

	if (!IsValidImage(ImageBase, IMAGE_VALID_IMPORT_ADDRESS_TABLE))
		return STATUS_INVALID_IMAGE_FORMAT;

	NtHeaders32 = (PIMAGE_NT_HEADERS32)ImageNtHeaders(ImageBase, &NtHeadersVersion);
	NtHeaders64 = (PIMAGE_NT_HEADERS64)NtHeaders32;

	InternalData.ImageBase = ImageBase;
	InternalData.ImportDescriptor = nullptr;

	switch (NtHeadersVersion)
	{
	case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
		InternalData.ImportDescriptor = PtrAdd(InternalData.ImportDescriptor, NtHeaders32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
		InternalData.SizeOfTable = NtHeaders32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size;
		InternalData.SizeOfImage = NtHeaders32->OptionalHeader.SizeOfImage;
		break;

	case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
		InternalData.ImportDescriptor = PtrAdd(InternalData.ImportDescriptor, NtHeaders64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
		InternalData.SizeOfTable = NtHeaders64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size;
		InternalData.SizeOfImage = NtHeaders64->OptionalHeader.SizeOfImage;
		break;

	default:
		return STATUS_INVALID_IMAGE_FORMAT;
	}

	InternalData.ImportDescriptor = PtrAdd(InternalData.ImportDescriptor, ImageBase);
	InternalData.EndOfImage = PtrAdd(ImageBase, InternalData.SizeOfImage);
	InternalData.EndOfTable = PtrAdd(InternalData.ImportDescriptor, InternalData.SizeOfTable);

	switch (NtHeadersVersion)
	{
	case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
		return WalkImportTableInternal<CallbackRoutine, CallbackContext, PIMAGE_THUNK_DATA32>(&InternalData, Callback, Context);

	case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
		return WalkImportTableInternal<CallbackRoutine, CallbackContext, PIMAGE_THUNK_DATA64>(&InternalData, Callback, Context);
	}

	return STATUS_INVALID_IMAGE_FORMAT;
}

typedef struct
{
	PVOID       ImageBase;
	PULONG      AddressOfFunction;
	PCSTR       DllName;
	ULONG_PTR   Ordinal;
	PCSTR       FunctionName;
	PVOID       Context;
	BOOL        IsForward;

} WALK_EXPORT_TABLE_DATA, *PWALK_EXPORT_TABLE_DATA;

typedef NTSTATUS(*WalkExportTableCallback)(PWALK_EXPORT_TABLE_DATA Data);

#define WalkEATCallbackM(Data) [&] (PWALK_EXPORT_TABLE_DATA Data) -> NTSTATUS

template<class CallbackRoutine, class CallbackContext>
inline NTSTATUS WalkExportTableT(PVOID ImageBase, CallbackRoutine Callback, CallbackContext Context = nullptr)
{
	PCSTR                       DllName;
	ULONG_PTR                   Ordinal, OrdinalBase, SizeOfTable, SizeOfImage, NtHeadersVersion;
	NTSTATUS                    Status;
	PIMAGE_NT_HEADERS32         NtHeaders32;
	PIMAGE_NT_HEADERS64         NtHeaders64;
	PIMAGE_EXPORT_DIRECTORY     ExportDirectory;
	ULONG_PTR                   NumberOfNames, NumberOfFunctions, NumberOfOrdinals, MaskSize;
	PULONG                      AddressOfFuntions;
	PULONG                      WalkedMask;
	PUSHORT                     AddressOfNameOrdinals;
	PCSTR                      *AddressOfNames;
	PVOID                       EndOfImage, EndOfTable;

	if (!IsValidImage(ImageBase, IMAGE_VALID_EXPORT_ADDRESS_TABLE))
		return STATUS_INVALID_IMAGE_FORMAT;

	NtHeaders32 = (PIMAGE_NT_HEADERS32)ImageNtHeaders(ImageBase, &NtHeadersVersion);
	NtHeaders64 = (PIMAGE_NT_HEADERS64)NtHeaders32;
	if (NtHeaders32 == nullptr)
		return STATUS_INVALID_IMAGE_FORMAT;

	ExportDirectory = nullptr;
	ExportDirectory = PtrAdd(ExportDirectory, ImageBase);

	switch (NtHeadersVersion)
	{
	case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
		ExportDirectory = PtrAdd(ExportDirectory, NtHeaders32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
		SizeOfTable = NtHeaders32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
		SizeOfImage = NtHeaders32->OptionalHeader.SizeOfImage;
		break;

	case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
		ExportDirectory = PtrAdd(ExportDirectory, NtHeaders64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
		SizeOfTable = NtHeaders64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
		SizeOfImage = NtHeaders64->OptionalHeader.SizeOfImage;
		break;

	default:
		return STATUS_INVALID_IMAGE_FORMAT;
	}

	EndOfTable = PtrAdd(ExportDirectory, SizeOfTable);
	EndOfImage = PtrAdd(ImageBase, SizeOfImage);
	NumberOfNames = ExportDirectory->NumberOfNames;
	NumberOfFunctions = ExportDirectory->NumberOfFunctions;
	NumberOfOrdinals = NumberOfFunctions - NumberOfNames;
	AddressOfFuntions = PtrAdd((PULONG)ImageBase, ExportDirectory->AddressOfFunctions);
	AddressOfNames = PtrAdd((PCSTR *)ImageBase, ExportDirectory->AddressOfNames);
	AddressOfNameOrdinals = PtrAdd((PUSHORT)ImageBase, ExportDirectory->AddressOfNameOrdinals);
	DllName = (PCSTR)ImageBase + ExportDirectory->Name;

	if (AddressOfFuntions >= EndOfImage)
	{
		return STATUS_INVALID_IMAGE_WIN_32;
	}

	AddressOfNames = AddressOfNames >= EndOfImage ? nullptr : AddressOfNames;
	AddressOfNameOrdinals = AddressOfNameOrdinals >= EndOfImage ? nullptr : AddressOfNameOrdinals;

	OrdinalBase = ExportDirectory->Base;

	MaskSize = ROUND_UP((NumberOfFunctions + OrdinalBase + 1), bitsof(*WalkedMask)) / bitsof(*WalkedMask);
	MaskSize = MaskSize * sizeof(*WalkedMask);
	WalkedMask = (PULONG)AllocStack(MaskSize);
	if (WalkedMask == nullptr)
		return STATUS_NO_MEMORY;

	ZeroMemory(WalkedMask, MaskSize);

	if (AddressOfNameOrdinals != nullptr && AddressOfNames != nullptr)
		for (; NumberOfNames; ++AddressOfNames, ++AddressOfNameOrdinals, --NumberOfNames)
		{
			PCSTR       FunctionName;
			ULONG_PTR   Index, Mask;
			PULONG      Function;

			FunctionName = PtrAdd((PCSTR)ImageBase, *AddressOfNames);
			Ordinal = *AddressOfNameOrdinals;

			if (Ordinal >= NumberOfFunctions)
				continue;

			Function = AddressOfFuntions + *AddressOfNameOrdinals;
			if (*Function >= SizeOfImage || *Function == NULL)
				continue;

			Ordinal += OrdinalBase;

			Index = Ordinal / bitsof(*WalkedMask);
			Mask = (ULONG_PTR)(1 << ((Ordinal + 1) % bitsof(*WalkedMask) - 1));
			WalkedMask[Index] |= Mask;

			WALK_EXPORT_TABLE_DATA Data;

			Data.ImageBase = ImageBase;
			Data.AddressOfFunction = Function;
			Data.DllName = DllName;
			Data.Ordinal = Ordinal;
			Data.FunctionName = FunctionName;
			Data.Context = (PVOID)(ULONG_PTR)Context;
			Data.IsForward = PtrAdd(ImageBase, *Function) >= ExportDirectory && PtrAdd(ImageBase, *Function) < EndOfTable;

			Status = Callback(&Data);
			if (!NT_SUCCESS(Status))
			{
				//FreeMemory(WalkedMask);
				return Status;
			}
		}

	Ordinal = OrdinalBase;

	for (; NumberOfFunctions; ++AddressOfFuntions, ++Ordinal, --NumberOfFunctions)
	{
		if (FLAG_ON(WalkedMask[Ordinal / bitsof(*WalkedMask)], 1 << ((Ordinal + 1) % bitsof(*WalkedMask) - 1)))
		{
			continue;
		}

		if (*AddressOfFuntions == NULL)
			continue;

		WALK_EXPORT_TABLE_DATA Data;

		Data.ImageBase = ImageBase;
		Data.AddressOfFunction = AddressOfFuntions;
		Data.DllName = DllName;
		Data.Ordinal = Ordinal;
		Data.FunctionName = nullptr;
		Data.Context = (PVOID)(ULONG_PTR)Context;
		Data.IsForward = PtrAdd(ImageBase, *AddressOfFuntions) >= ExportDirectory && PtrAdd(ImageBase, *AddressOfFuntions) < EndOfTable;

		Status = Callback(&Data);
		if (!NT_SUCCESS(Status))
		{
			//FreeMemory(WalkedMask);
			return Status;
		}
	}

	//FreeMemory(WalkedMask);
	return STATUS_SUCCESS;
}

ForceInline NTSTATUS WalkExportTableInternal(PVOID ImageBase, WalkExportTableCallback Callback, PVOID Context)
{
	return WalkExportTableT(ImageBase, Callback, Context);
}

template<class CallbackRoutine, class CallbackContext>
ForceInline NTSTATUS WalkExportTable(PVOID ImageBase, CallbackRoutine Callback, CallbackContext Context)
{
	return WalkExportTableInternal(ImageBase, (WalkExportTableCallback)Callback, (PVOID)Context);
}

#define WalkDITCallbackM(Data) [&] (PWALK_IMPORT_TABLE_DATA Data) -> NTSTATUS

template<class CallbackRoutine, class CallbackContext>
inline NTSTATUS WalkDelayImportT(PVOID ImageBase, CallbackRoutine Callback, CallbackContext Context = nullptr)
{
	ULONG_PTR                   NtHeadersVersion;
	NTSTATUS                    Status;
	PIMAGE_NT_HEADERS32         NtHeaders32;
	PIMAGE_NT_HEADERS64         NtHeaders64;

	WALK_IMPORT_TABLE_INTERNAL_DATA InternalData;

	if (!IsValidImage(ImageBase, IMAGE_VALID_IMPORT_ADDRESS_TABLE))
		return STATUS_INVALID_IMAGE_FORMAT;

	NtHeaders32 = (PIMAGE_NT_HEADERS32)ImageNtHeaders(ImageBase, &NtHeadersVersion);
	NtHeaders64 = (PIMAGE_NT_HEADERS64)NtHeaders32;

	InternalData.ImageBase = ImageBase;
	InternalData.ImportDescriptor = nullptr;

	switch (NtHeadersVersion)
	{
	case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
		InternalData.ImportDescriptor = PtrAdd(InternalData.ImportDescriptor, NtHeaders32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].VirtualAddress);
		InternalData.SizeOfTable = NtHeaders32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].Size;
		InternalData.SizeOfImage = NtHeaders32->OptionalHeader.SizeOfImage;
		break;

	case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
		InternalData.ImportDescriptor = PtrAdd(InternalData.ImportDescriptor, NtHeaders64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].VirtualAddress);
		InternalData.SizeOfTable = NtHeaders64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].Size;
		InternalData.SizeOfImage = NtHeaders64->OptionalHeader.SizeOfImage;
		break;

	default:
		return STATUS_INVALID_IMAGE_FORMAT;
	}

	InternalData.ImportDescriptor = PtrAdd(InternalData.ImportDescriptor, ImageBase);
	InternalData.EndOfImage = PtrAdd(ImageBase, InternalData.SizeOfImage);
	InternalData.EndOfTable = PtrAdd(InternalData.ImportDescriptor, InternalData.SizeOfTable);

	switch (NtHeadersVersion)
	{
	case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
		return WalkImportTableInternal<CallbackRoutine, CallbackContext, PIMAGE_THUNK_DATA32>(&InternalData, Callback, Context);

	case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
		return WalkImportTableInternal<CallbackRoutine, CallbackContext, PIMAGE_THUNK_DATA64>(&InternalData, Callback, Context);
	}

	return STATUS_INVALID_IMAGE_FORMAT;
}

#define WalkOpCodeM(Buffer, OpLength, Ret) \
            [&] (PBYTE Buffer, ULONG_PTR OpLength, PVOID &Ret) -> NTSTATUS

template<typename T>
PVOID WalkOpCode64T(PVOID Buffer, LONG_PTR Size, T Callback)
{
	PVOID ret = nullptr;

	PBYTE _Buffer = (PBYTE)Buffer;

	for (; Size > 0;)
	{
		ULONG_PTR Length;

		Length = GetOpCodeSize64_2(_Buffer);

		if (NT_SUCCESS(Callback(_Buffer, Length, ret)))
			break;

		_Buffer += Length;
		Size -= Length;
	}

	return ret;
}

template<typename T>
PVOID WalkOpCodeT(PVOID Buffer, LONG_PTR Size, T Callback)
{
	PVOID ret = nullptr;

	PBYTE _Buffer = (PBYTE)Buffer;

	for (; Size > 0;)
	{
		ULONG_PTR Length;

		Length = GetOpCodeSize_2(_Buffer);

		if (NT_SUCCESS(Callback(_Buffer, Length, ret)))
			break;

		_Buffer += Length;
		Size -= Length;
	}

	return ret;
}

#if ML_USER_MODE


_ML_C_HEAD_

inline Long fsize(FILE *fp)
{
	fseek(fp, 0, SEEK_CUR);
	return _filelength(_fileno(fp));
}

inline Int64 fsize64(FILE *fp)
{
	fseek(fp, 0, SEEK_CUR);
	return _filelengthi64(_fileno(fp));
}

inline Int64 my_fseek64(FILE *fp, Int64 offset, Int origin)
{
	Int64 ret;

	ret = fseek(fp, 0, SEEK_CUR);
	ret = _lseeki64(_fileno(fp), offset, origin);

	return ret == -1 ? 1 : 0;
}

inline Int64 my_ftell64(FILE *fp)
{
	int ret;
	fpos_t pos;

	ret = fseek(fp, 0, SEEK_CUR);
	ret = fgetpos(fp, &pos);

	return ret ? ret : pos;
}

_ML_C_TAIL_

#endif // ring3



#define NFD_NOT_RESOLVE_PATH    (0x00000001u)
#define NFD_EXPAND_ENVVAR       (0x00000002u)

/************************************************************************
file disk
************************************************************************/
class NtFileDisk
{
protected:
	HANDLE          m_FileHandle;
	LARGE_INTEGER   m_FileSize, m_Position;

public:
	ForceInline NtFileDisk();
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

		RtlInitAnsiString(&FileNameA, FileName);
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

	NoInline
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
			PCWSTR          Format,
			...
		);

	NTSTATUS
		Print(
			PLARGE_INTEGER  BytesWritten,
			PCSTR           Format,
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
#if ML_KERNEL_MODE
		return ZwClose(FileHandle);
#else
		return NtClose(FileHandle);
#endif
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

		RtlInitUnicodeString(&DeviceName, FileName);
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

#if ML_USER_MODE

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

		/* check for console input/output */
#if 0
		if (0 == _wcsicmp(L"CONOUT$", lpFileName)
			|| 0 == _wcsicmp(L"CONIN$", lpFileName))
		{
			return OpenConsoleW(lpFileName,
				dwDesiredAccess,
				lpSecurityAttributes ? lpSecurityAttributes->bInheritHandle : FALSE,
				FILE_SHARE_READ | FILE_SHARE_WRITE);
		}
#endif
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
			lpFileName,
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
			Status = ZwQueryInformationFile(
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
				Status = ZwQueryEaFile(hTemplateFile,
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
		Status = ZwCreateFile(&FileHandle,
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

#if 0
		/* free the extended attributes buffer if allocated */
		if (EaBuffer != NULL)
		{
			RtlFreeHeap(RtlGetProcessHeap(), 0, EaBuffer);
		}
#endif

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

#endif // r3

};

#if !ML_KERNEL_MODE

/************************************************************************
memory file
************************************************************************/
class NtFileMemory
{
protected:
	PVOID           m_BufferBase, m_BufferCurrent, m_BufferEnd;
	LARGE_INTEGER   m_FileSize;
	MemoryAllocator m_Memory;

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
		PVOID       Buffer;
		ULONG_PTR   Size;
		NTSTATUS    Status;

		ReleaseAll();

		Status = File.Open(FileName, Flags, ShareMode, Access, CreationDisposition, Attributes, CreateOptions);
		FAIL_RETURN(Status);

		Size = File.GetSize32();
		m_BufferBase = m_Memory.Alloc(ROUND_UP(Size + sizeof(ULONG_PTR), 16));
		if (m_BufferBase == nullptr)
			return STATUS_NO_MEMORY;

		Status = File.Read(m_BufferBase, Size);
		if (NT_FAILED(Status))
		{
			ReleaseAll();
			return Status;
		}

		*(PULONG)PtrAdd(m_BufferBase, Size) = 0;

		m_FileSize.QuadPart = Size;
		m_BufferCurrent = m_BufferBase;
		m_BufferEnd = PtrAdd(m_BufferCurrent, Size);

		return Status;
	}

	NTSTATUS Open(PVOID Buffer, ULONG_PTR Size)
	{
		ReleaseAll();

		m_BufferBase = m_Memory.Alloc(ROUND_UP(Size + sizeof(ULONG_PTR), 16));
		if (m_BufferBase == nullptr)
			return STATUS_NO_MEMORY;

		CopyMemory(m_BufferBase, Buffer, Size);

		*(PULONG)PtrAdd(m_BufferBase, Size) = 0;

		m_FileSize.QuadPart = Size;
		m_BufferCurrent = m_BufferBase;
		m_BufferEnd = PtrAdd(m_BufferCurrent, Size);

		return STATUS_SUCCESS;
	}

	NTSTATUS GetPosition(PLARGE_INTEGER Position)
	{
		if (Position == nullptr)
			return STATUS_INVALID_PARAMETER;

		Position->QuadPart = PtrOffset(m_BufferCurrent, m_BufferBase);

		return STATUS_SUCCESS;
	}

	ULONG_PTR GetPosition32()
	{
		return PtrOffset(m_BufferCurrent, m_BufferBase);
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

	NoInline
		NTSTATUS
		Seek(
			LARGE_INTEGER   Offset,
			ULONG_PTR       MoveMethod = FILE_BEGIN,
			PLARGE_INTEGER  NewPosition = nullptr
		)
	{
		LARGE_INTEGER NewOffset;

		NewOffset.QuadPart = PtrOffset(m_BufferCurrent, m_BufferBase);

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

			BufferBase = m_Memory.ReAlloc(m_BufferBase, (SizeT)NewOffset.QuadPart, HEAP_ZERO_MEMORY);
			if (BufferBase == nullptr)
				return STATUS_NO_MEMORY;

			m_FileSize.QuadPart = NewOffset.QuadPart;
			m_BufferBase = BufferBase;
			m_BufferEnd = PtrAdd(BufferBase, m_FileSize.QuadPart);
		}

		m_BufferCurrent = PtrAdd(m_BufferBase, NewOffset.QuadPart);

		if (NewPosition != nullptr)
			NewPosition->QuadPart = NewOffset.QuadPart;

		return STATUS_SUCCESS;
	}

	NoInline
		NTSTATUS
		Read(
			PVOID           Buffer,
			ULONG_PTR       Size,
			PLARGE_INTEGER  BytesRead = nullptr
		)
	{
		ULONG_PTR SizeRemain;

		SizeRemain = PtrOffset(m_BufferEnd, m_BufferCurrent);
		Size = ML_MIN(Size, SizeRemain);

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

		m_BufferCurrent = PtrAdd(m_BufferCurrent, Size);

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
		m_Memory.SafeFree(&m_BufferBase);
		m_BufferCurrent = nullptr;
		m_BufferEnd = nullptr;

		return STATUS_SUCCESS;
	}
};

#endif // ML_KERNEL_MODE


ULONG_PTR FASTCALL LdeGetOpCodeSize32(PVOID Code, PVOID DEF_VAL(*OpCodePtr, NULL));
ULONG_PTR FASTCALL LdeGetOpCodeSize64(PVOID Code, PVOID DEF_VAL(*OpCodePtr, NULL));
ULONG_PTR FASTCALL SizeOfProc(PVOID Proc);
BOOL FASTCALL IsRelativeCmd(PBYTE OpCode);

#if ML_SUPPORT_LAMBDA

NTSTATUS EnableHeapCorruptionHelper();
NTSTATUS DisableHeapCorruptionHelper();

#endif // ML_SUPPORT_LAMBDA

// class


ML_NAMESPACE

template<typename T>
struct TypeTraits
{
	typedef T           VALUE_TYPE;
	typedef T&          REF_TYPE;
	typedef const T&    CONST_REF_TYPE;
	VALUE_TYPE          value;
};

template<typename T>
struct TypeTraits<T&>
{
	typedef T           VALUE_TYPE;
	typedef T&          REF_TYPE;
	typedef const T&    CONST_REF_TYPE;
	VALUE_TYPE          value;
};

template<typename T>
struct TypeTraits<const T&>
{
	typedef T           VALUE_TYPE;
	typedef T&          REF_TYPE;
	typedef const T&    CONST_REF_TYPE;
	VALUE_TYPE          value;
};

template<>
struct TypeTraits<VOID>
{
	typedef VOID VALUE_TYPE;
	typedef VOID REF_TYPE;
	typedef VOID CONST_REF_TYPE;
};



template<typename T> class Function;

#pragma warning(push)
#pragma warning(disable:4510 4610)

template<typename T> class Function : public Function<TYPE_OF(&T::operator())> {};

#pragma warning(pop)

#pragma push_macro("DEFINE_FUNCTION_CLASS")

#undef DEFINE_FUNCTION_CLASS
#define DEFINE_FUNCTION_CLASS(call_convention)\
    template<typename R, typename... ARGS> \
    class Function<R call_convention(ARGS...)> \
						    { \
    public: \
        typedef Function<R call_convention(ARGS...)> SELF_TYPE; \
        typedef R RET_TYPE; \
        static const ULONG_PTR NumberOfArguments = sizeof...(ARGS); \
        typedef R (FUNCTION_TYPE)(ARGS...); \
\
        class Invoker \
						        { \
            ULONG_PTR RefCount; \
        public: \
            Invoker() : RefCount(1) {} \
            virtual ~Invoker() {} \
            virtual R Invoke(ARGS...) = 0; \
\
            void AddRef() \
						            { \
                ++this->RefCount; \
						            } \
\
            void Release() \
						            { \
                if (--this->RefCount == 0) \
                    delete this; \
						            } \
\
						        }; \
\
        template<typename F> \
        class FunctionInvoker : public Invoker \
						        { \
        protected: \
            F func; \
\
        public: \
            FunctionInvoker(const F& function) : func(function) \
						            { \
						            } \
\
            R Invoke(ARGS... args) \
						            { \
                return func(args...); \
						            } \
						        }; \
\
        Invoker *invoker; \
\
        NoInline Function(const SELF_TYPE &func) \
						        { \
            this->invoker = func.invoker; \
            this->invoker->AddRef(); \
						        } \
\
        template<typename F> \
        NoInline Function(const F &func) \
						        { \
            this->invoker = new FunctionInvoker<F>(func); \
						        } \
\
        template<typename F> \
        SELF_TYPE& operator=(const F &func) \
						        { \
            this->~Function(); \
            this->invoker = new FunctionInvoker<F>(func); \
            return *this; \
						        } \
\
        NoInline ~Function() \
						        { \
            if (invoker != nullptr) \
                invoker->Release(); \
						        } \
\
        R operator()(ARGS... args) \
						        { \
            return this->invoker->Invoke(args...); \
						        } \
						    }

#define DEFINE_FUNCTION_POINTER_CLASS(call_convention)\
    template<typename R, typename... ARGS> \
    class Function<R (call_convention*)(ARGS...)> \
						    { \
    public: \
        typedef Function<R call_convention(ARGS...)> SELF_TYPE; \
        typedef R RET_TYPE; \
        static const ULONG_PTR NumberOfArguments = sizeof...(ARGS); \
        typedef R (FUNCTION_TYPE)(ARGS...); \
\
        class Invoker \
						        { \
            ULONG_PTR RefCount; \
        public: \
            Invoker() : RefCount(1) {} \
            virtual ~Invoker() {} \
            virtual R Invoke(ARGS...) = 0; \
\
            void AddRef() \
						            { \
                ++this->RefCount; \
						            } \
\
            void Release() \
						            { \
                if (--this->RefCount == 0) \
                    delete this; \
						            } \
\
						        }; \
\
        template<typename F> \
        class FunctionInvoker : public Invoker \
						        { \
        protected: \
            F func; \
\
        public: \
            FunctionInvoker(const F& function) : func(function) \
						            { \
						            } \
\
            R Invoke(ARGS... args) \
						            { \
                return func(args...); \
						            } \
						        }; \
\
        Invoker *invoker; \
\
        NoInline Function(const SELF_TYPE &func) \
						        { \
            this->invoker = func.invoker; \
            this->invoker->AddRef(); \
						        } \
\
        template<typename F> \
        NoInline Function(const F &func) \
						        { \
            this->invoker = new FunctionInvoker<F>(func); \
						        } \
\
        template<typename F> \
        SELF_TYPE& operator=(const F &func) \
						        { \
            this->~Function(); \
            this->invoker = new FunctionInvoker<F>(func); \
            return *this; \
						        } \
\
        NoInline ~Function() \
						        { \
            if (invoker != nullptr) \
                invoker->Release(); \
						        } \
\
        R operator()(ARGS... args) \
						        { \
            return this->invoker->Invoke(args...); \
						        } \
						    };

#define DEFINE_CLASS_METHOD_CLASS(...) \
    template<typename CLASS, typename R, typename... ARGS> \
    class Function<R(__VA_ARGS__ CLASS::*)(const CLASS&, ARGS...)> \
						    { \
    public: \
        typedef Function<R __VA_ARGS__(const CLASS&, ARGS...)> SELF_TYPE; \
        typedef R RET_TYPE; \
        static const ULONG_PTR NumberOfArguments = sizeof...(ARGS); \
        typedef R (FUNCTION_TYPE)(const CLASS&, ARGS...); \
\
        class Invoker \
						        { \
            ULONG_PTR RefCount; \
        public: \
            Invoker() : RefCount(1) {} \
            virtual ~Invoker() {} \
            virtual R Invoke(const CLASS&, ARGS...) = 0; \
\
            void AddRef() \
						            { \
                ++this->RefCount; \
						            } \
\
            void Release() \
						            { \
                if (--this->RefCount == 0) \
                    delete this; \
						            } \
\
						        }; \
\
        template<typename F> \
        class FunctionInvoker : public Invoker \
						        { \
        protected: \
            F func; \
\
        public: \
            FunctionInvoker(const F& function) : func(function) \
						            { \
						            } \
\
            R Invoke(const CLASS& thiz, ARGS... args) \
						            { \
                return func(thiz, args...); \
						            } \
						        }; \
\
        Invoker *invoker; \
\
        NoInline Function(const SELF_TYPE &func) \
						        { \
            this->invoker = func.invoker; \
            this->invoker->AddRef(); \
						        } \
\
        template<typename F> \
        NoInline Function(const F &func) \
						        { \
            this->invoker = new FunctionInvoker<F>(func); \
						        } \
\
        template<typename F> \
        SELF_TYPE& operator=(const F &func) \
						        { \
            this->~Function(); \
            this->invoker = new FunctionInvoker<F>(func); \
            return *this; \
						        } \
\
        NoInline ~Function() \
						        { \
            if (invoker != nullptr) \
                invoker->Release(); \
						        } \
\
        R operator()(const CLASS& thiz, ARGS... args) \
						        { \
            return this->invoker->Invoke(thiz, args...); \
						        } \
						    };


#define DEFINE_LAMBDA_CLASS(...) \
    template<typename CLASS, typename R, typename... ARGS> \
    class Function<R(__VA_ARGS__ CLASS::*)(ARGS...) const> \
						    { \
    public: \
        typedef Function<R __VA_ARGS__(ARGS...)> SELF_TYPE; \
        typedef R RET_TYPE; \
        static const ULONG_PTR NumberOfArguments = sizeof...(ARGS); \
        typedef R (FUNCTION_TYPE)(ARGS...); \
\
        class Invoker \
						        { \
            ULONG_PTR RefCount; \
        public: \
            Invoker() : RefCount(1) {} \
            virtual ~Invoker() {} \
            virtual R Invoke(ARGS...) = 0; \
\
            void AddRef() \
						            { \
                ++this->RefCount; \
						            } \
\
            void Release() \
						            { \
                if (--this->RefCount == 0) \
                    delete this; \
						            } \
\
						        }; \
\
        template<typename F> \
        class FunctionInvoker : public Invoker \
						        { \
        protected: \
            F func; \
\
        public: \
            FunctionInvoker(const F& function) : func(function) \
						            { \
						            } \
\
            R Invoke(ARGS... args) \
						            { \
                return func(args...); \
						            } \
						        }; \
\
        Invoker *invoker; \
\
        NoInline Function(const SELF_TYPE &func) \
						        { \
            this->invoker = func.invoker; \
            this->invoker->AddRef(); \
						        } \
\
        template<typename F> \
        NoInline Function(const F &func) \
						        { \
            this->invoker = new FunctionInvoker<F>(func); \
						        } \
\
        template<typename F> \
        SELF_TYPE& operator=(const F &func) \
						        { \
            this->~Function(); \
            this->invoker = new FunctionInvoker<F>(func); \
            return *this; \
						        } \
\
        NoInline ~Function() \
						        { \
            if (invoker != nullptr) \
                invoker->Release(); \
						        } \
\
        R operator()(ARGS... args) \
						        { \
            return this->invoker->Invoke(args...); \
						        } \
						    };


#if ML_X86

DEFINE_FUNCTION_CLASS(__stdcall);
DEFINE_FUNCTION_CLASS(__cdecl);

DEFINE_FUNCTION_POINTER_CLASS(__stdcall);
DEFINE_FUNCTION_POINTER_CLASS(__cdecl);

// DEFINE_CLASS_METHOD_CLASS(__stdcall);
// DEFINE_CLASS_METHOD_CLASS(__cdecl);
// DEFINE_CLASS_METHOD_CLASS();

DEFINE_LAMBDA_CLASS(__stdcall);
DEFINE_LAMBDA_CLASS(__cdecl);
DEFINE_LAMBDA_CLASS();

#if !CPP_CLI_DEFINED

DEFINE_FUNCTION_CLASS(__fastcall);
DEFINE_FUNCTION_POINTER_CLASS(__fastcall);

// DEFINE_CLASS_METHOD_CLASS(__fastcall);
DEFINE_LAMBDA_CLASS(__fastcall);

#endif // c++/cli

#elif ML_AMD64

DEFINE_FUNCTION_CLASS(__cdecl);
DEFINE_FUNCTION_POINTER_CLASS(__cdecl);

// DEFINE_CLASS_METHOD_CLASS(__cdecl);
DEFINE_LAMBDA_CLASS(__cdecl);

#endif

#pragma pop_macro("DEFINE_FUNCTION_CLASS")



//--------------------------------------------------------------------------------------
// A growable array
//--------------------------------------------------------------------------------------

#pragma warning(push)
#pragma warning(disable:4324)

		// #define FOR_EACH_VEC(_Var, _MlGrowableArray) (_Var) = (_MlGrowableArray).GetData(); for (ULONG_PTR _VecItemCount = (_MlGrowableArray).GetSize(); _VecItemCount != 0; ++(_Var), --_VecItemCount)
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

	NoInline NTSTATUS Add(CONST_TYPE_REF Value)
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

	NoInline NTSTATUS Insert(ULONG_PTR Index, CONST_TYPE_REF Value)
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

	NoInline ULONG_PTR IndexOf(CONST_TYPE_REF Value, ULONG_PTR Start, ULONG_PTR NumberOfElements)
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

	NoInline NTSTATUS Remove(ULONG_PTR Index)
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

	NoInline void RemoveAll()
	{
		SetSize(0);
	}

	NoInline void Reset()
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
			FreeMemoryP(m_Data);
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

			NewMaxSize = ML_MAX(NewMaxSize, m_MaxSize + GrowBy);

			// Verify that (nNewMaxSize * sizeof(TYPE)) is not greater than UINT_MAX or the realloc will overrun
			//            if (sizeof(TYPE) > ULONG_MAX / NewMaxSize)
			//                return STATUS_INVALID_PARAMETER;

#if ML_KERNEL_MODE

			TYPE* DataNew = (TYPE *)AllocateMemoryP(NewMaxSize * sizeof(*DataNew));

			if (DataNew == nullptr)
				return STATUS_NO_MEMORY;

			if (DataNew != nullptr && m_Data != nullptr)
			{
				CopyMemory(DataNew, m_Data, m_MaxSize * sizeof(*DataNew));
				FreeMemoryP(m_Data);
			}

#else // r3
			TYPE* DataNew = (TYPE *)ReAllocateMemoryP(m_Data, NewMaxSize * sizeof(*DataNew));
#endif // r

			if (DataNew == nullptr)
				return STATUS_NO_MEMORY;

			m_Data = DataNew;
			m_MaxSize = NewMaxSize;
		}

		return STATUS_SUCCESS;
	}
};

#pragma warning(pop)



BOOL
IsNameInExpression2(
	IN PUNICODE_STRING  Expression,
	IN PUNICODE_STRING  Name,
	IN BOOL             DEF_VAL(IgnoreCase, TRUE),
	IN PWSTR            DEF_VAL(UpcaseTable, NULL) OPTIONAL
);


#define STRING_DEBUG 0
#define USE_TEMPLATE 1

#pragma warning(push)
#pragma warning(disable:4172)


template<typename STRING_LENGTH_TYPE = USHORT, typename LARGE_LENGTH_TYPE = ULONG>
class StringImplementT
{
protected:

	typedef typename STRING_LENGTH_TYPE STRING_LENGTH_TYPE;
	typedef typename LARGE_LENGTH_TYPE  LARGE_LENGTH_TYPE;

	typedef WCHAR           STRING_CHAR_TYPE;
	typedef PWSTR           STRING_POINTER_TYPE;
	typedef PCWSTR          STRING_CONST_POINTER_TYPE;

#if USE_TEMPLATE


	typedef struct STRING_TYPE
	{
		STRING_LENGTH_TYPE  Length;
		STRING_LENGTH_TYPE  MaximumLength;
		STRING_POINTER_TYPE Buffer;

	} STRING_TYPE, *PSTRING_TYPE;

	typedef const STRING_TYPE* PCSTRING_TYPE;


#else

	typedef UNICODE_STRING      STRING_TYPE;
	typedef PUNICODE_STRING     PSTRING_TYPE;
	typedef PCUNICODE_STRING    PCSTRING_TYPE;

#endif

	static const LARGE_LENGTH_TYPE  kMaxNumberValue = (LARGE_LENGTH_TYPE)-1ll;
	static const LARGE_LENGTH_TYPE  kCharSize = sizeof(STRING_CHAR_TYPE);
	static const LARGE_LENGTH_TYPE  kMaxLength = ((LARGE_LENGTH_TYPE)1 << bitsof(STRING_LENGTH_TYPE)) - 1 - kCharSize;

public:
	static const LARGE_LENGTH_TYPE  kInvalidIndex = kMaxNumberValue;

protected:

#if USE_TEMPLATE
	template<typename, typename>
#endif

	friend class StringT;

protected:

	STRING_TYPE         String;
	STRING_CHAR_TYPE    Buffer[4];

protected:
	StringImplementT()
	{
		Reset();
	}

	VOID Reset()
	{
		RtlInitEmptyString(&this->String);
	}

	operator PCSTRING_TYPE()
	{
		return &this->String;
	}

	operator PSTRING_TYPE()
	{
		return &this->String;
	}

	LONG Compare(StringImplementT *Impl, BOOL CaseInSensitive = FALSE)
	{
		return Compare(*Impl, CaseInSensitive);
	}

	LONG Compare(STRING_CONST_POINTER_TYPE Str, LARGE_LENGTH_TYPE Count, BOOL CaseInSensitive = FALSE)
	{
		STRING_TYPE String;

		String.Buffer = (STRING_POINTER_TYPE)Str;
		String.Length = (STRING_LENGTH_TYPE)CountToLength(Count);
		String.MaximumLength = String.Length;

		return Compare(&String, CaseInSensitive);
	}

	LONG Compare(PCSTRING_TYPE Str, BOOL CaseInSensitive = FALSE)
	{
		UNICODE_STRING Str1, Str2;

		Str1.Length = (USHORT)GetLength();
		Str1.MaximumLength = (USHORT)Str1.Length;
		Str1.Buffer = GetBuffer();

		Str2.Length = (USHORT)Str->Length;
		Str2.MaximumLength = (USHORT)Str->MaximumLength;
		Str2.Buffer = Str->Buffer;

		return RtlCompareUnicodeString(&Str1, &Str2, (BOOLEAN)CaseInSensitive);
	}

	NoInline NTSTATUS Concat(STRING_CONST_POINTER_TYPE Str, LARGE_LENGTH_TYPE Count)
	{
		FAIL_RETURN(VerifyBufferLength(GetCount() + Count));
		CopyString(Str, Count, TRUE);
		return STATUS_SUCCESS;
	}

	NoInline NTSTATUS CopyFrom(STRING_CONST_POINTER_TYPE Str, LARGE_LENGTH_TYPE Count)
	{
		FAIL_RETURN(VerifyBufferLength(Count));
		CopyString(Str, Count, FALSE);
		return STATUS_SUCCESS;
	}

	LARGE_LENGTH_TYPE FormatCountV(STRING_CONST_POINTER_TYPE Format, va_list Arguments)
	{
		return _vscwprintf(Format, Arguments);
	}

	LARGE_LENGTH_TYPE FormatV(STRING_CONST_POINTER_TYPE Format, va_list Arguments)
	{
		LARGE_LENGTH_TYPE Count;

		Count = _vsnwprintf(GetBuffer(), (size_t)LengthToCount(GetMaxLength()), Format, Arguments);
		this->String.Length = (STRING_LENGTH_TYPE)CountToLength(Count);

		return Count;
	}

	LARGE_LENGTH_TYPE GetLength()
	{
		return this->String.Length;
	}

	VOID SetLength(LARGE_LENGTH_TYPE Length)
	{
		this->String.Length = ML_MIN(this->String.Length, Length);
	}

	LARGE_LENGTH_TYPE GetMaxLength()
	{
		return this->String.MaximumLength;
	}

	LARGE_LENGTH_TYPE GetCount()
	{
		return GetLength() / kCharSize;
	}

	STRING_POINTER_TYPE GetBuffer()
	{
		return this->String.Buffer;
	}

	NoInline LARGE_LENGTH_TYPE IndexOf(STRING_CONST_POINTER_TYPE Str, LARGE_LENGTH_TYPE StartIndex = 0)
	{
		if (StartIndex >= GetCount())
			return kInvalidIndex;

		STRING_CONST_POINTER_TYPE Begin, End, Found;

		Begin = GetBuffer();
		End = PtrAdd(GetBuffer(), GetLength());

		if (Str[0] != 0)
		{
			Found = (STRING_CONST_POINTER_TYPE)KMP(Begin + StartIndex, StrLengthW(Begin + StartIndex), Str, StrLengthW(Str));
		}
		else
		{
			Found = Begin + StartIndex;
			while (Found <= End && Found[0] != 0)
				++Found;

			Found = Found < End ? Found : nullptr;
		}

		return Found == nullptr ? kInvalidIndex : Found - Begin;
	}

	BOOL IsNullOrEmpty()
	{
		return GetCount() == 0;
	}

	LARGE_LENGTH_TYPE LastIndexOf(STRING_CONST_POINTER_TYPE Str, LARGE_LENGTH_TYPE StartIndex = 0)
	{
		if (StartIndex >= GetCount())
			return kInvalidIndex;

		STRING_CONST_POINTER_TYPE Begin, End, Found, Prev;

		Begin = GetBuffer();
		End = PtrAdd(GetBuffer(), GetLength());

		Prev = nullptr;
		Begin += StartIndex;
		while (Begin < End)
		{
			Found = (STRING_CONST_POINTER_TYPE)KMP(Begin, StrLengthW(Begin), Str, StrLengthW(Str));
			if (Found == nullptr)
				break;

			Prev = Found;
			Begin = Found + 1;
		}

		return Prev == nullptr ? kInvalidIndex : Prev - GetBuffer();
	}

	BOOL MatchExpression(PCSTRING_TYPE Expression, BOOL IgnoreCase)
	{
		UNICODE_STRING Expr, Name;

		Expr.Length = Expression->Length;
		Expr.MaximumLength = Expr.Length;
		Expr.Buffer = Expression->Buffer;

		Name.Length = GetLength();
		Name.MaximumLength = Name.Length;
		Name.Buffer = GetBuffer();

		return IsNameInExpression2(&Expr, &Name, IgnoreCase);
	}

	VOID ToLower()
	{
		StringLowerW(GetBuffer(), GetCount());
	}

	VOID ToUpper()
	{
		StringUpperW(GetBuffer(), GetCount());
	}


	/************************************************************************
	internal
	************************************************************************/

	NTSTATUS VerifyBufferLength(LARGE_LENGTH_TYPE Count)
	{
		LARGE_LENGTH_TYPE Length;

		Length = CountToLength(Count);

		if (Length > kMaxLength)
			return STATUS_NAME_TOO_LONG;

		if (Length >= this->String.MaximumLength)
			return STATUS_BUFFER_TOO_SMALL;

		return STATUS_SUCCESS;
	}

	NoInline VOID CopyString(STRING_CONST_POINTER_TYPE Str, LARGE_LENGTH_TYPE Count, BOOL Concat)
	{
		LARGE_LENGTH_TYPE Length;

		if (!Concat)
		{
			this->String.Length = 0;
			AddNull();
		}

		if (Count == 0)
			return;

		Length = CountToLength(Count);
		CopyMemory(GetBuffer() + GetCount(), Str, Length);
		this->String.Length += (STRING_LENGTH_TYPE)Length;
		AddNull();
	}

	VOID SetBuffer(STRING_POINTER_TYPE Buffer, LARGE_LENGTH_TYPE MaximumLength)
	{
		this->String.Buffer = Buffer;
		this->String.Length = 0;
		this->String.MaximumLength = (STRING_LENGTH_TYPE)MaximumLength;

		GetBuffer()[0] = 0;
	}

	VOID AddNull()
	{
		*PtrAdd(GetBuffer(), GetLength()) = 0;
	}

	VOID CopyBuffer(StringImplementT *Impl)
	{
		CopyString(Impl->GetBuffer(), Impl->GetCount(), FALSE);
	}


	static ForceInline LARGE_LENGTH_TYPE CountToLength(LARGE_LENGTH_TYPE Count)
	{
		return Count * kCharSize;
	}

	static ForceInline LARGE_LENGTH_TYPE CountToLengthAddNull(LARGE_LENGTH_TYPE Count)
	{
		return Count * kCharSize + kCharSize;
	}

	static ForceInline LARGE_LENGTH_TYPE LengthToCount(LARGE_LENGTH_TYPE Length)
	{
		return Length / kCharSize;
	}

	static LARGE_LENGTH_TYPE GetStringCount(STRING_CONST_POINTER_TYPE Str)
	{
		return StrLengthW(Str);
	}

	static VOID InitString(PSTRING_TYPE String, STRING_CONST_POINTER_TYPE Buffer, LARGE_LENGTH_TYPE Count = kMaxNumberValue)
	{
		if (Count == kMaxNumberValue)
			Count = GetStringCount(Buffer);

		String->Buffer = (STRING_POINTER_TYPE)Buffer;
		String->Length = CountToLength(Count);
		String->MaximumLength = String->Length;
	}
};




ML_NAMESPACE_BEGIN(Reg);

PVOID
AllocateKeyInfo(
	ULONG_PTR Size
);

VOID
FreeKeyInfo(
	PVOID Info
);

NTSTATUS
OpenPredefinedKeyHandle(
	PHANDLE     KeyHandle,
	HANDLE      PredefinedKey,
	ACCESS_MASK DEF_VAL(DesiredAccess, KEY_ALL_ACCESS)
);

NTSTATUS
OpenKey(
	PHANDLE     KeyHandle,
	HANDLE      hKey,
	ACCESS_MASK DesiredAccess,
	PCWSTR      SubKey
);

NTSTATUS
CloseKeyHandle(
	HANDLE KeyHandle
);

NTSTATUS
GetKeyValue(
	HANDLE                      hKey,
	PCWSTR                      SubKey,
	PCWSTR                      ValueName,
	KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
	PVOID                       KeyValueInformation,
	ULONG                       Length,
	PULONG                      DEF_VAL(ResultLength, NULL),
	ULONG                       DEF_VAL(Flags, 0)
);

NTSTATUS
GetKeyValue(
	HANDLE                          hKey,
	PCWSTR                          SubKey,
	PCWSTR                          ValueName,
	PKEY_VALUE_PARTIAL_INFORMATION* Value
);

NTSTATUS
SetKeyValue(
	HANDLE      hKey,
	PCWSTR      SubKey,
	PCWSTR      ValueName,
	ULONG       ValueType,
	LPCVOID     ValueData,
	DWORD       ValueDataLength,
	ULONG_PTR   Flags = 0
);

NTSTATUS
DeleteKey(
	HANDLE      hKey,
	PCWSTR      SubKey,
	ULONG_PTR   Flags = 0
);

NTSTATUS
DeleteKeyValue(
	HANDLE      hKey,
	PCWSTR      SubKey,
	PCWSTR      ValueName,
	ULONG_PTR   Flags = 0
);

ML_NAMESPACE_END_(Reg);

//typedef StringImplementT<> StringImplement;

#if USE_TEMPLATE
template<typename STRING_LENGTH_TYPE = USHORT, typename LARGE_LENGTH_TYPE = ULONG>
#endif

class StringT
{
public:

	typedef GrowableArray<StringT>  StringArray;
	typedef GrowableArray<BYTE>     ByteArray;

	enum
	{
		WithBOM = 1 << 0,
	};

	class AnsiString : public ByteArray
	{
	public:
		operator bool() const
		{
			return this->GetSize() != 0;
		}

		operator PCSTR() const
		{
			return (PCSTR)this->GetData();
		}

		operator PSTR() const
		{
			return (PSTR)this->GetData();
		}

		operator PVOID() const
		{
			return (PVOID)this->GetData();
		}

		operator LPCVOID() const
		{
			return (LPCVOID)this->GetData();
		}

		operator LPCBYTE() const
		{
			return (LPCBYTE)this->GetData();
		}

		operator PBYTE() const
		{
			return (PBYTE)this->GetData();
		}
	};

protected:

#if USE_TEMPLATE
	typedef StringImplementT<STRING_LENGTH_TYPE, LARGE_LENGTH_TYPE> StringImplement;
#else
	typedef StringImplementT<> StringImplement;
#endif

	typedef typename StringImplement::STRING_LENGTH_TYPE             STRING_LENGTH_TYPE;
	typedef typename StringImplement::LARGE_LENGTH_TYPE              LARGE_LENGTH_TYPE;

	typedef typename StringImplement::STRING_CHAR_TYPE               STRING_CHAR_TYPE;
	typedef typename StringImplement::STRING_TYPE                    STRING_TYPE;
	typedef typename StringImplement::PSTRING_TYPE                   PSTRING_TYPE;
	typedef typename StringImplement::PCSTRING_TYPE                  PCSTRING_TYPE;
	typedef typename StringImplement::STRING_POINTER_TYPE            STRING_POINTER_TYPE;
	typedef typename StringImplement::STRING_CONST_POINTER_TYPE      STRING_CONST_POINTER_TYPE;

	static const LARGE_LENGTH_TYPE  kMaxNumberValue = StringImplement::kMaxNumberValue;
	static const LARGE_LENGTH_TYPE  kMaxLength = StringImplement::kMaxLength;
	static const LARGE_LENGTH_TYPE  kCharSize = StringImplement::kCharSize;
	static const ULONG_PTR          kStringImplSize = sizeof(StringImplement) - FIELD_SIZE(StringImplement, Buffer);

	typename StringImplement::STRING_POINTER_TYPE Buffer;

public:
	static const LARGE_LENGTH_TYPE  kInvalidIndex = StringImplement::kInvalidIndex;

public:
	StringT()
	{
		Reset();
		*this = L"";
	}

	StringT(const StringT& Str)
	{
		Reset();
		*this = Str;
	}

	StringT(STRING_CONST_POINTER_TYPE Str)
	{
		Reset();
		*this = Str;
	}

	StringT(STRING_CONST_POINTER_TYPE Str, ULONG_PTR InitLength)
	{
		Reset();
		CopyFrom(Str, InitLength);
	}

	StringT(const UNICODE_STRING &Str)
	{
		Reset();
		*this = Str;
	}

	StringT(STRING_CHAR_TYPE Chr)
	{
		STRING_CHAR_TYPE Buffer[2];

		Reset();

		Buffer[0] = Chr;
		Buffer[1] = 0;

		*this = Buffer;
	}

	NoInline ~StringT()
	{
		ReleaseBuffer();

#if STRING_DEBUG
		this->Buffer = (STRING_POINTER_TYPE)~0x112233;
#endif

	}

	LARGE_LENGTH_TYPE GetCount() const
	{
		return GetImplement()->GetCount();
	}

	LARGE_LENGTH_TYPE GetSize() const
	{
		return GetImplement()->GetLength();
	}

	STRING_POINTER_TYPE GetBuffer() const
	{
		return GetImplement()->GetBuffer();
	}

	operator STRING_POINTER_TYPE() const
	{
		return GetBuffer();
	}

	operator STRING_CONST_POINTER_TYPE() const
	{
		return GetBuffer();
	}
	/*
	operator PSTRING_TYPE()
	{
	return GetImplement()->operator PSTRING_TYPE();
	}

	operator PCSTRING_TYPE() const
	{
	return GetImplement()->operator PCSTRING_TYPE();
	}
	*/
	operator UNICODE_STRING() const
	{
		UNICODE_STRING str;
		StringImplement *Impl = GetImplement();

		if (Impl->GetLength() > (TYPE_OF(str.Length)) - 1)
		{
			RtlInitEmptyString(&str);
			return str;
		}

		str.Length = (USHORT)Impl->GetLength();
		str.MaximumLength = str.Length;
		str.Buffer = Impl->GetBuffer();

		return str;
	}

	ForceInline operator PCUNICODE_STRING() const
	{
		return (PCUNICODE_STRING)&((UNICODE_STRING)(*this));
	}

	ForceInline operator PUNICODE_STRING() const
	{
		return &((UNICODE_STRING)(*this));
	}

	operator bool() const
	{
		return this->GetCount() != 0;
	}

	bool operator!() const
	{
		return this->GetCount() == 0;
	}

	NoInline StringT& operator=(STRING_CONST_POINTER_TYPE Str)
	{
		CopyFrom(Str);
		return *this;
	}

	NoInline StringT& operator=(const StringT& Str)
	{
		if (this == &Str)
			return *this;

		CopyFrom(Str.Buffer, Str.GetCount());

		return *this;
	}

	NoInline StringT& operator=(PCUNICODE_STRING Str)
	{
		CopyFrom(Str->Buffer, Str->Length / sizeof(Str->Buffer[0]));
		return *this;
	}

	StringT& operator=(const UNICODE_STRING &Str)
	{
		return operator=(&Str);
	}

	NoInline BOOL operator!=(STRING_CONST_POINTER_TYPE Str) const
	{
		return Compare(Str) != 0;
	}

	NoInline BOOL operator!=(const StringT& Str) const
	{
		return Compare(Str.GetBuffer(), Str.GetCount()) != 0;
	}

	NoInline BOOL operator==(STRING_CONST_POINTER_TYPE Str) const
	{
		return Compare(Str) == 0;
	}

	NoInline BOOL operator==(const StringT& Str) const
	{
		return Compare(Str.GetBuffer(), Str.GetCount()) == 0;
	}

	NoInline StringT operator+(STRING_CONST_POINTER_TYPE Str) const
	{
		return StringT(*this).Concat(Str);
	}

	NoInline StringT operator+(const StringT& Str) const
	{
		return StringT(*this).Concat(Str);
	}

	NoInline StringT& operator+=(STRING_CHAR_TYPE Chr)
	{
		STRING_CHAR_TYPE Buffer[2];

		Buffer[0] = Chr;
		Buffer[1] = 0;

		return Concat(Buffer, 1);
	}

	NoInline StringT& operator+=(STRING_CONST_POINTER_TYPE Str)
	{
		return Concat(Str);
	}

	NoInline StringT& operator+=(const StringT& Str)
	{
		return Concat(Str);
	}

	NoInline StringT& operator+=(const UNICODE_STRING& Str)
	{
		return Concat(Str.Buffer, Str.Length / sizeof(Str.Buffer[0]));
	}

	STRING_CHAR_TYPE& operator[](INT Index) const
	{
		return GetBuffer()[Index < 0 ? GetCount() + Index : Index];
	}

	STRING_CONST_POINTER_TYPE begin()
	{
		return &(*this)[0];
	}

	STRING_CONST_POINTER_TYPE end()
	{
		return &(*this)[-1] + 1;
	}

	NoInline NTSTATUS Assign(LARGE_LENGTH_TYPE Count)
	{
		return ResizeBuffer(Count);
	}

	LONG Compare(STRING_CONST_POINTER_TYPE Str, BOOL CaseInSensitive = FALSE) const
	{
		return GetImplement()->Compare(Str, StringImplement::GetStringCount(Str), CaseInSensitive);
	}

	LONG Compare(STRING_CONST_POINTER_TYPE Str, LARGE_LENGTH_TYPE Count, BOOL CaseInSensitive = FALSE) const
	{
		return GetImplement()->Compare(Str, Count, CaseInSensitive);
	}

	StringT& Concat(STRING_CONST_POINTER_TYPE Str)
	{
		return Concat(Str, StringImplement::GetStringCount(Str));
	}

	StringT& Concat(const StringT& Str)
	{
		StringImplement *Impl = Str.GetImplement();
		return Concat(Impl->GetBuffer(), Impl->GetCount());
	}

	StringT& Concat(STRING_CONST_POINTER_TYPE Str, LARGE_LENGTH_TYPE Count)
	{
		NTSTATUS Status;

		Status = ResizeBuffer(GetCount() + Count);
		if (NT_FAILED(Status))
			return *this;

		GetImplement()->Concat(Str, Count);

		return *this;
	}

	NTSTATUS CopyFrom(STRING_CONST_POINTER_TYPE Str)
	{
		return CopyFrom(Str, StringImplement::GetStringCount(Str));
	}

	NoInline NTSTATUS CopyFrom(STRING_CONST_POINTER_TYPE Str, LARGE_LENGTH_TYPE Count)
	{
		NTSTATUS Status;

		Status = ResizeBuffer(Count);
		FAIL_RETURN(Status);

		return GetImplement()->CopyFrom(Str, Count);
	}

	NoInline StringT Copy() const
	{
		return StringT(*this);
	}

	NoInline static StringT Format(STRING_CONST_POINTER_TYPE format, ...)
	{
		va_list Arguments;
		va_start(Arguments, format);
		return FormatV(format, Arguments);
	}

	NoInline static StringT FormatV(STRING_CONST_POINTER_TYPE Format, va_list Arguments)
	{
		NTSTATUS    Status;
		StringT      NewString;

		Status = NewString.ResizeBuffer(NewString.GetImplement()->FormatCountV(Format, Arguments));
		if (NT_FAILED(Status))
			return NewString;

		NewString.GetImplement()->FormatV(Format, Arguments);
		return NewString;
	}

	NoInline LARGE_LENGTH_TYPE IndexOf(STRING_CHAR_TYPE Chr, LARGE_LENGTH_TYPE StartIndex = 0) const
	{
		STRING_CHAR_TYPE Buffer[2];

		Buffer[0] = Chr;
		Buffer[1] = 0;

		return IndexOf(Buffer, StartIndex);
	}

	NoInline LARGE_LENGTH_TYPE IndexOf(STRING_CONST_POINTER_TYPE Str, LARGE_LENGTH_TYPE StartIndex = 0) const
	{
		return GetImplement()->IndexOf(Str, StartIndex);
	}

	NoInline BOOL IsNullOrEmpty() const
	{
		return GetImplement() == nullptr ? TRUE : GetImplement()->IsNullOrEmpty();
	}

	NoInline LARGE_LENGTH_TYPE LastIndexOf(STRING_CHAR_TYPE Chr, LARGE_LENGTH_TYPE StartIndex = 0) const
	{
		STRING_CHAR_TYPE Buffer[2];

		Buffer[0] = Chr;
		Buffer[1] = 0;

		return LastIndexOf(Buffer, StartIndex);
	}

	NoInline LARGE_LENGTH_TYPE LastIndexOf(STRING_CONST_POINTER_TYPE Str, LARGE_LENGTH_TYPE StartIndex = 0) const
	{
		return GetImplement()->LastIndexOf(Str, StartIndex);
	}

	NoInline BOOL MatchExpression(STRING_CONST_POINTER_TYPE Expression, BOOL IgnoreCase = TRUE) const
	{
		STRING_TYPE Expr;
		StringImplement::InitString(&Expr, Expression);
		return GetImplement()->MatchExpression(&Expr, IgnoreCase);
	}

	NoInline BOOL MatchExpression(const UNICODE_STRING& Expression, BOOL IgnoreCase = TRUE) const
	{
		STRING_TYPE Expr;
		Expr.Length = Expression.Length;
		Expr.MaximumLength = Expression.MaximumLength;
		Expr.Buffer = Expression.Buffer;
		return GetImplement()->MatchExpression(&Expr, IgnoreCase);
	}

	NoInline BOOL MatchExpression(StringT Expression, BOOL IgnoreCase = TRUE) const
	{
		return GetImplement()->MatchExpression(&Expression.GetImplement()->String, IgnoreCase);
	}

	NoInline StringT PadLeft(LARGE_LENGTH_TYPE TotalWidth, STRING_CHAR_TYPE PaddingChar = ' ') const
	{
		NTSTATUS            Status;
		StringT              Padded = *this;
		LARGE_LENGTH_TYPE   LengthToPad;

		if (TotalWidth <= Padded.GetCount())
			return Padded;

		LengthToPad = TotalWidth - Padded.GetCount();
		Status = Padded.ResizeBuffer(TotalWidth);
		if (NT_FAILED(Status))
			return Padded;

		Padded = L"";

		for (; LengthToPad; --LengthToPad)
			Padded += PaddingChar;

		Padded += *this;

		return Padded;
	}

	NoInline StringT PadRight(LARGE_LENGTH_TYPE TotalWidth, STRING_CHAR_TYPE PaddingChar = ' ') const
	{
		NTSTATUS            Status;
		StringT             Padded = *this;
		LARGE_LENGTH_TYPE   LengthToPad;

		if (TotalWidth <= Padded.GetCount())
			return Padded;

		LengthToPad = TotalWidth - Padded.GetCount();
		Status = Padded.ResizeBuffer(TotalWidth);
		if (NT_FAILED(Status))
			return Padded;

		for (; LengthToPad; --LengthToPad)
			Padded += PaddingChar;

		return Padded;
	}

	NoInline StringT Remove(LARGE_LENGTH_TYPE StartIndex, LARGE_LENGTH_TYPE Count = kMaxNumberValue) const
	{
		NTSTATUS            Status;
		StringT              NewString;
		StringImplement*    Impl;
		LARGE_LENGTH_TYPE   TailLength;

		if (StartIndex >= GetCount() || StartIndex == 0)
			return NewString;

		if (Count == kMaxNumberValue || StartIndex + Count >= GetCount())
			Count = GetCount() - StartIndex;

		TailLength = GetCount() - (StartIndex + Count);

		Status = NewString.ResizeBuffer(StartIndex + TailLength);
		if (NT_FAILED(Status))
			return NewString;

		Impl = NewString.GetImplement();
		Impl->CopyFrom(GetBuffer(), StartIndex);
		Impl->Concat(GetBuffer() + StartIndex + Count, TailLength);

		return NewString;
	}

	NoInline StringT Replace(const StringT& OldValue, const StringT& NewValue) const
	{
		return ReplaceWorker(OldValue, OldValue.GetCount(), NewValue, NewValue.GetCount());
	}

	NoInline StringT Replace(STRING_CONST_POINTER_TYPE OldValue, STRING_CONST_POINTER_TYPE NewValue) const
	{
		return ReplaceWorker(OldValue, StringImplement::GetStringCount(OldValue), NewValue, StringImplement::GetStringCount(NewValue));
	}

	NoInline StringT Replace(STRING_CHAR_TYPE OldValue, STRING_CHAR_TYPE NewValue) const
	{
		STRING_CHAR_TYPE Old[2], New[2];

		Old[0] = OldValue;
		Old[1] = 0;
		New[0] = NewValue;
		New[1] = 0;

		return ReplaceWorker(Old, 1, New, 1);
	}

	StringArray SplitLines(STRING_CONST_POINTER_TYPE Separator = L"\n") const
	{
		return Replace(L"\r\n", L"\n").Replace(L"\r", L"\n").Split(Separator);
	}

	StringArray Split(const STRING_CHAR_TYPE Separator, LARGE_LENGTH_TYPE MaxSplit = kMaxNumberValue) const
	{
		STRING_CHAR_TYPE Buffer[2];

		Buffer[0] = Separator;
		Buffer[1] = 0;

		return SplitWorker(Buffer, 1, MaxSplit);
	}

	NoInline StringArray Split(STRING_CONST_POINTER_TYPE Separator, LARGE_LENGTH_TYPE MaxSplit = kMaxNumberValue) const
	{
		return SplitWorker(Separator, StringImplement::GetStringCount(Separator), MaxSplit);
	}

	StringT Join(const StringArray& Values) const
	{
		return Join(*this, Values);
	}

	NoInline static StringT Join(const StringT& Separator, const StringArray& Values)
	{
		StringT Str;
		StringImplement* Impl;

		for (auto &value : Values)
		{
			Str += value + Separator;
		}

		Impl = Str.GetImplement();
		Impl->SetLength(Impl->GetLength() - Separator.GetSize());
		Impl->AddNull();

		return Str;
	}

	NoInline BOOL StartsWith(STRING_CONST_POINTER_TYPE Starts, BOOL CaseInSensitive = FALSE) const
	{
		return StartsWithWorker(Starts, StringImplement::GetStringCount(Starts), CaseInSensitive);
	}

	NoInline BOOL StartsWith(const StringT& Starts, BOOL CaseInSensitive = FALSE) const
	{
		return &Starts == this ? TRUE : StartsWithWorker(Starts, Starts.GetCount(), CaseInSensitive);
	}

	NoInline BOOL EndsWith(STRING_CONST_POINTER_TYPE Ends, BOOL CaseInSensitive = FALSE) const
	{
		return EndsWithWorker(Ends, StringImplement::GetStringCount(Ends), CaseInSensitive);
	}

	NoInline BOOL EndsWith(StringT& Ends, BOOL CaseInSensitive = FALSE) const
	{
		return &Ends == this ? TRUE : EndsWithWorker(Ends, Ends.GetCount(), CaseInSensitive);
	}

	NoInline StringT SubString(LARGE_LENGTH_TYPE StartIndex, LARGE_LENGTH_TYPE Count = kMaxNumberValue) const
	{
		StringT NewString;

		if (StartIndex >= GetCount())
			return NewString;

		if (Count == kMaxNumberValue || StartIndex + Count >= GetCount())
			Count = GetCount() - StartIndex;

		NewString.CopyFrom(GetBuffer() + StartIndex, Count);

		return NewString;
	}

	NoInline StringT ToLower() const
	{
		StringT NewString = *this;
		NewString.GetImplement()->ToLower();
		return NewString;
	}

	NoInline StringT ToUpper() const
	{
		StringT NewString = *this;
		NewString.GetImplement()->ToUpper();
		return NewString;
	}

	NoInline StringT TrimEnd(StringT TrimChars) const
	{
		STRING_POINTER_TYPE Begin, End;

		Begin = GetBuffer();
		End = Begin + GetCount();

		while (End > Begin)
		{
			if (TrimChars.IndexOf(End[-1]) == kInvalidIndex)
				break;

			--End;
		}

		if (End <= Begin)
			return StringT();

		return SubString(0, End - Begin);
	}

	NoInline StringT TrimStart(StringT TrimChars) const
	{
		STRING_POINTER_TYPE Begin, End;

		Begin = GetBuffer();
		End = Begin + GetCount();

		while (Begin != End)
		{
			if (TrimChars.IndexOf(Begin[0]) == kInvalidIndex)
				break;

			++Begin;
		}

		if (Begin == End)
			return StringT();

		return SubString(Begin - GetBuffer());
	}

	StringT Strip() const
	{
		return Trim(L" \t");
	}

	NoInline StringT Trim(StringT TrimChars) const
	{
		return TrimStart(TrimChars).TrimEnd(TrimChars);
	}

	NoInline ULONG64 ToHex() const
	{
		return StringToInt64HexW(*this);
	}

	NoInline ULONG64 ToInteger() const
	{
		return StringToInt64W(*this);
	}

	AnsiString Encode(ULONG_PTR Encoding = CP_ACP, ULONG_PTR Flags = 0) const
	{
		return Encode(*this, Encoding, Flags);
	}

	NoInline static StringT Decode(LPCVOID Bytes, ULONG_PTR Length, ULONG_PTR Encoding)
	{
		NTSTATUS    Status;
		PWSTR       UnicodeString;
		ULONG       UnicodeStringActualByteCount;
		StringT     Str;

		if (Encoding == CP_UTF16_LE)
		{
			if (*(PWSTR)Bytes == BOM_UTF16_LE)
			{
				Bytes = (PWSTR)Bytes + 1;
				Length -= sizeof(WCHAR);
			}

			Str.CopyFrom((PWSTR)Bytes, Length / sizeof(WCHAR));
			return Str;
		}
		else if (Encoding == CP_UTF8)
		{

			Status = RtlUTF8ToUnicodeN(nullptr, 0, &UnicodeStringActualByteCount, (PSTR)Bytes, Length);

			if (NT_FAILED(Status))
				return L"";

			UnicodeString = (PWSTR)AllocateMemory(UnicodeStringActualByteCount);
			if (UnicodeString == nullptr)
				return Str;

			Status = RtlUTF8ToUnicodeN(UnicodeString, UnicodeStringActualByteCount, &UnicodeStringActualByteCount, (PSTR)Bytes, Length);

			if (NT_SUCCESS(Status))
				Str.CopyFrom(UnicodeString, UnicodeStringActualByteCount / sizeof(UnicodeString[0]));

			FreeMemory(UnicodeString);

			return Str;
		}

		switch (Encoding)
		{
		case CP_ACP:
			Encoding = Nt_CurrentPeb()->AnsiCodePageData[1];
			break;

		case CP_OEMCP:
			Encoding = Nt_CurrentPeb()->OemCodePageData[1];
			break;
		}

		NtFileMemory NlsFile;
		CPTABLEINFO CodePageTable;

		Status = InitCodePageTable(&CodePageTable, NlsFile, Encoding);
		if (NT_FAILED(Status))
			return Str;

		CustomCPToUnicodeSize(&CodePageTable, &UnicodeStringActualByteCount, (PSTR)Bytes, Length);

		UnicodeString = (PWSTR)AllocateMemory(UnicodeStringActualByteCount);
		if (UnicodeString == nullptr)
			return Str;

		Status = RtlCustomCPToUnicodeN(&CodePageTable, UnicodeString, UnicodeStringActualByteCount, &UnicodeStringActualByteCount, (PSTR)Bytes, Length);
		if (NT_SUCCESS(Status))
			Str.CopyFrom(UnicodeString, UnicodeStringActualByteCount / sizeof(UnicodeString[0]));

		FreeMemory(UnicodeString);

		return Str;
	}

	NoInline static AnsiString Encode(const StringT& Str, ULONG_PTR Encoding = CP_ACP, ULONG_PTR Flags = 0)
	{
		NTSTATUS    Status;
		ULONG_PTR   Length;
		AnsiString  Array;


		if (Encoding == CP_UTF16_LE)
		{
			PWSTR Buffer;

			Length = (ULONG_PTR)Str.GetSize() + sizeof(WCHAR) * (1 + FLAG_ON(Flags, WithBOM));
			Status = Array.SetSize(Length);
			if (NT_FAILED(Status))
				return Array;

			Buffer = (PWSTR)Array.GetData();

			if (FLAG_ON(Flags, WithBOM))
			{
				Buffer[0] = BOM_UTF16_LE;
				++Buffer;
			}

			CopyMemory(Buffer, (PWSTR)Str, Str.GetSize());
			Buffer[Str.GetCount() + 1] = 0;

			Array.UpdateDataCount(Length);

			return Array;
		}
		else if (Encoding == CP_UTF8)
		{
			PSTR Buffer;
			ULONG UTF8StringActualByteCount;

#if ML_SUPPORT_XP

			UTF8StringActualByteCount = WideCharToMultiByte(CP_UTF8, 0, Str, Str.GetCount(), nullptr, 0, nullptr, nullptr);
			Status = UTF8StringActualByteCount == 0 ? NTSTATUS_FROM_WIN32(RtlGetLastWin32Error()) : STATUS_SUCCESS;

#else

			Status = RtlUnicodeToUTF8N(nullptr, 0, &UTF8StringActualByteCount, Str, (ULONG)Str.GetSize());
#endif

			if (NT_FAILED(Status))
				return Array;

			Length = UTF8StringActualByteCount + 1 + (FLAG_ON(Flags, WithBOM) ? 3 : 0);
			Status = Array.SetSize(Length);
			if (NT_FAILED(Status))
				return Array;

			Array.UpdateDataCount(Length);

			Buffer = (PSTR)Array.GetData();

			if (FLAG_ON(Flags, WithBOM))
			{
				*(PULONG)Buffer = BOM_UTF8;
				Buffer += 3;
				Length -= 3;
			}

#if ML_SUPPORT_XP

			UTF8StringActualByteCount = WideCharToMultiByte(CP_UTF8, 0, Str, Str.GetCount(), Buffer, Length, nullptr, nullptr);
			Status = UTF8StringActualByteCount == 0 ? NTSTATUS_FROM_WIN32(RtlGetLastWin32Error()) : STATUS_SUCCESS;

#else
			Status = RtlUnicodeToUTF8N(Buffer, Length, &UTF8StringActualByteCount, Str, (ULONG)Str.GetSize());
#endif

			if (NT_FAILED(Status))
			{
				Array.RemoveAll();
				return Array;
			}

			Buffer[UTF8StringActualByteCount] = 0;

			return Array;
		}
		else if (Encoding == CP_ACP)
		{
			PSTR Buffer;
			ULONG StringActualByteCount;

			Status = RtlUnicodeToMultiByteSize(&StringActualByteCount, Str, (ULONG)Str.GetSize());
			if (NT_FAILED(Status))
				return Array;

			Length = StringActualByteCount + 1;
			Status = Array.SetSize(Length);
			if (NT_FAILED(Status))
				return Array;

			Array.UpdateDataCount(Length);

			Buffer = (PSTR)Array.GetData();

			Status = RtlUnicodeToMultiByteN(Buffer, Length, &StringActualByteCount, Str, (ULONG)Str.GetSize());
			if (NT_FAILED(Status))
			{
				Array.RemoveAll();
				return Array;
			}

			Buffer[StringActualByteCount] = 0;

			return Array;
		}
		else if (Encoding == CP_OEMCP)
		{
			PSTR Buffer;
			ULONG StringActualByteCount;

			Status = RtlUnicodeToOemN(nullptr, 0, &StringActualByteCount, Str, (ULONG)Str.GetSize());
			if (NT_FAILED(Status))
				return Array;

			Length = StringActualByteCount + 1;
			Status = Array.SetSize(Length);
			if (NT_FAILED(Status))
				return Array;

			Array.UpdateDataCount(Length);

			Buffer = (PSTR)Array.GetData();

			Status = RtlUnicodeToOemN(Buffer, Length, &StringActualByteCount, Str, (ULONG)Str.GetSize());
			if (NT_FAILED(Status))
			{
				Array.RemoveAll();
				return Array;
			}

			Buffer[StringActualByteCount] = 0;

			return Array;
		}

		PSTR            Buffer;
		ULONG           BytesInMultiByteString;
		NtFileMemory    NlsFile;
		CPTABLEINFO     CodePageTable;

		Status = InitCodePageTable(&CodePageTable, NlsFile, Encoding);
		if (NT_FAILED(Status))
			return Array;

		UnicodeToCustomCPSize(&CodePageTable, &BytesInMultiByteString, Str, (ULONG)Str.GetSize());

		Status = Array.SetSize(BytesInMultiByteString);
		if (NT_FAILED(Status))
		{
			return Array;
		}

		Buffer = (PSTR)Array.GetData();
		Array.UpdateDataCount(BytesInMultiByteString);

		Status = RtlUnicodeToCustomCPN(&CodePageTable, Buffer, BytesInMultiByteString, &BytesInMultiByteString, Str, (ULONG)Str.GetSize());
		Array.Add(0);

		return Array;
	}

protected:
	NoInline static NTSTATUS InitCodePageTable(PCPTABLEINFO CodePageTable, NtFileMemory &NlsFile, ULONG_PTR Encoding)
	{
		NTSTATUS    Status;
		StringT     NlsFileName;

		Status = GetNlsFile(NlsFileName, Encoding);
		FAIL_RETURN(Status);

		Status = NlsFile.Open(StringT(L"\\SystemRoot\\system32\\") + NlsFileName, NFD_NOT_RESOLVE_PATH);
		FAIL_RETURN(Status);

		RtlInitCodePageTable((PUSHORT)NlsFile.GetBuffer(), CodePageTable);

		return STATUS_SUCCESS;
	}

	NoInline
		static
		NTSTATUS
		CustomCPToUnicodeSize(
			PCPTABLEINFO    CustomCP,
			PULONG          BytesInUnicodeString,
			PCSTR           MultiByteString,
			ULONG           BytesInMultiByteString
		)
	{
		ULONG   cbUnicode = 0;
		PUSHORT NlsLeadByteInfo;

		if (CustomCP->DBCSCodePage)
		{
			NlsLeadByteInfo = CustomCP->DBCSOffsets;

			while (BytesInMultiByteString--)
			{
				if (NlsLeadByteInfo[*(PUCHAR)MultiByteString++])
				{
					if (BytesInMultiByteString == 0)
					{
						cbUnicode += sizeof(WCHAR);
						break;
					}
					else
					{
						BytesInMultiByteString--;
						MultiByteString++;
					}
				}

				cbUnicode += sizeof(WCHAR);
			}

			*BytesInUnicodeString = cbUnicode;
		}
		else
		{
			*BytesInUnicodeString = BytesInMultiByteString * sizeof(WCHAR);
		}

		return STATUS_SUCCESS;
	}

	NoInline
		static
		NTSTATUS
		UnicodeToCustomCPSize(
			PCPTABLEINFO    CustomCP,
			PULONG          BytesInMultiByteString,
			PCWSTR          UnicodeString,
			ULONG           BytesInUnicodeString
		)
	{
		ULONG cbMultiByte = 0;
		ULONG CharsInUnicodeString;
		PUSHORT NlsUnicodeToMbAnsiData;

		CharsInUnicodeString = BytesInUnicodeString / sizeof(WCHAR);

		if (CustomCP->DBCSCodePage)
		{
			USHORT MbChar;

			NlsUnicodeToMbAnsiData = (PUSHORT)CustomCP->WideCharTable;

			while (CharsInUnicodeString--)
			{
				MbChar = NlsUnicodeToMbAnsiData[*UnicodeString++];
				if (HIBYTE(MbChar) == 0)
				{
					cbMultiByte++;
				}
				else
				{
					cbMultiByte += 2;
				}
			}

			*BytesInMultiByteString = cbMultiByte;
		}
		else
		{
			*BytesInMultiByteString = CharsInUnicodeString;
		}

		return STATUS_SUCCESS;
	}

	NoInline static NTSTATUS GetNlsFile(StringT& NlsFile, ULONG NlsIndex, PCWSTR SubKey = L"System\\CurrentControlSet\\Control\\Nls\\CodePage")
	{
		BOOL        Success;
		WCHAR       NlsIndexBuffer[16];
		NTSTATUS    Status;
		PKEY_VALUE_PARTIAL_INFORMATION FileName;

		_snwprintf(NlsIndexBuffer, countof(NlsIndexBuffer), L"%d", NlsIndex);

		Status = Reg::GetKeyValue(HKEY_LOCAL_MACHINE, SubKey, NlsIndexBuffer, &FileName);
		FAIL_RETURN(Status);

		NlsFile.CopyFrom((PWSTR)FileName->Data, FileName->DataLength / sizeof(WCHAR));

		Reg::FreeKeyInfo(FileName);

		return STATUS_SUCCESS;
	}

	StringImplement* GetImplement() const
	{
		return this->Buffer == nullptr ? nullptr : FIELD_BASE(this->Buffer, StringImplement, Buffer);
	}

	VOID SetImplement(StringImplement *Impl)
	{
		this->Buffer = Impl->Buffer;
	}

	VOID Reset()
	{
		this->Buffer = nullptr;
	}

	NTSTATUS ReleaseImplement(StringImplement *Impl)
	{
		return FreeMemory(Impl) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
	}

	NTSTATUS ReleaseBuffer()
	{
		StringImplement* Impl = GetImplement();

		if (Impl != nullptr)
		{
			ReleaseImplement(Impl);
		}

		Reset();

		return STATUS_SUCCESS;
	}

	NTSTATUS AllocateBuffer(LARGE_LENGTH_TYPE Count)
	{
		return ResizeBuffer(Count);
	}

	NTSTATUS ResizeBuffer(LARGE_LENGTH_TYPE Count)
	{
		LARGE_LENGTH_TYPE   Length, MaxLength;
		STRING_TYPE         Buffer;
		StringImplement     *Impl, *OldImpl;

		Length = StringImplement::CountToLength(Count);
		if (Length > kMaxLength)
			return STATUS_NAME_TOO_LONG;

		OldImpl = GetImplement();

		LOOP_ONCE
		{
			if (OldImpl == nullptr)
			break;

			if (OldImpl->GetMaxLength() > Length)
				return STATUS_SUCCESS;
		}

		Length = StringImplement::CountToLengthAddNull(Count);
		MaxLength = ROUND_UP(Length * 3 / 2, kCharSize);
		Impl = (StringImplement *)AllocateMemory((ULONG_PTR)(kStringImplSize + MaxLength));
		if (Impl == nullptr)
			return STATUS_NO_MEMORY;

		Impl->SetBuffer(Impl->Buffer, MaxLength);

		if (OldImpl != nullptr)
		{
			Impl->CopyBuffer(OldImpl);
			ReleaseImplement(OldImpl);
		}

		SetImplement(Impl);

		return STATUS_SUCCESS;
	}

	StringT
		ReplaceWorker(
			STRING_CONST_POINTER_TYPE   OldValue,
			LARGE_LENGTH_TYPE           OldCount,
			STRING_CONST_POINTER_TYPE   NewValue,
			LARGE_LENGTH_TYPE           NewCount
		) const
	{
		StringT                     NewString;
		LARGE_LENGTH_TYPE           StartIndex, Sub;
		STRING_CONST_POINTER_TYPE   Base, Begin, End;
		StringImplement*            Impl;

		Impl = GetImplement();

		Base = GetBuffer();
		Begin = Base;
		End = Begin + GetCount();
		StartIndex = 0;

		while (Begin < End)
		{
			Sub = Impl->IndexOf(OldValue, StartIndex);
			if (Sub == kInvalidIndex)
				break;

			if (StartIndex != Sub)
				NewString.Concat(Base + StartIndex, Sub - StartIndex);

			NewString.Concat(NewValue, NewCount);

			StartIndex = Sub + OldCount;
			Begin = Base + StartIndex;
		}

		if (Begin < End)
			NewString.Concat(Begin, End - Begin);

		return NewString;
	}

	StringArray SplitWorker(STRING_CONST_POINTER_TYPE Separator, LARGE_LENGTH_TYPE Length, LARGE_LENGTH_TYPE MaxSplit) const
	{
		StringArray                 Array;
		LARGE_LENGTH_TYPE           StartIndex, Sep;
		STRING_CONST_POINTER_TYPE   Begin, End, Base;
		StringImplement*            Impl;

		Impl = GetImplement();

		Base = GetBuffer();
		Begin = Base;
		End = Begin + GetCount();
		StartIndex = 0;

		for (; MaxSplit; --MaxSplit)
		{
			Sep = Impl->IndexOf(Separator, StartIndex);
			if (Sep == kInvalidIndex)
				break;

			//if (Sep != StartIndex)
			{
				Array.Add(SubString(StartIndex, Sep - StartIndex));
			}
			//else
			{
				//Array.Add(L"");
			}

			StartIndex = Sep + Length;
			Begin = Base + StartIndex;
		}

		if (Begin < End)
		{
			Array.Add(SubString(Begin - Base));
		}

		return Array;
	}

	BOOL StartsWithWorker(STRING_CONST_POINTER_TYPE Starts, LARGE_LENGTH_TYPE Count, BOOL CaseInSensitive)
	{
		StringImplement* Impl;

		Impl = GetImplement();

		if (Count == 0 && Impl->GetCount() == 0)
			return TRUE;

		if (Count > Impl->GetCount())
			return FALSE;

		if (Count == Impl->GetCount())
			return Compare(Starts, Count, CaseInSensitive) == 0;

		return SubString(0, Count).Compare(Starts, Count, CaseInSensitive) == 0;
	}

	BOOL EndsWithWorker(STRING_CONST_POINTER_TYPE Ends, LARGE_LENGTH_TYPE Count, BOOL CaseInSensitive) const
	{
		StringImplement* Impl;

		Impl = GetImplement();

		if (Count == 0 && Impl->GetCount() == 0)
			return TRUE;

		if (Count > Impl->GetCount())
			return FALSE;

		if (Count == Impl->GetCount())
			return Compare(Ends, Count, CaseInSensitive) == 0;

		return SubString(Impl->GetCount() - Count).Compare(Ends, Count, CaseInSensitive) == 0;
	}

private:
	static PVOID AllocateMemory(ULONG_PTR Size)
	{

#if STRING_DEBUG
		++DebugAllocCount;
#endif

		return AllocateMemoryP(Size);
	}

	static BOOL FreeMemory(PVOID Memory)
	{

#if STRING_DEBUG
		--DebugAllocCount;
		if (DebugAllocCount < 0)
			RtlRaiseException(0);
#endif

		return FreeMemoryP(Memory);
	}


#if STRING_DEBUG
	static LONG_PTR DebugAllocCount;
#endif
};

#if STRING_DEBUG

DECL_SELECTANY LONG_PTR StringT<>::DebugAllocCount = 0;

#endif

#if USE_TEMPLATE

typedef StringT<ULONG, ULONG64> String;
//typedef StringT<ULONG, ULONG64> LargeString;

#else

typedef StringT String;

#endif

typedef String::StringArray StringArray;
typedef String::ByteArray ByteArray;

#define CONVERSION_BUFFER_LENGTH 128
#define MATRIX_CONVERSION_BUFFER_LENGTH 256;


inline unsigned int MurmurHash32(const void * key, int len, unsigned int seed = 0xEE6B27EB)
{
	// 'm' and 'r' are mixing constants generated offline.
	// They're not really 'magic', they just happen to work well.

	const unsigned int m = 0x5bd1e995;
	const int r = 24;

	// Initialize the hash to a 'random' value

	unsigned int h = seed ^ len;

	// Mix 4 bytes at a time into the hash

	const unsigned char * data = (const unsigned char *)key;

	while (len >= 4)
	{
		unsigned int k = *(unsigned int *)data;

		k *= m;
		k ^= k >> r;
		k *= m;

		h *= m;
		h ^= k;

		data += 4;
		len -= 4;
	}

	// Handle the last few bytes of the input array

	switch (len)
	{
	case 3: h ^= data[2] << 16;
	case 2: h ^= data[1] << 8;
	case 1: h ^= data[0];
		h *= m;
	};

	// Do a few final mixes of the hash to ensure the last few
	// bytes are well-incorporated.

	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;

	return h;
}

#pragma warning(pop)

LONG_PTR MlInitialize();
LONG_PTR MlUnInitialize();


template<typename TYPE, ULONG_PTR INITIAL_TABLE_SIZE = 521>
class HashTableT
{
	typedef TypeTraits<TYPE> VALUE_TYPE;

	typedef typename VALUE_TYPE::VALUE_TYPE      VALUE;
	typedef typename VALUE_TYPE::REF_TYPE        VALUE_REF;
	typedef typename VALUE_TYPE::CONST_REF_TYPE  CONST_VALUE_REF;
	typedef VALUE   *PVALUE;


	typedef struct HASH_VALUE
	{
		ULONG           Index;
		ULARGE_INTEGER  Key;

		HASH_VALUE()
		{
			Reset();
		}

		ForceInline BOOL Valid()
		{
			return this->Key.QuadPart != 0;
		}

		ForceInline VOID Reset()
		{
			ZeroMemory(this, sizeof(*this));
		}

		BOOL operator== (const HASH_VALUE& that)
		{
			return this->Key.QuadPart == that.Key.QuadPart;
		}

	} HASH_VALUE;

	typedef struct
	{
		HASH_VALUE  Hash;
		VALUE       Element;

	} HASH_TABLE_ITEM, *PHASH_TABLE_ITEM;


	typedef HASH_TABLE_ITEM HashTableEntry;

	template<class KEY_TYPE>
	class Hasher
	{
	public:
		static HASH_VALUE Hash(HashTableT* thiz, typename TypeTraits<KEY_TYPE>::CONST_REF_TYPE Key);
	};

#define HASHER_END };
#define HASHER(type, argtype) \
    template<> class Hasher<type> \
												    { \
    public: \
        static HASH_VALUE Hash(HashTableT* thiz, argtype Key)

	HASHER(PWSTR, PWSTR)
	{
		return thiz->HashString(Key);
	}
	HASHER_END

		HASHER(PCWSTR, PCWSTR)
	{
		return thiz->HashString(Key);
	}
	HASHER_END

		HASHER(INT32, INT32)
	{
		return thiz->HashData(&Key, sizeof(Key));
	}
	HASHER_END

		HASHER(INT64, INT64)
	{
		return thiz->HashData(&Key, sizeof(Key));
	}
	HASHER_END

		HASHER(UINT32, UINT32)
	{
		return thiz->HashData(&Key, sizeof(Key));
	}
	HASHER_END

		HASHER(UINT64, UINT64)
	{
		return thiz->HashData(&Key, sizeof(Key));
	}
	HASHER_END

		HASHER(LONG, LONG)
	{
		return thiz->HashData(&Key, sizeof(Key));
	}
	HASHER_END

		HASHER(ULONG, ULONG)
	{
		return thiz->HashData(&Key, sizeof(Key));
	}
	HASHER_END

public:
	NoInline HashTableT()
	{
		this->Entries = nullptr;
		this->CalcTable = nullptr;
		this->TableUsedSize = 0;
		this->TableSize = 0;

		this->GrowthFactor = 1.3;
	}

	NoInline ~HashTableT()
	{
		SafeDeleteT(this->CalcTable);
		SafeDeleteArrayT(this->Entries);
	}

	NoInline NTSTATUS Initialize()
	{
		HashTableEntry *Entry;

		ml::MlInitialize();
		this->CalcTable = new ULONG[this->CalcTableSize];
		if (this->CalcTable == nullptr)
			return STATUS_NO_MEMORY;
		
		ULONG Seed = 0x00100001;

		for (int index1 = 0; index1 != 0x100; index1++)
		{
			for (int index2 = index1, i = 5; i != 0; index2 += 0x100, --i)
			{
				ULONG temp1, temp2;

				Seed = (Seed * 125 + 3) % 0x2AAAAB;
				temp1 = (Seed & 0xFFFF) << 16;
				Seed = (Seed * 125 + 3) % 0x2AAAAB;
				temp2 = (Seed & 0xFFFF);
				this->CalcTable[index2] = temp1 | temp2;
			}
		}

		return STATUS_SUCCESS;
	}

	ULONG_PTR Count()
	{
		return this->TableUsedSize;
	}

	ULONG_PTR Size()
	{
		return this->TableSize;
	}
	
	NoInline HASH_VALUE HashString(PCSTR Ansi, ULONG_PTR Length = -1)
	{
		PSTR Local;

		if (Length == -1)
			Length = StrLengthA(Ansi);

		Local = (PSTR)Ansi;

		return HashData(Local, Length);
	}

	NoInline HASH_VALUE HashString(PCWSTR Unicode, ULONG_PTR Length = -1)
	{
		PWSTR Local;

		if (Length == -1)
			Length = StrLengthW(Unicode);

		Length *= sizeof(Unicode[0]);
		Local = (PWSTR)Unicode;

		return HashData(Local, Length);
	}

	ForceInline VOID Update(ULONG b, ULONG& v1, ULONG& v2, ULONG t)
	{
		v1 = this->CalcTable[(t << 8) + b] ^ (v1 + v2);
		v2 = b + v1 + v2 + (v2 << 5) + 3;
	}

	NoInline HASH_VALUE HashData(PVOID Bytes, ULONG_PTR Length)
	{
		ULONG       Seed1, Seed2, Seed3, b;
		PBYTE       Data;
		HASH_VALUE  Hash;

		Data = (PBYTE)Bytes;

		Hash.Index = 0x7FED7FED;
		Hash.Key.LowPart = 0x7FED7FED;
		Hash.Key.HighPart = 0x7FED7FED;

		Seed1 = 0xEEEEEEEE;
		Seed2 = 0xEEEEEEEE;
		Seed3 = 0xEEEEEEEE;
		for (; Length; Length--)
		{
			b = *Data++;

			Update(b, Hash.Index, Seed1, 0);
			Update(b, Hash.Key.LowPart, Seed2, 1);
			Update(b, Hash.Key.HighPart, Seed3, 2);
		}

		return Hash;
	}

	template<typename KEY_TYPE>
	NoInline VALUE_REF Add(const KEY_TYPE& Key, CONST_VALUE_REF Value)
	{
		return AddElement(Hasher<KEY_TYPE>::Hash(this, Key), Value).Element;
	}

	NoInline VALUE_REF Add(PVOID Key, ULONG_PTR Length, CONST_VALUE_REF Value)
	{
		return AddElement(HashData(Key, Length), Value).Element;
	}

	template<typename KEY_TYPE>
	NoInline PVALUE Get(const KEY_TYPE& Key)
	{
		return LookupElement(Hasher<KEY_TYPE>::Hash(this, Key));
	}

	NoInline PVALUE Get(PVOID Key, ULONG_PTR Length)
	{
		return LookupElement(HashString(Key, Length));
	}

	template<class STRING_TYPE>
	NoInline BOOL Contains(STRING_TYPE StringKey)
	{
		return Get(StringKey) != nullptr;
	}

	NoInline BOOL Contains(PVOID Bytes, ULONG_PTR Length)
	{
		return Get(Bytes, Length) != nullptr;
	}

	template<class STRING_TYPE>
	NoInline VOID Remove(STRING_TYPE StringKey)
	{
		RemoveElement(HashString(StringKey));
	}

	NoInline VOID Remove(PVOID Bytes, ULONG_PTR Length)
	{
		RemoveElement(HashData(Bytes, Length));
	}

protected:
	NoInline NTSTATUS IncreaseCapacity()
	{
		if (this->TableSize * 3 / 4 > this->TableUsedSize)
			return STATUS_SUCCESS;

		ULONG_PTR NewSize, OldSize;
		HashTableEntry *NewEntries, *OldEntries, *Entry;

		NewSize = GetTableSize(this->TableSize == 0 ? INITIAL_TABLE_SIZE : (ULONG_PTR)(this->TableSize * this->GrowthFactor));
		NewEntries = new HashTableEntry[NewSize];
		if (NewEntries == nullptr)
			return STATUS_NO_MEMORY;

		OldEntries = this->Entries;
		OldSize = this->TableSize;

		this->Entries = NewEntries;
		this->TableSize = NewSize;
		this->TableUsedSize = 0;

		if (OldEntries == nullptr)
			return STATUS_SUCCESS;

		FOR_EACH(Entry, OldEntries, OldSize)
		{
			if (Entry->Hash.Valid())
				AddElement(Entry->Hash, Entry->Element);
		}

		delete[] OldEntries;

		return STATUS_SUCCESS;
	}

	NoInline HASH_TABLE_ITEM& AddElement(const HASH_VALUE& Hash, CONST_VALUE_REF Element)
	{
		IncreaseCapacity();

		auto Entry = LookupEntry(Hash, TRUE);

		Entry->Hash = Hash;
		Entry->Element = Element;
		++this->TableUsedSize;

		return *Entry;
	}

	NoInline VOID RemoveElement(const HASH_VALUE& Hash)
	{
		auto Entry = LookupEntry(Hash, FALSE);

		if (Entry == nullptr)
			return;

		Entry->Hash.Reset();
		Entry->Element.~TYPE();
		--this->TableUsedSize;
	}

	NoInline PVALUE LookupElement(const HASH_VALUE& Hash)
	{
		auto Entry = this->LookupEntry(Hash, FALSE);
		return Entry == nullptr ? nullptr : &Entry->Element;
	}

	NoInline HashTableEntry* LookupEntry(const HASH_VALUE& Hash, BOOL Empty)
	{
		ULONG_PTR       Index, InitialIndex;
		HashTableEntry* Entry;

		if (this->TableSize == 0)
			return nullptr;

		InitialIndex = Hash.Index % this->TableSize;
		Index = InitialIndex;
		Entry = &this->Entries[InitialIndex];

		if (Empty == FALSE)
		{
			do
			{
				if (Entry->Hash == Hash)
					return Entry;

				if (Entry->Hash.Valid() == FALSE)
					return nullptr;

				++Index;
				++Entry;
				Index = Index == this->TableSize ? 0 : Index;
				Entry = Index == 0 ? this->Entries : Entry;

			} while (Index != InitialIndex);
		}
		else
		{
			do
			{
				if (Entry->Hash == Hash)
					return Entry;

				if (Entry->Hash.Valid() == FALSE)
					return Entry;

				++Index;
				++Entry;
				Index = Index == this->TableSize ? 0 : Index;
				Entry = Index == 0 ? this->Entries : Entry;

			} while (Index != InitialIndex);
		}

		return nullptr;
	}

	BOOL MillerRabin(ULONG64 n, ULONG_PTR k)
	{
		if (n == k)
			return TRUE;

		if (n == 2 || n == 3)
			return TRUE;

		if (n <= 1 || !(n & 1))
			return FALSE;

		ULONG64 s, d, b, e, x;

		// Factor n-1 as d 2^s
		for (s = 0, d = n - 1; !(d & 1); s++)
			d >>= 1;

		// x = k^d mod n using exponentiation by squaring
		// The squaring overflows for n >= 2^32
		for (x = 1, b = k % n, e = d; e; e >>= 1)
		{
			if (e & 1)
				x = (x * b) % n;

			b = (b * b) % n;
		}

		// Verify k^(d 2^[0��s-1]) mod n != 1
		if (x == 1 || x == n - 1)
			return TRUE;

		while (s-- > 1)
		{
			x = (x * x) % n;
			if (x == 1)
				return FALSE;

			if (x == n - 1)
				return TRUE;
		}

		return FALSE;
	}

	BOOL IsPrime(ULONG_PTR n)
	{
		return (n > 73 &&
			!(n % 2 && n % 3 && n % 5 && n % 7 &&
				n % 11 && n % 13 && n % 17 && n % 19 && n % 23 && n % 29 &&
				n % 31 && n % 37 && n % 41 && n % 43 && n % 47 && n % 53 &&
				n % 59 && n % 61 && n % 67 && n % 71 && n % 73)
			) ? FALSE :
			MillerRabin(n, 2) && MillerRabin(n, 7) && MillerRabin(n, 61);
	}

	ULONG_PTR GetTableSize(ULONG_PTR TableSize)
	{
		if ((TableSize & 1) == 0)
			++TableSize;

		while (IsPrime(TableSize) == FALSE)
			TableSize += 2;

		return TableSize;
	}

protected:
	HashTableEntry* Entries;
	PULONG          CalcTable;
	ULONG_PTR       TableSize;
	ULONG_PTR       TableUsedSize;
	DOUBLE          GrowthFactor;

	static const ULONG_PTR CalcTableSize = 0x500;
};
//Map



	ML_NAMESPACE_BEGIN(Native)

		ML_NAMESPACE_BEGIN(Ps);

	static const HANDLE CurrentProcess = NtCurrentProcess();
	static const HANDLE CurrentThread = NtCurrentThread();

#define INVALID_SESSION_ID  ULONG_PTR_MAX
#define INVALID_CLIENT_ID   ULONG_PTR_MAX

	ForceInline ULONG_PTR CurrentPid()
	{
		return (ULONG_PTR)Nt_CurrentTeb()->ClientId.UniqueProcess;
	}

	ForceInline ULONG_PTR CurrentTid()
	{
		return (ULONG_PTR)Nt_CurrentTeb()->ClientId.UniqueThread;
	}

	ForceInline PPEB CurrentPeb()
	{
		return Nt_CurrentPeb();
	}

	ForceInline PTEB CurrentTeb()
	{
		return Nt_CurrentTeb();
	}

	ULONG_PTR
		HandleToProcessId(
			HANDLE Process
		);

	ULONG_PTR
		HandleToThreadId(
			HANDLE Thread
		);

	HANDLE
		ProcessIdToHandle(
			ULONG_PTR ProcessId,
			ULONG_PTR Access = PROCESS_ALL_ACCESS
		);

	NTSTATUS
		ProcessIdToHandleEx(
			PHANDLE     ProcessHandle,
			ULONG_PTR   ProcessId,
			ULONG_PTR   Access = PROCESS_ALL_ACCESS
		);

	HANDLE
		ThreadIdToHandle(
			ULONG_PTR ThreadId,
			ULONG_PTR Access = THREAD_ALL_ACCESS
		);

	NTSTATUS
		ThreadIdToHandleEx(
			PHANDLE     ThreadHandle,
			ULONG_PTR   ThreadId,
			ULONG_PTR   Access = THREAD_ALL_ACCESS
		);

	ForceInline ULONG_PTR HandleToPid(HANDLE Process)
	{
		return HandleToProcessId(Process);
	}

	ForceInline ULONG_PTR HandleToTid(HANDLE Process)
	{
		return HandleToThreadId(Process);
	}

	ULONG_PTR
		HandleToThreadId(
			HANDLE Thread
		);

	template<class PidType>
	ForceInline NTSTATUS PidToHandleEx(PHANDLE ProcessHandle, PidType ProcessId, ULONG_PTR Access = PROCESS_ALL_ACCESS)
	{
		return ProcessIdToHandleEx(ProcessHandle, (ULONG_PTR)ProcessId, Access);
	}

	ForceInline HANDLE ProcessIdToHandle(HANDLE ProcessId, ULONG_PTR Access = PROCESS_ALL_ACCESS)
	{
		return ProcessIdToHandle((ULONG_PTR)ProcessId, Access);
	}

	template<class PidType>
	ForceInline HANDLE PidToHandle(PidType ProcessId, ULONG_PTR Access = PROCESS_ALL_ACCESS)
	{
		return ProcessIdToHandle((ULONG_PTR)ProcessId, Access);
	}

	ForceInline HANDLE ThreadIdToHandle(HANDLE ThreadId, ULONG_PTR Access = THREAD_ALL_ACCESS)
	{
		return ThreadIdToHandle((ULONG_PTR)ThreadId, Access);
	}

	template<class TidType>
	ForceInline HANDLE TidToHandle(TidType ThreadId, ULONG_PTR Access = THREAD_ALL_ACCESS)
	{
		return ThreadIdToHandle((ULONG_PTR)ThreadId, Access);
	}

	ULONG_PTR
		GetSessionId(
			HANDLE Process
		);

	ULONG_PTR
		GetSessionId(
			ULONG_PTR ProcessId
		);

	inline ULONG_PTR GetCurrentSessionId()
	{
		return GetSessionId(CurrentProcess);
	}

	BOOL
		IsWow64Process(
			HANDLE Process = CurrentProcess
		);

	PSYSTEM_PROCESS_INFORMATION QuerySystemProcesses();

	typedef struct ML_THREAD_CONTEXT
	{
		union
		{
			struct
			{
				BOOLEAN Rdi : 1;
				BOOLEAN Rsi : 1;
				BOOLEAN Rbp : 1;
				BOOLEAN Rsp : 1;
				BOOLEAN Rbx : 1;
				BOOLEAN Rdx : 1;
				BOOLEAN Rcx : 1;
				BOOLEAN Rax : 1;
			};

			ULONG_PTR Value;

		} Flags;

		ULONG_PTR Rdi;
		ULONG_PTR Rsi;
		ULONG_PTR Rbp;
		ULONG_PTR Rsp;
		ULONG_PTR Rbx;
		ULONG_PTR Rdx;
		ULONG_PTR Rcx;
		ULONG_PTR Rax;

		ML_THREAD_CONTEXT()
		{
			Flags.Value = 0;
			RtlZeroMemory(this, sizeof(*this));
		}

	} ML_THREAD_CONTEXT, *PML_THREAD_CONTEXT;

	VOID
		CDECL
		CaptureRegisters(
			PML_THREAD_CONTEXT Context
		);

	VOID
		CDECL
		SetRegisters(
			PML_THREAD_CONTEXT Context
		);

	enum
	{
		ThreadNoAlertable = FALSE,
		ThreadAlertable = TRUE,
	};

	VOID
		Sleep(
			ULONG_PTR   Milliseconds,
			BOOL        Alertable = ThreadNoAlertable
		);

#if ML_USER_MODE

	PTEB_ACTIVE_FRAME
		FindThreadFrame(
			ULONG_PTR Context
		);

	PTEB_ACTIVE_FRAME
		FindThreadFrameEx(
			ULONG_PTR Context,
			ULONG_PTR Data
		);

	NTSTATUS
		CreateProcess(
			PCWSTR                  ApplicationName,
			PWSTR                   CommandLine = NULL,
			PCWSTR                  CurrentDirectory = NULL,
			ULONG                   CreationFlags = 0,
			LPSTARTUPINFOW          StartupInfo = NULL,
			LPPROCESS_INFORMATION   ProcessInformation = NULL,
			LPSECURITY_ATTRIBUTES   ProcessAttributes = NULL,
			LPSECURITY_ATTRIBUTES   ThreadAttributes = NULL,
			PVOID                   Environment = NULL,
			HANDLE                  Token = NULL,
			BOOL                    InheritHandles = FALSE
		);


#define CPWD_NORMAL                 0
#define CPWD_BEFORE_KERNEL32        (1 << 0)
#define CPWD_BEFORE_ENTRY_POINT     (1 << 1)
#define CPWD_BEFORE_TLS_CALLBACK    (1 << 2)

	typedef struct ML_PROCESS_INFORMATION : public PROCESS_INFORMATION
	{
		PVOID FirstCallLdrLoadDll;

	} ML_PROCESS_INFORMATION, *PML_PROCESS_INFORMATION;

	NTSTATUS
		CreateProcessWithDll(
			ULONG_PTR               Flags,
			PCWSTR                  DllPath,
			PCWSTR                  ApplicationName,
			PWSTR                   CommandLine = NULL,
			PCWSTR                  CurrentDirectory = NULL,
			ULONG                   CreationFlags = 0,
			LPSTARTUPINFOW          StartupInfo = NULL,
			PML_PROCESS_INFORMATION ProcessInformation = NULL,
			LPSECURITY_ATTRIBUTES   ProcessAttributes = NULL,
			LPSECURITY_ATTRIBUTES   ThreadAttributes = NULL,
			PVOID                   Environment = NULL,
			HANDLE                  Token = NULL
		);

	NTSTATUS
		CreateThread(
			PVOID                   StartAddress,
			PVOID                   StartParameter = NULL,
			BOOL                    CreateSuspended = FALSE,
			HANDLE                  ProcessHandle = Ps::CurrentProcess,
			PHANDLE                 ThreadHandle = NULL,
			PCLIENT_ID              ClientID = NULL,
			ULONG                   ZeroBits = 0,
			ULONG_PTR               MaximumStackSize = 0,
			ULONG_PTR               CommittedStackSize = 0,
			PSECURITY_DESCRIPTOR    SecurityDescriptor = NULL
		);

	DECL_NORETURN
		VOID
		ExitProcess(
			NTSTATUS ExitStatus
		);

	inline PWSTR GetCommandLine()
	{
		return (PWSTR)CurrentPeb()->ProcessParameters->CommandLine.Buffer;
	}

#define ThreadCallbackM(...) (PTHREAD_START_ROUTINE)(LambdaCastHelper<ULONG(NTAPI *)(__VA_ARGS__)>::FUNC)[] (__VA_ARGS__) -> ULONG

#define ThreadLambdaType(_type, ...) [] (_type __VA_ARGS__) -> ULONG
#define ThreadLambdaType_(_type, ...)  (PTHREAD_START_ROUTINE)ThreadLambdaType(_type, __VA_ARGS__)
#define ThreadLambda(...) ThreadLambdaType(ULONG_PTR, __VA_ARGS__)

	template<typename F, typename P>
	NTSTATUS
		CreateThreadT(
			const F&                StartAddress,
			P                       StartParameter = nullptr,
			BOOL                    CreateSuspended = FALSE,
			HANDLE                  ProcessHandle = Ps::CurrentProcess,
			PHANDLE                 ThreadHandle = nullptr,
			PCLIENT_ID              ClientID = nullptr,
			ULONG                   StackZeroBits = 0,
			ULONG                   StackReserved = 0,
			ULONG                   StackCommit = 0,
			PSECURITY_DESCRIPTOR    SecurityDescriptor = nullptr
		)
	{
		NTSTATUS Status;

		typedef struct StartParameterWrapper
		{
			ml::Function<ULONG(P)> func;
			P StartParameter;

			StartParameterWrapper(const F &StartAddress, const P &Parameter) : func(StartAddress), StartParameter(Parameter)
			{
			}

		} StartParameterWrapper;

		StartParameterWrapper *func;

		func = new StartParameterWrapper(StartAddress, StartParameter);
		if (func == NULL)
			return STATUS_NO_MEMORY;

		Status = CreateThread(PTHREAD_START_ROUTINE(
			[](PVOID Function) -> ULONG
		{
			ULONG RetValue;
			StartParameterWrapper *func = (StartParameterWrapper *)Function;

			RetValue = (func->func)(func->StartParameter);

			delete func;

			return RetValue;
		}),
			func,
			CreateSuspended, ProcessHandle, ThreadHandle, ClientID, StackZeroBits, StackReserved, StackCommit, SecurityDescriptor
			);

		if (NT_FAILED(Status))
		{
			delete func;
		}

		return Status;
	}

	template<typename F>
	NTSTATUS QueueApcRoutine(const F &ApcFunction, HANDLE Thread = CurrentThread)
	{
		NTSTATUS Status;
		ml::Function<void()> *func;

		func = new ml::Function<void()>(ApcFunction);
		if (func == NULL)
			return STATUS_NO_MEMORY;

		Status = NtQueueApcThread(Thread,
			[](PVOID ApcFunction, PVOID, PVOID) -> VOID
		{
			ml::Function<void()> *func = (ml::Function<void()> *)ApcFunction;

			(*func)();

			delete func;
		},
			func,
			NULL,
			NULL
			);

		if (NT_FAILED(Status))
		{
			delete func;
		}

		return Status;
	}

#else // r0

#define LambdaKernelApc(Apc) [] (PKAPC Apc, PKNORMAL_ROUTINE* NormalRoutine, PVOID* NormalContext, PVOID* SystemArgument1, PVOID* SystemArgument2) -> VOID
#define LambdaNormalApc(NormalContext) [] (PVOID NormalContext, PVOID, PVOID) -> VOID
#define NormalApcCallbackM(...) (PKNORMAL_ROUTINE)(LambdaCastHelper<VOID(NTAPI *)(__VA_ARGS__, PVOID, PVOID)>::FUNC)[] (__VA_ARGS__, PVOID, PVOID) -> VOID

	typedef struct
	{
		KAPC                Apc;
		PKKERNEL_ROUTINE    Callback;
		PVOID               Argument1;
		PVOID               Argument2;

	} ML_KERNEL_APC, *PML_KERNEL_APC;

	inline
		NTSTATUS
		QueueNormalApc(
			PETHREAD            Thread,
			PKNORMAL_ROUTINE    NormalRoutine,
			PVOID               NormalContext = nullptr,
			KPRIORITY           Increment = IO_NO_INCREMENT
		)
	{
		PKAPC       Apc;
		NTSTATUS    Status;

		Apc = new KAPC;
		if (Apc == nullptr)
			return STATUS_NO_MEMORY;

		KeInitializeApc(Apc, Thread, OriginalApcEnvironment,
			LambdaKernelApc(Apc)
		{
			delete Apc;
		},
			nullptr,
			NormalRoutine,
			KernelMode,
			NormalContext
			);

		if (KeInsertQueueApc(Apc, Apc, nullptr, Increment) == FALSE)
		{
			delete Apc;
			return STATUS_UNSUCCESSFUL;
		}

		return STATUS_SUCCESS;
	}

	inline
		NTSTATUS
		QueueNormalApc(
			HANDLE              ThreadId,
			PKNORMAL_ROUTINE    NormalRoutine,
			PVOID               NormalContext = nullptr,
			KPRIORITY           Increment = IO_NO_INCREMENT
		)
	{
		NTSTATUS Status;
		PETHREAD Thread;

		Status = PsLookupThreadByThreadId(ThreadId, &Thread);
		FAIL_RETURN(Status);

		Status = QueueNormalApc(Thread, NormalRoutine, NormalContext, Increment);
		ObDereferenceObject(Thread);

		return Status;
	}

	inline PETHREAD TidToThreadObject(HANDLE ThreadId)
	{
		PETHREAD Thread;

		return NT_FAILED(PsLookupThreadByThreadId(ThreadId, &Thread)) ? nullptr : Thread;
	}

	inline PEPROCESS PidToProcessObject(HANDLE ProcessId)
	{
		PEPROCESS Process;

		return NT_FAILED(PsLookupProcessByProcessId(ProcessId, &Process)) ? nullptr : Process;
	}

#endif // r0

	ML_NAMESPACE_END_(Ps);


	ML_NAMESPACE_BEGIN(Ldr);

	inline PVOID GetExeModuleHandle()
	{
		return (PVOID)Ps::CurrentPeb()->ImageBaseAddress;
	}

	LDR_MODULE*
		FindLdrModuleByName(
			PUNICODE_STRING ModuleName
		);

	LDR_MODULE*
		FindLdrModuleByHandle(
			PVOID BaseAddress
		);

	PIMAGE_RESOURCE_DATA_ENTRY
		FindResource(
			PVOID   Module,
			PCWSTR  Name,
			PCWSTR  Type
		);

	PVOID
		LoadResource(
			PVOID                       Module,
			PIMAGE_RESOURCE_DATA_ENTRY  ResourceDataEntry,
			PULONG                      Size = nullptr
		);

#define LOAD_PE_IGNORE_IAT              0x00000001
#define LOAD_PE_IGNORE_RELOC            0x00000002
#define LOAD_PE_DLL_NOT_FOUND_CONTINUE  0x00000004
#define LOAD_PE_NOT_RESOLVE_PATH        0x00000010
#define LOAD_PE_TOP_TO_DOWN             0x00000020

	NTSTATUS
		LoadPeImage(
			PCWSTR      FullDllPath,
			PVOID*      DllBaseAddress,
			PVOID       OldBaseAddress = nullptr,
			ULONG_PTR   Flags = 0
		);

	NTSTATUS
		UnloadPeImage(
			PVOID DllBase
		);

	NTSTATUS
		RelocPeImage(
			PVOID   ImageBuffer,
			PVOID64 ImageDefaultBase,
			PVOID   ImageOldBase = nullptr,
			PVOID   ImageNewBase = nullptr
		);

	NTSTATUS
		RelocBlock(
			PVOID                   ImageBase,
			LONG64                  ImageBaseOffset,
			PIMAGE_BASE_RELOCATION2 Relocation
		);

	PVOID
		FASTCALL
		GetRoutineAddress(
			PVOID ModuleBase,
			PCSTR ProcedureName
		);

	inline
		PVOID
		FASTCALL
		GetRoutineAddress(
			PVOID ModuleBase,
			ULONG Ordinal
		)
	{
		return GetRoutineAddress(ModuleBase, (PCSTR)(ULONG_PTR)Ordinal);
	}

	PVOID
		LoadDll(
			PCWSTR ModuleFileName
		);

	NTSTATUS
		UnloadDll(
			PVOID DllHandle
		);

	ML_NAMESPACE_END_(Ldr);


	ML_NAMESPACE_BEGIN(Mm);


	NTSTATUS
		AllocVirtualMemory(
			PVOID*      BaseAddress,
			ULONG_PTR   Size,
			ULONG       Protect = PAGE_EXECUTE_READWRITE,
			ULONG       AllocationType = MEM_RESERVE | MEM_COMMIT,
			HANDLE      ProcessHandle = Ps::CurrentProcess
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
		ProtectVirtualMemory(
			PVOID       BaseAddress,
			ULONG_PTR   Size,
			ULONG       NewProtect,
			PULONG      OldProtect = nullptr,
			HANDLE      ProcessHandle = Ps::CurrentProcess
		);

	NTSTATUS
		ProtectMemory(
			HANDLE      ProcessHandle,
			PVOID       BaseAddress,
			ULONG_PTR   Size,
			ULONG       NewProtect,
			PULONG      OldProtect = nullptr
		);

	NTSTATUS
		FreeVirtualMemory(
			PVOID   BaseAddress,
			HANDLE  ProcessHandle = Ps::CurrentProcess,
			ULONG   FreeType = MEM_RELEASE
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

	NTSTATUS
		WriteProtectMemory(
			HANDLE      ProcessHandle,
			PVOID       BaseAddress,
			PVOID       Buffer,
			ULONG_PTR   Size,
			PULONG_PTR  BytesWritten = nullptr
		);

	NTSTATUS
		QueryMappedImageName(
			HANDLE  ProcessHandle,
			PVOID   ImageBase,
			String& ImageName
		);

	ML_NAMESPACE_END_(Mm);

	ML_NAMESPACE_BEGIN(Rtl);

	inline PWSTR QueryCommandLine()
	{
		return (PWSTR)Ps::CurrentPeb()->ProcessParameters->CommandLine.Buffer;
	}

	NTSTATUS
		DosPathNameToNtPathName(
			IN  PCWSTR                  DosName,
			OUT PUNICODE_STRING         NtName,
			OUT PCWSTR*                 DEF_VAL(DosFilePath, NULL) OPTIONAL,
			OUT PRTL_RELATIVE_NAME_U    DEF_VAL(FileName, NULL) OPTIONAL
		);

	NTSTATUS
		NtPathNameToDosPathName(
			IN  PUNICODE_STRING DosPath,
			OUT PUNICODE_STRING NtPath
		);

	NTSTATUS
		GetSystemDirectory(
			PUNICODE_STRING Buffer,
			BOOL            Wow64NoRedirect = FALSE
		);

	NTSTATUS
		GetModuleDirectory(
			ml::String& Path,
			PVOID       ModuleBase = nullptr
		);

	NTSTATUS
		GetWorkingDirectory(
			PUNICODE_STRING Buffer
		);

	NTSTATUS
		SetWorkingDirectory(
			PCWSTR PathName
		);

	NTSTATUS
		SetWorkingDirectory(
			PUNICODE_STRING PathName
		);

	NTSTATUS
		SetExeDirectoryAsCurrent(
			VOID
		);

	
	BOOL
		IsNameInExpression(
			IN PUNICODE_STRING  Expression,
			IN PUNICODE_STRING  Name,
			IN BOOL             DEF_VAL(IgnoreCase, TRUE),
			IN PWSTR            DEF_VAL(UpcaseTable, NULL) OPTIONAL
		);

	NTSTATUS
		EnvironmentAppend(
			PUNICODE_STRING Key,
			PUNICODE_STRING Value
		);

	inline
		BOOL
		IsNameInExpression(
			IN PCWSTR   Expression,
			IN PCWSTR   Name,
			IN BOOL     DEF_VAL(IgnoreCase, TRUE),
			IN PWSTR    DEF_VAL(UpcaseTable, NULL) OPTIONAL
		)
	{
		UNICODE_STRING ExpressionString, NameString;

		RtlInitUnicodeString(&ExpressionString, Expression);
		RtlInitUnicodeString(&NameString, Name);

		return IsNameInExpression(&ExpressionString, &NameString, IgnoreCase, UpcaseTable);
	}

	ML_NAMESPACE_END_(Rtl);

	ML_NAMESPACE_BEGIN(Nls);

	NTSTATUS
		FASTCALL
		AnsiToUnicode(
			PWSTR       UnicodeBuffer,
			ULONG_PTR   BufferCount,
			PCSTR       AnsiString,
			LONG_PTR    AnsiLength = -1,
			PULONG_PTR  BytesInUnicode = nullptr
		);

	NTSTATUS
		UnicodeToAnsi(
			PSTR        AnsiBuffer,
			ULONG_PTR   BufferCount,
			PCWSTR      UnicodeString,
			LONG_PTR    UnicodeLength = -1,
			PULONG_PTR  BytesInAnsi = nullptr
		);

	NTSTATUS
		AnsiToUnicodeString(
			PUNICODE_STRING Unicode,
			PCSTR           AnsiString,
			LONG_PTR        AnsiLength = -1,
			BOOL            AllocateDestinationString = TRUE
		);

	NTSTATUS
		UnicodeToAnsiString(
			PANSI_STRING    Ansi,
			PCWSTR          UnicodeString,
			LONG_PTR        UnicodeLength = -1,
			BOOL            AllocateDestinationString = TRUE
		);

	ML_NAMESPACE_END_(Nls);

	ML_NAMESPACE_BEGIN(Str);

	class UnicodeString : public UNICODE_STRING
	{
	protected:

	public:
		UnicodeString();
		~UnicodeString();

		NTSTATUS Create(PCWSTR Buffer, ULONG_PTR Length = -1);
		NTSTATUS Create(PCUNICODE_STRING Buffer);

		BOOL operator==(PCWSTR Buffer) const;
		BOOL operator==(PCUNICODE_STRING String) const;
		BOOL operator==(const UnicodeString &String) const;

		UnicodeString& operator=(PCWSTR Buffer);
		UnicodeString& operator=(PCUNICODE_STRING Buffer);
	};

	ML_NAMESPACE_END_(Str);

	ML_NAMESPACE_BEGIN(Reg);

#if !defined(HKEY_CURRENT_USER_LOCAL_SETTINGS)
#define HKEY_CURRENT_USER_LOCAL_SETTINGS    (( HKEY ) (ULONG_PTR)((LONG)0x80000007) )
#endif

#if ML_KERNEL_MODE

	typedef HANDLE HKEY;

#define HKEY_CLASSES_ROOT                   (( HKEY ) (ULONG_PTR)((LONG)0x80000000) )
#define HKEY_CURRENT_USER                   (( HKEY ) (ULONG_PTR)((LONG)0x80000001) )
#define HKEY_LOCAL_MACHINE                  (( HKEY ) (ULONG_PTR)((LONG)0x80000002) )
#define HKEY_USERS                          (( HKEY ) (ULONG_PTR)((LONG)0x80000003) )
#define HKEY_PERFORMANCE_DATA               (( HKEY ) (ULONG_PTR)((LONG)0x80000004) )
#define HKEY_PERFORMANCE_TEXT               (( HKEY ) (ULONG_PTR)((LONG)0x80000050) )
#define HKEY_PERFORMANCE_NLSTEXT            (( HKEY ) (ULONG_PTR)((LONG)0x80000060) )
#define HKEY_CURRENT_CONFIG                 (( HKEY ) (ULONG_PTR)((LONG)0x80000005) )
#define HKEY_DYN_DATA                       (( HKEY ) (ULONG_PTR)((LONG)0x80000006) )
#define HKEY_CURRENT_USER_LOCAL_SETTINGS    (( HKEY ) (ULONG_PTR)((LONG)0x80000007) )

#endif // r0

#define HKEY_MACHINE_CLASS                  (( HKEY ) (ULONG_PTR)((LONG)0x80000020) )

	PVOID
		AllocateKeyInfo(
			ULONG_PTR Size
		);

	VOID
		FreeKeyInfo(
			PVOID Info
		);

	NTSTATUS
		OpenPredefinedKeyHandle(
			PHANDLE     KeyHandle,
			HANDLE      PredefinedKey,
			ACCESS_MASK DEF_VAL(DesiredAccess, KEY_ALL_ACCESS)
		);

	NTSTATUS
		OpenKey(
			PHANDLE     KeyHandle,
			HANDLE      hKey,
			ACCESS_MASK DesiredAccess,
			PCWSTR      SubKey
		);

	NTSTATUS
		CloseKeyHandle(
			HANDLE KeyHandle
		);

	NTSTATUS
		GetKeyValue(
			HANDLE                      hKey,
			PCWSTR                      SubKey,
			PCWSTR                      ValueName,
			KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
			PVOID                       KeyValueInformation,
			ULONG                       Length,
			PULONG                      DEF_VAL(ResultLength, NULL),
			ULONG                       DEF_VAL(Flags, 0)
		);

	NTSTATUS
		GetKeyValue(
			HANDLE                          hKey,
			PCWSTR                          SubKey,
			PCWSTR                          ValueName,
			PKEY_VALUE_PARTIAL_INFORMATION* Value
		);

	NTSTATUS
		SetKeyValue(
			HANDLE      hKey,
			PCWSTR      SubKey,
			PCWSTR      ValueName,
			ULONG       ValueType,
			LPCVOID     ValueData,
			DWORD       ValueDataLength,
			ULONG_PTR   Flags = 0
		);

	NTSTATUS
		DeleteKey(
			HANDLE      hKey,
			PCWSTR      SubKey,
			ULONG_PTR   Flags = 0
		);

	NTSTATUS
		DeleteKeyValue(
			HANDLE      hKey,
			PCWSTR      SubKey,
			PCWSTR      ValueName,
			ULONG_PTR   Flags = 0
		);

	ML_NAMESPACE_END_(Reg);

	ML_NAMESPACE_BEGIN(Exp);

#if ML_KERNEL_MODE

	/*
	* MessageBox() Flags
	*/
#define MB_OK                       0x00000000L
#define MB_OKCANCEL                 0x00000001L
#define MB_ABORTRETRYIGNORE         0x00000002L
#define MB_YESNOCANCEL              0x00000003L
#define MB_YESNO                    0x00000004L
#define MB_RETRYCANCEL              0x00000005L
#if(WINVER >= 0x0500)
#define MB_CANCELTRYCONTINUE        0x00000006L
#endif /* WINVER >= 0x0500 */


#define MB_ICONHAND                 0x00000010L
#define MB_ICONQUESTION             0x00000020L
#define MB_ICONEXCLAMATION          0x00000030L
#define MB_ICONASTERISK             0x00000040L

#if(WINVER >= 0x0400)
#define MB_USERICON                 0x00000080L
#define MB_ICONWARNING              MB_ICONEXCLAMATION
#define MB_ICONERROR                MB_ICONHAND
#endif /* WINVER >= 0x0400 */

#define MB_ICONINFORMATION          MB_ICONASTERISK
#define MB_ICONSTOP                 MB_ICONHAND

#define MB_DEFBUTTON1               0x00000000L
#define MB_DEFBUTTON2               0x00000100L
#define MB_DEFBUTTON3               0x00000200L
#if(WINVER >= 0x0400)
#define MB_DEFBUTTON4               0x00000300L
#endif /* WINVER >= 0x0400 */

#define MB_APPLMODAL                0x00000000L
#define MB_SYSTEMMODAL              0x00001000L
#define MB_TASKMODAL                0x00002000L
#if(WINVER >= 0x0400)
#define MB_HELP                     0x00004000L // Help Button
#endif /* WINVER >= 0x0400 */

#define MB_NOFOCUS                  0x00008000L
#define MB_SETFOREGROUND            0x00010000L
#define MB_DEFAULT_DESKTOP_ONLY     0x00020000L

#if(WINVER >= 0x0400)
#define MB_TOPMOST                  0x00040000L
#define MB_RIGHT                    0x00080000L
#define MB_RTLREADING               0x00100000L

#endif /* WINVER >= 0x0400 */

#ifdef _WIN32_WINNT
#if (_WIN32_WINNT >= 0x0400)
#define MB_SERVICE_NOTIFICATION          0x00200000L
#else
#define MB_SERVICE_NOTIFICATION          0x00040000L
#endif
#define MB_SERVICE_NOTIFICATION_NT3X     0x00040000L
#endif

#define MB_TYPEMASK                 0x0000000FL
#define MB_ICONMASK                 0x000000F0L
#define MB_DEFMASK                  0x00000F00L
#define MB_MODEMASK                 0x00003000L
#define MB_MISCMASK                 0x0000C000L

#endif // r0

	NTSTATUS
		ExceptionBox(
			PCWSTR      Text,
			PCWSTR      Title = nullptr,
			PULONG_PTR  Response = nullptr,
			ULONG_PTR   Type = MB_ICONASTERISK | MB_OK
		);

	ML_NAMESPACE_END_(Exp);

	ML_NAMESPACE_BEGIN(Lpc);

	typedef NTSTATUS IPC_STATUS;

	enum
	{
		IPCType_None,
		IPCType_ServerListen,
		IPCType_ServerConnected,
		IPCType_Client,
	};

	class InterProcessBase
	{
	protected:
		ULONG_PTR IpcType;

	public:
		InterProcessBase()
		{
			IpcType = IPCType_None;
		}

		ULONG_PTR GetIpcType()
		{
			return IpcType;
		}

		IPC_STATUS ReleaseAll()
		{
			return STATUS_NOT_IMPLEMENTED;
		}
	};

	class InterProcessServerBase : public InterProcessBase
	{
	public:
		InterProcessServerBase()
		{
		}
	};

	class InterProcessClientBase : public InterProcessBase
	{
	public:
		InterProcessClientBase()
		{
			IpcType = IPCType_Client;
		}
	};

	enum IPC_MESSAGE_TYPE
	{
		IpcMessageType_ConnectionRequest,
		IpcMessageType_SmallData,
	};

	typedef struct IPC_MESSAGE : public PORT_MESSAGE
	{
		PVOID GetMessageData()
		{
			return PtrAdd(this, sizeof(PORT_MESSAGE));
		}

		ULONG   MessageType;
		ULONG64 MessageDataLength;

		static const ULONG_PTR kOverhead = sizeof(ULONG) + sizeof(ULONG64);
		static const ULONG_PTR kMaxSmallDataLength = PORT_CREATE_MAXIMUM_MESSAGE_LENGTH - sizeof(PORT_MESSAGE) - kOverhead;

		union
		{
			struct
			{
				ULONG64 MaxMessageLength;

			} ConnectionRequest;

			struct
			{
				BYTE Buffer[kMaxSmallDataLength];

			} SmallData;

		} MessageData;

	} IPC_MESSAGE, *PIPC_MESSAGE;


	/************************************************************************
	InterProcessLpcServer
	************************************************************************/

	class InterProcessLpcServer : public InterProcessServerBase
	{
	protected:
		union
		{
			LPC_HANDLE          PortHandle;
			LPC_HANDLE          LpcPort;
			LPC_HANDLE          ConnectedPort;
		};

		union
		{
			struct
			{
				ULONG_PTR MaxConnectionInfoLength;
				ULONG_PTR MaxMessageLength;
				ULONG_PTR MaxSmallDataLength;

			} Listening;

			struct
			{
				PORT_VIEW           ServerView;
				REMOTE_PORT_VIEW    ClientView;
				PVOID               Context;

			} Connected;
		};

	public:
		InterProcessLpcServer();
		~InterProcessLpcServer();

		VOID Reset();
		IPC_STATUS ReleaseAll();

		operator LPC_HANDLE()
		{
			return PortHandle;
		}

		/************************************************************************
		listen
		************************************************************************/

		IPC_STATUS
			Create(
				IN PWSTR Name,
				IN ULONG MaxConnectionInfoLength = sizeof(IPC_MESSAGE),
				IN ULONG MaxMessageLength = PORT_CREATE_MAXIMUM_MESSAGE_LENGTH
			);

		IPC_STATUS
			Listen(
				IN PIPC_MESSAGE ConnectionRequest,
				IN ULONG        Timeout = INFINITE
			);

		IPC_STATUS
			Listen(
				IN PIPC_MESSAGE     ConnectionRequest,
				IN PLARGE_INTEGER   Timeout = NULL
			);

		IPC_STATUS
			Accept(
				OUT InterProcessLpcServer&  ConnectedPort,
				IN  PIPC_MESSAGE            ConnectionRequest,
				IN  BOOL                    AcceptConnection = TRUE,
				IN  PVOID                   Context = NULL OPTIONAL
			);

		/************************************************************************
		connected
		************************************************************************/

		PVOID GetDataBuffer(PIPC_MESSAGE Message)
		{
			return Message->MessageDataLength > Message->kMaxSmallDataLength ?
				this->Connected.ClientView.ViewBase :
				Message->MessageData.SmallData.Buffer;
		}

		IPC_STATUS
			Receive(
				OUT PVOID           Buffer,
				IN  ULONG_PTR       Size,
				OUT PULONG_PTR      BytesTransfered = NULL,
				OUT PVOID*          Context = NULL OPTIONAL,
				IN  ULONG           Timeout = INFINITE OPTIONAL
			);

		IPC_STATUS
			Receive(
				OUT PVOID           Buffer,
				IN  ULONG_PTR       Size,
				OUT PULONG_PTR      BytesTransfered = NULL,
				OUT PVOID*          Context = NULL OPTIONAL,
				IN  PLARGE_INTEGER  Timeout = NULL OPTIONAL
			);
	};


	/************************************************************************
	InterProcessLpcClient
	************************************************************************/

	class InterProcessLpcClient : public InterProcessClientBase
	{
	protected:
		LPC_HANDLE          ConnetionPort;
		PORT_VIEW           ClientView;
		REMOTE_PORT_VIEW    ServerView;
		PVOID               Context;
		ULONG_PTR           MaxSmallDataLength;

	public:
		InterProcessLpcClient();
		~InterProcessLpcClient();

		VOID Reset();
		IPC_STATUS ReleaseAll();

		LPC_STATUS
			Connect(
				IN PWSTR Name,
				IN ULONG MaxMessageLength = 0
			);

		LPC_STATUS
			Send(
				PVOID       Buffer,
				ULONG_PTR   Size,
				PULONG_PTR  BytesTransfered = NULL
			);
	};

	ML_NAMESPACE_END_(Lpc);

	ML_NAMESPACE_BEGIN(Io);

#if !defined(FILE_ATTRIBUTE_NO_SCRUB_DATA)
#define FILE_ATTRIBUTE_NO_SCRUB_DATA        0x00020000
#endif

#if ML_KERNEL_MODE

	//
	// Typedefs
	//
	
	typedef struct _SECURITY_ATTRIBUTES {
		DWORD nLength;
		LPVOID lpSecurityDescriptor;
		BOOL bInheritHandle;
	} SECURITY_ATTRIBUTES, *PSECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;


#endif // r0

	typedef struct _ML_FIND_DATA {
		ULONG           FileAttributes;
		LARGE_INTEGER   CreationTime;
		LARGE_INTEGER   LastAccessTime;
		LARGE_INTEGER   LastWriteTime;
		LARGE_INTEGER   FileSize;
		ULONG           Reserved0;
		ULONG           Reserved1;
		WCHAR           FileName[MAX_NTPATH];
		WCHAR           AlternateFileName[14];

	} ML_FIND_DATA, *PML_FIND_DATA;

	NTSTATUS
		QueryFirstFile(
			PHANDLE         Handle,
			PCWSTR          FileName,
			PML_FIND_DATA   FindFileData
		);

	NTSTATUS
		QueryNextFile(
			HANDLE          FindFileHandle,
			PML_FIND_DATA   FindFileData
		);

	inline
		NTSTATUS
		QueryClose(
			HANDLE FindFileHandle
		)
	{
#if ML_KERNEL_MODE
		return ZwClose(FindFileHandle);
#elif ML_USER_MODE
		return NtClose(FindFileHandle);
#endif
	}

	ULONG_PTR
		QueryFileAttributes(
			PCWSTR FileName
		);

	NTSTATUS
		QueryFileAttributesEx(
			PCWSTR      FileName,
			PULONG_PTR  FileAttributes
		);

	NTSTATUS
		ApplyFileAttributes(
			PCWSTR      FileName,
			ULONG_PTR   Attributes
		);

	NTSTATUS
		MoveFile(
			PCWSTR  ExistingFileName,
			PCWSTR  NewFileName,
			BOOL    DEF_VAL(ReplaceIfExists, FALSE)
		);

	NTSTATUS
		CopyFile(
			PCWSTR  ExistingFileName,
			PCWSTR  NewFileName,
			BOOL    FailIfExists
		);

	NTSTATUS
		DeleteFile(
			PCWSTR FileName
		);

	NTSTATUS
		DeleteDirectory(
			PCWSTR DirectoryName
		);

	BOOL
		IsPathExists(
			PCWSTR Path
		);

	NTSTATUS
		QueryDosLetterFromVolumeDeviceName(
			OUT PUNICODE_STRING DosLetter,
			IN  PUNICODE_STRING NtDeviceName
		);

	NTSTATUS
		QueryDosPathFromHandle(
			OUT PUNICODE_STRING DosPath,
			IN  HANDLE          FileHandle
		);

	NTSTATUS
		QueryDosDevice(
			PCWSTR          DeviceName,
			PUNICODE_STRING TargetPath
		);

	NTSTATUS
		QueryDosPathFromNtDeviceName(
			OUT PUNICODE_STRING DosPath,
			IN  PUNICODE_STRING NtDeviceName
		);

	NTSTATUS
		CreateNamedPipe(
			OUT PHANDLE                 ReadPipe,
			OUT PHANDLE                 WritePipe,
			IN  PUNICODE_STRING         DEF_VAL(PipeName, nullptr) OPTIONAL,
			IN  PSECURITY_ATTRIBUTES    DEF_VAL(PipeAttributes, nullptr) OPTIONAL,
			IN  ULONG_PTR               DEF_VAL(BufferSize, 0) OPTIONAL
		);

	template<typename F, typename P>
	NTSTATUS
		SetAsyncCall(
			CONST F&    Callback,
			ULONG64     DueTime,
			LONG        Period = 0,
			CONST P&    Context = nullptr
		)
	{
		NTSTATUS        Status;
		HANDLE          Timer;
		LARGE_INTEGER   _DueTime;

		typedef struct PARAMETER
		{
			ml::Function<BOOL(P)> func;
			P Context;
			LONG Period;
			HANDLE Timer;

			PARAMETER(const F &Callback, const P &Context, const LONG &Period) : func(Callback), Context(Context)
			{
				this->Period = Period;
				this->Timer = nullptr;
			}

			~PARAMETER()
			{
				if (this->Timer != nullptr)
				{
					NtCancelTimer(this->Timer, nullptr);
					NtClose(this->Timer);
				}
			}

		} PARAMETER;

		PARAMETER *func = new PARAMETER(Callback, Context, Period);
		if (func == nullptr)
			return STATUS_NO_MEMORY;

		Status = NtCreateTimer(&Timer, TIMER_ALL_ACCESS, nullptr, SynchronizationTimer);
		if (NT_FAILED(Status))
		{
			delete func;
			return Status;
		}

		func->Timer = Timer;
		FormatTimeOut(&_DueTime, (ULONG)DueTime);

		Status = NtSetTimer(Timer, &_DueTime,
			[](PVOID p, ULONG, LONG)
		{
			PARAMETER *param = (PARAMETER *)p;
			BOOL cancel = param->func(param->Context);
			if (param->Period == 0 || cancel)
			{
				delete param;
			}
		},
			func, FALSE, Period, nullptr
			);

		if (NT_FAILED(Status))
		{
			delete func;
		}

		return Status;
	}

	template<typename F>
	NTSTATUS
		SetAsyncCall(
			CONST F&    Callback,
			ULONG64     DueTime,
			LONG        Period = 0
		)
	{
		NTSTATUS        Status;
		HANDLE          Timer;
		LARGE_INTEGER   _DueTime;

		typedef struct PARAMETER
		{
			ml::Function<BOOL()> func;
			LONG Period;
			HANDLE Timer;

			PARAMETER(const F &Callback, const LONG &Period) : func(Callback)
			{
				this->Period = Period;
				this->Timer = nullptr;
			}

			~PARAMETER()
			{
				if (this->Timer != nullptr)
				{
					NtCancelTimer(this->Timer, nullptr);
					NtClose(this->Timer);
				}
			}

		} PARAMETER;

		PARAMETER *func = new PARAMETER(Callback, Period);
		if (func == nullptr)
			return STATUS_NO_MEMORY;

		Status = NtCreateTimer(&Timer, TIMER_ALL_ACCESS, nullptr, SynchronizationTimer);
		if (NT_FAILED(Status))
		{
			delete func;
			return Status;
		}

		func->Timer = Timer;
		FormatTimeOut(&_DueTime, (ULONG)DueTime);

		Status = NtSetTimer(Timer, &_DueTime,
			[](PVOID p, ULONG, LONG)
		{
			PARAMETER *param = (PARAMETER *)p;
			BOOL cancel = param->func();
			if (param->Period == 0 || cancel)
			{
				delete param;
			}
		},
			func, FALSE, Period, nullptr
			);

		if (NT_FAILED(Status))
		{
			delete func;
		}

		return Status;
	}

	ML_NAMESPACE_END_(Io);


	ML_NAMESPACE_BEGIN(Ob);

#if ML_KERNEL_MODE

	NTSTATUS
		CreateObjectType(
			IN  PUNICODE_STRING             TypeName,
			IN  POBJECT_TYPE_INITIALIZER    ObjectTypeInitializer,
			OUT POBJECT_TYPE*               ObjectType
		);

#endif // r0

	ML_NAMESPACE_END_(Ob);


#if ML_USER_MODE

	ML_NAMESPACE_BEGIN(Gdi);

	typedef struct
	{
		USHORT MajorVersion;
		USHORT MinorVersion;
		USHORT NumOfTables;
		USHORT SearchRange;
		USHORT EntrySelector;
		USHORT RangeShift;

	} TT_OFFSET_TABLE;

	// Tables in TTF file and there placement and name (tag)

	typedef struct
	{
		ULONG Tag;      // table name
		ULONG CheckSum; // Check sum
		ULONG Offset;   // Offset from beginning of file
		ULONG Length;   // length of the table in bytes

	} TT_TABLE_DIRECTORY;

	//Header of names table

	typedef struct
	{
		USHORT FormatSelector;    // format selector. Always 0
		USHORT NameRecordCount;   // Name Records count
		USHORT StorageOffset;     // Offset for strings storage, from start of the table

	} TT_NAME_TABLE_HEADER, *PTT_NAME_TABLE_HEADER;

	// Record in names table

	enum
	{
		TT_PLATFORM_ID_MAC_OS = 0x0100,
		TT_PLATFORM_ID_WINDOWS = 0x0300,

		TT_TABLE_TAG_NAME = TAG4('name'),
		TT_TABLE_TAG_CMAP = TAG4('cmap'),

		TT_ENCODEING_ID_ANSI = 0x0000,
		TT_ENCODEING_ID_UTF16_BE = 0x0100,

		TT_NAME_ID_FACENAME = 0x0100,
		TT_NAME_ID_TYPE = 0x0200,
		TT_NAME_ID_FOUNDER = 0x0300,
		TT_NAME_ID_FULLNAME = 0x0400,
		TT_NAME_ID_VERSION = 0x0500,
	};

	typedef struct
	{
		USHORT PlatformID;
		USHORT EncodingID;
		USHORT LanguageID;
		USHORT NameID;
		USHORT StringLength;
		USHORT StringOffset; // from start of storage area

	} TT_NAME_RECORD, *PTT_NAME_RECORD;

	typedef struct
	{
		USHORT  Version;        // Table version number (0).
		USHORT	NumberOfTables; // Number of encoding tables, n.

	} TT_CMAP_TABLE_HEADER, *PTT_CMAP_TABLE_HEADER;

	typedef struct
	{
		USHORT	PlatformID;                 // Platform ID.
		USHORT	EncodingID;                 // Platform-specific encoding ID.
		ULONG	DataOffset;                 // Byte offset from beginning of table to the subtable for this encoding.

	} TT_CMAP_RECORD, *PTT_CMAP_RECORD;

#pragma pack()

	ML_NAMESPACE_END_(Gdi);

#endif // r3


	ML_NAMESPACE_END_(Native);

	ML_NAMESPACE_END;



#define NOP  0x90
#define CALL 0xE8
#define JUMP 0xE9
#define PUSH 0x68
#define REXW 0x49

#pragma push_macro("MP_INLINE")

#define MP_INLINE ForceInline
#define MP_CALL FASTCALL


	ML_NAMESPACE_BEGIN(Mp);

#if ML_X86
#define TRAMPOLINE_SIZE     0x40
#elif ML_AMD64
#define TRAMPOLINE_SIZE     0x100
#else
#define TRAMPOLINE_SIZE     0x100
#endif

	ML_NAMESPACE_BEGIN(PatchMemoryTypes);

	enum
	{
		MemoryPatch,
		FunctionPatch,
	};

	ML_NAMESPACE_END_(PatchMemoryTypes);

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

	typedef struct _TRAMPOLINE_NAKED_CONTEXT
	{

#if ML_X86

		ULONG_PTR EFlags;
		ULONG_PTR Rdi;
		ULONG_PTR Rsi;
		ULONG_PTR Rbp;
		ULONG_PTR Rbx;
		ULONG_PTR Rdx;
		ULONG_PTR Rcx;
		ULONG_PTR Rax;
		ULONG_PTR Rsp;

#elif ML_AMD64

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

#if ML_X86

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

	ML_NAMESPACE_END_(Mp);

	/************************************************************************
	helper function
	************************************************************************/

	template<class PtrType>
	MP_INLINE
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

	NTSTATUS
		CopyOneOpCode(
			PVOID       Target,
			PVOID       Source,
			PULONG_PTR  DestinationOpLength,
			PULONG_PTR  SourceOpLength,
			ULONG_PTR   ForwardSize,
			ULONG_PTR   BackwardSize,
			PVOID       TargetIp = IMAGE_INVALID_VA
		);

	MP_INLINE ULONG_PTR GetOpCodeSize32(PVOID Buffer)
	{
		return LdeGetOpCodeSize32(Buffer);
	}

	MP_INLINE ULONG_PTR GetOpCodeSize64(PVOID Buffer)
	{
		return LdeGetOpCodeSize64(Buffer);
	}

	MP_INLINE ULONG_PTR GetOpCodeSize(PVOID Buffer)
	{
#if ML_AMD64
		return GetOpCodeSize64(Buffer);
#elif ML_X86
		return GetOpCodeSize32(Buffer);
#endif
	}

	//#pragma pop_macro("MP_CALL")
#pragma pop_macro("MP_INLINE")


	inline PVoid Sunday(PCVoid Buffer, LongPtr BufferSize, PCVoid Pattern, LongPtr PatternSize)
	{
		LongPtr occ[256];
		LongPtr Index, SearchLength;

		if (PatternSize > BufferSize)
			return nullptr;

		FillMemory(occ, sizeof(occ), -1);
		for (Index = 0; Index != PatternSize; ++Index)
		{
			occ[((PByte)Pattern)[Index]] = Index;
		}

		for (LongPtr Begin = 0; Begin != BufferSize - PatternSize;)
		{
			LongPtr i = Begin, j = 0;

			for (; j < PatternSize && i < BufferSize && ((PByte)Buffer)[i] == ((PByte)Pattern)[j]; ++i, ++j);

			if (j == PatternSize)
				return &((PByte)Buffer)[Begin];

			if (Begin + PatternSize >= BufferSize)
				return nullptr;

			Begin += PatternSize - occ[((PByte)Buffer)[Begin + PatternSize]];
		}

		return nullptr;
	}



#if !defined(COMPACT_TRIE_MAGIC)
#define COMPACT_TRIE_MAGIC  TAG4('TRIE')
#endif // COMPACT_TRIE_MAGIC

#define TRIE_DEBUG 1


	using ml::GrowableArray;


	typedef ULONG NODE_CONTEXT, *PNODE_CONTEXT;

	typedef ULONG  UNSIGNED_OFFSET, *PUNSIGNED_OFFSET;
	//typedef LONG   SIGNED_OFFSET, *PSIGNED_OFFSET;

	ML_NAMESPACE_BEGIN(TrieBitsIndex)

		enum
	{
		Flags = 32,
		NodeCount = 33,

		Max,
	};

	ML_NAMESPACE_END_(TrieBitsIndex);

	ML_NAMESPACE_BEGIN(TrieBitsFlags)

		enum
	{
		HasContext = 0x01,
		HasNodes = 0x02,
	};

	ML_NAMESPACE_END_(TrieBitsFlags);

	static const ULONG_PTR MAXIMUM_NEXT_NODE = 256;

	typedef struct TRIE_NODE
	{
		typedef TRIE_NODE *PTRIE_NODE;

		static const ULONG_PTR kInvalidIndex = ULONG_PTR_MAX;

		union
		{
			PVOID           Pointer;
			NODE_CONTEXT    Value;

		} Context;

		PTRIE_NODE Failure;

		BYTE SlotsFlags[TrieBitsIndex::Flags + 1];
		BYTE SlotIndex[MAXIMUM_NEXT_NODE];

		GrowableArray<PTRIE_NODE> Next2;

		TRIE_NODE()
		{
			ZeroMemory(this, sizeof(*this));
			Context.Pointer = (PVOID)-1;
		}

		VOID SetFlags(ULONG_PTR Flags)
		{
			SET_FLAG(SlotsFlags[TrieBitsIndex::Flags], Flags);
		}

		BOOL TestFlags(ULONG_PTR Flags)
		{
			return FLAG_ON(SlotsFlags[TrieBitsIndex::Flags], Flags);
		}

		PTRIE_NODE GetNext(ULONG_PTR Index)
		{
			if (FLAG_OFF(SlotsFlags[Index / 8], 1 << (Index % 8)))
				return nullptr;

			return Next2[SlotIndex[Index]];
		}

		PTRIE_NODE* AllocateSlot(ULONG_PTR Index)
		{
			NTSTATUS Status;

			if (FLAG_OFF(SlotsFlags[Index / 8], 1 << (Index % 8)))
			{
				PTRIE_NODE empty = nullptr;
				Status = Next2.Add(empty);
				if (NT_FAILED(Status))
					return nullptr;

				SET_FLAG(SlotsFlags[Index / 8], 1 << (Index % 8));
				SlotIndex[Index] = (BYTE)(Next2.GetSize() - 1);
			}

			return &Next2[SlotIndex[Index]];
		}

	} TRIE_NODE, *PTRIE_NODE;

	typedef struct
	{
		PVOID           Data;
		ULONG_PTR       SizeInBytes;
		NODE_CONTEXT    Context;

	} TRIE_BYTES_ENTRY, *PTRIE_BYTES_ENTRY;

#define ADD_TRIE_STRING(_str, ...) { _str, CONST_STRLEN(_str) * sizeof(_str[0]), __VA_ARGS__ }

	typedef struct
	{
		ULONG Magic;
		ULONG IndexBitsTableOffset;
		ULONG IndexTableOffset;
		ULONG OffsetTableOffset;
		ULONG NodesDataOffset;
		ULONG Reserve[3];

	} COMPACT_TRIE_HEADER, *PCOMPACT_TRIE_HEADER;

	typedef struct TRIE_NODE_OFFSET_TABLE_ENTRY
	{
		UNSIGNED_OFFSET   IndexBitsOffset;
		UNSIGNED_OFFSET   IndexOffset;

		TRIE_NODE_OFFSET_TABLE_ENTRY(ULONG_PTR IndexBitsOffset = 0, ULONG_PTR IndexOffset = 0)
		{
			this->IndexBitsOffset = (UNSIGNED_OFFSET)IndexBitsOffset;
			this->IndexOffset = (UNSIGNED_OFFSET)IndexOffset;
		}

		BOOL operator==(const TRIE_NODE_OFFSET_TABLE_ENTRY &entry)
		{
			return RtlCompareMemory(this, &entry, sizeof(entry)) == 0;
		}

	} TRIE_NODE_OFFSET_TABLE_ENTRY, *PTRIE_NODE_OFFSET_TABLE_ENTRY;


	typedef struct COMPACT_TRIE_NODE
	{
		USHORT  OffsetTableIndex;

		//ULONG   Context;
		// ULONG RVOffsetOfNodes[];

	} COMPACT_TRIE_NODE, *PCOMPACT_TRIE_NODE;


	class CompactTrie
	{
	protected:
		PCOMPACT_TRIE_HEADER            Header;
		PBYTE                           IndexBitsTable;
		PBYTE                           IndexTable;
		PTRIE_NODE_OFFSET_TABLE_ENTRY   OffsetTable;
		PCOMPACT_TRIE_NODE              Root;

	public:

		CompactTrie()
		{
			ZeroMemory(this, sizeof(*this));
		}

		~CompactTrie()
		{
			FreeMemoryP(Header);
		}

		NTSTATUS Initialize(PVOID CompactTree, ULONG_PTR SizeOfTree)
		{
			if (((PCOMPACT_TRIE_HEADER)CompactTree)->Magic * 2 != COMPACT_TRIE_MAGIC * 2)
				return STATUS_CONTEXT_MISMATCH;

			Header = (PCOMPACT_TRIE_HEADER)AllocateMemoryP(SizeOfTree);
			if (Header == nullptr)
				return STATUS_NO_MEMORY;

			CopyMemory(Header, CompactTree, SizeOfTree);

			IndexBitsTable = (PBYTE)PtrAdd(Header, Header->IndexBitsTableOffset);
			IndexTable = (PBYTE)PtrAdd(Header, Header->IndexTableOffset);
			OffsetTable = (PTRIE_NODE_OFFSET_TABLE_ENTRY)PtrAdd(Header, Header->OffsetTableOffset);
			Root = (PCOMPACT_TRIE_NODE)PtrAdd(Header, Header->NodesDataOffset);

			return STATUS_SUCCESS;
		}

		NoInline NTSTATUS Lookup(PTRIE_BYTES_ENTRY Entry, PNODE_CONTEXT Context = nullptr)
		{
			ULONG_PTR           Index;
			PBYTE               Buffer, IndexBitsTable, IndexTable;
			PUNSIGNED_OFFSET    NextNodeOffset;
			PCOMPACT_TRIE_NODE  Node;

			if (this->Root == nullptr)
				return STATUS_NO_MORE_ENTRIES;

			if (Entry->SizeInBytes == 0)
				return STATUS_NOT_FOUND;

			Node = this->Root;

			IndexBitsTable = PtrAdd(this->IndexBitsTable, (LONG_PTR)this->OffsetTable[Node->OffsetTableIndex].IndexBitsOffset);
			IndexTable = PtrAdd(this->IndexTable, (LONG_PTR)this->OffsetTable[Node->OffsetTableIndex].IndexOffset);

			FOR_EACH(Buffer, (PBYTE)Entry->Data, Entry->SizeInBytes)
			{
				Index = Buffer[0];

				if (FLAG_OFF(IndexBitsTable[Index / 8], 1 << (Index % 8)))
					return STATUS_NOT_FOUND;

				NextNodeOffset = (PUNSIGNED_OFFSET)(Node + 1);
				Node = PtrAdd(Node, NextNodeOffset[IndexTable[Index]]);

				IndexBitsTable = PtrAdd(this->IndexBitsTable, (LONG_PTR)this->OffsetTable[Node->OffsetTableIndex].IndexBitsOffset);
				IndexTable = PtrAdd(this->IndexTable, (LONG_PTR)this->OffsetTable[Node->OffsetTableIndex].IndexOffset);
			}

			if (FLAG_OFF(IndexBitsTable[TrieBitsIndex::Flags], TrieBitsFlags::HasContext))
				return STATUS_CONTEXT_MISMATCH;

			if (Context != nullptr)
			{
				ULONG_PTR NodeCount;
				PNODE_CONTEXT NodeContext;

				NodeContext = (PNODE_CONTEXT)Node;

				NodeCount = (IndexBitsTable[TrieBitsIndex::NodeCount] + 1) & -FLAG_ON(IndexBitsTable[TrieBitsIndex::Flags], TrieBitsFlags::HasNodes);
				NodeContext = PtrAdd(NodeContext, NodeCount * sizeof(*NextNodeOffset) + sizeof(*Node));

				*Context = *NodeContext;
			}

			return STATUS_SUCCESS;
		}
	};


	template<typename NodeType = TRIE_NODE>
	class StaticTrieT
	{
	public:
		typedef NodeType *PNodeType;
		typedef GrowableArray<NodeType> NodeArray;
		typedef GrowableArray<PNodeType> NodePArray;

	protected:
		typedef struct NODE_OFFSET_MAP
		{
			PNodeType Node;
			ULONG     Offset;

			NODE_OFFSET_MAP(PNodeType Node = nullptr, ULONG Offset = 0)
			{
				this->Node = Node;
				this->Offset = Offset;
			}

		} NODE_OFFSET_MAP, *PNODE_OFFSET_MAP;

		typedef struct NODE_FIX_ENTRY
		{
			PCOMPACT_TRIE_NODE  Compact;
			PNodeType           Node;
			PUNSIGNED_OFFSET    Offset;

			NODE_FIX_ENTRY()
			{
			}

			NODE_FIX_ENTRY(PCOMPACT_TRIE_NODE Compact, PNodeType Node, PVOID Offset)
			{
				this->Compact = Compact;
				this->Node = Node;
				this->Offset = (PUNSIGNED_OFFSET)Offset;
			}

		} NODE_FIX_ENTRY, *PNODE_FIX_ENTRY;

	protected:

		PNodeType Root;

	public:
		StaticTrieT()
		{
			this->Root = nullptr;
		}

		NoInline PVOID FindReuseData(PVOID Begin, PVOID End, PVOID Data, ULONG_PTR Size)
		{
			PVOID     Found;
			ULONG_PTR SearchLength;

			if (Begin >= End)
				return Begin;

			SearchLength = PtrOffset(End, Begin);

			Found = Sunday(Begin, SearchLength, Data, Size);
			if (Found == nullptr && SearchLength != 0)
			{
				for (ULONG_PTR Count = Size - 1; Count != 0; --Count)
				{
					if (Count > SearchLength)
						continue;

					if (RtlCompareMemory(PtrSub(End, Count), Data, Count) != Count)
						continue;

					Found = PtrSub(End, Count);
					break;
				}
			}

			return Found == nullptr ? End : Found;
		}

		NoInline PNODE_OFFSET_MAP LookupNodeOffsetEntry(GrowableArray<NODE_OFFSET_MAP> &NodeOffsetTable, PNodeType Node)
		{
			PNODE_OFFSET_MAP Entry;

			FOR_EACH_VEC(Entry, NodeOffsetTable)
			{
				if (Entry->Node == Node)
					return Entry;
			}

			return nullptr;
		}

		NTSTATUS BuildCompactTree(PVOID *CompactTree, PULONG_PTR CompactSize)
		{
			NTSTATUS                Status;
			PBYTE                   IndexBitsBase, IndexBits;
			PBYTE                   IndexBufferBase, IndexBuffer;
			PBYTE                   NodesBufferBase, NodesBuffer;
			ULONG_PTR               FinalSize, NodeCount, IndexBufferSize, IndexBitsSize, NodesBufferSize, OffsetTableSize;
			PCOMPACT_TRIE_HEADER    CompactHeader;
			PNODE_FIX_ENTRY         FixEntry;

			NodeCount = 0;
			Status = EnumNodes(
				[&](PNodeType Parent, PNodeType Node, ULONG_PTR IndexOfNext)
			{
				++NodeCount;
				return STATUS_SUCCESS;
			}
			);
			FAIL_RETURN(Status);

			GrowableArray<NODE_OFFSET_MAP>              NodeOffsetTable;
			GrowableArray<NODE_FIX_ENTRY>               NodeFixEntries;
			GrowableArray<TRIE_NODE_OFFSET_TABLE_ENTRY> OffsetEntries;

			IndexBitsBase = nullptr;
			IndexBufferBase = nullptr;
			NodesBufferBase = nullptr;

			SCOPE_EXIT
			{
				FreeMemoryP(IndexBitsBase);
				FreeMemoryP(IndexBufferBase);
				FreeMemoryP(NodesBufferBase);
			}
			SCOPE_EXIT_END;

			IndexBitsSize = NodeCount * TrieBitsIndex::Max;
			IndexBufferSize = NodeCount * MAXIMUM_NEXT_NODE;
			NodesBufferSize = NodeCount * sizeof(*this->Root);

			IndexBufferBase = (PBYTE)AllocateMemoryP(IndexBufferSize);
			if (IndexBufferBase == nullptr)
				return STATUS_NO_MEMORY;

			IndexBitsBase = (PBYTE)AllocateMemoryP(IndexBitsSize);
			if (IndexBitsBase == nullptr)
				return STATUS_NO_MEMORY;

			NodesBufferBase = (PBYTE)AllocateMemoryP(NodesBufferSize);
			if (NodesBufferBase == nullptr)
				return STATUS_NO_MEMORY;

			IndexBits = IndexBitsBase;
			IndexBuffer = IndexBufferBase;
			NodesBuffer = NodesBufferBase;

			ZeroMemory(IndexBits, IndexBitsSize);
			ZeroMemory(IndexBuffer, IndexBufferSize);
			ZeroMemory(NodesBuffer, NodesBufferSize);

			Status = EnumNodes(
				[&](PNodeType Parent, PNodeType Node, ULONG_PTR IndexOfNext)
			{
				BYTE                            LocalIndex[MAXIMUM_NEXT_NODE];
				BYTE                            LocalBits[TrieBitsIndex::Max];
				ULONG_PTR                       OffsetTableIndex;
				ULONG_PTR                       NodeCount, Length, FirstNodeIndex, LastNodeIndex;
				PNodeType                      *NextBase, Next;
				PBYTE                           ReferenceBuffer, SearchBase, SearchEnd;
				TRIE_NODE_OFFSET_TABLE_ENTRY    TableEntry;
				PCOMPACT_TRIE_NODE              CompactNode;

				ZeroMemory(LocalIndex, sizeof(LocalIndex));
				ZeroMemory(LocalBits, sizeof(LocalBits));

				CompactNode = (PCOMPACT_TRIE_NODE)NodesBuffer;
				NodesBuffer += sizeof(*CompactNode);

				NodeCount = 0;

				FirstNodeIndex = ULONG_PTR_MAX;
				LastNodeIndex = 0;

				for (ULONG_PTR Index = 0; Index != MAXIMUM_NEXT_NODE; ++Index)
				{
					Next = Node->GetNext(Index);
					if (Next == nullptr)
						continue;

					LocalBits[Index / 8] |= 1 << (Index % 8);
					LocalIndex[Index] = (BYTE)NodeCount++;

					NodeFixEntries.Add(NODE_FIX_ENTRY(CompactNode, Next, NodesBuffer));

					NodesBuffer += sizeof(*NodeFixEntries[0].Offset);

					FirstNodeIndex = ML_MIN(FirstNodeIndex, Index);
					LastNodeIndex = Index + 1;
				}

				if (Node->TestFlags(TrieBitsFlags::HasContext))
				{
					*(PNODE_CONTEXT)NodesBuffer = Node->Context.Value;
					NodesBuffer += sizeof(Node->Context.Value);
					SET_FLAG(LocalBits[TrieBitsIndex::Flags], TrieBitsFlags::HasContext);
				}

				if (NodeCount != 0)
				{
					SET_FLAG(LocalBits[TrieBitsIndex::Flags], TrieBitsFlags::HasNodes);
					LocalBits[TrieBitsIndex::NodeCount] = (BYTE)(NodeCount - 1);
				}

				NodeOffsetTable.Add(NODE_OFFSET_MAP(Node, PtrOffset(CompactNode, NodesBufferBase)));

				SearchBase = IndexBitsBase;
				SearchEnd = IndexBits;
				Length = sizeof(LocalBits);
				ReferenceBuffer = (PBYTE)FindReuseData(SearchBase, SearchEnd, LocalBits, Length);

				TableEntry.IndexBitsOffset = (UNSIGNED_OFFSET)PtrOffset(ReferenceBuffer, IndexBitsBase);

				if (ReferenceBuffer + Length > SearchEnd)
				{
					CopyMemory(ReferenceBuffer, LocalBits, Length);
					IndexBits = PtrAdd(ReferenceBuffer, Length);
				}

				TableEntry.IndexOffset = (UNSIGNED_OFFSET)-1;

				if (NodeCount != 0)
				{
					ULONG_PTR ForwardLength;

					ForwardLength = FirstNodeIndex * sizeof(LocalIndex[0]);

					SearchBase = IndexBufferBase + sizeof(LocalIndex);
					SearchEnd = IndexBuffer;
					Length = (LastNodeIndex - FirstNodeIndex) * sizeof(LocalIndex[0]);
					ReferenceBuffer = (PBYTE)FindReuseData(SearchBase, SearchEnd, &LocalIndex[FirstNodeIndex], Length);

					TableEntry.IndexOffset = (UNSIGNED_OFFSET)(PtrOffset(ReferenceBuffer, IndexBufferBase) - ForwardLength);

					if (ReferenceBuffer + Length > SearchEnd)
					{
						CopyMemory(ReferenceBuffer, &LocalIndex[FirstNodeIndex], Length);
						IndexBuffer = PtrAdd(ReferenceBuffer, Length);
					}
				}

				OffsetTableIndex = OffsetEntries.IndexOf(TableEntry);
				if (OffsetTableIndex == OffsetEntries.kInvalidIndex)
				{
					OffsetEntries.Add(TableEntry);
					OffsetTableIndex = OffsetEntries.GetSize() - 1;
				}

				if (OffsetTableIndex > 0xFFFF)
					DbgBreakPoint();

				CompactNode->OffsetTableIndex = (USHORT)OffsetTableIndex;

				return STATUS_SUCCESS;
			}
			);

			FOR_EACH_VEC(FixEntry, NodeFixEntries)
			{
				ULONG_PTR   CompactSize, NodeCount, Offset, ContextSize;
				PBYTE       IndexBits, IndexTable;
				PVOID       CompactEnd;
				PTRIE_NODE_OFFSET_TABLE_ENTRY TableEntry = &OffsetEntries[FixEntry->Compact->OffsetTableIndex];

				IndexBits = PtrAdd(IndexBitsBase, TableEntry->IndexBitsOffset);
				IndexTable = PtrAdd(IndexBufferBase, TableEntry->IndexOffset);

				NodeCount = (IndexBits[TrieBitsIndex::NodeCount] + 1) & -FLAG_ON(IndexBits[TrieBitsIndex::Flags], TrieBitsFlags::HasNodes);
				ContextSize = FLAG_ON(IndexBits[TrieBitsIndex::Flags], TrieBitsFlags::HasContext) ? sizeof(NODE_CONTEXT) : 0;

				CompactSize = sizeof(*FixEntry->Compact);
				CompactSize += NodeCount * sizeof(*FixEntry->Offset);
				CompactSize += ContextSize;

				//CompactEnd = PtrAdd(FixEntry->Compact, CompactSize);

				Offset = LookupNodeOffsetEntry(NodeOffsetTable, FixEntry->Node)->Offset;
				Offset = PtrOffset(Offset, PtrOffset(FixEntry->Compact, NodesBufferBase));

				//if (Offset >= 0xFFFF) DbgBreakPoint();

				*FixEntry->Offset = (UNSIGNED_OFFSET)Offset;
			}

			IndexBitsSize = PtrOffset(IndexBits, IndexBitsBase);
			IndexBufferSize = PtrOffset(IndexBuffer, IndexBufferBase);
			NodesBufferSize = PtrOffset(NodesBuffer, NodesBufferBase);
			OffsetTableSize = OffsetEntries.GetSize() * sizeof(OffsetEntries[0]);

			FinalSize = sizeof(*CompactHeader) + IndexBitsSize + IndexBufferSize + OffsetTableSize + NodesBufferSize + 0x200;

			*CompactTree = AllocateMemoryP(FinalSize);

			if (*CompactTree != nullptr)
			{
				CompactHeader = (PCOMPACT_TRIE_HEADER)*CompactTree;

				ZeroMemory(CompactHeader, FinalSize);

				CompactHeader->Magic = COMPACT_TRIE_MAGIC;
				CompactHeader->IndexBitsTableOffset = sizeof(*CompactHeader);
				CompactHeader->IndexTableOffset = CompactHeader->IndexBitsTableOffset + ROUND_UP(IndexBitsSize, 16);
				CompactHeader->OffsetTableOffset = CompactHeader->IndexTableOffset + ROUND_UP(IndexBufferSize, 16);
				CompactHeader->NodesDataOffset = CompactHeader->OffsetTableOffset + ROUND_UP(OffsetTableSize, 16);

				++CompactHeader;

				CopyMemory(CompactHeader, IndexBitsBase, IndexBitsSize);
				CompactHeader = PtrAdd(CompactHeader, ROUND_UP(IndexBitsSize, 16));

				CopyMemory(CompactHeader, IndexBufferBase, IndexBufferSize);
				CompactHeader = PtrAdd(CompactHeader, ROUND_UP(IndexBufferSize, 16));

				CopyMemory(CompactHeader, OffsetEntries.GetData(), OffsetTableSize);
				CompactHeader = PtrAdd(CompactHeader, ROUND_UP(OffsetTableSize, 16));

				CopyMemory(CompactHeader, NodesBufferBase, NodesBufferSize);
				CompactHeader = PtrAdd(CompactHeader, ROUND_UP(NodesBufferSize, 16));

				*CompactSize = PtrOffset(CompactHeader, *CompactTree);
			}
			else
			{
				Status = STATUS_NO_MEMORY;
			}

			return Status;
		}

		/*++

		NTSTATUS CallBack(PNodeType Parent, PNodeType Node, ULONG_PTR IndexOfNext);

		--*/

		template<typename CALL_BACK> NTSTATUS EnumNodes(CALL_BACK CallBack)
		{
			NTSTATUS    Status;
			ULONG_PTR   QueueIndex;
			PNodeType   Root, Node, Next;

			static const ULONG_PTR NumberOfNext = MAXIMUM_NEXT_NODE;

			Root = this->Root;
			QueueIndex = 0;

			if (Root == nullptr)
				return STATUS_NO_MORE_ENTRIES;


			NodePArray Queue;

			Status = Queue.Add(Root);
			FAIL_RETURN(Status);

			Status = CallBack(nullptr, Root, Queue.kInvalidIndex);
			FAIL_RETURN(Status);

			do
			{
				Node = Queue[QueueIndex++];

				for (ULONG_PTR Index = 0; Index != NumberOfNext; ++Index)
				{
					Next = Node->GetNext(Index);
					if (Next == nullptr)
						continue;

					Status = CallBack(Node, Next, Index);
					FAIL_RETURN(Status);

					Status = Queue.Add(Next);
					FAIL_RETURN(Status);
				}

			} while (QueueIndex != Queue.GetSize());

			return STATUS_SUCCESS;
		}

		NTSTATUS LookupWithoutFailure(PTRIE_BYTES_ENTRY DataToLookup, PNODE_CONTEXT Context = nullptr)
		{
			return LookupWorker(
				[](PNodeType)
			{
				return nullptr;
			},
				DataToLookup,
				Context
				);
		}

		NTSTATUS Lookup(PTRIE_BYTES_ENTRY DataToLookup, PNODE_CONTEXT Context = nullptr)
		{
			return LookupWorker(
				[](PNodeType Node)
			{
				return Node->Failure;
			},
				DataToLookup,
				Context
				);
		}


	protected:

		template<typename T>
		NoInline
			NTSTATUS
			LookupWorker(
				T                   GetFailureRoutine,
				PTRIE_BYTES_ENTRY   DataToLookup,
				PNODE_CONTEXT       Context = nullptr
			)
		{
			PBYTE       Buffer;
			ULONG_PTR   Index;
			PNodeType   Node, Next;

			Node = this->Root;
			if (Node == nullptr)
				return STATUS_FLT_NOT_INITIALIZED;

			FOR_EACH(Buffer, (PBYTE)DataToLookup->Data, DataToLookup->SizeInBytes)
			{
				Index = Buffer[0];
				Next = Node->GetNext(Index);
				while (Next == nullptr)
				{
					Node = GetFailureRoutine(Node);
					if (Node == nullptr)
						return STATUS_NOT_FOUND;

					Next = Node->GetNext(Index);
				}

				Node = Next;
			}

			if (!Node->TestFlags(TrieBitsFlags::HasContext))
				return STATUS_CONTEXT_MISMATCH;

			if (Context != nullptr)
				*Context = Node->Context.Value;

			return STATUS_SUCCESS;
		}
	};


	typedef struct
	{
		PVOID       Pattern;
		ULONG_PTR   Size;
		ULONG_PTR   Flags;
		LONG_PTR    HeadOffsetToNext;
		LONG_PTR    OffsetToPrevHead;
		ULONG_PTR   Extra;

	} SEARCH_PATTERN_DATA, *PSEARCH_PATTERN_DATA;

#define PATTERN_AUTO_SEARCH     (1u << 0)
#define PATTERN_SEARCH_RANGE    (1u << 1)

#define ADD_PATTERN_(_bytes, _size, ...) { _bytes, _size, __VA_ARGS__ }

#define ADD_PATTERN(_bytes_arr, ...) ADD_PATTERN_((_bytes_arr), sizeof(_bytes_arr), __VA_ARGS__)
#define ADD_PATTERN_F(_bytes_arr, _flags, _offset_to_next, _offset_to_prev) ADD_PATTERN_((_bytes_arr), sizeof(_bytes_arr), _flags, _offset_to_next, _offset_to_prev)

	template<typename CallbackRoutine, typename ContextType>
	inline
		PVOID
		SearchPatternT(
			PSEARCH_PATTERN_DATA    Patterns,
			ULONG_PTR               PatternCount,
			PVOID                   Begin,
			LONG_PTR                Length,
			CallbackRoutine         Callback,
			ContextType             Context
		)
	{
		LONG_PTR                RemainSize;
		ULONG_PTR               Offset;
		PVOID                   Found;
		PBYTE                   Buffer, End;
		NTSTATUS                Status;
		PSEARCH_PATTERN_DATA    Data;

		Buffer = (PBYTE)Begin;
		End = Buffer + Length;
		while (Length > 0)
		{
			ULONG_PTR Count;

			Data = Patterns;
			RemainSize = Length;

			Found = Sunday(Buffer, RemainSize, Data->Pattern, Data->Size);
			if (Found == nullptr)
				return Found;

			Buffer = (PBYTE)Found + Data->Size;

			if (Data->HeadOffsetToNext != 0)
			{
				Buffer += Data->HeadOffsetToNext - Data->Size;
			}

			RemainSize = PtrOffset(End, Buffer);

			if (RemainSize < 0)
				return nullptr;

			if (PatternCount != 1)
			{
				for (Count = PatternCount - 1; Count != 0; --Count)
				{
					++Data;

					if (Data->OffsetToPrevHead != 0)
					{
						Offset = Data->OffsetToPrevHead - Data[-1].Size;
						Buffer += Offset;
						RemainSize -= Offset;
					}

					if ((ULONG_PTR)RemainSize < Data->Size)
						return nullptr;

					if (FLAG_ON(Data->Flags, PATTERN_AUTO_SEARCH))
					{
						PVOID AutoSearch;

						Offset = FLAG_ON(Data->Flags, PATTERN_SEARCH_RANGE) ? Data->Extra : RemainSize;
						AutoSearch = Sunday(Buffer, Offset, Data->Pattern, Data->Size);
						if (AutoSearch == nullptr)
							return nullptr;

						Buffer = (PBYTE)AutoSearch + Data->Size;

						if (Data->HeadOffsetToNext != 0)
						{
							Buffer += Data->HeadOffsetToNext - Data->Size;
						}

						RemainSize = PtrOffset(End, Buffer);

						continue;
					}

					if (RtlCompareMemory(Buffer, Data->Pattern, Data->Size) != Data->Size)
						break;

					Buffer += Data->Size;

					if (Data->HeadOffsetToNext != 0)
					{
						Buffer += Data->HeadOffsetToNext - Data->Size;
					}

					RemainSize = PtrOffset(End, Buffer);

					if (RemainSize < 0)
						return nullptr;
				}

				if (Count != 0)
				{
					Buffer = (PBYTE)Found + Patterns[0].Size + Patterns[0].HeadOffsetToNext;

					if (Patterns[0].HeadOffsetToNext != 0)
					{
						Buffer += Patterns[0].HeadOffsetToNext - Patterns[0].Size;
					}

					Length = PtrOffset(End, Buffer);
					continue;
				}
			}
			else
			{
				;
			}

			Status = Callback(Found, Begin, RemainSize, Context);
			if (NT_SUCCESS(Status))
				return Found;

			Buffer = (PBYTE)Found + Patterns[0].Size + Patterns[0].HeadOffsetToNext;
			Length = PtrOffset(End, Buffer);
		}

		return nullptr;
	}

	ForceInline
		PVOID
		SearchPattern(
			PSEARCH_PATTERN_DATA    Patterns,
			ULONG_PTR               PatternCount,
			PVOID                   Begin,
			LONG_PTR                Length
		)
	{
		auto AutoPatternCheck = [](PVOID, PVOID, ULONG_PTR, PVOID) { return STATUS_SUCCESS; };

		return SearchPatternT(Patterns, PatternCount, Begin, Length, AutoPatternCheck, (PVOID)0);
	}

	inline
		PVOID
		SearchPatternSafe(
			PSEARCH_PATTERN_DATA    Patterns,
			ULONG_PTR               PatternCount,
			PVOID                   Begin,
			LONG_PTR                Length
		)
	{
		SEH_TRY
		{
			return SearchPattern(Patterns, PatternCount, Begin, Length);
		}
			SEH_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
		{
			return nullptr;
		}
	}

	template<class T = VOID>
	inline PVOID SearchPatternSafe(const ml::String& Pattern, PVOID Begin, LONG_PTR Length)
	{
		GrowableArray<SEARCH_PATTERN_DATA>  Patterns;
		GrowableArray<ml::String::ByteArray *>  BytesArray;
		ml::String::ByteArray*                  CurrentBytes;
		SEARCH_PATTERN_DATA                 CurrentPattern;
		ULONG_PTR                           GapBytes;

		SCOPE_EXIT
		{
			for (auto &p : BytesArray)
			delete p;
		}
		SCOPE_EXIT_END;

		CurrentBytes = nullptr;
		GapBytes = 0;
		ZeroMemory(&CurrentPattern, sizeof(CurrentPattern));

		for (ml::String &p : Pattern.Split(' '))
		{
			if (!p)
				continue;

			if (p.GetCount() != 2)
				return nullptr;

			if (p[0] == '?' && p[1] == '?')
			{
				++GapBytes;
			}
			else
			{
				ULONG Hex;

				if (GapBytes != 0)
				{
					CurrentPattern.Pattern = CurrentBytes->GetData();
					CurrentPattern.Size = CurrentBytes->GetSize();
					CurrentPattern.HeadOffsetToNext = CurrentPattern.Size + GapBytes;
					Patterns.Add(CurrentPattern);

					ZeroMemory(&CurrentPattern, sizeof(CurrentPattern));

					GapBytes = 0;
					BytesArray.Add(CurrentBytes);
					CurrentBytes = nullptr;
				}

				if (CurrentBytes == nullptr)
					CurrentBytes = new ml::String::ByteArray;

				Hex = p.ToHex();
				CurrentBytes->Add(Hex);
			}
		}

		if (CurrentBytes != nullptr)
		{
			BytesArray.Add(CurrentBytes);

			CurrentPattern.Pattern = CurrentBytes->GetData();
			CurrentPattern.Size = CurrentBytes->GetSize();
			Patterns.Add(CurrentPattern);
		}

		return SearchPatternSafe(Patterns.GetData(), Patterns.GetSize(), Begin, Length);
	}

	template<class T = VOID>
	inline PVOID SearchPatternSafe(PCSTR Pattern, PVOID Begin, LONG_PTR Length)
	{
		return SearchPatternSafe(ml::String::Decode((PVOID)Pattern, StrLengthA(Pattern), CP_ACP), Begin, Length);
	}

	inline PVOID ReverseSearchFunctionHeader(PVOID Start, ULONG_PTR Length)
	{
		PBYTE Buffer;

		Buffer = (PBYTE)Start;

		for (; Length != 0; --Buffer, --Length)
		{
			switch (Buffer[0])
			{
			case CALL:
				// push    local_var_size
				// mov     eax, exception_handler
				// call    _SEH_prolog

				if (Buffer[-5] != 0xB8)
					continue;

				if (Buffer[-7] == 0x6A)
				{
					Buffer -= 7;
				}
				else if (Buffer[-10] == 0x68)
				{
					Buffer -= 10;
				}
				else
				{
					continue;
				}

				break;

			case 0x55:
				if (Buffer[1] != 0x8B || Buffer[2] != 0xEC)
					continue;

				// push ebp
				// mov ebp, esp

				break;

			default:
				continue;
			}

			return Buffer;
		}

		return nullptr;
	}

	inline PVOID SearchStringReference(LDR_MODULE* Module, PVOID String, ULONG_PTR SizeInBytes, ULONG_PTR BeginOffset = 0)
	{
		PVOID StringValue, StringReference;

		SEARCH_PATTERN_DATA Str[] =
		{
			ADD_PATTERN_(String, SizeInBytes),
		};

		StringValue = SearchPattern(Str, countof(Str), Module->DllBase, Module->SizeOfImage);
		if (StringValue == nullptr)
			return nullptr;

		SEARCH_PATTERN_DATA Stub[] =
		{
			ADD_PATTERN(&StringValue),
		};

		StringReference = SearchPattern(Stub, countof(Stub), PtrAdd(Module->DllBase, BeginOffset), PtrSub(Module->SizeOfImage, BeginOffset));
		if (StringReference == nullptr)
			return nullptr;

		return StringReference;
	}




	using namespace ml::Native;
	using namespace ml::Native::Ldr;
	using namespace ml::Native::Mm;
	using namespace ml::Native::Ps;
	using namespace ml::Native::Reg;
	using namespace ml::Native::Rtl;
	using namespace ml::Native::Nls;
	using namespace ml::Native::Exp;
	using namespace ml::Native::Lpc;
	using namespace ml::Native::Io;


#if ML_KERNEL_MODE

	ML_NAMESPACE_BEGIN(WindowsVersionsInfoClass)

		enum
	{
		Windows7 = 0x00000601,
		Windows7SP1 = 0x01000601,
		Windows2008R2 = 0x80000601,

		Windows8 = 0x00000602,
		WindowsServer2012 = 0x80000602,

		Windows81 = 0x00000603,
		WindowsServer2012R2 = 0x80000603,

		InvalidVersion = 0xFFFFFFFF,
	};

	inline ULONG GetWindowsVersion()
	{
		NTSTATUS                        Status;
		ULONG                           CSDVersion;
		RTL_OSVERSIONINFOEXW            Version;
		PKEY_VALUE_PARTIAL_INFORMATION  Value;

		static ULONG WindowsVersion = 0;

		if (WindowsVersion != 0)
		{
			return WindowsVersion;
		}

		Version.dwOSVersionInfoSize = sizeof(Version);
		Status = RtlGetVersion((PRTL_OSVERSIONINFOW)&Version);
		if (NT_FAILED(Status))
			return InvalidVersion;

		Status = Reg::GetKeyValue(HKEY_LOCAL_MACHINE, L"System\\CurrentControlSet\\Control\\Windows", L"CSDVersion", &Value);
		if (NT_FAILED(Status))
			return InvalidVersion;

		if (Value->Type != REG_DWORD || Value->DataLength != sizeof(CSDVersion))
		{
			FreeKeyInfo(Value);
			return InvalidVersion;
		}

		CSDVersion = *(PULONG)Value->Data;
		FreeKeyInfo(Value);

		WindowsVersion = (Version.dwMajorVersion << 8 | Version.dwMinorVersion | ((CSDVersion << 16) & 0x7FFF0000)) | (Version.wProductType != VER_NT_WORKSTATION ? 0x80000000 : 0);

		return WindowsVersion;
	}

	inline BOOL IsSupportedVersion()
	{
		switch (WindowsVersionsInfoClass::GetWindowsVersion())
		{
		case WindowsVersionsInfoClass::Windows7:
		case WindowsVersionsInfoClass::Windows7SP1:
		case WindowsVersionsInfoClass::Windows8:
		case WindowsVersionsInfoClass::Windows81:
			return TRUE;

		default:
			return FALSE;
		}
	}

	ML_NAMESPACE_END_(WindowsVersionsInfoClass);

#pragma push_macro("DEFINE_PROPERTY")
#pragma push_macro("DECLARE_ACCESSOR")
#pragma push_macro("FIELD_ACCESSOR")
#pragma push_macro("SET_ACCESSOR")
#pragma push_macro("CALL_ACCESSOR")

#undef DEFINE_PROPERTY
#undef DECLARE_ACCESSOR
#undef FIELD_ACCESSOR
#undef SET_ACCESSOR
#undef CALL_ACCESSOR


	typedef struct _OBJECT_TYPE_INITIALIZER_WIN7_760X               // 25 elements, 0x50 bytes (sizeof)
	{
		/*0x000*/     USHORT       Length;
		union                                             // 2 elements, 0x1 bytes (sizeof)
		{
			/*0x002*/         UCHAR        ObjectTypeFlags;
			struct                                        // 7 elements, 0x1 bytes (sizeof)
			{
				/*0x002*/             UCHAR        CaseInsensitive : 1;         // 0 BitPosition
				/*0x002*/             UCHAR        UnnamedObjectsOnly : 1;      // 1 BitPosition
				/*0x002*/             UCHAR        UseDefaultObject : 1;        // 2 BitPosition
				/*0x002*/             UCHAR        SecurityRequired : 1;        // 3 BitPosition
				/*0x002*/             UCHAR        MaintainHandleCount : 1;     // 4 BitPosition
				/*0x002*/             UCHAR        MaintainTypeList : 1;        // 5 BitPosition
				/*0x002*/             UCHAR        SupportsObjectCallbacks : 1; // 6 BitPosition
			};
		};
		/*0x004*/     ULONG      ObjectTypeCode;
		/*0x008*/     ULONG      InvalidAttributes;
		/*0x00C*/     struct _GENERIC_MAPPING GenericMapping;           // 4 elements, 0x10 bytes (sizeof)
		/*0x01C*/     ULONG      ValidAccessMask;
		/*0x020*/     ULONG      RetainAccess;
		/*0x024*/     enum _POOL_TYPE PoolType;
		/*0x028*/     ULONG      DefaultPagedPoolCharge;
		/*0x02C*/     ULONG      DefaultNonPagedPoolCharge;
		/*0x030*/     PVOID      DumpProcedure;                       // FUNCT_00A4_0ED5_DumpProcedure*
		/*0x034*/     PVOID      OpenProcedure;                       // FUNCT_000F_0EDD_OpenProcedure*
		/*0x038*/     PVOID      CloseProcedure;                      // FUNCT_00A4_0EEB_CloseProcedure*
		/*0x03C*/     PVOID      DeleteProcedure;                     // FUNCT_00A4_0662_Free_InterfaceReference_InterfaceDereference_DeleteProcedure_WorkerRoutine_Callback_ReleaseFromLazyWrite_ReleaseFromReadAhead*
		/*0x040*/     PVOID      ParseProcedure;                      // FUNCT_000F_0EF1_ParseProcedure*
		/*0x044*/     PVOID      SecurityProcedure;                   // FUNCT_000F_0EFD_SecurityProcedure*
		/*0x048*/     PVOID      QueryNameProcedure;                  // FUNCT_000F_0F0E_QueryNameProcedure*
		/*0x04C*/     PVOID      OkayToCloseProcedure;                // FUNCT_0067_0F16_OkayToCloseProcedure*

	} OBJECT_TYPE_INITIALIZER_WIN7_760X, *POBJECT_TYPE_INITIALIZER_WIN7_760X;


	typedef struct _OBJECT_TYPE_INITIALIZER_WIN8_9X00                    // 29 elements, 0x58 bytes (sizeof)
	{
		/*0x000*/     USHORT       Length;
		union                                             // 2 elements, 0x1 bytes (sizeof)
		{
			/*0x002*/         UCHAR        ObjectTypeFlags;
			struct                                        // 8 elements, 0x1 bytes (sizeof)
			{
				/*0x002*/             UCHAR        CaseInsensitive : 1;         // 0 BitPosition
				/*0x002*/             UCHAR        UnnamedObjectsOnly : 1;      // 1 BitPosition
				/*0x002*/             UCHAR        UseDefaultObject : 1;        // 2 BitPosition
				/*0x002*/             UCHAR        SecurityRequired : 1;        // 3 BitPosition
				/*0x002*/             UCHAR        MaintainHandleCount : 1;     // 4 BitPosition
				/*0x002*/             UCHAR        MaintainTypeList : 1;        // 5 BitPosition
				/*0x002*/             UCHAR        SupportsObjectCallbacks : 1; // 6 BitPosition
				/*0x002*/             UCHAR        CacheAligned : 1;            // 7 BitPosition
			};
		};

		/*0x004*/     ULONG      ObjectTypeCode;
		/*0x008*/     ULONG      InvalidAttributes;
		/*0x00C*/     struct _GENERIC_MAPPING GenericMapping;           // 4 elements, 0x10 bytes (sizeof)
		/*0x01C*/     ULONG      ValidAccessMask;
		/*0x020*/     ULONG      RetainAccess;
		/*0x024*/     enum _POOL_TYPE PoolType;
		/*0x028*/     ULONG      DefaultPagedPoolCharge;
		/*0x02C*/     ULONG      DefaultNonPagedPoolCharge;
		/*0x030*/     PVOID      DumpProcedure;                       // FUNCT_00BC_140F_DumpProcedure*
		/*0x034*/     PVOID      OpenProcedure;                       // FUNCT_0072_1417_OpenProcedure*
		/*0x038*/     PVOID      CloseProcedure;                      // FUNCT_00BC_1425_CloseProcedure*
		/*0x03C*/     PVOID      DeleteProcedure;                     // FUNCT_00BC_06F1_Free_WorkerRoutine_DeleteCallback_InterfaceReference_InterfaceDereference_DeleteProcedure_DevicePowerRequired_DevicePowerNotRequired_Callback_ReleaseFromLazyWrite_ReleaseFromReadAhead*
		/*0x040*/     PVOID      ParseProcedure;                      // FUNCT_0072_142B_ParseProcedure*
		/*0x044*/     PVOID      SecurityProcedure;                   // FUNCT_0072_1437_SecurityProcedure*
		/*0x048*/     PVOID      QueryNameProcedure;                  // FUNCT_0072_1448_QueryNameProcedure*
		/*0x04C*/     PVOID      OkayToCloseProcedure;                // FUNCT_007B_1450_OkayToCloseProcedure*
		/*0x050*/     ULONG      WaitObjectFlagMask;
		/*0x054*/     USHORT     WaitObjectFlagOffset;
		/*0x056*/     USHORT     WaitObjectPointerOffset;

	} OBJECT_TYPE_INITIALIZER_WIN8_9X00, *POBJECT_TYPE_INITIALIZER_WIN8_9X00;


	typedef struct _OBJECT_TYPE_WIN8                            // 12 elements, 0x90 bytes (sizeof)
	{
		/*0x000*/     LIST_ENTRY        TypeList;                   // 2 elements, 0x8 bytes (sizeof)
		/*0x008*/     UNICODE_STRING    Name;                       // 3 elements, 0x8 bytes (sizeof)
		/*0x010*/     PVOID             DefaultObject;              // VOID*
		/*0x014*/     UCHAR             Index;
		/*0x018*/     ULONG             TotalNumberOfObjects;
		/*0x01C*/     ULONG             TotalNumberOfHandles;
		/*0x020*/     ULONG             HighWaterNumberOfObjects;
		/*0x024*/     ULONG             HighWaterNumberOfHandles;
		OBJECT_TYPE_INITIALIZER_WIN8_9X00 TypeInfo[1];
#if 0
		/*0x028*/     struct _OBJECT_TYPE_INITIALIZER TypeInfo;     // 29 elements, 0x58 bytes (sizeof)
		/*0x080*/     struct _EX_PUSH_LOCK TypeLock;                // 7 elements, 0x4 bytes (sizeof)
		/*0x084*/     ULONG32      Key;
		/*0x088*/     struct _LIST_ENTRY CallbackList;              // 2 elements, 0x8 bytes (sizeof)
#endif

	} OBJECT_TYPE_WIN8_9X00, *POBJECT_TYPE_WIN8_9X00;

	typedef OBJECT_TYPE_WIN8_9X00 OBJECT_TYPE_WIN7_760X, *POBJECT_TYPE_WIN7_760X;


	template<class _FIELD_TYPE, ULONG_PTR FieldOffset>
	ForceInline _FIELD_TYPE& FieldAccessor(PVOID Object)
	{
		return *(_FIELD_TYPE *)PtrAdd(Object, FieldOffset);
	}


#define DEFINE_PROPERTY_(_RetType, _Object, _Type, _Field) \
            READONLY_PROPERTY(_RetType, _Field) \
												            { \
                CALL_ACCESSOR(_Object, _Field); \
												            }

#define DEFINE_PROPERTY(_Object, _Type, _Field) DEFINE_PROPERTY_(FIELD_TYPE(_Type, _Field)&, _Object, _Type, _Field)


#define DECLARE_ACCESSOR_(_RetType, _Object, _Type, _Field) \
            protected: _RetType (*Accessor_##_Field)(PVOID Object);\
            public: DEFINE_PROPERTY_(_RetType, _Object, _Type, _Field)

#define DECLARE_ACCESSOR(_Object, _Type, _Field) DECLARE_ACCESSOR_(FIELD_TYPE(_Type, _Field)&, _Object, _Type, _Field)


#define FIELD_ACCESSOR(_Type, _Field) FieldAccessor<FIELD_TYPE(_Type, _Field), FIELD_OFFSET(_Type,_Field)>

#define SET_ACCESSOR(_Type, _Field) \
            __if_exists(_Type::_Field) \
												            { \
                *(PVOID *)&Accessor_##_Field = PtrAdd((PVOID)0, FIELD_ACCESSOR(_Type, _Field)) \
												            } \
            __if_not_exists(_Type::_Field) \
												            { \
                *(PVOID *)&Accessor_##_Field = PtrSub(IMAGE_INVALID_VA, 0x5432) \
												            }

#define CALL_ACCESSOR(_Object, _Field) return this->Accessor_##_Field(_Object)

	class ML_OBJECT_TYPE
	{
	protected:
		POBJECT_TYPE_WIN8_9X00 ObjectType;

	public:

		BOOL Success;

		DECLARE_ACCESSOR(this->ObjectType, OBJECT_TYPE_WIN8_9X00, TypeList);
		DECLARE_ACCESSOR(this->ObjectType, OBJECT_TYPE_WIN8_9X00, Name);
		DECLARE_ACCESSOR(this->ObjectType, OBJECT_TYPE_WIN8_9X00, DefaultObject);
		DECLARE_ACCESSOR(this->ObjectType, OBJECT_TYPE_WIN8_9X00, Index);
		DECLARE_ACCESSOR(this->ObjectType, OBJECT_TYPE_WIN8_9X00, TotalNumberOfObjects);
		DECLARE_ACCESSOR(this->ObjectType, OBJECT_TYPE_WIN8_9X00, TotalNumberOfHandles);
		DECLARE_ACCESSOR(this->ObjectType, OBJECT_TYPE_WIN8_9X00, HighWaterNumberOfObjects);
		DECLARE_ACCESSOR(this->ObjectType, OBJECT_TYPE_WIN8_9X00, HighWaterNumberOfHandles);
		DECLARE_ACCESSOR_(POBJECT_TYPE_INITIALIZER, this->ObjectType, OBJECT_TYPE_WIN8_9X00, TypeInfo);

		NoInline ML_OBJECT_TYPE(POBJECT_TYPE ObjectType = nullptr)
		{
			this->Success = FALSE;

			if (WindowsVersionsInfoClass::IsSupportedVersion() == FALSE)
				return;

			*(POBJECT_TYPE *)&this->ObjectType = ObjectType;
			Initialize<OBJECT_TYPE_WIN8_9X00>();

			this->Success = TRUE;
		}

		ML_OBJECT_TYPE* operator->()
		{
			return this;
		}

		operator POBJECT_TYPE()
		{
			return (POBJECT_TYPE)this->ObjectType;
		}

	protected:

		template<class OBJECT_TYPE_TYPE>
		VOID Initialize()
		{
			SET_ACCESSOR(OBJECT_TYPE_TYPE, TypeList);
			SET_ACCESSOR(OBJECT_TYPE_TYPE, Name);
			SET_ACCESSOR(OBJECT_TYPE_TYPE, DefaultObject);
			SET_ACCESSOR(OBJECT_TYPE_TYPE, Index);
			SET_ACCESSOR(OBJECT_TYPE_TYPE, TotalNumberOfObjects);
			SET_ACCESSOR(OBJECT_TYPE_TYPE, TotalNumberOfHandles);
			SET_ACCESSOR(OBJECT_TYPE_TYPE, HighWaterNumberOfObjects);
			SET_ACCESSOR(OBJECT_TYPE_TYPE, HighWaterNumberOfHandles);
			SET_ACCESSOR(OBJECT_TYPE_TYPE, TypeInfo);
		}
	};

	class ML_OBJECT_TYPE_INITIALIZER_DATA
	{
	protected:
		POBJECT_TYPE_INITIALIZER ObjectTypeInitializerPointer;

		union
		{
			OBJECT_TYPE_INITIALIZER_WIN7_760X   Win7;
			OBJECT_TYPE_INITIALIZER_WIN8_9X00   Win8;

		} ObjectTypeInitializer;

	public:

		BOOL Success;

		DECLARE_ACCESSOR(&this->ObjectTypeInitializer, OBJECT_TYPE_INITIALIZER_WIN8_9X00, Length);
		DECLARE_ACCESSOR(&this->ObjectTypeInitializer, OBJECT_TYPE_INITIALIZER_WIN8_9X00, ObjectTypeFlags);
		DECLARE_ACCESSOR(&this->ObjectTypeInitializer, OBJECT_TYPE_INITIALIZER_WIN8_9X00, ObjectTypeCode);
		DECLARE_ACCESSOR(&this->ObjectTypeInitializer, OBJECT_TYPE_INITIALIZER_WIN8_9X00, InvalidAttributes);
		DECLARE_ACCESSOR(&this->ObjectTypeInitializer, OBJECT_TYPE_INITIALIZER_WIN8_9X00, GenericMapping);
		DECLARE_ACCESSOR(&this->ObjectTypeInitializer, OBJECT_TYPE_INITIALIZER_WIN8_9X00, ValidAccessMask);
		DECLARE_ACCESSOR(&this->ObjectTypeInitializer, OBJECT_TYPE_INITIALIZER_WIN8_9X00, RetainAccess);
		DECLARE_ACCESSOR(&this->ObjectTypeInitializer, OBJECT_TYPE_INITIALIZER_WIN8_9X00, PoolType);
		DECLARE_ACCESSOR(&this->ObjectTypeInitializer, OBJECT_TYPE_INITIALIZER_WIN8_9X00, DefaultPagedPoolCharge);
		DECLARE_ACCESSOR(&this->ObjectTypeInitializer, OBJECT_TYPE_INITIALIZER_WIN8_9X00, DefaultNonPagedPoolCharge);
		DECLARE_ACCESSOR(&this->ObjectTypeInitializer, OBJECT_TYPE_INITIALIZER_WIN8_9X00, DumpProcedure);
		DECLARE_ACCESSOR(&this->ObjectTypeInitializer, OBJECT_TYPE_INITIALIZER_WIN8_9X00, OpenProcedure);
		DECLARE_ACCESSOR(&this->ObjectTypeInitializer, OBJECT_TYPE_INITIALIZER_WIN8_9X00, CloseProcedure);
		DECLARE_ACCESSOR(&this->ObjectTypeInitializer, OBJECT_TYPE_INITIALIZER_WIN8_9X00, DeleteProcedure);
		DECLARE_ACCESSOR(&this->ObjectTypeInitializer, OBJECT_TYPE_INITIALIZER_WIN8_9X00, ParseProcedure);
		DECLARE_ACCESSOR(&this->ObjectTypeInitializer, OBJECT_TYPE_INITIALIZER_WIN8_9X00, SecurityProcedure);
		DECLARE_ACCESSOR(&this->ObjectTypeInitializer, OBJECT_TYPE_INITIALIZER_WIN8_9X00, QueryNameProcedure);
		DECLARE_ACCESSOR(&this->ObjectTypeInitializer, OBJECT_TYPE_INITIALIZER_WIN8_9X00, OkayToCloseProcedure);
		DECLARE_ACCESSOR(&this->ObjectTypeInitializer, OBJECT_TYPE_INITIALIZER_WIN8_9X00, WaitObjectFlagMask);
		DECLARE_ACCESSOR(&this->ObjectTypeInitializer, OBJECT_TYPE_INITIALIZER_WIN8_9X00, WaitObjectFlagOffset);
		DECLARE_ACCESSOR(&this->ObjectTypeInitializer, OBJECT_TYPE_INITIALIZER_WIN8_9X00, WaitObjectPointerOffset);

		NoInline ML_OBJECT_TYPE_INITIALIZER_DATA(POBJECT_TYPE ObjectType = nullptr)
		{
			*(PVOID *)&this->ObjectTypeInitializerPointer = IMAGE_INVALID_VA;
			this->Success = FALSE;

			switch (WindowsVersionsInfoClass::GetWindowsVersion())
			{
			case WindowsVersionsInfoClass::Windows7:
			case WindowsVersionsInfoClass::Windows7SP1:
				InitializeObjectTypeInitializer(&this->ObjectTypeInitializer.Win7);
				break;

			case WindowsVersionsInfoClass::Windows8:
			case WindowsVersionsInfoClass::Windows81:
				InitializeObjectTypeInitializer(&this->ObjectTypeInitializer.Win8);
				break;

			default:
				return;
			}

			if (ObjectType != nullptr)
				(*this) = ObjectType;

			this->Success = TRUE;
		}

		ML_OBJECT_TYPE_INITIALIZER_DATA* operator->()
		{
			return this;
		}

		NoInline ML_OBJECT_TYPE_INITIALIZER_DATA& operator=(POBJECT_TYPE ObjectType)
		{
			ML_OBJECT_TYPE Type = ObjectType;

			CopyMemory(&this->ObjectTypeInitializer, Type->TypeInfo, this->Length);
			return *this;
		}

		operator POBJECT_TYPE_INITIALIZER()
		{
			return this->ObjectTypeInitializerPointer;
		}

	protected:

		POBJECT_TYPE_INITIALIZER FindObjectTypeInitializerFromObjectType(POBJECT_TYPE ObjectType)
		{
			USHORT  Length;
			PBYTE   Buffer;

			Length = this->Length;
			Buffer = (PBYTE)ObjectType;

			for (;; Buffer += sizeof(ULONG_PTR))
			{
				if (*(PUSHORT)Buffer != Length)
					continue;
			}
		}

		template<class OBJECT_TYPE_INITIALIZER_TYPE>
		VOID InitializeObjectTypeInitializer(OBJECT_TYPE_INITIALIZER_TYPE *Ptr)
		{
			*(OBJECT_TYPE_INITIALIZER_TYPE **)&this->ObjectTypeInitializerPointer = Ptr;

			ZeroMemory(Ptr, sizeof(OBJECT_TYPE_INITIALIZER_TYPE));
			Ptr->Length = sizeof(*Ptr);

			SET_ACCESSOR(OBJECT_TYPE_INITIALIZER_TYPE, Length);
			SET_ACCESSOR(OBJECT_TYPE_INITIALIZER_TYPE, ObjectTypeFlags);
			SET_ACCESSOR(OBJECT_TYPE_INITIALIZER_TYPE, ObjectTypeCode);
			SET_ACCESSOR(OBJECT_TYPE_INITIALIZER_TYPE, InvalidAttributes);
			SET_ACCESSOR(OBJECT_TYPE_INITIALIZER_TYPE, GenericMapping);
			SET_ACCESSOR(OBJECT_TYPE_INITIALIZER_TYPE, ValidAccessMask);
			SET_ACCESSOR(OBJECT_TYPE_INITIALIZER_TYPE, RetainAccess);
			SET_ACCESSOR(OBJECT_TYPE_INITIALIZER_TYPE, PoolType);
			SET_ACCESSOR(OBJECT_TYPE_INITIALIZER_TYPE, DefaultPagedPoolCharge);
			SET_ACCESSOR(OBJECT_TYPE_INITIALIZER_TYPE, DefaultNonPagedPoolCharge);
			SET_ACCESSOR(OBJECT_TYPE_INITIALIZER_TYPE, DumpProcedure);
			SET_ACCESSOR(OBJECT_TYPE_INITIALIZER_TYPE, OpenProcedure);
			SET_ACCESSOR(OBJECT_TYPE_INITIALIZER_TYPE, CloseProcedure);
			SET_ACCESSOR(OBJECT_TYPE_INITIALIZER_TYPE, DeleteProcedure);
			SET_ACCESSOR(OBJECT_TYPE_INITIALIZER_TYPE, ParseProcedure);
			SET_ACCESSOR(OBJECT_TYPE_INITIALIZER_TYPE, SecurityProcedure);
			SET_ACCESSOR(OBJECT_TYPE_INITIALIZER_TYPE, QueryNameProcedure);
			SET_ACCESSOR(OBJECT_TYPE_INITIALIZER_TYPE, OkayToCloseProcedure);
			SET_ACCESSOR(OBJECT_TYPE_INITIALIZER_TYPE, WaitObjectFlagMask);
			SET_ACCESSOR(OBJECT_TYPE_INITIALIZER_TYPE, WaitObjectFlagOffset);
			SET_ACCESSOR(OBJECT_TYPE_INITIALIZER_TYPE, WaitObjectPointerOffset);
		}
	};


	class ML_EPROCESS_DYNAMIC
	{
	protected:
		PEPROCESS Process;

		static ULONG_PTR Offset_ProcessLock;
		static ULONG_PTR Offset_RundownProtect;
		static ULONG_PTR Offset_UniqueProcessId;

	public:
		ML_EPROCESS_DYNAMIC(PEPROCESS Process = nullptr)
		{
			(*this) = Process;
		}

		ML_EPROCESS_DYNAMIC& operator=(PEPROCESS Process)
		{
			this->Process = Process;
			return *this;
		}

		operator PEPROCESS()
		{
			return this->Process;
		}

		READONLY_PROPERTY(EX_RUNDOWN_REF&, RundownProtect)
		{
			return *(PEX_RUNDOWN_REF)PtrAdd(this->Process, Offset_RundownProtect);
		}

		READONLY_PROPERTY(HANDLE, UniqueProcessId)
		{
			return *(PHANDLE)PtrAdd(this->Process, Offset_UniqueProcessId);
		}

		ML_EPROCESS_DYNAMIC* operator->()
		{
			return this;
		}

		NoInline static NTSTATUS InitializeOffset()
		{
			ULONG_PTR   Index, OffsetTable[0x1000 / sizeof(ULONG_PTR)];
			PULONG_PTR  Buffer;

			Index = 0;
			FOR_EACH_ARRAY(Buffer, OffsetTable)
			{
				*Buffer = Index++;
			}

			Index = (ULONG_PTR)PsGetProcessId((PEPROCESS)OffsetTable);

			Offset_UniqueProcessId = Index * sizeof(Index);
			Offset_RundownProtect = PtrSub(Offset_UniqueProcessId, sizeof(((ML_EPROCESS_DYNAMIC *)0)->RundownProtect));

			return STATUS_SUCCESS;
		}
	};

	DECL_SELECTANY ULONG_PTR ML_EPROCESS_DYNAMIC::Offset_ProcessLock = 0;
	DECL_SELECTANY ULONG_PTR ML_EPROCESS_DYNAMIC::Offset_RundownProtect = 0;
	DECL_SELECTANY ULONG_PTR ML_EPROCESS_DYNAMIC::Offset_UniqueProcessId = 0;



	class ML_ETHREAD_DYNAMIC
	{
	protected:
		PETHREAD Thread;

		static ULONG_PTR Offset_Teb;
		static ULONG_PTR Offset_CreateTime;

	public:
		ML_ETHREAD_DYNAMIC(PETHREAD Thread = nullptr)
		{
			(*this) = Thread;
		}

		ML_ETHREAD_DYNAMIC& operator=(PETHREAD Thread)
		{
			this->Thread = Thread;
			return *this;
		}

		operator PETHREAD()
		{
			return this->Thread;
		}

		PROPERTY(PVOID, Teb);

		GET(Teb)
		{
			return *(PVOID *)PtrAdd(this->Thread, Offset_Teb);
		}

		SET(Teb)
		{
			*(PVOID *)PtrAdd(this->Thread, Offset_Teb) = value;
		}

		PROPERTY(LARGE_INTEGER&, CreateTime);

		GET(CreateTime)
		{
			return *(PLARGE_INTEGER)PtrAdd(this->Thread, Offset_CreateTime);
		}

		SET(CreateTime)
		{
			((PLARGE_INTEGER)PtrAdd(this->Thread, Offset_CreateTime))->QuadPart = value.QuadPart;
		}

		ML_ETHREAD_DYNAMIC* operator->()
		{
			return this;
		}

		NoInline static NTSTATUS InitializeOffset()
		{
			NTSTATUS            Status;
			ULONG_PTR           Index, OffsetTable[0x1000 / sizeof(ULONG_PTR)];
			PULONG_PTR          Buffer;
			KERNEL_USER_TIMES   Times;
			HANDLE              Handle;
			PLARGE_INTEGER      Begin, End;

			Index = 0;
			FOR_EACH_ARRAY(Buffer, OffsetTable)
			{
				*Buffer = Index++;
			}

			Index = (ULONG_PTR)PsGetThreadTeb((PETHREAD)OffsetTable);
			Offset_Teb = Index * sizeof(Index);

			Status = ThreadIdToHandleEx(&Handle, (ULONG_PTR)PsGetCurrentThreadId());
			FAIL_RETURN(Status);

			Status = ZwQueryInformationThread(Handle, ThreadTimes, &Times, sizeof(Times), nullptr);
			ZwClose(Handle);
			FAIL_RETURN(Status);

			Begin = (PLARGE_INTEGER)PsGetCurrentThread();
			End = PtrAdd(Begin, 0x1000);
			for (; Begin < End; ++Begin)
			{
				if (Begin->QuadPart == Times.CreateTime.QuadPart)
				{
					Offset_CreateTime = PtrOffset(Begin, PsGetCurrentThread());
					return STATUS_SUCCESS;
				}
			}

			return STATUS_NOT_FOUND;
		}
	};

	DECL_SELECTANY ULONG_PTR ML_ETHREAD_DYNAMIC::Offset_Teb = 0;
	DECL_SELECTANY ULONG_PTR ML_ETHREAD_DYNAMIC::Offset_CreateTime = 0;


	class ML_OBJECT_HEADER_DYNAMIC
	{
	protected:
		PVOID Object;

		static ULONG_PTR Offset_TypeIndex;

	public:
		ML_OBJECT_HEADER_DYNAMIC(PVOID Object = nullptr)
		{
			(*this) = Object;
		}

		ML_OBJECT_HEADER_DYNAMIC* operator->()
		{
			return this;
		}

		ML_OBJECT_HEADER_DYNAMIC& operator=(PVOID Object)
		{
			this->Object = Object;
			return *this;
		}

		PROPERTY(UCHAR, TypeIndex);

		GET(TypeIndex)
		{
			return *(PUCHAR)PtrAdd(this->Object, Offset_TypeIndex);
		}

		SET(TypeIndex)
		{
			_InterlockedExchange8((PCHAR)PtrAdd(this->Object, Offset_TypeIndex), value);
		}

		NoInline static NTSTATUS InitializeOffset()
		{
			BYTE            TypeIndexTable[0x60];
			PBYTE           Buffer, TypeIndexTableEnd;
			ULONG_PTR       TypeIndex;
			ML_OBJECT_TYPE  TypeObjectType;

			ZeroMemory(TypeIndexTable, sizeof(TypeIndexTable));

			TypeIndexTableEnd = &TypeIndexTable[countof(TypeIndexTable)];

			TypeObjectType = ObGetObjectType(*TmResourceManagerObjectType);
			TypeIndex = TypeObjectType->Index;

			for (Buffer = TypeIndexTableEnd - 1; Buffer != TypeIndexTable; --Buffer)
			{
				*Buffer = TypeIndex;
				if (ObGetObjectType(TypeIndexTableEnd) == TypeObjectType)
				{
					Offset_TypeIndex = PtrOffset(Buffer, TypeIndexTableEnd);
					return STATUS_SUCCESS;
				}
				*Buffer = 0;
			}

			return STATUS_UNSUCCESSFUL;
		}
	};

	DECL_SELECTANY ULONG_PTR ML_OBJECT_HEADER_DYNAMIC::Offset_TypeIndex = 0;


#pragma pop_macro("DEFINE_PROPERTY")
#pragma pop_macro("DECLARE_ACCESSOR")
#pragma pop_macro("FIELD_ACCESSOR")
#pragma pop_macro("SET_ACCESSOR")
#pragma pop_macro("CALL_ACCESSOR")

	class StructAccessorManager
	{
	public:

		NoInline static NTSTATUS Initialize()
		{
			FAIL_RETURN(ML_OBJECT_HEADER_DYNAMIC::InitializeOffset());
			FAIL_RETURN(ML_EPROCESS_DYNAMIC::InitializeOffset());
			FAIL_RETURN(ML_ETHREAD_DYNAMIC::InitializeOffset());

			ML_OBJECT_TYPE ObjectType;

			if (ObjectType->Success == FALSE)
				return STATUS_NOT_SUPPORTED;

			ML_OBJECT_TYPE_INITIALIZER_DATA ObjectTypeInitializer;

			if (ObjectTypeInitializer->Success == FALSE)
				return STATUS_NOT_SUPPORTED;

			return STATUS_SUCCESS;
		}
	};

#endif // r0


