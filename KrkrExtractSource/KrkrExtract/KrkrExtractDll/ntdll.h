#ifndef NTPROTOS_H
#define NTPROTOS_H

#pragma once

#define _NTDEF_

#ifndef WIN32_NO_STATUS
#include <ntstatus.h>
#define WIN32_NO_STATUS
__if_not_exists(NTSTATUS)
{
	typedef long NTSTATUS, *PNTSTATUS;
}
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

__if_not_exists(CLONG)
{
	typedef LONG CLONG;
}

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

#define ROUND_TO_PAGES(Size, PageSize)  (((ULONG_PTR)(Size) + PageSize - 1) & ~(PageSize - 1))
#ifndef RVA_TO_ADDR
#define RVA_TO_ADDR(Mapping,Rva) ((PVOID)(((ULONG_PTR) (Mapping)) + (Rva)))
#endif

typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING;
typedef UNICODE_STRING *PUNICODE_STRING;
typedef const UNICODE_STRING *PCUNICODE_STRING;

typedef struct _ANSI_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PSTR  Buffer;
} ANSI_STRING, STRING, *PSTRING;
typedef ANSI_STRING *PANSI_STRING;
typedef const ANSI_STRING *PCANSI_STRING;

typedef struct _OBJECT_ATTRIBUTES {
    ULONG Length;
    HANDLE RootDirectory;
    PUNICODE_STRING ObjectName;
    ULONG Attributes;
    PVOID SecurityDescriptor;
    PVOID SecurityQualityOfService;
} OBJECT_ATTRIBUTES;
typedef OBJECT_ATTRIBUTES *POBJECT_ATTRIBUTES;

#ifndef InitializeObjectAttributes
#define InitializeObjectAttributes( p, n, a, r, s ) { \
    (p)->Length = sizeof( OBJECT_ATTRIBUTES );          \
    (p)->RootDirectory = r;                             \
    (p)->Attributes = a;                                \
    (p)->ObjectName = n;                                \
    (p)->SecurityDescriptor = s;                        \
    (p)->SecurityQualityOfService = NULL;               \
    }
#endif

typedef struct _RTL_DRIVE_LETTER_CURDIR
{
	USHORT Flags;
	USHORT Length;
	ULONG TimeStamp;
	STRING DosPath;
} RTL_DRIVE_LETTER_CURDIR, *PRTL_DRIVE_LETTER_CURDIR;

typedef struct _CLIENT_ID {
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

typedef struct _CURDIR
{
	UNICODE_STRING DosPath;
	HANDLE Handle;
} CURDIR, *PCURDIR;

typedef struct _RTL_USER_PROCESS_PARAMETERS_XP
{
	ULONG MaximumLength;
	ULONG Length;
	ULONG Flags;
	ULONG DebugFlags;
	PVOID ConsoleHandle;
	ULONG ConsoleFlags;
	HANDLE StdInputHandle;
	HANDLE StdOutputHandle;
	HANDLE StdErrorHandle;
	UNICODE_STRING CurrentDirectoryPath;
	HANDLE CurrentDirectoryHandle;
	UNICODE_STRING DllPath;
	UNICODE_STRING ImagePathName;
	UNICODE_STRING CommandLine;
	PVOID Environment;
	ULONG StartingPositionLeft;
	ULONG StartingPositionTop;
	ULONG Width;
	ULONG Height;
	ULONG CharWidth;
	ULONG CharHeight;
	ULONG ConsoleTextAttributes;
	ULONG WindowFlags;
	ULONG ShowWindowFlags; 
	UNICODE_STRING WindowTitle;
	UNICODE_STRING DesktopName;
	UNICODE_STRING ShellInfo;
	UNICODE_STRING RuntimeData;
	RTL_DRIVE_LETTER_CURDIR DLCurrentDirectory[0x20];
} RTL_USER_PROCESS_PARAMETERS_XP, *PRTL_USER_PROCESS_PARAMETERS_XP, *PPROCESS_PARAMETERS_XP;

typedef struct _RTL_USER_PROCESS_PARAMETERS_VISTA
{
	ULONG MaximumLength;
	ULONG Length;
	ULONG Flags;
	ULONG DebugFlags;
	PVOID ConsoleHandle;
	ULONG ConsoleFlags;
	HANDLE StdInputHandle;
	HANDLE StdOutputHandle;
	HANDLE StdErrorHandle;
	CURDIR CurrentDirectoryPath;
	UNICODE_STRING DllPath;
	UNICODE_STRING ImagePathName;
	UNICODE_STRING CommandLine;
	PVOID Environment;
	ULONG StartingPositionLeft;
	ULONG StartingPositionTop;
	ULONG Width;
	ULONG Height;
	ULONG CharWidth;
	ULONG CharHeight;
	ULONG ConsoleTextAttributes;
	ULONG WindowFlags;
	ULONG ShowWindowFlags; 
	UNICODE_STRING WindowTitle;
	UNICODE_STRING DesktopName;
	UNICODE_STRING ShellInfo;
	UNICODE_STRING RuntimeData;
	RTL_DRIVE_LETTER_CURDIR DLCurrentDirectory[0x20];
	volatile ULONG_PTR EnvironmentSize;
} RTL_USER_PROCESS_PARAMETERS_VISTA, *PRTL_USER_PROCESS_PARAMETERS_VISTA, *PPROCESS_PARAMETERS_VISTA;

typedef struct _RTL_USER_PROCESS_PARAMETERS_7
{
	ULONG MaximumLength;
	ULONG Length;
	ULONG Flags;
	ULONG DebugFlags;
	PVOID ConsoleHandle;
	ULONG ConsoleFlags;
	HANDLE StdInputHandle;
	HANDLE StdOutputHandle;
	HANDLE StdErrorHandle;
	CURDIR CurrentDirectoryPath;
	UNICODE_STRING DllPath;
	UNICODE_STRING ImagePathName;
	UNICODE_STRING CommandLine;
	PVOID Environment;
	ULONG StartingPositionLeft;
	ULONG StartingPositionTop;
	ULONG Width;
	ULONG Height;
	ULONG CharWidth;
	ULONG CharHeight;
	ULONG ConsoleTextAttributes;
	ULONG WindowFlags;
	ULONG ShowWindowFlags; 
	UNICODE_STRING WindowTitle;
	UNICODE_STRING DesktopName;
	UNICODE_STRING ShellInfo;
	UNICODE_STRING RuntimeData;
	RTL_DRIVE_LETTER_CURDIR DLCurrentDirectory[0x20];
	volatile ULONG_PTR EnvironmentSize;
	volatile ULONG_PTR EnvironmentVersion;
} RTL_USER_PROCESS_PARAMETERS_7, *PRTL_USER_PROCESS_PARAMETERS_7, *PPROCESS_PARAMETERS_7;

typedef struct _INITIAL_TEB {
	PVOID FixedStackBase;
	PVOID FixedStackLimit;
	PVOID StackBase;
	PVOID StackLimit;
	PVOID AllocatedStackBase;
} INITIAL_TEB, *PINITIAL_TEB;


typedef struct _PEB_LDR_DATA_XP {
	ULONG Length;
	BOOLEAN Initialized;
	PVOID SsHandle;
	LIST_ENTRY InLoadOrderModuleList;
	LIST_ENTRY InMemoryOrderModuleList;
	LIST_ENTRY InInitializationOrderModuleList;
	PVOID EntryInProgress;
} PEB_LDR_DATA_XP, *PPEB_LDR_DATA_XP;

typedef struct _PEB_LDR_DATA_VISTA_7 {
	ULONG Length;
	BOOLEAN Initialized;
	PVOID SsHandle;
	LIST_ENTRY InLoadOrderModuleList;
	LIST_ENTRY InMemoryOrderModuleList;
	LIST_ENTRY InInitializationOrderModuleList;
	PVOID EntryInProgress;
	BOOLEAN ShutdownInProgress;
	PVOID ShutdownThread;
} PEB_LDR_DATA_VISTA_7, *PPEB_LDR_DATA_VISTA_7;

typedef struct _LDR_DATA_TABLE_ENTRY_XP {
    LIST_ENTRY InLoadOrderModuleList;
	LIST_ENTRY InMemoryOrderModuleList;
	LIST_ENTRY InInitializationOrderModuleList;
	PVOID BaseAddress;
	PVOID EntryPoint;
	ULONG SizeOfImage;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
	ULONG Flags;
	SHORT LoadCount;
	SHORT TlsIndex;
	union
	{
		LIST_ENTRY HashLinks;
		PVOID SectionPointer;
	};
	ULONG Checksum;
	union
	{
		ULONG TimeDataStamp;
		PVOID LoadedImports;
	};
	PVOID EntryPointActivationContext;
	PVOID PatchInformation;
} LDR_DATA_TABLE_ENTRY_XP, *PLDR_DATA_TABLE_ENTRY_XP, LDR_MODULE_XP, *PLDR_MODULE_XP;

typedef struct _LDR_DATA_TABLE_ENTRY_VISTA {
    LIST_ENTRY InLoadOrderModuleList;
	LIST_ENTRY InMemoryOrderModuleList;
	LIST_ENTRY InInitializationOrderModuleList;
	PVOID BaseAddress;
	PVOID EntryPoint;
	ULONG SizeOfImage;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
	ULONG Flags;
	SHORT LoadCount;
	SHORT TlsIndex;
	union
	{
		LIST_ENTRY HashLinks;
		PVOID SectionPointer;
	};
	ULONG Checksum;
	union
	{
		ULONG TimeDataStamp;
		PVOID LoadedImports;
	};
	PVOID EntryPointActivationContext;
	PVOID PatchInformation;
	LIST_ENTRY ForwarderLinks;
	LIST_ENTRY ServiceTagLinks;
	LIST_ENTRY StaticLinks;
} LDR_DATA_TABLE_ENTRY_VISTA, *PLDR_DATA_TABLE_ENTRY_VISTA, LDR_MODULE_VISTA, *PLDR_MODULE_VISTA;

typedef struct _LDR_DATA_TABLE_ENTRY_7 {
    LIST_ENTRY InLoadOrderModuleList;
	LIST_ENTRY InMemoryOrderModuleList;
	LIST_ENTRY InInitializationOrderModuleList;
	PVOID BaseAddress;
	PVOID EntryPoint;
	ULONG SizeOfImage;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
	ULONG Flags;
	SHORT LoadCount;
	SHORT TlsIndex;
	union
	{
		LIST_ENTRY HashLinks;
		PVOID SectionPointer;
	};
	ULONG Checksum;
	union
	{
		ULONG TimeDataStamp;
		PVOID LoadedImports;
	};
	PVOID EntryPointActivationContext;
	PVOID PatchInformation;
	LIST_ENTRY ForwarderLinks;
	LIST_ENTRY ServiceTagLinks;
	LIST_ENTRY StaticLinks;
	PVOID ContextInformation;
	ULONG_PTR OriginalBase;
	LARGE_INTEGER LoadTime;
} LDR_DATA_TABLE_ENTRY_7, *PLDR_DATA_TABLE_ENTRY_7, LDR_MODULE_7, *PLDR_MODULE_7;

typedef
VOID
(NTAPI *PPS_POST_PROCESS_INIT_ROUTINE) (
    VOID
    );

typedef struct _PEB_FREE_BLOCK
{
	struct _PEB_FREE_BLOCK* Next;
	ULONG Size;
} PEB_FREE_BLOCK;

// XP SP3 version
typedef struct _PEB_XP {
    BOOLEAN InheritedAddressSpace;
    BOOLEAN ReadImageFileExecOptions;
    BOOLEAN BeingDebugged;
    BOOLEAN Spare;
    HANDLE Mutant;
    PVOID ImageBaseAddress;
    PPEB_LDR_DATA_XP LoaderData;
    RTL_USER_PROCESS_PARAMETERS_XP* ProcessParameters;
    PVOID SubSystemData;
    PVOID ProcessHeap;
    CRITICAL_SECTION* FastPebLock;
    PVOID FastPebLockRoutine;
    PVOID FastPebUnlockRoutine;
    ULONG EnvironmentUpdateCount;
    PVOID* KernelCallbackTable;
	ULONG SystemReserved[1];
	ULONG AtlThunkSListPtr32;
	PEB_FREE_BLOCK* FreeList;
    ULONG TlsExpansionCounter;
    PVOID TlsBitmap;
    ULONG TlsBitmapBits[2];
    PVOID ReadOnlySharedMemoryBase;
    PVOID ReadOnlySharedMemoryHeap;
    PVOID* ReadOnlyStaticServerData;
    PVOID AnsiCodePageData; 
    PVOID OemCodePageData; 
    PVOID UnicodeCaseTableData; 
    ULONG NumberOfProcessors; 
    ULONG NtGlobalFlag;
    LARGE_INTEGER CriticalSectionTimeout;
    ULONG HeapSegmentReserve;
    ULONG HeapSegmentCommit;
    ULONG HeapDeCommitTotalFreeThreshold;
    ULONG HeapDeCommitFreeBlockThreshold;
    ULONG NumberOfHeaps; 
    ULONG MaximumNumberOfHeaps; 
    PVOID* *ProcessHeaps;
    PVOID GdiSharedHandleTable;
    PVOID ProcessStarterHelper;
    PVOID GdiDCAttributeList; 
    PVOID LoaderLock; 
    ULONG OSMajorVersion;
    ULONG OSMinorVersion;
    USHORT OSBuildNumber;
    USHORT OSPlatformId; 
    ULONG ImageSubSystem;
    ULONG ImageSubSystemMajorVersion;
    ULONG ImageSubSystemMinorVersion;
	ULONG ImageProcessAffinityMask;
    ULONG GdiHandleBuffer[34];
    PVOID PostProcessInitRoutine;
    ULONG TlsExpansionBitmap;
    ULONG TlsExpansionBitmapBits[32];
    ULONG SessionId;
	ULARGE_INTEGER AppCompatFlags;
	ULARGE_INTEGER AppcompatFlagsUser;
	PVOID pShimData;
	PVOID AppCompatInfo;
	UNICODE_STRING CSDVersion;
	PVOID ActivateContextData;
	PVOID ProcessAssemblyStorageMap;
	PVOID SystemDefaultActivationContextData;
	PVOID SystemAssemblyStorageMap;
	ULONG MinimumStackCommit;
} PEB_XP, *PPEB_XP;

// Win Vista SP1 / SP2 / Win 7 / Win 7 SP1 version
typedef struct _PEB_VISTA_7 {
    BOOLEAN InheritedAddressSpace;
    BOOLEAN ReadImageFileExecOptions;
    BOOLEAN BeingDebugged;
#include <pshpack1.h>
	union
	{
		BOOLEAN BitField;
		struct
		{
			BOOLEAN ImageUsesLargePages :1;
			BOOLEAN IsProtectedProcess :1;
			BOOLEAN IsLegacyProcess :1;
			BOOLEAN IsImageDynamicallyRelocated :1;
			BOOLEAN SkipPatchingUser32Forwarders :1;
			BOOLEAN SpareBits :3;
		};
	};
#include <poppack.h>
    HANDLE Mutant;
    PVOID ImageBaseAddress;
    PPEB_LDR_DATA_VISTA_7 LoaderData;
    RTL_USER_PROCESS_PARAMETERS_VISTA* ProcessParameters;
    PVOID SubSystemData;
    PVOID ProcessHeap;
    CRITICAL_SECTION* FastPebLock;
	PVOID AtlThunkSListPtr;
	HKEY IFEOKey;
#include <pshpack1.h>
	union
	{
		ULONG CrossProcessFlags;
		struct
		{
			ULONG ProcessInJob :1;
			ULONG ProcessInitializing :1;
			ULONG ProcessUsingVEH :1;
			ULONG ProcessUsingVCH :1;
			ULONG ProcessUsingFTH :1; // 7 Only
			ULONG ReservedBits0 :0x1b;
		};
	};
#include <poppack.h>
	PVOID KernelCallbackTable;
	PVOID UserSharedInfoPtr;
	ULONG SystemReserved[1];
	ULONG AtlThunkSListPtr32;
	PVOID ApiSetMap;
	ULONG TlsExpansionCounter;
    PVOID TlsBitmap;
    ULONG TlsBitmapBits[0x2];
    PVOID ReadOnlySharedMemoryBase;
	PVOID HotpatchInformation;
	PVOID* ReadOnlyStaticServerData;
    PVOID AnsiCodePageData; 
    PVOID OemCodePageData; 
    PVOID UnicodeCaseTableData; 
    ULONG NumberOfProcessors; 
    ULONG NtGlobalFlag;
    LARGE_INTEGER CriticalSectionTimeout;
    ULONG_PTR HeapSegmentReserve;
    ULONG_PTR HeapSegmentCommit;
    ULONG_PTR HeapDeCommitTotalFreeThreshold;
    ULONG_PTR HeapDeCommitFreeBlockThreshold;
    ULONG NumberOfHeaps; 
    ULONG MaximumNumberOfHeaps; 
    PVOID* *ProcessHeaps;
    PVOID GdiSharedHandleTable;
    PVOID ProcessStarterHelper;
    PVOID GdiDCAttributeList;
    PCRITICAL_SECTION LoaderLock; 
    ULONG OSMajorVersion;
    ULONG OSMinorVersion;
    USHORT OSBuildNumber;
    USHORT OSPlatformId; 
    ULONG ImageSubSystem;
    ULONG ImageSubSystemMajorVersion;
    ULONG ImageSubSystemMinorVersion;
	KAFFINITY ActiveProcessAffinityMask;
#ifdef _WIN64
	ULONG GdiHandleBuffer[0x3c];
#else
    ULONG GdiHandleBuffer[0x22];
#endif
    PVOID PostProcessInitRoutine;
    PVOID TlsExpansionBitmap;
    ULONG TlsExpansionBitmapBits[0x20];
    ULONG SessionId;
	ULARGE_INTEGER AppCompatFlags;
	ULARGE_INTEGER AppCompatFlagsUser;
	PVOID pShimData;
	PVOID AppCompatInfo;
	UNICODE_STRING CSDVersion;
	const PVOID ActivationContextData;
	PVOID ProcessAssemblyStorageMap;
	const PVOID SystemDefaultActivationContextData;
	PVOID SystemAssemblyStorageMap;
	ULONG_PTR MinimumStackCommit;
	PVOID FlsCallback;
	LIST_ENTRY FlsListHead;
    PVOID FlsBitmap;
	ULONG FlsBitmapBits[0x4];
	ULONG FlsHighIndex;
	PVOID WerRegistrationData;
	PVOID WerShipAssertPtr; // Last on Vista
	PVOID pContextData;
	PVOID pImageHeaderHash;
#include <pshpack1.h>
	union
	{
		ULONG TracingFlags;
		struct
		{
			ULONG HeapTracingEnable :1;
			ULONG CritSecTracingEnabled :1;
			ULONG SpareTracingBits :0x1e;
		};
	};
#include <poppack.h>
} PEB_VISTA_7, *PPEB_VISTA_7;

typedef enum _LPC_TYPE {
	LPC_NEW_MESSAGE, // A new message
	LPC_REQUEST, // A request message
	LPC_REPLY, // A reply to a request message
	LPC_DATAGRAM, //
	LPC_LOST_REPLY, //
	LPC_PORT_CLOSED, // Sent when port is deleted
	LPC_CLIENT_DIED, // Messages to thread termination ports
	LPC_EXCEPTION, // Messages to thread exception port
	LPC_DEBUG_EVENT, // Messages to thread debug port
	LPC_ERROR_EVENT, // Used by ZwRaiseHardError
	LPC_CONNECTION_REQUEST // Used by ZwConnectPort
} LPC_TYPE;

typedef struct _PORT_MESSAGE 
{
    union {
        struct {
            SHORT DataLength;
            SHORT TotalLength;
        } s1;
        ULONG Length;
    } u1;
    union {
        struct {
            SHORT Type;
            SHORT DataInfoOffset;
        } s2;
        ULONG ZeroInit;
    } u2;
    union {
        CLIENT_ID ClientId;
        double DoNotUseThisField;       // Force quadword alignment
    } DUMMYUNIONNAME3;
    ULONG MessageId;
    union {
        SIZE_T ClientViewSize;          // Only valid on LPC_CONNECTION_REQUEST message
        ULONG CallbackId;                   // Only valid on LPC_REQUEST message
    } DUMMYUNIONNAME4;
//  UCHAR Data[];
} PORT_MESSAGE, *PPORT_MESSAGE;

typedef struct _DEBUG_MESSAGE {
	PORT_MESSAGE PortMessage;
	ULONG EventCode;
	ULONG Status;
	union {
		struct {
			EXCEPTION_RECORD ExceptionRecord;
			ULONG FirstChance;
		} Exception;
		struct {
			ULONG Reserved;
			PVOID StartAddress;
		} CreateThread;
		struct {
			ULONG Reserved;
			HANDLE FileHandle;
			PVOID Base;
			ULONG PointerToSymbolTable;
			ULONG NumberOfSymbols;
			ULONG Reserved2;
			PVOID EntryPoint;
		} CreateProcess;
		struct {
			ULONG ExitCode;
		} ExitThread;
		struct {
			ULONG ExitCode;
		} ExitProcess;
		struct {
			HANDLE FileHandle;
			PVOID Base;
			ULONG PointerToSymbolTable;
			ULONG NumberOfSymbols;
		} LoadDll;
		struct {
			PVOID Base;
		} UnloadDll;
		struct {
			LPSTR lpDebugStringData;
			WORD fUnicode;
			WORD nDebugStringLength;
		} OutputDebugStringInfo;
		struct
		{
			DWORD dwError;
			DWORD dwType;
		}RipInfo;
	} u;
} DEBUG_MESSAGE, *PDEBUG_MESSAGE;

typedef struct _IO_STATUS_BLOCK {
    union {
        NTSTATUS Status;
        PVOID Pointer;
    };
    ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

#ifdef __cplusplus
extern "C"
{
#endif

NTSYSAPI
VOID
NTAPI 
RtlInitUnicodeString (
    PUNICODE_STRING DestinationString,
    PCWSTR SourceString
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlCreateUnicodeString(
    OUT PUNICODE_STRING  DestinationString,
    IN PCWSTR  SourceString
    );


#if !defined(MIDL_PASS)
FORCEINLINE
VOID
RtlInitEmptyUnicodeString(
    __out PUNICODE_STRING UnicodeString,
    __bcount_opt(BufferSize) __drv_aliasesMem PWCHAR Buffer,
    __in USHORT BufferSize
    )
{
    UnicodeString->Length = 0;
    UnicodeString->MaximumLength = BufferSize;
    UnicodeString->Buffer = Buffer;
}

FORCEINLINE
VOID
RtlInitEmptyAnsiString(
    __out PANSI_STRING AnsiString,
    __bcount_opt(BufferSize) __drv_aliasesMem PCHAR Buffer,
    __in USHORT BufferSize
    )
{
    AnsiString->Length = 0;
    AnsiString->MaximumLength = BufferSize;
    AnsiString->Buffer = Buffer;
}
#endif // !defined(MIDL_PASS)

FORCEINLINE
VOID
RtlInitUnicodeStringLen(
    __out PUNICODE_STRING UnicodeString,
    __bcount_opt(BufferSize) __drv_aliasesMem PWCHAR Buffer,
    __in USHORT ToUseChars
    )
{
    UnicodeString->Length = UnicodeString->MaximumLength = ToUseChars;
    UnicodeString->Buffer = Buffer;
}

FORCEINLINE
VOID
RtlInitAnsiStringLen(
    __out PANSI_STRING AnsiString,
    __bcount_opt(BufferSize) __drv_aliasesMem PCHAR Buffer,
    __in USHORT ToUseChars
    )
{
    AnsiString->Length = AnsiString->MaximumLength = ToUseChars;
    AnsiString->Buffer = Buffer;
}

#define HASH_STRING_ALGORITHM_DEFAULT   (0)
#define HASH_STRING_ALGORITHM_X65599    (1)
#define HASH_STRING_ALGORITHM_INVALID   (0xffffffff)

#if (NTDDI_VERSION >= NTDDI_WINXP)
__drv_maxIRQL(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlHashUnicodeString(
	__in PCUNICODE_STRING String,
	__in BOOLEAN CaseInSensitive,
	__in ULONG HashAlgorithm,
	__out PULONG HashValue
);

#endif // NTDDI_VERSION >= NTDDI_WINXP

NTSYSAPI
ULONG
NTAPI
RtlGetFullPathName_U(
	const WCHAR* name,
	ULONG size,
	WCHAR* buffer,
	WCHAR** file_part
	);

//
// Event type
//

typedef enum _EVENT_TYPE {
    NotificationEvent,
    SynchronizationEvent
    } EVENT_TYPE;

NTSYSAPI
NTSTATUS
NTAPI
NtSetEvent(
	IN HANDLE EventHandle,
	OUT PLONG PreviousState OPTIONAL
	);

NTSYSAPI
NTSTATUS
NTAPI
NtCreateEvent(
	OUT PHANDLE  EventHandle,
	IN ACCESS_MASK  DesiredAccess,
	IN POBJECT_ATTRIBUTES  ObjectAttributes OPTIONAL,
	IN EVENT_TYPE  EventType,
	IN BOOLEAN  InitialState
);

NTSYSAPI
NTSTATUS
NTAPI
NtWaitForSingleObject(
    IN HANDLE Handle,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
    );

typedef VOID (CALLBACK*PIO_APC_ROUTINE)(VOID);

NTSYSAPI
NTSTATUS
NTAPI
NtCreateFile(
	OUT PHANDLE  FileHandle,
	IN ACCESS_MASK  DesiredAccess,
	IN POBJECT_ATTRIBUTES  ObjectAttributes,
	OUT PIO_STATUS_BLOCK  IoStatusBlock,
	IN PLARGE_INTEGER  AllocationSize  OPTIONAL,
	IN ULONG  FileAttributes,
	IN ULONG  ShareAccess,
	IN ULONG  CreateDisposition,
	IN ULONG  CreateOptions,
	IN PVOID  EaBuffer  OPTIONAL,
	IN ULONG  EaLength
);

NTSYSAPI
NTSTATUS 
NTAPI
NtWriteFile(
	IN HANDLE  FileHandle,
	IN HANDLE  Event  OPTIONAL,
	IN PIO_APC_ROUTINE  ApcRoutine  OPTIONAL,
	IN PVOID  ApcContext  OPTIONAL,
	OUT PIO_STATUS_BLOCK  IoStatusBlock,
	IN PVOID  Buffer,
	IN ULONG  Length,
	IN PLARGE_INTEGER  ByteOffset  OPTIONAL,
	IN PULONG  Key  OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
NtReadFile(
	IN HANDLE  FileHandle,
	IN HANDLE  Event  OPTIONAL,
	IN PIO_APC_ROUTINE  ApcRoutine  OPTIONAL,
	IN PVOID  ApcContext  OPTIONAL,
	OUT PIO_STATUS_BLOCK  IoStatusBlock,
	OUT PVOID  Buffer,
	IN ULONG  Length,
	IN PLARGE_INTEGER  ByteOffset  OPTIONAL,
	IN PULONG  Key  OPTIONAL
);


NTSYSAPI
NTSTATUS
NTAPI
NtTerminateThread(
	IN HANDLE ThreadHandle,
	IN NTSTATUS ExitStatus
	); 

NTSYSAPI 
NTSTATUS
NTAPI
NtAlertThread(
	IN HANDLE ThreadHandle
	);

NTSYSAPI
NTSTATUS
NTAPI
NtResumeThread(
	IN HANDLE ThreadHandle,
	OUT PULONG SuspendCount OPTIONAL
	);

NTSYSAPI
NTSTATUS
NTAPI
NtTerminateProcess(
	IN HANDLE ProcessHandle,
	IN NTSTATUS ExitStatus
	);

typedef enum _HARDERROR_RESPONSE_OPTION {
    OptionAbortRetryIgnore,
    OptionOk,
    OptionOkCancel,
    OptionRetryCancel,
    OptionYesNo,
    OptionYesNoCancel,
    OptionShutdownSystem
} HARDERROR_RESPONSE_OPTION, *PHARDERROR_RESPONSE_OPTION;

NTSYSAPI
NTSTATUS
NTAPI
NtRaiseHardError(
	IN NTSTATUS ErrorStatus,
    IN ULONG NumberOfParameters,
    IN ULONG UnicodeStringParameterMask,
    IN PULONG_PTR Parameters,
    IN HARDERROR_RESPONSE_OPTION ValidResponseOptions,
    OUT PHARDERROR_RESPONSE_OPTION Response
);

typedef enum _MEMORY_INFORMATION_CLASS
{
	MemoryBasicInformation
} MEMORY_INFORMATION_CLASS, *PMEMORY_INFORMATION_CLASS;

NTSYSAPI
NTSTATUS
NTAPI
NtQueryVirtualMemory(
	IN HANDLE ProcessHandle,
	IN PVOID BaseAddress,
	IN MEMORY_INFORMATION_CLASS MemoryInformationClass,
	OUT PVOID Buffer,
	IN ULONG Length,
	OUT PULONG ResultLength OPTIONAL
	); 

typedef enum _MEMORY_MAP_TYPE
{
	Map_Process = 1
} MEMORY_MAP_TYPE, *PMEMORY_MAP_TYPE;

NTSYSAPI
NTSTATUS
NTAPI
NtLockVirtualMemory(
	IN HANDLE ProcessHandle,
	IN OUT PVOID* BaseAddress,
	IN PSIZE_T RegionSize,
	IN MEMORY_MAP_TYPE MapType
	);

NTSYSAPI
NTSTATUS 
NTAPI
NtAllocateVirtualMemory(
	IN HANDLE  ProcessHandle,
	IN OUT PVOID  *BaseAddress,
	IN ULONG  ZeroBits,
	IN OUT PSIZE_T  RegionSize,
	IN ULONG  AllocationType,
	IN ULONG  Protect
	); 

NTSYSAPI
NTSTATUS 
NTAPI
NtFreeVirtualMemory(
	IN HANDLE  ProcessHandle,
	IN OUT PVOID  *BaseAddress,
	IN OUT PSIZE_T  RegionSize,
	IN ULONG  FreeType
	);

NTSYSAPI
NTSTATUS
NTAPI
NtWriteVirtualMemory(
     IN HANDLE ProcessHandle,
     OUT PVOID BaseAddress,
     IN PVOID Buffer,
     IN ULONG BufferSize,
     OUT PULONG NumberOfBytesWritten OPTIONAL
     );

typedef VOID (NTAPI*PPS_APC_ROUTINE)(PVOID, PVOID, PVOID);

NTSYSAPI
NTSTATUS
NTAPI
NtQueueApcThread(
    IN HANDLE ThreadHandle,
    IN PPS_APC_ROUTINE ApcRoutine,
    IN PVOID ApcArgument1,
    IN PVOID ApcArgument2,
    IN PVOID ApcArgument3
    );

#define NtCurrentProcess() ( (HANDLE)(LONG_PTR) -1 )  
#define ZwCurrentProcess() NtCurrentProcess()         
#define NtCurrentThread() ( (HANDLE)(LONG_PTR) -2 )   
#define ZwCurrentThread() NtCurrentThread()

NTSYSAPI
SIZE_T
NTAPI
RtlCompactHeap(
    IN HANDLE hHeap,
    IN ULONG flags
    );

NTSYSAPI
PVOID
NTAPI
RtlDestroyHeap(
    IN HANDLE hHeap
    );

NTSYSAPI
VOID
NTAPI
RtlEnterCriticalSection(
	IN LPCRITICAL_SECTION pSection
	);

NTSYSAPI
VOID
NTAPI
RtlLeaveCriticalSection(
	IN LPCRITICAL_SECTION pSection
	);

VOID
NTAPI
RtlDeleteCriticalSection(
	IN LPCRITICAL_SECTION pSection
	);

ULONG
NTAPI
RtlNtStatusToDosError(
   IN NTSTATUS status
   );

VOID
NTAPI
RtlSetLastWin32Error(
   IN ULONG error
   );

NTSTATUS
NTAPI
NtClose(
	IN HANDLE hObject
	);

//
// Define the create disposition values
//

#define FILE_SUPERSEDE                  0x00000000
#define FILE_OPEN                       0x00000001
#define FILE_CREATE                     0x00000002
#define FILE_OPEN_IF                    0x00000003
#define FILE_OVERWRITE                  0x00000004
#define FILE_OVERWRITE_IF               0x00000005
#define FILE_MAXIMUM_DISPOSITION        0x00000005

//
// Define the create/open option flags
//

#define FILE_DIRECTORY_FILE                     0x00000001
#define FILE_WRITE_THROUGH                      0x00000002
#define FILE_SEQUENTIAL_ONLY                    0x00000004
#define FILE_NO_INTERMEDIATE_BUFFERING          0x00000008

#define FILE_SYNCHRONOUS_IO_ALERT               0x00000010
#define FILE_SYNCHRONOUS_IO_NONALERT            0x00000020
#define FILE_NON_DIRECTORY_FILE                 0x00000040
#define FILE_CREATE_TREE_CONNECTION             0x00000080

#define FILE_COMPLETE_IF_OPLOCKED               0x00000100
#define FILE_NO_EA_KNOWLEDGE                    0x00000200
#define FILE_OPEN_REMOTE_INSTANCE               0x00000400
#define FILE_RANDOM_ACCESS                      0x00000800

#define FILE_DELETE_ON_CLOSE                    0x00001000
#define FILE_OPEN_BY_FILE_ID                    0x00002000
#define FILE_OPEN_FOR_BACKUP_INTENT             0x00004000
#define FILE_NO_COMPRESSION                     0x00008000

#if (NTDDI_VERSION >= NTDDI_WIN7)
#define FILE_OPEN_REQUIRING_OPLOCK              0x00010000
#define FILE_DISALLOW_EXCLUSIVE                 0x00020000
#endif /* NTDDI_VERSION >= NTDDI_WIN7 */

#define FILE_RESERVE_OPFILTER                   0x00100000
#define FILE_OPEN_REPARSE_POINT                 0x00200000
#define FILE_OPEN_NO_RECALL                     0x00400000
#define FILE_OPEN_FOR_FREE_SPACE_QUERY          0x00800000

#define FILE_VALID_OPTION_FLAGS                 0x00ffffff
#define FILE_VALID_PIPE_OPTION_FLAGS            0x00000032
#define FILE_VALID_MAILSLOT_OPTION_FLAGS        0x00000032
#define FILE_VALID_SET_FLAGS                    0x00000036

//
// Define the I/O status information return values for NtCreateFile/NtOpenFile
//

#define FILE_SUPERSEDED                 0x00000000
#define FILE_OPENED                     0x00000001
#define FILE_CREATED                    0x00000002
#define FILE_OVERWRITTEN                0x00000003
#define FILE_EXISTS                     0x00000004
#define FILE_DOES_NOT_EXIST             0x00000005

//
// Define special ByteOffset parameters for read and write operations
//

#define FILE_WRITE_TO_END_OF_FILE       0xffffffff
#define FILE_USE_FILE_POINTER_POSITION  0xfffffffe

//
// Define alignment requirement values
//

#define FILE_BYTE_ALIGNMENT             0x00000000
#define FILE_WORD_ALIGNMENT             0x00000001
#define FILE_LONG_ALIGNMENT             0x00000003
#define FILE_QUAD_ALIGNMENT             0x00000007
#define FILE_OCTA_ALIGNMENT             0x0000000f
#define FILE_32_BYTE_ALIGNMENT          0x0000001f
#define FILE_64_BYTE_ALIGNMENT          0x0000003f
#define FILE_128_BYTE_ALIGNMENT         0x0000007f
#define FILE_256_BYTE_ALIGNMENT         0x000000ff
#define FILE_512_BYTE_ALIGNMENT         0x000001ff

//
// Define the maximum length of a filename string
//

#define MAXIMUM_FILENAME_LENGTH         256

typedef enum _FILE_INFORMATION_CLASS {
    FileDirectoryInformation         = 1,
    FileFullDirectoryInformation,   // 2
    FileBothDirectoryInformation,   // 3
    FileBasicInformation,           // 4
    FileStandardInformation,        // 5
    FileInternalInformation,        // 6
    FileEaInformation,              // 7
    FileAccessInformation,          // 8
    FileNameInformation,            // 9
    FileRenameInformation,          // 10
    FileLinkInformation,            // 11
    FileNamesInformation,           // 12
    FileDispositionInformation,     // 13
    FilePositionInformation,        // 14
    FileFullEaInformation,          // 15
    FileModeInformation,            // 16
    FileAlignmentInformation,       // 17
    FileAllInformation,             // 18
    FileAllocationInformation,      // 19
    FileEndOfFileInformation,       // 20
    FileAlternateNameInformation,   // 21
    FileStreamInformation,          // 22
    FilePipeInformation,            // 23
    FilePipeLocalInformation,       // 24
    FilePipeRemoteInformation,      // 25
    FileMailslotQueryInformation,   // 26
    FileMailslotSetInformation,     // 27
    FileCompressionInformation,     // 28
    FileObjectIdInformation,        // 29
    FileCompletionInformation,      // 30
    FileMoveClusterInformation,     // 31
    FileQuotaInformation,           // 32
    FileReparsePointInformation,    // 33
    FileNetworkOpenInformation,     // 34
    FileAttributeTagInformation,    // 35
    FileTrackingInformation,        // 36
    FileIdBothDirectoryInformation, // 37
    FileIdFullDirectoryInformation, // 38
    FileValidDataLengthInformation, // 39
    FileShortNameInformation,       // 40
    FileIoCompletionNotificationInformation, // 41
    FileIoStatusBlockRangeInformation,       // 42
    FileIoPriorityHintInformation,           // 43
    FileSfioReserveInformation,              // 44
    FileSfioVolumeInformation,               // 45
    FileHardLinkInformation,                 // 46
    FileProcessIdsUsingFileInformation,      // 47
    FileNormalizedNameInformation,           // 48
    FileNetworkPhysicalNameInformation,      // 49 
    FileIdGlobalTxDirectoryInformation,      // 50
    FileMaximumInformation
} FILE_INFORMATION_CLASS, *PFILE_INFORMATION_CLASS;

//
// Define the various structures which are returned on query operations
//

typedef struct _FILE_ALIGNMENT_INFORMATION {
    ULONG AlignmentRequirement;
} FILE_ALIGNMENT_INFORMATION, *PFILE_ALIGNMENT_INFORMATION;

typedef struct _FILE_NAME_INFORMATION {
    ULONG FileNameLength;
    WCHAR FileName[1];
} FILE_NAME_INFORMATION, *PFILE_NAME_INFORMATION;

typedef struct _FILE_ATTRIBUTE_TAG_INFORMATION {
    ULONG FileAttributes;
    ULONG ReparseTag;
} FILE_ATTRIBUTE_TAG_INFORMATION, *PFILE_ATTRIBUTE_TAG_INFORMATION;

typedef struct _FILE_DISPOSITION_INFORMATION {
    BOOLEAN DeleteFile;
} FILE_DISPOSITION_INFORMATION, *PFILE_DISPOSITION_INFORMATION;

typedef struct _FILE_END_OF_FILE_INFORMATION {
    LARGE_INTEGER EndOfFile;
} FILE_END_OF_FILE_INFORMATION, *PFILE_END_OF_FILE_INFORMATION;

typedef struct _FILE_VALID_DATA_LENGTH_INFORMATION {
    LARGE_INTEGER ValidDataLength;
} FILE_VALID_DATA_LENGTH_INFORMATION, *PFILE_VALID_DATA_LENGTH_INFORMATION;

typedef struct _FILE_BASIC_INFORMATION {
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    ULONG FileAttributes;
} FILE_BASIC_INFORMATION, *PFILE_BASIC_INFORMATION;

typedef struct _FILE_STANDARD_INFORMATION {
    LARGE_INTEGER AllocationSize;
    LARGE_INTEGER EndOfFile;
    ULONG NumberOfLinks;
    BOOLEAN DeletePending;
    BOOLEAN Directory;
} FILE_STANDARD_INFORMATION, *PFILE_STANDARD_INFORMATION;


typedef struct _FILE_POSITION_INFORMATION {
    LARGE_INTEGER CurrentByteOffset;
} FILE_POSITION_INFORMATION, *PFILE_POSITION_INFORMATION;

typedef struct _FILE_NETWORK_OPEN_INFORMATION {
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER AllocationSize;
    LARGE_INTEGER EndOfFile;
    ULONG FileAttributes;
} FILE_NETWORK_OPEN_INFORMATION, *PFILE_NETWORK_OPEN_INFORMATION;

typedef struct _FILE_FULL_EA_INFORMATION {
    ULONG NextEntryOffset;
    UCHAR Flags;
    UCHAR EaNameLength;
    USHORT EaValueLength;
    CHAR EaName[1];
} FILE_FULL_EA_INFORMATION, *PFILE_FULL_EA_INFORMATION;

//
// Support to reserve bandwidth for a file handle.
//

typedef struct _FILE_SFIO_RESERVE_INFORMATION {
    ULONG RequestsPerPeriod;
    ULONG Period;
    BOOLEAN RetryFailures;
    BOOLEAN Discardable;
    ULONG RequestSize;
    ULONG NumOutstandingRequests;
} FILE_SFIO_RESERVE_INFORMATION, *PFILE_SFIO_RESERVE_INFORMATION;

//
// Support to query bandwidth properties of a volume.
//

typedef struct _FILE_SFIO_VOLUME_INFORMATION {
    ULONG MaximumRequestsPerPeriod;
    ULONG MinimumPeriod;
    ULONG MinimumTransferSize;
} FILE_SFIO_VOLUME_INFORMATION, *PFILE_SFIO_VOLUME_INFORMATION;

//
// NtQuery(Set)InformationFile return types:
//
//      FILE_BASIC_INFORMATION
//      FILE_STANDARD_INFORMATION
//      FILE_INTERNAL_INFORMATION
//      FILE_EA_INFORMATION
//      FILE_ACCESS_INFORMATION
//      FILE_POSITION_INFORMATION
//      FILE_MODE_INFORMATION
//      FILE_ALIGNMENT_INFORMATION
//      FILE_NAME_INFORMATION
//      FILE_ALL_INFORMATION
//
//      FILE_NETWORK_OPEN_INFORMATION
//
//      FILE_ALLOCATION_INFORMATION
//      FILE_COMPRESSION_INFORMATION
//      FILE_DISPOSITION_INFORMATION
//      FILE_END_OF_FILE_INFORMATION
//      FILE_LINK_INFORMATION
//      FILE_MOVE_CLUSTER_INFORMATION
//      FILE_RENAME_INFORMATION
//      FILE_SHORT_NAME_INFORMATION
//      FILE_STREAM_INFORMATION
//      FILE_COMPLETION_INFORMATION
//
//      FILE_PIPE_INFORMATION
//      FILE_PIPE_LOCAL_INFORMATION
//      FILE_PIPE_REMOTE_INFORMATION
//
//      FILE_MAILSLOT_QUERY_INFORMATION
//      FILE_MAILSLOT_SET_INFORMATION
//      FILE_REPARSE_POINT_INFORMATION
//
//      FILE_NETWORK_PHYSICAL_NAME_INFORMATION
//

typedef struct _FILE_INTERNAL_INFORMATION {
    LARGE_INTEGER IndexNumber;
} FILE_INTERNAL_INFORMATION, *PFILE_INTERNAL_INFORMATION;

typedef struct _FILE_EA_INFORMATION {
    ULONG EaSize;
} FILE_EA_INFORMATION, *PFILE_EA_INFORMATION;

typedef struct _FILE_ACCESS_INFORMATION {
    ACCESS_MASK AccessFlags;
} FILE_ACCESS_INFORMATION, *PFILE_ACCESS_INFORMATION;

typedef struct _FILE_MODE_INFORMATION {
    ULONG Mode;
} FILE_MODE_INFORMATION, *PFILE_MODE_INFORMATION;

typedef struct _FILE_ALL_INFORMATION {
    FILE_BASIC_INFORMATION BasicInformation;
    FILE_STANDARD_INFORMATION StandardInformation;
    FILE_INTERNAL_INFORMATION InternalInformation;
    FILE_EA_INFORMATION EaInformation;
    FILE_ACCESS_INFORMATION AccessInformation;
    FILE_POSITION_INFORMATION PositionInformation;
    FILE_MODE_INFORMATION ModeInformation;
    FILE_ALIGNMENT_INFORMATION AlignmentInformation;
    FILE_NAME_INFORMATION NameInformation;
} FILE_ALL_INFORMATION, *PFILE_ALL_INFORMATION;

typedef struct _FILE_ALLOCATION_INFORMATION {
    LARGE_INTEGER AllocationSize;
} FILE_ALLOCATION_INFORMATION, *PFILE_ALLOCATION_INFORMATION;

typedef struct _FILE_COMPRESSION_INFORMATION {
    LARGE_INTEGER CompressedFileSize;
    USHORT CompressionFormat;
    UCHAR CompressionUnitShift;
    UCHAR ChunkShift;
    UCHAR ClusterShift;
    UCHAR Reserved[3];
} FILE_COMPRESSION_INFORMATION, *PFILE_COMPRESSION_INFORMATION;


#ifdef _MAC
#pragma warning( disable : 4121)
#endif

typedef struct _FILE_LINK_INFORMATION {
    BOOLEAN ReplaceIfExists;
    HANDLE RootDirectory;
    ULONG FileNameLength;
    WCHAR FileName[1];
} FILE_LINK_INFORMATION, *PFILE_LINK_INFORMATION;


#ifdef _MAC
#pragma warning( default : 4121 )
#endif

typedef struct _FILE_MOVE_CLUSTER_INFORMATION {
    ULONG ClusterCount;
    HANDLE RootDirectory;
    ULONG FileNameLength;
    WCHAR FileName[1];
} FILE_MOVE_CLUSTER_INFORMATION, *PFILE_MOVE_CLUSTER_INFORMATION;

#ifdef _MAC
#pragma warning( disable : 4121)
#endif

typedef struct _FILE_RENAME_INFORMATION {
    BOOLEAN ReplaceIfExists;
    HANDLE RootDirectory;
    ULONG FileNameLength;
    WCHAR FileName[1];
} FILE_RENAME_INFORMATION, *PFILE_RENAME_INFORMATION;

#ifdef _MAC
#pragma warning( default : 4121 )
#endif

typedef struct _FILE_STREAM_INFORMATION {
    ULONG NextEntryOffset;
    ULONG StreamNameLength;
    LARGE_INTEGER StreamSize;
    LARGE_INTEGER StreamAllocationSize;
    WCHAR StreamName[1];
} FILE_STREAM_INFORMATION, *PFILE_STREAM_INFORMATION;

typedef struct _FILE_TRACKING_INFORMATION {
    HANDLE DestinationFile;
    ULONG ObjectInformationLength;
    CHAR ObjectInformation[1];
} FILE_TRACKING_INFORMATION, *PFILE_TRACKING_INFORMATION;

typedef struct _FILE_COMPLETION_INFORMATION {
    HANDLE Port;
    PVOID Key;
} FILE_COMPLETION_INFORMATION, *PFILE_COMPLETION_INFORMATION;

typedef struct _FILE_PIPE_INFORMATION {
     ULONG ReadMode;
     ULONG CompletionMode;
} FILE_PIPE_INFORMATION, *PFILE_PIPE_INFORMATION;

typedef struct _FILE_PIPE_LOCAL_INFORMATION {
     ULONG NamedPipeType;
     ULONG NamedPipeConfiguration;
     ULONG MaximumInstances;
     ULONG CurrentInstances;
     ULONG InboundQuota;
     ULONG ReadDataAvailable;
     ULONG OutboundQuota;
     ULONG WriteQuotaAvailable;
     ULONG NamedPipeState;
     ULONG NamedPipeEnd;
} FILE_PIPE_LOCAL_INFORMATION, *PFILE_PIPE_LOCAL_INFORMATION;

typedef struct _FILE_PIPE_REMOTE_INFORMATION {
     LARGE_INTEGER CollectDataTime;
     ULONG MaximumCollectionCount;
} FILE_PIPE_REMOTE_INFORMATION, *PFILE_PIPE_REMOTE_INFORMATION;

typedef struct _FILE_MAILSLOT_QUERY_INFORMATION {
    ULONG MaximumMessageSize;
    ULONG MailslotQuota;
    ULONG NextMessageSize;
    ULONG MessagesAvailable;
    LARGE_INTEGER ReadTimeout;
} FILE_MAILSLOT_QUERY_INFORMATION, *PFILE_MAILSLOT_QUERY_INFORMATION;

typedef struct _FILE_MAILSLOT_SET_INFORMATION {
    PLARGE_INTEGER ReadTimeout;
} FILE_MAILSLOT_SET_INFORMATION, *PFILE_MAILSLOT_SET_INFORMATION;

typedef struct _FILE_REPARSE_POINT_INFORMATION {
    LONGLONG FileReference;
    ULONG Tag;
} FILE_REPARSE_POINT_INFORMATION, *PFILE_REPARSE_POINT_INFORMATION;

typedef struct _FILE_LINK_ENTRY_INFORMATION {
    ULONG NextEntryOffset;
    LONGLONG ParentFileId;
    ULONG FileNameLength;
    WCHAR FileName[1];
} FILE_LINK_ENTRY_INFORMATION, *PFILE_LINK_ENTRY_INFORMATION;

typedef struct _FILE_LINKS_INFORMATION {
    ULONG BytesNeeded;
    ULONG EntriesReturned;
    FILE_LINK_ENTRY_INFORMATION Entry;
} FILE_LINKS_INFORMATION, *PFILE_LINKS_INFORMATION;

typedef struct _FILE_NETWORK_PHYSICAL_NAME_INFORMATION {
    ULONG FileNameLength;
    WCHAR FileName[1];
} FILE_NETWORK_PHYSICAL_NAME_INFORMATION, *PFILE_NETWORK_PHYSICAL_NAME_INFORMATION;

//
// Define the file system information class values
//
// WARNING:  The order of the following values are assumed by the I/O system.
//           Any changes made here should be reflected there as well.

typedef enum _FSINFOCLASS {
    FileFsVolumeInformation       = 1,
    FileFsLabelInformation,      // 2
    FileFsSizeInformation,       // 3
    FileFsDeviceInformation,     // 4
    FileFsAttributeInformation,  // 5
    FileFsControlInformation,    // 6
    FileFsFullSizeInformation,   // 7
    FileFsObjectIdInformation,   // 8
    FileFsDriverPathInformation, // 9
    FileFsVolumeFlagsInformation,// 10
    FileFsMaximumInformation
} FS_INFORMATION_CLASS, *PFS_INFORMATION_CLASS;

typedef struct _FILE_FS_LABEL_INFORMATION {
    ULONG VolumeLabelLength;
    WCHAR VolumeLabel[1];
} FILE_FS_LABEL_INFORMATION, *PFILE_FS_LABEL_INFORMATION;

typedef struct _FILE_FS_VOLUME_INFORMATION {
    LARGE_INTEGER VolumeCreationTime;
    ULONG VolumeSerialNumber;
    ULONG VolumeLabelLength;
    BOOLEAN SupportsObjects;
    WCHAR VolumeLabel[1];
} FILE_FS_VOLUME_INFORMATION, *PFILE_FS_VOLUME_INFORMATION;

typedef struct _FILE_FS_SIZE_INFORMATION {
    LARGE_INTEGER TotalAllocationUnits;
    LARGE_INTEGER AvailableAllocationUnits;
    ULONG SectorsPerAllocationUnit;
    ULONG BytesPerSector;
} FILE_FS_SIZE_INFORMATION, *PFILE_FS_SIZE_INFORMATION;

typedef struct _FILE_FS_FULL_SIZE_INFORMATION {
    LARGE_INTEGER TotalAllocationUnits;
    LARGE_INTEGER CallerAvailableAllocationUnits;
    LARGE_INTEGER ActualAvailableAllocationUnits;
    ULONG SectorsPerAllocationUnit;
    ULONG BytesPerSector;
} FILE_FS_FULL_SIZE_INFORMATION, *PFILE_FS_FULL_SIZE_INFORMATION;

typedef struct _FILE_FS_OBJECTID_INFORMATION {
    UCHAR ObjectId[16];
    UCHAR ExtendedInfo[48];
} FILE_FS_OBJECTID_INFORMATION, *PFILE_FS_OBJECTID_INFORMATION;

typedef struct _FILE_FS_DEVICE_INFORMATION {
    ULONG DeviceType;
    ULONG Characteristics;
} FILE_FS_DEVICE_INFORMATION, *PFILE_FS_DEVICE_INFORMATION;

NTSYSAPI
NTSTATUS
NTAPI
NtQueryVolumeInformationFile(
	IN HANDLE FileHandle,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	OUT PVOID FsInformation,
	IN ULONG Length,
	IN FS_INFORMATION_CLASS FsInformationClass
);

NTSYSAPI
NTSTATUS
NTAPI
NtQueryFullAttributesFile(
	IN POBJECT_ATTRIBUTES pObjAttrs,
	IN OUT PFILE_NETWORK_OPEN_INFORMATION pNetInf
);

NTSYSAPI
NTSTATUS
NTAPI
NtSetInformationFile(
	IN HANDLE  FileHandle,
	OUT PIO_STATUS_BLOCK  IoStatusBlock,
	IN PVOID  FileInformation,
	IN ULONG  Length,
	IN FILE_INFORMATION_CLASS  FileInformationClass
);

NTSYSAPI
NTSTATUS
NTAPI
NtQueryInformationFile(
	IN HANDLE  FileHandle,
	OUT PIO_STATUS_BLOCK  IoStatusBlock,
	IN PVOID  FileInformation,
	IN ULONG  Length,
	IN FILE_INFORMATION_CLASS  FileInformationClass
);

#define DIRECTORY_QUERY                 (0x0001)
#define DIRECTORY_TRAVERSE              (0x0002)
#define DIRECTORY_CREATE_OBJECT         (0x0004)
#define DIRECTORY_CREATE_SUBDIRECTORY   (0x0008)

#define DIRECTORY_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | 0xF)

NTSTATUS 
NTAPI
NtCreateDirectoryObject(
						OUT PHANDLE DirectoryHandle,
						IN ACCESS_MASK DesiredAccess,
						IN POBJECT_ATTRIBUTES ObjectAttributes
						);

NTSTATUS 
NTAPI
NtOpenDirectoryObject(
					OUT PHANDLE DirectoryHandle,
					IN ACCESS_MASK DesiredAccess,
					IN POBJECT_ATTRIBUTES ObjectAttributes
					);

typedef struct _OBJECT_DIRECTORY_INFORMATION 
{
    UNICODE_STRING Name;
    UNICODE_STRING TypeName;
} OBJECT_DIRECTORY_INFORMATION, *POBJECT_DIRECTORY_INFORMATION;

NTSTATUS
NTAPI
NtQueryDirectoryObject(
						HANDLE DirectoryHandle,
						PVOID Buffer,
						ULONG Length,
						BOOLEAN ReturnSingleEntry,
						BOOLEAN RestartScan,
						PULONG Context,
						PULONG ReturnLength
						);


VOID
NTAPI 
RtlInitAnsiString (
    PANSI_STRING DestinationString,
    PCSTR SourceString
    );

NTSTATUS RtlAnsiStringToUnicodeString(
	IN OUT PUNICODE_STRING DestinationString,
    IN PCANSI_STRING SourceString,
    IN BOOLEAN AllocateDestinationString
);

NTSTATUS
NTAPI
RtlCreateProcessParameters(
		OUT PPROCESS_PARAMETERS_VISTA *ProcessParameters,
		IN PUNICODE_STRING ImageFile,
		IN PUNICODE_STRING DllPath OPTIONAL,
		IN PUNICODE_STRING CurrentDirectory OPTIONAL,
		IN PUNICODE_STRING CommandLine OPTIONAL,
		IN ULONG CreationFlags,
		IN PUNICODE_STRING WindowTitle OPTIONAL,
		IN PUNICODE_STRING Desktop OPTIONAL,
		IN PUNICODE_STRING Reserved OPTIONAL,
		IN PUNICODE_STRING Reserved2 OPTIONAL
);

NTSTATUS
NTAPI
RtlDestroyProcessParameters(
		IN PPROCESS_PARAMETERS_VISTA ProcessParameters
		);

NTSYSAPI 
BOOLEAN 
NTAPI 
RtlDosPathNameToNtPathName_U( 
							 IN PCWSTR DosName, 
							 OUT PUNICODE_STRING NtName, 
							 OUT PCWSTR *DosFilePath OPTIONAL, 
							 OUT PUNICODE_STRING NtFilePath OPTIONAL 
							 );

NTSYSAPI
NTSTATUS
NTAPI
RtlNtPathNameToDosPathName(
						   IN ULONG Flags,
						   IN OUT PUNICODE_STRING Path,
						   OUT PULONG Disposition,
						   IN OUT OPTIONAL PWSTR* FilePart
						   );

NTSYSAPI
VOID
NTAPI
RtlFreeUnicodeString(
					 PUNICODE_STRING UnicodeString
					 );

NTSYSAPI
PIMAGE_NT_HEADERS
NTAPI
RtlImageNtHeader(
				 __in PVOID baseAddress
				 );

NTSYSAPI
PVOID
NTAPI
RtlImageDirectoryEntryToData(
							 __in PVOID pBaseAddress,
							 __in BOOLEAN bImageLoaded,
							 __in ULONG dwDirectory,
							 __out PULONG pSize
							 );

NTSYSAPI
PVOID
NTAPI
RtlImageDirectoryEntryToDataEx(
							 __in PVOID pBaseAddress,
							 __in BOOLEAN bImageLoaded,
							 __in ULONG dwDirectory,
							 __out PULONG pSize,
							 __out_opt PIMAGE_SECTION_HEADER pSection
							 );

NTSTATUS
NTAPI
NtCreateProcess(
				OUT PHANDLE ProcessHandle,
				IN ACCESS_MASK DesiredAccess,
				IN POBJECT_ATTRIBUTES ObjectAttributes,
				IN HANDLE InheritFromProcessHandle,
				IN BOOLEAN InheritHandles,
				IN HANDLE SectionHandle OPTIONAL,
				IN HANDLE DebugPort OPTIONAL,
				IN HANDLE ExceptionPort OPTIONAL
				);
NTSYSAPI
NTSTATUS
NTAPI
NtCreatePort(
			 OUT PHANDLE PortHandle,
			 IN POBJECT_ATTRIBUTES ObjectAttributes,
			 IN ULONG MaxDataSize,
			 IN ULONG MaxMessageSize,
			 IN ULONG Reserved
			 );

NTSYSAPI
NTSTATUS
NTAPI
NtCreateWaitablePort(
			 OUT PHANDLE PortHandle,
			 IN POBJECT_ATTRIBUTES ObjectAttributes,
			 IN ULONG MaxDataSize,
			 IN ULONG MaxMessageSize,
			 IN ULONG Reserved
			 );

NTSYSAPI
NTSTATUS
NTAPI
NtReplyWaitReceivePort(
					   IN HANDLE PortHandle,
					   OUT PULONG PortIdentifier OPTIONAL,
					   IN PPORT_MESSAGE ReplyMessage OPTIONAL,
					   OUT PPORT_MESSAGE Message
					   );

NTSYSAPI
NTSTATUS
NTAPI
NtReplyPort(
		   IN HANDLE PortHandle,
		   IN PPORT_MESSAGE ReplyMessage
		   );

NTSYSAPI
NTSTATUS
NTAPI
NtCreateThread(
			   OUT PHANDLE ThreadHandle,
			   IN ACCESS_MASK DesiredAccess,
			   IN POBJECT_ATTRIBUTES ObjectAttributes,
			   IN HANDLE ProcessHandle,
			   OUT PCLIENT_ID ClientId,
			   IN PCONTEXT ThreadContext,
			   IN PINITIAL_TEB UserStack,
			   IN BOOLEAN CreateSuspended
			   );

typedef enum _SECTION_INHERIT {
    ViewShare = 1,
    ViewUnmap = 2
} SECTION_INHERIT;

NTSYSAPI
NTSTATUS
NTAPI
NtCreateSection (
    __out PHANDLE SectionHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in_opt PLARGE_INTEGER MaximumSize,
    __in ULONG SectionPageProtection,
    __in ULONG AllocationAttributes,
    __in_opt HANDLE FileHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
NtOpenSection(
    __out PHANDLE SectionHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSAPI
NTSTATUS
NTAPI
NtMapViewOfSection(
    __in HANDLE SectionHandle,
    __in HANDLE ProcessHandle,
    __inout PVOID *BaseAddress,
    __in ULONG_PTR ZeroBits,
    __in SIZE_T CommitSize,
    __inout_opt PLARGE_INTEGER SectionOffset,
    __inout PSIZE_T ViewSize,
    __in SECTION_INHERIT InheritDisposition,
    __in ULONG AllocationType,
    __in ULONG Win32Protect
    );

NTSYSAPI
NTSTATUS
NTAPI
NtUnmapViewOfSection(
					__in HANDLE ProcessHandle,
					__in_opt PVOID BaseAddress
					);

typedef enum _OBJECT_INFORMATION_CLASS {
	ObjectBasicInformation,
	ObjectNameInformation,
	ObjectTypeInformation
} OBJECT_INFORMATION_CLASS;

typedef struct _OBJECT_NAME_INFORMATION 
{
	UNICODE_STRING Name;
} OBJECT_NAME_INFORMATION, *POBJECT_NAME_INFORMATION;

typedef struct __PUBLIC_OBJECT_TYPE_INFORMATION {
	UNICODE_STRING TypeName;
	ULONG Reserved [22];    // reserved for internal use
} PUBLIC_OBJECT_TYPE_INFORMATION, *PPUBLIC_OBJECT_TYPE_INFORMATION;

typedef struct _OBJECT_TYPE_INFORMATION {
  UNICODE_STRING          TypeName;
  ULONG                   TotalNumberOfHandles;
  ULONG                   TotalNumberOfObjects;
  UNICODE_STRING		  OtherName;
  UNICODE_STRING		  OtherName2;
  ULONG                   HighWaterNumberOfHandles;
  ULONG                   HighWaterNumberOfObjects;
  UNICODE_STRING		  OtherName3;
  UNICODE_STRING		  OtherName4;
  ACCESS_MASK             InvalidAttributes;
  GENERIC_MAPPING         GenericMapping;
  ACCESS_MASK             ValidAttributes;
  BOOLEAN                 SecurityRequired;
  BOOLEAN                 MaintainHandleCount;
  USHORT                  MaintainTypeList;
  ULONG               PoolType;
  ULONG                   DefaultPagedPoolCharge;
  ULONG                   DefaultNonPagedPoolCharge;
} OBJECT_TYPE_INFORMATION, *POBJECT_TYPE_INFORMATION;

NTSYSAPI
NTSTATUS 
NTAPI 
NtQueryObject(
	__in_opt   HANDLE Handle,
	__in       OBJECT_INFORMATION_CLASS ObjectInformationClass,
	__out_opt  PVOID ObjectInformation,
	__in       ULONG ObjectInformationLength,
	__out_opt  PULONG ReturnLength
);


NTSYSAPI 
NTSTATUS
NTAPI
LdrLoadDll(
	IN PWCHAR SearchPath OPTIONAL,
	IN PULONG Flags OPTIONAL,
	IN PUNICODE_STRING ModuleFileName,
	OUT HMODULE *ModuleHandle
); 

NTSYSAPI
NTSTATUS
NTAPI
LdrUnloadDll(
			 IN HANDLE ModuleHandle
			 );

NTSYSAPI 
NTSTATUS
NTAPI
LdrGetLoadDll(
	IN PWCHAR SearchPath OPTIONAL,
	IN PULONG Flags OPTIONAL,
	IN PUNICODE_STRING ModuleFileName,
	OUT HMODULE *ModuleHandle
); 

NTSYSAPI 
NTSTATUS
NTAPI
LdrGetProcedureAddress(
					   IN HMODULE ModuleHandle,
					   IN PANSI_STRING FunctionName OPTIONAL,
					   IN WORD Oridinal OPTIONAL,
					   OUT PVOID *FunctionAddress
					   ); 

typedef enum __PROCESSINFOCLASS 
{
	ProcessBasicInformation,
	ProcessQuotaLimits,
	ProcessIoCounters,
	ProcessVmCounters,
	ProcessTimes,
	ProcessBasePriority, // invalid for query
	ProcessRaisePriority, // invalid for query
	ProcessDebugPort,
	ProcessExceptionPort, // invalid for query
	ProcessAccessToken, // invalid for query
	ProcessLdtInformation,
	ProcessLdtSize, // invalid for query
	ProcessDefaultHardErrorMode,
	ProcessIoPortHandlers,          // Note: this is kernel mode only, invalid for query
	ProcessPooledUsageAndLimits,
	ProcessWorkingSetWatch,
	ProcessUserModeIOPL, // invalid class
	ProcessEnableAlignmentFaultFixup, // invalid class
	ProcessPriorityClass,
	ProcessWx86Information,
	ProcessHandleCount, 
	ProcessAffinityMask, // invalid for query
	ProcessPriorityBoost,
	ProcessDeviceMap,
	ProcessSessionInformation,
	ProcessForegroundInformation, // invalid for query
	ProcessWow64Information,
	ProcessImageFileName,
	ProcessLUIDDeviceMapsEnabled,
	ProcessBreakOnTermination,
	ProcessDebugObjectHandle,
	ProcessDebugFlags, // EProcess->Flags.NoDebugInherit
	ProcessHandleTracing, 
	ProcessIoPriority,
	ProcessExecuteFlags,
	ProcessTlsInformation, // invalid class
	ProcessCookie,
	ProcessImageInformation, // last available on XPSP3
	ProcessCycleTime,
	ProcessPagePriority,
	ProcessInstrumentationCallback, // invalid class
	ProcessThreadStackAllocation, // invalid class
	ProcessWorkingSetWatchEx,
	ProcessImageFileNameWin32, // buffer is a UNICODE_STRING
	ProcessImageFileMapping, // buffer is a pointer to a file handle open with SYNCHRONIZE | FILE_EXECUTE access, return value is whether the handle is the same used to start the process
	ProcessAffinityUpdateMode,
	ProcessMemoryAllocationMode,
	ProcessGroupInformation,
    ProcessTokenVirtualizationEnabled, // invalid class
    ProcessConsoleHostProcess, // retrieves the pid for the process' corresponding conhost process
    ProcessWindowInformation, // returns the windowflags and windowtitle members of the process' peb->rtl_user_process_params
	MaxProcessInfoClass             // MaxProcessInfoClass should always be the last enum
} PROCESSINFOCLASS;

typedef struct _PROCESS_WINDOW_INFORMATION
{
	ULONG WindowFlags;
	USHORT titleLen;
	WCHAR title[0];
} PROCESS_WINDOW_INFORMATION;

typedef struct _PROCESS_IMAGE_INFORMATION
{
	PVOID pEntryPoint;
	PVOID unk;
	ULONG_PTR stackReserve;
	ULONG_PTR stackCommit;
	USHORT subsystem;
	USHORT unk2;
	ULONG osVersion;
	ULONG imageVersion;
	USHORT dllCharacteristics;
	USHORT fileHeaderCharacteristics;
	USHORT machine;
	USHORT unk3;
	PVOID unk4;
	PVOID unk5;
	ULONG checksum;
} PROCESS_IMAGE_INFORMATION;

//
// Process Information Structures
//

//
// Working set page priority information.
// Used with ProcessPagePriority and ThreadPagePriority
//

typedef struct _PAGE_PRIORITY_INFORMATION {
    ULONG PagePriority;
} PAGE_PRIORITY_INFORMATION, *PPAGE_PRIORITY_INFORMATION;

//
// PageFaultHistory Information
//  NtQueryInformationProcess using ProcessWorkingSetWatch
//
typedef struct _PROCESS_WS_WATCH_INFORMATION {
    PVOID FaultingPc;
    PVOID FaultingVa;
} PROCESS_WS_WATCH_INFORMATION, *PPROCESS_WS_WATCH_INFORMATION;

//
// Basic and Extended Basic Process Information
//  NtQueryInformationProcess using ProcessBasicInformation
//

typedef ULONG KPRIORITY;

typedef struct _PROCESS_BASIC_INFORMATION {
    NTSTATUS ExitStatus;
    PPEB_VISTA_7 PebBaseAddress;
    ULONG_PTR AffinityMask;
    KPRIORITY BasePriority;
    ULONG_PTR UniqueProcessId;
    ULONG_PTR InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION,*PPROCESS_BASIC_INFORMATION;

typedef struct _PROCESS_EXTENDED_BASIC_INFORMATION {
    SIZE_T Size;    // Must be set to structure size on input
    PROCESS_BASIC_INFORMATION BasicInfo;
    union {
        ULONG Flags;
        struct {
            ULONG IsProtectedProcess : 1;
            ULONG IsWow64Process : 1;
            ULONG IsProcessDeleting : 1;
            ULONG IsCrossSessionCreate : 1;
            ULONG SpareBits : 28;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
} PROCESS_EXTENDED_BASIC_INFORMATION, *PPROCESS_EXTENDED_BASIC_INFORMATION;

//
// Process Device Map information
//  NtQueryInformationProcess using ProcessDeviceMap
//  NtSetInformationProcess using ProcessDeviceMap
//

typedef struct _PROCESS_DEVICEMAP_INFORMATION {
    union {
        struct {
            HANDLE DirectoryHandle;
        } Set;
        struct {
            ULONG DriveMap;
            UCHAR DriveType[ 32 ];
        } Query;
    } DUMMYUNIONNAME;
} PROCESS_DEVICEMAP_INFORMATION, *PPROCESS_DEVICEMAP_INFORMATION;

typedef struct _PROCESS_DEVICEMAP_INFORMATION_EX {
    union {
        struct {
            HANDLE DirectoryHandle;
        } Set;
        struct {
            ULONG DriveMap;
            UCHAR DriveType[ 32 ];
        } Query;
    } DUMMYUNIONNAME;
    ULONG Flags;    // specifies that the query type
} PROCESS_DEVICEMAP_INFORMATION_EX, *PPROCESS_DEVICEMAP_INFORMATION_EX;

//
// PROCESS_DEVICEMAP_INFORMATION_EX flags
//
#define PROCESS_LUID_DOSDEVICES_ONLY 0x00000001

//
// Multi-User Session specific Process Information
//  NtQueryInformationProcess using ProcessSessionInformation
//

typedef struct _PROCESS_SESSION_INFORMATION {
    ULONG SessionId;
} PROCESS_SESSION_INFORMATION, *PPROCESS_SESSION_INFORMATION;

typedef struct _PROCESS_HANDLE_TRACING_ENABLE {
    ULONG Flags;
} PROCESS_HANDLE_TRACING_ENABLE, *PPROCESS_HANDLE_TRACING_ENABLE;

typedef struct _PROCESS_HANDLE_TRACING_ENABLE_EX {
    ULONG Flags;
    ULONG TotalSlots;
} PROCESS_HANDLE_TRACING_ENABLE_EX, *PPROCESS_HANDLE_TRACING_ENABLE_EX;


#define PROCESS_HANDLE_TRACING_MAX_STACKS 16

typedef struct _PROCESS_HANDLE_TRACING_ENTRY {
    HANDLE Handle;
    CLIENT_ID ClientId;
    ULONG Type;
    PVOID Stacks[PROCESS_HANDLE_TRACING_MAX_STACKS];
} PROCESS_HANDLE_TRACING_ENTRY, *PPROCESS_HANDLE_TRACING_ENTRY;

typedef struct _PROCESS_HANDLE_TRACING_QUERY {
    HANDLE Handle;
    ULONG  TotalTraces;
    PROCESS_HANDLE_TRACING_ENTRY HandleTrace[1];
} PROCESS_HANDLE_TRACING_QUERY, *PPROCESS_HANDLE_TRACING_QUERY;

//
// Process Quotas
//  NtQueryInformationProcess using ProcessQuotaLimits
//  NtQueryInformationProcess using ProcessPooledQuotaLimits
//  NtSetInformationProcess using ProcessQuotaLimits
//

#define QUOTA_LIMITS_HARDWS_MIN_ENABLE  0x00000001
#define QUOTA_LIMITS_HARDWS_MIN_DISABLE 0x00000002
#define QUOTA_LIMITS_HARDWS_MAX_ENABLE  0x00000004
#define QUOTA_LIMITS_HARDWS_MAX_DISABLE 0x00000008
#define QUOTA_LIMITS_USE_DEFAULT_LIMITS 0x00000010

//
// Process Security Context Information
//  NtSetInformationProcess using ProcessAccessToken
// PROCESS_SET_ACCESS_TOKEN access to the process is needed
// to use this info level.
//

typedef struct _PROCESS_ACCESS_TOKEN {

    //
    // Handle to Primary token to assign to the process.
    // TOKEN_ASSIGN_PRIMARY access to this token is needed.
    //

    HANDLE Token;

    //
    // Handle to the initial thread of the process.
    // A process's access token can only be changed if the process has
    // no threads or a single thread that has not yet begun execution.
    //
    // N.B. This field is unused.
    //

    HANDLE Thread;

} PROCESS_ACCESS_TOKEN, *PPROCESS_ACCESS_TOKEN;

//
// Process Exception Port Information
//  NtSetInformationProcess using ProcessExceptionPort
// PROCESS_SET_PORT access to the process is needed
// to use this info level.
//

#define PROCESS_EXCEPTION_PORT_ALL_STATE_BITS     0x00000003UL
#define PROCESS_EXCEPTION_PORT_ALL_STATE_FLAGS    ((ULONG_PTR)((1UL << PROCESS_EXCEPTION_PORT_ALL_STATE_BITS) - 1))

typedef struct _PROCESS_EXCEPTION_PORT {

    //
    // Handle to the exception port. No particular access required.
    //

    IN HANDLE ExceptionPortHandle;

    //
    // Miscellaneous state flags to be cached along with the exception
    // port in the kernel.
    //

    IN OUT ULONG StateFlags;

} PROCESS_EXCEPTION_PORT, *PPROCESS_EXCEPTION_PORT;

//
// Process/Thread System and User Time
//  NtQueryInformationProcess using ProcessTimes
//  NtQueryInformationThread using ThreadTimes
//

typedef struct _KERNEL_USER_TIMES {
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER ExitTime;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
} KERNEL_USER_TIMES;
typedef KERNEL_USER_TIMES *PKERNEL_USER_TIMES;

//
// Thread Information Classes
//

typedef enum _THREADINFOCLASS {
    ThreadBasicInformation,
    ThreadTimes,
    ThreadPriority,
    ThreadBasePriority,
    ThreadAffinityMask,
    ThreadImpersonationToken,
    ThreadDescriptorTableEntry,
    ThreadEnableAlignmentFaultFixup,
    ThreadEventPair_Reusable,
    ThreadQuerySetWin32StartAddress,
    ThreadZeroTlsCell,
    ThreadPerformanceCount,
    ThreadAmILastThread,
    ThreadIdealProcessor,
    ThreadPriorityBoost,
    ThreadSetTlsArrayAddress,   // Obsolete
    ThreadIsIoPending,
    ThreadHideFromDebugger,
    ThreadBreakOnTermination,
    ThreadSwitchLegacyState,
    ThreadIsTerminated,
    ThreadLastSystemCall,
    ThreadIoPriority,
    ThreadCycleTime,
    ThreadPagePriority,
    ThreadActualBasePriority,
    ThreadTebInformation,
    ThreadCSwitchMon,          // Obsolete
    ThreadCSwitchPmu,
    ThreadWow64Context,
    ThreadGroupInformation,
    ThreadUmsInformation,      // UMS
    ThreadCounterProfiling,
    ThreadIdealProcessorEx,
    MaxThreadInfoClass
} THREADINFOCLASS;

typedef struct _THREAD_BASIC_INFORMATION 
{
	NTSTATUS ExitStatus; 
	PVOID TebBaseAddress; 
	CLIENT_ID ClientId; 
	KAFFINITY AffinityMask; 
	KPRIORITY Priority; 
	KPRIORITY BasePriority;
} THREAD_BASIC_INFORMATION, *PTHREAD_BASIC_INFORMATION;

typedef struct _THREAD_TIMES_INFORMATION 
{
	LARGE_INTEGER CreationTime;
	LARGE_INTEGER ExitTime; 
	LARGE_INTEGER KernelTime;
	LARGE_INTEGER UserTime;
} THREAD_TIMES_INFORMATION, *PTHREAD_TIMES_INFORMATION;

//typedef enum _SYSTEM_INFORMATION_CLASS {
//    SystemBasicInformation,
//    SystemProcessorInformation,
//    SystemPerformanceInformation,
//    SystemTimeOfDayInformation,
//    SystemPathInformation,
//    SystemProcessInformation,
//    SystemCallCountInformation,
//    SystemDeviceInformation,
//    SystemProcessorPerformanceInformation,
//    SystemFlagsInformation,
//    SystemCallTimeInformation,
//    SystemModuleInformation,
//    SystemLocksInformation,
//    SystemStackTraceInformation,
//    SystemPagedPoolInformation,
//    SystemNonPagedPoolInformation,
//    SystemHandleInformation,
//    SystemObjectInformation,
//    SystemPageFileInformation,
//    SystemVdmInstemulInformation,
//    SystemVdmBopInformation,
//    SystemFileCacheInformation,
//    SystemPoolTagInformation,
//    SystemInterruptInformation,
//    SystemDpcBehaviorInformation,
//    SystemFullMemoryInformation,
//    SystemLoadGdiDriverInformation,
//    SystemUnloadGdiDriverInformation,
//    SystemTimeAdjustmentInformation,
//    SystemSummaryMemoryInformation,
//    SystemNextEventIdInformation,
//    SystemEventIdsInformation,
//    SystemCrashDumpInformation,
//    SystemExceptionInformation,
//    SystemCrashDumpStateInformation,
//    SystemKernelDebuggerInformation,
//    SystemContextSwitchInformation,
//    SystemRegistryQuotaInformation,
//    SystemExtendServiceTableInformation,
//    SystemPrioritySeperation,
//    SystemPlugPlayBusInformation,
//    SystemDockInformation,
//	__if_exists(SystemPowerInformation)
//	{
//		SIC_SystemPowerInformation,
//	}
//	__if_not_exists(SystemPowerInformation)
//	{
//		SystemPowerInformation,
//	}
//    SystemProcessorSpeedInformation,
//    SystemCurrentTimeZoneInformation,
//    SystemLookasideInformation
//} SYSTEM_INFORMATION_CLASS, *PSYSTEM_INFORMATION_CLASS;
//
//typedef enum _SYSTEM_INFORMATION_CLASS
//{
//    SystemBasicInformation,
//    SystemProcessorInformation,
//    SystemPerformanceInformation,
//    SystemTimeOfDayInformation,
//    SystemPathInformation,
//    SystemProcessInformation,
//    SystemCallCountInformation,
//    SystemDeviceInformation,
//    SystemProcessorPerformanceInformation,
//    SystemFlagsInformation,
//    SystemCallTimeInformation,
//    SystemModuleInformation,
//    SystemLocksInformation,
//    SystemStackTraceInformation,
//    SystemPagedPoolInformation,
//    SystemNonPagedPoolInformation,
//    SystemHandleInformation,
//    SystemObjectInformation,
//    SystemPageFileInformation,
//    SystemVdmInstemulInformation,
//    SystemVdmBopInformation,
//    SystemFileCacheInformation,
//    SystemPoolTagInformation,
//    SystemInterruptInformation,
//    SystemDpcBehaviorInformation,
//    SystemFullMemoryInformation,
//    SystemLoadGdiDriverInformation,
//    SystemUnloadGdiDriverInformation,
//    SystemTimeAdjustmentInformation,
//    SystemSummaryMemoryInformation,
//    SystemMirrorMemoryInformation,
//    SystemPerformanceTraceInformation,
//    SystemCrashDumpInformation,
//    SystemExceptionInformation,
//    SystemCrashDumpStateInformation,
//    SystemKernelDebuggerInformation,
//    SystemContextSwitchInformation,
//    SystemRegistryQuotaInformation,
//    SystemExtendServiceTableInformation, // used to be SystemLoadAndCallImage
//    SystemPrioritySeparation,
//    SystemVerifierAddDriverInformation,
//    SystemVerifierRemoveDriverInformation,
//    SystemProcessorIdleInformation,
//    SystemLegacyDriverInformation,
//    SystemCurrentTimeZoneInformation,
//    SystemLookasideInformation,
//    SystemTimeSlipNotification,
//    SystemSessionCreate,
//    SystemSessionDetach,
//    SystemSessionInformation,
//    SystemRangeStartInformation,
//    SystemVerifierInformation,
//    SystemVerifierThunkExtend,
//    SystemSessionProcessInformation,
//    SystemLoadGdiDriverInSystemSpace,
//    SystemNumaProcessorMap,
//    SystemPrefetcherInformation,
//    SystemExtendedProcessInformation,
//    SystemRecommendedSharedDataAlignment,
//    SystemComPlusPackage,
//    SystemNumaAvailableMemory, // 60
//    SystemProcessorPowerInformation,
//    SystemEmulationBasicInformation,
//    SystemEmulationProcessorInformation,
//    SystemExtendedHandleInformation,
//    SystemLostDelayedWriteInformation,
//    SystemBigPoolInformation,
//    SystemSessionPoolTagInformation,
//    SystemSessionMappedViewInformation,
//    SystemHotpatchInformation,
//    SystemObjectSecurityMode, // 70
//    SystemWatchdogTimerHandler, // doesn't seem to be implemented
//    SystemWatchdogTimerInformation,
//    SystemLogicalProcessorInformation,
//    SystemWow64SharedInformation,
//    SystemRegisterFirmwareTableInformationHandler,
//    SystemFirmwareTableInformation,
//    SystemModuleInformationEx,
//    SystemVerifierTriageInformation,
//    SystemSuperfetchInformation,
//    SystemMemoryListInformation, // 80
//    SystemFileCacheInformationEx,
//    SystemNotImplemented19,
//    SystemProcessorDebugInformation,
//    SystemVerifierInformation2,
//    SystemNotImplemented20,
//    SystemRefTraceInformation,
//    SystemSpecialPoolTag, // MmSpecialPoolTag, then MmSpecialPoolCatchOverruns != 0
//    SystemProcessImageName,
//    SystemNotImplemented21,
//    SystemBootEnvironmentInformation, // 90
//    SystemEnlightenmentInformation,
//    SystemVerifierInformationEx,
//    SystemNotImplemented22,
//    SystemNotImplemented23,
//    SystemCovInformation,
//    SystemNotImplemented24,
//    SystemNotImplemented25,
//    SystemPartitionInformation,
//    SystemSystemDiskInformation, // this and SystemPartitionInformation both call IoQuerySystemDeviceName
//    SystemPerformanceDistributionInformation, // 100
//    SystemNumaProximityNodeInformation,
//    SystemTimeZoneInformation2,
//    SystemCodeIntegrityInformation,
//    SystemNotImplemented26,
//    SystemUnknownInformation, // No symbols for this case, very strange...
//    SystemVaInformation // 106, calls MmQuerySystemVaInformation
//} SYSTEM_INFORMATION_CLASS, *PSYSTEM_INFORMATION_CLASS;

typedef enum _SYSTEM_INFORMATION_CLASS
{
    SystemBasicInformation,
    SystemProcessorInformation,
    SystemPerformanceInformation,
    SystemTimeOfDayInformation,
    SystemPathInformation, /// Obsolete: Use KUSER_SHARED_DATA
    SystemProcessInformation,
    SystemCallCountInformation,
    SystemDeviceInformation,
    SystemProcessorPerformanceInformation,
    SystemFlagsInformation,
    SystemCallTimeInformation,
    SystemModuleInformation,
    SystemLocksInformation,
    SystemStackTraceInformation,
    SystemPagedPoolInformation,
    SystemNonPagedPoolInformation,
    SystemHandleInformation,
    SystemObjectInformation,
    SystemPageFileInformation,
    SystemVdmInstemulInformation,
    SystemVdmBopInformation,
    SystemFileCacheInformation,
    SystemPoolTagInformation,
    SystemInterruptInformation,
    SystemDpcBehaviorInformation,
    SystemFullMemoryInformation,
    SystemLoadGdiDriverInformation,
    SystemUnloadGdiDriverInformation,
    SystemTimeAdjustmentInformation,
    SystemSummaryMemoryInformation,
    SystemMirrorMemoryInformation,
    SystemPerformanceTraceInformation,
    SystemObsolete0,
    SystemExceptionInformation,
    SystemCrashDumpStateInformation,
    SystemKernelDebuggerInformation,
    SystemContextSwitchInformation,
    SystemRegistryQuotaInformation,
    SystemExtendServiceTableInformation,  // used to be SystemLoadAndCallImage
    SystemPrioritySeperation,
    SystemPlugPlayBusInformation,
    SystemDockInformation,
    SystemPowerInformationNative,
    SystemProcessorSpeedInformation,
    SystemCurrentTimeZoneInformation,
    SystemLookasideInformation,
    SystemTimeSlipNotification,
    SystemSessionCreate,
    SystemSessionDetach,
    SystemSessionInformation,
    SystemRangeStartInformation,
    SystemVerifierInformation,
    SystemAddVerifier,
    SystemSessionProcessesInformation,
    SystemLoadGdiDriverInSystemSpaceInformation,
    SystemNumaProcessorMap,
    SystemPrefetcherInformation,
    SystemExtendedProcessInformation,
    SystemRecommendedSharedDataAlignment,
    SystemComPlusPackage,
    SystemNumaAvailableMemory,
    SystemProcessorPowerInformation,
    SystemEmulationBasicInformation,
    SystemEmulationProcessorInformation,
    SystemExtendedHanfleInformation,
    SystemLostDelayedWriteInformation,
    SystemBigPoolInformation,
    SystemSessionPoolTagInformation,
    SystemSessionMappedViewInformation,
    SystemHotpatchInformation,
    SystemObjectSecurityMode,
    SystemWatchDogTimerHandler,
    SystemWatchDogTimerInformation,
    SystemLogicalProcessorInformation,
    SystemWo64SharedInformationObosolete,
    SystemRegisterFirmwareTableInformationHandler,
    SystemFirmwareTableInformation,
    SystemModuleInformationEx,
    SystemVerifierTriageInformation,
    SystemSuperfetchInformation,
    SystemMemoryListInformation,
    SystemFileCacheInformationEx,
    SystemThreadPriorityClientIdInformation,
    SystemProcessorIdleCycleTimeInformation,
    SystemVerifierCancellationInformation,
    SystemProcessorPowerInformationEx,
    SystemRefTraceInformation,
    SystemSpecialPoolInformation,
    SystemProcessIdInformation,
    SystemErrorPortInformation,
    SystemBootEnvironmentInformation,
    SystemHypervisorInformation,
    SystemVerifierInformationEx,
    SystemTimeZoneInformation,
    SystemImageFileExecutionOptionsInformation,
    SystemCoverageInformation,
    SystemPrefetchPathInformation,
    SystemVerifierFaultsInformation,
    MaxSystemInfoClass,
} SYSTEM_INFORMATION_CLASS;


typedef struct _SYSTEM_MODULE 
{
	ULONG                Reserved1;
	ULONG                Reserved2;
	PVOID                ImageBaseAddress;
	ULONG                ImageSize;
	ULONG                Flags;
	WORD                 Id;
	WORD                 Rank;
	WORD                 w018;
	WORD                 NameOffset;
	BYTE                 Name[256];
} SYSTEM_MODULE, *PSYSTEM_MODULE;

typedef struct _SYSTEM_MODULE_INFORMATION 
{
	ULONG                ModulesCount;
	SYSTEM_MODULE        Modules[0];
} SYSTEM_MODULE_INFORMATION, *PSYSTEM_MODULE_INFORMATION;

//typedef enum _SYSTEM_INFORMATION_CLASS {
//    SystemBasicInformation = 0,
//    SystemPerformanceInformation = 2,
//    SystemTimeOfDayInformation = 3,
//    SystemProcessInformation = 5,
//    SystemProcessorPerformanceInformation = 8,
//	SystemModuleInformation = 11,
//	SystemHandleInformation = 16,
//    SystemInterruptInformation = 23,
//    SystemExceptionInformation = 33,
//    SystemRegistryQuotaInformation = 37,
//    SystemLookasideInformation = 45
//} SYSTEM_INFORMATION_CLASS;

NTSTATUS NTAPI NtQueryInformationProcess(
	HANDLE ProcessHandle,
	PROCESSINFOCLASS ProcessInformationClass,
	PVOID ProcessInformation,
	ULONG ProcessInformationLength,
	PULONG ReturnLength
);

NTSTATUS NTAPI NtSetInformationProcess(
	HANDLE ProcessHandle,
	PROCESSINFOCLASS ProcessInformationClass,
	PVOID ProcessInformation,
	ULONG ProcessInformationLength
);

NTSTATUS NTAPI NtQueryInformationThread (
	IN HANDLE ThreadHandle,
	IN THREADINFOCLASS ThreadInformationClass,
	OUT PVOID ThreadInformation,
	IN ULONG ThreadInformationLength,
	OUT PULONG ReturnLength OPTIONAL
);

NTSTATUS WINAPI NtQuerySystemInformation (
	IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
	OUT PVOID SystemInformation,
	IN ULONG SystemInformationLength,
	OUT PULONG ReturnLength OPTIONAL
);

typedef struct _SYSTEM_REGISTRY_QUOTA_INFORMATION {
    ULONG RegistryQuotaAllowed;
    ULONG RegistryQuotaUsed;
    PVOID Reserved1;
} SYSTEM_REGISTRY_QUOTA_INFORMATION, *PSYSTEM_REGISTRY_QUOTA_INFORMATION;

typedef struct _SYSTEM_BASIC_INFORMATION {
    BYTE Reserved1[24];
    PVOID Reserved2[4];
    CCHAR NumberOfProcessors;
} SYSTEM_BASIC_INFORMATION, *PSYSTEM_BASIC_INFORMATION;

typedef struct _SYSTEM_TIMEOFDAY_INFORMATION {
    BYTE Reserved1[48];
} SYSTEM_TIMEOFDAY_INFORMATION, *PSYSTEM_TIMEOFDAY_INFORMATION;

typedef struct _SYSTEM_PERFORMANCE_INFORMATION {
    BYTE Reserved1[312];
} SYSTEM_PERFORMANCE_INFORMATION, *PSYSTEM_PERFORMANCE_INFORMATION;

typedef struct _SYSTEM_EXCEPTION_INFORMATION {
    BYTE Reserved1[16];
} SYSTEM_EXCEPTION_INFORMATION, *PSYSTEM_EXCEPTION_INFORMATION;

typedef struct _SYSTEM_LOOKASIDE_INFORMATION {
    BYTE Reserved1[32];
} SYSTEM_LOOKASIDE_INFORMATION, *PSYSTEM_LOOKASIDE_INFORMATION;

typedef struct _SYSTEM_INTERRUPT_INFORMATION {
    BYTE Reserved1[24];
} SYSTEM_INTERRUPT_INFORMATION, *PSYSTEM_INTERRUPT_INFORMATION;

typedef struct _SYSTEM_HANDLE_INFORMATION
{
	ULONG	ProcessId;
	UCHAR	ObjectTypeNumber;
	UCHAR	Flags;
	USHORT	Handle;
	PVOID	Object;
	ACCESS_MASK	GrantedAccess;
} SYSTEM_HANDLE_INFORMATION, *PSYSTEM_HANDLE_INFORMATION;

typedef struct _VM_COUNTERS {
    SIZE_T        PeakVirtualSize;
    SIZE_T        VirtualSize;
    ULONG         PageFaultCount;
    SIZE_T        PeakWorkingSetSize;
    SIZE_T        WorkingSetSize;
    SIZE_T        QuotaPeakPagedPoolUsage;
    SIZE_T        QuotaPagedPoolUsage;
    SIZE_T        QuotaPeakNonPagedPoolUsage;
    SIZE_T        QuotaNonPagedPoolUsage;
    SIZE_T        PagefileUsage;
    SIZE_T        PeakPagefileUsage;
	SIZE_T		  PrivatePageCount;
} VM_COUNTERS;

typedef struct _SYSTEM_THREAD_INFORMATION {
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER CreateTime;
    ULONG         WaitTime;
    PVOID         StartAddress;
    CLIENT_ID     ClientId;
    KPRIORITY     Priority;
    KPRIORITY     BasePriority;
    ULONG         ContextSwitchCount;
    LONG          State;
    LONG          WaitReason;
} SYSTEM_THREAD_INFORMATION, * PSYSTEM_THREAD_INFORMATION;

typedef struct _SYSTEM_PROCESS_INFORMATION {
    ULONG             NextEntryDelta;
    ULONG             ThreadCount;
    LARGE_INTEGER     Reserved1[3];
    LARGE_INTEGER     CreateTime;
    LARGE_INTEGER     UserTime;
    LARGE_INTEGER     KernelTime;
    UNICODE_STRING    ProcessName;
    KPRIORITY         BasePriority;
    HANDLE            ProcessId;
    HANDLE            InheritedFromProcessId;
    ULONG             HandleCount;
	ULONG             SessionId;
    ULONG_PTR         PageDirectoryBase;
    VM_COUNTERS       VmCounters;
#if _WIN32_WINNT >= 0x500
    IO_COUNTERS       IoCounters;
#endif
    SYSTEM_THREAD_INFORMATION Threads[1];
} SYSTEM_PROCESS_INFORMATION, * PSYSTEM_PROCESS_INFORMATION;

typedef struct _ACTIVATION_CONTEXT_STACK
{
	ULONG Flags;
	ULONG NextCookieSequenceNumber;
	PVOID ActiveFrame;
	LIST_ENTRY FrameListCache;
} ACTIVATION_CONTEXT_STACK;

typedef struct _GDI_TEB_BATCH
{
	ULONG Offset;
	HDC hdc;
	ULONG buffer[310];
} GDI_TEB_BATCH;

typedef struct _Wx86ThreadState
{
	PULONG CallBx86Eip;
	PVOID DeallocationCpu;
	BOOLEAN UseKnownWx86Dll;
	BOOLEAN OleStubInvoked;
} Wx86ThreadState;

typedef struct _TEB_ACTIVE_FRAME_CONTEXT
{
	ULONG Flags;
	char* FrameName;
} TEB_ACTIVE_FRAME_CONTEXT;

typedef struct _TEB_ACTIVE_FRAME
{
	ULONG Flags;
	struct _TEB_ACTIVE_FRAME* Previous;
	TEB_ACTIVE_FRAME_CONTEXT* Context;
} TEB_ACTIVE_FRAME;

// XP SP3 layout
typedef struct _TEB_XP
{
	NT_TIB					NtTib;
	PVOID                   EnvironmentPointer;
	CLIENT_ID               Cid;
	PVOID                   ActiveRpcInfo;
	PVOID                   ThreadLocalStoragePointer;
	PPEB_XP                 Peb;
	ULONG                   LastErrorValue;
	ULONG                   CountOfOwnedCriticalSections;
	PVOID                   CsrClientThread;
	PVOID                   Win32ThreadInfo;
	ULONG                   User32Reserved[0x1a];
	ULONG                   UserReserved[0x5];
	PVOID                   WOW32Reserved;
	ULONG                   CurrentLocale;
	ULONG                   FpSoftwareStatusRegister;
	PVOID                   SystemReserved1[0x36];
	ULONG                   ExceptionCode;
	ACTIVATION_CONTEXT_STACK ActivationContextStack;
	BYTE                    SpareBytes1[0x18];
	GDI_TEB_BATCH			GdiTebBatch;
	CLIENT_ID               RealClientId;
	HANDLE					GdiCachedProcessHandle;
	ULONG					GdiClientPID;
	ULONG					GdiClientTID;
	PVOID                   GdiThreadLocaleInfo;
	UINT_PTR                Win32ClientInfo[0x3e];
	PVOID                   GlDispatchTable[0xe9];	
	UINT_PTR                GlReserved1[0x1d];
	PVOID                   GlReserved2;
	PVOID                   GlSectionInfo;
	PVOID                   GlSection;
	PVOID                   GlTable;
	PVOID                   GlCurrentRC;
	PVOID                   GlContext;
	NTSTATUS                LastStatusValue;
	UNICODE_STRING          StaticUnicodeString;
	WCHAR                   StaticUnicodeBuffer[0x105];
	PVOID                   DeallocationStack;
	PVOID                   TlsSlots[0x40];
	LIST_ENTRY              TlsLinks;
	PVOID                   Vdm;
	PVOID                   ReservedForNtRpc;
	PVOID                   DbgSsReserved[0x2];
	ULONG                   HardErrorsAreDisabled;
	PVOID                   Instrumentation[0x10];
	PVOID                   WinSockData;
	ULONG                   GdiBatchCount;
	BOOLEAN                 InDbgPrint;
	BOOLEAN                 FreeStackOnTermination;
	BOOLEAN                 HasFiberData;
	BOOLEAN                 IdealProcessor;
	ULONG					Spare3;
	PVOID                   ReservedForPerf;
	PVOID                   ReservedForOle;
	ULONG                   WaitingOnLoaderLock;
	Wx86ThreadState			Wx86Thread;
	PVOID*                  TlsExpansionSlots;
	ULONG                   ImpersonationLocale;
	ULONG                   IsImpersonating;
	PVOID                   NlsCache;
	PVOID                   pShimData;
	ULONG                   HeapVirtualAffinity;
	HANDLE                  CurrentTransactionHandle;
	TEB_ACTIVE_FRAME*       ActiveFrame;
	BOOLEAN                 SafeThunkCall;
	BOOLEAN					BooleanSpare[3];
} TEB_XP, *PTEB_XP;

// Win Vista SP0 layout
typedef struct _TEB_VISTA
{
	NT_TIB					NtTib;
	PVOID                   EnvironmentPointer;
	CLIENT_ID               Cid;
	PVOID                   ActiveRpcInfo;
	PVOID                   ThreadLocalStoragePointer;
	PPEB_VISTA_7            Peb;
	ULONG                   LastErrorValue;
	ULONG                   CountOfOwnedCriticalSections;
	PVOID                   CsrClientThread;
	PVOID                   Win32ThreadInfo;
	ULONG                   User32Reserved[0x1a];
	ULONG                   UserReserved[0x5];
	PVOID                   WOW32Reserved;
	ULONG                   CurrentLocale;
	ULONG                   FpSoftwareStatusRegister;
	PVOID                   SystemReserved1[0x36];
	ULONG                   ExceptionCode;
	ACTIVATION_CONTEXT_STACK* ActivationContextStack;
#ifdef _WIN64
	BYTE                    SpareBytes1[0x18];
#else
	BYTE                    SpareBytes1[0x24];
#endif
	ULONG					TxFsContext;
	GDI_TEB_BATCH			GdiTebBatch;
	CLIENT_ID               RealClientId;
	HANDLE					GdiCachedProcessHandle;
	ULONG					GdiClientPID;
	ULONG					GdiClientTID;
	PVOID                   GdiThreadLocaleInfo;
	UINT_PTR                Win32ClientInfo[0x3e];
	PVOID                   GlDispatchTable[0xe9];	
	UINT_PTR                GlReserved1[0x1d];
	PVOID                   GlReserved2;
	PVOID                   GlSectionInfo;
	PVOID                   GlSection;
	PVOID                   GlTable;
	PVOID                   GlCurrentRC;
	PVOID                   GlContext;
	NTSTATUS                LastStatusValue;
	UNICODE_STRING          StaticUnicodeString;
	WCHAR                   StaticUnicodeBuffer[0x105];
	PVOID                   DeallocationStack;
	PVOID                   TlsSlots[0x40];
	LIST_ENTRY              TlsLinks;
	PVOID                   Vdm;
	PVOID                   ReservedForNtRpc;
	PVOID                   DbgSsReserved[0x2];
	ULONG                   HardErrorsAreDisabled;
#ifdef _WIN64
	PVOID                   Instrumentation[0xB];
#else
	PVOID                   Instrumentation[0x9];
#endif
	GUID                    ActivityId;
	PVOID                   EtwLocalData;
	PVOID                   EtwTraceData;
	PVOID					WinSockData;
	ULONG                   GdiBatchCount;
	BOOLEAN                 SpareBool0;
	BOOLEAN                 SpareBool1;
	BOOLEAN                 SpareBool2;
	BOOLEAN                 IdealProcessor;
	ULONG                   GuaranteedStackBytes;
	PVOID                   ReservedForPerf;
	PVOID                   ReservedForOle;
	ULONG                   WaitingOnLoaderLock;
	PVOID                   SavedPriorityState;
	ULONG_PTR				SoftPatchPtr1;
	PVOID					ThreadPoolData;
	PVOID*                  TlsExpansionSlots;
	ULONG                   ImpersonationLocale;
	ULONG                   IsImpersonating;
	PVOID                   NlsCache;
	PVOID                   pShimData;
	ULONG                   HeapVirtualAffinity;
	HANDLE                  CurrentTransactionHandle;
	TEB_ACTIVE_FRAME*       ActiveFrame;
	PVOID					FlsData;
	PVOID					PreferredLanguages;
	PVOID					UserPrefLanguages;
	PVOID					MergedPrefLanguages;
	ULONG					MuiImpersonation;
#include <pshpack1.h>
	union
	{
		volatile USHORT		CrossTebFlags;
		USHORT				SpareCrossTebBits: 0x10;
	};
	union
	{
		USHORT				SameTebFlags;
		struct
		{
			USHORT          DbgSafeThunkCall: 1;
			USHORT          DbgInDbgPrint: 1;
			USHORT          DbgHasFiberData: 1;
			USHORT          DbgSkipThreadAttach: 1;
			USHORT          DbgWerInShipAssertCode: 1;
			USHORT          DbgRanProcessInit: 1;
			USHORT          DbgClonedThread: 1;
			USHORT          DbgSuppressDebugMsg: 1;
			USHORT          SpareSameTebBits: 8;
		};
	};
#include <poppack.h>
	PVOID					TxnScopeEnterCallback;
	PVOID					TxnScopeExitCallback;
	PVOID					TxnScopeContext;
	ULONG					LockCount;
	ULONG					ProcessRundown;
	UINT64					LastSwitchTime;
	UINT64					TotalSwitchOutTime;
	LARGE_INTEGER			WaitReasonBitmap;
} TEB_VISTA, *PTEB_VISTA;

// Win Vista SP1 & SP2 layout
typedef struct _TEB_VISTA_SP
{
	NT_TIB					NtTib;
	PVOID                   EnvironmentPointer;
	CLIENT_ID               Cid;
	PVOID                   ActiveRpcInfo;
	PVOID                   ThreadLocalStoragePointer;
	PPEB_VISTA_7            Peb;
	ULONG                   LastErrorValue;
	ULONG                   CountOfOwnedCriticalSections;
	PVOID                   CsrClientThread;
	PVOID                   Win32ThreadInfo;
	ULONG                   User32Reserved[0x1a];
	ULONG                   UserReserved[0x5];
	PVOID                   WOW32Reserved;
	ULONG                   CurrentLocale;
	ULONG                   FpSoftwareStatusRegister;
	PVOID                   SystemReserved1[0x36];
	ULONG                   ExceptionCode;
	ACTIVATION_CONTEXT_STACK* ActivationContextStack;
#ifdef _WIN64
	BYTE                    SpareBytes1[0x18];
#else
	BYTE                    SpareBytes1[0x24];
#endif
	ULONG					TxFsContext;
	GDI_TEB_BATCH			GdiTebBatch;
	CLIENT_ID               RealClientId;
	HANDLE					GdiCachedProcessHandle;
	ULONG					GdiClientPID;
	ULONG					GdiClientTID;
	PVOID                   GdiThreadLocaleInfo;
	UINT_PTR                Win32ClientInfo[0x3e];
	PVOID                   GlDispatchTable[0xe9];	
	UINT_PTR                GlReserved1[0x1d];
	PVOID                   GlReserved2;
	PVOID                   GlSectionInfo;
	PVOID                   GlSection;
	PVOID                   GlTable;
	PVOID                   GlCurrentRC;
	PVOID                   GlContext;
	NTSTATUS                LastStatusValue;
	UNICODE_STRING          StaticUnicodeString;
	WCHAR                   StaticUnicodeBuffer[0x105];
	PVOID                   DeallocationStack;
	PVOID                   TlsSlots[0x40];
	LIST_ENTRY              TlsLinks;
	PVOID                   Vdm;
	PVOID                   ReservedForNtRpc;
	PVOID                   DbgSsReserved[0x2];
	ULONG                   HardErrorsAreDisabled;
#ifdef _WIN64
	PVOID                   Instrumentation[0xB];
#else
	PVOID                   Instrumentation[0x9];
#endif
	GUID                    ActivityId;
	PVOID                   EtwLocalData;
	PVOID                   EtwTraceData;
	PVOID					WinSockData;
	union
	{
		ULONG               GdiBatchCount;
		_TEB_VISTA_SP*		pTeb64;
	};
	BOOLEAN                 SpareBool0;
	BOOLEAN                 SpareBool1;
	BOOLEAN                 SpareBool2;
	BOOLEAN                 IdealProcessor;
	ULONG                   GuaranteedStackBytes;
	PVOID                   ReservedForPerf;
	PVOID                   ReservedForOle;
	ULONG                   WaitingOnLoaderLock;
	PVOID                   SavedPriorityState;
	ULONG_PTR				SoftPatchPtr1;
	PVOID					ThreadPoolData;
	PVOID*                  TlsExpansionSlots;
	ULONG                   ImpersonationLocale;
	ULONG                   IsImpersonating;
	PVOID                   NlsCache;
	PVOID                   pShimData;
	ULONG                   HeapVirtualAffinity;
	HANDLE                  CurrentTransactionHandle;
	TEB_ACTIVE_FRAME*       ActiveFrame;
	PVOID					FlsData;
	PVOID					PreferredLanguages;
	PVOID					UserPrefLanguages;
	PVOID					MergedPrefLanguages;
	ULONG					MuiImpersonation;
#include <pshpack1.h>
	union
	{
		volatile USHORT		CrossTebFlags;
		USHORT				SpareCrossTebBits: 0x10;
	};
	union
	{
		USHORT				SameTebFlags;
		struct
		{
			USHORT          DbgSafeThunkCall: 1;
			USHORT          DbgInDbgPrint: 1;
			USHORT          DbgHasFiberData: 1;
			USHORT          DbgSkipThreadAttach: 1;
			USHORT          DbgWerInShipAssertCode: 1;
			USHORT          DbgRanProcessInit: 1;
			USHORT          DbgClonedThread: 1;
			USHORT          DbgSuppressDebugMsg: 1;
			USHORT          RtlDisableUserStackWalk: 1;
			USHORT          DbgRtlExceptionAttached: 1;
			USHORT          SpareSameTebBits: 6;
		};
	};
#include <poppack.h>
	PVOID					TxnScopeEnterCallback;
	PVOID					TxnScopeExitCallback;
	PVOID					TxnScopeContext;
	ULONG					LockCount;
	ULONG					ProcessRundown;
	UINT64					LastSwitchTime;
	UINT64					TotalSwitchOutTime;
	LARGE_INTEGER			WaitReasonBitmap;
} TEB_VISTA_SP, *PTEB_VISTA_SP;

// Win7, 7 SP1 layout
typedef struct _TEB_7
{
	NT_TIB					NtTib;
	PVOID                   EnvironmentPointer;
	CLIENT_ID               Cid;
	PVOID                   ActiveRpcInfo;
	PVOID                   ThreadLocalStoragePointer;
	PPEB_VISTA_7            Peb;
	ULONG                   LastErrorValue;
	ULONG                   CountOfOwnedCriticalSections;
	PVOID                   CsrClientThread;
	PVOID                   Win32ThreadInfo;
	ULONG                   User32Reserved[0x1a];
	ULONG                   UserReserved[0x5];
	PVOID                   WOW32Reserved;
	ULONG                   CurrentLocale;
	ULONG                   FpSoftwareStatusRegister;
	PVOID                   SystemReserved1[0x36];
	ULONG                   ExceptionCode;
	ACTIVATION_CONTEXT_STACK* ActivationContextStack;
#ifdef _WIN64
	BYTE                    SpareBytes1[0x18];
#else
	BYTE                    SpareBytes1[0x24];
#endif
	ULONG					TxFsContext;
	GDI_TEB_BATCH			GdiTebBatch;
	CLIENT_ID               RealClientId;
	HANDLE					GdiCachedProcessHandle;
	ULONG					GdiClientPID;
	ULONG					GdiClientTID;
	PVOID                   GdiThreadLocaleInfo;
	UINT_PTR                Win32ClientInfo[0x3e];
	PVOID                   GlDispatchTable[0xe9];	
	UINT_PTR                GlReserved1[0x1d];
	PVOID                   GlReserved2;
	PVOID                   GlSectionInfo;
	PVOID                   GlSection;
	PVOID                   GlTable;
	PVOID                   GlCurrentRC;
	PVOID                   GlContext;
	NTSTATUS                LastStatusValue;
	UNICODE_STRING          StaticUnicodeString;
	WCHAR                   StaticUnicodeBuffer[0x105];
	PVOID                   DeallocationStack;
	PVOID                   TlsSlots[0x40];
	LIST_ENTRY              TlsLinks;
	PVOID                   Vdm;
	PVOID                   ReservedForNtRpc;
	PVOID                   DbgSsReserved[0x2];
	ULONG                   HardErrorsAreDisabled;
#ifdef _WIN64
	PVOID                   Instrumentation[0xB];
#else
	PVOID                   Instrumentation[0x9];
#endif
	GUID                    ActivityId;
	PVOID                   EtwLocalData;
	PVOID                   EtwTraceData;
	PVOID					WinSockData;
	union
	{
		ULONG               GdiBatchCount;
		_TEB_7*				pTeb64;
	};
#include <pshpack1.h>
	union
	{
		PROCESSOR_NUMBER        CurrentIdealProcessor;
		ULONG                   IdealProcessorValue;
		struct
		{
			BOOLEAN                 ReservedPad0;
			BOOLEAN                 ReservedPad1;
			BOOLEAN                 ReservedPad2;
			BOOLEAN                 IdealProcessor;
		};
	};
#include <poppack.h>
	ULONG                   GuaranteedStackBytes;
	PVOID                   ReservedForPerf;
	PVOID                   ReservedForOle;
	ULONG                   WaitingOnLoaderLock;
	PVOID                   SavedPriorityState;
	ULONG_PTR				SoftPatchPtr1;
	PVOID					ThreadPoolData;
	PVOID*                  TlsExpansionSlots;
	ULONG                   MuiGeneration;
	ULONG                   IsImpersonating;
	PVOID                   NlsCache;
	PVOID                   pShimData;
	ULONG                   HeapVirtualAffinity;
	HANDLE                  CurrentTransactionHandle;
	TEB_ACTIVE_FRAME*       ActiveFrame;
	PVOID					FlsData;
	PVOID					PreferredLanguages;
	PVOID					UserPrefLanguages;
	PVOID					MergedPrefLanguages;
	ULONG					MuiImpersonation;
#include <pshpack1.h>
	union
	{
		volatile USHORT		CrossTebFlags;
		USHORT				SpareCrossTebBits: 0x10;
	};
	union
	{
		USHORT				SameTebFlags;
		struct
		{
			USHORT          SafeThunkCall: 1;
			USHORT          InDbgPrint: 1;
			USHORT          HasFiberData: 1;
			USHORT          SkipThreadAttach: 1;
			USHORT          WerInShipAssertCode: 1;
			USHORT          RanProcessInit: 1;
			USHORT          ClonedThread: 1;
			USHORT          SuppressDebugMsg: 1;
			USHORT          DisableUserStackWalk: 1;
			USHORT          RtlExceptionAttached: 1;
			USHORT          InitialThread: 1;
			USHORT          SpareSameTebBits: 5;
		};
	};
#include <poppack.h>
	BOOLEAN                 FreeStackOnTermination;
	ULONG                   ImpersonationLocale;
	PVOID					TxnScopeEnterCallback;
	PVOID					TxnScopeExitCallback;
	PVOID					TxnScopeContext;
	ULONG					LockCount;
	ULONG					SpareUlong0;
	PVOID					ResourceRetValue;
} TEB_7, *PTEB_7;

__if_not_exists(NtCurrentTeb)
{
	PTEB NTAPI NtCurrentTeb(void);
}

#if _WIN32_WINNT <= 0x0500

inline PPEB NTAPI RtlGetCurrentPeb(void)
{
	PTEB pTeb = NtCurrentTeb();
	return pTeb->Peb;
}

#else

PPEB_VISTA_7 NTAPI RtlGetCurrentPeb(void);

NTSYSAPI
NTSTATUS
NTAPI
NtDuplicateObject(
    __in HANDLE SourceProcessHandle,
    __in HANDLE SourceHandle,
    __in_opt HANDLE TargetProcessHandle,
    __out_opt PHANDLE TargetHandle,
    __in ACCESS_MASK DesiredAccess,
    __in ULONG HandleAttributes,
    __in ULONG Options
    );

#endif

//
// This enumerated type is used as the function return value of the function
// that is used to search the tree for a key. FoundNode indicates that the
// function found the key. Insert as left indicates that the key was not found
// and the node should be inserted as the left child of the parent. Insert as
// right indicates that the key was not found and the node should be inserted
//  as the right child of the parent.
//
typedef enum _TABLE_SEARCH_RESULT{
    TableEmptyTree,
    TableFoundNode,
    TableInsertAsLeft,
    TableInsertAsRight
} TABLE_SEARCH_RESULT;

//
//  The results of a compare can be less than, equal, or greater than.
//

typedef enum _RTL_GENERIC_COMPARE_RESULTS {
    GenericLessThan,
    GenericGreaterThan,
    GenericEqual
} RTL_GENERIC_COMPARE_RESULTS;

//
//  Define the Avl version of the generic table package.  Note a generic table
//  should really be an opaque type.  We provide routines to manipulate the structure.
//
//  A generic table is package for inserting, deleting, and looking up elements
//  in a table (e.g., in a symbol table).  To use this package the user
//  defines the structure of the elements stored in the table, provides a
//  comparison function, a memory allocation function, and a memory
//  deallocation function.
//
//  Note: the user compare function must impose a complete ordering among
//  all of the elements, and the table does not allow for duplicate entries.
//

//
// Add an empty typedef so that functions can reference the
// a pointer to the generic table struct before it is declared.
//

struct _RTL_AVL_TABLE;

//
//  The comparison function takes as input pointers to elements containing
//  user defined structures and returns the results of comparing the two
//  elements.
//

typedef
__drv_sameIRQL
__drv_functionClass(RTL_AVL_COMPARE_ROUTINE)
RTL_GENERIC_COMPARE_RESULTS
NTAPI
RTL_AVL_COMPARE_ROUTINE (
    __in struct _RTL_AVL_TABLE *Table,
    __in PVOID FirstStruct,
    __in PVOID SecondStruct
    );
typedef RTL_AVL_COMPARE_ROUTINE *PRTL_AVL_COMPARE_ROUTINE;

//
//  The allocation function is called by the generic table package whenever
//  it needs to allocate memory for the table.
//

typedef
__drv_sameIRQL
__drv_functionClass(RTL_AVL_ALLOCATE_ROUTINE)
__drv_allocatesMem(Mem)
PVOID
NTAPI
RTL_AVL_ALLOCATE_ROUTINE (
    __in struct _RTL_AVL_TABLE *Table,
    __in CLONG ByteSize
    );
typedef RTL_AVL_ALLOCATE_ROUTINE *PRTL_AVL_ALLOCATE_ROUTINE;

//
//  The deallocation function is called by the generic table package whenever
//  it needs to deallocate memory from the table that was allocated by calling
//  the user supplied allocation function.
//

typedef
__drv_sameIRQL
__drv_functionClass(RTL_AVL_FREE_ROUTINE)
VOID
NTAPI
RTL_AVL_FREE_ROUTINE (
    __in struct _RTL_AVL_TABLE *Table,
    __in __drv_freesMem(Mem) __post_invalid PVOID Buffer
    );
typedef RTL_AVL_FREE_ROUTINE *PRTL_AVL_FREE_ROUTINE;

//
//  The match function takes as input the user data to be matched and a pointer
//  to some match data, which was passed along with the function pointer.  It
//  returns TRUE for a match and FALSE for no match.
//
//  RTL_AVL_MATCH_FUNCTION returns
//      STATUS_SUCCESS if the IndexRow matches
//      STATUS_NO_MATCH if the IndexRow does not match, but the enumeration should
//          continue
//      STATUS_NO_MORE_MATCHES if the IndexRow does not match, and the enumeration
//          should terminate
//


typedef
__drv_sameIRQL
__drv_functionClass(RTL_AVL_MATCH_FUNCTION)
NTSTATUS
NTAPI
RTL_AVL_MATCH_FUNCTION (
    __in struct _RTL_AVL_TABLE *Table,
    __in PVOID UserData,
    __in PVOID MatchData
    );
typedef RTL_AVL_MATCH_FUNCTION *PRTL_AVL_MATCH_FUNCTION;

//
//  Define the balanced tree links and Balance field.  (No Rank field
//  defined at this time.)
//
//  Callers should treat this structure as opaque!
//
//  The root of a balanced binary tree is not a real node in the tree
//  but rather points to a real node which is the root.  It is always
//  in the table below, and its fields are used as follows:
//
//      Parent      Pointer to self, to allow for detection of the root.
//      LeftChild   NULL
//      RightChild  Pointer to real root
//      Balance     Undefined, however it is set to a convenient value
//                  (depending on the algorithm) prior to rebalancing
//                  in insert and delete routines.
//

typedef struct _RTL_BALANCED_LINKS {
    struct _RTL_BALANCED_LINKS *Parent;
    struct _RTL_BALANCED_LINKS *LeftChild;
    struct _RTL_BALANCED_LINKS *RightChild;
    CHAR Balance;
    UCHAR Reserved[3];
} RTL_BALANCED_LINKS;
typedef RTL_BALANCED_LINKS *PRTL_BALANCED_LINKS;

//
//  To use the generic table package the user declares a variable of type
//  GENERIC_TABLE and then uses the routines described below to initialize
//  the table and to manipulate the table.  Note that the generic table
//  should really be an opaque type.
//

typedef struct _RTL_AVL_TABLE {
    RTL_BALANCED_LINKS BalancedRoot;
    PVOID OrderedPointer;
    ULONG WhichOrderedElement;
    ULONG NumberGenericTableElements;
    ULONG DepthOfTree;
    PRTL_BALANCED_LINKS RestartKey;
    ULONG DeleteCount;
    PRTL_AVL_COMPARE_ROUTINE CompareRoutine;
    PRTL_AVL_ALLOCATE_ROUTINE AllocateRoutine;
    PRTL_AVL_FREE_ROUTINE FreeRoutine;
    PVOID TableContext;
} RTL_AVL_TABLE;
typedef RTL_AVL_TABLE *PRTL_AVL_TABLE;

//
//  The procedure InitializeGenericTable takes as input an uninitialized
//  generic table variable and pointers to the three user supplied routines.
//  This must be called for every individual generic table variable before
//  it can be used.
//

#if (NTDDI_VERSION >= NTDDI_WINXP)
NTSYSAPI
VOID
NTAPI
RtlInitializeGenericTableAvl (
    __out PRTL_AVL_TABLE Table,
    __in PRTL_AVL_COMPARE_ROUTINE CompareRoutine,
    __in PRTL_AVL_ALLOCATE_ROUTINE AllocateRoutine,
    __in PRTL_AVL_FREE_ROUTINE FreeRoutine,
    __in_opt PVOID TableContext
    );
#endif // NTDDI_VERSION >= NTDDI_WINXP

//
//  The function InsertElementGenericTable will insert a new element
//  in a table.  It does this by allocating space for the new element
//  (this includes AVL links), inserting the element in the table, and
//  then returning to the user a pointer to the new element.  If an element
//  with the same key already exists in the table the return value is a pointer
//  to the old element.  The optional output parameter NewElement is used
//  to indicate if the element previously existed in the table.  Note: the user
//  supplied Buffer is only used for searching the table, upon insertion its
//  contents are copied to the newly created element.  This means that
//  pointer to the input buffer will not point to the new element.
//

#if (NTDDI_VERSION >= NTDDI_WINXP)
NTSYSAPI
PVOID
NTAPI
RtlInsertElementGenericTableAvl (
    __in PRTL_AVL_TABLE Table,
    __in_bcount(BufferSize) PVOID Buffer,
    __in CLONG BufferSize,
    __out_opt PBOOLEAN NewElement
    );
#endif // NTDDI_VERSION >= NTDDI_WINXP

//
//  The function InsertElementGenericTableFull will insert a new element
//  in a table.  It does this by allocating space for the new element
//  (this includes AVL links), inserting the element in the table, and
//  then returning to the user a pointer to the new element.  If an element
//  with the same key already exists in the table the return value is a pointer
//  to the old element.  The optional output parameter NewElement is used
//  to indicate if the element previously existed in the table.  Note: the user
//  supplied Buffer is only used for searching the table, upon insertion its
//  contents are copied to the newly created element.  This means that
//  pointer to the input buffer will not point to the new element.
//  This routine is passed the NodeOrParent and SearchResult from a
//  previous RtlLookupElementGenericTableFull.
//

#if (NTDDI_VERSION >= NTDDI_WINXP)
NTSYSAPI
PVOID
NTAPI
RtlInsertElementGenericTableFullAvl (
    __in PRTL_AVL_TABLE Table,
    __in_bcount(BufferSize) PVOID Buffer,
    __in CLONG BufferSize,
    __out_opt PBOOLEAN NewElement,
    __in PVOID NodeOrParent,
    __in TABLE_SEARCH_RESULT SearchResult
    );
#endif // NTDDI_VERSION >= NTDDI_WINXP

//
//  The function DeleteElementGenericTable will find and delete an element
//  from a generic table.  If the element is located and deleted the return
//  value is TRUE, otherwise if the element is not located the return value
//  is FALSE.  The user supplied input buffer is only used as a key in
//  locating the element in the table.
//

#if (NTDDI_VERSION >= NTDDI_WINXP)
NTSYSAPI
BOOLEAN
NTAPI
RtlDeleteElementGenericTableAvl (
    __in PRTL_AVL_TABLE Table,
    __in PVOID Buffer
    );
#endif // NTDDI_VERSION >= NTDDI_WINXP

//
//  The function LookupElementGenericTable will find an element in a generic
//  table.  If the element is located the return value is a pointer to
//  the user defined structure associated with the element, otherwise if
//  the element is not located the return value is NULL.  The user supplied
//  input buffer is only used as a key in locating the element in the table.
//

#if (NTDDI_VERSION >= NTDDI_WINXP)
__checkReturn
NTSYSAPI
PVOID
NTAPI
RtlLookupElementGenericTableAvl (
    __in PRTL_AVL_TABLE Table,
    __in PVOID Buffer
    );
#endif // NTDDI_VERSION >= NTDDI_WINXP

//
//  The function LookupElementGenericTableFull will find an element in a generic
//  table.  If the element is located the return value is a pointer to
//  the user defined structure associated with the element.  If the element is not
//  located then a pointer to the parent for the insert location is returned.  The
//  user must look at the SearchResult value to determine which is being returned.
//  The user can use the SearchResult and parent for a subsequent FullInsertElement
//  call to optimize the insert.
//

#if (NTDDI_VERSION >= NTDDI_WINXP)
NTSYSAPI
PVOID
NTAPI
RtlLookupElementGenericTableFullAvl (
    __in PRTL_AVL_TABLE Table,
    __in PVOID Buffer,
    __out PVOID *NodeOrParent,
    __out TABLE_SEARCH_RESULT *SearchResult
    );
#endif // NTDDI_VERSION >= NTDDI_WINXP

//
//  The function EnumerateGenericTable will return to the caller one-by-one
//  the elements of of a table.  The return value is a pointer to the user
//  defined structure associated with the element.  The input parameter
//  Restart indicates if the enumeration should start from the beginning
//  or should return the next element.  If the are no more new elements to
//  return the return value is NULL.  As an example of its use, to enumerate
//  all of the elements in a table the user would write:
//
//      for (ptr = EnumerateGenericTable(Table, TRUE);
//           ptr != NULL;
//           ptr = EnumerateGenericTable(Table, FALSE)) {
//              :
//      }
//
//  NOTE:   This routine does not modify the structure of the tree, but saves
//          the last node returned in the generic table itself, and for this
//          reason requires exclusive access to the table for the duration of
//          the enumeration.
//

#if (NTDDI_VERSION >= NTDDI_WINXP)
__checkReturn
NTSYSAPI
PVOID
NTAPI
RtlEnumerateGenericTableAvl (
    __in PRTL_AVL_TABLE Table,
    __in BOOLEAN Restart
    );
#endif // NTDDI_VERSION >= NTDDI_WINXP

//
//  The function EnumerateGenericTableWithoutSplaying will return to the
//  caller one-by-one the elements of of a table.  The return value is a
//  pointer to the user defined structure associated with the element.
//  The input parameter RestartKey indicates if the enumeration should
//  start from the beginning or should return the next element.  If the
//  are no more new elements to return the return value is NULL.  As an
//  example of its use, to enumerate all of the elements in a table the
//  user would write:
//
//      RestartKey = NULL;
//      for (ptr = EnumerateGenericTableWithoutSplaying(Table, &RestartKey);
//           ptr != NULL;
//           ptr = EnumerateGenericTableWithoutSplaying(Table, &RestartKey)) {
//              :
//      }
//
//  If RestartKey is NULL, the package will start from the least entry in the
//  table, otherwise it will start from the last entry returned.
//
//  NOTE:   This routine does not modify either the structure of the tree
//          or the generic table itself, but must insure that no deletes
//          occur for the duration of the enumeration, typically by having
//          at least shared access to the table for the duration.
//

#if (NTDDI_VERSION >= NTDDI_WINXP)
__checkReturn
NTSYSAPI
PVOID
NTAPI
RtlEnumerateGenericTableWithoutSplayingAvl (
    __in PRTL_AVL_TABLE Table,
    __inout PVOID *RestartKey
    );
#endif // NTDDI_VERSION >= NTDDI_WINXP

//
//  RtlLookupFirstMatchingElementGenericTableAvl will return the left-most
//  element in the tree matching the data in Buffer.  If, for example, the tree
//  contains filenames there may exist several that differ only in case. A case-
//  blind searcher can use this routine to position himself in the tree at the
//  first match, and use an enumeration routine (such as RtlEnumerateGenericTableWithoutSplayingAvl
//  to return each subsequent match.
//

#if (NTDDI_VERSION >= NTDDI_WINXP)
__checkReturn
NTSYSAPI
PVOID
NTAPI
RtlLookupFirstMatchingElementGenericTableAvl (
    __in PRTL_AVL_TABLE Table,
    __in PVOID Buffer,
    __out PVOID *RestartKey
    );
#endif // NTDDI_VERSION >= NTDDI_WINXP

//
//  The function EnumerateGenericTableLikeADirectory will return to the
//  caller one-by-one the elements of of a table.  The return value is a
//  pointer to the user defined structure associated with the element.
//  The input parameter RestartKey indicates if the enumeration should
//  start from the beginning or should return the next element.  If the
//  are no more new elements to return the return value is NULL.  As an
//  example of its use, to enumerate all of the elements in a table the
//  user would write:
//
//      RestartKey = NULL;
//      for (ptr = EnumerateGenericTableLikeADirectory(Table, &RestartKey, ...);
//           ptr != NULL;
//           ptr = EnumerateGenericTableLikeADirectory(Table, &RestartKey, ...)) {
//              :
//      }
//
//  If RestartKey is NULL, the package will start from the least entry in the
//  table, otherwise it will start from the last entry returned.
//
//  NOTE:   This routine does not modify either the structure of the tree
//          or the generic table itself.  The table must only be acquired
//          shared for the duration of this call, and all synchronization
//          may optionally be dropped between calls.  Enumeration is always
//          correctly resumed in the most efficient manner possible via the
//          IN OUT parameters provided.
//
//  ******  Explain NextFlag.  Directory enumeration resumes from a key
//          requires more thought.  Also need the match pattern and IgnoreCase.
//          Should some structure be introduced to carry it all?
//

#if (NTDDI_VERSION >= NTDDI_WINXP)
__checkReturn
NTSYSAPI
PVOID
NTAPI
RtlEnumerateGenericTableLikeADirectory (
    __in PRTL_AVL_TABLE Table,
    __in_opt PRTL_AVL_MATCH_FUNCTION MatchFunction,
    __in_opt PVOID MatchData,
    __in ULONG NextFlag,
    __inout PVOID *RestartKey,
    __inout PULONG DeleteCount,
    __in PVOID Buffer
    );
#endif // NTDDI_VERSION >= NTDDI_WINXP

//
// The function GetElementGenericTable will return the i'th element
// inserted in the generic table.  I = 0 implies the first element,
// I = (RtlNumberGenericTableElements(Table)-1) will return the last element
// inserted into the generic table.  The type of I is ULONG.  Values
// of I > than (NumberGenericTableElements(Table)-1) will return NULL.  If
// an arbitrary element is deleted from the generic table it will cause
// all elements inserted after the deleted element to "move up".

#if (NTDDI_VERSION >= NTDDI_WINXP)
__checkReturn
NTSYSAPI
PVOID
NTAPI
RtlGetElementGenericTableAvl (
    __in PRTL_AVL_TABLE Table,
    __in ULONG I
    );
#endif // NTDDI_VERSION >= NTDDI_WINXP

//
// The function NumberGenericTableElements returns a ULONG value
// which is the number of generic table elements currently inserted
// in the generic table.

#if (NTDDI_VERSION >= NTDDI_WINXP)
NTSYSAPI
ULONG
NTAPI
RtlNumberGenericTableElementsAvl (
    __in PRTL_AVL_TABLE Table
    );
#endif // NTDDI_VERSION >= NTDDI_WINXP

//
//  The function IsGenericTableEmpty will return to the caller TRUE if
//  the input table is empty (i.e., does not contain any elements) and
//  FALSE otherwise.
//

#if (NTDDI_VERSION >= NTDDI_WINXP)
__checkReturn
NTSYSAPI
BOOLEAN
NTAPI
RtlIsGenericTableEmptyAvl (
    __in PRTL_AVL_TABLE Table
    );
#endif // NTDDI_VERSION >= NTDDI_WINXP

//
//  As an aid to allowing existing generic table users to do (in most
//  cases) a single-line edit to switch over to Avl table use, we
//  have the following defines and inline routine definitions which
//  redirect calls and types.  Note that the type override (performed
//  by #define below) will not work in the unexpected event that someone
//  has used a pointer or type specifier in their own #define, since
//  #define processing is one pass and does not nest.  The __inline
//  declarations below do not have this limitation, however.
//
//  To switch to using Avl tables, add the following line before your
//  includes:
//
//  #define RTL_USE_AVL_TABLES 0
//

#ifdef RTL_USE_AVL_TABLES

#undef PRTL_GENERIC_COMPARE_ROUTINE
#undef RTL_GENERIC_COMPARE_ROUTINE
#undef PRTL_GENERIC_ALLOCATE_ROUTINE
#undef RTL_GENERIC_ALLOCATE_ROUTINE
#undef PRTL_GENERIC_FREE_ROUTINE
#undef RTL_GENERIC_FREE_ROUTINE
#undef RTL_GENERIC_TABLE
#undef PRTL_GENERIC_TABLE

#define PRTL_GENERIC_COMPARE_ROUTINE PRTL_AVL_COMPARE_ROUTINE
#define RTL_GENERIC_COMPARE_ROUTINE RTL_AVL_COMPARE_ROUTINE
#define PRTL_GENERIC_ALLOCATE_ROUTINE PRTL_AVL_ALLOCATE_ROUTINE
#define RTL_GENERIC_ALLOCATE_ROUTINE RTL_AVL_ALLOCATE_ROUTINE
#define PRTL_GENERIC_FREE_ROUTINE PRTL_AVL_FREE_ROUTINE
#define RTL_GENERIC_FREE_ROUTINE RTL_AVL_FREE_ROUTINE
#define RTL_GENERIC_TABLE RTL_AVL_TABLE
#define PRTL_GENERIC_TABLE PRTL_AVL_TABLE

#define RtlInitializeGenericTable               RtlInitializeGenericTableAvl
#define RtlInsertElementGenericTable            RtlInsertElementGenericTableAvl
#define RtlInsertElementGenericTableFull        RtlInsertElementGenericTableFullAvl
#define RtlDeleteElementGenericTable            RtlDeleteElementGenericTableAvl
#define RtlLookupElementGenericTable            RtlLookupElementGenericTableAvl
#define RtlLookupElementGenericTableFull        RtlLookupElementGenericTableFullAvl
#define RtlEnumerateGenericTable                RtlEnumerateGenericTableAvl
#define RtlEnumerateGenericTableWithoutSplaying RtlEnumerateGenericTableWithoutSplayingAvl
#define RtlGetElementGenericTable               RtlGetElementGenericTableAvl
#define RtlNumberGenericTableElements           RtlNumberGenericTableElementsAvl
#define RtlIsGenericTableEmpty                  RtlIsGenericTableEmptyAvl

#endif // RTL_USE_AVL_TABLES

//
//  Define the generic table package.  Note a generic table should really
//  be an opaque type.  We provide routines to manipulate the structure.
//
//  A generic table is package for inserting, deleting, and looking up elements
//  in a table (e.g., in a symbol table).  To use this package the user
//  defines the structure of the elements stored in the table, provides a
//  comparison function, a memory allocation function, and a memory
//  deallocation function.
//
//  Note: the user compare function must impose a complete ordering among
//  all of the elements, and the table does not allow for duplicate entries.
//

//
//  Do not do the following defines if using Avl
//

#ifndef RTL_USE_AVL_TABLES

struct _RTL_GENERIC_TABLE;

//
//  The comparison function takes as input pointers to elements containing
//  user defined structures and returns the results of comparing the two
//  elements.
//

typedef
RTL_GENERIC_COMPARE_RESULTS
NTAPI
RTL_GENERIC_COMPARE_ROUTINE (
    __in struct _RTL_GENERIC_TABLE *Table,
    __in PVOID FirstStruct,
    __in PVOID SecondStruct
    );
typedef RTL_GENERIC_COMPARE_ROUTINE *PRTL_GENERIC_COMPARE_ROUTINE;

//
//  The allocation function is called by the generic table package whenever
//  it needs to allocate memory for the table.
//

typedef
PVOID
NTAPI
RTL_GENERIC_ALLOCATE_ROUTINE (
    __in struct _RTL_GENERIC_TABLE *Table,
    __in CLONG ByteSize
    );
typedef RTL_GENERIC_ALLOCATE_ROUTINE *PRTL_GENERIC_ALLOCATE_ROUTINE;

//
//  The deallocation function is called by the generic table package whenever
//  it needs to deallocate memory from the table that was allocated by calling
//  the user supplied allocation function.
//

typedef
VOID
NTAPI
RTL_GENERIC_FREE_ROUTINE (
    __in struct _RTL_GENERIC_TABLE *Table,
    __in PVOID Buffer
    );
typedef RTL_GENERIC_FREE_ROUTINE *PRTL_GENERIC_FREE_ROUTINE;

//
//  To use the generic table package the user declares a variable of type
//  GENERIC_TABLE and then uses the routines described below to initialize
//  the table and to manipulate the table.  Note that the generic table
//  should really be an opaque type.
//

typedef struct _RTL_SPLAY_LINKS {
    struct _RTL_SPLAY_LINKS *Parent;
    struct _RTL_SPLAY_LINKS *LeftChild;
    struct _RTL_SPLAY_LINKS *RightChild;
} RTL_SPLAY_LINKS;
typedef RTL_SPLAY_LINKS *PRTL_SPLAY_LINKS;

typedef struct _RTL_GENERIC_TABLE {
    PRTL_SPLAY_LINKS TableRoot;
    LIST_ENTRY InsertOrderList;
    PLIST_ENTRY OrderedPointer;
    ULONG WhichOrderedElement;
    ULONG NumberGenericTableElements;
    PRTL_GENERIC_COMPARE_ROUTINE CompareRoutine;
    PRTL_GENERIC_ALLOCATE_ROUTINE AllocateRoutine;
    PRTL_GENERIC_FREE_ROUTINE FreeRoutine;
    PVOID TableContext;
} RTL_GENERIC_TABLE;
typedef RTL_GENERIC_TABLE *PRTL_GENERIC_TABLE;

NTSYSAPI
VOID
NTAPI
RtlInitializeGenericTable (
    __out PRTL_GENERIC_TABLE Table,
    __in PRTL_GENERIC_COMPARE_ROUTINE CompareRoutine,
    __in PRTL_GENERIC_ALLOCATE_ROUTINE AllocateRoutine,
    __in PRTL_GENERIC_FREE_ROUTINE FreeRoutine,
    __in_opt PVOID TableContext
    );

//
//  The function InsertElementGenericTable will insert a new element
//  in a table.  It does this by allocating space for the new element
//  (this includes splay links), inserting the element in the table, and
//  then returning to the user a pointer to the new element.  If an element
//  with the same key already exists in the table the return value is a pointer
//  to the old element.  The optional output parameter NewElement is used
//  to indicate if the element previously existed in the table.  Note: the user
//  supplied Buffer is only used for searching the table, upon insertion its
//  contents are copied to the newly created element.  This means that
//  pointer to the input buffer will not point to the new element.
//

#if (NTDDI_VERSION >= NTDDI_WIN2K)
NTSYSAPI
PVOID
NTAPI
RtlInsertElementGenericTable (
    __in PRTL_GENERIC_TABLE Table,
    __in_bcount(BufferSize) PVOID Buffer,
    __in CLONG BufferSize,
    __out_opt PBOOLEAN NewElement
    );
#endif

//
//  The function InsertElementGenericTableFull will insert a new element
//  in a table.  It does this by allocating space for the new element
//  (this includes splay links), inserting the element in the table, and
//  then returning to the user a pointer to the new element.  If an element
//  with the same key already exists in the table the return value is a pointer
//  to the old element.  The optional output parameter NewElement is used
//  to indicate if the element previously existed in the table.  Note: the user
//  supplied Buffer is only used for searching the table, upon insertion its
//  contents are copied to the newly created element.  This means that
//  pointer to the input buffer will not point to the new element.
//  This routine is passed the NodeOrParent and SearchResult from a
//  previous RtlLookupElementGenericTableFull.
//

#if (NTDDI_VERSION >= NTDDI_WIN2K)
NTSYSAPI
PVOID
NTAPI
RtlInsertElementGenericTableFull (
    __in PRTL_GENERIC_TABLE Table,
    __in_bcount(BufferSize) PVOID Buffer,
    __in CLONG BufferSize,
    __out_opt PBOOLEAN NewElement,
    __in PVOID NodeOrParent,
    __in TABLE_SEARCH_RESULT SearchResult
    );
#endif

//
//  The function DeleteElementGenericTable will find and delete an element
//  from a generic table.  If the element is located and deleted the return
//  value is TRUE, otherwise if the element is not located the return value
//  is FALSE.  The user supplied input buffer is only used as a key in
//  locating the element in the table.
//

#if (NTDDI_VERSION >= NTDDI_WIN2K)
NTSYSAPI
BOOLEAN
NTAPI
RtlDeleteElementGenericTable (
    __in PRTL_GENERIC_TABLE Table,
    __in PVOID Buffer
    );
#endif

//
//  The function LookupElementGenericTable will find an element in a generic
//  table.  If the element is located the return value is a pointer to
//  the user defined structure associated with the element, otherwise if
//  the element is not located the return value is NULL.  The user supplied
//  input buffer is only used as a key in locating the element in the table.
//

#if (NTDDI_VERSION >= NTDDI_WIN2K)
__checkReturn
NTSYSAPI
PVOID
NTAPI
RtlLookupElementGenericTable (
    __in PRTL_GENERIC_TABLE Table,
    __in PVOID Buffer
    );
#endif

//
//  The function LookupElementGenericTableFull will find an element in a generic
//  table.  If the element is located the return value is a pointer to
//  the user defined structure associated with the element.  If the element is not
//  located then a pointer to the parent for the insert location is returned.  The
//  user must look at the SearchResult value to determine which is being returned.
//  The user can use the SearchResult and parent for a subsequent FullInsertElement
//  call to optimize the insert.
//

#if (NTDDI_VERSION >= NTDDI_WIN2K)
NTSYSAPI
PVOID
NTAPI
RtlLookupElementGenericTableFull (
    __in PRTL_GENERIC_TABLE Table,
    __in PVOID Buffer,
    __out PVOID *NodeOrParent,
    __out TABLE_SEARCH_RESULT *SearchResult
    );
#endif

//
//  The function EnumerateGenericTable will return to the caller one-by-one
//  the elements of of a table.  The return value is a pointer to the user
//  defined structure associated with the element.  The input parameter
//  Restart indicates if the enumeration should start from the beginning
//  or should return the next element.  If the are no more new elements to
//  return the return value is NULL.  As an example of its use, to enumerate
//  all of the elements in a table the user would write:
//
//      for (ptr = EnumerateGenericTable(Table, TRUE);
//           ptr != NULL;
//           ptr = EnumerateGenericTable(Table, FALSE)) {
//              :
//      }
//
//
//  PLEASE NOTE:
//
//      If you enumerate a GenericTable using RtlEnumerateGenericTable, you
//      will flatten the table, turning it into a sorted linked list.
//      To enumerate the table without perturbing the splay links, use
//      RtlEnumerateGenericTableWithoutSplaying

#if (NTDDI_VERSION >= NTDDI_WIN2K)
__checkReturn
NTSYSAPI
PVOID
NTAPI
RtlEnumerateGenericTable (
    __in PRTL_GENERIC_TABLE Table,
    __in BOOLEAN Restart
    );
#endif

//
//  The function EnumerateGenericTableWithoutSplaying will return to the
//  caller one-by-one the elements of of a table.  The return value is a
//  pointer to the user defined structure associated with the element.
//  The input parameter RestartKey indicates if the enumeration should
//  start from the beginning or should return the next element.  If the
//  are no more new elements to return the return value is NULL.  As an
//  example of its use, to enumerate all of the elements in a table the
//  user would write:
//
//      RestartKey = NULL;
//      for (ptr = EnumerateGenericTableWithoutSplaying(Table, &RestartKey);
//           ptr != NULL;
//           ptr = EnumerateGenericTableWithoutSplaying(Table, &RestartKey)) {
//              :
//      }
//
//  If RestartKey is NULL, the package will start from the least entry in the
//  table, otherwise it will start from the last entry returned.
//
//
//  Note that unlike RtlEnumerateGenericTable, this routine will NOT perturb
//  the splay order of the tree.
//

#if (NTDDI_VERSION >= NTDDI_WIN2K)
__checkReturn
NTSYSAPI
PVOID
NTAPI
RtlEnumerateGenericTableWithoutSplaying (
    __in PRTL_GENERIC_TABLE Table,
    __inout PVOID *RestartKey
    );
#endif

//
// The function GetElementGenericTable will return the i'th element
// inserted in the generic table.  I = 0 implies the first element,
// I = (RtlNumberGenericTableElements(Table)-1) will return the last element
// inserted into the generic table.  The type of I is ULONG.  Values
// of I > than (NumberGenericTableElements(Table)-1) will return NULL.  If
// an arbitrary element is deleted from the generic table it will cause
// all elements inserted after the deleted element to "move up".

#if (NTDDI_VERSION >= NTDDI_WIN2K)
__checkReturn
NTSYSAPI
PVOID
NTAPI
RtlGetElementGenericTable(
    __in PRTL_GENERIC_TABLE Table,
    __in ULONG I
    );
#endif

//
// The function NumberGenericTableElements returns a ULONG value
// which is the number of generic table elements currently inserted
// in the generic table.

#if (NTDDI_VERSION >= NTDDI_WIN2K)
NTSYSAPI
ULONG
NTAPI
RtlNumberGenericTableElements(
    __in PRTL_GENERIC_TABLE Table
    );
#endif

//
//  The function IsGenericTableEmpty will return to the caller TRUE if
//  the input table is empty (i.e., does not contain any elements) and
//  FALSE otherwise.
//

#if (NTDDI_VERSION >= NTDDI_WIN2K)
__checkReturn
NTSYSAPI
BOOLEAN
NTAPI
RtlIsGenericTableEmpty (
    __in PRTL_GENERIC_TABLE Table
    );
#endif

#endif // RTL_USE_AVL_TREES

//
// Open/Create Options
//

#define REG_OPTION_RESERVED         (0x00000000L)   // Parameter is reserved

#define REG_OPTION_NON_VOLATILE     (0x00000000L)   // Key is preserved
                                                    // when system is rebooted

#define REG_OPTION_VOLATILE         (0x00000001L)   // Key is not preserved
                                                    // when system is rebooted

#define REG_OPTION_CREATE_LINK      (0x00000002L)   // Created key is a
                                                    // symbolic link

#define REG_OPTION_BACKUP_RESTORE   (0x00000004L)   // open for backup or restore
                                                    // special access rules
                                                    // privilege required

#define REG_OPTION_OPEN_LINK        (0x00000008L)   // Open symbolic link

#define REG_LEGAL_OPTION            \
                (REG_OPTION_RESERVED            |\
                 REG_OPTION_NON_VOLATILE        |\
                 REG_OPTION_VOLATILE            |\
                 REG_OPTION_CREATE_LINK         |\
                 REG_OPTION_BACKUP_RESTORE      |\
                 REG_OPTION_OPEN_LINK)

#define REG_OPEN_LEGAL_OPTION       \
                (REG_OPTION_RESERVED            |\
                 REG_OPTION_BACKUP_RESTORE      |\
                 REG_OPTION_OPEN_LINK)

//
// Key creation/open disposition
//

#define REG_CREATED_NEW_KEY         (0x00000001L)   // New Registry Key created
#define REG_OPENED_EXISTING_KEY     (0x00000002L)   // Existing Key opened

//
// hive format to be used by Reg(Nt)SaveKeyEx
//
#define REG_STANDARD_FORMAT     1
#define REG_LATEST_FORMAT       2
#define REG_NO_COMPRESSION      4

//
// Key restore & hive load flags
//

#define REG_WHOLE_HIVE_VOLATILE         (0x00000001L)   // Restore whole hive volatile
#define REG_REFRESH_HIVE                (0x00000002L)   // Unwind changes to last flush
#define REG_NO_LAZY_FLUSH               (0x00000004L)   // Never lazy flush this hive
#define REG_FORCE_RESTORE               (0x00000008L)   // Force the restore process even when we have open handles on subkeys
#define REG_APP_HIVE                    (0x00000010L)   // Loads the hive visible to the calling process
#define REG_PROCESS_PRIVATE             (0x00000020L)   // Hive cannot be mounted by any other process while in use
#define REG_START_JOURNAL               (0x00000040L)   // Starts Hive Journal
#define REG_HIVE_EXACT_FILE_GROWTH      (0x00000080L)   // Grow hive file in exact 4k increments
#define REG_HIVE_NO_RM                  (0x00000100L)   // No RM is started for this hive (no transactions)
#define REG_HIVE_SINGLE_LOG             (0x00000200L)   // Legacy single logging is used for this hive
#define REG_BOOT_HIVE                   (0x00000400L)   // This hive might be used by the OS loader

//
// Unload Flags
//
#define REG_FORCE_UNLOAD            1

//
// Notify filter values
//

#define REG_NOTIFY_CHANGE_NAME          (0x00000001L) // Create or delete (child)
#define REG_NOTIFY_CHANGE_ATTRIBUTES    (0x00000002L)
#define REG_NOTIFY_CHANGE_LAST_SET      (0x00000004L) // time stamp
#define REG_NOTIFY_CHANGE_SECURITY      (0x00000008L)

#define REG_LEGAL_CHANGE_FILTER                 \
                (REG_NOTIFY_CHANGE_NAME          |\
                 REG_NOTIFY_CHANGE_ATTRIBUTES    |\
                 REG_NOTIFY_CHANGE_LAST_SET      |\
                 REG_NOTIFY_CHANGE_SECURITY)
 
//
// Key query structures
//

typedef struct _KEY_BASIC_INFORMATION {
    LARGE_INTEGER LastWriteTime;
    ULONG   TitleIndex;
    ULONG   NameLength;
    WCHAR   Name[1];            // Variable length string
} KEY_BASIC_INFORMATION, *PKEY_BASIC_INFORMATION;

typedef struct _KEY_NODE_INFORMATION {
    LARGE_INTEGER LastWriteTime;
    ULONG   TitleIndex;
    ULONG   ClassOffset;
    ULONG   ClassLength;
    ULONG   NameLength;
    WCHAR   Name[1];            // Variable length string
//          Class[1];           // Variable length string not declared
} KEY_NODE_INFORMATION, *PKEY_NODE_INFORMATION;

typedef struct _KEY_FULL_INFORMATION {
    LARGE_INTEGER LastWriteTime;
    ULONG   TitleIndex;
    ULONG   ClassOffset;
    ULONG   ClassLength;
    ULONG   SubKeys;
    ULONG   MaxNameLen;
    ULONG   MaxClassLen;
    ULONG   Values;
    ULONG   MaxValueNameLen;
    ULONG   MaxValueDataLen;
    WCHAR   Class[1];           // Variable length
} KEY_FULL_INFORMATION, *PKEY_FULL_INFORMATION;

typedef enum _KEY_INFORMATION_CLASS {
    KeyBasicInformation,
    KeyNodeInformation,
    KeyFullInformation,
    KeyNameInformation,
    KeyCachedInformation,
    KeyFlagsInformation,
    KeyVirtualizationInformation,
    KeyHandleTagsInformation,
    MaxKeyInfoClass  // MaxKeyInfoClass should always be the last enum
} KEY_INFORMATION_CLASS;

typedef struct _KEY_WRITE_TIME_INFORMATION {
    LARGE_INTEGER LastWriteTime;
} KEY_WRITE_TIME_INFORMATION, *PKEY_WRITE_TIME_INFORMATION;

typedef struct _KEY_WOW64_FLAGS_INFORMATION {
    ULONG   UserFlags;
} KEY_WOW64_FLAGS_INFORMATION, *PKEY_WOW64_FLAGS_INFORMATION;

typedef struct _KEY_HANDLE_TAGS_INFORMATION {
    ULONG   HandleTags;
} KEY_HANDLE_TAGS_INFORMATION, *PKEY_HANDLE_TAGS_INFORMATION;

typedef struct _KEY_CONTROL_FLAGS_INFORMATION {
    ULONG   ControlFlags;
} KEY_CONTROL_FLAGS_INFORMATION, *PKEY_CONTROL_FLAGS_INFORMATION;

typedef struct _KEY_SET_VIRTUALIZATION_INFORMATION {
    ULONG   VirtualTarget           : 1; // Tells if the key is a virtual target key. 
    ULONG   VirtualStore	        : 1; // Tells if the key is a virtual store key.
    ULONG   VirtualSource           : 1; // Tells if the key has been virtualized at least one (virtual hint)
    ULONG   Reserved                : 29;   
} KEY_SET_VIRTUALIZATION_INFORMATION, *PKEY_SET_VIRTUALIZATION_INFORMATION;

typedef enum _KEY_SET_INFORMATION_CLASS {
    KeyWriteTimeInformation,
    KeyWow64FlagsInformation,
    KeyControlFlagsInformation,
    KeySetVirtualizationInformation,
    KeySetDebugInformation,
    KeySetHandleTagsInformation,
    MaxKeySetInfoClass  // MaxKeySetInfoClass should always be the last enum
} KEY_SET_INFORMATION_CLASS;

//
// Value entry query structures
//

typedef struct _KEY_VALUE_BASIC_INFORMATION {
    ULONG   TitleIndex;
    ULONG   Type;
    ULONG   NameLength;
    WCHAR   Name[1];            // Variable size
} KEY_VALUE_BASIC_INFORMATION, *PKEY_VALUE_BASIC_INFORMATION;

typedef struct _KEY_VALUE_FULL_INFORMATION {
    ULONG   TitleIndex;
    ULONG   Type;
    ULONG   DataOffset;
    ULONG   DataLength;
    ULONG   NameLength;
    WCHAR   Name[1];            // Variable size
//          Data[1];            // Variable size data not declared
} KEY_VALUE_FULL_INFORMATION, *PKEY_VALUE_FULL_INFORMATION;

typedef struct _KEY_VALUE_PARTIAL_INFORMATION {
    ULONG   TitleIndex;
    ULONG   Type;
    ULONG   DataLength;
    UCHAR   Data[1];            // Variable size
} KEY_VALUE_PARTIAL_INFORMATION, *PKEY_VALUE_PARTIAL_INFORMATION;

typedef struct _KEY_VALUE_PARTIAL_INFORMATION_ALIGN64 {
    ULONG   Type;
    ULONG   DataLength;
    UCHAR   Data[1];            // Variable size
} KEY_VALUE_PARTIAL_INFORMATION_ALIGN64, *PKEY_VALUE_PARTIAL_INFORMATION_ALIGN64;

typedef struct _KEY_VALUE_ENTRY {
    PUNICODE_STRING ValueName;
    ULONG           DataLength;
    ULONG           DataOffset;
    ULONG           Type;
} KEY_VALUE_ENTRY, *PKEY_VALUE_ENTRY;

typedef enum _KEY_VALUE_INFORMATION_CLASS {
    KeyValueBasicInformation,
    KeyValueFullInformation,
    KeyValuePartialInformation,
    KeyValueFullInformationAlign64,
    KeyValuePartialInformationAlign64,
    MaxKeyValueInfoClass  // MaxKeyValueInfoClass should always be the last enum
} KEY_VALUE_INFORMATION_CLASS;

#if (NTDDI_VERSION >= NTDDI_WIN2K)
__drv_maxIRQL(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
NtCreateKey(
    __out PHANDLE KeyHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __reserved ULONG TitleIndex,
    __in_opt PUNICODE_STRING Class,
    __in ULONG CreateOptions,
    __out_opt PULONG Disposition
    );
#endif

#if (NTDDI_VERSION >= NTDDI_VISTA)
__drv_maxIRQL(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NtCreateKeyTransacted(
    __out PHANDLE KeyHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __reserved ULONG TitleIndex,
    __in_opt PUNICODE_STRING Class,
    __in ULONG CreateOptions,
    __in HANDLE TransactionHandle,
    __out_opt PULONG Disposition
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
__drv_maxIRQL(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
NtOpenKey(
    __out PHANDLE KeyHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
__drv_maxIRQL(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
NtOpenKeyEx(
    __out PHANDLE KeyHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __in ULONG OpenOptions
    );
#endif



#if (NTDDI_VERSION >= NTDDI_VISTA)
__drv_maxIRQL(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
NtOpenKeyTransacted(
    __out PHANDLE KeyHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __in HANDLE TransactionHandle
    );
#endif


#if (NTDDI_VERSION >= NTDDI_WIN7)
__drv_maxIRQL(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
NtOpenKeyTransactedEx(
    __out PHANDLE KeyHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __in ULONG OpenOptions,
    __in HANDLE TransactionHandle
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
__drv_maxIRQL(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
NtDeleteKey(
    __in HANDLE KeyHandle
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
__drv_maxIRQL(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
NtDeleteValueKey(
    __in HANDLE KeyHandle,
    __in PUNICODE_STRING ValueName
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
__drv_maxIRQL(PASSIVE_LEVEL)
__drv_when(Length==0, __drv_valueIs(<0))
__drv_when(Length>0, __drv_valueIs(<0;==0))
NTSYSAPI
NTSTATUS
NTAPI
NtEnumerateKey(
    __in HANDLE KeyHandle,
    __in ULONG Index,
    __in KEY_INFORMATION_CLASS KeyInformationClass,
    __out_bcount_opt(Length) PVOID KeyInformation,
    __in ULONG Length,
    __out PULONG ResultLength
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
__drv_maxIRQL(PASSIVE_LEVEL)
__drv_when(Length==0, __drv_valueIs(<0))
__drv_when(Length>0, __drv_valueIs(<0;==0))
NTSYSAPI
NTSTATUS
NTAPI
NtEnumerateValueKey(
    __in HANDLE KeyHandle,
    __in ULONG Index,
    __in KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    __out_bcount_opt(Length) PVOID KeyValueInformation,
    __in ULONG Length,
    __out PULONG ResultLength
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
__drv_maxIRQL(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
NtFlushKey(
    __in HANDLE KeyHandle
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
__drv_maxIRQL(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
NtNotifyChangeMultipleKeys(
    __in HANDLE MasterKeyHandle,
    __in_opt ULONG Count,
    __in_ecount_opt(Count) OBJECT_ATTRIBUTES SubordinateObjects[],
    __in_opt HANDLE Event,
    __in_opt PIO_APC_ROUTINE ApcRoutine,
    __in_opt PVOID ApcContext,
    __out PIO_STATUS_BLOCK IoStatusBlock,
    __in ULONG CompletionFilter,
    __in BOOLEAN WatchTree,
    __out_bcount_opt(BufferSize) PVOID Buffer,
    __in ULONG BufferSize,
    __in BOOLEAN Asynchronous
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
__drv_maxIRQL(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
NtQueryMultipleValueKey(
    __in HANDLE KeyHandle,
    __inout_ecount(EntryCount) PKEY_VALUE_ENTRY ValueEntries,
    __in ULONG EntryCount,
    __out_bcount(*BufferLength) PVOID ValueBuffer,
    __inout PULONG BufferLength,
    __out_opt PULONG RequiredBufferLength
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
__drv_maxIRQL(PASSIVE_LEVEL)
__drv_when(Length==0, __drv_valueIs(<0))
__drv_when(Length>0, __drv_valueIs(<0;==0))
NTSYSAPI
NTSTATUS
NTAPI
NtQueryKey(
    __in HANDLE KeyHandle,
    __in KEY_INFORMATION_CLASS KeyInformationClass,
    __out_bcount_opt(Length) PVOID KeyInformation,
    __in ULONG Length,
    __out PULONG ResultLength
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
__drv_maxIRQL(PASSIVE_LEVEL)
__drv_when(Length==0, __drv_valueIs(<0))
__drv_when(Length>0, __drv_valueIs(<0;==0))
NTSYSAPI
NTSTATUS
NTAPI
NtQueryValueKey(
    __in HANDLE KeyHandle,
    __in PUNICODE_STRING ValueName,
    __in KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    __out_bcount_opt(Length) PVOID KeyValueInformation,
    __in ULONG Length,
    __out PULONG ResultLength
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
__drv_maxIRQL(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
NtRenameKey(
    __in HANDLE           KeyHandle,
    __in PUNICODE_STRING  NewName
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
__drv_maxIRQL(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
NtSetInformationKey(
    __in HANDLE KeyHandle,
    __in KEY_SET_INFORMATION_CLASS KeySetInformationClass,
    __in_bcount(KeySetInformationLength) PVOID KeySetInformation,
    __in ULONG KeySetInformationLength
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
__drv_maxIRQL(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
NtSetValueKey(
    __in HANDLE KeyHandle,
    __in PUNICODE_STRING ValueName,
    __in_opt ULONG TitleIndex,
    __in ULONG Type,
    __in_bcount_opt(DataSize) PVOID Data,
    __in ULONG DataSize
    );
#endif

#if !defined(NTDLL_DONT_INCLUDE_CRT_FUNC) && !defined(_INC_STDIO)

#ifdef __cplusplus
extern "C"
{
#endif

DECLSPEC_IMPORT int __cdecl _vsnprintf(LPSTR, size_t, const CHAR*, va_list);
DECLSPEC_IMPORT int __cdecl _vsnwprintf(LPWSTR, size_t, const WCHAR*, va_list);
DECLSPEC_IMPORT int __cdecl _snprintf(LPSTR, size_t, const CHAR*, ...);
DECLSPEC_IMPORT int __cdecl _snwprintf(LPWSTR, size_t, const WCHAR*, ...);
DECLSPEC_IMPORT int __cdecl _sscanf(LPSTR buffer, CHAR* format, ...);

#ifdef __cplusplus
}
#endif

#elif !defined(NTDLL_DONT_INCLUDE_CRT_FUNC) && !defined(_INC_STDLIB)

#ifdef __cplusplus
extern "C"
{
#endif

DECLSPEC_IMPORT unsigned long __cdecl wcstoul(LPCWSTR buffer, WCHAR** ppEndChar, int base);
DECLSPEC_IMPORT unsigned __int64 __cdecl _wcstoui64(LPCWSTR buffer, WCHAR** ppEndChar, int base);
DECLSPEC_IMPORT __int64 __cdecl _wtoi64(LPCWSTR buffer);

#ifdef __cplusplus
}
#endif

#endif


#ifdef __cplusplus
}
#endif

#define HANDLE_DETACHED_PROCESS    (HANDLE)-1
#define HANDLE_CREATE_NEW_CONSOLE  (HANDLE)-2
#define HANDLE_CREATE_NO_WINDOW    (HANDLE)-4

#define OBJ_INHERIT             0x00000002L
#define OBJ_PERMANENT           0x00000010L
#define OBJ_EXCLUSIVE           0x00000020L
#define OBJ_CASE_INSENSITIVE    0x00000040L
#define OBJ_OPENIF              0x00000080L
#define OBJ_OPENLINK            0x00000100L
#define OBJ_KERNEL_HANDLE       0x00000200L
#define OBJ_FORCE_ACCESS_CHECK  0x00000400L
#define OBJ_VALID_ATTRIBUTES    0x000007F2L

#define LDR_IMAGE_IS_DLL                0x00000004
#define LDR_LOAD_IN_PROGRESS            0x00001000
#define LDR_UNLOAD_IN_PROGRESS          0x00002000
#define LDR_NO_DLL_CALLS                0x00040000
#define LDR_PROCESS_ATTACHED            0x00080000
#define LDR_MODULE_REBASED              0x00200000

#endif
