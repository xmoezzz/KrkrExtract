#pragma once

#include <ntstatus.h>
#include <Windows.h>
#include "APIHash.h"

#ifndef CPP_DEFINED
#if defined(__cplusplus)
#define CPP_DEFINED 1
#else
#define CPP_DEFINED 0
#endif
#endif

#ifndef FASTCALL
#define FASTCALL __fastcall
#endif

#define ML_MAX(a, b) ((a) < (b) ? (b) : (a))
#define ML_MIN(a, b) ((a) < (b) ? (a) : (b))

#define MY_MAX ML_MAX
#define MY_MIN ML_MIN

#if defined(__INTEL_COMPILER)
#define ML_COMPILER_INTEL __INTEL_COMPILER
#define ML_COMPILER_MSC 1700
#define MY_COMPILER_MSC ML_COMPILER_MSC

#elif defined(_MSC_VER)
#define ML_COMPILER_MSC _MSC_VER
#define MY_COMPILER_MSC ML_COMPILER_MSC

#endif

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

#define CHAR_UPPER(ch) (IN_RANGE('a', (ch), 'z') ? ((ch) & (Byte)0xDF) : ch)
#define CHAR_LOWER(ch) (IN_RANGE('A', (ch), 'Z') ? ((ch) | (Byte)~0xDF) : ch)

#define _CHAR_UPPER4W(ch) (UINT64)((ch) & 0xFFDFFFDFFFDFFFDF)
#define CHAR_UPPER4W(ch) _CHAR_UPPER4W((UINT64)(ch))
#define CHAR_UPPER3W(ch) (UINT64)(CHAR_UPPER4W(ch) & 0x0000FFFFFFFFFFFF)
#define CHAR_UPPER2W(ch) (UINT64)(CHAR_UPPER4W(ch) & 0x00000000FFFFFFFF)
#define CHAR_UPPER1W(ch) (UINT64)(CHAR_UPPER4W(ch) & 0x000000000000FFFF)

#define _CHAR_UPPER4(ch) (UINT32)((ch) & 0xDFDFDFDF)
#define CHAR_UPPER4(ch) (UINT32)_CHAR_UPPER4((UINT32)(ch))
#define CHAR_UPPER3(ch) (UINT32)(CHAR_UPPER4(ch) & 0x00FFFFFF)
#define CHAR_UPPER2(ch) (UINT32)(CHAR_UPPER4(ch) & 0x0000FFFF)
#define CHAR_UPPER1(ch) (UINT32)(CHAR_UPPER4(ch) & 0x000000FF)
#define CHAR_UPPER8(ch) ((UINT64)(ch) & 0xDFDFDFDFDFDFDFDF)

#define _TAG2(s) ((((s) << 8) | ((s) >> 8)) & 0xFFFF)
#define TAG2(s) _TAG2((USHORT)(s))

#define _TAG3(s) ( \
                (((s) >> 16) & 0xFF)       | \
                (((s)        & 0xFF00))    | \
                (((s) << 16) & 0x00FF0000) \
                )
#define TAG3(s) _TAG3((DWORD)(s))

#define _TAG4(s) ( \
                (((s) >> 24) & 0xFF)       | \
                (((s) >> 8 ) & 0xFF00)     | \
                (((s) << 24) & 0xFF000000) | \
                (((s) << 8 ) & 0x00FF0000) \
                )
#define TAG4(s) _TAG4((DWORD)(s))

#define TAG8(left, right) (((UINT64)TAG4(right) << 32) | TAG4(left))

#define _TAG2W(x) (((x) & 0xFF) << 16 | ((x) & 0xFF00) >> 8)
#define TAG2W(x) (UINT32)_TAG2W((UINT32)(x))

#define _TAG3W(x) (TAG4W(x) >> 16)
#define TAG3W(x) (UINT64)_TAG3W((UINT64)(x))

#define _TAG4W(x) (((UINT64)TAG2W((x) & 0xFFFF) << 32) | ((UINT64)TAG2W((x) >> 16)))
#define TAG4W(x) (UINT64)_TAG4W((UINT64)(x))

#ifndef ForceInline
#if (MY_COMPILER_MSC >= 1200)
#define ForceInline __forceinline
#else
#define ForceInline __inline
#endif
#endif

#ifndef EXTC
#if CPP_DEFINED
#define EXTC extern "C"
#define EXTC_IMPORT EXTC __declspec(dllimport)
#define EXTC_EXPORT EXTC DECL_EXPORT
#else
#define EXTC
#define EXTC_IMPORT __declspec(dllimport)
#endif
#endif

#define NATIVE_API  EXTC_IMPORT
#define NTKRNLAPI   EXTC_IMPORT

typedef struct _STRING
{
	USHORT  Length;
	USHORT  MaximumLength;
	PCHAR   Buffer;
} STRING;

typedef STRING *PSTRING;
typedef STRING ANSI_STRING;
typedef PSTRING PANSI_STRING;

typedef struct _UNICODE_STRING {
	USHORT  Length;
	USHORT  MaximumLength;
	PWSTR   Buffer;
} UNICODE_STRING;

typedef struct _LARGE_UNICODE_STRING
{
	ULONG Length;
	ULONG MaximumLength : 31;
	ULONG Ansi : 1;

	union
	{
		PWSTR   UnicodeBuffer;
		PSTR    AnsiBuffer;
		ULONG64 Buffer;
	};

} LARGE_UNICODE_STRING, *PLARGE_UNICODE_STRING;

#pragma pack(push, 8)

typedef struct
{
	USHORT Length;
	USHORT MaximumLength;
	union
	{
		PWSTR  Buffer;
		ULONG64 Dummy;
	};

} ANSI_STRING3264, *PANSI_STRING3264;

typedef struct
{
	USHORT Length;
	USHORT MaximumLength;
	union
	{
		PWSTR  Buffer;
		ULONG64 Dummy;
	};

} UNICODE_STRING3264, *PUNICODE_STRING3264;

typedef UNICODE_STRING3264 UNICODE_STRING64;
typedef PUNICODE_STRING3264 PUNICODE_STRING64;

#pragma pack(pop)

typedef UNICODE_STRING *PUNICODE_STRING;
typedef const UNICODE_STRING *PCUNICODE_STRING;

#define SET_FLAG(_V, _F)    ((_V) |= (_F))
#define CLEAR_FLAG(_V, _F)  ((_V) &= ~(_F))
#define FLAG_ON(_V, _F)     (!!((_V) & (_F)))
#define FLAG_OFF(_V, _F)     (!FLAG_ON(_V, _F))

#ifndef EXTCPP
#if CPP_DEFINED
#define EXTCPP extern "C++"
#else
#define EXTCPP
#endif
#endif /* EXTCPP */

#if !defined(_M_IA64)
#define MEMORY_PAGE_SIZE (4 * 1024)
#else
#define MEMORY_PAGE_SIZE (8 * 1024)
#endif

#define LOOP_ALWAYS for (;;)
#define LOOP_FOREVER LOOP_ALWAYS
#define LOOP_ONCE   for (BOOL __condition_ = TRUE; __condition_; __condition_ = FALSE)

#define FIELD_BASE(address, type, field) (type *)((ULONG_PTR)address - (ULONG_PTR)&((type *)0)->field)
#define FIELD_TYPE(_Type, _Field)  TYPE_OF(((_Type*)0)->_Field)

#ifndef FIELD_SIZE
#define FIELD_SIZE(type, field) (sizeof(((type *)0)->field))
#endif // FIELD_SIZE

#if !defined(BREAK_IF)
#define BREAK_IF(c) if (c) break;
#endif /* BREAK_IF */

#if !defined(CONTINUE_IF)
#define CONTINUE_IF(c) if (c) continue;
#endif /* CONTINUE_IF */

#if !defined(RETURN_IF)
#define RETURN_IF(c, r) if (c) return r
#endif /* RETURN_IF */

#ifndef ASM
#define ASM __declspec(naked)
#endif /* ASM */

#ifndef NAKED
#define NAKED __declspec(naked)
#endif /* ASM */

#if !defined(INLINE_ASM)
#define INLINE_ASM __asm
#endif /* INLINE_ASM */

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


#ifndef NT_SUCCESS
FORCEINLINE BOOL IsStatusSuccess(NTSTATUS Status) { return (Status & Status) >= 0; }
#define NT_SUCCESS(Status)  IsStatusSuccess(Status)
#endif

#define LDRP_STATIC_LINK                0x00000002
#define LDRP_IMAGE_DLL                  0x00000004
#define LDRP_LOAD_IN_PROGRESS           0x00001000
#define LDRP_UNLOAD_IN_PROGRESS         0x00002000
#define LDRP_ENTRY_PROCESSED            0x00004000
#define LDRP_ENTRY_INSERTED             0x00008000
#define LDRP_CURRENT_LOAD               0x00010000
#define LDRP_FAILED_BUILTIN_LOAD        0x00020000
#define LDRP_DONT_CALL_FOR_THREADS      0x00040000
#define LDRP_PROCESS_ATTACH_CALLED      0x00080000
#define LDRP_DEBUG_SYMBOLS_LOADED       0x00100000
#define LDRP_IMAGE_NOT_AT_BASE          0x00200000
#define LDRP_COR_IMAGE                  0x00400000
#define LDRP_COR_OWNS_UNMAP             0x00800000
#define LDRP_SYSTEM_MAPPED              0x01000000
#define LDRP_IMAGE_VERIFYING            0x02000000
#define LDRP_DRIVER_DEPENDENT_DLL       0x04000000
#define LDRP_ENTRY_NATIVE               0x08000000
#define LDRP_REDIRECTED                 0x10000000
#define LDRP_NON_PAGED_DEBUG_INFO       0x20000000
#define LDRP_MM_LOADED                  0x40000000
#define LDRP_COMPAT_DATABASE_PROCESSED  0x80000000

typedef struct _LDR_DATA_TABLE_ENTRY
{
	/* +0x000 */ LIST_ENTRY     InLoadOrderLinks;
	/* +0x008 */ LIST_ENTRY     InMemoryOrderLinks;
	/* +0x010 */ LIST_ENTRY     InInitializationOrderLinks;
	/* +0x018 */ PVOID          DllBase;
	/* +0x01c */ PVOID          EntryPoint;
	/* +0x020 */ ULONG          SizeOfImage;
	/* +0x024 */ UNICODE_STRING FullDllName;
	/* +0x02c */ UNICODE_STRING BaseDllName;
	/* +0x034 */ ULONG          Flags;
	/* +0x038 */ USHORT         LoadCount;
	/* +0x03a */ USHORT         TlsIndex;

	union
	{
		/* +0x03c */    LIST_ENTRY     HashLinks;
		struct
		{
			/* +0x03c */        PVOID          SectionPointer;
			/* +0x040 */        ULONG          CheckSum;
		};
	};

	union
	{
		/* +0x044 */    ULONG          TimeDateStamp;
		/* +0x044 */    PVOID          LoadedImports;
	};

	/* +0x048 */ PVOID          EntryPointActivationContext;
	/* +0x04c */ PVOID          PatchInformation;
	/* +0x050 */ LIST_ENTRY     ForwarderLinks;
	/* +0x058 */ LIST_ENTRY     ServiceTagLinks;
	/* +0x060 */ LIST_ENTRY     StaticLinks;
	/* +0x068 */ PVOID          ContextInformation;
	/* +0x06c */ ULONG          OriginalBase;
	/* +0x070 */ LARGE_INTEGER  LoadTime;

#if CPP_DEFINED

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

#endif // cpp

} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;

typedef const LDR_DATA_TABLE_ENTRY* PCLDR_DATA_TABLE_ENTRY;

typedef LDR_DATA_TABLE_ENTRY LDR_MODULE;
typedef PLDR_DATA_TABLE_ENTRY PLDR_MODULE;
typedef PCLDR_DATA_TABLE_ENTRY PCLDR_MODULE;

typedef struct
{
	UNICODE_STRING  WindowsDirectory;
	UNICODE_STRING  SystemDirectory;
	UNICODE_STRING  BaseNamedObjects;
	ULONG           Unknown;
	USHORT          Unknown2;
	USHORT          ServicePackLength;
	USHORT          ServicePackMaximumLength;
	WCHAR           ServicePack[1];

} *PSTATIC_SERVER_DATA;

typedef struct
{
	PVOID               Reserve;
	PSTATIC_SERVER_DATA StaticServerData;

} *STATIC_SERVER_DATA_PTR;

typedef struct
{
	ULONG       Length;                             // +0x00
	BOOL        Initialized;                        // +0x04
	PVOID       SsHandle;                           // +0x08
	LIST_ENTRY  InLoadOrderModuleList;              // +0x0c
	LIST_ENTRY  InMemoryOrderModuleList;            // +0x14
	LIST_ENTRY  InInitializationOrderModuleList;    // +0x1c
	PVOID       EntryInProgress;                    // +0x24
	ULONG       ShutdownInProgress;                 // +0x28
	ULONG_PTR   ShutdownThreadId;                   // +0x2C
} PEB_LDR_DATA, *PPEB_LDR_DATA;                     // +0x30

typedef struct
{
	/* 0x000 */ USHORT      Flags;
	/* 0x002 */ USHORT      Length;
	/* 0x004 */ ULONG       TimeStamp;
	/* 0x008 */ ANSI_STRING DosPath;
} RTL_DRIVE_LETTER_CURDIR;

typedef struct
{
	UNICODE_STRING  DosPath;
	HANDLE          Handle;
} CURDIR;

#define USER_PROCESS_PARAMETERS_NORMALIZED  (1u << 0)

typedef struct
{
	/* 0x000 */ ULONG                   MaximumLength;
	/* 0x004 */ ULONG                   Length;
	/* 0x008 */ ULONG                   Flags;
	/* 0x00c */ ULONG                   DebugFlags;
	/* 0x010 */ HANDLE                  ConsoleHandle;
	/* 0x014 */ ULONG                   ConsoleFlags;
	/* 0x018 */ HANDLE                  StandardInput;
	/* 0x01c */ HANDLE                  StandardOutput;
	/* 0x020 */ HANDLE                  StandardError;
	/* 0x024 */ CURDIR                  CurrentDirectory;
	/* 0x030 */ UNICODE_STRING          DllPath;
	/* 0x038 */ UNICODE_STRING          ImagePathName;
	/* 0x040 */ UNICODE_STRING          CommandLine;
	/* 0x048 */ PWCHAR                  Environment;
	/* 0x04c */ ULONG                   StartingX;
	/* 0x050 */ ULONG                   StartingY;
	/* 0x054 */ ULONG                   CountX;
	/* 0x058 */ ULONG                   CountY;
	/* 0x05c */ ULONG                   CountCharsX;
	/* 0x060 */ ULONG                   CountCharsY;
	/* 0x064 */ ULONG                   FillAttribute;
	/* 0x068 */ ULONG                   WindowFlags;
	/* 0x06c */ ULONG                   ShowWindowFlags;
	/* 0x070 */ UNICODE_STRING          WindowTitle;
	/* 0x078 */ UNICODE_STRING          DesktopInfo;
	/* 0x080 */ UNICODE_STRING          ShellInfo;
	/* 0x088 */ UNICODE_STRING          RuntimeData;
	/* 0x090 */ RTL_DRIVE_LETTER_CURDIR CurrentDirectores[32];
	/* 0x290 */ ULONG_PTR               EnvironmentSize;
	/* 0x294 */ ULONG_PTR               EnvironmentVersion;
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

typedef struct PEB_BASE
{
	/* 0x000 */ UCHAR                           InheritedAddressSpace;
	/* 0x001 */ UCHAR                           ReadImageFileExecOptions;
	/* 0x002 */ UCHAR                           BeingDebugged;
	/* 0x003 */ struct
	{
		UCHAR                       ImageUsesLargePages : 1;
		UCHAR                       IsProtectedProcess : 1;
		UCHAR                       IsLegacyProcess : 1;
		UCHAR                       IsImageDynamicallyRelocated : 1;
		UCHAR                       SkipPatchingUser32Forwarders : 1;
		UCHAR                       SpareBits : 3;
	};
	/* 0x004 */ PVOID                           Mutant;
	/* 0x008 */ PVOID                           ImageBaseAddress;
	/* 0x00c */ PPEB_LDR_DATA                   Ldr;
	/* 0x010 */ PRTL_USER_PROCESS_PARAMETERS    ProcessParameters;
	/* 0x014 */ PVOID                           SubSystemData;
	/* 0x018 */ HANDLE                          ProcessHeap;
	/* 0x01c */ PRTL_CRITICAL_SECTION           FastPebLock;
	/* 0x020 */ PVOID                           AtlThunkSListPtr;
	/* 0x024 */ PVOID                           IFEOKey;
	/* 0x028 */ union
	{
		ULONG_PTR CrossProcessFlags;
		struct
		{
			UCHAR ProcessInJob : 1;
			UCHAR ProcessInitializing : 1;
			UCHAR ProcessUsingVEH : 1;
			UCHAR ProcessUsingVCH : 1;
			UCHAR ProcessUsingFTH : 1;
		};
	};

	/* 0x02C */ union
	{
		PVOID                       KernelCallbackTable;
		PVOID                       UserSharedInfoPtr;
	};

	/* 0x030 */ ULONG                           SystemReserved[1];
	/* 0x034 */ ULONG                           AtlThunkSListPtr32;
	/* 0x038 */ PVOID                           ApiSetMap;
	/* 0x03c */ ULONG                           TlsExpansionCounter;
	/* 0x040 */ PVOID                           TlsBitmap;
	/* 0x044 */ ULONG                           TlsBitmapBits[2];
	/* 0x04c */ PVOID                           ReadOnlySharedMemoryBase;
	/* 0x050 */ PVOID                           HotpatchInformation;
	/* 0x054 */ STATIC_SERVER_DATA_PTR          ReadOnlyStaticServerData;
	/* 0x058 */ PUSHORT                         AnsiCodePageData;
	/* 0x05c */ PUSHORT                         OemCodePageData;
	/* 0x060 */ PVOID                           UnicodeCaseTableData;
	/* 0x064 */ ULONG                           NumberOfProcessors;
	/* 0x068 */ ULONG                           NtGlobalFlag;
	/* 0x06C */ ULONG                           Dummy;
	/* 0x070 */ LARGE_INTEGER                   CriticalSectionTimeout;
	/* 0x078 */ ULONG                           HeapSegmentReserve;
	/* 0x07c */ ULONG                           HeapSegmentCommit;
	/* 0x080 */ ULONG                           HeapDeCommitTotalFreeThreshold;
	/* 0x084 */ ULONG                           HeapDeCommitFreeBlockThreshold;
	/* 0x088 */ ULONG                           NumberOfHeaps;
	/* 0x08c */ ULONG                           MaximumNumberOfHeaps;
	/* 0x090 */ PVOID                           ProcessHeaps;
	/* 0x094 */ PVOID                           GdiSharedHandleTable;
	/* 0x098 */ PVOID                           ProcessStarterHelper;
	/* 0x09c */ ULONG                           GdiDCAttributeList;
	/* 0x0a0 */ PRTL_CRITICAL_SECTION           LoaderLock;
	/* 0x0a4 */ ULONG                           OSMajorVersion;
	/* 0x0a8 */ ULONG                           OSMinorVersion;
	/* 0x0ac */ USHORT                          OSBuildNumber;
	/* 0x0ae */ USHORT                          OSCSDVersion;
	/* 0x0b0 */ ULONG                           OSPlatformId;
} PEB_BASE, *PPEB_BASE;

typedef struct _CLIENT_ID
{
	HANDLE  UniqueProcess;
	HANDLE  UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

typedef struct TEB_BASE
{
	/* 0x000 */ NT_TIB      NtTib;
	/* 0x01C */ PVOID       EnvironmentPointer;
	/* 0x020 */ CLIENT_ID   ClientId;
	/* 0x028 */ HANDLE      ActiveRpcHandle;
	/* 0x02C */ PVOID       ThreadLocalStoragePointer;
	/* 0x030 */ PPEB_BASE   ProcessEnvironmentBlock;
	/* 0x034 */ ULONG       LastErrorValue;
	/* 0x038 */ ULONG       CountOfOwnedCriticalSections;
	/* 0x03C */ PVOID       CsrClientThread;
	/* 0x040 */ PVOID       Win32ThreadInfo;
	/* 0x044 */ ULONG       User32Reserved[26];
	/* 0x0AC */ ULONG       UserReserved[5];
	/* 0x0C0 */ PVOID       WOW32Reserved;
	/* 0x0C4 */ ULONG       CurrentLocale;
	/* 0x0C8 */ ULONG       FpSoftwareStatusRegister;
	/* 0x0CC */ PVOID       SystemReserved1[54];
	/* 0x1A4 */ LONG        ExceptionCode;

} TEB_BASE, *PTEB_BASE;

inline ULONG_PTR ReadFsPtr(ULONG_PTR Offset)
{
#if ML_AMD64
	return (ULONG_PTR)__readgsqword((ULONG)Offset);
#else // x86
	return (ULONG_PTR)__readfsdword(Offset);
#endif
}

#define TEB_OFFSET FIELD_OFFSET(TEB_BASE, NtTib.Self)
#define PEB_OFFSET FIELD_OFFSET(TEB_BASE, ProcessEnvironmentBlock)

ForceInline
PPEB_BASE Nt_CurrentPeb()
{
	return (PPEB_BASE)(ULONG_PTR)ReadFsPtr(PEB_OFFSET);
}

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

NATIVE_API
NTSTATUS
NTAPI
NtGetContextThread(
IN  HANDLE ThreadHandle,
OUT PCONTEXT Context
);

NATIVE_API
NTSTATUS
NTAPI
NtWriteVirtualMemory(
IN    HANDLE  ProcessHandle,
IN    PVOID   BaseAddress,
IN    PVOID   Buffer,
IN    SIZE_T  NumberOfBytesToWrite,
OUT   PSIZE_T NumberOfBytesWritten OPTIONAL
);

NATIVE_API
NTSTATUS
NTAPI
NtAllocateVirtualMemory(
IN      HANDLE      ProcessHandle,
IN OUT  PVOID*      BaseAddress,
IN      ULONG_PTR   ZeroBits,
IN OUT  PSIZE_T     RegionSize,
IN      ULONG       AllocationType,
IN      ULONG       Protect
);

NATIVE_API
NTSTATUS
NTAPI
NtFlushInstructionCache(
IN HANDLE ProcessHandle,
IN PVOID  BaseAddress,
IN SIZE_T NumberOfBytesToFlush
);

NATIVE_API
NTSTATUS
NTAPI
NtResumeThread(
IN  HANDLE ThreadHandle,
OUT PULONG PreviousSuspendCount OPTIONAL
);

NATIVE_API
NTSTATUS
NTAPI
NtReadVirtualMemory(
IN    HANDLE  ProcessHandle,
IN    PVOID   BaseAddress,
OUT   PVOID   Buffer,
IN    SIZE_T  NumberOfBytesToRead,
OUT   PSIZE_T NumberOfBytesRead OPTIONAL
);


NATIVE_API
NTSTATUS
NTAPI
NtSetContextThread(
IN HANDLE ThreadHandle,
IN PCONTEXT Context
);

NATIVE_API
NTSTATUS
NTAPI
NtDelayExecution(
IN BOOLEAN          Alertable,
IN PLARGE_INTEGER   Interval
);

NATIVE_API
NTSTATUS
NTAPI
NtFreeVirtualMemory(
IN      HANDLE  ProcessHandle,
IN OUT  PVOID  *BaseAddress,
IN OUT  PSIZE_T RegionSize,
IN      ULONG   FreeType
);

NATIVE_API
VOID
NTAPI
RtlInitUnicodeString(
IN OUT  PUNICODE_STRING DestinationString,
IN      LPCWSTR         SourceString
);

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

EXTCPP
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

EXTCPP
template<PointerOperationType::PointerOperationClass OP, class PtrType1>
ForceInline
PtrType1 PtrOperator(PtrType1 Ptr1, unsigned short Ptr2)
{
	return PtrOperator<OP>(Ptr1, (ULONG_PTR)Ptr2);
}

EXTCPP
template<PointerOperationType::PointerOperationClass OP, class PtrType1>
ForceInline
PtrType1 PtrOperator(PtrType1 Ptr1, short Ptr2)
{
	return PtrOperator<OP>(Ptr1, (ULONG_PTR)Ptr2);
}

EXTCPP
template<PointerOperationType::PointerOperationClass OP, class PtrType1>
ForceInline
PtrType1 PtrOperator(PtrType1 Ptr1, unsigned char Ptr2)
{
	return PtrOperator<OP>(Ptr1, (ULONG_PTR)Ptr2);
}

EXTCPP
template<PointerOperationType::PointerOperationClass OP, class PtrType1>
ForceInline
PtrType1 PtrOperator(PtrType1 Ptr1, char Ptr2)
{
	return PtrOperator<OP>(Ptr1, (ULONG_PTR)Ptr2);
}

EXTCPP
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

EXTCPP
template<class PtrType1, class PtrType2>
ForceInline
PtrType1 PtrAdd(PtrType1 Ptr, PtrType2 Offset)
{
	return PtrOperator<PointerOperationType::POINTER_OP_ADD>(Ptr, Offset);
}

EXTCPP
template<class PtrType1, class PtrType2>
ForceInline
PtrType1 PtrSub(PtrType1 Ptr, PtrType2 Offset)
{
	return PtrOperator<PointerOperationType::POINTER_OP_SUB>(Ptr, Offset);
}

EXTCPP
template<class PtrType1, class PtrType2>
ForceInline
PtrType1 PtrAnd(PtrType1 Ptr, PtrType2 Offset)
{
	return PtrOperator<PointerOperationType::POINTER_OP_AND>(Ptr, Offset);
}

EXTCPP
template<class PtrType1, class PtrType2>
ForceInline
PtrType1 PtrOr(PtrType1 Ptr, PtrType2 Offset)
{
	return PtrOperator<PointerOperationType::POINTER_OP_OR>(Ptr, Offset);
}

EXTCPP
template<class PtrType1, class PtrType2>
ForceInline
PtrType1 PtrXor(PtrType1 Ptr, PtrType2 Offset)
{
	return PtrOperator<PointerOperationType::POINTER_OP_XOR>(Ptr, Offset);
}

EXTCPP
template<class PtrType1, class PtrType2>
ForceInline
PtrType1 PtrMod(PtrType1 Ptr, PtrType2 Offset)
{
	return PtrOperator<PointerOperationType::POINTER_OP_MOD>(Ptr, Offset);
}

ForceInline ULONG HashAPI(PCSTR pszName)
{
	ULONG Hash = 0;

	while (*(PBYTE)pszName)
	{
		Hash = _rotl(Hash, 0x0D) ^ *(PBYTE)pszName++;
	}

	return Hash;
}

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
PVOID*          InjectBuffer = NULL,
ULONG_PTR       Timeout = 500
);
