#include "my.h"

#if ML_KERNEL_MODE


ML_NAMESPACE

LONG_PTR MlInitialize()
{
	return 0;
}

LONG_PTR MlUnInitialize()
{
	return 0;
}

MY_NAMESPACE_END

#else



_ML_C_HEAD_

#if ML_KERNEL_MODE

PVOID AllocateMemory(ULONG_PTR Size, POOL_TYPE PoolType /* = PagedPool */)
{
	return MemoryAllocator::AllocateMemory(Size, PoolType);
}

PVOID AllocateMemoryP(ULONG_PTR Size, POOL_TYPE PoolType)
{
	return AllocateMemory(Size, PoolType);
}

BOOL FreeMemory(PVOID Memory, ULONG Flags)
{
	return Memory != NULL ? MemoryAllocator::FreeMemory(Memory) : FALSE;
}

BOOL FreeMemoryP(PVOID Memory, ULONG Flags)
{
	return FreeMemory(Memory, Flags);
}

#else // user mode

PVOID AllocateMemoryP(ULONG_PTR Size, ULONG Flags)
{
	return MemoryAllocator::AllocateMemory(Size, Flags);
}

PVOID ReAllocateMemoryP(PVOID Memory, ULONG_PTR Size, ULONG Flags)
{
	return MemoryAllocator::ReAllocateMemory(Memory, Size, Flags);
}

PVOID AllocateMemory(ULONG_PTR Size, ULONG Flags)
{
#if USE_CRT_VER
	return malloc(Size);
#elif USE_NT_VER
	return RtlAllocateHeap(CurrentPeb()->ProcessHeap, Flags, Size);
#else
	return HeapAlloc(GetProcessHeap(), Flags, Size);
#endif
}

PVOID ReAllocateMemory(PVOID Memory, ULONG_PTR Size, ULONG Flags)
{
	PVOID Block;

	if (Memory == NULL)
		return AllocateMemory(Size, Flags);

#if USE_CRT_VER
	Block = realloc(Memory, Size);
#elif USE_NT_VER
	Block = RtlReAllocateHeap(CurrentPeb()->ProcessHeap, Flags, Memory, Size);
#else
	Block = HeapReAlloc(GetProcessHeap(), Flags, Memory, Size);
#endif

	if (Block == nullptr)
	{
		FreeMemory(Memory);
		return nullptr;
	}

	return Block;
}

BOOL FreeMemory(PVOID Memory, ULONG Flags)
{
#if USE_CRT_VER
	free(Memory);
	return TRUE;
#elif USE_NT_VER
	return Memory != NULL ? RtlFreeHeap(CurrentPeb()->ProcessHeap, Flags, Memory) : FALSE;
#else
	return Memory != NULL ? HeapFree(GetProcessHeap(), Flags, Memory) : FALSE;
#endif
}

BOOL FreeMemoryP(PVOID Memory, ULONG Flags)
{
	return MemoryAllocator::FreeMemory(Memory, Flags);
}

PVOID AllocateVirtualMemory(ULONG_PTR Size, ULONG Protect, HANDLE ProcessHandle)
{
#if USE_NT_VER
	PVOID BaseAddress = NULL;
	NTSTATUS Status;
	Status = Nt_AllocateMemory(ProcessHandle, &BaseAddress, Size, Protect, MEM_RESERVE | MEM_COMMIT);
	return NT_SUCCESS(Status) ? BaseAddress : NULL;
#else
	return VirtualAllocEx(ProcessHandle, NULL, Size, MEM_RESERVE | MEM_COMMIT, Protect);
#endif
}

BOOL FreeVirtualMemory(PVOID Memory, HANDLE ProcessHandle)
{
#if USE_NT_VER
	return NT_SUCCESS(Nt_FreeMemory(ProcessHandle, Memory));
#else
	return VirtualFreeEx(ProcessHandle, Memory, 0, MEM_RELEASE);
#endif
}

#endif  // ML_KERNEL_MODE

_ML_C_TAIL_

#pragma warning(disable:4245)
#pragma warning(disable:4750)

_ML_C_HEAD_

/************************************************************************
strings
************************************************************************/
ULONG
InternalCopyUnicodeString(
	PUNICODE_STRING Unicode,
	PWCHAR          Buffer,
	ULONG_PTR       BufferCount,
	BOOL            IsDirectory = FALSE
)
{
	ULONG_PTR Length;

	Length = MY_MIN(Unicode->Length, BufferCount * sizeof(WCHAR));
	CopyMemory(Buffer, Unicode->Buffer, Length);
	Length /= sizeof(WCHAR);

	if (IsDirectory && Length < BufferCount && Buffer[Length - 1] != '\\')
		Buffer[Length++] = '\\';

	if (Length < BufferCount)
		Buffer[Length] = 0;

	return Length;
}

BOOL Nt_TerminateThread(IN HANDLE hThread, IN ULONG dwExitCode)
{
	return NT_FAILED(NtTerminateThread(hThread, dwExitCode)) ? FALSE : TRUE;
}


NTSTATUS
FASTCALL
Nt_AnsiToUnicode(
	PWCHAR      UnicodeBuffer,
	ULONG_PTR   BufferCount,
	LPCSTR      AnsiString,
	LONG_PTR    AnsiLength,
	PULONG_PTR  BytesInUnicode
)
{
	return ml::Native::Nls::AnsiToUnicode(UnicodeBuffer, BufferCount, AnsiString, AnsiLength, BytesInUnicode);
}

NTSTATUS
Nt_UnicodeToAnsi(
	PCHAR       AnsiBuffer,
	ULONG_PTR   BufferCount,
	LPCWSTR     UnicodeString,
	LONG_PTR    UnicodeLength,
	PULONG_PTR  BytesInAnsi
)
{
	return ml::Native::Nls::UnicodeToAnsi(AnsiBuffer, BufferCount, UnicodeString, UnicodeLength, BytesInAnsi);
}

NTSTATUS
Nt_AnsiToUnicodeString(
	PUNICODE_STRING Unicode,
	PCHAR           AnsiString,
	LONG_PTR        AnsiLength,
	BOOL            AllocateDestinationString
)
{
	return ml::Native::Nls::AnsiToUnicodeString(Unicode, AnsiString, AnsiLength, AllocateDestinationString);
}

NTSTATUS
Nt_UnicodeToAnsiString(
	PANSI_STRING    Ansi,
	LPCWSTR         UnicodeString,
	LONG_PTR        UnicodeLength,
	BOOL            AllocateDestinationString
)
{
	return ml::Native::Nls::UnicodeToAnsiString(Ansi, UnicodeString, UnicodeLength, AllocateDestinationString);
}


/************************************************************************
process api
************************************************************************/

BOOL
Nt_IsWow64Process(
	HANDLE Process /* = NtCurrentProcess() */
)
{
	PVOID       Wow64Information;
	NTSTATUS    Status;

	Status = ZwQueryInformationProcess(
		Process,
		ProcessWow64Information,
		&Wow64Information,
		sizeof(Wow64Information),
		NULL
	);

	return NT_SUCCESS(Status) && Wow64Information != NULL;
}

ULONG Nt_GetErrorMode()
{
	ULONG HardErrorMode;

	if (!NT_SUCCESS(NtQueryInformationProcess(NtCurrentProcess(), ProcessDefaultHardErrorMode, &HardErrorMode, sizeof(HardErrorMode), NULL)))
		HardErrorMode = 0;

	return HardErrorMode;
}

ULONG Nt_SetErrorMode(ULONG Mode)
{
	ULONG HardErrorMode, PreviousHardErrorMode;
	NTSTATUS Status;

	PreviousHardErrorMode = Nt_GetErrorMode();
	HardErrorMode = PreviousHardErrorMode | Mode;
	NtSetInformationProcess(NtCurrentProcess(), ProcessDefaultHardErrorMode, &HardErrorMode, sizeof(HardErrorMode));

	return PreviousHardErrorMode;
}

/************************************************************************
common file io
************************************************************************/

NTSTATUS Nt_DeleteFile(PCWSTR FileName)
{
	return Io::DeleteFile(FileName);
}

/************************************************************************
common path
************************************************************************/

ULONG_PTR
Nt_QueryDosDevice(
	PCWSTR      DeviceName,
	PWSTR       TargetPath,
	ULONG_PTR   PathSize
)
{
	NTSTATUS        Status;
	UNICODE_STRING  Target;

	Status = Io::QueryDosDevice(DeviceName, &Target);
	if (NT_FAILED(Status))
		return 0;

	PathSize = InternalCopyUnicodeString(&Target, TargetPath, PathSize);
	RtlFreeUnicodeString(&Target);

	return PathSize;
}

/************************************************************************
common query
************************************************************************/

POBJECT_TYPES_INFORMATION QuerySystemObjectTypes()
{
	ULONG                       Size, Length;
	NTSTATUS                    Status;
	POBJECT_TYPES_INFORMATION   ObjectTypes, StackPointer;

	Size = sizeof(*ObjectTypes);
	ObjectTypes = (POBJECT_TYPES_INFORMATION)AllocStack(Size);
	StackPointer = ObjectTypes;

	LOOP_FOREVER
	{

#if ML_KERNEL_MODE

		Status = ZwQueryObject(NULL, ObjectTypesInformation, ObjectTypes, Size, &Length);
		if (Status != STATUS_INFO_LENGTH_MISMATCH)
			break;

		if (ObjectTypes == StackPointer)
			ObjectTypes = NULL;

		FreeMemoryP(ObjectTypes);
		ObjectTypes = (POBJECT_TYPES_INFORMATION)AllocateMemoryP(Length);

#else
		Status = NtQueryObject(NULL, ObjectTypesInformation, ObjectTypes, Size, &Length);
		if (Status != STATUS_INFO_LENGTH_MISMATCH)
			break;

		if (ObjectTypes == StackPointer)
			ObjectTypes = NULL;

		ObjectTypes = (POBJECT_TYPES_INFORMATION)ReAllocateMemoryP(ObjectTypes, Length);

#endif

		if (ObjectTypes == NULL)
			return NULL;

		Size = Length;
	}

		if (!NT_SUCCESS(Status))
		{
			FreeMemoryP(ObjectTypes);
			ObjectTypes = NULL;
		}

	return ObjectTypes;
}

PSYSTEM_HANDLE_INFORMATION_EX QuerySystemHandles()
{
	ULONG                           Size, Length;
	NTSTATUS                        Status;
	PSYSTEM_HANDLE_INFORMATION_EX   SystemHandles;

	Size = sizeof(*SystemHandles) * 2;

	SystemHandles = (PSYSTEM_HANDLE_INFORMATION_EX)AllocateMemoryP(Size);
	if (SystemHandles == nullptr)
		return nullptr;

	LOOP_FOREVER
	{

#if ML_KERNEL_MODE

		Status = ZwQuerySystemInformation(SystemExtendedHandleInformation, SystemHandles, Size, &Length);
		if (Status != STATUS_INFO_LENGTH_MISMATCH)
			break;

		FreeMemoryP(SystemHandles);
		SystemHandles = (PSYSTEM_HANDLE_INFORMATION_EX)AllocateMemoryP(Length);

#else

		Status = NtQuerySystemInformation(SystemExtendedHandleInformation, SystemHandles, Size, &Length);
		if (Status != STATUS_INFO_LENGTH_MISMATCH)
			break;

		SystemHandles = (PSYSTEM_HANDLE_INFORMATION_EX)ReAllocateMemoryP(SystemHandles, Length);

#endif

		if (SystemHandles == nullptr)
			return nullptr;

		Size = Length;
	}

		if (!NT_SUCCESS(Status))
		{
			FreeMemoryP(SystemHandles);
			SystemHandles = nullptr;
		}

	return SystemHandles;
}

BOOL ReleaseSystemInformation(PVOID Processes)
{
	return FreeMemoryP(Processes);
}

HANDLE QueryCsrssProcessId(ULONG_PTR Session)
{
	return NULL;
}

/************************************************************************
common process
************************************************************************/

/************************************************************************
common token
************************************************************************/

ULONG_PTR
Nt_GetSessionId(
	HANDLE Process
)
{
	return ml::Native::Ps::GetSessionId(Process);
}

#if ML_KERNEL_MODE

/************************************************************************
kernel mode
************************************************************************/
LDR_MODULE* LookupPsLoadedModuleList(LDR_MODULE* LdrModule, PVOID CallDriverEntry)
{
	LDR_MODULE *PsLoadedModuleList;
	UNICODE_STRING NtName;

	RTL_CONST_STRING(NtName, L"ntoskrnl.exe");

	PsLoadedModuleList = LdrModule;
	do
	{
		PsLoadedModuleList = PsLoadedModuleList->NextLoadOrder();
		/*
		if (PsLoadedModuleList->SizeOfImage != 0 && (PsLoadedModuleList->DllBase != NULL || PsLoadedModuleList->EntryPoint != NULL))
		{
		continue;
		}
		*/
		if (!IN_RANGEEX(PsLoadedModuleList->DllBase, CallDriverEntry, PtrAdd(PsLoadedModuleList->DllBase, PsLoadedModuleList->SizeOfImage)))
			continue;

		if (!RtlEqualUnicodeString(&PsLoadedModuleList->BaseDllName, &NtName, TRUE))
			continue;

		return PsLoadedModuleList->PrevLoadOrder();

	} while (LdrModule != PsLoadedModuleList);

	return NULL;
}

NTSTATUS QuerySystemModuleByHandle(PVOID ImageBase, PRTL_PROCESS_MODULE_INFORMATION Module)
{
	NTSTATUS                        Status;
	ULONG                           ReturnedSize;
	ULONG_PTR                       Size, NumberOfModules;
	PRTL_PROCESS_MODULE_INFORMATION SystemModule;
	PRTL_PROCESS_MODULES            ModuleInfo;

	Size = 0;
	ModuleInfo = NULL;

	LOOP_FOREVER
	{
		Status = ZwQuerySystemInformation(SystemModuleInformation, ModuleInfo, Size, &ReturnedSize);
		if (Status != STATUS_INFO_LENGTH_MISMATCH)
			break;

		FreeMemory(ModuleInfo);
		ModuleInfo = (PRTL_PROCESS_MODULES)AllocateMemory(ReturnedSize - Size);
		Size = ReturnedSize;
	}

		SCOPE_EXIT
	{
		FreeMemory(ModuleInfo);
	}
	SCOPE_EXIT_END;

	if (!NT_SUCCESS(Status))
		return Status;

	if (ImageBase == NULL)
	{
		*Module = ModuleInfo->Modules[0];
		return STATUS_SUCCESS;
	}

	SystemModule = ModuleInfo->Modules;
	for (NumberOfModules = ModuleInfo->NumberOfModules; NumberOfModules != 0; ++SystemModule, --NumberOfModules)
	{
		if (IN_RANGE((ULONG_PTR)SystemModule->ImageBase, (ULONG_PTR)ImageBase, (ULONG_PTR)SystemModule->ImageBase + SystemModule->ImageSize))
		{
			*Module = *SystemModule;
			return STATUS_SUCCESS;
		}
	}

	return STATUS_DLL_NOT_FOUND;
}

NTSTATUS QueryModuleNameByHandle(PVOID ImageBase, PUNICODE_STRING ModuleName)
{
	NTSTATUS                        Status;
	ANSI_STRING                     ModuleAnsiName;
	RTL_PROCESS_MODULE_INFORMATION  SystemModule;

	Status = QuerySystemModuleByHandle(ImageBase, &SystemModule);
	if (!NT_SUCCESS(Status))
		return Status;

	RtlInitAnsiString(&ModuleAnsiName, (PSTR)SystemModule.FullPathName);
	return RtlAnsiStringToUnicodeString(ModuleName, &ModuleAnsiName, TRUE);
}

NTSTATUS
KiQueueUserApc(
	PETHREAD            Thread,
	PKNORMAL_ROUTINE    ApcRoutine,
	PVOID               ApcRoutineContext,
	PVOID               Argument1,
	PVOID               Argument2
)
{
	PKAPC       Apc;
	NTSTATUS    Status;

	Apc = (PKAPC)AllocateMemoryP(sizeof(*Apc));
	if (Apc == NULL)
		return STATUS_NO_MEMORY;

	KeInitializeApc(
		Apc,
		Thread,
		OriginalApcEnvironment,
		[](PKAPC Apc, PKNORMAL_ROUTINE *NormalRoutine, PVOID *NormalContext, PVOID *SystemArgument1, PVOID *SystemArgument2)
	{
		FreeMemoryP(Apc);
	},
		NULL,
		ApcRoutine,
		UserMode,
		ApcRoutineContext
		);

	Status = KeInsertQueueApc(Apc, Argument1, Argument2, 0);

	return Status ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

#else   // r3

/************************************************************************
user mode
************************************************************************/

/************************************************************************
user token
************************************************************************/

NTSTATUS
Nt_GetTokenInfo(
	HANDLE                  TokenHandle,
	TOKEN_INFORMATION_CLASS TokenInformationClass,
	PVOID                   Information,
	PULONG_PTR              Size
)
{
	NTSTATUS Status;
	PVOID TokenInformation;
	ULONG Length;

	TokenInformation = NULL;
	Length = 0;

	LOOP_FOREVER
	{
		Status = NtQueryInformationToken(TokenHandle, TokenInformationClass, TokenInformation, Length, &Length);
		if (Status != STATUS_BUFFER_TOO_SMALL)
			break;

		TokenInformation = ReAllocateMemory(TokenInformation, Length);
	}

		if (NT_FAILED(Status))
		{
			FreeMemory(TokenInformation);
			TokenInformation = NULL;
		}
		else
		{
			*(PVOID *)Information = TokenInformation;
			if (Size != NULL)
				*Size = Length;
		}

	return Status;
}

VOID ReleaseTokenInfo(PVOID TokenInfo)
{
	FreeMemory(TokenInfo);
}

NTSTATUS
Nt_SetProcessThreadToken(
	HANDLE Process,
	HANDLE Thread,
	HANDLE Token
)
{
	BOOLEAN                 Enabled;
	NTSTATUS                Status;
	OBJECT_ATTRIBUTES       ObjectAttributes;
	PROCESS_ACCESS_TOKEN    ProcessToken;

	InitializeObjectAttributes(&ObjectAttributes, NULL, 0, NULL, NULL);

	Status = NtDuplicateToken(Token, 0, &ObjectAttributes, FALSE, TokenPrimary, &ProcessToken.Token);
	FAIL_RETURN(Status);

	if (NT_FAILED(RtlAdjustPrivilege(SE_ASSIGNPRIMARYTOKEN_PRIVILEGE, TRUE, TRUE, &Enabled)))
		Enabled = FALSE;

	ProcessToken.Thread = Thread;
	Status = NtSetInformationProcess(Process, ProcessAccessToken, &ProcessToken, sizeof(ProcessToken));

	if (Enabled)
		RtlAdjustPrivilege(SE_ASSIGNPRIMARYTOKEN_PRIVILEGE, FALSE, TRUE, &Enabled);

	NtClose(ProcessToken.Token);

	return Status;
}

NTSTATUS
Nt_AdjustPrivilege(
	ULONG_PTR   Privilege,
	BOOL        Enable,
	BOOL        CurrentThread
)
{
	BOOLEAN Enabled;
	return RtlAdjustPrivilege(Privilege, (BOOLEAN)Enable, (BOOLEAN)CurrentThread, &Enabled);
}

/************************************************************************
debug api
************************************************************************/

NTSTATUS Nt_DebugActiveProcess(ULONG_PTR ProcessId)
{
	HANDLE      ProcessHandle;
	NTSTATUS    Status;

	Status = DbgUiConnectToDbg();
	if (!NT_SUCCESS(Status))
		return Status;

	ProcessHandle = ml::Native::Ps::ProcessIdToHandle(ProcessId);
	if (ProcessHandle == NULL)
		return STATUS_UNSUCCESSFUL;

	Status = DbgUiDebugActiveProcess(ProcessHandle);
	NtClose(ProcessHandle);

	return Status;
}

NTSTATUS Nt_DebugActiveProcessStop(ULONG_PTR ProcessId)
{
	HANDLE      Process;
	NTSTATUS    Status;

	Process = ml::Native::Ps::ProcessIdToHandle(ProcessId);
	if (Process == NULL)
		return STATUS_UNSUCCESSFUL;

	Status = DbgUiStopDebugging(Process);
	NtClose(Process);

	return Status;
}

NTSTATUS Nt_WaitForDebugEvent(PDBGUI_WAIT_STATE_CHANGE WaitState, ULONG Timeout, HANDLE DebugObject)
{
	NTSTATUS        Status;
	LARGE_INTEGER   TimeOut;

	FormatTimeOut(&TimeOut, Timeout);
	LOOP_FOREVER
	{
		Status = NtWaitForDebugEvent(DebugObject, TRUE, &TimeOut, WaitState);
		switch (Status)
		{
		case STATUS_USER_APC:
		case STATUS_ALERTED:
			continue;
		}

		if (!NT_SUCCESS(Status) || Status == STATUS_TIMEOUT)
			return Status;

		break;
	}

	return Status;
}

/************************************************************************
list entry
************************************************************************/


VOID Nt_ListLock(PCRITICAL_SECTION Lock)
{
	RtlEnterCriticalSection(Lock);
}

VOID Nt_ListUnlock(PCRITICAL_SECTION Lock)
{
	RtlLeaveCriticalSection(Lock);
}

PLIST_ENTRY
ExInterlockedInsertHeadList(
	PLIST_ENTRY         ListHead,
	PLIST_ENTRY         Entry,
	PCRITICAL_SECTION   Lock
)
{
	Nt_ListLock(Lock);
	InsertHeadList(ListHead, Entry);
	Nt_ListUnlock(Lock);
	return ListHead;
}

PLIST_ENTRY
ExInterlockedInsertTailList(
	PLIST_ENTRY         ListHead,
	PLIST_ENTRY         Entry,
	PCRITICAL_SECTION   Lock
)
{
	PLIST_ENTRY TailList;

	Nt_ListLock(Lock);
	TailList = ListHead->Blink;
	InsertTailList(ListHead, Entry);
	Nt_ListUnlock(Lock);

	return TailList;
}

PLIST_ENTRY
ExInterlockedRemoveHeadList(
	PLIST_ENTRY         ListHead,
	PCRITICAL_SECTION   Lock
)
{
	Nt_ListLock(Lock);
	ListHead = RemoveHeadList(ListHead);
	Nt_ListUnlock(Lock);
	return ListHead;
}

PSINGLE_LIST_ENTRY
ExInterlockedPopEntryList(
	PSINGLE_LIST_ENTRY  ListHead,
	PCRITICAL_SECTION   Lock
)
{
	Nt_ListLock(Lock);
	ListHead = PopEntryList(ListHead);
	Nt_ListUnlock(Lock);
	return ListHead;
}

PSINGLE_LIST_ENTRY
ExInterlockedPushEntryList(
	PSINGLE_LIST_ENTRY  ListHead,
	PSINGLE_LIST_ENTRY  Entry,
	PCRITICAL_SECTION   Lock
)
{
	Nt_ListLock(Lock);
	PushEntryList(ListHead, Entry);
	Nt_ListUnlock(Lock);
	return ListHead;
}

VOID
Nt_InitializeSListHead(
	PSLIST_HEADER SListHead
)
{
	RtlZeroMemory(SListHead, sizeof(SLIST_HEADER));
}

/************************************************************************
registry api
************************************************************************/

#if !defined(HKEY_CURRENT_USER_LOCAL_SETTINGS)
#define HKEY_CURRENT_USER_LOCAL_SETTINGS    (( HKEY ) (ULONG_PTR)((LONG)0x80000007) )
#endif

NTSTATUS Nt_OpenPredefinedKeyHandle(PHANDLE KeyHandle, HKEY PredefinedKey, ACCESS_MASK DesiredAccess)
{
	return Reg::OpenPredefinedKeyHandle(KeyHandle, PredefinedKey, DesiredAccess);
}

NTSTATUS
Nt_RegGetValue(
	HANDLE                      hKey,
	PCWSTR                      SubKey,
	PCWSTR                      ValueName,
	KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
	PVOID                       KeyValueInformation,
	ULONG                       Length,
	PULONG                      ResultLength,
	ULONG                       Flags
)
{
	return Reg::GetKeyValue(hKey, SubKey, ValueName, KeyValueInformationClass, KeyValueInformation, Length, ResultLength, Flags);
}

NTSTATUS
Nt_RegSetValue(
	HANDLE  hKey,
	PCWSTR  SubKey,
	PCWSTR  ValueName,
	ULONG   ValueType,
	LPCVOID ValueData,
	ULONG   ValueDataLength
)
{
	return Reg::SetKeyValue(hKey, SubKey, ValueName, ValueType, ValueData, ValueDataLength);
}

NTSTATUS
Nt_RegDeleteValue(
	HANDLE  hKey,
	PCWSTR  SubKey,
	PCWSTR  ValueName
)
{
	return Reg::DeleteKeyValue(hKey, SubKey, ValueName);
}

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
)
{
	return ZwProtectVirtualMemory(ProcessHandle, &BaseAddress, &Size, NewProtect, OldProtect);
}

NTSTATUS
Nt_ReadMemory(
	HANDLE      ProcessHandle,
	PVOID       BaseAddress,
	PVOID       Buffer,
	ULONG_PTR   Size,
	PULONG_PTR  BytesRead /* = NULL */
)
{
	return ZwReadVirtualMemory(ProcessHandle, BaseAddress, Buffer, Size, BytesRead);
}

NTSTATUS
Nt_WriteMemory(
	HANDLE      ProcessHandle,
	PVOID       BaseAddress,
	PVOID       Buffer,
	ULONG_PTR   Size,
	PULONG_PTR  BytesWritten /* = NULL */
)
{
	return ZwWriteVirtualMemory(ProcessHandle, BaseAddress, Buffer, Size, BytesWritten);
}

NTSTATUS
Nt_WriteProtectMemory(
	HANDLE      ProcessHandle,
	PVOID       BaseAddress,
	PVOID       Buffer,
	ULONG_PTR   Size,
	PULONG_PTR  BytesWritten /* = NULL */
)
{
	ULONG       Protect;
	NTSTATUS    Status;

	Status = Nt_ProtectMemory(ProcessHandle, BaseAddress, Size, PAGE_EXECUTE_READWRITE, &Protect);
	FAIL_RETURN(Status);

	Status = Nt_WriteMemory(ProcessHandle, BaseAddress, Buffer, Size, BytesWritten);

	if (Protect != PAGE_EXECUTE_READWRITE)
		Nt_ProtectMemory(ProcessHandle, BaseAddress, Size, Protect, &Protect);

	return Status;
}


#if ML_AMD64

NTSTATUS
InjectDllToRemoteProcess(
	HANDLE          ProcessHandle,
	HANDLE          ThreadHandle,
	PUNICODE_STRING DllFullPath,
	ULONG_PTR       Flags,
	PVOID*          InjectBuffer,
	ULONG_PTR       Timeout
)
{
	return STATUS_NOT_IMPLEMENTED;
}

#elif ML_X86

ASM PVOID InjectRemote_LoadDll()
{
	INLINE_ASM
	{
		__emit  0xCC;

		push eax;           // ret addr
		pushad;
		pushfd;
		call SELF_LOCALIZATION;
	SELF_LOCALIZATION:
		pop  esi;
		and  esi, 0FFFFF000h;
		lodsd;
		mov[esp + 0x24], eax;   // ret addr
		lodsd;
		xor  ecx, ecx;
		push ecx;
		push esp;               // pModuleHandle
		push esi;               // ModuleFileName
		push ecx;               // Flags
		push ecx;               // PathToFile
		call eax;               // LdrLoadDll
		pop  eax;               // pop ModuleHandle
		xchg eax, esi;
		and  eax, 0FFFFF000h;
		and  dword ptr[eax], 0;
		popfd;
		popad;
		ret;
	}
}

static ASM VOID InjectRemote_LoadDllEnd() { ASM_UNIQUE() }

ASM VOID InjectRemote_IssueRemoteBreakin()
{
	INLINE_ASM
	{
		call    SELF_LOCATE;
	SELF_LOCATE:
		pop     ecx;
		lea     ecx,[ecx + 14];
		push    ecx;
		xor     eax, eax;
		push    dword ptr fs : [eax];
		mov     dword ptr fs : [eax], esp;
		__emit  0xCC;

		call    SELF_LOCATE2;
	SELF_LOCATE2:
		pop     esi;
		and     esi, not(MEMORY_PAGE_SIZE - 1);
		lodsd;
		mov     ebx, eax;
		lodsd;
		mov     ecx,[esi];
		jecxz   ISSUE_BREAK_IN_ONLY;

		xor     ecx, ecx;
		push    eax;    // sub esp, 4
		push    esp;    // pModuleHandle
		push    esi;    // ModuleFileName
		push    ecx;    // Flags
		push    ecx;    // PathToFile
		call    eax;    // LdrLoadDll
		//        pop     eax;    // add esp, 4;

	ISSUE_BREAK_IN_ONLY:

		xchg eax, esi;
		and  eax, 0FFFFF000h;
		and  dword ptr[eax], 0;

	EXIT_THREAD:
		//        push    0;
		push - 2;
		call    ebx;
		jmp     EXIT_THREAD;
	}
}

static ASM VOID InjectRemote_IssueRemoteBreakinEnd() { ASM_UNIQUE() }

NTSTATUS
InjectDllToRemoteProcess(
	HANDLE          ProcessHandle,
	HANDLE          ThreadHandle,
	PUNICODE_STRING DllFullPath,
	ULONG_PTR       Flags,
	PVOID*          InjectBuffer,
	ULONG_PTR       Timeout
)
{
	NTSTATUS        Status;
	PVOID           Buffer, InjectContext, ShellCode;
	ULONG_PTR       DllPathLength, InjectContextSize, ShellCodeSize;
	CONTEXT         ThreadContext;
	LARGE_INTEGER   TimeOut;

	typedef struct
	{
		ULONG           ReturnAddr;
		PVOID           xLdrLoadDll;
		UNICODE_STRING  DllPath;
	} INJECT_REMOTE_LOAD_DLL;

	typedef struct
	{
		PVOID           xNtTerminateThread;
		PVOID           xLdrLoadDll;
		UNICODE_STRING  DllPath;
		//    WCHAR           Buffer[1];
	} INJECT_REMOTE_ISSUE_BREAK_IN;

	union
	{
		INJECT_REMOTE_LOAD_DLL          LoadDll;
		INJECT_REMOTE_ISSUE_BREAK_IN    Breakin;
	};

	Buffer = NULL;
	if (FLAG_ON(Flags, INJECT_PREALLOC_BUFFER))
	{
		if (InjectBuffer == NULL)
			return STATUS_INVALID_PARAMETER;

		Buffer = *InjectBuffer;
		if (Buffer == NULL)
			return STATUS_NO_MEMORY;
	}
	else if (InjectBuffer != NULL)
	{
		*InjectBuffer = NULL;
	}

	ThreadContext.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
	Status = NtGetContextThread(ThreadHandle, &ThreadContext);
	if (!NT_SUCCESS(Status))
		return Status;

	if (!FLAG_ON(Flags, INJECT_PREALLOC_BUFFER))
	{
		Status = Mm::AllocVirtualMemoryEx(ProcessHandle, &Buffer, MEMORY_PAGE_SIZE);
		if (!NT_SUCCESS(Status))
			return Status;
	}

	if (FLAG_OFF(Flags, INJECT_ISSUE_BREAKIN_ONLY) && DllFullPath != nullptr)
	{
		DllPathLength = DllFullPath->Length;
	}
	else
	{
		DllPathLength = 0;
	}

	if (FLAG_ON(Flags, INJECT_ISSUE_REMOTE_BREAKIN | INJECT_ISSUE_BREAKIN_ONLY))
	{
		PVOID ntdll;

		InjectContext = &Breakin;
		InjectContextSize = sizeof(Breakin);
		ShellCode = InjectRemote_IssueRemoteBreakin;
		ShellCodeSize = PtrOffset(InjectRemote_IssueRemoteBreakinEnd, InjectRemote_IssueRemoteBreakin);

		ntdll = GetNtdllHandle();

		Breakin.xLdrLoadDll = EATLookupRoutineByHashPNoFix(ntdll, NTDLL_LdrLoadDll);
		Breakin.xNtTerminateThread = EATLookupRoutineByHashPNoFix(ntdll, NTDLL_NtTerminateThread);

		if (FLAG_ON(Flags, INJECT_ISSUE_BREAKIN_ONLY))
		{
			RtlInitEmptyUnicodeString(&Breakin.DllPath, NULL, 0);
		}
		else
		{
			Breakin.DllPath.Length = (USHORT)DllPathLength;
			Breakin.DllPath.MaximumLength = (USHORT)DllPathLength;
			Breakin.DllPath.Buffer = (PWSTR)((ULONG_PTR)Buffer + sizeof(Breakin));
		}
	}
	else
	{
		PVOID ntdll;

		InjectContext = &LoadDll;
		InjectContextSize = sizeof(LoadDll);
		ShellCode = PtrAdd(InjectRemote_LoadDll, FLAG_OFF(Flags, INJECT_ISSUE_BREAKIN));
		ShellCodeSize = PtrOffset(InjectRemote_LoadDllEnd, InjectRemote_LoadDll);

		ntdll = GetNtdllHandle();

		LoadDll.xLdrLoadDll = EATLookupRoutineByHashPNoFix(ntdll, NTDLL_LdrLoadDll);
		LoadDll.ReturnAddr = ThreadContext.Eip;
		LoadDll.DllPath.Length = (USHORT)DllPathLength;
		LoadDll.DllPath.MaximumLength = (USHORT)DllPathLength;
		LoadDll.DllPath.Buffer = (PWSTR)((ULONG_PTR)Buffer + sizeof(LoadDll));
	}

	Status = STATUS_UNSUCCESSFUL;
	LOOP_ONCE
	{
		Status = WriteMemory(ProcessHandle, Buffer, InjectContext, InjectContextSize);
		if (!NT_SUCCESS(Status))
			break;

		if (DllFullPath != nullptr && FLAG_OFF(Flags, INJECT_ISSUE_BREAKIN_ONLY))
		{
			Status = WriteMemory(
				ProcessHandle,
				(PVOID)((ULONG_PTR)Buffer + InjectContextSize),
				(PVOID)DllFullPath->Buffer,
				DllPathLength
				);
			if (!NT_SUCCESS(Status))
				break;
		}

		ThreadContext.Eip = ROUND_UP((ULONG_PTR)Buffer + InjectContextSize + DllPathLength, 16);
		Status = WriteMemory(
			ProcessHandle,
			(PVOID)ThreadContext.Eip,
			ShellCode,
			ShellCodeSize
			);
		if (!NT_SUCCESS(Status))
			break;

		NtFlushInstructionCache(ProcessHandle, (PVOID)ThreadContext.Eip, ShellCodeSize);

		if (FLAG_OFF(Flags, INJECT_DONT_CHANGE_IP))
		{
			Status = NtSetContextThread(ThreadHandle, &ThreadContext);
			if (!NT_SUCCESS(Status))
				break;

			Status = NtGetContextThread(ThreadHandle, &ThreadContext);
		}

		if (FLAG_ON(Flags, INJECT_THREAD_SUSPENDED))
		{
			if (InjectBuffer != NULL)
			{
				*InjectBuffer = FLAG_ON(Flags, INJECT_POINT_TO_SHELL_CODE) ? (PVOID)ThreadContext.Eip : Buffer;
			}

			return Status;
		}

		Status = NtResumeThread(ThreadHandle, NULL);
		if (!NT_SUCCESS(Status))
			break;

		FormatTimeOut(&TimeOut, Timeout);
		for (ULONG TryTimes = 30; TryTimes; --TryTimes)
		{
			ULONG Val;
			Status = ReadMemory(ProcessHandle, Buffer, &Val, sizeof(Val));
			if (!NT_SUCCESS(Status))
				break;

			if (Val != 0)
			{
				NtDelayExecution(FALSE, &TimeOut);
				continue;
			}

			break;
		}

		if (!NT_SUCCESS(Status))
			break;

		NtDelayExecution(FALSE, &TimeOut);
		Status = NtGetContextThread(ThreadHandle, &ThreadContext);
		if (!NT_SUCCESS(Status))
			break;

		if ((ULONG_PTR)ThreadContext.Eip < (ULONG_PTR)Buffer ||
			(ULONG_PTR)ThreadContext.Eip >(ULONG_PTR)Buffer + MEMORY_PAGE_SIZE)
		{
			Status = STATUS_SUCCESS;
		}
		else
		{
			Status = STATUS_UNSUCCESSFUL;
		}
	}

		if (FLAG_OFF(Flags, INJECT_PREALLOC_BUFFER))
			Mm::FreeVirtualMemory(Buffer, ProcessHandle);

	return Status;
}

#else

NTSTATUS
InjectDllToRemoteProcess(
	HANDLE          ProcessHandle,
	HANDLE          ThreadHandle,
	PUNICODE_STRING DllFullPath,
	ULONG_PTR       Flags,
	PVOID*          InjectBuffer,
	ULONG_PTR       Timeout
)
{
	return STATUS_NOT_IMPLEMENTED;
}

#endif // x64

VOID Nt_ExitProcess(ULONG ExitCode)
{
	return ml::Native::Ps::ExitProcess(ExitCode);
}

NTSTATUS
Nt_CreateProcess2(
	CreateProcessInternal2Routine   Routine,
	HANDLE                          Token,
	PCWSTR                          ApplicationName,
	PWSTR                           CommandLine,
	PCWSTR                          CurrentDirectory,
	ULONG                           CreationFlags,
	LPSTARTUPINFOW                  StartupInfo,
	LPPROCESS_INFORMATION           ProcessInformation,
	LPSECURITY_ATTRIBUTES           ProcessAttributes,
	LPSECURITY_ATTRIBUTES           ThreadAttributes,
	PVOID                           Environment
)
{
	BOOL                            Result;
	STARTUPINFOW                    DefaultStartupInfo;
	PROCESS_INFORMATION             DefaultProcessInformation;

	SEH_TRY
	{
		if (StartupInfo == NULL)
		{
			ZeroMemory(&DefaultStartupInfo, sizeof(DefaultStartupInfo));
			DefaultStartupInfo.cb = sizeof(DefaultStartupInfo);
			StartupInfo = &DefaultStartupInfo;
		}

		if (ProcessInformation == NULL)
			ProcessInformation = &DefaultProcessInformation;

		CREATE_PROCESS_INTERNAL_PARAM Parameter =
		{
			Token,
			ApplicationName,
			CommandLine,
			ProcessAttributes,
			ThreadAttributes,
			FALSE,
			CreationFlags,
			Environment,
			CurrentDirectory,
			StartupInfo,
			ProcessInformation,
			NULL,
		};

		Result = Routine(Parameter);
		if (!Result)
			return STATUS_UNSUCCESSFUL;

		if (ProcessInformation == &DefaultProcessInformation)
		{
			NtClose(DefaultProcessInformation.hProcess);
			NtClose(DefaultProcessInformation.hThread);
		}

		return STATUS_SUCCESS;
	}
		SEH_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
		;
	}

	return STATUS_UNSUCCESSFUL;
}

NTSTATUS
Nt_CreateProcess(
	HANDLE                  Token,
	PCWSTR                  ApplicationName,
	PWSTR                   CommandLine,
	PCWSTR                  CurrentDirectory,
	ULONG                   CreationFlags,
	LPSTARTUPINFOW          StartupInfo,
	LPPROCESS_INFORMATION   ProcessInformation,
	LPSECURITY_ATTRIBUTES   ProcessAttributes,
	LPSECURITY_ATTRIBUTES   ThreadAttributes,
	PVOID                   Environment
)
{
	BOOL                            Result;
	STARTUPINFOW                    DefaultStartupInfo;
	PROCESS_INFORMATION             DefaultProcessInformation;

	if (StartupInfo == NULL)
	{
		ZeroMemory(&DefaultStartupInfo, sizeof(DefaultStartupInfo));
		DefaultStartupInfo.cb = sizeof(DefaultStartupInfo);
		StartupInfo = &DefaultStartupInfo;
	}

	if (ProcessInformation == NULL)
		ProcessInformation = &DefaultProcessInformation;

#if ML_DISABLE_THIRD_LIB

	Result = CreateProcessAsUserW(
		Token,
		ApplicationName,
		(PWSTR)CommandLine,
		ProcessAttributes,
		ThreadAttributes,
		FALSE,
		CreationFlags,
		Environment,
		CurrentDirectory,
		StartupInfo,
		ProcessInformation
	);

#else

	API_POINTER(CreateProcessInternalW) StubCreateProcessInternalW = (API_POINTER(CreateProcessInternalW))
		EATLookupRoutineByHashPNoFix(
			GetKernel32Handle(), 
			KERNEL32_CreateProcessInternalW
		);

	if (StubCreateProcessInternalW)
	{
		Result = StubCreateProcessInternalW(
			Token,
			ApplicationName,
			CommandLine,
			ProcessAttributes,
			ThreadAttributes,
			FALSE,
			CreationFlags,
			Environment,
			CurrentDirectory,
			StartupInfo,
			ProcessInformation,
			NULL
		);
	}
	else
	{
		Result = FALSE;
	}

#endif // ML_DISABLE_THIRD_LIB

	if (!Result)
		return STATUS_UNSUCCESSFUL;

	if (ProcessInformation == &DefaultProcessInformation)
	{
		NtClose(DefaultProcessInformation.hProcess);
		NtClose(DefaultProcessInformation.hThread);
	}

	return STATUS_SUCCESS;
}

NTSTATUS
Nt_CreateThread(
	PVOID                   StartAddress,
	PVOID                   StartParameter,
	BOOL                    CreateSuspended,
	HANDLE                  ProcessHandle,
	PHANDLE                 ThreadHandle,
	PCLIENT_ID              ClientID,
	ULONG                   StackZeroBits,
	ULONG                   StackReserved,
	ULONG                   StackCommit,
	PSECURITY_DESCRIPTOR    SecurityDescriptor
)
{
	NTSTATUS    Status;
	CLIENT_ID   ThreadId;
	HANDLE      NewThreadHandle;

	Status = RtlCreateUserThread(
		ProcessHandle,
		SecurityDescriptor,
		(BOOLEAN)CreateSuspended,
		StackZeroBits,
		StackReserved,
		StackCommit,
		(PUSER_THREAD_START_ROUTINE)StartAddress,
		StartParameter,
		&NewThreadHandle,
		&ThreadId
	);
	if (!NT_SUCCESS(Status))
		return Status;

	if (ClientID != NULL)
		*ClientID = ThreadId;

	if (ThreadHandle != NULL)
	{
		*ThreadHandle = NewThreadHandle;
	}
	else
	{
		NtClose(NewThreadHandle);
	}

	return Status;
}

NTSTATUS
Nt_SetThreadPriority(
	HANDLE  ThreadHandle,
	LONG    Priority
)
{
	NTSTATUS Status;

	switch (Priority)
	{
	case THREAD_PRIORITY_TIME_CRITICAL:
		Priority = (HIGH_PRIORITY + 1) / 2;
		break;

	case THREAD_PRIORITY_IDLE:
		Priority = -((HIGH_PRIORITY + 1) / 2);
		break;
	}

	Status = NtSetInformationThread(
		ThreadHandle,
		ThreadBasePriority,
		&Priority,
		sizeof(LONG)
	);

	return Status;
}

LDR_MODULE* Nt_FindLdrModuleByName(PUNICODE_STRING ModuleName)
{
	return ml::Native::Ldr::FindLdrModuleByName(ModuleName);
}

LDR_MODULE* Nt_FindLdrModuleByHandle(PVOID BaseAddress)
{
	return ml::Native::Ldr::FindLdrModuleByHandle(BaseAddress);
}

PTEB_ACTIVE_FRAME Nt_FindThreadFrameByContext(ULONG_PTR Context)
{
	return ml::Native::Ps::FindThreadFrame(Context);
}


/************************************************************************
path
************************************************************************/

ULONG
Nt_GetSystemDirectory64(
	PWSTR Buffer,
	ULONG BufferCount
)
{
	ULONG                           Length, ResultLength;
	NTSTATUS                        Status;
	PKEY_VALUE_PARTIAL_INFORMATION  ValuePartial;

	Length = MAX_NTPATH * 2 + sizeof(*ValuePartial);
	ValuePartial = (PKEY_VALUE_PARTIAL_INFORMATION)AllocStack(Length);

	LOOP_FOREVER
	{
		Status = Nt_RegGetValue(
		HKEY_LOCAL_MACHINE,
		L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Installer",
		L"InstallerLocation",
		KeyValuePartialInformation,
		ValuePartial,
		Length,
		&ResultLength,
		KEY_WOW64_64KEY
		);
		if (Status != STATUS_BUFFER_TOO_SMALL || ResultLength <= Length)
			break;

		ValuePartial = (PKEY_VALUE_PARTIAL_INFORMATION)AllocStack(ResultLength - Length);
		Length = ResultLength;
	}

		if (!NT_SUCCESS(Status))
			return 0;

	Length = MY_MIN(ValuePartial->DataLength, BufferCount * sizeof(WCHAR));
	CopyMemory(Buffer, ValuePartial->Data, Length);

	Length /= sizeof(WCHAR);
	while (Buffer[Length - 1] == 0)
		--Length;

	if (Length < BufferCount)
	{
		if (Buffer[Length - 1] != '\\')
			Buffer[Length++] = '\\';
	}

	if (Length < BufferCount)
		Buffer[Length] = 0;

	return Length;
}

ULONG Nt_GetWindowsDirectory(PWCHAR Buffer, ULONG BufferCount)
{
	ULONG Length;
	LDR_MODULE* LdrModule;

	LdrModule = FIELD_BASE(Nt_CurrentPeb()->Ldr->InInitializationOrderModuleList.Flink, LDR_MODULE, InInitializationOrderLinks);
	Length = (LdrModule->FullDllName.Length - LdrModule->BaseDllName.Length) / sizeof(WCHAR) - 2;

	while (LdrModule->FullDllName.Buffer[Length] != '\\' && --Length);
	if (Length == 0)
		return 0;

	++Length;
	Length *= sizeof(WCHAR);
	Length = MY_MIN(Length, BufferCount * sizeof(WCHAR));
	CopyMemory(Buffer, LdrModule->FullDllName.Buffer, Length);
	Length /= sizeof(WCHAR);
	if (Length < BufferCount)
		Buffer[Length] = 0;

	return Length;
}

ULONG Nt_GetSystemDirectory(PWCHAR Buffer, ULONG BufferCount)
{
	ULONG Length;
	LDR_MODULE* LdrModule;

	LdrModule = FIELD_BASE(Nt_CurrentPeb()->Ldr->InInitializationOrderModuleList.Flink, LDR_MODULE, InInitializationOrderLinks);
	Length = LdrModule->FullDllName.Length - LdrModule->BaseDllName.Length;

	if (Buffer == NULL)
		return Length / sizeof(WCHAR);

	Length = MY_MIN(Length, BufferCount * sizeof(WCHAR));
	CopyMemory(Buffer, LdrModule->FullDllName.Buffer, Length);
	Length /= sizeof(WCHAR);
	if (Length < BufferCount)
		Buffer[Length] = 0;

	return Length;
}

ULONG_PTR
Nt_GetDosPathFromNtDeviceName(
	PWSTR           DosPath,
	ULONG_PTR       DosPathLength,
	PUNICODE_STRING NtDeviceName
)
{
	ULONG_PTR       Length;
	NTSTATUS        Status;
	UNICODE_STRING  DosPathString;

	Status = Io::QueryDosPathFromNtDeviceName(&DosPathString, NtDeviceName);
	if (NT_FAILED(Status))
		return 0;

	DosPathLength = InternalCopyUnicodeString(&DosPathString, DosPath, DosPathLength);
	RtlFreeUnicodeString(&DosPathString);

	return DosPathLength;
}

NTSTATUS
Nt_GetDosLetterFromVolumeDeviceName(
	PUNICODE_STRING DosLetter,
	PUNICODE_STRING NtDeviceName
)
{
	return Io::QueryDosLetterFromVolumeDeviceName(DosLetter, NtDeviceName);
}

NTSTATUS Nt_GetDosPathFromHandle(PUNICODE_STRING DosPath, HANDLE FileHandle)
{
	return Io::QueryDosPathFromHandle(DosPath, FileHandle);
}

NTSTATUS Nt_NtPathNameToDosPathName(PUNICODE_STRING DosPath, PUNICODE_STRING NtPath)
{
	return Rtl::NtPathNameToDosPathName(DosPath, NtPath);
}

VOID ConvertFindData(PWIN32_FIND_DATAW W32, PML_FIND_DATA ML)
{
	CopyStruct(W32->cFileName, ML->FileName, sizeof(W32->cFileName));
	CopyStruct(W32->cAlternateFileName, ML->AlternateFileName, sizeof(W32->cAlternateFileName));

	W32->dwFileAttributes = ML->FileAttributes;
	W32->ftCreationTime.dwLowDateTime = ML->CreationTime.LowPart;
	W32->ftCreationTime.dwHighDateTime = ML->CreationTime.HighPart;

	W32->ftLastWriteTime.dwLowDateTime = ML->LastWriteTime.LowPart;
	W32->ftLastWriteTime.dwHighDateTime = ML->LastWriteTime.HighPart;

	W32->ftLastAccessTime.dwLowDateTime = ML->LastAccessTime.LowPart;
	W32->ftLastAccessTime.dwHighDateTime = ML->LastAccessTime.HighPart;

	W32->dwReserved0 = ML->Reserved0;
	W32->dwReserved1 = ML->Reserved1;

	W32->nFileSizeLow = ML->FileSize.LowPart;
	W32->nFileSizeHigh = ML->FileSize.HighPart;
}

BOOL Nt_FindClose(HANDLE FindFileHandle)
{
	return NT_SUCCESS(Io::QueryClose(FindFileHandle));
}

HANDLE Nt_FindFirstFile(LPCWSTR FileName, PWIN32_FIND_DATAW FindFileData)
{
	NTSTATUS        Status;
	HANDLE          DirectoryHandle;
	ML_FIND_DATA    FindData;

	Status = Io::QueryFirstFile(&DirectoryHandle, FileName, &FindData);
	if (NT_SUCCESS(Status))
		ConvertFindData(FindFileData, &FindData);

	return NT_SUCCESS(Status) ? DirectoryHandle : INVALID_HANDLE_VALUE;
}

BOOL Nt_FindNextFile(HANDLE FindFileHandle, PWIN32_FIND_DATAW FindFileData)
{
	NTSTATUS        Status;
	ML_FIND_DATA    FindData;

	Status = Io::QueryNextFile(FindFileHandle, &FindData);
	if (NT_SUCCESS(Status))
		ConvertFindData(FindFileData, &FindData);

	return NT_SUCCESS(Status);
}

ULONG Nt_GetFileAttributes(LPCWSTR FileName)
{
	return Io::QueryFileAttributes(FileName);
}

NTSTATUS Nt_MoveFile(LPCWSTR ExistingFileName, LPCWSTR NewFileName)
{
	return Io::MoveFile(ExistingFileName, NewFileName);
}

NTSTATUS Nt_CopyFile(PCWSTR ExistingFileName, PCWSTR NewFileName, BOOL FailIfExists)
{
	return Io::CopyFile(ExistingFileName, NewFileName, FailIfExists);
}

BOOL Nt_IsPathExists(LPCWSTR pszPath)
{
	return Io::IsPathExists(pszPath);
}

ULONG_PTR Nt_GetModulePath(PVOID ModuleBase, PWSTR Path, ULONG_PTR BufferCount)
{
	LDR_MODULE  *LdrModule;
	ULONG_PTR   PathLength;

	LdrModule = Nt_FindLdrModuleByHandle(ModuleBase);
	PathLength = (LdrModule->FullDllName.Length - LdrModule->BaseDllName.Length) / sizeof(WCHAR);

	InternalCopyUnicodeString(&LdrModule->FullDllName, Path, PathLength);
	if (PathLength < BufferCount)
		Path[PathLength] = 0;

	return PathLength;
}

ULONG Nt_GetExeDirectory(PWCHAR Path, ULONG BufferCount)
{
	return Nt_GetModulePath(NULL, Path, BufferCount);
}

ULONG_PTR Nt_GetModuleFileName(PVOID ModuleBase, LPWSTR Filename, ULONG_PTR BufferCount)
{
	ULONG_PTR               Length;
	PEB                     *Peb;
	LDR_DATA_TABLE_ENTRY    *LdrModule;
	LDR_DATA_TABLE_ENTRY    *FirstLdrModule;

	Peb = Nt_CurrentPeb();
	LdrModule = FIELD_BASE(Peb->Ldr->InLoadOrderModuleList.Flink, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

	FirstLdrModule = LdrModule;

	if (ModuleBase == NULL)
		ModuleBase = Peb->ImageBaseAddress;

	LOOP_FOREVER
	{
		if ((ULONG_PTR)ModuleBase >= (ULONG_PTR)LdrModule->DllBase &&
		(ULONG_PTR)ModuleBase < (ULONG_PTR)LdrModule->DllBase + LdrModule->SizeOfImage)
		{
			break;
		}

		LdrModule = (PLDR_DATA_TABLE_ENTRY)LdrModule->InLoadOrderLinks.Flink;
		if (LdrModule == FirstLdrModule)
			return 0;
	}

	Length = InternalCopyUnicodeString(&LdrModule->FullDllName, Filename, BufferCount);
	if (LdrModule->FullDllName.Length / sizeof(WCHAR) < BufferCount)
		Filename[BufferCount] = 0;

	return Length;
}

BOOL Nt_SetExeDirectoryAsCurrent()
{
	return NT_SUCCESS(Rtl::SetExeDirectoryAsCurrent());
}

/************************************************************************
misc
************************************************************************/

PVOID Nt_LoadLibrary(PCWSTR ModuleFileName)
{
	return ml::Native::Ldr::LoadDll(ModuleFileName);
}

PVOID FASTCALL Nt_GetProcAddress(PVOID ModuleBase, LPCSTR lpProcName)
{
	return Ldr::GetRoutineAddress(ModuleBase, lpProcName);
}

PVOID Nt_GetModuleHandle(LPCWSTR lpModuleName)
{
	UNICODE_STRING  ModuleFileName;
	LDR_MODULE*     Module;

	RtlInitUnicodeString(&ModuleFileName, lpModuleName);
	Module = Ldr::FindLdrModuleByName(&ModuleFileName);
	return Module == NULL ? NULL : Module->DllBase;
}

VOID Nt_Sleep(ULONG_PTR Milliseconds, BOOL Alertable)
{
	return Ps::Sleep(Milliseconds, Alertable);
}

#endif  // MY_NT_DDK

_ML_C_TAIL_

#pragma warning(default:4245)
#pragma warning(default:4750)


#if ML_KERNEL_MODE

NTSTATUS
ProbeForReadSafe(
	PVOID   Address,
	SIZE_T  Length,
	ULONG   Alignment
)
{
	NTSTATUS Status = STATUS_SUCCESS;

	SEH_TRY
	{
		ProbeForRead(Address, Length, Alignment);
	}
		SEH_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
		Status = GetExceptionCode();
	}

	return Status;
}

NTSTATUS
ProbeForWriteSafe(
	PVOID   Address,
	SIZE_T  Length,
	ULONG   Alignment
)
{
	NTSTATUS Status = STATUS_SUCCESS;

	SEH_TRY
	{
		ProbeForWrite(Address, Length, Alignment);
	}
		SEH_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
		Status = GetExceptionCode();
	}

	return Status;
}

NTSTATUS
MmProbeAndLockPagesSafe(
	PMDL            MemoryDescriptorList,
	KPROCESSOR_MODE AccessMode,
	LOCK_OPERATION  Operation
)
{
	NTSTATUS Status = STATUS_SUCCESS;

	SEH_TRY
	{
		MmProbeAndLockPages(MemoryDescriptorList, AccessMode, Operation);
	}
		SEH_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
		Status = GetExceptionCode();
	}

	return Status;
}

#else // r3

#pragma comment(lib, "dbghelp.lib")
#include <DbgHelp.h>


NTSTATUS CreateMiniDump(PEXCEPTION_POINTERS ExceptionPointers)
{
	WCHAR           MiniDumpFile[MAX_NTPATH];
	PWSTR           FormatBuffer;
	NtFileDisk      File;
	BOOL            Success;
	NTSTATUS        Status;
	PROCESS_TIMES   Times;

	MINIDUMP_EXCEPTION_INFORMATION ExceptionInformation;

	NtQueryInformationProcess(NtCurrentProcess(), ProcessTimes, &Times, sizeof(Times), NULL);

	Nt_GetModuleFileName(NULL, MiniDumpFile, countof(MiniDumpFile));
	FormatBuffer = findextw(MiniDumpFile);
	swprintf_s(FormatBuffer, countof(MiniDumpFile) - ((FormatBuffer - MiniDumpFile) / 2),  L".%I64X.crash.dmp", Times.CreationTime.QuadPart);

	Status = File.Create(MiniDumpFile);
	FAIL_RETURN(Status);

	ExceptionInformation.ClientPointers = FALSE;
	ExceptionInformation.ExceptionPointers = ExceptionPointers;
	ExceptionInformation.ThreadId = Ps::CurrentTid();

	Success = MiniDumpWriteDump(
		NtCurrentProcess(),
		(ULONG_PTR)Ps::CurrentTeb()->ClientId.UniqueProcess,
		File,
		MiniDumpNormal,
		&ExceptionInformation,
		NULL,
		NULL
	);

	return Success ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

/************************************************************************
directory
************************************************************************/
BOOL IsPathExistsW(LPCWSTR pszPath)
{
	if (GetFileAttributesW(pszPath) != -1)
		return TRUE;

	switch (GetLastError())
	{
	case ERROR_FILE_NOT_FOUND:
	case ERROR_PATH_NOT_FOUND:
	case ERROR_INVALID_NAME:
		return FALSE;
	}

	return TRUE;
}

BOOL IsPathExistsA(LPCSTR pszPath)
{
	WCHAR szPath[MAX_PATH];

	MultiByteToWideChar(CP_ACP, 0, pszPath, -1, szPath, countof(szPath));

	return IsPathExistsW(szPath);
}

BOOL CreateDirectoryWorker(LPCWSTR PathName)
{
#if USE_NT_VER
	NTSTATUS    Status;
	HANDLE      DirectoryHandle;

	Status = NtFileDisk::CreateDirectory(&DirectoryHandle, PathName);
	if (!NT_SUCCESS(Status))
		return FALSE;

	NtClose(DirectoryHandle);
	return TRUE;

#else
	return CreateDirectoryW(PathName, NULL);
#endif // USE_NT_VER
}

BOOL CreateDirectoryRecursiveW(LPCWSTR lpPathName)
{
	WCHAR  ch;
	LPWSTR lpPath = (LPWSTR)lpPathName;

	if (lpPath == NULL || *lpPath == 0)
		return FALSE;

#if USE_NT_VER
	if (Nt_IsPathExists(lpPathName))
		return TRUE;
#else
	if (IsPathExistsW(lpPathName))
		return TRUE;
#endif // USE_NT_VER

	do
	{
		if (*lpPath == '\\' || *lpPath == '/')
		{
			if (*(lpPath + 1) == 0)
			{
				break;
			}
			ch = *lpPath;
			*lpPath = 0;
			CreateDirectoryWorker(lpPathName);
			*lpPath = ch;
		}
		++lpPath;
	} while (*lpPath);

	return CreateDirectoryWorker(lpPathName);
}

BOOL FASTCALL MyCreateDirectoryA(LPCSTR lpPathName)
{
	WCHAR szPath[MAX_PATH * 2];

	MultiByteToWideChar(CP_ACP, 0, lpPathName, -1, szPath, countof(szPath));
	return CreateDirectoryRecursiveW(szPath);
}

ULONG GetExeDirectoryW(LPWSTR pszPath, ULONG BufferCount)
{
	WCHAR szPath[MAX_PATH];
	ULONG i;

	if (pszPath == NULL)
	{
		pszPath = szPath;
		BufferCount = countof(szPath);
	}

	i = GetModuleFileNameW(NULL, pszPath, BufferCount);
	if (i)
	{
		while (pszPath[--i] != '\\');
		pszPath[++i] = 0;
	}

	return i;
}

ULONG GetExeDirectoryA(LPSTR pszPath, ULONG BufferCount)
{
	WCHAR  szPath[MAX_PATH];
	ULONG i;

	i = GetExeDirectoryW(szPath, countof(szPath));
	if (i)
	{
		i = WideCharToMultiByte(CP_ACP, 0, szPath, i + 1, pszPath, BufferCount, NULL, NULL) - 1;
	}

	return i;
}


inline BOOL __IsParentDirectory(LPWSTR pszPath)
{
	if (*(PULONG)pszPath == '.')
		return TRUE;
	else if ((*(PULONG64)pszPath & 0x0000FFFFFFFFFFFF) == TAG2W('..'))
		return TRUE;

	return FALSE;
}

LONG ProcessFindResult(ENUM_DIRECTORY_INFO *pFindInfo, LPWSTR lpPath, LPWSTR lpFileName)
{
	LONG  Result;
	PVOID lpBuffer;

	StrCopyW(lpFileName, pFindInfo->wfd.cFileName);
	StrCopyW(pFindInfo->wfd.cFileName, lpPath);

	lpBuffer = pFindInfo->lpOutBuffer;
	if (lpBuffer != NULL)
	{
		if (pFindInfo->ElemCount.QuadPart >= pFindInfo->MaxCount.QuadPart)
		{
			pFindInfo->MaxCount.QuadPart *= 2;
#if USE_NT_VER
			lpBuffer = RtlReAllocateHeap(
				pFindInfo->hHeap,
				HEAP_ZERO_MEMORY,
				pFindInfo->lpBuffer,
				(ULONG_PTR)pFindInfo->MaxCount.QuadPart * pFindInfo->ElemSize
			);
#else
			lpBuffer = HeapReAlloc(
				pFindInfo->hHeap,
				HEAP_ZERO_MEMORY,
				pFindInfo->lpBuffer,
				(ULONG_PTR)pFindInfo->MaxCount.QuadPart * pFindInfo->ElemSize
			);
#endif // USE_NT_VER

			if (lpBuffer == NULL)
				return -1;

			pFindInfo->lpBuffer = lpBuffer;
			lpBuffer = (PBYTE)lpBuffer + pFindInfo->ElemCount.QuadPart * pFindInfo->ElemSize;
			pFindInfo->lpOutBuffer = lpBuffer;
		}

		lpBuffer = pFindInfo->lpOutBuffer;
	}

	Result = pFindInfo->CallBack(lpBuffer, &pFindInfo->wfd, pFindInfo->Context);
	if (Result != 0)
		pFindInfo->ElemCount.QuadPart += Result;

	if (lpBuffer != NULL)
		pFindInfo->lpOutBuffer = (PBYTE)lpBuffer + pFindInfo->ElemSize;

	return Result;
}

__if_not_exists(_wfinddata64_t)
{
	typedef struct
	{
		unsigned    attrib;
		__int64  time_create;    /* -1 for FAT file systems */
		__int64  time_access;    /* -1 for FAT file systems */
		__int64  time_write;
		__int64     size;
		wchar_t     name[260];
	} _wfinddata64_t;
}

VOID CopyCrtFindDataToWin32FindData(PWIN32_FIND_DATAW FindFileData, _wfinddata64_t *finddata)
{
	LARGE_INTEGER  LargeInt;
	ULARGE_INTEGER LargeUInt;

	FindFileData->dwFileAttributes = finddata->attrib;

	LargeInt.QuadPart = finddata->size;
	FindFileData->nFileSizeLow = LargeInt.LowPart;
	FindFileData->nFileSizeHigh = LargeInt.HighPart;

	LargeUInt.QuadPart = finddata->time_create;
	FindFileData->ftCreationTime.dwLowDateTime = LargeUInt.LowPart;
	FindFileData->ftCreationTime.dwHighDateTime = LargeUInt.HighPart;

	LargeUInt.QuadPart = finddata->time_access;
	FindFileData->ftLastAccessTime.dwLowDateTime = LargeUInt.LowPart;
	FindFileData->ftLastAccessTime.dwHighDateTime = LargeUInt.HighPart;

	LargeUInt.QuadPart = finddata->time_write;
	FindFileData->ftLastWriteTime.dwLowDateTime = LargeUInt.LowPart;
	FindFileData->ftLastWriteTime.dwHighDateTime = LargeUInt.HighPart;

	FindFileData->cAlternateFileName[0] = 0;
	CopyStruct(FindFileData->cFileName, finddata->name, MY_MIN(sizeof(finddata->name), sizeof(FindFileData->cFileName)));
}

HANDLE FindFirstFileWorker(LPCWSTR FileName, PWIN32_FIND_DATAW FindFileData)
{
#if USE_CRT_VER
	intptr_t handle;
	_wfinddata64_t finddata;

	handle = _wfindfirst64(FileName, &finddata);
	if (handle == -1)
		return INVALID_HANDLE_VALUE;

	CopyCrtFindDataToWin32FindData(FindFileData, &finddata);
	return (HANDLE)handle;

#elif USE_NT_VER
	return Nt_FindFirstFile(FileName, FindFileData);
#else
	return FindFirstFileW(FileName, FindFileData);
#endif
}

BOOL FindNextFileWorker(HANDLE FindFileHandle, PWIN32_FIND_DATAW FindFileData)
{
#if USE_CRT_VER
	_wfinddata64_t finddata;
	if (_wfindnext64((intptr_t)FindFileHandle, &finddata) == -1)
		return FALSE;

	CopyCrtFindDataToWin32FindData(FindFileData, &finddata);
	return TRUE;

#elif USE_NT_VER
	return Nt_FindNextFile(FindFileHandle, FindFileData);
#else
	return FindNextFileW(FindFileHandle, FindFileData);
#endif
}

BOOL FindCloseWorker(HANDLE FindFileHandle)
{
#if USE_CRT_VER
	return _findclose((intptr_t)FindFileHandle) == 0;
#elif USE_NT_VER
	return Nt_FindClose(FindFileHandle);
#else
	return FindClose(FindFileHandle);
#endif
}

BOOL IsCircleSymbolicLink(ENUM_DIRECTORY_INFO *pFindInfo, LPWSTR lpPath, LPWSTR lpFileName)
{
	if (!FLAG_ON(pFindInfo->wfd.dwFileAttributes, FILE_ATTRIBUTE_REPARSE_POINT))
		return FALSE;

	ULONG_PTR   Length, CurrentPathLength;
	PWCHAR      ShorterPath, LinkedPath;
	HANDLE      SymbolicLink;
	NTSTATUS    Status;

	StrCopyW(lpFileName, pFindInfo->wfd.cFileName);
	if (!NT_SUCCESS(NtFileDisk::OpenSymbolicLink(&SymbolicLink, lpPath)))
		return FALSE;

	LinkedPath = pFindInfo->SymbolicLinkPath;
	Length = countof(pFindInfo->SymbolicLinkPath);
	Status = NtFileDisk::QuerySymbolicTarget(SymbolicLink, LinkedPath, &Length);

	NtFileDisk::Close(SymbolicLink);
	if (!NT_SUCCESS(Status))
		return FALSE;

	if (LinkedPath[Length - 1] != '\\')
		LinkedPath[Length++] = '\\';

	CurrentPathLength = StrLengthW(lpPath);
	if (CurrentPathLength < Length)
	{
		ShorterPath = lpPath;
		Length = CurrentPathLength;
	}
	else
	{
		ShorterPath = LinkedPath;
	}

	if (lpPath[Length - 1] != '\\' || LinkedPath[Length - 1] != '\\')
		return FALSE;

	return !StrNICompareW(lpPath, LinkedPath, Length);
}

BOOL EnumDirectoryFilesWorker(LPWSTR lpPath, ENUM_DIRECTORY_INFO *pFindInfo)
{
	BOOL    bIsDirectory;
	LPWSTR  lpEnd, lpFileName;
	HANDLE  hFind;
	LONG    Result;

	lpEnd = lpPath + StrLengthW(lpPath);
	if (pFindInfo->wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		if (*(lpEnd - 1) != '\\')
			*lpEnd++ = '\\';

		lpFileName = lpEnd;
		StrCopyW(lpEnd, pFindInfo->pszFilter);
		bIsDirectory = TRUE;
	}
	else
	{
		lpFileName = findnamew(lpPath);
		bIsDirectory = FALSE;
	}

	hFind = FindFirstFileWorker(lpPath, &pFindInfo->wfd);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (FLAG_ON(pFindInfo->wfd.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY))
			{
				if (__IsParentDirectory(pFindInfo->wfd.cFileName))
					continue;

				if (IsCircleSymbolicLink(pFindInfo, lpPath, lpFileName))
					continue;

				if (FLAG_ON(pFindInfo->Flags, EDF_BEFORE))
				{
					Result = ProcessFindResult(pFindInfo, lpPath, lpFileName);
					if (Result == -1)
					{
						FindCloseWorker(hFind);
						return FALSE;
					}
				}

				if (!FLAG_ON(pFindInfo->Flags, EDF_SUBDIR))
					continue;

				if (!FLAG_ON(pFindInfo->Flags, EDF_BEFORE))
					StrCopyW(lpFileName, pFindInfo->wfd.cFileName);

				if (!EnumDirectoryFilesWorker(lpPath, pFindInfo))
					break;
			}
			else
			{
				Result = ProcessFindResult(pFindInfo, lpPath, lpFileName);
				if (Result == -1)
				{
					FindCloseWorker(hFind);
					return FALSE;
				}
			}
		} while (FindNextFileWorker(hFind, &pFindInfo->wfd));

		FindCloseWorker(hFind);
	}

	if (bIsDirectory && FLAG_ON(pFindInfo->Flags, EDF_AFTER))
	{
		pFindInfo->wfd.cFileName[0] = 0;
		SET_FLAG(pFindInfo->wfd.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY);
		Result = ProcessFindResult(pFindInfo, lpPath, lpFileName);
		if (Result == -1)
			return FALSE;
	}

	*lpEnd = 0;

	return TRUE;
}

BOOL EnumDirectoryFilesFree(PVOID lpBuffer)
{
	return MemoryAllocator::FreeMemory(lpBuffer, 0);
}

BOOL
EnumDirectoryFiles(
	PVOID                      *lpFilesBuffer,
	LPCWSTR                     pszFilter,
	ULONG                       ElemSize,
	LPCWSTR                     pszPath,
	PLARGE_INTEGER              pElemCount,
	FEnumDirectoryFilesCallBack CallBack,
	ULONG_PTR                   Context,
	ULONG                       Flags
)
{
	WCHAR szPath[MAX_NTPATH];
	ENUM_DIRECTORY_INFO info;
	HANDLE hFind;

	if (CallBack == NULL)
		return FALSE;

	if (pElemCount != NULL)
		pElemCount->QuadPart = 0;

	if (pszPath != NULL)
	{
		hFind = FindFirstFileWorker(pszPath, &info.wfd);
		if (hFind == INVALID_HANDLE_VALUE)
			return FALSE;

		FindCloseWorker(hFind);
	}
	else
	{
		info.wfd.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
	}

	info.ElemSize = ElemSize;
	info.MaxCount.QuadPart = 100;
	info.hHeap = MemoryAllocator::GetGlobalHeap();
	if (lpFilesBuffer != NULL)
	{
#if USE_NT_VER
		info.lpBuffer = RtlAllocateHeap(info.hHeap, HEAP_ZERO_MEMORY, (ULONG_PTR)info.MaxCount.QuadPart * info.ElemSize);
#else
		info.lpBuffer = HeapAlloc(info.hHeap, HEAP_ZERO_MEMORY, (ULONG_PTR)info.MaxCount.QuadPart * info.ElemSize);
#endif // USE_NT_VER
		if (info.lpBuffer == NULL)
			return FALSE;
	}
	else
	{
		info.lpBuffer = NULL;
	}

	info.lpOutBuffer = info.lpBuffer;
	info.CallBack = CallBack;
	info.Context = Context;
	info.ElemCount.QuadPart = 0;
	info.pszFilter = pszFilter;
	info.Flags = Flags;

	if (pszPath != NULL)
	{
		StrCopyW(szPath, pszPath);
	}
	else
	{
		*(PULONG)szPath = L'.';
	}

	if (!EnumDirectoryFilesWorker(szPath, &info))
	{
		EnumDirectoryFilesFree(info.lpBuffer);
		return FALSE;
	}

	if (pElemCount != NULL)
		*pElemCount = info.ElemCount;
	if (lpFilesBuffer != NULL)
		*lpFilesBuffer = info.lpBuffer;

	return TRUE;
}

/************************************************************************
command line
************************************************************************/

LONG_PTR FASTCALL CmdLineToArgvWorkerA(LPSTR pszCmdLine, LPSTR *pArgv, PLONG_PTR pCmdLineLength)
{
	LONG_PTR  argc;
	Char separator, ch;
	LPSTR pCmdLine;

	argc = 0;
	pCmdLine = pszCmdLine;

	LOOP_ALWAYS
	{
		separator = *pszCmdLine++;
		if (separator == 0)
			break;
		if (separator != '"')
			separator = ' ';

		++argc;
		if (pArgv)
			*pArgv++ = (LPSTR)pszCmdLine - 1 + (separator == '"');

		LOOP_ALWAYS
		{
			ch = *pszCmdLine;
			if (ch == separator)
			{
				if (pArgv)
					*pszCmdLine = 0;
				++pszCmdLine;
				break;
			}
			else if (ch == 0)
				break;

			++pszCmdLine;
		}
			/*
			if (pArgv)
			*(pszCmdLine - 1)= 0;
			*/
			while (*pszCmdLine == '\t' || *pszCmdLine == ' ')
				++pszCmdLine;
	}

		if (pCmdLineLength)
			*pCmdLineLength = pszCmdLine - pCmdLine + 0;

	return argc;
}

LPSTR* FASTCALL CmdLineToArgvA(LPSTR pszCmdLine, PLONG_PTR pArgc)
{
	LONG_PTR argc, CmdLineLength;
	LPSTR *argv;

	argc = CmdLineToArgvWorkerA(pszCmdLine, NULL, &CmdLineLength);
	argv = (LPSTR *)AllocateMemory(CmdLineLength * sizeof(*pszCmdLine) + (argc + 1) * 4);
	if (argv == NULL)
		return NULL;

	CopyMemory(&argv[argc + 1], pszCmdLine, CmdLineLength * sizeof(*pszCmdLine));
	argv[argc] = 0;
	pszCmdLine = (LPSTR)&argv[argc + 1];

	argc = CmdLineToArgvWorkerA(pszCmdLine, argv, NULL);
	if (pArgc)
		*pArgc = argc;

	return (LPSTR *)argv;
}

LONG_PTR FASTCALL CmdLineToArgvWorkerW(LPWSTR pszCmdLine, LPWSTR *pArgv, PLONG_PTR pCmdLineLength)
{
	LONG_PTR  argc;
	WCHAR separator, ch;
	LPWSTR pCmdLine;

	argc = 0;
	pCmdLine = pszCmdLine;

	LOOP_ALWAYS
	{
		separator = *pszCmdLine++;
		if (separator == 0)
			break;
		if (separator != '"')
			separator = ' ';

		++argc;
		if (pArgv)
			*pArgv++ = (LPWSTR)pszCmdLine - 1 + (separator == '"');

		LOOP_ALWAYS
		{
			ch = *pszCmdLine;
			if (ch == separator)
			{
				if (pArgv)
					*pszCmdLine = 0;
				++pszCmdLine;
				break;
			}
			else if (ch == 0)
				break;

			++pszCmdLine;
		}
			/*
			if (pArgv && ch != 0)
			*(pszCmdLine - 1)= 0;
			*/
			while (*pszCmdLine == '\t' || *pszCmdLine == ' ')
				++pszCmdLine;
	}

		if (pCmdLineLength)
			*pCmdLineLength = pszCmdLine - pCmdLine + 0;

	return argc;
}

LPWSTR* FASTCALL CmdLineToArgvW(LPWSTR pszCmdLine, PLONG_PTR pArgc)
{
	LONG_PTR argc, CmdLineLength;
	LPWSTR *argv;

	argc = CmdLineToArgvWorkerW(pszCmdLine, NULL, &CmdLineLength);
	argv = (LPWSTR *)AllocateMemory(CmdLineLength * sizeof(*pszCmdLine) + (argc + 1) * 4);
	if (argv == NULL)
		return NULL;

	CopyMemory(&argv[argc + 1], pszCmdLine, CmdLineLength * sizeof(*pszCmdLine));
	argv[argc] = 0;
	pszCmdLine = (LPWSTR)&argv[argc + 1];

	argc = CmdLineToArgvWorkerW(pszCmdLine, argv, NULL);
	if (pArgc)
		*pArgc = argc;

	return (LPWSTR *)argv;
}

#endif  // ML_KERNEL_MODE



PChar FASTCALL findexta(PCChar lpFileName)
{
	Char ch;
	PChar pDot, pSlash, pszFileName;

	pDot = 0;
	pSlash = 0;
	pszFileName = (PChar)lpFileName;
	LOOP_ALWAYS
	{
		ch = *pszFileName++;
		if (ch == 0)
			break;
		if (ch < 0)
		{
			if (*pszFileName == 0)
				break;
			++pszFileName;
		}
		else if (ch == '/' || ch == '\\')
			pSlash = pszFileName - 1;
		else if (ch == '.')
			pDot = pszFileName - 1;
	}

		if (pDot == NULL)
			pDot = pszFileName - 1;
		else if (pDot < pSlash)
			pDot = pszFileName - 1;

	return pDot;
}

PWSTR FASTCALL findextw(PCWChar lpFileName)
{
	WChar ch;
	PWSTR pDot, pSlash, pszFileName;

	pDot = 0;
	pSlash = 0;
	pszFileName = (PWSTR)lpFileName;
	LOOP_ALWAYS
	{
		ch = *pszFileName++;
		if (ch == 0)
			break;
		if (ch == '/' || ch == '\\')
			pSlash = pszFileName - 1;
		else if (ch == '.')
			pDot = pszFileName - 1;
	}

		if (pDot == NULL)
			pDot = pszFileName - 1;
		else if (pDot < pSlash)
			pDot = pszFileName - 1;

	return pDot;
}

PChar FASTCALL findnamea(PCChar lpFileName)
{
	Char ch;
	PChar pBackSlash, pSlash, pszFileName;

	pBackSlash = 0;
	pSlash = 0;
	pszFileName = (PChar)lpFileName;
	LOOP_ALWAYS
	{
		ch = *pszFileName++;
		if (ch == 0)
			break;
		if (ch < 0)
		{
			if (*pszFileName == 0)
				break;
			++pszFileName;
		}
		else if (ch == '/')
			pSlash = pszFileName - 1;
		else if (ch == '\\')
			pBackSlash = pszFileName - 1;
	}

	pBackSlash = MY_MAX(pBackSlash, pSlash);
	if (pBackSlash == 0)
		pBackSlash = (PChar)lpFileName;
	else
		++pBackSlash;

	return pBackSlash;
}

PWSTR FASTCALL findnamew(PCWChar lpFileName, SizeT Length /* = -1 */)
{
	WChar ch;
	PWSTR pBackSlash, pSlash, pszFileName;

	if (lpFileName == NULL)
		return NULL;

	pBackSlash = 0;
	pSlash = 0;
	pszFileName = (PWSTR)lpFileName;

	for (; Length != 0; --Length)
	{
		ch = *pszFileName++;
		if (ch == 0)
			break;
		if (ch == '/')
			pSlash = pszFileName - 1;
		else if (ch == '\\')
			pBackSlash = pszFileName - 1;
	}

	pBackSlash = MY_MAX(pBackSlash, pSlash);
	if (pBackSlash == 0)
		pBackSlash = (PWSTR)lpFileName;
	else
		++pBackSlash;

	return pBackSlash;
}

PChar StringLowerA(PChar pszString, ULong Length)
{
	PChar pszText;
	for (pszText = pszString; *pszText && Length != 0; ++pszText, --Length)
	{
		Char ch = *pszText;
		if (ch < 0)
		{
			if (*++pszText == 0 || --Length == 0)
				break;

			continue;
		}

		if (IN_RANGE('A', ch, 'Z'))
			*pszText = ch | 0x20;
	}

	return pszString;
}

PWChar StringLowerW(PWChar String, ULong Length)
{
	PWChar Text;

	if (Length == -1)
		Length = StrLengthW(String);

	for (Text = String; Length != 0; ++Text, --Length)
	{
		*Text = CHAR_LOWER(*Text);
	}

	return String;
}

PChar StringUpperA(PChar pszString, ULong Length)
{
	PChar pszText;
	for (pszText = pszString; *pszText && Length != 0; ++pszText, --Length)
	{
		Char ch = *pszText;
		if (ch < 0)
		{
			if (*++pszText == 0 || --Length == 0)
				break;

			continue;
		}

		if (IN_RANGE('a', ch, 'z'))
			*pszText = ch & 0xDF;
	}

	return pszString;
}

PWChar StringUpperW(PWChar String, ULong Length)
{
	PWChar Text;

	if (Length == -1)
		Length = StrLengthW(String);

	for (Text = String; Length != 0; ++Text, --Length)
	{
		*Text = CHAR_UPPER(*Text);
	}

	return String;
}

Long_Ptr FASTCALL StrLengthA(PCChar pString)
{
#if 0

	PCChar s;
	ULong quad, v1, flag;

	if (pString == NULL)
		return 0;

	s = pString;
	while ((Long_Ptr)(s) & 3)
	{
		if (*s++ == 0)
		{
			--s;
			return s - pString;
		}
	}

	do
	{
		quad = *(PULong)s;
		s += sizeof(quad);

		quad = ((quad + 0xFEFEFEFF) & ~quad) & 0x80808080;

	} while (quad == 0);

	v1 = quad >> 16;
	flag = quad & 0x8080;

	quad = flag ? quad : (v1);
	s += flag ? 0 : 2;

	s -= 3 + ((quad >> 7) & 1);

	return s - pString;

#else

	Long ch;
	Long_Ptr SizeOfUnit;
	PCChar pBuffer = pString;

	if (pString == NULL)
		return 0;

	SizeOfUnit = sizeof(Int32);
	while ((Long_Ptr)(pBuffer) & 3)
	{
		if (*pBuffer++ == 0)
		{
			--pBuffer;
			goto end_of_calc;
		}
	}

	LOOP_ALWAYS
	{
		Long temp;
		ch = *(PLong)pBuffer;
		pBuffer += SizeOfUnit;

		temp = (0x7EFEFEFF + ch) ^ (ch ^ -1);
		if ((temp & 0x81010100) == 0)
			continue;

		if (LoByte(ch) == 0)
		{
			pBuffer -= SizeOfUnit;
			break;
		}

		if ((ch & 0xFF00) == 0)
		{
			pBuffer -= SizeOfUnit - 1;
			break;
		}

		if ((ch & 0x00FF0000) == 0)
		{
			pBuffer -= SizeOfUnit - 2;
			break;
		}

		if ((ch & 0xFF000000) == 0)
		{
			pBuffer -= SizeOfUnit - 3;
			break;
		}
	}

	end_of_calc:
	return pBuffer - pString;

#endif
}

Long_Ptr FASTCALL StrLengthW(PCWChar pString)
{
	Long ch;
	PCWChar pBuffer;

	if (pString == NULL)
		return 0;

	pBuffer = pString;
	while ((Int_Ptr)pBuffer & 3)
	{
		if (*pBuffer++ == 0)
		{
			--pBuffer;
			goto end_of_strlen;
		}
	}

	LOOP_ALWAYS
	{
		ch = *(PInt)pBuffer;
		if ((ch & 0xFFFF) == 0)
		{
			break;
		}
		else if ((ch & 0xFFFF0000) == 0)
		{
			pBuffer = (PCWChar)((PByte)pBuffer + sizeof(*pBuffer));
			break;
		}

		pBuffer = (PCWChar)((PByte)pBuffer + sizeof(ch));
	}

	end_of_strlen:

	return pBuffer - pString;
}

PChar FASTCALL StrCopyA(PChar DestinationString, PCChar SourceString)
{
	UInt32 ch, SizeOfUnit;
	PChar Buffer = DestinationString;

	if (Buffer == NULL || SourceString == NULL)
		return DestinationString;

	SizeOfUnit = sizeof(*(PInt)Buffer);
	while ((ULong_Ptr)(SourceString) & 3)
	{
		if ((*Buffer++ = *SourceString++) == 0)
			return DestinationString;
	}

	LOOP_ALWAYS
	{
		UInt32 temp;
		ch = *(PInt)SourceString;

		temp = (0x7EFEFEFF + ch) ^ (ch ^ -1);
		if ((temp & 0x81010100) == 0)
		{
			*(PULong)Buffer = ch;
			Buffer += SizeOfUnit;
			SourceString += SizeOfUnit;
			continue;
		}

		if (LoByte(ch) == 0)
		{
			*Buffer = (Char)ch;
			break;
		}

		if ((ch & 0xFF00) == 0)
		{
			*(PUInt16)Buffer = (UInt16)ch;
			break;
		}

		if ((ch & 0x00FF0000) == 0)
		{
			*(PUInt16)Buffer = (UInt16)ch;
			Buffer[2] = LoByte(HiWord(ch));
			break;
		}

		*(PULong)Buffer = ch;

		if ((ch & 0xFF000000) == 0)
			break;

		Buffer += SizeOfUnit;
		SourceString += SizeOfUnit;
	}

	return DestinationString;
}

PWChar FASTCALL StrCopyW(PWChar DestinationString, PCWChar SourceString)
{
	PWChar Destination;
	ULong  ch;

	if (DestinationString == NULL || SourceString == NULL)
		return DestinationString;

	Destination = DestinationString;

	while ((ULong_Ptr)SourceString & 3)
	{
		if ((*DestinationString++ = *SourceString++) == 0)
			return Destination;
	}

	do
	{
		ch = *(PULong)SourceString;

		if (TEST_BITS(ch, 0x0000FFFF) == 0)
		{
			*DestinationString = (WChar)ch;
			break;
		}

		*(PULong)DestinationString = ch;
		DestinationString += 2;
		SourceString += 2;
	} while (TEST_BITS(ch, 0xFFFF0000) != 0);

	return Destination;
}

PChar FASTCALL StrConcatenateA(PChar DestinationString, PCChar SourceString)
{
	return StrCopyA(DestinationString + StrLengthA(DestinationString), SourceString);
}

PWChar FASTCALL StrConcatenateW(PWChar DestinationString, PCWChar SourceString)
{
	return StrCopyW(DestinationString + StrLengthW(DestinationString), SourceString);
}

Long_Ptr FASTCALL StrNICompareA(PCChar pString1, PCChar pString2, SizeT LengthToCompare, LongPtr CaseConvertTo)
{
	Long ch1, ch2, temp;

	if (LengthToCompare == 0)
		return 0;

	if (pString1 == NULL || pString2 == NULL)
		return (Long_Ptr)pString1 - (Long_Ptr)pString2;

	do
	{
		ch1 = *pString1;
		ch2 = *pString2;
		temp = ch1 ^ ch2;
		if ((ch1 | ch2) < 0)
		{
			if (temp != 0)
				break;

			if (--LengthToCompare == 0)
				break;

			ch1 = *++pString1;
			ch2 = *++pString2;
			if (ch1 != ch2)
				break;
		}
		else
		{
			if (TEST_BITS(temp, 0xDF))
				break;
		}

		if (ch1 == 0)
			break;

		++pString1;
		++pString2;

	} while (--LengthToCompare != 0);

	switch (CaseConvertTo)
	{
	case StrCmp_Keep:
		break;

	case StrCmp_ToLower:
		ch1 = CHAR_LOWER(ch1);
		ch2 = CHAR_LOWER(ch2);
		break;

	case StrCmp_ToUpper:
		ch1 = CHAR_UPPER(ch1);
		ch2 = CHAR_UPPER(ch2);
		break;
	}

	return LengthToCompare == 0 ? 0 : (ULong)ch1 < (ULong)ch2 ? -1 : (ULong)ch1 >(ULong)ch2 ? 1 : 0;
}

Long_Ptr FASTCALL StrNICompareW(PCWChar pString1, PCWChar pString2, SizeT LengthToCompare, LongPtr CaseConvertTo)
{
	ULong ch1, ch2;

	if (LengthToCompare == 0)
		return 0;

	if (pString1 == NULL || pString2 == NULL)
		return 1;

	do
	{
		ch1 = *(PInt16)pString1;
		ch2 = *(PInt16)pString2;

		if ((ch1 | ch2) & 0xFF80)
		{
			if (ch1 != ch2)
				break;
		}
		else if (((ch1 ^ ch2) & 0xFFDF) != 0)
		{
			break;
		}

		++pString1;
		++pString2;
	} while (--LengthToCompare != 0 && ch1);

	switch (CaseConvertTo)
	{
	case StrCmp_Keep:
		break;

	case StrCmp_ToLower:
		ch1 = CHAR_LOWER(ch1);
		ch2 = CHAR_LOWER(ch2);
		break;

	case StrCmp_ToUpper:
		ch1 = CHAR_UPPER(ch1);
		ch2 = CHAR_UPPER(ch2);
		break;
	}

	// return LengthToCompare == 0 ? 0 : (Long_Ptr)(Int16)(ch1 - ch2);
	return LengthToCompare == 0 ? 0 : (ULong)ch1 < (ULong)ch2 ? -1 : (ULong)ch1 >(ULong)ch2 ? 1 : 0;
}

Long_Ptr FASTCALL StrNCompareA(PCChar pString1, PCChar pString2, SizeT LengthToCompare)
{
	Long ch1, ch2, xorr;

	if (LengthToCompare == 0)
		return 0;

	if (pString1 == NULL || pString2 == NULL)
		return pString1 - pString2;

	while (((Long_Ptr)pString1 & (Long_Ptr)pString2) & 3)
	{
		ch1 = *pString1;
		ch2 = *pString2;
		if (ch1 ^ ch2)
			return (Long_Ptr)(Char)(ch1 - ch2);

		*pString1++;
		*pString2++;
		--LengthToCompare;
	}

	ch1 = 0;
	ch2 = 0;
	xorr = 0;
	if (LengthToCompare > 3) do
	{
		ch1 = *(PLong)pString1;
		ch2 = *(PLong)pString2;

		xorr = ch1 ^ ch2;
		if (TEST_BITS(ch1, 0xFF) == 0)
		{
			ch1 &= 0xFF;
			ch2 &= 0xFF;
			xorr &= 0xFF;
			LengthToCompare = 0;
			break;
		}
		else if (TEST_BITS(ch1, 0xFF00) == 0)
		{
			ch1 &= 0xFFFF;
			ch2 &= 0xFFFF;
			xorr &= 0xFFFF;
			LengthToCompare = 0;
			break;
		}
		else if (TEST_BITS(ch1, 0xFF0000) == 0)
		{
			ch1 &= 0xFFFFFF;
			ch2 &= 0xFFFFFF;
			xorr &= 0xFFFFFF;
			LengthToCompare = 0;
			break;
		}
		else if (TEST_BITS(ch1, 0xFF000000) == 0)
		{
			LengthToCompare = 0;
			break;
		}

		if (xorr)
		{
			return ch1 < ch2 ? -1 : 1;
		}

		pString1 = (PCChar)((PByte)pString1 + sizeof(ch1));
		pString2 = (PCChar)((PByte)pString2 + sizeof(ch2));

		LengthToCompare -= 4;
	} while (LengthToCompare >= 4);

	while (LengthToCompare--)
	{
		ch1 = *pString1++;
		ch2 = *pString2++;
		xorr = ch1 ^ ch2;
		if (xorr != 0)
			break;
	}

	if (xorr == 0)
		return 0;

	return ch1 < ch2 ? -1 : 1;
}

Long_Ptr FASTCALL StrNCompareW(PCWChar pString1, PCWChar pString2, SizeT LengthToCompare)
{
	Long ch1, ch2;
	//    WChar ch1, ch2;

	if (LengthToCompare == 0)
		return 0;

	if (pString1 == NULL || pString2 == NULL)
		return pString1 - pString2;

	while (((Long_Ptr)pString1 & (Long_Ptr)pString2) & 3)
	{
		ch1 = *pString1;
		ch2 = *pString2;
		if (ch1 != ch2)
		{
			ch1 = (Long)(Int16)(ch1 - ch2);
			return ch1 < 0 ? -1 : ch1 > 0 ? 1 : 0;
			//            return (Int)(Int16)(ch1 - ch2);
		}

		if (ch1 == 0)
			return 0;

		*pString1++;
		*pString2++;
	}

	do
	{
		ch1 = *(PInt32)pString1;
		ch2 = *(PInt32)pString2;
		if (LoWord(ch1) != LoWord(ch2))
		{
			ch1 = (Int)(Int16)((ch1 - ch2) & 0xFFFF);
			return ch1 < 0 ? -1 : ch1 > 0 ? 1 : 0;
			//            return (Int)(Int16)(ch1 - ch2);
		}

		if (LoWord(ch1) == 0)
			return 0;

		ch1 >>= 16;
		ch2 >>= 16;
		if (ch1 != ch2)
		{
			ch1 = (Int)(Int16)(ch1 - ch2);
			return ch1 < 0 ? -1 : ch1 > 0 ? 1 : 0;
			//            return (Int)(Int16)(ch1 - ch2);
		}

		if (ch1 == 0)
			return 0;

		pString1 = (PCWChar)((PByte)pString1 + sizeof(ch1));
		pString2 = (PCWChar)((PByte)pString2 + sizeof(ch2));

	} while (--LengthToCompare != 0);

	return 0;
}

PWSTR StrFindCharW(PCWChar lpString, WChar CharMatch)
{
	WChar ch;

	if (lpString == NULL)
		return NULL;

	if (CharMatch == 0)
		return (PWSTR)lpString + StrLengthW(lpString);

	do
	{
		ch = *lpString++;

	} while (ch && ch != CharMatch);

	return ch ? (PWSTR)lpString - 1 : NULL;
}


PWSTR StrFindLastCharW(PCWChar lpString, WChar CharMatch)
{
	WChar ch;
	ULONG_PTR iPos, CurPos;
	ULONG_PTR Length = StrLengthW(lpString);

	if (lpString == NULL)
		return NULL;

	if (CharMatch == 0)
		return (PWSTR)lpString + Length;

	iPos = 0;
	CurPos = 0;

	do
	{
		ch = lpString[iPos];
		if (ch == CharMatch)
			CurPos = iPos;

		iPos++;

	} while (ch);

	return CurPos ? (PWSTR)lpString + CurPos : NULL;
}


PWSTR PeekLineW(PVoid pBuffer, PVoid pEndOfBuffer, PVoid pOutBuffer, PSizeT pcbOutBuffer)
{
	WChar  ch;
	PWSTR pStart, pEnd, pOutput;
	SSizeT BufferSize;
	SizeT  cbOutSize;

	if (pBuffer == NULL)
		return NULL;

	if (pcbOutBuffer == NULL)
		return NULL;

	if (pcbOutBuffer != NULL)
	{
		BufferSize = *pcbOutBuffer;
		*pcbOutBuffer = 0;
	}
	else
	{
		BufferSize = 0;
	}

	pStart = (PWSTR)pBuffer;
	pEnd = pEndOfBuffer != NULL ? (PWSTR)pEndOfBuffer : pStart + StrLengthW(pStart);

	if (pStart >= pEnd)
		return NULL;

	pOutput = (PWSTR)pOutBuffer;
	if (*pStart == BOM_UTF16_LE)
		++pStart;

	cbOutSize = 0;
	while (pStart < pEnd)
	{
		ch = *pStart++;
		if (ch == '\n')
			break;

		if (BufferSize > 0 && ch != '\r')
		{
			if (pOutput != NULL)
				*pOutput++ = ch;

			cbOutSize += sizeof(ch);
			BufferSize -= sizeof(ch);
		}
	}

	if (pOutput != NULL && BufferSize > 0)
		*pOutput = 0;

	*pcbOutBuffer = cbOutSize;

	if (cbOutSize == 0)
		return pStart < pEnd ? pStart : NULL;

	return pStart;
}

#define DECL_NUMBER_TABLE32(name) \
        UInt32 name[] = \
        { \
            0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, \
            0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, \
            0x03020100, 0x07060504, 0xFFFF0908, 0xFFFFFFFF, 0x0C0B0AFF, 0xFF0F0E0D, \
        }

Int32 StringToInt32A(PCChar pszString)
{
	/*
	Char NumberTable[] =
	{
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,  0,  1,
	2,  3,  4,  5,  6,  7,  8,  9, -1, -1,
	-1, -1, -1, -1, -1, 10, 11, 12, 13, 14,
	15,
	};
	*/
	if (pszString == NULL)
		return 0;

	DECL_NUMBER_TABLE32(NumberTable32);
	PChar NumberTable = (PChar)NumberTable32;

	Int32 Result, Temp, Sign;
	Char ch;

	if (*pszString == '-')
	{
		Sign = 1;
		++pszString;
	}
	else
	{
		Sign = 0;
	}

	Result = 0;
	LOOP_ALWAYS
	{
		ch = *pszString++;
		if (ch == 0)
			break;
		if (IN_RANGE('a', ch, 'z'))
			ch &= 0xDF;
		if ((UInt32)ch > sizeof(NumberTable32))
			return 0;

		Temp = NumberTable[ch];
		if ((Char)Temp == (Char)-1)
			break;

		Result = Result * 10 + Temp;
	}

		if (Sign)
			Result = -Result;

	return Result;
}

Int32 StringToInt32W(PCWChar pszString)
{
	if (pszString == NULL)
		return 0;

	DECL_NUMBER_TABLE32(NumberTable32);
	PChar NumberTable = (PChar)NumberTable32;

	Int32 Result, Temp, Sign;
	WChar ch;

	if (*pszString == '-')
	{
		Sign = 1;
		++pszString;
	}
	else
	{
		Sign = 0;
	}

	Result = 0;
	LOOP_ALWAYS
	{
		ch = *pszString++;
		if (ch == 0)
			break;
		if (IN_RANGE('a', ch, 'z'))
			ch &= 0xFFDF;
		if ((UInt32)ch > sizeof(NumberTable32))
			break;

		Temp = NumberTable[ch];
		if ((Char)Temp == (Char)-1)
			break;

		Result = Result * 10 + Temp;
	}

		if (Sign)
			Result = -Result;

	return Result;
}

Int32 StringToInt32HexW(PCWSTR pszString)
{
	Int32 Result;
	WChar ch;

	Result = 0;
	LOOP_ALWAYS
	{
		ch = *pszString++;
		if (ch == 0)
			break;
		if (IN_RANGE('0', ch, '9'))
		{
			ch -= '0';
		}
		else
		{
			ch &= 0xFFDF;
			if (IN_RANGE('A', ch, 'F'))
				ch = ch - 'A' + 10;
			else
				break;
		}

		Result = (Result << 4) | ch;
	}

	return Result;
}

Int64 StringToInt64HexW(PCWSTR pszString)
{
	Int64 Result;
	WChar ch;

	Result = 0;
	LOOP_ALWAYS
	{
		ch = *pszString++;
		if (ch == 0)
			break;
		if (IN_RANGE('0', ch, '9'))
		{
			ch -= '0';
		}
		else
		{
			ch &= 0xFFDF;
			if (IN_RANGE('A', ch, 'F'))
				ch = ch - 'A' + 10;
			else
				break;
		}

		Result = (Result << 4) | ch;
	}

	return Result;
}

Int64 StringToInt64A(PChar pszString)
{
	if (pszString == NULL)
		return 0;

	DECL_NUMBER_TABLE32(NumberTable32);
	PChar NumberTable = (PChar)NumberTable32;

	Int64 Result, Temp, Sign;
	Char ch;

	if (*pszString == '-')
	{
		Sign = 1;
		++pszString;
	}
	else
	{
		Sign = 0;
	}

	Result = 0;
	LOOP_ALWAYS
	{
		ch = *pszString++;
		if (ch == 0)
			break;
		if (IN_RANGE('a', ch, 'z'))
			ch &= 0xDF;
		if ((UInt32)ch > sizeof(NumberTable32))
			return 0;

		Temp = NumberTable[ch];
		if ((Char)Temp == (Char)-1)
			break;

		Result = Result * 10 + Temp;
	}

		if (Sign)
			Result = -Result;

	return Result;
}

Int64 StringToInt64W(PCWChar pszString)
{
	if (pszString == NULL)
		return 0;

	DECL_NUMBER_TABLE32(NumberTable32);
	PChar NumberTable = (PChar)NumberTable32;

	Int64 Result, Temp, Sign;
	WChar ch;

	if (*pszString == '-')
	{
		Sign = 1;
		++pszString;
	}
	else
	{
		Sign = 0;
	}

	Result = 0;
	LOOP_ALWAYS
	{
		ch = *pszString++;
		if (ch == 0)
			break;
		if (IN_RANGE('a', ch, 'z'))
			ch &= 0xFFDF;
		if ((UInt32)ch > sizeof(NumberTable32))
			break;

		Temp = NumberTable[ch];
		if ((Char)Temp == (Char)-1)
			break;

		Result = Result * 10 + Temp;
	}

	if (Sign)
		Result = -Result;

	return Result;
}

Bool IsShiftJISChar(SizeT uChar)
{
	Byte LowByte, HighByte;

	LowByte = LOBYTE(uChar);
	HighByte = HIBYTE(uChar);
	if ((HighByte >= 0x80 && HighByte <= 0x9F) ||
		(HighByte >= 0xE0 && HighByte <= 0xFC))
	{
		if ((LowByte >= 0x40 && LowByte <= 0x7E) ||
			(LowByte >= 0x80 && LowByte <= 0xFC))
		{
			return True;
		}
	}

	return False;
}

Bool IsShiftJISString(PCChar pString, SizeT Length)
{
	Bool   bMBChar;
	PCChar p;

	if (pString == NULL)
		return False;

	bMBChar = False;
	p = pString;
	for (; Length; ++p, --Length)
	{
		Char  c;
		Int32 ch;

		c = *p;
		CONTINUE_IF(c > 0);
		BREAK_IF(c == 0 || --Length == 0);

		ch = *(PUInt16)p++;
		if (!IsShiftJISChar(SWAPCHAR(ch)))
			return False;
		else
			bMBChar = True;
	}

	return bMBChar;
}

Bool
StrMatchExpression(
	IN PWChar   Expression,
	IN PWChar   Name,
	IN Bool     IgnoreCase,
	IN PWChar   UpcaseTable OPTIONAL
)
{
	return Rtl::IsNameInExpression(Expression, Name, IgnoreCase, UpcaseTable);
}

/***********************************************************************
*           WPRINTF_ParseFormatA
*
* Parse a format specification. A format specification has the form:
*
* [-][#][0][width][.precision]type
*
* Return value is the length of the format specification in characters.
***********************************************************************/
Int FormatString_ParseFormatA(PCChar format, FMTSTR_FORMAT *res)
{
	PCChar p = format;

	res->flags = 0;
	res->width = 0;
	res->precision = 0;
	if (*p == 'I' && p[1] == '6' && p[2] == '4') { res->flags |= FMTSTR_FLAG_I64; p += 3; }
	if (*p == '-') { res->flags |= FMTSTR_FLAG_LEFTALIGN; p++; }
	if (*p == '#') { res->flags |= FMTSTR_FLAG_PREFIX_HEX; p++; }
	if (*p == '0') { res->flags |= FMTSTR_FLAG_ZEROPAD; p++; }
	while ((*p >= '0') && (*p <= '9'))  /* width field */
	{
		res->width = res->width * 10 + *p - '0';
		p++;
	}
	if (*p == '.')  /* precision field */
	{
		p++;
		while ((*p >= '0') && (*p <= '9'))
		{
			res->precision = res->precision * 10 + *p - '0';
			p++;
		}
	}
	if (*p == 'l') { res->flags |= FMTSTR_FLAG_LONG; p++; }
	else if (*p == 'h') { res->flags |= FMTSTR_FLAG_SHORT; p++; }
	else if (*p == 'w') { res->flags |= FMTSTR_FLAG_WIDE; p++; }
	switch (*p)
	{
	case 'c':
		res->type = (res->flags & FMTSTR_FLAG_LONG) ? FMTSTR_FORMAT_WCHAR : FMTSTR_FORMAT_CHAR;
		break;

	case 'C':
		res->type = (res->flags & FMTSTR_FLAG_SHORT) ? FMTSTR_FORMAT_CHAR : FMTSTR_FORMAT_WCHAR;
		break;

	case 'u':
		if (!TEST_BITS(res->flags, FMTSTR_FLAG_FORCESIGNED))
		{
			res->type = FMTSTR_FORMAT_UNSIGNED;
			break;
		}
		NO_BREAK;
	case 'd':
	case 'i':
		res->type = FMTSTR_FORMAT_SIGNED;
		break;

	case 's':
		res->type = (res->flags & (FMTSTR_FLAG_LONG | FMTSTR_FLAG_WIDE)) ? FMTSTR_FORMAT_WSTRING : FMTSTR_FORMAT_STRING;
		break;

	case 'S':
		res->type = (res->flags & (FMTSTR_FLAG_SHORT | FMTSTR_FLAG_WIDE)) ? FMTSTR_FORMAT_STRING : FMTSTR_FORMAT_WSTRING;
		break;

	case 'X':
		res->flags |= FMTSTR_FLAG_UPPER_HEX;
		/* fall through */
	case 'x':
		res->type = FMTSTR_FORMAT_HEXA;
		break;

#if MY_OS_WIN32
	case 'b':
	case 'B':
		res->type = FMTSTR_FORMAT_BINARY;
		break;
#endif

	default: /* unknown format char */
		res->type = FMTSTR_FORMAT_UNKNOWN;
		p--;  /* print format as normal char */
		break;
	}
	return (Int)(p - format) + 1;
}


/***********************************************************************
*           WPRINTF_ParseFormatW
*
* Parse a format specification. A format specification has the form:
*
* [-][#][0][width][.precision]type
*
* Return value is the length of the format specification in characters.
*/

Int FormatString_ParseFormatW(PCWChar format, FMTSTR_FORMAT *res)
{
	PCWChar p = format;

	res->flags = 0;
	res->width = 0;
	res->precision = 0;
	if (*p == 'I' && p[1] == '6' && p[2] == '4') { res->flags |= FMTSTR_FLAG_I64; p += 3; }
	if (*p == '+') { res->flags |= FMTSTR_FLAG_FORCESIGNED; p++; }
	if (*p == '-') { res->flags |= FMTSTR_FLAG_LEFTALIGN; p++; }
	if (*p == '#') { res->flags |= FMTSTR_FLAG_PREFIX_HEX; p++; }
	if (*p == '0') { res->flags |= FMTSTR_FLAG_ZEROPAD; p++; }
	while ((*p >= '0') && (*p <= '9'))  /* width field */
	{
		res->width = res->width * 10 + *p - '0';
		p++;
	}
	if (*p == '.')  /* precision field */
	{
		p++;
		while ((*p >= '0') && (*p <= '9'))
		{
			res->precision = res->precision * 10 + *p - '0';
			p++;
		}
	}
	if (*p == 'l') { res->flags |= FMTSTR_FLAG_LONG; p++; }
	else if (*p == 'h') { res->flags |= FMTSTR_FLAG_SHORT; p++; }
	else if (*p == 'w') { res->flags |= FMTSTR_FLAG_WIDE; p++; }
	switch ((Char)*p)
	{
	case 'c':
		res->type = (res->flags & FMTSTR_FLAG_SHORT) ? FMTSTR_FORMAT_CHAR : FMTSTR_FORMAT_WCHAR;
		break;

	case 'C':
		res->type = (res->flags & FMTSTR_FLAG_LONG) ? FMTSTR_FORMAT_WCHAR : FMTSTR_FORMAT_CHAR;
		break;

	case 'u':
		if (!TEST_BITS(res->flags, FMTSTR_FLAG_FORCESIGNED))
		{
			res->type = FMTSTR_FORMAT_UNSIGNED;
			break;
		}
		NO_BREAK;
	case 'd':
	case 'i':
		res->type = FMTSTR_FORMAT_SIGNED;
		break;

	case 's':
		res->type = ((res->flags & FMTSTR_FLAG_SHORT) && !(res->flags & FMTSTR_FLAG_WIDE)) ? FMTSTR_FORMAT_STRING : FMTSTR_FORMAT_WSTRING;
		break;

	case 'S':
		res->type = (res->flags & (FMTSTR_FLAG_LONG | FMTSTR_FLAG_WIDE)) ? FMTSTR_FORMAT_WSTRING : FMTSTR_FORMAT_STRING;
		break;

	case 'p':
		res->width = 8;
		res->flags |= FMTSTR_FLAG_ZEROPAD;
		/* fall through */
	case 'X':
		res->flags |= FMTSTR_FLAG_UPPER_HEX;
		/* fall through */
	case 'x':
		res->type = FMTSTR_FORMAT_HEXA;
		break;

#if MY_OS_WIN32
	case 'b':
	case 'B':
		res->type = FMTSTR_FORMAT_BINARY;
		break;
#endif

	default:
		res->type = FMTSTR_FORMAT_UNKNOWN;
		p--;  /* print format as normal char */
		break;
	}

	return (Int)(p - format) + 1;
}

inline
UInt
FormatString_GetLen(
	FMTSTR_FORMAT   *format,
	FMTSTR_DATA     *arg,
	PChar            number,
	UInt             maxlen
)
{
	UInt len = 0, UpperFlag;

	if (format->flags & FMTSTR_FLAG_LEFTALIGN)
		format->flags &= ~FMTSTR_FLAG_ZEROPAD;

	if (format->width > maxlen)
		format->width = maxlen;

	switch (format->type)
	{
	case FMTSTR_FORMAT_CHAR:
	case FMTSTR_FORMAT_WCHAR:
		return (format->precision = 1);

	case FMTSTR_FORMAT_STRING:
		if (arg->lpcstr_view)
		{
			len = StrLengthA(arg->lpcstr_view);
			if (format->precision != 0 && format->precision > len)
				len = format->precision;
			/*
			for (len = 0; format->precision == 0 || (len < format->precision); len++)
			{
			Char ch = arg->lpcstr_view[len];
			if (ch == 0)
			break;
			}
			*/
		}
		else
		{
			len = format->precision == NULL ? 6 : ML_MIN(6, format->precision);
		}

		if (len > maxlen)
			len = maxlen;

		return (format->precision = len);

	case FMTSTR_FORMAT_WSTRING:
		if (arg->lpcwstr_view)
		{
			len = StrLengthW(arg->lpcwstr_view);
			if (format->precision != 0 && format->precision > len)
				len = format->precision;
			/*
			for (len = 0; !format->precision || (len < format->precision); len++)
			if (arg->lpcwstr_view[len] == 0)
			break;
			*/
		}
		else
		{
			len = format->precision == NULL ? 6 : ML_MIN(6, format->precision);
		}

		if (len > maxlen)
			len = maxlen;

		return (format->precision = len);

	case FMTSTR_FORMAT_SIGNED:
		if (format->flags & FMTSTR_FLAG_I64)
		{
			if ((Int64)arg->int64_view < 0)
			{
				*number++ = '-';
				arg->int64_view = -(Int64)arg->int64_view;
				++len;
			}
		}
		else
		{
			if ((Int)arg->int_view < 0)
			{
				*number++ = '-';
				arg->int_view = -(Int)arg->int_view;
				++len;
			}
		}
		NO_BREAK;

	case FMTSTR_FORMAT_UNSIGNED:
		if (format->flags & FMTSTR_FLAG_I64)
		{
			if (arg->int64_view == 0)
			{
				*number++ = '0';
				++len;
				break;
			}

			for (UInt64 i = arg->int64_view; i; i /= 10)
			{
				++number;
			}
			for (UInt64 i = arg->int64_view; i; i /= 10)
			{
				UInt64 v = i % 10;
				*--number = (Char)(v + '0');
				++len;
			}
		}
		else
		{
			if (arg->int_view == 0)
			{
				*number++ = '0';
				++len;
				break;
			}

			for (UInt i = arg->int_view; i; i /= 10)
			{
				++number;
			}
			for (UInt i = arg->int_view; i; i /= 10)
			{
				UInt v = i % 10;
				*--number = (Char)(v + '0');
				++len;
			}
		}
		break;

	case FMTSTR_FORMAT_HEXA:
		UpperFlag = (format->flags & FMTSTR_FLAG_UPPER_HEX) ? 0xDF : 0xFF;

		if (format->flags & FMTSTR_FLAG_I64)
		{
			for (UInt64 i = arg->int64_view; i; i >>= 4)
				++number;

			for (UInt64 i = arg->int64_view; i; i >>= 4)
			{
				UInt64 v = i & 0xF;
				*--number = (Char)(v < 10 ? v + '0' : (v - 10 + ('a' & UpperFlag)));
				++len;
			}
		}
		else
		{
			for (UInt i = arg->int_view; i; i >>= 4)
				++number;

			for (UInt i = arg->int_view; i; i >>= 4)
			{
				UInt v = i & 0xF;
				*--number = (Char)(v < 10 ? v + '0' : (v - 10 + ('a' & UpperFlag)));
				++len;
			}
		}
		break;

#if MY_OS_WIN32

	case FMTSTR_FORMAT_BINARY:
	{
		ULong bit;
		Large_Integer val;

		val.QuadPart = arg->int64_view;
		if ((format->flags & FMTSTR_FLAG_I64) && val.HighPart != 0)
		{
			_BitScanReverse(&bit, val.HighPart);
			bit += 32;
		}
		else if (val.LowPart != 0)
		{
			_BitScanReverse(&bit, val.LowPart);
		}
		else
		{
			bit = 0;
		}

		number += bit;
		++bit;
		do
		{
			*number-- = (Char)(val.LowPart & 1) + '0';
			++len;
		} while (--bit);
	}
	break;

#endif

	default:
		return 0;
	}

	if (len > maxlen) len = maxlen;
	if (format->precision < len) format->precision = len;
	if (format->precision > maxlen) format->precision = maxlen;
	if ((format->flags & FMTSTR_FLAG_ZEROPAD) && (format->width > format->precision))
		format->precision = format->width;
	if (format->flags & FMTSTR_FLAG_PREFIX_HEX) len += 2;
	return len;
}

/***********************************************************************
*           wvnsprintfA   (SHLWAPI.@)
*
* Print formatted output to a string, up to a maximum number of chars.
*
* PARAMS
* buffer [O] Destination for output string
* maxlen [I] Maximum number of characters to write
* spec   [I] Format string
*
* RETURNS
*  Success: The number of characters written.
*  Failure: -1.
*/
Int FormatStringvnA(PChar pszBuffer, UInt cchLimitIn, PCChar pszFormat, va_list args)
{
	FMTSTR_FORMAT format;
	PChar p = pszBuffer;
	UInt i, len, sign;
	Char number[65];
	FMTSTR_DATA argData;

	if (pszBuffer == NULL)
		cchLimitIn = (UInt)-1;

	while (*pszFormat && (cchLimitIn > 1))
	{
		if (*pszFormat != '%')
		{
			if (pszBuffer != NULL)
				*p = *pszFormat;

			++p;
			++pszFormat;
			cchLimitIn--;
			continue;
		}

		pszFormat++;
		if (*pszFormat == '%')
		{
			if (pszBuffer != NULL)
				*p = *pszFormat;

			++p;
			++pszFormat;
			cchLimitIn--;
			continue;
		}

		pszFormat += FormatString_ParseFormatA(pszFormat, &format);

		switch (format.type)
		{
		case FMTSTR_FORMAT_WCHAR:
			argData.wchar_view = (WChar)va_arg(args, int);
			break;

		case FMTSTR_FORMAT_CHAR:
			argData.char_view = (Char)va_arg(args, int);
			break;

		case FMTSTR_FORMAT_STRING:
			argData.lpcstr_view = va_arg(args, PCChar);
			break;

		case FMTSTR_FORMAT_WSTRING:
			argData.lpcwstr_view = va_arg(args, PCWChar);
			break;

#if MY_OS_WIN32
		case FMTSTR_FORMAT_BINARY:
#endif
		case FMTSTR_FORMAT_HEXA:
		case FMTSTR_FORMAT_SIGNED:
		case FMTSTR_FORMAT_UNSIGNED:
			if (format.flags & FMTSTR_FLAG_I64)
				argData.int64_view = va_arg(args, Int64);
			else
				argData.int_view = va_arg(args, Int);
			break;

		default:
			argData.wchar_view = 0;
			break;
		}

		len = FormatString_GetLen(&format, &argData, number, cchLimitIn - 1);
		sign = 0;
		if (!(format.flags & FMTSTR_FLAG_LEFTALIGN) && format.precision < format.width)
		{
			i = format.width - format.precision;
			if (i)
			{
				if (pszBuffer != NULL)
					memset(p, ' ', i);
				p += i;
				cchLimitIn -= i;
			}
		}

		switch (format.type)
		{
		case FMTSTR_FORMAT_WCHAR:
			if (pszBuffer != NULL)
				*p = (Char)argData.wchar_view;
			++p;
			break;

		case FMTSTR_FORMAT_CHAR:
			if (pszBuffer != NULL)
				*p = argData.char_view;
			++p;
			break;

		case FMTSTR_FORMAT_STRING:
		case FMTSTR_FORMAT_WSTRING:
			if (len == 0)
				break;

			if (format.type == FMTSTR_FORMAT_STRING)
			{
				if (argData.lpcstr_view)
				{
					if (pszBuffer != NULL)
						CopyMemory(p, argData.lpcstr_view, len);
					p += len;
					break;
				}
			}
			else if (argData.lpcwstr_view)
			{
				Int MaxLength;
				PCWChar ptr = argData.lpcwstr_view;

				MaxLength = pszBuffer == NULL ? 0 : cchLimitIn - (p - pszBuffer);
#if MY_NT_DDK
				UNICODE_STRING  Unicode;
				ANSI_STRING     Ansi;

				Unicode.Buffer = (PWChar)ptr;
				Unicode.Length = (UShort)(len * sizeof(*ptr));
				Unicode.MaximumLength = Unicode.Length;

				Ansi.Buffer = p;
				Ansi.MaximumLength = Ansi.Length;

				RtlUnicodeStringToAnsiString(&Ansi, &Unicode, FALSE);
				len = Ansi.Length;
#else
				len = WideCharToMultiByte(CP_ACP, 0, ptr, len, p, MaxLength, NULL, NULL);
#endif
				p += len;
				break;
			}

			switch (len)
			{
			default:
				p[5] = ')';
			case 5:
				p[4] = 'l';
			case 4:
				p[3] = 'l';
			case 3:
				p[2] = 'u';
			case 2:
				p[1] = 'n';
			case 1:
				p[0] = '(';
				break;
			}
			p += ML_MIN(6, len);
			break;

		case FMTSTR_FORMAT_HEXA:
			if ((format.flags & FMTSTR_FLAG_PREFIX_HEX) && (cchLimitIn > 3))
			{
				*p++ = '0';
				*p++ = (format.flags & FMTSTR_FLAG_UPPER_HEX) ? 'X' : 'x';
				cchLimitIn -= 2;
				len -= 2;
			}
			/* fall through */
		case FMTSTR_FORMAT_SIGNED:
			/* Transfer the sign now, just in case it will be zero-padded*/
			if (number[0] == '-')
			{
				*p++ = '-';
				sign = 1;
			}
			/* fall through */
		case FMTSTR_FORMAT_UNSIGNED:
			//                for (i = len; i < format.precision; i++, cchLimitIn--) *p++ = '0';
			if (len < format.precision)
			{
				i = format.precision - len;
				if (i)
				{
					if (pszBuffer != NULL)
						memset(p, '0', i);
					p += i;
					cchLimitIn -= i;
				}
			}
			/* fall through */

#if MY_OS_WIN32

		case FMTSTR_FORMAT_BINARY:
			i = len - sign;
			if (i)
			{
				if (pszBuffer != NULL)
					CopyMemory(p, number + sign, i);
				p += i;
			}
			break;
#endif

		case FMTSTR_FORMAT_UNKNOWN:
			continue;
		}

		if ((format.flags & FMTSTR_FLAG_LEFTALIGN) && format.precision < format.width)
		{
			i = format.width - format.precision;
			if (i)
			{
				if (pszBuffer != NULL)
					memset(p, ' ', i);
				p += i;
				cchLimitIn -= i;
			}
		}
		/*
		if (format.flags & FMTSTR_FLAG_LEFTALIGN)
		for (i = format.precision; i < format.width; i++, cchLimitIn--)
		*p++ = ' ';
		*/
		cchLimitIn -= len;
	}

	if (pszBuffer != NULL)
		*p = 0;

	return (cchLimitIn >= 1) ? (Int)(p - pszBuffer) : -1;
}

/***********************************************************************
*           wvnsprintfW   (SHLWAPI.@)
*
* See wvnsprintfA.
*/
Int FormatStringvnW(PWChar pszBuffer, UInt cchLimitIn, PCWChar pszFormat, va_list args)
{
	FMTSTR_FORMAT format;
	PWChar p = pszBuffer;
	UInt i, len, sign;
	Char number[65];
	FMTSTR_DATA argData;

	if (pszBuffer == NULL)
		cchLimitIn = (UInt)-1;

	while (*pszFormat && (cchLimitIn > 1))
	{
		if (*pszFormat != '%')
		{
			if (pszBuffer != NULL)
				*p = *pszFormat;

			++p;
			++pszFormat;
			cchLimitIn--;
			continue;
		}

		pszFormat++;

		if (*pszFormat == '%')
		{
			if (pszBuffer != NULL)
				*p = *pszFormat;

			++p;
			++pszFormat;
			cchLimitIn--;
			continue;
		}

		pszFormat += FormatString_ParseFormatW(pszFormat, &format);

		switch (format.type)
		{
		case FMTSTR_FORMAT_WCHAR:
			argData.wchar_view = (WChar)va_arg(args, int);
			break;

		case FMTSTR_FORMAT_CHAR:
			argData.char_view = (Char)va_arg(args, int);
			break;

		case FMTSTR_FORMAT_STRING:
			argData.lpcstr_view = va_arg(args, PCChar);
			break;

		case FMTSTR_FORMAT_WSTRING:
			argData.lpcwstr_view = va_arg(args, PCWChar);
			break;

#if MY_OS_WIN32
		case FMTSTR_FORMAT_BINARY:
#endif
		case FMTSTR_FORMAT_HEXA:
		case FMTSTR_FORMAT_SIGNED:
		case FMTSTR_FORMAT_UNSIGNED:
			if (format.flags & FMTSTR_FLAG_I64)
				argData.int64_view = va_arg(args, Int64);
			else
				argData.int_view = va_arg(args, Int);
			break;

		default:
			argData.wchar_view = 0;
			break;
		}

		len = FormatString_GetLen(&format, &argData, number, cchLimitIn - 1);
		sign = 0;
		if (!(format.flags & FMTSTR_FLAG_LEFTALIGN))
		{
			if (pszBuffer != NULL)
			{
				for (i = format.precision; i < format.width; i++, cchLimitIn--)
					*p++ = ' ';
			}
			else if (format.precision < format.width)
			{
				UInt l = format.width - format.precision;
				p += l;
				cchLimitIn -= l;
			}
		}

		switch (format.type)
		{
		case FMTSTR_FORMAT_WCHAR:
			if (pszBuffer != NULL)
				*p = argData.wchar_view;
			++p;
			break;

		case FMTSTR_FORMAT_CHAR:
			if (pszBuffer != NULL)
				*p = argData.char_view;
			++p;
			break;

		case FMTSTR_FORMAT_STRING:
		case FMTSTR_FORMAT_WSTRING:
			if (len == 0)
				break;

			if (format.type == FMTSTR_FORMAT_STRING)
			{
				if (argData.lpcstr_view)
				{
					PCChar ptr = argData.lpcstr_view;
					UInt MaxLength = cchLimitIn - (p - pszBuffer);

					MaxLength = (cchLimitIn - (p - pszBuffer));
					if (MaxLength > len * 2)
						MaxLength = len * 2;

#if MY_NT_DDK

					UNICODE_STRING  Unicode;
					ANSI_STRING     Ansi;

					Unicode.Buffer = (PWChar)ptr;
					Unicode.Length = (UShort)(len * sizeof(*ptr));
					Unicode.MaximumLength = Unicode.Length;

					Ansi.Buffer = (PChar)p;
					Ansi.MaximumLength = Ansi.Length;

					RtlUnicodeStringToAnsiString(&Ansi, &Unicode, FALSE);
					len = Ansi.Length;
#else
					len = MultiByteToWideChar(CP_ACP, 0, ptr, len, p, pszBuffer == NULL ? 0 : MaxLength);
#endif // MY_NT_DDK
					p += len;
					break;
				}
			}
			else if (argData.lpcwstr_view != NULL)
			{
				if (pszBuffer != NULL)
					CopyMemory(p, argData.lpcwstr_view, len * sizeof(WChar));
				p += len;
				cchLimitIn -= len;
				break;
			}

			if (pszBuffer != NULL)
				switch (len)
				{
				default:
					p[5] = ')';
				case 5:
					p[4] = 'l';
				case 4:
					p[3] = 'l';
				case 3:
					p[2] = 'u';
				case 2:
					p[1] = 'n';
				case 1:
					p[0] = '(';
					break;
				}

			len = ML_MIN(6, len);
			p += len;
			break;

		case FMTSTR_FORMAT_HEXA:
			if ((format.flags & FMTSTR_FLAG_PREFIX_HEX) && (cchLimitIn > 3))
			{
				if (pszBuffer != NULL)
				{
					*p++ = '0';
					*p++ = (format.flags & FMTSTR_FLAG_UPPER_HEX) ? 'X' : 'x';
				}
				else
				{
					p += 2;
				}
				cchLimitIn -= 2;
				len -= 2;
			}
			/* fall through */
		case FMTSTR_FORMAT_SIGNED:
			/* Transfer the sign now, just in case it will be zero-padded*/
			if (number[0] == '-')
			{
				if (pszBuffer != NULL)
					*p = '-';
				++p;
				sign = 1;
			}
			/* fall through */
		case FMTSTR_FORMAT_UNSIGNED:
			if (pszBuffer != NULL)
			{
				for (i = len; i < format.precision; i++, cchLimitIn--)
					*p++ = '0';
			}
			else if (len < format.precision)
			{
				UInt l = format.precision - len;
				p += l;
				cchLimitIn -= l;
			}
			/* fall through */
#if MY_OS_WIN32
		case FMTSTR_FORMAT_BINARY:
#endif
			if (pszBuffer != NULL)
			{
				for (i = sign; i < len; i++)
					*p++ = (WChar)number[i];
			}
			else if (sign < len)
			{
				UInt l = len - sign;
				p += l;
			}

			break;

		case FMTSTR_FORMAT_UNKNOWN:
			continue;
		}
		if (format.flags & FMTSTR_FLAG_LEFTALIGN)
		{
			if (pszBuffer != NULL)
			{
				for (i = format.precision; i < format.width; i++, cchLimitIn--)
					*p++ = ' ';
			}
			else if (format.precision < format.width)
			{
				UInt l = format.width - format.precision;
				p += l;
				cchLimitIn -= l;
			}
		}
		cchLimitIn -= len;
	}

	if (pszBuffer != NULL)
		*p = 0;

	return (cchLimitIn >= 1) ? (Int)(p - pszBuffer) : -1;
}

Int FormatStringvA(PChar pszBuffer, PCChar pszFormat, va_list args)
{
	return FormatStringvnA(pszBuffer, MAX_INT, pszFormat, args);
}

Int FormatStringvW(PWChar pszBuffer, PCWChar pszFormat, va_list args)
{
	return FormatStringvnW(pszBuffer, MAX_INT / 2, pszFormat, args);
}

Int FormatStringA(PChar pszBuffer, PCChar pszFormat, ...)
{
	va_list valist;

	va_start(valist, pszFormat);
	return FormatStringvA(pszBuffer, pszFormat, valist);
}

Int FormatStringW(PWChar pszBuffer, PCWChar pszFormat, ...)
{
	va_list valist;

	va_start(valist, pszFormat);
	return FormatStringvW(pszBuffer, pszFormat, valist);
}


_ML_C_HEAD_

#if ML_USER_MODE

ULONG_PTR PrintConsoleA(PCSTR Format, ...)
{
	BOOL        Result;
	ULONG       Length;
	CHAR        Buffer[0xF00 / 2];
	va_list     Args;
	HANDLE      StdHandle;

	va_start(Args, Format);
	//Length = _vsnprintf(Buffer, countof(Buffer) - 1, Format, Args);
	Length = FormatStringvnA(Buffer, countof(Buffer) - 1, Format, Args);
	if (Length == -1)
		return Length;

	StdHandle = Nt_CurrentPeb()->ProcessParameters->StandardOutput;

	if (PtrAnd(StdHandle, 0x10000003) == (HANDLE)3)
	{
		Result = WriteConsoleA(
			StdHandle,
			Buffer,
			Length,
			&Length,
			NULL
		);
	}
	else
	{
		LARGE_INTEGER BytesWritten;

		Result = NT_SUCCESS(NtFileDisk::Write(StdHandle, Buffer, Length, &BytesWritten));
		Length = Result ? (ULONG_PTR)BytesWritten.QuadPart : 0;
	}

	return Result ? Length : 0;
}

ULONG_PTR PrintConsoleW(PCWSTR Format, ...)
{
	return PrintConsole(Format);
}

ULONG_PTR PrintConsole(PCWSTR Format, ...)
{
	BOOL        Success, IsConsole;
	ULONG       Length;
	WCHAR       Buffer[0xF00 / 2];
	va_list     Args;
	HANDLE      StdOutput;

	va_start(Args, Format);
	//Length = _vsnwprintf(Buffer, countof(Buffer) - 1, Format, Args);
	Length = FormatStringvnW(Buffer, countof(Buffer) - 1, Format, Args);
	if (Length == -1)
		return Length;

	StdOutput = CurrentPeb()->ProcessParameters->StandardOutput;

	IsConsole = TRUE;

	LOOP_ONCE
	{
		IO_STATUS_BLOCK             IoStatus;
		FILE_FS_DEVICE_INFORMATION  VolumeInfo;

		if (PtrAnd(StdOutput, 0x10000003) == (HANDLE)3)
			break;

		IoStatus.Status = NtQueryVolumeInformationFile(
			StdOutput,
			&IoStatus,
			&VolumeInfo,
			sizeof(VolumeInfo),
			FileFsDeviceInformation
			);
		if (NT_FAILED(IoStatus.Status))
			break;

		IsConsole = VolumeInfo.DeviceType == FILE_DEVICE_CONSOLE;
	}

		// if (PtrAnd(StdHandle, (1 << (bitsof(ULONG_PTR) - 4)) | 3) == (HANDLE)3)
		if (IsConsole)
		{
			Success = WriteConsoleW(
				StdOutput,
				Buffer,
				Length,
				&Length,
				NULL
			);
		}
		else
		{
			LARGE_INTEGER BytesWritten;

			Success = NT_SUCCESS(NtFileDisk::Write(StdOutput, Buffer, Length * sizeof(WCHAR), &BytesWritten));
			Length = Success ? ((ULONG_PTR)BytesWritten.QuadPart / sizeof(WCHAR)) : 0;
		}

	return Success ? Length : 0;
}

CHAR ConsoleReadChar()
{
	HANDLE hConsole;
	INPUT_RECORD ir;
	DWORD dwRead;

	hConsole = CreateFileW(
		L"CONIN$",
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		0,
		NULL
	);
	LOOP_FOREVER
	{
		ReadConsoleInputW(hConsole, &ir, 1, &dwRead);
		if (dwRead == 0)
			break;

		if (ir.EventType == KEY_EVENT &&
			ir.Event.KeyEvent.bKeyDown == TRUE &&
			ir.Event.KeyEvent.uChar.UnicodeChar)
		{
			break;
		}
	}

	CloseHandle(hConsole);

	return ir.Event.KeyEvent.uChar.AsciiChar;
}

VOID ClearConsoleScreen()
{
	ULONG                       BytesWritten;
	COORD                       Position;
	HANDLE                      StdOutputHandle;
	CONSOLE_SCREEN_BUFFER_INFO  ScreenBufferInfo;

	StdOutputHandle = Nt_CurrentPeb()->ProcessParameters->StandardOutput;

	GetConsoleScreenBufferInfo(StdOutputHandle, &ScreenBufferInfo);

	Position.X = 0;
	Position.Y = 0;
	FillConsoleOutputCharacterW(StdOutputHandle, ' ', ScreenBufferInfo.dwSize.X * ScreenBufferInfo.dwSize.Y, Position, &BytesWritten);

	SetConsoleCursorPosition(StdOutputHandle, Position);
}

VOID PauseConsole(PCWSTR PauseText)
{
	if (PauseText != NULL)
		PrintConsole(L"%s\n", PauseText);
	ConsoleReadChar();
}

#endif // ML_USER_MODE

_ML_C_TAIL_
#if !ML_DISABLE_THIRD_LIB

#if !ML_DISABLE_THIRD_LIB_UCL

#define NRV2E

#include <ucl/ucl_conf.h>
#include <ucl/ucl.h>
#include <ucl/n2_99.c>
#include <ucl/alloc.c>

_ML_C_HEAD_

Bool
FASTCALL
UCL_NRV2E_Compress(
	PVoid   Input,
	ULong   InputSize,
	PVoid   Output,
	PULong  OutputSize,
	Long    Level
)
{
	Bool                    Result;
	ucl_compress_config_t   UclConfig;

	FillMemory(&UclConfig, sizeof(UclConfig), (Byte)-1);
	UclConfig.bb_endian = 0;
	UclConfig.bb_size = 32;
	UclConfig.max_offset = 0x3FFFFF;

	Result = ucl_nrv2e_99_compress(
		(PByte)Input,
		InputSize,
		(PByte)Output,
		(ucl_uint *)OutputSize,
		NULL,
		Level,
		&UclConfig,
		NULL
	);

	return Result == UCL_E_OK;
}

Bool
FASTCALL
UCL_NRV2E_Decompress(
	PVoid   Input,
	ULong   InputSize,
	PVoid   Output,
	PULong  OutputSize
)
{
	ULong Size;
	UNREFERENCED_PARAMETER(InputSize);
	Size = (ULONG)UCL_NRV2E_DecompressASMFast32(Input, Output);
	if (OutputSize != NULL)
		*OutputSize = Size;

	return True;
}

#if MY_OS_WIN32

#if ML_AMD64

ASM ULONG_PTR FASTCALL UCL_NRV2E_DecompressASMFast32(PVOID /* pvInput */, PVOID /* pvOutput */)
{
	return 0;
}

#else // x86

ASM ULONG_PTR FASTCALL UCL_NRV2E_DecompressASMFast32(PVOID /* pvInput */, PVOID /* pvOutput */)
{
	INLINE_ASM
	{
		add     esp, -0x18;
		mov[esp + 0x00], ebx;
		mov[esp + 0x04], ebp;
		mov[esp + 0x08], esi;
		mov[esp + 0x0C], edi;
		mov[esp + 0x10], edx;
		cld;
		mov     esi, ecx;
		mov     edi, edx;
		or ebp, 0xFFFFFFFF;
		xor     ecx, ecx;
		jmp L029;

		INLINE_ASM __emit 0x8D INLINE_ASM __emit 0xB4 INLINE_ASM __emit 0x26 INLINE_ASM __emit 0x00 INLINE_ASM __emit 0x00 INLINE_ASM __emit 0x00 INLINE_ASM __emit 0x00;   // lea esi, [esi]
		INLINE_ASM __emit 0x8D INLINE_ASM __emit 0xB4 INLINE_ASM __emit 0x26 INLINE_ASM __emit 0x00 INLINE_ASM __emit 0x00 INLINE_ASM __emit 0x00 INLINE_ASM __emit 0x00;   // lea esi, [esi]
	L022:
		mov     al, byte ptr[esi];
		inc     esi;
		mov     byte ptr[edi], al;
		inc     edi;
	L026:
		add     ebx, ebx;
		jnb L033;
		jnz L022;
	L029:
		mov     ebx, dword ptr[esi];
		sub     esi, -0x4;
		adc     ebx, ebx;
		jb L022;
	L033:
		mov     eax, 0x1;
	L034:
		add     ebx, ebx;
		jnz L039;
		mov     ebx, dword ptr[esi];
		sub     esi, -0x4;
		adc     ebx, ebx;
	L039:
		adc     eax, eax;
		add     ebx, ebx;
		jnb L047;
		jnz L055;
		mov     ebx, dword ptr[esi];
		sub     esi, -0x4;
		adc     ebx, ebx;
		jb L055;
	L047:
		dec     eax;
		add     ebx, ebx;
		jnz L053;
		mov     ebx, dword ptr[esi];
		sub     esi, -0x4;
		adc     ebx, ebx;
	L053:
		adc     eax, eax;
		jmp L034;
	L055:
		sub     eax, 0x3;
		jb L072;
		shl     eax, 0x8;
		mov     al, byte ptr[esi];
		inc     esi;
		xor     eax, 0xFFFFFFFF;
		je L120;
		sar     eax, 1;
		mov     ebp, eax;
		jnb L078;
	L065:
		add     ebx, ebx;
		jnz L070;
		mov     ebx, dword ptr[esi];
		sub     esi, -0x4;
		adc     ebx, ebx;
	L070:
		adc     ecx, ecx;
		jmp L099;
	L072:
		add     ebx, ebx;
		jnz L077;
		mov     ebx, dword ptr[esi];
		sub     esi, -0x4;
		adc     ebx, ebx;
	L077:
		jb L065;
	L078:
		inc     ecx;
		add     ebx, ebx;
		jnz L084;
		mov     ebx, dword ptr[esi];
		sub     esi, -0x4;
		adc     ebx, ebx;
	L084:
		jb L065;
	L085:
		add     ebx, ebx;
		jnz L090;
		mov     ebx, dword ptr[esi];
		sub     esi, -0x4;
		adc     ebx, ebx;
	L090:
		adc     ecx, ecx;
		add     ebx, ebx;
		jnb L085;
		jnz L098;
		mov     ebx, dword ptr[esi];
		sub     esi, -0x4;
		adc     ebx, ebx;
		jnb L085;
	L098:
		add     ecx, 0x2;
	L099:
		cmp     ebp, -0x500;
		adc     ecx, 0x2;
		lea     edx, dword ptr[edi + ebp];
		cmp     ebp, -0x4;
		jbe L111;
	L104:
		mov     al, byte ptr[edx];
		inc     edx;
		mov     byte ptr[edi], al;
		inc     edi;
		dec     ecx;
		jnz L104;
		jmp L026;
	L111:
		mov[esp + 0x14], ecx;
		and     ecx, ~3;
		jecxz   L111_END;
	L111_:
		mov     eax, dword ptr[edx];
		add     edx, 0x4;
		mov     dword ptr[edi], eax;
		add     edi, 0x4;
		sub     ecx, 0x4;
		ja L111_;

	L111_END:

		mov     ecx,[esp + 0x14];
		and     ecx, 3;
		jecxz   L111_LOOP_2_END;
		mov[esp + 0x14], ecx;

	L111_LOOP_2:
		mov     al,[edx];
		mov[edi], al;
		inc     edx;
		inc     edi;
		loop    L111_LOOP_2;

		mov     ecx,[esp + 0x14];
		sub     edx, ecx;
		add     edx, 4;

	L111_LOOP_2_END:

		//        add     edi, ecx;
		xor     ecx, ecx;
		jmp L026;
	L120:
		mov     eax, edi;
		mov     ebx,[esp + 0x00];
		mov     ebp,[esp + 0x04];
		mov     esi,[esp + 0x08];
		mov     edi,[esp + 0x0C];
		sub     eax,[esp + 0x10];
		add     esp, 0x18;
		ret;
	}
}

#endif // x64

#endif // MY_OS_WIN32

_ML_C_TAIL_

#endif // !ML_DISABLE_THIRD_LIB_UCL

#endif // !ML_DISABLE_THIRD_LIB

#if MY_OS_WIN32

#if !MY_X64

#define SHA224_DIGEST_SIZE ( 224 / 8)
#define SHA256_DIGEST_SIZE ( 256 / 8)
#define SHA384_DIGEST_SIZE ( 384 / 8)
#define SHA512_DIGEST_SIZE ( 512 / 8)

#define SHA256_BLOCK_SIZE  ( 512 / 8)
#define SHA512_BLOCK_SIZE  (1024 / 8)
#define SHA384_BLOCK_SIZE  SHA512_BLOCK_SIZE
#define SHA224_BLOCK_SIZE  SHA256_BLOCK_SIZE

#pragma warning(push, 0)

#if ML_X86

ASM VOID STDCALL sha256_update(sha256_ctx *ctx, PVOID message, ULONG len)
{
	static ULONG sha256_k[] =
	{
		0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5,
		0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5,
		0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3,
		0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174,
		0xE49B69C1, 0xEFBE4786, 0x0FC19DC6, 0x240CA1CC,
		0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA,
		0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7,
		0xC6E00BF3, 0xD5A79147, 0x06CA6351, 0x14292967,
		0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13,
		0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85,
		0xA2BFE8A1, 0xA81A664B, 0xC24B8B70, 0xC76C51A3,
		0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070,
		0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5,
		0x391C0CB3, 0x4ED8AA4A, 0x5B9CCA4F, 0x682E6FF3,
		0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208,
		0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2
	};

	INLINE_ASM
	{
		sub     esp, 0x128
		mov     dword ptr[esp + 0xC], ebp
		mov     dword ptr[esp + 0x8], ebx
		mov     dword ptr[esp + 0x4], edi
		mov     dword ptr[esp], esi
		mov     esi, dword ptr[esp + 0x12C]
		mov     eax, dword ptr[esi]
		mov     ebx, dword ptr[esi + 0x4]
		mov     ecx, dword ptr[esi + 0x8]
		mov     edx, dword ptr[esi + 0xC]
		mov     ebp, dword ptr[esi + 0x10]
		mov     edi, dword ptr[esi + 0x14]
		bswap   eax
		bswap   ebx
		bswap   ecx
		bswap   edx
		bswap   ebp
		bswap   edi
		mov     dword ptr[esp + 0x28], eax
		mov     dword ptr[esp + 0x2C], ebx
		mov     dword ptr[esp + 0x30], ecx
		mov     dword ptr[esp + 0x34], edx
		mov     dword ptr[esp + 0x38], ebp
		mov     dword ptr[esp + 0x3C], edi
		mov     eax, dword ptr[esi + 0x18]
		mov     ebx, dword ptr[esi + 0x1C]
		mov     ecx, dword ptr[esi + 0x20]
		mov     edx, dword ptr[esi + 0x24]
		mov     ebp, dword ptr[esi + 0x28]
		mov     edi, dword ptr[esi + 0x2C]
		bswap   eax
		bswap   ebx
		bswap   ecx
		bswap   edx
		bswap   ebp
		bswap   edi
		mov     dword ptr[esp + 0x40], eax
		mov     dword ptr[esp + 0x44], ebx
		mov     dword ptr[esp + 0x48], ecx
		mov     dword ptr[esp + 0x4C], edx
		mov     dword ptr[esp + 0x50], ebp
		mov     dword ptr[esp + 0x54], edi
		mov     eax, dword ptr[esi + 0x30]
		mov     ebx, dword ptr[esi + 0x34]
		mov     ecx, dword ptr[esi + 0x38]
		mov     edx, dword ptr[esi + 0x3C]
		bswap   eax
		bswap   ebx
		bswap   ecx
		bswap   edx
		mov     dword ptr[esp + 0x58], eax
		mov     dword ptr[esp + 0x5C], ebx
		mov     dword ptr[esp + 0x60], ecx
		mov     dword ptr[esp + 0x64], edx
		mov     ecx, -0xC0
		mov     esi, dword ptr[esp + 0x130]
		jmp L057;
		ASM_DUMMY(6);
		ASM_DUMMY(6);
		ASM_DUMMY(6);
		ASM_DUMMY(5);
	L057:
		mov     eax, dword ptr[esp + ecx + 0xEC]
			mov     ebx, dword ptr[esp + ecx + 0x120]
			mov     edi, dword ptr[esp + ecx + 0x10C]
			mov     ebp, eax
			mov     edx, ebx
			ror     eax, 0x7
			shr     ebp, 0x3
			ror     ebx, 0x11
			shr     edx, 0xA
			xor ebp, eax
			ror     eax, 0xB
			xor edx, ebx
			ror     ebx, 0x2
			xor eax, ebp
			add     edi, dword ptr[esp + ecx + 0xE8]
			xor ebx, edx
			add     edi, eax
			add     edi, ebx
			mov     dword ptr[esp + ecx + 0x128], edi
			add     ecx, 0x4
			jnz L057
			mov     eax, dword ptr[esi]
			mov     ebx, dword ptr[esi + 0x4]
			mov     ecx, dword ptr[esi + 0x8]
			mov     edx, dword ptr[esi + 0xC]
			mov     ebp, dword ptr[esi + 0x10]
			mov     edi, dword ptr[esi + 0x14]
			mov     dword ptr[esp + 0x10], ebx
			mov     dword ptr[esp + 0x14], ecx
			mov     dword ptr[esp + 0x18], edx
			mov     dword ptr[esp + 0x1C], edi
			mov     ebx, dword ptr[esi + 0x18]
			mov     edx, dword ptr[esi + 0x1C]
			mov     dword ptr[esp + 0x20], ebx
			mov     ecx, -0x100
			ASM_DUMMY(6);
	L093:
		mov     ebx, ebp
			add     edx, dword ptr[esp + ecx + 0x128]
			mov     edi, ebp
			ror     ebx, 0x6
			mov     esi, ebp
			ror     edi, 0xB
			add     edx, dword ptr[ecx + sha256_k + 100h]
			ror     esi, 0x19
			xor ebx, edi
			mov     edi, dword ptr[esp + 0x20]
			xor ebx, esi
			mov     esi, dword ptr[esp + 0x1C]
			mov     dword ptr[esp + 0x1C], ebp
			mov     dword ptr[esp + 0x20], esi
			xor     esi, edi
			mov     dword ptr[esp + 0x24], edi
			add     edx, ebx
			and     esi, ebp
			mov     ebx, dword ptr[esp + 0x10]
			xor esi, edi
			mov     edi, dword ptr[esp + 0x14]
			add     edx, esi
			mov     ebp, ebx
			mov     dword ptr[esp + 0x14], ebx
			xor     ebx, eax
			mov     dword ptr[esp + 0x10], eax
			and     ebp, eax
			and     ebx, edi
			mov     esi, eax
			add     ebx, ebp
			ror     esi, 0x2
			mov     ebp, eax
			ror     eax, 0xD
			xor eax, esi
			ror     ebp, 0x16
			xor eax, ebp
			mov     ebp, dword ptr[esp + 0x18]
			mov     dword ptr[esp + 0x18], edi
			add     ebp, edx
			add     eax, edx
			mov     edx, dword ptr[esp + 0x24]
			add     eax, ebx
			add     ecx, 0x4
			jnz L093
			mov     esi, dword ptr[esp + 0x130]
			mov     ebx, dword ptr[esp + 0x10]
			mov     ecx, dword ptr[esp + 0x14]
			add     dword ptr[esi], eax
			add     dword ptr[esi + 0x4], ebx
			mov     ebx, dword ptr[esp + 0x1C]
			mov     eax, dword ptr[esp + 0x20]
			add     dword ptr[esi + 0x8], ecx
			add     dword ptr[esi + 0xC], edi
			add     dword ptr[esi + 0x10], ebp
			add     dword ptr[esi + 0x14], ebx
			add     dword ptr[esi + 0x18], eax
			add     dword ptr[esi + 0x1C], edx
			mov     esi, dword ptr[esp]
			mov     edi, dword ptr[esp + 0x4]
			mov     ebx, dword ptr[esp + 0x8]
			mov     ebp, dword ptr[esp + 0xC]
			add     esp, 0x128
			ret     0x8

	}
}

ASM VOID STDCALL sha256(PVOID message, ULONG len, PVOID digest)
{
	INLINE_ASM
	{
		push    ebp
		mov     ebp, esp
		add     esp, -0x80
		push    edi
		push    esi
		mov     edx, dword ptr[ebp + 0x10]
		mov     edi, dword ptr[ebp + 0xC]
		mov     esi, dword ptr[ebp + 0x8]
		mov     dword ptr[edx], 0x6A09E667
		mov     dword ptr[edx + 0x4], 0xBB67AE85
		mov     dword ptr[edx + 0x8], 0x3C6EF372
		mov     dword ptr[edx + 0xC], 0xA54FF53A
		mov     dword ptr[edx + 0x10], 0x510E527F
		mov     dword ptr[edx + 0x14], 0x9B05688C
		mov     dword ptr[edx + 0x18], 0x1F83D9AB
		mov     dword ptr[edx + 0x1C], 0x5BE0CD19
		shr     edi, 0x6
		je L024
	L018 :
		push    dword ptr[ebp + 0x10]
			push    esi
			call    sha256_update
			add     esi, 0x40
			dec     edi
			jnz L018
		L024 :
		mov     ecx, dword ptr[ebp + 0xC]
			lea     edi, dword ptr[ebp - 0x80]
			and ecx, 0x3F
			push    dword ptr[ebp + 0x10]
			push    edi
			mov     eax, ecx
			cld
			rep     movs byte ptr es : [edi], byte ptr[esi]
			mov     byte ptr[edi], 0x80
			inc     edi
			mov     ecx, eax
			xor     eax, eax
			bswap   esi
			sub     ecx, 0x37
			ja L042
			neg     ecx
			rep     stos byte ptr es : [edi]
			jmp L049
		L042 :
		sub     ecx, 0x40
			neg     ecx
			rep     stos byte ptr es : [edi]
			call    sha256_update
			lea     edx, dword ptr[ebp - 0x40]
			push    dword ptr[ebp + 0x10]
			push    edx
		L049 :
		mov     esi, dword ptr[ebp + 0xC]
			mov     eax, esi
			rol     esi, 0x3
			shl     eax, 0x3
			and esi, 0x7
			bswap   eax
			bswap   esi
			mov     dword ptr[edi + 0x4], eax
			mov     dword ptr[edi], esi
			call    sha256_update
			pop     esi
			pop     edi
			leave
			ret     0xC
	}
}

#endif

#pragma warning(pop)

#endif // MY_X64
#endif // MY_OS_WIN32




#pragma warning(disable:4750)

_ML_C_HEAD_

#if ML_KERNEL_MODE

PVOID GetImageBaseAddress(PVOID ImageAddress)
{
	ULONG                           Size, BufferSize;
	NTSTATUS                        Status;
	PRTL_PROCESS_MODULE_INFORMATION Module;
	PRTL_PROCESS_MODULES            ModuleInformation;

	Size = 0;
	BufferSize = 0;
	ModuleInformation = nullptr;

	LOOP_FOREVER
	{
		Status = ZwQuerySystemInformation(SystemModuleInformation, ModuleInformation, BufferSize, &Size);

		if (NT_SUCCESS(Status))
			break;

		if (Status != STATUS_INFO_LENGTH_MISMATCH)
			return nullptr;

		ModuleInformation = (PRTL_PROCESS_MODULES)AllocStack(Size - BufferSize);
		BufferSize = Size;
	}

	Module = ModuleInformation->Modules;
	for (ULONG Count = ModuleInformation->NumberOfModules; Count; --Count)
	{
		ULONG_PTR ImageBase, EndOfImage;

		ImageBase = (ULONG_PTR)Module->ImageBase;
		EndOfImage = ImageBase + Module->ImageSize;
		if (IN_RANGE(ImageBase, (ULONG_PTR)ImageAddress, EndOfImage))
			return (PVOID)ImageBase;

		++Module;
	}

	return nullptr;
}

#else // user mode

PVOID GetImageBaseAddress(PVOID ImageAddress)
{
	LDR_MODULE* LdrModule;

	LdrModule = Ldr::FindLdrModuleByHandle(ImageAddress);

	return LdrModule == nullptr ? nullptr : LdrModule->DllBase;
}

#endif // ML_KERNEL_MODE

BOOL ValidateDataDirectory(PIMAGE_DATA_DIRECTORY DataDirectory, ULONG_PTR SizeOfImage)
{
	if (DataDirectory->Size == 0)
		return FALSE;

	if ((ULONG64)DataDirectory->VirtualAddress + DataDirectory->Size > ULONG_MAX)
		return FALSE;

	return DataDirectory->VirtualAddress <= SizeOfImage && DataDirectory->VirtualAddress + DataDirectory->Size <= SizeOfImage;
}

BOOL IsValidImage(PVOID ImageBase, ULONG_PTR Flags)
{
	PVOID                       Base, End, EndOfImage;
	ULONG_PTR                   Size, SizeOfImage;
	PIMAGE_DOS_HEADER           DosHeader;
	PIMAGE_NT_HEADERS           NtHeader;
	PIMAGE_NT_HEADERS64         NtHeader64;
	PIMAGE_DATA_DIRECTORY       DataDirectory, Directory;
	PIMAGE_IMPORT_DESCRIPTOR    ImportDescriptor;

	DosHeader = (PIMAGE_DOS_HEADER)ImageBase;
	if (DosHeader->e_magic != IMAGE_DOS_SIGNATURE)
		return FALSE;

	NtHeader = (PIMAGE_NT_HEADERS)((ULONG_PTR)ImageBase + DosHeader->e_lfanew);
	if (NtHeader->Signature != IMAGE_NT_SIGNATURE)
		return FALSE;

	if (Flags == 0)
		return TRUE;

	switch (NtHeader->OptionalHeader.Magic)
	{
	case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
		DataDirectory = NtHeader->OptionalHeader.DataDirectory;
		SizeOfImage = NtHeader->OptionalHeader.SizeOfImage;
		break;

	case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
		NtHeader64 = (PIMAGE_NT_HEADERS64)NtHeader;
		DataDirectory = NtHeader64->OptionalHeader.DataDirectory;
		SizeOfImage = NtHeader64->OptionalHeader.SizeOfImage;
		break;

	default:
		return FALSE;
	}

	if (FLAG_ON(Flags, IMAGE_VALID_IMPORT_ADDRESS_TABLE))
	{
		if (!ValidateDataDirectory(&DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT], SizeOfImage))
			return FALSE;

		ImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)((ULONG_PTR)ImageBase + DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
		if (ImportDescriptor->Name == 0)
			return FALSE;
	}

	if (FLAG_ON(Flags, IMAGE_VALID_EXPORT_ADDRESS_TABLE))
	{
		PIMAGE_EXPORT_DIRECTORY ExportDirectory;

		if (!ValidateDataDirectory(&DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT], SizeOfImage))
			return FALSE;

		ExportDirectory = (PIMAGE_EXPORT_DIRECTORY)((ULONG_PTR)ImageBase + DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
		if (ExportDirectory->AddressOfFunctions >= SizeOfImage)
			return FALSE;
	}

	if (FLAG_ON(Flags, IMAGE_VALID_RELOC))
	{
		if (!ValidateDataDirectory(&DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC], SizeOfImage))
			return FALSE;
	}

	if (FLAG_ON(Flags, IMAGE_VALID_RESOURCE))
	{
		if (!ValidateDataDirectory(&DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE], SizeOfImage))
			return FALSE;
	}

	return TRUE;
}

NTSTATUS
ReadRelocFromDisk(
	PCWSTR      ImageModule,
	PVOID*      Relocation,
	PLONG_PTR   RelocSize
)
{
	PVOID                       RelocBuffer;
	NTSTATUS                    Status;
	NtFileDisk                  file;
	IMAGE_DOS_HEADER            DosHeader;
	IMAGE_NT_HEADERS32          NtHeaders32;
	IMAGE_NT_HEADERS64          NtHeaders64;
	PIMAGE_OPTIONAL_HEADER32    OptionalHeader32;
	PIMAGE_OPTIONAL_HEADER64    OptionalHeader64;
	PIMAGE_SECTION_HEADER       SectionHeader, Section;
	PIMAGE_DATA_DIRECTORY       RelocDirectory;

	*Relocation = nullptr;

	Status = file.Open(ImageModule);

	if (!NT_SUCCESS(Status))
		return Status;

	Status = file.Read(&DosHeader, sizeof(DosHeader));
	if (!NT_SUCCESS(Status))
		return Status;

	if (DosHeader.e_magic != IMAGE_DOS_SIGNATURE)
		return STATUS_INVALID_IMAGE_WIN_32;

	Status = file.Seek(DosHeader.e_lfanew, FILE_BEGIN);
	if (!NT_SUCCESS(Status))
		return Status;

	Status = file.Read(&NtHeaders32, sizeof(NtHeaders32) - sizeof(NtHeaders32.OptionalHeader));
	if (!NT_SUCCESS(Status))
		return Status;

	if (NtHeaders32.Signature != IMAGE_NT_SIGNATURE)
		return STATUS_INVALID_IMAGE_WIN_32;

	OptionalHeader32 = (PIMAGE_OPTIONAL_HEADER32)AllocStack(NtHeaders32.FileHeader.SizeOfOptionalHeader);
	Status = file.Read(OptionalHeader32, NtHeaders32.FileHeader.SizeOfOptionalHeader);
	if (!NT_SUCCESS(Status))
		return Status;

	RelocDirectory = nullptr;
	switch (OptionalHeader32->Magic)
	{
	case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
		OptionalHeader64 = (PIMAGE_OPTIONAL_HEADER64)OptionalHeader32;
		RelocDirectory = &OptionalHeader64->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
		break;

	case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
		RelocDirectory = &OptionalHeader32->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
		break;
	}

	if (RelocDirectory->Size == 0 || RelocDirectory->VirtualAddress == NULL)
		return STATUS_ILLEGAL_DLL_RELOCATION;

	SectionHeader = (PIMAGE_SECTION_HEADER)AllocStack(NtHeaders32.FileHeader.NumberOfSections * sizeof(*SectionHeader));
	Status = file.Read(SectionHeader, NtHeaders32.FileHeader.NumberOfSections * sizeof(*SectionHeader));
	if (!NT_SUCCESS(Status))
		return Status;

	ULONG_PTR SectionCount, RelocBegin, RelocEnd;

	RelocBegin = RelocDirectory->VirtualAddress;
	RelocEnd = RelocBegin + RelocDirectory->Size;

	Section = SectionHeader;
	for (SectionCount = NtHeaders32.FileHeader.NumberOfSections; SectionCount != 0; ++Section, --SectionCount)
	{
		if (Section->VirtualAddress > RelocEnd)
			continue;

		if (Section->VirtualAddress + Section->SizeOfRawData < RelocBegin)
			continue;

		break;
	}

	if (SectionCount == 0)
		return STATUS_ILLEGAL_DLL_RELOCATION;

	Status = file.Seek(Section->PointerToRawData + RelocDirectory->VirtualAddress - Section->VirtualAddress, FILE_BEGIN);
	if (!NT_SUCCESS(Status))
		return Status;

	RelocBuffer = AllocateMemoryP(RelocDirectory->Size);
	if (RelocBuffer == nullptr)
		return STATUS_NO_MEMORY;

	Status = file.Read(RelocBuffer, RelocDirectory->Size);
	if (!NT_SUCCESS(Status))
	{
		FreeMemoryP(RelocBuffer);
		return Status;
	}

	*Relocation = RelocBuffer;
	*RelocSize = RelocDirectory->Size;

	return STATUS_SUCCESS;
}

NTSTATUS
ProcessOneRelocate(
	LDR_MODULE              *ImageModule,
	PIMAGE_BASE_RELOCATION2 RelocationBase,
	LONG_PTR                SizeOfRelocation,
	PRELOCATE_ADDRESS_INFO  Address
)
{
	PVOID       RelocateBase, ImageBase, ValuePtr, AddressToRelocate, NewAddress;
	LONG_PTR    SizeOfBlock;
	PIMAGE_BASE_RELOCATION2 Relocation;

	ImageBase = ImageModule->DllBase;
	Relocation = RelocationBase;
	AddressToRelocate = Address->AddressToRelocate;
	NewAddress = Address->NewAddress;

	for (; SizeOfRelocation > 0;)
	{
		PIMAGE_RELOCATION_ADDRESS_ENTRY TypeOffset;

		TypeOffset = Relocation->TypeOffset;
		SizeOfBlock = Relocation->SizeOfBlock;
		RelocateBase = PtrAdd(ImageBase, Relocation->VirtualAddress);

		SizeOfRelocation -= SizeOfBlock;
		SizeOfBlock -= sizeof(*Relocation) - sizeof(Relocation->TypeOffset);

		for (; SizeOfBlock > 0; ++TypeOffset, SizeOfBlock -= sizeof(*TypeOffset))
		{
			if (*(PUSHORT)TypeOffset == 0)
				continue;

			ValuePtr = PtrAdd(RelocateBase, TypeOffset->Offset);

#if ML_KERNEL_MODE
			if (!MmIsAddressValid(ValuePtr))
				continue;
#endif

			if (*(PVOID *)ValuePtr != AddressToRelocate)
				continue;

			if (Address->CanRelocate != nullptr)
			{
				if (!Address->CanRelocate(Address, ValuePtr))
					continue;
			}

			{
				PVOID Address;
#if ML_USER_MODE
				Address = ValuePtr;

#elif ML_KERNEL_MODE
				HookProtector hp(DISPATCH_LEVEL, ValuePtr, sizeof(PVOID));
#endif

				switch (TypeOffset->Type)
				{
				case IMAGE_REL_BASED_ABSOLUTE:
				case IMAGE_REL_BASED_HIGH:
				case IMAGE_REL_BASED_LOW:
				case IMAGE_REL_BASED_HIGHLOW:
					*(PVOID *)Address = NewAddress;
					break;

				default:
					break;
				}
			}
		}

		Relocation = PtrAdd(Relocation, Relocation->SizeOfBlock);
	}

	return STATUS_SUCCESS;
}

NTSTATUS
RelocateAddress(
	LDR_MODULE              *ImageModule,
	PRELOCATE_ADDRESS_INFO  Address,
	ULONG                   Count
)
{
	NTSTATUS                Status;
	LONG_PTR                SizeOfRelocation;
	PIMAGE_BASE_RELOCATION2 RelocationBase;

	Status = ReadRelocFromDisk(ImageModule->FullDllName.Buffer, (PVOID *)&RelocationBase, &SizeOfRelocation);
	if (!NT_SUCCESS(Status))
		return Status;

	for (; Count; --Count)
	{
		if (Address->AddressToRelocate != nullptr)
			ProcessOneRelocate(ImageModule, RelocationBase, SizeOfRelocation, Address);

		++Address;
	}

	MemoryAllocator::FreeMemory(RelocationBase);

	return STATUS_SUCCESS;
}

PVOID IATLookupRoutineByEntryNoFix(PVOID ImageBase, PVOID RoutineEntry)
{
	PIMAGE_DOS_HEADER           DosHeader;
	PIMAGE_NT_HEADERS           NtHeaders;
	PIMAGE_IMPORT_DESCRIPTOR    ImportDescriptor;
	PIMAGE_THUNK_DATA           ThunkData;

	if (!IsValidImage(ImageBase, IMAGE_VALID_IMPORT_ADDRESS_TABLE))
		return nullptr;

	DosHeader = (PIMAGE_DOS_HEADER)ImageBase;
	NtHeaders = (PIMAGE_NT_HEADERS)((ULONG_PTR)ImageBase + DosHeader->e_lfanew);

	ImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)((ULONG_PTR)ImageBase + NtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
	while (ImportDescriptor->Name)
	{
		ThunkData = (PIMAGE_THUNK_DATA)((ULONG_PTR)ImageBase + ImportDescriptor->FirstThunk);
		while (ThunkData->u1.Function)
		{
			if ((ULONG_PTR)ThunkData->u1.Function == (ULONG_PTR)RoutineEntry)
				return (PVOID)&ThunkData->u1.Function;
			++ThunkData;
		}

		++ImportDescriptor;
	}

	return nullptr;
}

ULONG_PTR IATLookupRoutineRVAByHashNoFix(PVOID ImageBase, ULONG_PTR Hash)
{
	PIMAGE_DOS_HEADER           DosHeader;
	PIMAGE_NT_HEADERS           NtHeaders;
	PIMAGE_IMPORT_DESCRIPTOR    ImportDescriptor;
	PIMAGE_THUNK_DATA           OriginalThunk, FirstThunk;

	if (IsValidImage(ImageBase, IMAGE_VALID_IMPORT_ADDRESS_TABLE) == FALSE)
		return IMAGE_INVALID_RVA;

	DosHeader = (PIMAGE_DOS_HEADER)ImageBase;
	NtHeaders = (PIMAGE_NT_HEADERS)((ULONG_PTR)ImageBase + DosHeader->e_lfanew);

	ImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)((ULONG_PTR)ImageBase + NtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

	for (; ImportDescriptor->Name != NULL && ImportDescriptor->FirstThunk != NULL; ++ImportDescriptor)
	{
		OriginalThunk = (PIMAGE_THUNK_DATA)ImageBase;
		if (ImportDescriptor->OriginalFirstThunk)
		{
			OriginalThunk = PtrAdd(OriginalThunk, ImportDescriptor->OriginalFirstThunk);
		}
		else
		{
			OriginalThunk = PtrAdd(OriginalThunk, ImportDescriptor->FirstThunk);
		}

		FirstThunk = (PIMAGE_THUNK_DATA)PtrAdd(ImageBase, ImportDescriptor->FirstThunk);

		for (; OriginalThunk->u1.AddressOfData != NULL; ++OriginalThunk, ++FirstThunk)
		{
			LONG_PTR FunctionName;

			FunctionName = OriginalThunk->u1.AddressOfData;
			if (FunctionName < 0)
				continue;

			FunctionName += (LONG_PTR)PtrAdd(ImageBase, 2);

			if ((HashAPI((PCSTR)FunctionName) ^ Hash) == 0)
			{
				return PtrOffset(&FirstThunk->u1.Function, ImageBase);
			}
		}
	}

	return IMAGE_INVALID_RVA;
}

PSTR EATLookupNameByHashNoFix(PVOID ImageBase, ULONG_PTR Hash)
{
	PIMAGE_DOS_HEADER           DosHeader;
	PIMAGE_NT_HEADERS           NtHeaders;
	PIMAGE_EXPORT_DIRECTORY     ExportDirectory;
	ULONG_PTR                   NumberOfNames;
	PULONG_PTR                  AddressOfFuntions;
	PCSTR                      *AddressOfNames;

	if (!IsValidImage(ImageBase, IMAGE_VALID_EXPORT_ADDRESS_TABLE))
		return nullptr;

	DosHeader = (PIMAGE_DOS_HEADER)ImageBase;
	NtHeaders = (PIMAGE_NT_HEADERS)((ULONG_PTR)ImageBase + DosHeader->e_lfanew);

	ExportDirectory = nullptr;
	ExportDirectory = PtrAdd(ExportDirectory, ImageBase);
	ExportDirectory = PtrAdd(ExportDirectory, NtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);

	NumberOfNames = ExportDirectory->NumberOfNames;
	AddressOfFuntions = PtrAdd((PULONG_PTR)ImageBase, ExportDirectory->AddressOfFunctions);
	AddressOfNames = PtrAdd((PCSTR *)ImageBase, ExportDirectory->AddressOfNames);

	do
	{
		if (!(HashAPI(PtrAdd((PCSTR)(ImageBase), *AddressOfNames)) ^ Hash))
		{
			return PtrAdd((PSTR)(ImageBase), *AddressOfNames);
		}

		++AddressOfNames;

	} while (--NumberOfNames);

	return nullptr;
}

PSTR EATLookupNameByNameNoFix(PVOID ImageBase, PSTR Name)
{
	return EATLookupNameByHashNoFix(ImageBase, HashAPI(Name));
}

PVOID EATLookupRoutineByHashNoFix(PVOID ImageBase, ULONG_PTR Hash)
{
	PIMAGE_DOS_HEADER           DosHeader;
	PIMAGE_NT_HEADERS           NtHeaders;
	PIMAGE_EXPORT_DIRECTORY     ExportDirectory;
	ULONG_PTR                   NumberOfNames;
	PULONG_PTR                  AddressOfFuntions;
	PCSTR                      *AddressOfNames;
	PUSHORT                     AddressOfNameOrdinals;

	if (Hash != 0 && !IsValidImage(ImageBase, IMAGE_VALID_EXPORT_ADDRESS_TABLE))
		return nullptr;

	DosHeader = (PIMAGE_DOS_HEADER)ImageBase;
	NtHeaders = (PIMAGE_NT_HEADERS)((ULONG_PTR)ImageBase + DosHeader->e_lfanew);

	switch (NtHeaders->OptionalHeader.Magic)
	{
	case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
		break;

	case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
		return EATLookupRoutineByHashNoFix64(ImageBase, Hash);
	}

	if (Hash == 0)
	{
		return &NtHeaders->OptionalHeader.AddressOfEntryPoint;
	}

	ExportDirectory = nullptr;
	ExportDirectory = PtrAdd(ExportDirectory, ImageBase);
	ExportDirectory = PtrAdd(ExportDirectory, NtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);

	NumberOfNames = ExportDirectory->NumberOfNames;
	AddressOfFuntions = PtrAdd((PULONG_PTR)ImageBase, ExportDirectory->AddressOfFunctions);
	AddressOfNames = PtrAdd((PCSTR *)ImageBase, ExportDirectory->AddressOfNames);
	AddressOfNameOrdinals = PtrAdd((PUSHORT)ImageBase, ExportDirectory->AddressOfNameOrdinals);

	do
	{
		if (!(HashAPI(PtrAdd((PCSTR)(ImageBase), *AddressOfNames)) ^ Hash))
		{
			return AddressOfFuntions + *AddressOfNameOrdinals;
		}

		++AddressOfNameOrdinals;
		++AddressOfNames;
	} while (--NumberOfNames);

	return nullptr;
}

PVOID FASTCALL EATLookupRoutineByHashPNoFix(PVOID ImageBase, ULONG_PTR Hash)
{
	PVOID Pointer;

	Pointer = EATLookupRoutineByHashNoFix(ImageBase, Hash);
	if (Pointer != nullptr)
		Pointer = PtrAdd(ImageBase, *(PULONG)Pointer);

	return Pointer;
}

/************************************************************************
x64 ver
************************************************************************/

PVOID EATLookupRoutineByHashNoFix64(PVOID ImageBase, ULONG_PTR Hash)
{
	PIMAGE_DOS_HEADER           DosHeader;
	PIMAGE_NT_HEADERS64         NtHeaders;
	PIMAGE_EXPORT_DIRECTORY     ExportDirectory;
	ULONG_PTR                   NumberOfNames;
	PULONG                      AddressOfFuntions;
	PULONG                      AddressOfNames;
	PUSHORT                     AddressOfNameOrdinals;

	if (Hash != 0 && !IsValidImage(ImageBase, IMAGE_VALID_EXPORT_ADDRESS_TABLE))
		return nullptr;

	DosHeader = (PIMAGE_DOS_HEADER)ImageBase;
	NtHeaders = (PIMAGE_NT_HEADERS64)((ULONG_PTR)ImageBase + DosHeader->e_lfanew);

	if (Hash == 0)
	{
		return &NtHeaders->OptionalHeader.AddressOfEntryPoint;
	}

	ExportDirectory = nullptr;
	ExportDirectory = PtrAdd(ExportDirectory, ImageBase);
	ExportDirectory = PtrAdd(ExportDirectory, NtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);

	NumberOfNames = ExportDirectory->NumberOfNames;
	AddressOfFuntions = PtrAdd((PULONG)ImageBase, (ULONG_PTR)ExportDirectory->AddressOfFunctions);
	AddressOfNames = PtrAdd((PULONG)ImageBase, (ULONG_PTR)ExportDirectory->AddressOfNames);
	AddressOfNameOrdinals = PtrAdd((PUSHORT)ImageBase, (ULONG_PTR)ExportDirectory->AddressOfNameOrdinals);

	do
	{
		if (!(HashAPI(PtrAdd((PCSTR)(ImageBase), *AddressOfNames)) ^ Hash))
		{
			return AddressOfFuntions + *AddressOfNameOrdinals;
		}

		++AddressOfNameOrdinals;
		++AddressOfNames;
	} while (--NumberOfNames);

	return nullptr;
}

PVOID EATLookupRoutineByHashPNoFix64(PVOID ImageBase, ULONG_PTR Hash)
{
	PVOID Pointer;

	Pointer = EATLookupRoutineByHashNoFix64(ImageBase, Hash);
	if (Pointer != nullptr)
		Pointer = PtrAdd((PVOID)(ULONG_PTR)*(PULONG)Pointer, ImageBase);

	return Pointer;
}

#if CPP_DEFINED

_ML_CPP_HEAD_

PVOID LookupImportTable(PVOID ImageBase, PCSTR DllName, ULONG Hash)
{
	PCSTR TargetDllName;
	PVOID IATVA;

	IATVA = IMAGE_INVALID_VA;
	TargetDllName = nullptr;

	WalkImportTableT(ImageBase,
		WalkIATCallbackM(Data)
	{
		if (DllName != nullptr)
		{
			if (TargetDllName != nullptr)
			{
				if (TargetDllName != Data->DllName)
					return STATUS_NOT_FOUND;
			}
			else if (StrICompareA(Data->DllName, DllName, StrCmp_ToLower) != 0)
			{
				return STATUS_VALIDATE_CONTINUE;
			}
			else
			{
				TargetDllName = Data->DllName;
			}
		}

		if (Data->FunctionName == nullptr)
			return STATUS_SUCCESS;

		if (HashAPI(Data->FunctionName) == Hash)
		{
			IATVA = Data->ThunkData;
			return STATUS_NO_MORE_MATCHES;
		}

		return STATUS_SUCCESS;
	},
		0
		);

	return IATVA;
}

PVOID LookupImportTable(PVOID ImageBase, PCSTR DllName, PCSTR RoutineName)
{
	return LookupImportTable(ImageBase, DllName, HashAPI(RoutineName));
}

PVOID LookupExportTable(PVOID ImageBase, ULONG Hash)
{
	ULONG RoutineRVA = (ULONG)IMAGE_INVALID_RVA;

	WalkExportTableT(ImageBase,
		WalkEATCallbackM(Data)
	{
		if (Data->FunctionName == nullptr)
			return STATUS_NO_MORE_MATCHES;

		if (HashAPI(Data->FunctionName) == Hash)
		{
			RoutineRVA = *Data->AddressOfFunction;
			return STATUS_NO_MORE_MATCHES;
		}

		return STATUS_SUCCESS;
	}, 0
	);

	return RoutineRVA == IMAGE_INVALID_RVA ? IMAGE_INVALID_VA : PtrAdd(ImageBase, RoutineRVA);
}

PVOID LookupExportTable(PVOID ImageBase, PCSTR RoutineName)
{
	return LookupExportTable(ImageBase, HashAPI(RoutineName));
}

_ML_CPP_TAIL_

#endif // cpp

#if ML_USER_MODE

typedef struct LOAD_MEM_DLL_INFO : public TEB_ACTIVE_FRAME
{
	ULONG           Flags;
	PVOID           MappedBase;
	PVOID           MemDllBase;
	SIZE_T          DllBufferSize;
	SIZE_T          ViewSize;
	UNICODE_STRING  Lz32Path;

	union
	{
		HANDLE DllFileHandle;
		HANDLE SectionHandle;
	};

	UNICODE_STRING  MemDllFullPath;

	API_POINTER(NtQueryAttributesFile)  NtQueryAttributesFile;
	API_POINTER(NtOpenFile)             NtOpenFile;
	API_POINTER(NtCreateSection)        NtCreateSection;
	API_POINTER(NtMapViewOfSection)     NtMapViewOfSection;
	API_POINTER(NtClose)                NtClose;
	API_POINTER(NtQuerySection)         NtQuerySection;
	API_POINTER(LdrLoadDll)             LdrLoadDll;

} LOAD_MEM_DLL_INFO, *PLOAD_MEM_DLL_INFO;

PLOAD_MEM_DLL_INFO GetLaodMemDllInfo()
{
	return (PLOAD_MEM_DLL_INFO)FindThreadFrame(LOAD_MEM_DLL_INFO_MAGIC);
}

NTSTATUS
NTAPI
LoadMemoryDll_NtQueryAttributesFile(
	POBJECT_ATTRIBUTES      ObjectAttributes,
	PFILE_BASIC_INFORMATION FileInformation
)
{
	PLOAD_MEM_DLL_INFO MemDllInfo;

	MemDllInfo = GetLaodMemDllInfo();

	if (RtlCompareUnicodeString(ObjectAttributes->ObjectName, &MemDllInfo->MemDllFullPath, TRUE) != 0)
	{
		return MemDllInfo->NtQueryAttributesFile(ObjectAttributes, FileInformation);
	}

	return STATUS_SUCCESS;
}

NTSTATUS
STDCALL
LoadMemoryDll_NtOpenFile(
	PHANDLE             FileHandle,
	ACCESS_MASK         DesiredAccess,
	POBJECT_ATTRIBUTES  ObjectAttributes,
	PIO_STATUS_BLOCK    IoStatusBlock,
	ULONG               ShareAccess,
	ULONG               OpenOptions
)
{
	NTSTATUS            Status;
	PLOAD_MEM_DLL_INFO  MemDllInfo;

	MemDllInfo = GetLaodMemDllInfo();
	if (RtlCompareUnicodeString(ObjectAttributes->ObjectName, &MemDllInfo->MemDllFullPath, TRUE))
	{
		return MemDllInfo->NtOpenFile(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess, OpenOptions);
	}

	ObjectAttributes->ObjectName = &MemDllInfo->Lz32Path;
	Status = MemDllInfo->NtOpenFile(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess, OpenOptions);
	if (NT_SUCCESS(Status))
	{
		MemDllInfo->DllFileHandle = *FileHandle;
	}

	return Status;
}

NTSTATUS
STDCALL
LoadMemoryDll_NtCreateSection(
	PHANDLE             SectionHandle,
	ACCESS_MASK         DesiredAccess,
	POBJECT_ATTRIBUTES  ObjectAttributes,
	PLARGE_INTEGER      MaximumSize,
	ULONG               SectionPageProtection,
	ULONG               AllocationAttributes,
	HANDLE              FileHandle
)
{
	BOOL                IsDllHandle;
	NTSTATUS            Status;
	LARGE_INTEGER       SectionSize;
	PLOAD_MEM_DLL_INFO  MemDllInfo;

	IsDllHandle = FALSE;
	MemDllInfo = GetLaodMemDllInfo();

	if (FileHandle != nullptr)
	{
		if (MemDllInfo->DllFileHandle == FileHandle)
		{
			//            if (MaximumSize == NULL)
			MaximumSize = &SectionSize;

			MaximumSize->QuadPart = MemDllInfo->ViewSize;
			DesiredAccess = SECTION_MAP_READ | SECTION_MAP_WRITE | SECTION_MAP_EXECUTE;
			SectionPageProtection = PAGE_EXECUTE_READWRITE;
			AllocationAttributes = SEC_COMMIT;
			FileHandle = nullptr;
			IsDllHandle = TRUE;
		}
	}

	Status = MemDllInfo->NtCreateSection(
		SectionHandle,
		DesiredAccess,
		ObjectAttributes,
		MaximumSize,
		SectionPageProtection,
		AllocationAttributes,
		FileHandle
	);

	if (!NT_SUCCESS(Status) || !IsDllHandle)
	{
		return Status;
	}

	MemDllInfo->SectionHandle = *SectionHandle;

	return Status;
}

NTSTATUS
STDCALL
LoadMemoryDll_NtMapViewOfSection(
	HANDLE              SectionHandle,
	HANDLE              ProcessHandle,
	PVOID              *BaseAddress,
	ULONG_PTR           ZeroBits,
	SIZE_T              CommitSize,
	PLARGE_INTEGER      SectionOffset,
	PSIZE_T             ViewSize,
	SECTION_INHERIT     InheritDisposition,
	ULONG               AllocationType,
	ULONG               Win32Protect
)
{
	NTSTATUS                    Status;
	PLOAD_MEM_DLL_INFO          MemDllInfo;
	PIMAGE_DOS_HEADER           DosHeader;
	PIMAGE_NT_HEADERS           NtHeader;
	PIMAGE_SECTION_HEADER       SectionHeader;
	PBYTE                       DllBase, ModuleBase;

	MemDllInfo = GetLaodMemDllInfo();

	if (SectionHandle == nullptr)
		goto CALL_ORIGINAL;

	if (MemDllInfo == nullptr)
		goto CALL_ORIGINAL;

	if (SectionHandle != MemDllInfo->SectionHandle)
		goto CALL_ORIGINAL;

	if (SectionOffset != nullptr)
		SectionOffset->QuadPart = 0;

	*ViewSize = MemDllInfo->ViewSize;
	Status = MemDllInfo->NtMapViewOfSection(
		SectionHandle,
		ProcessHandle,
		BaseAddress,
		0,
		0,
		nullptr,
		ViewSize,
		ViewShare,
		0,
		PAGE_EXECUTE_READWRITE
	);
	if (NT_FAILED(Status))
		return Status;

	MemDllInfo->MappedBase = *BaseAddress;

	ModuleBase = (PBYTE)*BaseAddress;
	DllBase = (PBYTE)MemDllInfo->MemDllBase;
	DosHeader = (PIMAGE_DOS_HEADER)DllBase;
	NtHeader = (PIMAGE_NT_HEADERS)((ULONG_PTR)DllBase + DosHeader->e_lfanew);

	if (FLAG_ON(MemDllInfo->Flags, LMD_MAPPED_DLL))
	{
		CopyMemory(ModuleBase, DllBase, MemDllInfo->ViewSize);
	}
	else
	{
		//        DosHeader       = (PIMAGE_DOS_HEADER)DllBase;
		//        NtHeader        = (PIMAGE_NT_HEADERS)((ULONG_PTR)DllBase + DosHeader->e_lfanew);
		SectionHeader = (PIMAGE_SECTION_HEADER)((ULONG_PTR)&NtHeader->OptionalHeader + NtHeader->FileHeader.SizeOfOptionalHeader);
		for (ULONG NumberOfSections = NtHeader->FileHeader.NumberOfSections; NumberOfSections; ++SectionHeader, --NumberOfSections)
		{
			CopyMemory(
				ModuleBase + SectionHeader->VirtualAddress,
				DllBase + SectionHeader->PointerToRawData,
				SectionHeader->SizeOfRawData
			);
		}

		//        CopyMemory(ModuleBase, DllBase, (ULONG_PTR)SectionHeader - (ULONG_PTR)DllBase);
		CopyMemory(ModuleBase, DllBase, MEMORY_PAGE_SIZE);
	}

	if (FLAG_ON(MemDllInfo->Flags, LMD_IGNORE_IAT_DLL_MISSING))
	{
		WalkImportTableT(ModuleBase,
			WalkIATCallbackM(Data)
		{
			return STATUS_VALIDATE_CONTINUE;
		},
			nullptr
			);
	}

	Status = (ULONG_PTR)ModuleBase != NtHeader->OptionalHeader.ImageBase ? STATUS_IMAGE_NOT_AT_BASE : STATUS_SUCCESS;
	return Status;

CALL_ORIGINAL:
	return MemDllInfo->NtMapViewOfSection(SectionHandle, ProcessHandle, BaseAddress, ZeroBits, CommitSize, SectionOffset, ViewSize, InheritDisposition, AllocationType, Win32Protect);
}

NTSTATUS
STDCALL
LoadMemoryDll_NtQuerySection(
	HANDLE                      SectionHandle,
	SECTION_INFORMATION_CLASS   SectionInformationClass,
	PVOID                       SectionInformation,
	SIZE_T                      Length,
	PSIZE_T                     ReturnLength
)
{
	PIMAGE_DOS_HEADER           DosHeader;
	PIMAGE_NT_HEADERS           NtHeaders;
	PIMAGE_OPTIONAL_HEADER      OptionalHeader;
	PLOAD_MEM_DLL_INFO          MemDllInfo;
	SECTION_IMAGE_INFORMATION  *ImageInfo;
	SECTION_BASIC_INFORMATION  *BasicInfo;

	MemDllInfo = GetLaodMemDllInfo();
	if (SectionHandle == nullptr || MemDllInfo->SectionHandle != SectionHandle)
		goto DEFAULT_PROC;

	DosHeader = (PIMAGE_DOS_HEADER)MemDllInfo->MemDllBase;
	NtHeaders = (PIMAGE_NT_HEADERS)((ULONG_PTR)DosHeader + DosHeader->e_lfanew);
	OptionalHeader = &NtHeaders->OptionalHeader;

	switch (SectionInformationClass)
	{
	case SectionBasicInformation:
		BasicInfo = (SECTION_BASIC_INFORMATION *)SectionInformation;
		BasicInfo->BaseAddress = MemDllInfo->MappedBase;
		BasicInfo->AllocationAttributes = 0;
		BasicInfo->MaximumSize.QuadPart = MemDllInfo->ViewSize;
		break;

	case SectionImageInformation:
		if (ReturnLength != nullptr)
			*ReturnLength = sizeof(*ImageInfo);

		if (Length < sizeof(*ImageInfo))
			return STATUS_BUFFER_TOO_SMALL;

		if (SectionInformation == nullptr)
			break;

		ImageInfo = (SECTION_IMAGE_INFORMATION *)SectionInformation;
		ImageInfo->TransferAddress = (PVOID)((ULONG_PTR)DosHeader + OptionalHeader->AddressOfEntryPoint);
		ImageInfo->ZeroBits = 0;
		ImageInfo->MaximumStackSize = OptionalHeader->SizeOfStackReserve;
		ImageInfo->CommittedStackSize = OptionalHeader->SizeOfStackCommit;
		ImageInfo->SubSystemType = OptionalHeader->Subsystem;
		ImageInfo->SubSystemMinorVersion = OptionalHeader->MinorSubsystemVersion;
		ImageInfo->SubSystemMajorVersion = OptionalHeader->MajorSubsystemVersion;
		ImageInfo->OperatingSystemVersion = 0;
		ImageInfo->ImageCharacteristics = NtHeaders->FileHeader.Characteristics;
		ImageInfo->DllCharacteristics = OptionalHeader->DllCharacteristics;
		ImageInfo->Machine = NtHeaders->FileHeader.Machine;
		ImageInfo->ImageContainsCode = 0; // OptionalHeader->SizeOfCode;
		ImageInfo->LoaderFlags = OptionalHeader->LoaderFlags;
		ImageInfo->ImageFileSize = (TYPE_OF(ImageInfo->ImageFileSize))MemDllInfo->DllBufferSize;
		ImageInfo->CheckSum = (TYPE_OF(ImageInfo->CheckSum))OptionalHeader->CheckSum;
		break;

	case SectionRelocationInformation:
		if (SectionInformation != nullptr)
			*(PULONG_PTR)SectionInformation = (ULONG_PTR)MemDllInfo->MappedBase - (ULONG_PTR)OptionalHeader->ImageBase;

		if (ReturnLength != nullptr)
			*ReturnLength = sizeof(ULONG_PTR);

		break;

	default:
		goto DEFAULT_PROC;
	}

	return STATUS_SUCCESS;

DEFAULT_PROC:
	return MemDllInfo->NtQuerySection(SectionHandle, SectionInformationClass, SectionInformation, Length, ReturnLength);
}

NTSTATUS
STDCALL
LoadMemoryDll_NtClose(
	HANDLE Handle
)
{
	PLOAD_MEM_DLL_INFO MemDllInfo;

	MemDllInfo = GetLaodMemDllInfo();
	if (Handle != nullptr)
	{
		if (MemDllInfo->DllFileHandle == Handle)
		{
			MemDllInfo->DllFileHandle = nullptr;
		}
		else if (MemDllInfo->SectionHandle == Handle)
		{
			MemDllInfo->SectionHandle = nullptr;
		}
	}

	return MemDllInfo->NtClose(Handle);
}

NTSTATUS
LoadDllFromMemory(
	PVOID           DllBuffer,
	ULONG           DllBufferSize,
	PUNICODE_STRING ModuleFileName,
	PVOID*          ModuleHandle,
	ULONG           Flags /* = 0 */
)
{
	NTSTATUS            Status;
	ULONG               Length;
	PVOID               ModuleBase, ShadowNtdll;
	LDR_MODULE          *Ntdll;
	LOAD_MEM_DLL_INFO   MemDllInfo;
	PIMAGE_DOS_HEADER   DosHeader;
	PIMAGE_NT_HEADERS   NtHeader;
	WCHAR               Lz32DosPath[MAX_NTPATH];

	API_POINTER(NtQueryAttributesFile)  NtQueryAttributesFile;
	API_POINTER(NtOpenFile)             NtOpenFile;
	API_POINTER(NtCreateSection)        NtCreateSection;
	API_POINTER(NtMapViewOfSection)     NtMapViewOfSection;
	API_POINTER(NtClose)                NtClose;
	API_POINTER(NtQuerySection)         NtQuerySection;
	API_POINTER(LdrLoadDll)             LdrLoadDll;

	Ntdll = GetNtdllLdrModule();

	Status = LoadPeImage(Ntdll->FullDllName.Buffer, &ShadowNtdll, Ntdll->DllBase, LOAD_PE_IGNORE_IAT);
	if (NT_FAILED(Status))
	{
		return Status;
	}

	*(PVOID *)&NtQueryAttributesFile = LookupExportTable(ShadowNtdll, NTDLL_NtQueryAttributesFile);
	*(PVOID *)&NtOpenFile = LookupExportTable(ShadowNtdll, NTDLL_NtOpenFile);
	*(PVOID *)&NtCreateSection = LookupExportTable(ShadowNtdll, NTDLL_NtCreateSection);
	*(PVOID *)&NtMapViewOfSection = LookupExportTable(ShadowNtdll, NTDLL_NtMapViewOfSection);
	*(PVOID *)&NtClose = LookupExportTable(ShadowNtdll, NTDLL_NtClose);
	*(PVOID *)&NtQuerySection = LookupExportTable(ShadowNtdll, NTDLL_NtQuerySection);
	*(PVOID *)&LdrLoadDll = LookupExportTable(ShadowNtdll, NTDLL_LdrLoadDll);

	ZeroMemory(&MemDllInfo, sizeof(MemDllInfo));
	MemDllInfo.Flags = LOAD_MEM_DLL_INFO_MAGIC;
	RtlPushFrame(&MemDllInfo);

	Status = STATUS_UNSUCCESSFUL;
	LOOP_ONCE
	{
		if (!RtlDosPathNameToNtPathName_U(ModuleFileName->Buffer, &MemDllInfo.MemDllFullPath, nullptr, nullptr))
		break;

		Length = Nt_GetSystemDirectory(Lz32DosPath, countof(Lz32DosPath));

		*(PULONG64)(Lz32DosPath + Length) = TAG4W('lz32');
		*(PULONG64)(Lz32DosPath + Length + 4) = TAG4W('.dll');
		Lz32DosPath[Length + 8] = 0;
		if (!RtlDosPathNameToNtPathName_U(Lz32DosPath, &MemDllInfo.Lz32Path, nullptr, nullptr))
			break;

		MemDllInfo.Flags = Flags;
		MemDllInfo.MemDllBase = DllBuffer;
		MemDllInfo.DllBufferSize = DllBufferSize;
		DosHeader = (PIMAGE_DOS_HEADER)DllBuffer;
		NtHeader = (PIMAGE_NT_HEADERS)((ULONG_PTR)DllBuffer + DosHeader->e_lfanew);
		MemDllInfo.ViewSize = NtHeader->OptionalHeader.SizeOfImage;

		Mp::PATCH_MEMORY_DATA p[] =
		{
			Mp::FunctionJumpVa(NtQueryAttributesFile, LoadMemoryDll_NtQueryAttributesFile, &MemDllInfo.NtQueryAttributesFile),
			Mp::FunctionJumpVa(NtOpenFile, LoadMemoryDll_NtOpenFile, &MemDllInfo.NtOpenFile),
			Mp::FunctionJumpVa(NtCreateSection, LoadMemoryDll_NtCreateSection, &MemDllInfo.NtCreateSection),
			Mp::FunctionJumpVa(NtMapViewOfSection, LoadMemoryDll_NtMapViewOfSection, &MemDllInfo.NtMapViewOfSection),
			Mp::FunctionJumpVa(NtClose, LoadMemoryDll_NtClose, &MemDllInfo.NtClose),
			Mp::FunctionJumpVa(NtQuerySection, LoadMemoryDll_NtQuerySection, &MemDllInfo.NtQuerySection),
		};

		Mp::PatchMemory(p, countof(p));
		Status = LdrLoadDll(nullptr, 0, ModuleFileName, &ModuleBase);
		Mp::RestoreMemory(p, countof(p));
		if (NT_FAILED(Status) && FLAG_OFF(Flags, LMD_MAPPED_DLL))
		{
			//            UnmapDllSection(MemDllInfo.MemDllBase);
			break;
		}

		if (ModuleHandle != nullptr)
			*ModuleHandle = (HANDLE)ModuleBase;
	}

	RtlPopFrame(&MemDllInfo);
	RtlFreeUnicodeString(&MemDllInfo.MemDllFullPath);
	RtlFreeUnicodeString(&MemDllInfo.Lz32Path);

	UnloadPeImage(ShadowNtdll);

	return Status;
}

#endif // ML_USER_MODE

_ML_C_TAIL_

#pragma warning(default:4750)

#if ML_SUPPORT_LAMBDA


ML_NAMESPACE
ML_NAMESPACE_BEGIN(DebugHelper);

static API_POINTER(RtlAllocateHeap)    StubRtlAllocateHeap;
static API_POINTER(RtlReAllocateHeap)  StubRtlReAllocateHeap;
static API_POINTER(RtlFreeHeap)        StubRtlFreeHeap;
static API_POINTER(RtlSizeHeap)        StubRtlSizeHeap;

BOOL IsCallerSelf(PVOID ReturnAddress)
{
	LDR_MODULE* Module;

	Module = FindLdrModuleByHandle(&__ImageBase);

	return IN_RANGE((ULONG_PTR)Module->DllBase, (ULONG_PTR)ReturnAddress, (ULONG_PTR)PtrAdd(Module->DllBase, Module->SizeOfImage));
}

static PVOID NTAPI HCH_RtlAllocateHeap(HANDLE HeapBase, ULONG Flags, SIZE_T Bytes)
{
	ULONG_PTR Size;
	PVOID Memory, Second;

	if (!IsCallerSelf(_ReturnAddress()))
		return StubRtlAllocateHeap(HeapBase, Flags, Bytes);

	Size = ROUND_UP(Bytes, MEMORY_PAGE_SIZE) + MEMORY_PAGE_SIZE * 2;

	Memory = NULL;
	Nt_AllocateMemory(NtCurrentProcess(), &Memory, Size, PAGE_EXECUTE_READWRITE, MEM_RESERVE);
	Nt_AllocateMemory(NtCurrentProcess(), &Memory, Size - MEMORY_PAGE_SIZE, PAGE_EXECUTE_READWRITE, MEM_COMMIT);

	Second = PtrAdd(Memory, Size - MEMORY_PAGE_SIZE);
	Nt_AllocateMemory(NtCurrentProcess(), &Second, MEMORY_PAGE_SIZE, PAGE_READONLY, MEM_COMMIT);

	Memory = PtrSub(Second, Bytes);
	*(PULONG_PTR)PtrSub(Memory, sizeof(PVOID)) = Bytes;

	return Memory;
}

static BOOL NTAPI HCH_RtlFreeHeap(HANDLE HeapBase, ULONG Flags, LPVOID Memory)
{
	if (!IsCallerSelf(_ReturnAddress()))
		return StubRtlFreeHeap(HeapBase, Flags, Memory);

	Memory = (PVOID)ROUND_DOWN((ULONG_PTR)Memory, MEMORY_PAGE_SIZE);
	Nt_FreeMemory(NtCurrentProcess(), PtrSub(Memory, MEMORY_PAGE_SIZE));
	return TRUE;
}

static PVOID NTAPI HCH_RtlReAllocateHeap(HANDLE HeapBase, ULONG Flags, PVOID Memory, SIZE_T Bytes)
{
	PVOID NewMemory;

	if (!IsCallerSelf(_ReturnAddress()))
		return StubRtlReAllocateHeap(HeapBase, Flags, Memory, Bytes);

	NewMemory = HCH_RtlAllocateHeap(HeapBase, Flags, Bytes);

	Bytes = MY_MIN(((PULONG_PTR)Memory)[-1], Bytes);
	CopyMemory(NewMemory, Memory, Bytes);

	HCH_RtlFreeHeap(HeapBase, Flags, Memory);

	return NewMemory;
}

static ULONG_PTR NTAPI HCH_RtlSizeHeap(HANDLE HeapBase, ULONG Flags, PVOID Memory)
{
	if (!IsCallerSelf(_ReturnAddress()))
		return StubRtlSizeHeap(HeapBase, Flags, Memory);

	return ((PULONG_PTR)Memory)[-1];
}

ML_NAMESPACE_END_(DebugHelper);
ML_NAMESPACE_END; // ml

NTSTATUS EnableHeapCorruptionHelper()
{
	Mp::PATCH_MEMORY_DATA p[] =
	{
		Mp::FunctionJumpVa(RtlAllocateHeap, ml::DebugHelper::HCH_RtlAllocateHeap, &ml::DebugHelper::StubRtlAllocateHeap),
		Mp::FunctionJumpVa(RtlReAllocateHeap, ml::DebugHelper::HCH_RtlReAllocateHeap, &ml::DebugHelper::StubRtlReAllocateHeap),
		Mp::FunctionJumpVa(RtlFreeHeap, ml::DebugHelper::HCH_RtlFreeHeap, &ml::DebugHelper::StubRtlFreeHeap),
		Mp::FunctionJumpVa(RtlSizeHeap, ml::DebugHelper::HCH_RtlSizeHeap, &ml::DebugHelper::StubRtlSizeHeap),
	};

	return Mp::PatchMemory(p, countof(p));
}

NTSTATUS DisableHeapCorruptionHelper()
{
	Mp::RestoreMemory(ml::DebugHelper::StubRtlAllocateHeap);
	Mp::RestoreMemory(ml::DebugHelper::StubRtlReAllocateHeap);
	Mp::RestoreMemory(ml::DebugHelper::StubRtlFreeHeap);
	Mp::RestoreMemory(ml::DebugHelper::StubRtlSizeHeap);

	return 0;
}

#endif // ML_SUPPORT_LAMBDA

// class

#if CPP_DEFINED


#pragma warning(disable:4750 4995)

#if CPP_DEFINED

NtFileDisk::NtFileDisk()
{
	m_FileHandle = nullptr;
}

NtFileDisk::NtFileDisk(const NtFileDisk &file)
{
	*this = file;
}

NtFileDisk::operator HANDLE() const
{
	return m_FileHandle;
}

NtFileDisk& NtFileDisk::operator=(const NtFileDisk &file)
{
	if (this == &file)
		return *this;

	Close();

	ZwDuplicateObject(
		NtCurrentProcess(),
		file,
		NtCurrentProcess(),
		&m_FileHandle,
		0,
		0,
		DUPLICATE_SAME_ACCESS
	);

	return *this;
}

NtFileDisk& NtFileDisk::operator=(HANDLE Handle)
{
	if (m_FileHandle == Handle)
		return *this;

	Close();

	ZwDuplicateObject(
		NtCurrentProcess(),
		Handle,
		NtCurrentProcess(),
		&m_FileHandle,
		0,
		0,
		DUPLICATE_SAME_ACCESS
	);

	return *this;
}

NTSTATUS NtFileDisk::QueryFullNtPath(PCWSTR FileName, PUNICODE_STRING NtFilePath, ULONG_PTR Flags)
{
#if ML_KERNEL_MODE

	return RtlCreateUnicodeString(NtFilePath, FileName) ? STATUS_SUCCESS : STATUS_OBJECT_PATH_NOT_FOUND;

#else // r3

	WCHAR ExpandBuffer[MAX_NTPATH];
	UNICODE_STRING Path, Expand;

	static UNICODE_STRING DosDevicesPrefix = RTL_CONSTANT_STRING(L"\\??\\");

	if (FLAG_ON(Flags, NFD_EXPAND_ENVVAR))
	{
		RtlInitUnicodeString(&Path, FileName);

		RtlInitEmptyString(&Expand, ExpandBuffer, sizeof(ExpandBuffer));
		RtlExpandEnvironmentStrings_U(nullptr, &Path, &Expand, nullptr);
	}
	else
	{
		RtlInitUnicodeString(&Expand, (PWSTR)FileName);
	}

	if (
		Expand.Length >= DosDevicesPrefix.Length &&
		RtlCompareMemory(Expand.Buffer, DosDevicesPrefix.Buffer, DosDevicesPrefix.Length) == DosDevicesPrefix.Length
		)
	{
		return RtlDuplicateUnicodeString(RTL_DUPSTR_ADD_NULL, &Expand, NtFilePath);
	}

	return RtlDosPathNameToNtPathName_U(Expand.Buffer, NtFilePath, nullptr, nullptr) ? STATUS_SUCCESS : STATUS_OBJECT_PATH_NOT_FOUND;

#endif // MY_NT_DDK
}

NTSTATUS
NtFileDisk::
CreateFileInternalWithFullPath(
	PHANDLE         FileHandle,
	PUNICODE_STRING FileName,
	ULONG_PTR       Flags,
	ULONG_PTR       ShareMode,
	ULONG_PTR       Access,
	ULONG_PTR       CreationDisposition,
	ULONG_PTR       Attributes,
	ULONG_PTR       CreateOptions /* = 0 */
)
{
	NTSTATUS            Status;
	OBJECT_ATTRIBUTES   ObjectAttributes;
	IO_STATUS_BLOCK     IoStatus;

	if (!FLAG_ON(Attributes, 0x40000000))
	{
		CreateOptions |= FILE_SYNCHRONOUS_IO_NONALERT;
	}

	CLEAR_FLAG(Attributes, 0x40000000);

	InitializeObjectAttributes(&ObjectAttributes, FileName, OBJ_CASE_INSENSITIVE, nullptr, nullptr);

	API_POINTER(ZwCreateFile) XCreateFile;

#if ML_KERNEL_MODE

	XCreateFile = ZwCreateFile;

#else

	XCreateFile = NtCreateFile;

#endif

	Status = XCreateFile(
		FileHandle,
		Access | SYNCHRONIZE | FILE_READ_ATTRIBUTES,
		&ObjectAttributes,
		&IoStatus,
		nullptr,
		Attributes,
		ShareMode,
		CreationDisposition,
		CreateOptions,
		nullptr,
		0
	);

	return Status;
}

NTSTATUS
NtFileDisk::
CreateFileInternal(
	PHANDLE     FileHandle,
	LPCWSTR     FileName,
	ULONG_PTR   Flags,
	ULONG_PTR   ShareMode,
	ULONG_PTR   Access,
	ULONG_PTR   CreationDisposition,
	ULONG_PTR   Attributes,
	ULONG_PTR   CreateOptions /* = 0 */
)
{
	NTSTATUS            Status;
	UNICODE_STRING      NtFilePath;

	if (FLAG_ON(Flags, NFD_NOT_RESOLVE_PATH))
	{
		RtlInitUnicodeString(&NtFilePath, FileName);
	}
	else
	{
		Status = QueryFullNtPath(FileName, &NtFilePath, Flags);
		if (!NT_SUCCESS(Status))
			return Status;
	}

	Status = CreateFileInternalWithFullPath(
		FileHandle,
		&NtFilePath,
		Flags,
		ShareMode,
		Access,
		CreationDisposition,
		Attributes,
		CreateOptions
	);

	if (!FLAG_ON(Flags, NFD_NOT_RESOLVE_PATH))
		RtlFreeUnicodeString(&NtFilePath);

	return Status;
}

NTSTATUS NtFileDisk::GetSizeInternal(HANDLE FileHandle, PLARGE_INTEGER pFileSize)
{
	NTSTATUS Status;
	IO_STATUS_BLOCK IoStatus;
	FILE_STANDARD_INFORMATION FileStandard;
	API_POINTER(ZwQueryInformationFile) XQueryInformationFile;

#if ML_KERNEL_MODE

	XQueryInformationFile = ZwQueryInformationFile;

#else

	XQueryInformationFile = NtQueryInformationFile;

#endif

	if (pFileSize == nullptr)
		return STATUS_INVALID_PARAMETER;

	pFileSize->QuadPart = 0;

	Status = XQueryInformationFile(
		FileHandle,
		&IoStatus,
		&FileStandard,
		sizeof(FileStandard),
		FileStandardInformation
	);

	if (!NT_SUCCESS(Status))
		return Status;

	pFileSize->QuadPart = FileStandard.EndOfFile.QuadPart;

	return Status;
}

NTSTATUS
NtFileDisk::
SeekInternal(
	HANDLE          FileHandle,
	LARGE_INTEGER   Offset,
	ULONG_PTR           MoveMethod,
	PLARGE_INTEGER  NewPosition /* = NULL */
)
{
	NTSTATUS                    Status;
	IO_STATUS_BLOCK             IoStatus;
	FILE_POSITION_INFORMATION   FilePosition;
	FILE_STANDARD_INFORMATION   FileStandard;
	API_POINTER(ZwQueryInformationFile) XQueryInformationFile;
	API_POINTER(ZwSetInformationFile)   XSetInformationFile;

#if ML_KERNEL_MODE

	XQueryInformationFile = ZwQueryInformationFile;
	XSetInformationFile = ZwSetInformationFile;

#else

	XQueryInformationFile = NtQueryInformationFile;
	XSetInformationFile = NtSetInformationFile;

#endif


	switch (MoveMethod)
	{
	case FILE_CURRENT:
		XQueryInformationFile(
			FileHandle,
			&IoStatus,
			&FilePosition,
			sizeof(FilePosition),
			FilePositionInformation);

		FilePosition.CurrentByteOffset.QuadPart += Offset.QuadPart;
		break;

	case FILE_END:
		XQueryInformationFile(
			FileHandle,
			&IoStatus,
			&FileStandard,
			sizeof(FileStandard),
			FileStandardInformation);

		FilePosition.CurrentByteOffset.QuadPart = FileStandard.EndOfFile.QuadPart + Offset.QuadPart;
		break;

	case FILE_BEGIN:
		FilePosition.CurrentByteOffset.QuadPart = Offset.QuadPart;
		break;

	default:
		return STATUS_INVALID_PARAMETER_2;
	}

	if (FilePosition.CurrentByteOffset.QuadPart < 0)
		return STATUS_INVALID_PARAMETER_1;

	Status = XSetInformationFile(
		FileHandle,
		&IoStatus,
		&FilePosition,
		sizeof(FILE_POSITION_INFORMATION),
		FilePositionInformation);

	if (!NT_SUCCESS(Status))
		return Status;

	if (NewPosition != nullptr)
	{
		NewPosition->QuadPart = FilePosition.CurrentByteOffset.QuadPart;
	}

	return Status;
}

NTSTATUS
NtFileDisk::
ReadInternal(
	HANDLE          FileHandle,
	PVOID           Buffer,
	ULONG_PTR           Size,
	PLARGE_INTEGER  BytesRead /* = NULL */,
	PLARGE_INTEGER  Offset     /* = NULL */
)
{
	NTSTATUS        Status;
	IO_STATUS_BLOCK IoStatus;
	API_POINTER(ZwReadFile) XReadFile;

	if (BytesRead != nullptr)
		BytesRead->QuadPart = 0;

#if ML_KERNEL_MODE

	XReadFile = ZwReadFile;

#else

	XReadFile = NtReadFile;

#endif

	Status = XReadFile(
		FileHandle,
		nullptr,
		nullptr,
		nullptr,
		&IoStatus,
		Buffer,
		Size,
		Offset,
		nullptr
	);
	/*
	if (Status == STATUS_PENDING)
	{
	Status = NtWaitForSingleObject(FileHandle, FALSE, NULL);
	if (NT_SUCCESS(Status))
	Status = IoStatus.Status;
	}
	*/
	if (!NT_SUCCESS(Status) || Status == STATUS_PENDING)
		return Status;

	if (BytesRead != nullptr)
		BytesRead->QuadPart = IoStatus.Information;

	return Status;
}

NTSTATUS
NtFileDisk::
WriteInternal(
	HANDLE          FileHandle,
	PVOID           Buffer,
	ULONG_PTR           Size,
	PLARGE_INTEGER  pBytesWritten  /* = NULL */,
	PLARGE_INTEGER  Offset         /* = NULL */
)
{
	NTSTATUS        Status;
	IO_STATUS_BLOCK IoStatus;

	API_POINTER(ZwWriteFile)    XWriteFile;

	if (pBytesWritten != nullptr)
		pBytesWritten->QuadPart = 0;

#if ML_KERNEL_MODE

	XWriteFile = ZwWriteFile;

#else

	XWriteFile = NtWriteFile;

#endif

	Status = XWriteFile(
		FileHandle,
		nullptr,
		nullptr,
		nullptr,
		&IoStatus,
		Buffer,
		Size,
		Offset,
		nullptr
	);
	/*
	if (Status == STATUS_PENDING)
	{
	Status = NtWaitForSingleObject(FileHandle, FALSE, NULL);
	if (NT_SUCCESS(Status))
	Status = IoStatus.Status;
	}
	*/
	if (!NT_SUCCESS(Status))
		return Status;

	if (pBytesWritten != nullptr)
		pBytesWritten->QuadPart = IoStatus.Information;

	return Status;
}

NTSTATUS NtFileDisk::DeleteInternal(HANDLE FileHandle)
{
	NTSTATUS Status;
	IO_STATUS_BLOCK  IoStatus;
	FILE_DISPOSITION_INFORMATION FileInformation;
	API_POINTER(ZwSetInformationFile)   XSetInformationFile;

#if ML_KERNEL_MODE

	XSetInformationFile = ZwSetInformationFile;

#else

	XSetInformationFile = NtSetInformationFile;

#endif

	FileInformation.DeleteFile = TRUE;
	Status = XSetInformationFile(
		FileHandle,
		&IoStatus,
		&FileInformation,
		sizeof(FileInformation),
		FileDispositionInformation);

	return Status;
}

NTSTATUS NtFileDisk::SetEndOfFileInternal(HANDLE FileHandle, LARGE_INTEGER EndPosition)
{
	NTSTATUS        Status;
	IO_STATUS_BLOCK IoStatus;

	union
	{
		FILE_END_OF_FILE_INFORMATION    EndOfFile;
		FILE_ALLOCATION_INFORMATION     FileAllocation;
	};

	API_POINTER(ZwSetInformationFile)   XSetInformationFile;

#if ML_KERNEL_MODE

	XSetInformationFile = ZwSetInformationFile;

#else

	XSetInformationFile = NtSetInformationFile;

#endif


	EndOfFile.EndOfFile.QuadPart = EndPosition.QuadPart;

	Status = XSetInformationFile(
		FileHandle,
		&IoStatus,
		&EndOfFile,
		sizeof(EndOfFile),
		FileEndOfFileInformation
	);

	if (!NT_SUCCESS(Status))
		return Status;

	FileAllocation.AllocationSize.QuadPart = EndPosition.QuadPart;
	Status = XSetInformationFile(
		FileHandle,
		&IoStatus,
		&FileAllocation,
		sizeof(FileAllocation),
		FileAllocationInformation
	);

	return Status;
}

NTSTATUS
NtFileDisk::
QuerySymbolicTargetInternal(
	HANDLE                  FileHandle,
	PREPARSE_DATA_BUFFER    ReparseBuffer,
	ULONG_PTR               BufferSize,
	PULONG_PTR              BytesReturned /* = NULL */
)
{
	NTSTATUS        Status;
	IO_STATUS_BLOCK IoStatus;

	API_POINTER(ZwDeviceIoControlFile) IoControlRoutine;

#if ML_KERNEL_MODE

	IoControlRoutine = ZwFsControlFile;

#else

	IoControlRoutine = NtFsControlFile;

#endif

	Status = IoControlRoutine(
		FileHandle,
		nullptr,
		nullptr,
		nullptr,
		&IoStatus,
		FSCTL_GET_REPARSE_POINT,
		nullptr,
		0,
		ReparseBuffer,
		(ULONG_PTR)BufferSize
	);

	if (BytesReturned != nullptr)
		*BytesReturned = IoStatus.Information;

	return Status;
}

NTSTATUS
NtFileDisk::
QuerySymbolicTargetInternal(
	HANDLE                  FileHandle,
	PWSTR                   PrintNameBuffer,
	PULONG_PTR              PrintNameBufferCount,
	PWSTR                   SubstituteNameBuffer       /* = NULL */,
	PULONG_PTR              SubstituteNameBufferCount  /* = NULL */
)
{
	PWSTR                   PrintName, SubstituteName;
	ULONG_PTR               PrintLength, SubstituteLength;
	ULONG_PTR               ReparseBufferSize, Returned;
	NTSTATUS                Status;
	PREPARSE_DATA_BUFFER    ReparseBuffer;

	ReparseBufferSize = sizeof(*ReparseBuffer) + MAX_NTPATH * sizeof(WCHAR);
	ReparseBuffer = (PREPARSE_DATA_BUFFER)AllocStack(ReparseBufferSize);

	LOOP_FOREVER
	{
		Status = QuerySymbolicTargetInternal(FileHandle, ReparseBuffer, ReparseBufferSize, &Returned);
		if (NT_SUCCESS(Status))
			break;
		if (Status != STATUS_BUFFER_TOO_SMALL)
			return Status;

		ReparseBufferSize += MAX_NTPATH * sizeof(WCHAR);
		ReparseBuffer = (PREPARSE_DATA_BUFFER)AllocStack(ReparseBufferSize);
	}

		if (PrintNameBuffer == nullptr || PrintNameBufferCount == nullptr)
		{
			if (PrintNameBufferCount != nullptr)
			{
				*PrintNameBufferCount = ReparseBuffer->SymbolicLinkReparseBuffer.PrintNameLength / sizeof(WCHAR);
			}
		}
		else
		{
			PrintName = ReparseBuffer->SymbolicLinkReparseBuffer.PathBuffer + ReparseBuffer->SymbolicLinkReparseBuffer.PrintNameOffset / sizeof(WCHAR);
			PrintLength = ReparseBuffer->SymbolicLinkReparseBuffer.PrintNameLength;

			PrintLength = MY_MIN(PrintLength, *PrintNameBufferCount);
			CopyMemory(PrintNameBuffer, PrintName, PrintLength);
			PrintLength /= sizeof(WCHAR);
			if (PrintLength < *PrintNameBufferCount)
				PrintNameBuffer[PrintLength] = 0;

			*PrintNameBufferCount = PrintLength;
		}

	if (SubstituteNameBuffer == nullptr || SubstituteNameBufferCount == nullptr)
	{
		if (SubstituteNameBufferCount != nullptr)
		{
			*SubstituteNameBufferCount = ReparseBuffer->SymbolicLinkReparseBuffer.SubstituteNameLength / sizeof(WCHAR);
		}
	}
	else
	{
		SubstituteName = ReparseBuffer->SymbolicLinkReparseBuffer.PathBuffer + ReparseBuffer->SymbolicLinkReparseBuffer.SubstituteNameOffset / sizeof(WCHAR);
		SubstituteLength = ReparseBuffer->SymbolicLinkReparseBuffer.SubstituteNameLength;

		SubstituteLength = MY_MIN(SubstituteLength, *SubstituteNameBufferCount);
		CopyMemory(SubstituteNameBuffer, SubstituteName, SubstituteLength);
		SubstituteLength /= sizeof(WCHAR);
		if (SubstituteLength < *SubstituteNameBufferCount)
			SubstituteNameBuffer[SubstituteLength] = 0;

		*SubstituteNameBufferCount = SubstituteLength;
	}

	return Status;
}

NTSTATUS NtFileDisk::Close()
{
	NTSTATUS Status;

	if (m_FileHandle == nullptr)
		return STATUS_SUCCESS;

#if ML_KERNEL_MODE

	Status = ZwClose(m_FileHandle);

#else

	Status = NtClose(m_FileHandle);

#endif

	if (!NT_SUCCESS(Status))
		return Status;

	m_FileHandle = nullptr;

	return Status;
}

NTSTATUS
NtFileDisk::
Create(
	LPCWSTR FileName,
	ULONG_PTR   Flags               /* = 0 */,
	ULONG_PTR   ShareMode           /* = FILE_SHARE_READ */,
	ULONG_PTR   Access              /* = GENERIC_WRITE */,
	ULONG_PTR   CreationDisposition /* = FILE_OVERWRITE_IF */,
	ULONG_PTR   Attributes          /* = FILE_ATTRIBUTE_NORMAL */,
	ULONG_PTR   CreateOptions       /* = 0 */
)
{
	NTSTATUS Status;

	Close();

	Status = CreateFileInternal(&m_FileHandle, FileName, Flags, ShareMode, Access, CreationDisposition, Attributes, CreateOptions);
	if (!NT_SUCCESS(Status))
		return Status;

	if (CreationDisposition != FILE_OVERWRITE_IF)
	{
		Status = GetSizeInternal(m_FileHandle, &m_FileSize);
		if (!NT_SUCCESS(Status))
		{
			Close();
			return Status;
		}
	}
	else
	{
		m_FileSize.QuadPart = 0;
	}

	m_Position.QuadPart = 0;

	return Status;
}

NTSTATUS
NtFileDisk::
OpenDevice(
	PCWSTR      FileName,
	ULONG_PTR   Flags               /* = 0 */,
	ULONG_PTR   ShareMode           /* = FILE_SHARE_READ */,
	ULONG_PTR   Access              /* = GENERIC_READ */,
	ULONG_PTR   CreationDisposition /* = FILE_OPEN */,
	ULONG_PTR   Attributes          /* = FILE_ATTRIBUTE_NORMAL */,
	ULONG_PTR   CreateOptions       /* = 0 */
)
{
	NTSTATUS        Status;
	UNICODE_STRING  DeviceName;

	Close();

	RtlInitUnicodeString(&DeviceName, FileName);

	Status = CreateFileInternalWithFullPath(&m_FileHandle, &DeviceName, Flags, ShareMode, Access, CreationDisposition, Attributes, CreateOptions);
	if (!NT_SUCCESS(Status))
		return Status;

	return Status;
}

NTSTATUS
NtFileDisk::
DeviceIoControl(
	ULONG_PTR   IoControlCode,
	PVOID       InputBuffer,
	ULONG_PTR   InputBufferLength,
	PVOID       OutputBuffer,
	ULONG_PTR   OutputBufferLength,
	PULONG_PTR  BytesReturned /* = NULL */
)
{
	NTSTATUS        Status;
	IO_STATUS_BLOCK IoStatus;
	TYPE_OF(ZwDeviceIoControlFile)* IoControlRoutine;

#if ML_KERNEL_MODE

	IoControlRoutine = DEVICE_TYPE_FROM_CTL_CODE(IoControlCode) == FILE_DEVICE_FILE_SYSTEM ? ZwFsControlFile : ZwDeviceIoControlFile;

#else

	IoControlRoutine = DEVICE_TYPE_FROM_CTL_CODE(IoControlCode) == FILE_DEVICE_FILE_SYSTEM ? NtFsControlFile : NtDeviceIoControlFile;

#endif

	Status = IoControlRoutine(
		m_FileHandle,
		nullptr,
		nullptr,
		nullptr,
		&IoStatus,
		IoControlCode,
		InputBuffer,
		InputBufferLength,
		OutputBuffer,
		OutputBufferLength
	);
	if (!NT_SUCCESS(Status))
		return Status;

	if (BytesReturned != nullptr)
		*BytesReturned = IoStatus.Information;

	return Status;
}

NTSTATUS NtFileDisk::GetSize(PLARGE_INTEGER FileSize)
{
	if (FileSize == nullptr)
		return STATUS_INVALID_PARAMETER;

	FileSize->QuadPart = m_FileSize.QuadPart;

	return STATUS_SUCCESS;
}

NTSTATUS NtFileDisk::GetPosition(PLARGE_INTEGER Position)
{
	if (Position == nullptr)
		return STATUS_INVALID_PARAMETER;

	Position->QuadPart = m_Position.QuadPart;

	return STATUS_SUCCESS;
}

NTSTATUS
NtFileDisk::
Seek(
	LARGE_INTEGER   Offset,
	ULONG_PTR       MoveMethod,
	PLARGE_INTEGER  NewPosition /* = NULL */
)
{
#if 1
	LARGE_INTEGER NewOffset = m_Position;

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
	{
		return STATUS_INVALID_PARAMETER_1;
	}

	m_Position = NewOffset;
	if (m_Position.QuadPart > m_FileSize.QuadPart)
		m_FileSize.QuadPart = m_Position.QuadPart;

	if (NewPosition != nullptr)
		NewPosition->QuadPart = m_Position.QuadPart;

	return STATUS_SUCCESS;
#else
	NTSTATUS Status;
	LARGE_INTEGER CurrentByteOffset;

	Status = SeekInternal(m_FileHandle, Offset, MoveMethod, &CurrentByteOffset);

	if (!NT_SUCCESS(Status))
		return Status;

	m_Position.QuadPart = CurrentByteOffset.QuadPart;
	if (m_Position.QuadPart > m_FileSize.QuadPart)
		m_FileSize.QuadPart = m_Position.QuadPart;

	if (NewPosition != nullptr)
		NewPosition->QuadPart = CurrentByteOffset.QuadPart;

	return Status;
#endif
}

NTSTATUS
NtFileDisk::
Read(
	PVOID           Buffer,
	ULONG_PTR       Size        /* = 0 */,
	PLARGE_INTEGER  BytesRead   /* = NULL */
)
{
	NTSTATUS        Status;
	LARGE_INTEGER   LocalBytesRead;

	if (Size == 0)
		Size = m_FileSize.LowPart;

	Status = ReadInternal(m_FileHandle, Buffer, Size, &LocalBytesRead, &m_Position);
	if (!NT_SUCCESS(Status) || Status == STATUS_PENDING)
		return Status;

	m_Position.QuadPart += LocalBytesRead.QuadPart;

	if (BytesRead != nullptr)
		BytesRead->QuadPart = LocalBytesRead.QuadPart;

	return Status;
}

NTSTATUS
NtFileDisk::
Write(
	PVOID           Buffer,
	ULONG_PTR           Size,
	PLARGE_INTEGER  pBytesWritten /* = NULL */
)
{
	NTSTATUS        Status;
	LARGE_INTEGER   BytesWritten;

	Status = WriteInternal(m_FileHandle, Buffer, Size, &BytesWritten, &m_Position);
	if (!NT_SUCCESS(Status) || Status == STATUS_PENDING)
		return Status;

	if (pBytesWritten != nullptr)
		pBytesWritten->QuadPart = BytesWritten.QuadPart;

	if (BytesWritten.QuadPart + m_Position.QuadPart > m_FileSize.QuadPart)
	{
		m_FileSize.QuadPart = BytesWritten.QuadPart + m_Position.QuadPart;
	}

	m_Position.QuadPart += BytesWritten.QuadPart;

	return Status;
}

NTSTATUS
NtFileDisk::
Print(
	PLARGE_INTEGER  BytesWritten,
	PCWSTR          Format,
	...
)
{
	ULONG_PTR   Length;
	WCHAR   Buffer[0xFF0 / 2];
	va_list Arg;

	va_start(Arg, Format);
	//Length = _vsnwprintf(Buffer, countof(Buffer) - 1, Format, Arg);
	Length = FormatStringvnW(Buffer, countof(Buffer) - 1, Format, Arg);
	if (Length == -1)
		return STATUS_BUFFER_TOO_SMALL;

	return Write(Buffer, Length * sizeof(WCHAR), BytesWritten);
}

NTSTATUS
NtFileDisk::
Print(
	PLARGE_INTEGER  BytesWritten,
	PCSTR           Format,
	...
)
{
	ULONG_PTR   Length;
	CHAR        Buffer[0xFF0];
	va_list     Arg;

	va_start(Arg, Format);
	//Length = _vsnwprintf(Buffer, countof(Buffer) - 1, Format, Arg);
	Length = FormatStringvnA(Buffer, countof(Buffer) - 1, Format, Arg);
	if (Length == -1)
		return STATUS_BUFFER_TOO_SMALL;

	return Write(Buffer, Length, BytesWritten);
}

NTSTATUS NtFileDisk::MapView(PVOID *BaseAddress, HANDLE ProcessHandle)
{
	NTSTATUS    Status;
	HANDLE      SectionHandle;
	ULONG_PTR   ViewSize;

	Status = NtCreateSection(
		&SectionHandle,
		GENERIC_READ,
		NULL,
		NULL,
		PAGE_READONLY,
		SEC_IMAGE,
		*this
	);
	FAIL_RETURN(Status);

	ViewSize = 0;
	Status = NtMapViewOfSection(
		SectionHandle,
		ProcessHandle,
		BaseAddress,
		0,
		0,
		NULL,
		&ViewSize,
		ViewShare,
		0,
		PAGE_READONLY
	);

	NtClose(SectionHandle);

	return Status;
}

NTSTATUS NtFileDisk::UnMapView(PVOID BaseAddress, HANDLE ProcessHandle)
{
	if (BaseAddress == nullptr)
		return STATUS_INVALID_PARAMETER;

#if ML_USER_MODE

	return NtUnmapViewOfSection(ProcessHandle, BaseAddress);

#else

	return STATUS_NOT_IMPLEMENTED;

#endif
}


NTSTATUS NtFileDisk::SetEndOfFile()
{
	NTSTATUS Status;

	Status = SetEndOfFileInternal(m_FileHandle, m_Position);
	if (NT_SUCCESS(Status))
	{
		m_FileSize.QuadPart = m_Position.QuadPart;
	}

	return Status;
}

NTSTATUS NtFileDisk::SetEndOfFile(LARGE_INTEGER EndPosition)
{
	NTSTATUS Status;

	Status = SetEndOfFileInternal(m_FileHandle, EndPosition);
	if (NT_SUCCESS(Status))
	{
		m_FileSize.QuadPart = EndPosition.QuadPart;
		m_Position.QuadPart = ML_MIN(m_Position.QuadPart, EndPosition.QuadPart);
	}

	return Status;
}

/************************************************************************
static ver
************************************************************************/

NTSTATUS NtFileDisk::SetEndOfFile(HANDLE FileHandle)
{
	NTSTATUS                    Status;
	FILE_POSITION_INFORMATION   FilePosition;
	IO_STATUS_BLOCK             IoStatus;
	API_POINTER(ZwQueryInformationFile) XQueryInformationFile;

#if ML_KERNEL_MODE

	XQueryInformationFile = ZwQueryInformationFile;

#else

	XQueryInformationFile = NtQueryInformationFile;

#endif

	Status = XQueryInformationFile(
		FileHandle,
		&IoStatus,
		&FilePosition,
		sizeof(FilePosition),
		FilePositionInformation
	);
	if (!NT_SUCCESS(Status))
		return Status;

	return SetEndOfFileInternal(FileHandle, FilePosition.CurrentByteOffset);
}

#endif // CPP_DEFINED

#pragma warning(default:4750 4995)
//#include "Hook.cpp"
//#include "HookHelper.cpp"
/*
___________________________________________________
Opcode Length Disassembler.
Coded By Ms-Rem ( Ms-Rem@yandex.ru ) ICQ 286370715
---------------------------------------------------
12.08.2005 - fixed many bugs...
09.08.2005 - fixed bug with 0F BA opcode.
07.08.2005 - added SSE, SSE2, SSE3 and 3Dnow instruction support.
06.08.2005 - fixed bug with F6 and F7 opcodes.
29.07.2005 - fixed bug with OP_X64_WORD opcodes.
*/


#define OP_X64_NONE           0x00
#define OP_X64_MODRM          0x01
#define OP_X64_DATA_I8        0x02
#define OP_X64_DATA_I16       0x04
#define OP_X64_DATA_I32       0x08
#define OP_X64_DATA_PRE66_67  0x10
#define OP_X64_WORD           0x20
#define OP_X64_REL32          0x40

ForceInline PUCHAR GetOpCodeFlags()
{
	static UCHAR OpcodeFlags[256] =
	{
		OP_X64_MODRM,                      // 00
		OP_X64_MODRM,                      // 01
		OP_X64_MODRM,                      // 02
		OP_X64_MODRM,                      // 03
		OP_X64_DATA_I8,                    // 04
		OP_X64_DATA_PRE66_67,              // 05
		OP_X64_NONE,                       // 06
		OP_X64_NONE,                       // 07
		OP_X64_MODRM,                      // 08
		OP_X64_MODRM,                      // 09
		OP_X64_MODRM,                      // 0A
		OP_X64_MODRM,                      // 0B
		OP_X64_DATA_I8,                    // 0C
		OP_X64_DATA_PRE66_67,              // 0D
		OP_X64_NONE,                       // 0E
		OP_X64_NONE,                       // 0F
		OP_X64_MODRM,                      // 10
		OP_X64_MODRM,                      // 11
		OP_X64_MODRM,                      // 12
		OP_X64_MODRM,                      // 13
		OP_X64_DATA_I8,                    // 14
		OP_X64_DATA_PRE66_67,              // 15
		OP_X64_NONE,                       // 16
		OP_X64_NONE,                       // 17
		OP_X64_MODRM,                      // 18
		OP_X64_MODRM,                      // 19
		OP_X64_MODRM,                      // 1A
		OP_X64_MODRM,                      // 1B
		OP_X64_DATA_I8,                    // 1C
		OP_X64_DATA_PRE66_67,              // 1D
		OP_X64_NONE,                       // 1E
		OP_X64_NONE,                       // 1F
		OP_X64_MODRM,                      // 20
		OP_X64_MODRM,                      // 21
		OP_X64_MODRM,                      // 22
		OP_X64_MODRM,                      // 23
		OP_X64_DATA_I8,                    // 24
		OP_X64_DATA_PRE66_67,              // 25
		OP_X64_NONE,                       // 26
		OP_X64_NONE,                       // 27
		OP_X64_MODRM,                      // 28
		OP_X64_MODRM,                      // 29
		OP_X64_MODRM,                      // 2A
		OP_X64_MODRM,                      // 2B
		OP_X64_DATA_I8,                    // 2C
		OP_X64_DATA_PRE66_67,              // 2D
		OP_X64_NONE,                       // 2E
		OP_X64_NONE,                       // 2F
		OP_X64_MODRM,                      // 30
		OP_X64_MODRM,                      // 31
		OP_X64_MODRM,                      // 32
		OP_X64_MODRM,                      // 33
		OP_X64_DATA_I8,                    // 34
		OP_X64_DATA_PRE66_67,              // 35
		OP_X64_NONE,                       // 36
		OP_X64_NONE,                       // 37
		OP_X64_MODRM,                      // 38
		OP_X64_MODRM,                      // 39
		OP_X64_MODRM,                      // 3A
		OP_X64_MODRM,                      // 3B
		OP_X64_DATA_I8,                    // 3C
		OP_X64_DATA_PRE66_67,              // 3D
		OP_X64_NONE,                       // 3E
		OP_X64_NONE,                       // 3F
		OP_X64_NONE,                       // 40
		OP_X64_NONE,                       // 41
		OP_X64_NONE,                       // 42
		OP_X64_NONE,                       // 43
		OP_X64_NONE,                       // 44
		OP_X64_NONE,                       // 45
		OP_X64_NONE,                       // 46
		OP_X64_NONE,                       // 47
		OP_X64_NONE,                       // 48
		OP_X64_NONE,                       // 49
		OP_X64_NONE,                       // 4A
		OP_X64_NONE,                       // 4B
		OP_X64_NONE,                       // 4C
		OP_X64_NONE,                       // 4D
		OP_X64_NONE,                       // 4E
		OP_X64_NONE,                       // 4F
		OP_X64_NONE,                       // 50
		OP_X64_NONE,                       // 51
		OP_X64_NONE,                       // 52
		OP_X64_NONE,                       // 53
		OP_X64_NONE,                       // 54
		OP_X64_NONE,                       // 55
		OP_X64_NONE,                       // 56
		OP_X64_NONE,                       // 57
		OP_X64_NONE,                       // 58
		OP_X64_NONE,                       // 59
		OP_X64_NONE,                       // 5A
		OP_X64_NONE,                       // 5B
		OP_X64_NONE,                       // 5C
		OP_X64_NONE,                       // 5D
		OP_X64_NONE,                       // 5E
		OP_X64_NONE,                       // 5F
		OP_X64_NONE,                       // 60
		OP_X64_NONE,                       // 61
		OP_X64_MODRM,                      // 62
		OP_X64_MODRM,                      // 63
		OP_X64_NONE,                       // 64
		OP_X64_NONE,                       // 65
		OP_X64_NONE,                       // 66
		OP_X64_NONE,                       // 67
		OP_X64_DATA_PRE66_67,              // 68
		OP_X64_MODRM | OP_X64_DATA_PRE66_67,   // 69
		OP_X64_DATA_I8,                    // 6A
		OP_X64_MODRM | OP_X64_DATA_I8,         // 6B
		OP_X64_NONE,                       // 6C
		OP_X64_NONE,                       // 6D
		OP_X64_NONE,                       // 6E
		OP_X64_NONE,                       // 6F
		OP_X64_DATA_I8,                    // 70
		OP_X64_DATA_I8,                    // 71
		OP_X64_DATA_I8,                    // 72
		OP_X64_DATA_I8,                    // 73
		OP_X64_DATA_I8,                    // 74
		OP_X64_DATA_I8,                    // 75
		OP_X64_DATA_I8,                    // 76
		OP_X64_DATA_I8,                    // 77
		OP_X64_DATA_I8,                    // 78
		OP_X64_DATA_I8,                    // 79
		OP_X64_DATA_I8,                    // 7A
		OP_X64_DATA_I8,                    // 7B
		OP_X64_DATA_I8,                    // 7C
		OP_X64_DATA_I8,                    // 7D
		OP_X64_DATA_I8,                    // 7E
		OP_X64_DATA_I8,                    // 7F
		OP_X64_MODRM | OP_X64_DATA_I8,         // 80
		OP_X64_MODRM | OP_X64_DATA_PRE66_67,   // 81
		OP_X64_MODRM | OP_X64_DATA_I8,         // 82
		OP_X64_MODRM | OP_X64_DATA_I8,         // 83
		OP_X64_MODRM,                      // 84
		OP_X64_MODRM,                      // 85
		OP_X64_MODRM,                      // 86
		OP_X64_MODRM,                      // 87
		OP_X64_MODRM,                      // 88
		OP_X64_MODRM,                      // 89
		OP_X64_MODRM,                      // 8A
		OP_X64_MODRM,                      // 8B
		OP_X64_MODRM,                      // 8C
		OP_X64_MODRM,                      // 8D
		OP_X64_MODRM,                      // 8E
		OP_X64_MODRM,                      // 8F
		OP_X64_NONE,                       // 90
		OP_X64_NONE,                       // 91
		OP_X64_NONE,                       // 92
		OP_X64_NONE,                       // 93
		OP_X64_NONE,                       // 94
		OP_X64_NONE,                       // 95
		OP_X64_NONE,                       // 96
		OP_X64_NONE,                       // 97
		OP_X64_NONE,                       // 98
		OP_X64_NONE,                       // 99
		OP_X64_DATA_I16 | OP_X64_DATA_PRE66_67,// 9A
		OP_X64_NONE,                       // 9B
		OP_X64_NONE,                       // 9C
		OP_X64_NONE,                       // 9D
		OP_X64_NONE,                       // 9E
		OP_X64_NONE,                       // 9F
		OP_X64_DATA_PRE66_67,              // A0
		OP_X64_DATA_PRE66_67,              // A1
		OP_X64_DATA_PRE66_67,              // A2
		OP_X64_DATA_PRE66_67,              // A3
		OP_X64_NONE,                       // A4
		OP_X64_NONE,                       // A5
		OP_X64_NONE,                       // A6
		OP_X64_NONE,                       // A7
		OP_X64_DATA_I8,                    // A8
		OP_X64_DATA_PRE66_67,              // A9
		OP_X64_NONE,                       // AA
		OP_X64_NONE,                       // AB
		OP_X64_NONE,                       // AC
		OP_X64_NONE,                       // AD
		OP_X64_NONE,                       // AE
		OP_X64_NONE,                       // AF
		OP_X64_DATA_I8,                    // B0
		OP_X64_DATA_I8,                    // B1
		OP_X64_DATA_I8,                    // B2
		OP_X64_DATA_I8,                    // B3
		OP_X64_DATA_I8,                    // B4
		OP_X64_DATA_I8,                    // B5
		OP_X64_DATA_I8,                    // B6
		OP_X64_DATA_I8,                    // B7
		OP_X64_DATA_PRE66_67,              // B8
		OP_X64_DATA_PRE66_67,              // B9
		OP_X64_DATA_PRE66_67,              // BA
		OP_X64_DATA_PRE66_67,              // BB
		OP_X64_DATA_PRE66_67,              // BC
		OP_X64_DATA_PRE66_67,              // BD
		OP_X64_DATA_PRE66_67,              // BE
		OP_X64_DATA_PRE66_67,              // BF
		OP_X64_MODRM | OP_X64_DATA_I8,         // C0
		OP_X64_MODRM | OP_X64_DATA_I8,         // C1
		OP_X64_DATA_I16,                   // C2
		OP_X64_NONE,                       // C3
		OP_X64_MODRM,                      // C4
		OP_X64_MODRM,                      // C5
		OP_X64_MODRM | OP_X64_DATA_I8,       // C6
		OP_X64_MODRM | OP_X64_DATA_PRE66_67, // C7
		OP_X64_DATA_I8 | OP_X64_DATA_I16,      // C8
		OP_X64_NONE,                       // C9
		OP_X64_DATA_I16,                   // CA
		OP_X64_NONE,                       // CB
		OP_X64_NONE,                       // CC
		OP_X64_DATA_I8,                    // CD
		OP_X64_NONE,                       // CE
		OP_X64_NONE,                       // CF
		OP_X64_MODRM,                      // D0
		OP_X64_MODRM,                      // D1
		OP_X64_MODRM,                      // D2
		OP_X64_MODRM,                      // D3
		OP_X64_DATA_I8,                    // D4
		OP_X64_DATA_I8,                    // D5
		OP_X64_NONE,                       // D6
		OP_X64_NONE,                       // D7
		OP_X64_WORD,                       // D8
		OP_X64_WORD,                       // D9
		OP_X64_WORD,                       // DA
		OP_X64_WORD,                       // DB
		OP_X64_WORD,                       // DC
		OP_X64_WORD,                       // DD
		OP_X64_WORD,                       // DE
		OP_X64_WORD,                       // DF
		OP_X64_DATA_I8,                    // E0
		OP_X64_DATA_I8,                    // E1
		OP_X64_DATA_I8,                    // E2
		OP_X64_DATA_I8,                    // E3
		OP_X64_DATA_I8,                    // E4
		OP_X64_DATA_I8,                    // E5
		OP_X64_DATA_I8,                    // E6
		OP_X64_DATA_I8,                    // E7
		OP_X64_DATA_PRE66_67 | OP_X64_REL32,   // E8
		OP_X64_DATA_PRE66_67 | OP_X64_REL32,   // E9
		OP_X64_DATA_I16 | OP_X64_DATA_PRE66_67,// EA
		OP_X64_DATA_I8,                    // EB
		OP_X64_NONE,                       // EC
		OP_X64_NONE,                       // ED
		OP_X64_NONE,                       // EE
		OP_X64_NONE,                       // EF
		OP_X64_NONE,                       // F0
		OP_X64_NONE,                       // F1
		OP_X64_NONE,                       // F2
		OP_X64_NONE,                       // F3
		OP_X64_NONE,                       // F4
		OP_X64_NONE,                       // F5
		OP_X64_MODRM,                      // F6
		OP_X64_MODRM,                      // F7
		OP_X64_NONE,                       // F8
		OP_X64_NONE,                       // F9
		OP_X64_NONE,                       // FA
		OP_X64_NONE,                       // FB
		OP_X64_NONE,                       // FC
		OP_X64_NONE,                       // FD
		OP_X64_MODRM,                      // FE
		OP_X64_MODRM | OP_X64_REL32            // FF
	};

	return OpcodeFlags;
}

ForceInline PUCHAR GetOpCodeFlagsExt()
{
	static UCHAR OpcodeFlagsExt[256] =
	{
		OP_X64_MODRM,                      // 00
		OP_X64_MODRM,                      // 01
		OP_X64_MODRM,                      // 02
		OP_X64_MODRM,                      // 03
		OP_X64_NONE,                       // 04
		OP_X64_NONE,                       // 05
		OP_X64_NONE,                       // 06
		OP_X64_NONE,                       // 07
		OP_X64_NONE,                       // 08
		OP_X64_NONE,                       // 09
		OP_X64_NONE,                       // 0A
		OP_X64_NONE,                       // 0B
		OP_X64_NONE,                       // 0C
		OP_X64_MODRM,                      // 0D
		OP_X64_NONE,                       // 0E
		OP_X64_MODRM | OP_X64_DATA_I8,         // 0F
		OP_X64_MODRM,                      // 10
		OP_X64_MODRM,                      // 11
		OP_X64_MODRM,                      // 12
		OP_X64_MODRM,                      // 13
		OP_X64_MODRM,                      // 14
		OP_X64_MODRM,                      // 15
		OP_X64_MODRM,                      // 16
		OP_X64_MODRM,                      // 17
		OP_X64_MODRM,                      // 18
		OP_X64_NONE,                       // 19
		OP_X64_NONE,                       // 1A
		OP_X64_NONE,                       // 1B
		OP_X64_NONE,                       // 1C
		OP_X64_NONE,                       // 1D
		OP_X64_NONE,                       // 1E
		OP_X64_NONE,                       // 1F
		OP_X64_MODRM,                      // 20
		OP_X64_MODRM,                      // 21
		OP_X64_MODRM,                      // 22
		OP_X64_MODRM,                      // 23
		OP_X64_MODRM,                      // 24
		OP_X64_NONE,                       // 25
		OP_X64_MODRM,                      // 26
		OP_X64_NONE,                       // 27
		OP_X64_MODRM,                      // 28
		OP_X64_MODRM,                      // 29
		OP_X64_MODRM,                      // 2A
		OP_X64_MODRM,                      // 2B
		OP_X64_MODRM,                      // 2C
		OP_X64_MODRM,                      // 2D
		OP_X64_MODRM,                      // 2E
		OP_X64_MODRM,                      // 2F
		OP_X64_NONE,                       // 30
		OP_X64_NONE,                       // 31
		OP_X64_NONE,                       // 32
		OP_X64_NONE,                       // 33
		OP_X64_NONE,                       // 34
		OP_X64_NONE,                       // 35
		OP_X64_NONE,                       // 36
		OP_X64_NONE,                       // 37
		OP_X64_NONE,                       // 38
		OP_X64_NONE,                       // 39
		OP_X64_NONE,                       // 3A
		OP_X64_NONE,                       // 3B
		OP_X64_NONE,                       // 3C
		OP_X64_NONE,                       // 3D
		OP_X64_NONE,                       // 3E
		OP_X64_NONE,                       // 3F
		OP_X64_MODRM,                      // 40
		OP_X64_MODRM,                      // 41
		OP_X64_MODRM,                      // 42
		OP_X64_MODRM,                      // 43
		OP_X64_MODRM,                      // 44
		OP_X64_MODRM,                      // 45
		OP_X64_MODRM,                      // 46
		OP_X64_MODRM,                      // 47
		OP_X64_MODRM,                      // 48
		OP_X64_MODRM,                      // 49
		OP_X64_MODRM,                      // 4A
		OP_X64_MODRM,                      // 4B
		OP_X64_MODRM,                      // 4C
		OP_X64_MODRM,                      // 4D
		OP_X64_MODRM,                      // 4E
		OP_X64_MODRM,                      // 4F
		OP_X64_MODRM,                      // 50
		OP_X64_MODRM,                      // 51
		OP_X64_MODRM,                      // 52
		OP_X64_MODRM,                      // 53
		OP_X64_MODRM,                      // 54
		OP_X64_MODRM,                      // 55
		OP_X64_MODRM,                      // 56
		OP_X64_MODRM,                      // 57
		OP_X64_MODRM,                      // 58
		OP_X64_MODRM,                      // 59
		OP_X64_MODRM,                      // 5A
		OP_X64_MODRM,                      // 5B
		OP_X64_MODRM,                      // 5C
		OP_X64_MODRM,                      // 5D
		OP_X64_MODRM,                      // 5E
		OP_X64_MODRM,                      // 5F
		OP_X64_MODRM,                      // 60
		OP_X64_MODRM,                      // 61
		OP_X64_MODRM,                      // 62
		OP_X64_MODRM,                      // 63
		OP_X64_MODRM,                      // 64
		OP_X64_MODRM,                      // 65
		OP_X64_MODRM,                      // 66
		OP_X64_MODRM,                      // 67
		OP_X64_MODRM,                      // 68
		OP_X64_MODRM,                      // 69
		OP_X64_MODRM,                      // 6A
		OP_X64_MODRM,                      // 6B
		OP_X64_MODRM,                      // 6C
		OP_X64_MODRM,                      // 6D
		OP_X64_MODRM,                      // 6E
		OP_X64_MODRM,                      // 6F
		OP_X64_MODRM | OP_X64_DATA_I8,         // 70
		OP_X64_MODRM | OP_X64_DATA_I8,         // 71
		OP_X64_MODRM | OP_X64_DATA_I8,         // 72
		OP_X64_MODRM | OP_X64_DATA_I8,         // 73
		OP_X64_MODRM,                      // 74
		OP_X64_MODRM,                      // 75
		OP_X64_MODRM,                      // 76
		OP_X64_NONE,                       // 77
		OP_X64_NONE,                       // 78
		OP_X64_NONE,                       // 79
		OP_X64_NONE,                       // 7A
		OP_X64_NONE,                       // 7B
		OP_X64_MODRM,                      // 7C
		OP_X64_MODRM,                      // 7D
		OP_X64_MODRM,                      // 7E
		OP_X64_MODRM,                      // 7F
		OP_X64_DATA_PRE66_67 | OP_X64_REL32,   // 80
		OP_X64_DATA_PRE66_67 | OP_X64_REL32,   // 81
		OP_X64_DATA_PRE66_67 | OP_X64_REL32,   // 82
		OP_X64_DATA_PRE66_67 | OP_X64_REL32,   // 83
		OP_X64_DATA_PRE66_67 | OP_X64_REL32,   // 84
		OP_X64_DATA_PRE66_67 | OP_X64_REL32,   // 85
		OP_X64_DATA_PRE66_67 | OP_X64_REL32,   // 86
		OP_X64_DATA_PRE66_67 | OP_X64_REL32,   // 87
		OP_X64_DATA_PRE66_67 | OP_X64_REL32,   // 88
		OP_X64_DATA_PRE66_67 | OP_X64_REL32,   // 89
		OP_X64_DATA_PRE66_67 | OP_X64_REL32,   // 8A
		OP_X64_DATA_PRE66_67 | OP_X64_REL32,   // 8B
		OP_X64_DATA_PRE66_67 | OP_X64_REL32,   // 8C
		OP_X64_DATA_PRE66_67 | OP_X64_REL32,   // 8D
		OP_X64_DATA_PRE66_67 | OP_X64_REL32,   // 8E
		OP_X64_DATA_PRE66_67 | OP_X64_REL32,   // 8F
		OP_X64_MODRM,                      // 90
		OP_X64_MODRM,                      // 91
		OP_X64_MODRM,                      // 92
		OP_X64_MODRM,                      // 93
		OP_X64_MODRM,                      // 94
		OP_X64_MODRM,                      // 95
		OP_X64_MODRM,                      // 96
		OP_X64_MODRM,                      // 97
		OP_X64_MODRM,                      // 98
		OP_X64_MODRM,                      // 99
		OP_X64_MODRM,                      // 9A
		OP_X64_MODRM,                      // 9B
		OP_X64_MODRM,                      // 9C
		OP_X64_MODRM,                      // 9D
		OP_X64_MODRM,                      // 9E
		OP_X64_MODRM,                      // 9F
		OP_X64_NONE,                       // A0
		OP_X64_NONE,                       // A1
		OP_X64_NONE,                       // A2
		OP_X64_MODRM,                      // A3
		OP_X64_MODRM | OP_X64_DATA_I8,         // A4
		OP_X64_MODRM,                      // A5
		OP_X64_NONE,                       // A6
		OP_X64_NONE,                       // A7
		OP_X64_NONE,                       // A8
		OP_X64_NONE,                       // A9
		OP_X64_NONE,                       // AA
		OP_X64_MODRM,                      // AB
		OP_X64_MODRM | OP_X64_DATA_I8,         // AC
		OP_X64_MODRM,                      // AD
		OP_X64_MODRM,                      // AE
		OP_X64_MODRM,                      // AF
		OP_X64_MODRM,                      // B0
		OP_X64_MODRM,                      // B1
		OP_X64_MODRM,                      // B2
		OP_X64_MODRM,                      // B3
		OP_X64_MODRM,                      // B4
		OP_X64_MODRM,                      // B5
		OP_X64_MODRM,                      // B6
		OP_X64_MODRM,                      // B7
		OP_X64_NONE,                       // B8
		OP_X64_NONE,                       // B9
		OP_X64_MODRM | OP_X64_DATA_I8,         // BA
		OP_X64_MODRM,                      // BB
		OP_X64_MODRM,                      // BC
		OP_X64_MODRM,                      // BD
		OP_X64_MODRM,                      // BE
		OP_X64_MODRM,                      // BF
		OP_X64_MODRM,                      // C0
		OP_X64_MODRM,                      // C1
		OP_X64_MODRM | OP_X64_DATA_I8,         // C2
		OP_X64_MODRM,                      // C3
		OP_X64_MODRM | OP_X64_DATA_I8,         // C4
		OP_X64_MODRM | OP_X64_DATA_I8,         // C5
		OP_X64_MODRM | OP_X64_DATA_I8,         // C6
		OP_X64_MODRM,                      // C7
		OP_X64_NONE,                       // C8
		OP_X64_NONE,                       // C9
		OP_X64_NONE,                       // CA
		OP_X64_NONE,                       // CB
		OP_X64_NONE,                       // CC
		OP_X64_NONE,                       // CD
		OP_X64_NONE,                       // CE
		OP_X64_NONE,                       // CF
		OP_X64_MODRM,                      // D0
		OP_X64_MODRM,                      // D1
		OP_X64_MODRM,                      // D2
		OP_X64_MODRM,                      // D3
		OP_X64_MODRM,                      // D4
		OP_X64_MODRM,                      // D5
		OP_X64_MODRM,                      // D6
		OP_X64_MODRM,                      // D7
		OP_X64_MODRM,                      // D8
		OP_X64_MODRM,                      // D9
		OP_X64_MODRM,                      // DA
		OP_X64_MODRM,                      // DB
		OP_X64_MODRM,                      // DC
		OP_X64_MODRM,                      // DD
		OP_X64_MODRM,                      // DE
		OP_X64_MODRM,                      // DF
		OP_X64_MODRM,                      // E0
		OP_X64_MODRM,                      // E1
		OP_X64_MODRM,                      // E2
		OP_X64_MODRM,                      // E3
		OP_X64_MODRM,                      // E4
		OP_X64_MODRM,                      // E5
		OP_X64_MODRM,                      // E6
		OP_X64_MODRM,                      // E7
		OP_X64_MODRM,                      // E8
		OP_X64_MODRM,                      // E9
		OP_X64_MODRM,                      // EA
		OP_X64_MODRM,                      // EB
		OP_X64_MODRM,                      // EC
		OP_X64_MODRM,                      // ED
		OP_X64_MODRM,                      // EE
		OP_X64_MODRM,                      // EF
		OP_X64_MODRM,                      // F0
		OP_X64_MODRM,                      // F1
		OP_X64_MODRM,                      // F2
		OP_X64_MODRM,                      // F3
		OP_X64_MODRM,                      // F4
		OP_X64_MODRM,                      // F5
		OP_X64_MODRM,                      // F6
		OP_X64_MODRM,                      // F7
		OP_X64_MODRM,                      // F8
		OP_X64_MODRM,                      // F9
		OP_X64_MODRM,                      // FA
		OP_X64_MODRM,                      // FB
		OP_X64_MODRM,                      // FC
		OP_X64_MODRM,                      // FD
		OP_X64_MODRM,                      // FE
		OP_X64_NONE                        // FF
	};

	return OpcodeFlagsExt;
}

ULONG_PTR FASTCALL LdeGetOpCodeSize64(PVOID Code, PVOID *OpCodePtr)
{
	// OpCode High 5 bits as index, (1 << low 3bits) as flag

	static UCHAR PrefixTable[0x20] =
	{
		0x00, 0x00, 0x00, 0x00, 0x40, 0x40, 0x40, 0x40, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0d, 0x00
	};

	PUCHAR      Ptr;
	BOOL        PFX66, PFX67;
	BOOL        SibPresent;
	BOOL        Rex;
	ULONG_PTR   Flags;
	ULONG_PTR   Mod, RM, Reg;
	ULONG_PTR   OffsetSize, Imm64;
	ULONG_PTR   Opcode;

	Imm64 = 0;
	OffsetSize = 0;
	PFX66 = FALSE;
	PFX67 = FALSE;
	Rex = FALSE;
	Ptr = (PUCHAR)Code;

	//    while ( (*cPtr == 0x2E) || (*cPtr == 0x3E) || (*cPtr == 0x36) ||
	//            (*cPtr == 0x26) || (*cPtr == 0x64) || (*cPtr == 0x65) ||
	//            (*cPtr == 0xF0) || (*cPtr == 0xF2) || (*cPtr == 0xF3) ||
	//            (*cPtr == 0x66) || (*cPtr == 0x67) )
	while (PrefixTable[*Ptr >> 3] & (1 << (*Ptr & 7)))
	{
		PFX66 = *Ptr == 0x66;
		PFX67 = *Ptr == 0x67;
		Ptr++;
		if (Ptr > (PUCHAR)Code + 16)
			return 0;
	}

	// 0x40 ~ 0x4F
	if (((*Ptr) >> 4) == 0x4)
	{
		Rex = *Ptr & 0x0F;
		if (Rex)
			++Ptr;
	}

	Opcode = *Ptr;
	if (OpCodePtr)
		*OpCodePtr = Ptr;

	if (*Ptr == 0x0F)
	{
		Ptr++;
		Flags = GetOpCodeFlagsExt()[*Ptr];
	}
	else
	{
		ULONG_PTR tmp;

		Flags = GetOpCodeFlags()[Opcode];

		// if (Opcode >= 0xA0 && Opcode <= 0xA3)
		tmp = Opcode >> 2;
		if (tmp == 0x28)
		{
			PFX66 = PFX67;
		}
		else if (Rex && (tmp >> 1) == 0x17)     // 0xB8 ~ 0xBF  mov r64, imm64
		{
			Imm64 = 4;
		}
	}

	Ptr++;
	Ptr += FLAG_ON(Flags, OP_X64_WORD);

	if (Flags & OP_X64_MODRM)
	{
		Mod = *Ptr >> 6;
		Reg = (*Ptr & 0x38) >> 3;
		RM = *Ptr & 7;
		Ptr++;

		if ((Opcode == 0xF6) && !Reg)
			SET_FLAG(Flags, OP_X64_DATA_I8);

		if ((Opcode == 0xF7) && !Reg)
			SET_FLAG(Flags, OP_X64_DATA_PRE66_67);

		SibPresent = !PFX67 & (RM == 4);
		switch (Mod)
		{
		case 0:
			if (PFX67 && (RM == 6))
				OffsetSize = 2;
			if (!PFX67 && (RM == 5))
				OffsetSize = 4;
			break;

		case 1:
			OffsetSize = 1;
			break;

		case 2:
			OffsetSize = PFX67 ? 2 : 4;
			break;

		case 3:
			SibPresent = FALSE;
			break;
		}

		if (SibPresent)
		{
			if (((*Ptr & 7) == 5) && ((!Mod) || (Mod == 2)))
				OffsetSize = 4;

			Ptr++;
		}

		Ptr += OffsetSize;
	}

	Ptr += FLAG_ON(Flags, OP_X64_DATA_I8);
	Ptr += FLAG_ON(Flags, OP_X64_DATA_I16) ? 2 : 0;
	Ptr += FLAG_ON(Flags, OP_X64_DATA_I32) ? 4 : 0;
	Ptr += FLAG_ON(Flags, OP_X64_DATA_PRE66_67) ? (PFX66 ? 2 : 4) : 0;
	Ptr += (Rex & 9) ? Imm64 : 0;   // 0x48 || 0x49

	return PtrOffset(Ptr, Code);
}

ULONG_PTR FASTCALL SizeOfProc(PVOID Proc)
{
	ULONG_PTR   Length;
	PBYTE       pOpcode;
	ULONG_PTR   Result = 0;

	do
	{
		Length = LdeGetOpCodeSize64(Proc, (PVOID *)&pOpcode);
		Result += Length;
		if ((Length == 1) && (*pOpcode == 0xC3))
			break;

		Proc = (PVOID)((ULONG_PTR)Proc + Length);
	} while (Length);

	return Result;
}

BOOL FASTCALL IsRelativeCmd(PBYTE OpCode)
{
	UCHAR Flags;

	if (*OpCode == 0x0F)
	{
		Flags = GetOpCodeFlagsExt()[OpCode[1]];
	}
	else
	{
		Flags = GetOpCodeFlags()[*OpCode];
	}

	return FLAG_ON(Flags, OP_X64_REL32);
}

#define OP_X86_NONE           0x000
#define OP_X86_DATA_I8        0x001
#define OP_X86_DATA_I16       0x002
#define OP_X86_DATA_I32       0x004
#define OP_X86_MODRM          0x008
#define OP_X86_DATA_PRE66_67  0x010
#define OP_X86_PREFIX         0x020
#define OP_X86_REL32          0x040
#define OP_X86_REL8           0x080

/* extended opcode flags (by analyzer) */
#define OP_X86_EXTENDED       0x100

ForceInline PBYTE GetPackedTable()
{
	static BYTE PackedTable[] =
	{
		0x80, 0x84, 0x80, 0x84, 0x80, 0x84, 0x80, 0x84,
		0x80, 0x88, 0x80, 0x88, 0x80, 0x88, 0x80, 0x88,
		0x8c, 0x8b, 0x8b, 0x8b, 0x8b, 0x8b, 0x8b, 0x8b,
		0x90, 0x94, 0x98, 0x8b, 0x9c, 0x9c, 0x9c, 0x9c,
		0xa0, 0x80, 0x80, 0x80, 0x8b, 0x8b, 0xa4, 0x8b,
		0xa8, 0x8b, 0x84, 0x8b, 0xac, 0xac, 0xa8, 0xa8,
		0xb0, 0xb4, 0xb8, 0xbc, 0x80, 0xc0, 0x80, 0x80,
		0x9c, 0xac, 0xc4, 0x8b, 0xc8, 0x90, 0x8b, 0x90,
		0x80, 0x8b, 0x8b, 0xcc, 0x80, 0x80, 0xd0, 0x8b,
		0x80, 0xd4, 0x80, 0x80, 0x8b, 0x8b, 0x8b, 0x8b,
		0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
		0x80, 0x80, 0x80, 0x80, 0xd8, 0xdc, 0x8b, 0x80,
		0xe0, 0xe0, 0xe0, 0xe0, 0x80, 0x80, 0x80, 0x80,
		0x8f, 0xcf, 0x8f, 0xdb, 0x80, 0x80, 0xe4, 0x80,
		0xe8, 0xd9, 0x8b, 0x8b, 0x80, 0x80, 0x80, 0x80,
		0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xdc,
		0x08, 0x08, 0x08, 0x08, 0x01, 0x10, 0x00, 0x00,
		0x01, 0x10, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x08, 0x08, 0x20, 0x20, 0x20, 0x20,
		0x10, 0x18, 0x01, 0x09, 0x81, 0x81, 0x81, 0x81,
		0x09, 0x18, 0x09, 0x09, 0x00, 0x00, 0x12, 0x00,
		0x10, 0x10, 0x10, 0x10, 0x01, 0x01, 0x01, 0x01,
		0x09, 0x09, 0x02, 0x00, 0x08, 0x08, 0x09, 0x18,
		0x03, 0x00, 0x02, 0x00, 0x00, 0x01, 0x00, 0x00,
		0x01, 0x01, 0x00, 0x00, 0x50, 0x50, 0x12, 0x81,
		0x20, 0x00, 0x20, 0x20, 0x00, 0x08, 0x00, 0x09,
		0x08, 0x00, 0x00, 0x00, 0x08, 0x00, 0x08, 0x00,
		0x09, 0x09, 0x09, 0x09, 0x08, 0x08, 0x08, 0x00,
		0x50, 0x50, 0x50, 0x50, 0x00, 0x00, 0x09, 0x08,
		0x08, 0x08, 0x09, 0x08
	};

	return PackedTable;
}

ForceInline ULONG_PTR GetOpCodeFlags32(ULONG_PTR OpCode)
{
	return GetPackedTable()[GetPackedTable()[OpCode / 4] + (OpCode % 4)];
}

ULONG_PTR GetOpCodeExtendedFlags32(unsigned char *addr)
{
	ULONG_PTR opcode;

	opcode = addr[0];
	if (opcode == 0x0F)
	{
		opcode = addr[1] | OP_X86_EXTENDED;
	}

	return GetOpCodeFlags32(opcode) | (opcode & OP_X86_EXTENDED);
}

ULONG_PTR FASTCALL LdeGetOpCodeSize32(PVOID Code, PVOID *OpCodePtr)
{
	unsigned char i_mod, i_rm, i_reg;
	ULONG_PTR op1, op2, flags;
	ULONG_PTR pfx66, pfx67;
	ULONG_PTR osize, oflen;
	PBYTE code;

	pfx66 = 0;
	pfx67 = 0;
	osize = 0;
	oflen = 0;

	code = (PBYTE)Code;
	op1 = *code;

	/* skip preffixes */
	while (GetOpCodeFlags32(op1) & OP_X86_PREFIX)
	{
		switch (op1)
		{
		case 0x66:
			pfx66 = 1;
			break;

		case 0x67:
			pfx67 = 1;
			break;
		}

		osize++;
		op1 = *++code;
	}

	/* get opcode size and flags */
	if (OpCodePtr != NULL)
		*OpCodePtr = code;

	++code;
	osize++;

	if (op1 == 0x0F)
	{
		op2 = (*code | OP_X86_EXTENDED);
		code++;
		osize++;
	}
	else
	{
		op2 = op1;

		/* pfx66 = pfx67 for opcodes A0 - A3 */
		if (op2 >= 0xA0 && op2 <= 0xA3)
		{
			pfx66 = pfx67;
		}
	}

	flags = GetOpCodeFlags32(op2);

	/* process MODRM byte */
	if (flags & OP_X86_MODRM)
	{
		i_mod = (*code >> 6);
		i_reg = (*code & 0x38) >> 3;
		i_rm = (*code & 7);
		code++;
		osize++;

		/* in F6 and F7 opcodes, immediate value present if i_reg == 0 */
		if (op1 == 0xF6 && i_reg == 0)
		{
			flags |= OP_X86_DATA_I8;
		}
		if (op1 == 0xF7 && i_reg == 0)
		{
			flags |= OP_X86_DATA_PRE66_67;
		}

		switch (i_mod)
		{
		case 0:
			if (pfx67)
			{
				if (i_rm == 6)
					oflen = 2;
			}
			else
			{
				if (i_rm == 5)
					oflen = 4;
			}
			break;

		case 1:
			oflen = 1;
			break;

		case 2:
			if (pfx67)
				oflen = 2;
			else
				oflen = 4;
			break;
		}

		/* process SIB byte */
		if (pfx67 == 0 && i_rm == 4 && i_mod != 3)
		{
			if ((*code & 7) == 5 && (i_mod != 1))
			{
				oflen = 4;
			}

			oflen++;
		}

		osize += oflen;
	}

	/* process offset */
	if (flags & OP_X86_DATA_PRE66_67)
	{
		osize += 4 - (pfx66 << 1);
	}
	/* process immediate value */
	osize += (flags & 7);

	return osize;
}

#pragma push_macro("POINTER_SIZE")

#undef POINTER_SIZE
#define POINTER_SIZE sizeof(PVOID)

NTSTATUS
CopyOneOpCode(
	PVOID       Target,
	PVOID       Source,
	PULONG_PTR  TargetOpLength,
	PULONG_PTR  SourceOpLength,
	ULONG_PTR   ForwardSize,
	ULONG_PTR   BackwardSize,
	PVOID       TargetIp
)
{
	ULONG_PTR   Length, OpCode, OpCodeLength, OpOffsetLength, Selector;
	LONG_PTR    OpOffset, *pOpOffset;
	PBYTE       Func, p;
	ULONG_PTR   NextOpAddress;
	LONG_PTR    NewOffset;

	enum { OpCodeOffsetShort = 1, OpCodeOffsetLong = POINTER_SIZE };

	TargetIp = TargetIp == IMAGE_INVALID_VA ? Target : TargetIp;

	NewOffset = 0;
	Func = (PBYTE)Source;
	p = (PBYTE)Target;

	Length = GetOpCodeSize(Func);

	OpCode = *Func;
	pOpOffset = (PLONG_PTR)(Func + 1);
	OpOffsetLength = OpCodeOffsetShort;
	OpCodeLength = 2;
	Selector = -1u;

	if (((OpCode & 0xF0) ^ 0x70) == 0)
		//    if (OpCode >= 0x70 && OpCode <= 0x7F)
	{
		OpCode = 0x800F | ((OpCode - 0x70) << 8);
	}
	else if (OpCode == 0xEB)
	{
		OpCode = 0xE9;
		OpCodeLength = 1;
	}
	else if (OpCode == 0xE8 || OpCode == 0xE9)
	{
		OpOffsetLength = OpCodeOffsetLong;
		OpCodeLength = 1;
	}
	else
	{
		ULONG _Op;

		OpCode = *(PUSHORT)Func;
		_Op = SWAP2(OpCode);

		if ((_Op & ~0xF) == 0x0F80)
			// if (_Op >= 0x0F80 && _Op <= 0x0F8F)
		{
			OpOffsetLength = OpCodeOffsetLong;
			pOpOffset = (PLONG_PTR)(Func + 2);
		}
		else
		{
		DEFAULT_OP_CODE:
			CopyMemory(p, Func, Length);
			p += Length;

			goto EXIT_PROC;
		}
	}

	OpOffset = 0;
	switch (OpOffsetLength)
	{
	case OpCodeOffsetShort:
		OpOffset = *(PCHAR)pOpOffset;
		break;

	case OpCodeOffsetLong:
	default:
		OpOffset = *(PLONG_PTR)pOpOffset;
		break;
	}

	//    ULONG_PTR NewOffset = (ULONG_PTR)Func + Length + OpOffset;
	//    if (NewOffset >= (ULONG_PTR)Destination && NewOffset < (ULONG_PTR)Destination + BytesToNop + HookOpSize)
	//        goto DEFAULT_OP_CODE;

	NewOffset = Length + OpOffset;

	if (NewOffset < 0)
	{
		if ((ULONG_PTR)-NewOffset < BackwardSize)
			goto DEFAULT_OP_CODE;
	}
	else if ((ULONG_PTR)NewOffset < ForwardSize)
	{
		goto DEFAULT_OP_CODE;
	}

	NextOpAddress = (ULONG_PTR)Func + Length;
	OpOffset = (NextOpAddress + OpOffset) - ((ULONG_PTR)TargetIp + OpCodeLength + OpCodeOffsetLong + (Selector == -1 ? 0 : 2));
	switch (OpCodeLength)
	{
	case 1:
		*p++ = (BYTE)OpCode;
		break;

	case 2:
	default:
		*(PUSHORT)p = (USHORT)OpCode;
		p += 2;
		break;
	}
	/*
	if (Selector != -1)
	{
	*(PUSHORT)p = Selector;
	p += 2;
	}
	*/
	*(PLONG_PTR)p = OpOffset;
	p += POINTER_SIZE;

	//    Length = PtrOffset(p, Destination);

EXIT_PROC:
	if (SourceOpLength != NULL)
		*SourceOpLength = Length;

	if (TargetOpLength != NULL)
		*TargetOpLength = PtrOffset(p, Target);

	return STATUS_SUCCESS;
}

ML_NAMESPACE_BEGIN(Mp);

#if ML_X86

#define SIZE_OF_JUMP_CODE   5

#elif ML_AMD64


#endif // arch

VOID PatchNop(PVOID Address, ULONG_PTR BytesToPatch)
{
	PBYTE Buffer = (PBYTE)Address;

	switch (BytesToPatch)
	{
	case 0:
		return;

	case 1:
		//
		// nop
		//
		Buffer[0] = 0x90;
		break;

	case 2:
		//
		// mov eax, eax
		//
		*(PUSHORT)Buffer = 0xC08B;
		break;

	case 3:
		//
		// lea eax, [eax+0]
		//
		*(PUSHORT)Buffer = 0x408D;
		Buffer[2] = 0x00;
		break;

	case 4:
		//
		// lea esi, [esi]
		//
		*(PULONG)Buffer = 0x0026748D;
		break;

	case 5:
		// 2 + 3
		*Buffer = 0x8B;
		*(PULONG)(Buffer + 1) = 0x408DC0;
		break;

	case 6:
		// lea eax, [eax+0]
		*(PULONG)Buffer = 0x808D;
		*(PUSHORT)(Buffer + 4) = 0;
		break;

	case 7:
		// lea esi, [esi]
		*(PULONG)Buffer = 0x0026B48D;
		*(PULONG)&Buffer[3] = 0;
		break;
	}
}

#if ML_AMD || ML_X86

class MemoryPatchManager
{
protected:
	GrowableArray<PTRAMPOLINE_DATA> TrampolineList;
	HANDLE ExecutableHeap;

public:
	MemoryPatchManager()
	{
		this->ExecutableHeap = RtlCreateHeap(HEAP_CREATE_ENABLE_EXECUTE | HEAP_GROWABLE | HEAP_CREATE_ALIGN_16, nullptr, 0, 0, nullptr, nullptr);
	}

protected:
	NoInline NTSTATUS HandleMemoryPatch(PPATCH_MEMORY_DATA PatchData, PVOID BaseAddress)
	{
		auto&       Patch = PatchData->Memory;
		PVOID       Address;
		ULONG       Protect;
		NTSTATUS    Status;

		if (Patch.Size == 0)
			return STATUS_SUCCESS;

		if (Patch.Address == IMAGE_INVALID_RVA)
			return STATUS_SUCCESS;

		Address = PtrAdd(Patch.Options.VirtualAddress ? nullptr : BaseAddress, Patch.Address);
		Status = ProtectMemory(Address, Patch.Size, PAGE_EXECUTE_READWRITE, &Protect);
		FAIL_RETURN(Status);

		if (Patch.Options.DataIsBuffer)
		{
			CopyMemory(Address, (PVOID)Patch.Data, Patch.Size);
		}
		else
		{
			if (Patch.Options.BackupData)
			{
				PTRAMPOLINE_DATA TrampolineData;

				TrampolineData = AllocateTrampolineData();
				if (TrampolineData == nullptr)
					return STATUS_NO_MEMORY;

				TrampolineData->PatchData = *PatchData;
				TrampolineData->PatchData.Memory.Backup = 0;
				TrampolineData->PatchData.Memory.Options.VirtualAddress = TRUE;
				TrampolineData->PatchData.Memory.Address = (ULONG_PTR)Address;
				CopyMemory(&TrampolineData->PatchData.Memory.Backup, Address, Patch.Size);

				*PatchData = TrampolineData->PatchData;

				this->TrampolineList.Add(TrampolineData);
			}

			CopyMemory(&Patch.Backup, Address, Patch.Size);
			CopyMemory(Address, &Patch.Data, Patch.Size);
		}

		Status = ProtectMemory(Address, Patch.Size, Protect, nullptr);

		return STATUS_SUCCESS;
	}

	NoInline NTSTATUS HandleFunctionPatch(PPATCH_MEMORY_DATA PatchData, PVOID BaseAddress)
	{
		auto&               Function = PatchData->Function;
		BYTE                LocalHookBuffer[TRAMPOLINE_SIZE];
		PVOID               Address;
		PBYTE               Trampoline;
		ULONG               Protect;
		ULONG_PTR           HookOpSize, CopyOpSize;
		NTSTATUS            Status;
		PTRAMPOLINE_DATA    TrampolineData;

		if (Function.Target == nullptr)
			return STATUS_SUCCESS;

		if (Function.Source == IMAGE_INVALID_RVA)
			return STATUS_SUCCESS;

		Address = PtrAdd(Function.Options.VirtualAddress ? nullptr : BaseAddress, Function.Source);

		HookOpSize = GetSizeOfHookOpSize(Function.HookOp);
		if (HookOpSize == ULONG_PTR_MAX)
			return STATUS_BUFFER_TOO_SMALL;

		Status = GetHookAddressAndSize(Address, HookOpSize, &Address, &CopyOpSize);
		FAIL_RETURN(Status);

		if (CopyOpSize > TRAMPOLINE_SIZE)
			RaiseDebugBreak();

		if (Function.Options.NakedTrampoline == FALSE)
		{
			Status = GenerateHookCode(LocalHookBuffer, Address, Function.Target, Function.HookOp, HookOpSize);
			FAIL_RETURN(Status);

			PatchNop(&LocalHookBuffer[HookOpSize], CopyOpSize - HookOpSize);
		}

		TrampolineData = nullptr;
		if (Function.Options.NakedTrampoline != FALSE || (Function.Trampoline != nullptr && Function.Options.DoNotDisassemble == FALSE))
		{
			TrampolineData = AllocateTrampolineData();
			if (TrampolineData == nullptr)
				return STATUS_NO_MEMORY;

			TrampolineData->PatchData = *PatchData;
			TrampolineData->PatchData.Function.NopBytes = CopyOpSize - HookOpSize;
			TrampolineData->PatchData.Function.Source = (ULONG_PTR)Address;
			TrampolineData->PatchData.Function.Options.VirtualAddress = TRUE;

			TrampolineData->OriginSize = CopyOpSize;
			CopyMemory(TrampolineData->OriginalCode, Address, CopyOpSize);

			TrampolineData->JumpBackAddress = PtrAdd(Address, CopyOpSize);
			Trampoline = TrampolineData->Trampoline;

			if (Function.Options.NakedTrampoline == FALSE)
			{
				CopyTrampolineStub(Trampoline, Address, CopyOpSize);
				if (CopyOpSize == HookOpSize && Function.HookOp == OpCall && Function.Options.KeepRawTrampoline == FALSE)
				{
					TrampolineData->Trampoline[0] = 0xE9;
				}
				else
				{
					GenerateJumpBack(Trampoline, &TrampolineData->JumpBackAddress);
				}
			}
			else
			{
				GenerateNakedTrampoline(Trampoline, Address, CopyOpSize, TrampolineData);

				Status = GenerateHookCode(LocalHookBuffer, Address, TrampolineData->Trampoline, Function.HookOp, HookOpSize);
				if (NT_FAILED(Status))
				{
					FreeTrampolineData(TrampolineData);
					return Status;
				}

				PatchNop(&LocalHookBuffer[HookOpSize], CopyOpSize - HookOpSize);
			}

			if (Trampoline - TrampolineData->Trampoline > TRAMPOLINE_SIZE)
				RaiseDebugBreak();

			FlushInstructionCache(TrampolineData->Trampoline, TRAMPOLINE_SIZE);

			if (Function.Trampoline != nullptr)
				*Function.Trampoline = TrampolineData->Trampoline;

			this->TrampolineList.Add(TrampolineData);
		}

		Status = ProtectMemory(Address, CopyOpSize, PAGE_EXECUTE_READWRITE, &Protect);
		if (NT_FAILED(Status))
		{
			FreeTrampolineData(TrampolineData);
			return Status;
		}

		CopyMemory(Address, LocalHookBuffer, CopyOpSize);
		FlushInstructionCache(Address, CopyOpSize);

		Status = ProtectMemory(Address, CopyOpSize, Protect, &Protect);

		return STATUS_SUCCESS;
	}

	NoInline NTSTATUS GenerateHookCode(PBYTE Buffer, PVOID SourceIp, PVOID Target, ULONG_PTR HookOp, ULONG_PTR HookOpSize)
	{
		ULONG_PTR RegisterIndex;

#if ML_X86

		switch (HookOp)
		{
		case OpCall:
		case OpJump:
			//
			// jmp imm
			//
			*Buffer++ = HookOp == OpCall ? 0xE8 : 0xE9;
			*(PULONG)Buffer = PtrOffset(Target, PtrAdd(SourceIp, HookOpSize));
			break;

		case OpPush:
			//
			// push imm
			// ret
			//
			*Buffer++ = 0x68;
			*(PULONG)Buffer = (ULONG)Target;
			Buffer += POINTER_SIZE;
			*Buffer = 0xC3;
			break;

		default:
			if (HookOp < OpJRax || HookOp > OpJRdi)
				return STATUS_INVALID_PARAMETER;

			//
			// mov r32, imm
			// jmp r32
			//
			RegisterIndex = HookOp - OpJRax;
			*Buffer++ = 0xB8 + (BYTE)RegisterIndex;
			*(PVOID *)Buffer = Target;
			Buffer += POINTER_SIZE;
			*(PUSHORT)Buffer = (USHORT)(0xE0FF + RegisterIndex);
			break;
		}

#elif ML_AMD64

		switch (HookOp)
		{
		case OpPush:
			//
			// push imm.low
			// mov dword ptr [rsp + 4], imm.high
			// ret
			//
			*Buffer++ = 0x68;
			*(PULONG)Buffer = (ULONG)((ULONG_PTR)Target >> 32);
			Buffer += POINTER_SIZE;

			*Buffer++ = 0xC7;
			*Buffer++ = 0x04;
			*Buffer++ = 0x24;
			*(PULONG)Buffer = (ULONG)(ULONG_PTR)Target;

			*Buffer = 0xC3;
			break;

		default:
			if (HookOp < OpJRax || HookOp > OpJRdi)
				return STATUS_INVALID_PARAMETER;

			//
			// mov r32, imm
			// jmp r32
			//
			RegisterIndex = HookOp - OpJRax;
			*Buffer++ = 0x48;
			*Buffer++ = 0xB8 + (BYTE)RegisterIndex;
			*(PVOID *)Buffer = Target;
			Buffer += POINTER_SIZE;
			*(PUSHORT)Buffer = (USHORT)(0xE0FF + RegisterIndex);
			break;
		}

#else

		return STATUS_NOT_IMPLEMENTED;

#endif

		return STATUS_SUCCESS;
	}

	NoInline NTSTATUS GenerateNakedTrampoline(PBYTE& Trampoline, PVOID Address, ULONG_PTR CopyOpSize, PTRAMPOLINE_DATA TrampolineData)
	{
		NTSTATUS    Status;
		PVOID*      AddressOfReturnAddress;
		auto&       Function = TrampolineData->PatchData.Function;

		if (Function.Options.ExecuteTrampoline != FALSE)
		{
			Status = CopyTrampolineStub(Trampoline, Address, CopyOpSize);
			FAIL_RETURN(Status);
		}

#if ML_X86

		//
		// push     eax
		// push     esp
		// push     eax
		// push     ecx
		// push     edx
		// push     ebx
		// push     ebp
		// push     esi
		// push     edi
		// pushfd
		//
		// mov      eax, ReturnAddress
		// mov      [esp + 24h], eax
		// mov      eax, CallBack
		// lea      ecx, [esp];      ecx = PTRAMPOLINE_NAKED_CONTEXT
		// call     eax
		//
		// popfd
		// pop      edi
		// pop      esi
		// pop      ebp
		// pop      ebx
		// pop      edx
		// pop      ecx
		// pop      eax
		// pop      esp
		//
		// ret
		//

		if (Function.HookOp != OpCall)
		{
			// push eax
			*Trampoline++ = 0x50;
		}

		*Trampoline++ = 0x54;       // push    esp
		*Trampoline++ = 0x50;       // push    eax
		*Trampoline++ = 0x51;       // push    ecx
		*Trampoline++ = 0x52;       // push    edx
		*Trampoline++ = 0x53;       // push    ebx
		*Trampoline++ = 0x55;       // push    ebp
		*Trampoline++ = 0x56;       // push    esi
		*Trampoline++ = 0x57;       // push    edi
		*Trampoline++ = 0x9C;       // pushfd

		// mov eax, imm
		*Trampoline++ = 0xB8;
		*(PVOID *)Trampoline = PtrAdd(Address, CopyOpSize);
		AddressOfReturnAddress = (PVOID *)Trampoline;
		Trampoline += POINTER_SIZE;

		// mov [esp + 24h], eax
		*Trampoline++ = 0x89;
		*Trampoline++ = 0x44;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x24;

		// mov eax, CallBack
		*Trampoline++ = 0xB8;
		*(PVOID *)Trampoline = Function.Target;
		Trampoline += POINTER_SIZE;

		// lea ecx, [esp]
		*Trampoline++ = 0x8D;
		*Trampoline++ = 0x0C;
		*Trampoline++ = 0x24;

		// call eax
		*Trampoline++ = 0xFF;
		*Trampoline++ = 0xD0;

		*Trampoline++ = 0x9D;       // popfd
		*Trampoline++ = 0x5F;       // pop     edi
		*Trampoline++ = 0x5E;       // pop     esi
		*Trampoline++ = 0x5D;       // pop     ebp
		*Trampoline++ = 0x5B;       // pop     ebx
		*Trampoline++ = 0x5A;       // pop     edx
		*Trampoline++ = 0x59;       // pop     ecx
		*Trampoline++ = 0x58;       // pop     eax
		*Trampoline++ = 0x5C;       // pop     esp

		// ret
		*Trampoline++ = 0xC3;

#else

		//
		// SIZE_OF_CONTEXT equ 78h
		// push    rax
		// pushfq
		// lea     rsp, [rsp - SIZE_OF_CONTEXT]
		// mov     [rsp + 00h], rax
		// mov     [rsp + 08h], rcx
		// mov     [rsp + 10h], rdx
		// mov     [rsp + 18h], rbx
		// mov     [rsp + 20h], rbp
		// mov     [rsp + 28h], rsi
		// mov     [rsp + 30h], rdi
		// mov     [rsp + 38h], r8
		// mov     [rsp + 40h], r9
		// mov     [rsp + 48h], r10
		// mov     [rsp + 50h], r11
		// mov     [rsp + 58h], r12
		// mov     [rsp + 60h], r13
		// mov     [rsp + 68h], r14
		// mov     [rsp + 70h], r15
		// mov     rax, ReturnAddress
		// mov     [rsp + SIZE_OF_CONTEXT + 10h], rax
		// mov     rax, CallBack
		// lea     rcx, [rsp];      rcx = PTRAMPOLINE_NAKED_CONTEXT
		// lea     rsp, [rsp - 20h]
		// call    rax
		// lea     rsp, [rsp + 20h]
		// mov     rax, [rsp + 00h]
		// mov     rcx, [rsp + 08h]
		// mov     rdx, [rsp + 10h]
		// mov     rbx, [rsp + 18h]
		// mov     rbp, [rsp + 20h]
		// mov     rsi, [rsp + 28h]
		// mov     rdi, [rsp + 30h]
		// mov     r8 , [rsp + 38h]
		// mov     r9 , [rsp + 40h]
		// mov     r10, [rsp + 48h]
		// mov     r11, [rsp + 50h]
		// mov     r12, [rsp + 58h]
		// mov     r13, [rsp + 60h]
		// mov     r14, [rsp + 68h]
		// mov     r15, [rsp + 70h]
		// lea     rsp, [rsp + SIZE_OF_CONTEXT]
		// popfq
		// ret
		//

		if (Function.HookOp != OpCall)
		{
			// push rax
			*Trampoline++ = 0x50;
		}

		// pushfq
		*Trampoline++ = 0x9C;

		// lea     rsp, [rsp-78h]
		*Trampoline++ = 0x48;
		*Trampoline++ = 0x8D;
		*Trampoline++ = 0x64;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x88;

		// mov     [rsp + 00h], rax
		*Trampoline++ = 0x48;
		*Trampoline++ = 0x89;
		*Trampoline++ = 0x04;
		*Trampoline++ = 0x24;

		// mov     [rsp + 08h], rcx
		*Trampoline++ = 0x48;
		*Trampoline++ = 0x89;
		*Trampoline++ = 0x4C;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x08;

		// mov     [rsp + 10h], rdx
		*Trampoline++ = 0x48;
		*Trampoline++ = 0x89;
		*Trampoline++ = 0x54;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x10;

		// mov     [rsp + 18h], rbx
		*Trampoline++ = 0x48;
		*Trampoline++ = 0x89;
		*Trampoline++ = 0x5C;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x18;

		// mov     [rsp + 20h], rbp
		*Trampoline++ = 0x48;
		*Trampoline++ = 0x89;
		*Trampoline++ = 0x6C;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x20;

		// mov     [rsp + 28h], rsi
		*Trampoline++ = 0x48;
		*Trampoline++ = 0x89;
		*Trampoline++ = 0x74;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x28;

		// mov     [rsp + 30h], rdi
		*Trampoline++ = 0x48;
		*Trampoline++ = 0x89;
		*Trampoline++ = 0x7C;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x30;

		// mov     [rsp + 38h], r8
		*Trampoline++ = 0x4C;
		*Trampoline++ = 0x89;
		*Trampoline++ = 0x44;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x38;

		// mov     [rsp + 40h], r9
		*Trampoline++ = 0x4C;
		*Trampoline++ = 0x89;
		*Trampoline++ = 0x4C;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x40;

		// mov     [rsp + 48h], r10
		*Trampoline++ = 0x4C;
		*Trampoline++ = 0x89;
		*Trampoline++ = 0x54;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x48;

		// mov     [rsp + 50h], r11
		*Trampoline++ = 0x4C;
		*Trampoline++ = 0x89;
		*Trampoline++ = 0x5C;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x50;

		// mov     [rsp + 58h], r12
		*Trampoline++ = 0x4C;
		*Trampoline++ = 0x89;
		*Trampoline++ = 0x64;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x58;

		// mov     [rsp + 60h], r13
		*Trampoline++ = 0x4C;
		*Trampoline++ = 0x89;
		*Trampoline++ = 0x6C;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x60;

		// mov     [rsp + 68h], r14
		*Trampoline++ = 0x4C;
		*Trampoline++ = 0x89;
		*Trampoline++ = 0x74;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x68;

		// mov     [rsp + 70h], r15
		*Trampoline++ = 0x4C;
		*Trampoline++ = 0x89;
		*Trampoline++ = 0x7C;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x70;

		// mov     rax, ReturnAddress
		*Trampoline++ = 0x48;
		*Trampoline++ = 0xB8;
		*(PVOID *)Trampoline = PtrAdd(Address, CopyOpSize);
		AddressOfReturnAddress = (PVOID *)Trampoline;
		Trampoline += POINTER_SIZE;

		// mov     [rsp + SIZE_OF_CONTEXT + 10], rax
		*Trampoline++ = 0x48;
		*Trampoline++ = 0x89;
		*Trampoline++ = 0x84;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x80;
		*Trampoline++ = 0x00;
		*Trampoline++ = 0x00;
		*Trampoline++ = 0x00;

		// mov     rax, CallBack
		*Trampoline++ = 0x48;
		*Trampoline++ = 0xB8;
		*(PVOID *)Trampoline = Function.Target;
		Trampoline += POINTER_SIZE;

		// lea     rcx, [rsp];      rcx = PTRAMPOLINE_NAKED_CONTEXT
		*Trampoline++ = 0x48;
		*Trampoline++ = 0x8D;
		*Trampoline++ = 0x0C;
		*Trampoline++ = 0x24;

		// lea     rsp, [rsp - 20h]
		*Trampoline++ = 0x48;
		*Trampoline++ = 0x8D;
		*Trampoline++ = 0x64;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0xE0;

		// call    rax
		*Trampoline++ = 0xFF;
		*Trampoline++ = 0xD0;

		// lea     rsp, [rsp + 20h]
		*Trampoline++ = 0x48;
		*Trampoline++ = 0x8D;
		*Trampoline++ = 0x64;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x20;

		// mov     rax, [rsp + 00h]
		*Trampoline++ = 0x48;
		*Trampoline++ = 0x8B;
		*Trampoline++ = 0x04;
		*Trampoline++ = 0x24;

		// mov     rcx, [rsp + 08h]
		*Trampoline++ = 0x48;
		*Trampoline++ = 0x8B;
		*Trampoline++ = 0x4C;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x08;

		// mov     rdx, [rsp + 10h]
		*Trampoline++ = 0x48;
		*Trampoline++ = 0x8B;
		*Trampoline++ = 0x54;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x10;

		// mov     rbx, [rsp + 18h]
		*Trampoline++ = 0x48;
		*Trampoline++ = 0x8B;
		*Trampoline++ = 0x5C;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x18;

		// mov     rbp, [rsp + 20h]
		*Trampoline++ = 0x48;
		*Trampoline++ = 0x8B;
		*Trampoline++ = 0x6C;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x20;

		// mov     rsi, [rsp + 28h]
		*Trampoline++ = 0x48;
		*Trampoline++ = 0x8B;
		*Trampoline++ = 0x74;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x28;

		// mov     rdi, [rsp + 30h]
		*Trampoline++ = 0x48;
		*Trampoline++ = 0x8B;
		*Trampoline++ = 0x7C;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x30;

		// mov     r8 , [rsp + 38h]
		*Trampoline++ = 0x4C;
		*Trampoline++ = 0x8B;
		*Trampoline++ = 0x44;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x38;

		// mov     r9 , [rsp + 40h]
		*Trampoline++ = 0x4C;
		*Trampoline++ = 0x8B;
		*Trampoline++ = 0x4C;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x40;

		// mov     r10, [rsp + 48h]
		*Trampoline++ = 0x4C;
		*Trampoline++ = 0x8B;
		*Trampoline++ = 0x54;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x48;

		// mov     r11, [rsp + 50h]
		*Trampoline++ = 0x4C;
		*Trampoline++ = 0x8B;
		*Trampoline++ = 0x5C;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x50;

		// mov     r12, [rsp + 58h]
		*Trampoline++ = 0x4C;
		*Trampoline++ = 0x8B;
		*Trampoline++ = 0x64;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x58;

		// mov     r13, [rsp + 60h]
		*Trampoline++ = 0x4C;
		*Trampoline++ = 0x8B;
		*Trampoline++ = 0x6C;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x60;

		// mov     r14, [rsp + 68h]
		*Trampoline++ = 0x4C;
		*Trampoline++ = 0x8B;
		*Trampoline++ = 0x74;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x68;

		// mov     r15, [rsp + 70h]
		*Trampoline++ = 0x4C;
		*Trampoline++ = 0x8B;
		*Trampoline++ = 0x7C;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x70;

		// lea     rsp, [rsp + SIZE_OF_CONTEXT]
		*Trampoline++ = 0x48;
		*Trampoline++ = 0x8D;
		*Trampoline++ = 0x64;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x78;

		// popfq
		*Trampoline++ = 0x9D;

		// ret
		*Trampoline++ = 0xC3;

		//#error NOT COMPLETE

#endif // arch

		if (Function.Options.ExecuteTrampoline == FALSE)
		{
			*AddressOfReturnAddress = Trampoline;

			Status = CopyTrampolineStub(Trampoline, Address, CopyOpSize);
			FAIL_RETURN(Status);

			Status = GenerateJumpBack(Trampoline, &TrampolineData->JumpBackAddress);
			FAIL_RETURN(Status);
		}

		return STATUS_SUCCESS;
	}

	NoInline NTSTATUS GenerateJumpBack(PBYTE& Trampoline, PVOID AddressOfJumpAddress)
	{
		// jmp [imm32]
		*Trampoline++ = 0xFF;
		*Trampoline++ = 0x25;

		*(PVOID *)Trampoline = AddressOfJumpAddress;
		Trampoline += 2 + 4;

		return STATUS_SUCCESS;
	}

	NoInline NTSTATUS CopyTrampolineStub(PBYTE& Trampoline, PVOID Address, ULONG_PTR CopyOpSize)
	{
		ULONG_PTR SourceOpLength, ForwardSize, BackwardSize;
		PBYTE Source = (PBYTE)Address;

		ForwardSize = CopyOpSize;
		BackwardSize = 0;

		for (LONG_PTR Bytes = CopyOpSize; Bytes > 0; Bytes -= SourceOpLength)
		{
			ULONG_PTR TargetOpLength;

			if (Source[0] == 0xC2 || Source[0] == 0xC3)
				return STATUS_BUFFER_TOO_SMALL;

			CopyOneOpCode(Trampoline, Source, &TargetOpLength, &SourceOpLength, ForwardSize, BackwardSize);

			BackwardSize += SourceOpLength;
			ForwardSize -= SourceOpLength;
			Trampoline += TargetOpLength;
			Source += SourceOpLength;
		}

		return STATUS_SUCCESS;
	}

	NoInline NTSTATUS GetHookAddressAndSize(PVOID Address, ULONG_PTR HookOpSize, PVOID *FinalAddress, PULONG_PTR Size)
	{
		ULONG_PTR   Length, OpSize;
		PBYTE       Buffer;

		OpSize = 0;
		Buffer = (PBYTE)Address;
		*FinalAddress = Address;

#if ML_X86 || ML_AMD64

		while (OpSize < HookOpSize)
		{
			if (Buffer[0] == 0xC2 || Buffer[0] == 0xC3)
				return STATUS_BUFFER_TOO_SMALL;

			Length = GetOpCodeSize(Buffer);

			// jmp short const
			if (Buffer[0] == 0xEB)
			{
				if (OpSize == 0)
				{
					if (Buffer[1] != 0)
					{
						Buffer += *(PCHAR)&Buffer[1] + Length;
						*FinalAddress = Buffer;
						continue;
					}
				}
				else if (OpSize < HookOpSize - Length)
				{
					return STATUS_BUFFER_TOO_SMALL;
				}
			}
			else if (Buffer[0] == 0xFF && Buffer[1] == 0x25)
			{
				// jmp [rimm]

#if ML_X86
				if (OpSize != 0 && OpSize + Length < HookOpSize)
					return STATUS_BUFFER_TOO_SMALL;

				if (OpSize == 0 && HookOpSize > Length)
				{
					Buffer = **(PBYTE **)&Buffer[2];
					*FinalAddress = Buffer;
					continue;
				}
#elif ML_AMD64
				if (OpSize != 0)
					return STATUS_BUFFER_TOO_SMALL;

				Buffer = *(PBYTE *)(Buffer + Length + *(PLONG)&Buffer[2]);
				*FinalAddress = Buffer;
				continue;
#endif // arch

			}

			Buffer += Length;
			OpSize += Length;
		}

#else

#error "not support"

#endif // arch

		*Size = OpSize;

		return STATUS_SUCCESS;
	}

	NoInline ULONG_PTR GetSizeOfHookOpSize(ULONG_PTR HookOp)
	{

#if ML_AMD64

		switch (HookOp)
		{
		case OpPush:
			// 68 00 00 00 80       push    80000000h
			// C7 04 24 00 00 00 80 mov     dword ptr [rsp], 80000000h
			// C3                   ret
			return 0xD;

			//case OpJumpIndirect:
			//    //
			//    // FF 25 00 00 00 00 jmp [rimm]
			//    // 00 00 00 00 00 00 00 00
			//    //
			//    break;
			//    //return 0xE;

		default:
			if (HookOp >= OpJRax && HookOp <= OpJRdi)
			{
				//
				// 48 B8 00 00 00 80 00 00 00 80    mov     rax, 8000000080000000h
				// FF E0                            jmp     rax
				//
				return 0xC;
			}
		}

#elif ML_X86

		switch (HookOp)
		{
		case OpPush:
			//
			// 68 00000000  push const
			// C3           ret
			//
			return 1 + POINTER_SIZE + 1;

		case OpCall:
		case OpJump:
			//
			// E8 00000000  call    const
			// E9 00000000  jmp     const
			//
			return 1 + POINTER_SIZE;

		default:
			if (HookOp >= OpJRax && HookOp <= OpJRdi)
			{
				//
				// b8 88888888      mov     r32, const
				// ffe0             jmp     r32
				//
				return 1 + POINTER_SIZE + 2;
			}
		}

#endif

		return ULONG_PTR_MAX;
	}

	ForceInline
		NTSTATUS
		ProtectMemory(
			PVOID       BaseAddress,
			ULONG_PTR   Size,
			ULONG       NewProtect,
			PULONG      OldProtect
		)
	{
		return Mm::ProtectMemory(CurrentProcess, BaseAddress, Size, NewProtect, OldProtect);
	}

	ForceInline
		NTSTATUS
		FlushInstructionCache(
			PVOID       BaseAddress,
			ULONG_PTR   NumberOfBytesToFlush
		)
	{
		return NtFlushInstructionCache(CurrentProcess, BaseAddress, NumberOfBytesToFlush);
	}

	PTRAMPOLINE_DATA AllocateTrampolineData()
	{
		return (PTRAMPOLINE_DATA)RtlAllocateHeap(this->ExecutableHeap, HEAP_ZERO_MEMORY, sizeof(TRAMPOLINE_DATA));
	}

	VOID FreeTrampolineData(PTRAMPOLINE_DATA TrampolineData)
	{
		if (TrampolineData != nullptr)
			RtlFreeHeap(this->ExecutableHeap, 0, TrampolineData);
	}

	NTSTATUS RemoveTrampolineData(PTRAMPOLINE_DATA TrampolineData)
	{
		NTSTATUS Status;
		ULONG_PTR Index;

		Index = this->TrampolineList.IndexOf(TrampolineData);
		if (Index == this->TrampolineList.kInvalidIndex)
			return STATUS_NOT_FOUND;

		this->TrampolineList.Remove(Index);
		FreeTrampolineData(TrampolineData);

		return STATUS_SUCCESS;
	}

	DECL_NORETURN VOID RaiseDebugBreak()
	{
		__debugbreak();
	}

public:
	NoInline NTSTATUS PatchMemory(PPATCH_MEMORY_DATA PatchData, ULONG_PTR PatchCount, PVOID BaseAddress)
	{
		NTSTATUS Status;

		FOR_EACH(PatchData, PatchData, PatchCount)
		{
			switch (PatchData->PatchType)
			{
			case PatchMemoryTypes::MemoryPatch:
				Status = HandleMemoryPatch(PatchData, BaseAddress);
				break;

			case PatchMemoryTypes::FunctionPatch:
				Status = HandleFunctionPatch(PatchData, BaseAddress);
				break;

			default:
				Status = STATUS_NOT_IMPLEMENTED;
				break;
			}

			FAIL_RETURN(Status);
		}

		return STATUS_SUCCESS;
	}

	NTSTATUS RestoreMemory(PTRAMPOLINE_DATA TrampolineData)
	{
		NTSTATUS Status;
		PATCH_MEMORY_DATA PatchData = MemoryPatchVa(&TrampolineData->OriginalCode, TrampolineData->OriginSize, TrampolineData->PatchData.Function.Source);
		Status = HandleMemoryPatch(&PatchData, nullptr);
		NT_SUCCESS(Status) && RemoveTrampolineData(TrampolineData);

		return Status;
	}

public:
	static NoInline NTSTATUS CreateInstance(MemoryPatchManager **Manager)
	{
		ml::MlInitialize();

		*Manager = (MemoryPatchManager *)AllocateMemory(sizeof(**Manager));
		if (*Manager == nullptr)
			return STATUS_NO_MEMORY;

		ZeroMemory(*Manager, sizeof(**Manager));
		new (*Manager) MemoryPatchManager;

		return STATUS_SUCCESS;
	}

	static NoInline NTSTATUS DestroyInstance(MemoryPatchManager *Manager)
	{
		if (Manager != nullptr)
		{
			Manager->~MemoryPatchManager();
			FreeMemory(Manager);
		}

		return STATUS_SUCCESS;
	}
};

ForceInline MemoryPatchManager*& MemoryPatchManagerInstance()
{
	static MemoryPatchManager *Manager;
	return Manager;
}

NTSTATUS MP_CALL PatchMemory(PPATCH_MEMORY_DATA PatchData, ULONG_PTR PatchCount, PVOID BaseAddress)
{
	NTSTATUS Status;

	if (MemoryPatchManagerInstance() == nullptr)
	{
		Status = MemoryPatchManager::CreateInstance(&MemoryPatchManagerInstance());
		FAIL_RETURN(Status);
	}

	return MemoryPatchManagerInstance()->PatchMemory(PatchData, PatchCount, BaseAddress);
}

NTSTATUS MP_CALL RestoreMemory(PTRAMPOLINE_DATA TrampolineData)
{
	if (MemoryPatchManagerInstance() == nullptr)
		return STATUS_FLT_NOT_INITIALIZED;

	return MemoryPatchManagerInstance()->RestoreMemory(TrampolineData);
}

#else

//
// DetoursNT
//

NTSTATUS MP_CALL PatchMemory(PPATCH_MEMORY_DATA PatchData, ULONG_PTR PatchCount, PVOID BaseAddress)
{
	return STATUS_NOT_SUPPORTED;
}

NTSTATUS MP_CALL RestoreMemory(PTRAMPOLINE_DATA TrampolineData)
{
	return STATUS_NOT_SUPPORTED;
}

#endif //ML_AMD || ML_X86

ML_NAMESPACE_END_(Mp);

#pragma pop_macro("POINTER_SIZE")

ML_NAMESPACE

ML_NAMESPACE_BEGIN(Native)


ML_NAMESPACE_BEGIN(Ldr)

/************************************************************************
UserMode
************************************************************************/

#if ML_USER_MODE

LDR_MODULE* FindLdrModuleByName(PUNICODE_STRING ModuleName)
{
	LDR_MODULE  *Ldr;
	PLIST_ENTRY LdrLink, NextLink;
	PVOID       LoaderLockCookie;
	NTSTATUS    Status;

	Status = LdrLockLoaderLock(0, NULL, &LoaderLockCookie);

	LdrLink = &Ps::CurrentPeb()->Ldr->InLoadOrderModuleList;
	NextLink = LdrLink->Flink;

	if (ModuleName == NULL)
	{
		if (NT_SUCCESS(Status))
			LdrUnlockLoaderLock(0, LoaderLockCookie);

		return FIELD_BASE(NextLink, LDR_MODULE, InLoadOrderLinks);
	}

	while (NextLink != LdrLink)
	{
		Ldr = FIELD_BASE(NextLink, LDR_MODULE, InLoadOrderLinks);

		if (RtlEqualUnicodeString(ModuleName, &Ldr->BaseDllName, TRUE))
		{
			if (NT_SUCCESS(Status))
				LdrUnlockLoaderLock(0, LoaderLockCookie);

			return Ldr;
		}

		NextLink = NextLink->Flink;
	}

	if (NT_SUCCESS(Status))
		LdrUnlockLoaderLock(0, LoaderLockCookie);

	return NULL;
}

LDR_MODULE* FindLdrModuleByHandle(PVOID BaseAddress)
{
	LDR_MODULE  *Ldr;
	PLIST_ENTRY LdrLink, NextLink;

	if (BaseAddress != NULL)
	{
		NTSTATUS Status;

		Status = LdrFindEntryForAddress(BaseAddress, &Ldr);
		return NT_SUCCESS(Status) ? Ldr : NULL;
	}

	LdrLink = &Nt_CurrentPeb()->Ldr->InLoadOrderModuleList;
	NextLink = LdrLink->Flink;

	return FIELD_BASE(NextLink, LDR_MODULE, InLoadOrderLinks);
}

#endif // r3

ForceInline PVOID GetModuleBase(PVOID Module)
{
#if ML_USER_MODE
	Module = FindLdrModuleByHandle(Module)->DllBase;
#endif

	return Module;
}

PIMAGE_RESOURCE_DATA_ENTRY FindResource(PVOID Module, PCWSTR Name, PCWSTR Type)
{
	NTSTATUS                    Status;
	LDR_RESOURCE_INFO           ResourceIdPath;
	PIMAGE_RESOURCE_DATA_ENTRY  ResourceDataEntry;

	ResourceIdPath.Type = (ULONG_PTR)Type;
	ResourceIdPath.Name = (ULONG_PTR)Name;
	ResourceIdPath.Language = 0;

	Status = LdrFindResource_U(GetModuleBase(Module), &ResourceIdPath, sizeof(ResourceIdPath), &ResourceDataEntry);

	return NT_SUCCESS(Status) ? ResourceDataEntry : NULL;
}

PVOID LoadResource(PVOID Module, PIMAGE_RESOURCE_DATA_ENTRY ResourceDataEntry, PULONG Size)
{
	PVOID       Address;
	NTSTATUS    Status;

	Status = LdrAccessResource(GetModuleBase(Module), ResourceDataEntry, &Address, Size);

	return NT_SUCCESS(Status) ? Address : NULL;
}


/************************************************************************
load pe image
************************************************************************/

#define LOAD_PE_MEMORY_TAG   TAG4('LPMT')

NTSTATUS
RelocBlock(
	PVOID                   ImageBase,
	LONG64                  ImageBaseOffset,
	PIMAGE_BASE_RELOCATION2 Relocation
)
{
	LONG        SizeOfBlock;
	PVOID       RelocateBase;
	PIMAGE_RELOCATION_ADDRESS_ENTRY TypeOffset;

	TypeOffset = Relocation->TypeOffset;
	SizeOfBlock = Relocation->SizeOfBlock;
	RelocateBase = PtrAdd(ImageBase, Relocation->VirtualAddress);
	SizeOfBlock -= sizeof(*Relocation) - sizeof(Relocation->TypeOffset);

	for (; SizeOfBlock > 0; ++TypeOffset, SizeOfBlock -= sizeof(*TypeOffset))
	{
		if (*(PUSHORT)TypeOffset == 0)
			continue;

		PLONG_PTR VA = (PLONG_PTR)PtrAdd(RelocateBase, TypeOffset->Offset);
		switch (TypeOffset->Type)
		{
		case IMAGE_REL_BASED_ABSOLUTE:
			break;

		case IMAGE_REL_BASED_HIGH:
			*(PUSHORT)VA += HIWORD((ULONG)ImageBaseOffset);
			break;

		case IMAGE_REL_BASED_LOW:
			*(PUSHORT)VA += LOWORD((ULONG)ImageBaseOffset);
			break;

		case IMAGE_REL_BASED_HIGHLOW:
			*(PLONG)VA += (ULONG)ImageBaseOffset;
			break;

		case IMAGE_REL_BASED_HIGHADJ:
			++TypeOffset;
			SizeOfBlock -= sizeof(*TypeOffset);

			*(PUSHORT)VA += ((*(PUSHORT)VA << 16) + (ULONG)ImageBaseOffset + *(PSHORT)TypeOffset + 0x8000) >> 16;
			break;

		case IMAGE_REL_BASED_MIPS_JMPADDR:
		case IMAGE_REL_BASED_IA64_IMM64:
			break;

		case IMAGE_REL_BASED_DIR64:
			*(PLONG64)VA += (LONG64)ImageBaseOffset;
			break;

		default:
			break;
		}
	}

	return STATUS_SUCCESS;
}

NTSTATUS
RelocPeImage(
	PVOID   ImageBuffer,
	PVOID64 ImageDefaultBase,
	PVOID   ImageOldBase,
	PVOID   ImageNewBase
)
{
	LONG                            SizeOfBlock;
	PBYTE                           Base, RelocateBase;
	LONG64                          BaseOffset;
	ULONG                           SizeOfRelocation;
	PIMAGE_BASE_RELOCATION2         Relocation, RelocationEnd;
	PIMAGE_RELOCATION_ADDRESS_ENTRY TypeOffset;
	PIMAGE_DATA_DIRECTORY           DataDirectory;
	PIMAGE_NT_HEADERS32             NtHeaders32;
	PIMAGE_NT_HEADERS64             NtHeaders64;

	if (ImageNewBase == nullptr)
		ImageNewBase = ImageBuffer;

	NtHeaders32 = (PIMAGE_NT_HEADERS32)RtlImageNtHeader(ImageBuffer);

	switch (NtHeaders32->OptionalHeader.Magic)
	{
	case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
		DataDirectory = NtHeaders32->OptionalHeader.DataDirectory;
		break;

	case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
		NtHeaders64 = (PIMAGE_NT_HEADERS64)NtHeaders32;
		DataDirectory = NtHeaders64->OptionalHeader.DataDirectory;
		break;

	default:
		return STATUS_INVALID_IMAGE_FORMAT;
	}

	Relocation = (PIMAGE_BASE_RELOCATION2)PtrAdd(ImageBuffer, DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
	SizeOfRelocation = DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;

	if ((PVOID)Relocation == ImageBuffer || SizeOfRelocation == 0)
		return STATUS_SUCCESS;

	RelocationEnd = PtrAdd(Relocation, SizeOfRelocation);

	Base = (PBYTE)ImageBuffer;
	BaseOffset = PtrSub(ImageOldBase == nullptr ? (LONG64)ImageNewBase : (LONG64)ImageOldBase, (LONG64)ImageDefaultBase);

	while (Relocation < RelocationEnd && Relocation->SizeOfBlock != 0)
	{
		RelocBlock(Base, BaseOffset, Relocation);
		Relocation = PtrAdd(Relocation, Relocation->SizeOfBlock);
	}

	return STATUS_SUCCESS;
}

#if ML_KERNEL_MODE

NTSTATUS ProcessStaticImport32(PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor, ULONG_PTR BaseAddress, ULONG_PTR Flags)
{
	return STATUS_NOT_IMPLEMENTED;
}

#else // user mode

NTSTATUS ProcessStaticImport32(PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor, ULONG_PTR BaseAddress, ULONG_PTR Flags)
{
	ULONG_PTR           Length;
	PVOID               DllBase;
	PCSTR               DllName;
	WCHAR               DllPathBuffer[MAX_NTPATH];
	UNICODE_STRING      DllPath;
	NTSTATUS            Status;
	PIMAGE_THUNK_DATA   OriginalThunk, FirstThunk;

	DllPath.Buffer = DllPathBuffer;
	DllPath.MaximumLength = sizeof(DllPathBuffer);
	for (; ImportDescriptor->Name != NULL && ImportDescriptor->FirstThunk != NULL; ++ImportDescriptor)
	{
		DllName = (PCSTR)BaseAddress + ImportDescriptor->Name;
		Nls::AnsiToUnicode(DllPathBuffer, countof(DllPathBuffer), DllName, -1, &Length);
		DllPath.Length = (USHORT)Length;
		Status = LdrLoadDll(NULL, 0, &DllPath, &DllBase);
		if (!NT_SUCCESS(Status))
		{
			if (FLAG_ON(Flags, LOAD_PE_DLL_NOT_FOUND_CONTINUE))
				continue;

			return Status;
		}

		OriginalThunk = (PIMAGE_THUNK_DATA)BaseAddress;
		if (ImportDescriptor->OriginalFirstThunk)
			OriginalThunk = PtrAdd(OriginalThunk, ImportDescriptor->OriginalFirstThunk);
		else
			OriginalThunk = PtrAdd(OriginalThunk, ImportDescriptor->FirstThunk);

		FirstThunk = (PIMAGE_THUNK_DATA)(BaseAddress + ImportDescriptor->FirstThunk);
		while (OriginalThunk->u1.AddressOfData != NULL)
		{
			LONG_PTR FunctionName;

			FunctionName = OriginalThunk->u1.AddressOfData;
			FunctionName = (FunctionName < 0) ? (USHORT)FunctionName : (FunctionName + BaseAddress + 2);
			*(PVOID *)&FirstThunk->u1.Function = GetRoutineAddress(DllBase, (PCSTR)FunctionName);
			++OriginalThunk;
			++FirstThunk;
		}
	}

	return STATUS_SUCCESS;
}

NTSTATUS ProcessStaticImport64(PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor, ULONG_PTR BaseAddress, ULONG_PTR Flags)
{
	ULONG_PTR           Length;
	PVOID               DllBase;
	PCSTR               DllName;
	WCHAR               DllPathBuffer[MAX_NTPATH];
	UNICODE_STRING      DllPath;
	NTSTATUS            Status;
	PIMAGE_THUNK_DATA64 OriginalThunk, FirstThunk;

	DllPath.Buffer = DllPathBuffer;
	DllPath.MaximumLength = sizeof(DllPathBuffer);
	for (; ImportDescriptor->Name != NULL && ImportDescriptor->FirstThunk != NULL; ++ImportDescriptor)
	{
		DllName = (PCSTR)BaseAddress + ImportDescriptor->Name;
		Nls::AnsiToUnicode(DllPathBuffer, countof(DllPathBuffer), DllName, -1, &Length);
		DllPath.Length = (USHORT)Length;
		Status = LdrLoadDll(NULL, 0, &DllPath, &DllBase);
		if (!NT_SUCCESS(Status))
			return Status;

		OriginalThunk = (PIMAGE_THUNK_DATA64)BaseAddress;
		if (ImportDescriptor->OriginalFirstThunk)
			OriginalThunk = PtrAdd(OriginalThunk, ImportDescriptor->OriginalFirstThunk);
		else
			OriginalThunk = PtrAdd(OriginalThunk, ImportDescriptor->FirstThunk);

		FirstThunk = (PIMAGE_THUNK_DATA64)(BaseAddress + ImportDescriptor->FirstThunk);
		while (OriginalThunk->u1.AddressOfData != NULL)
		{
			LONG_PTR FunctionName;

			FunctionName = (LONG_PTR)OriginalThunk->u1.AddressOfData;
			FunctionName = (FunctionName < 0) ? (USHORT)FunctionName : (FunctionName + BaseAddress + 2);
			*(PVOID *)&FirstThunk->u1.Function = GetRoutineAddress(DllBase, FunctionName);
			++OriginalThunk;
			++FirstThunk;
		}
	}

	return STATUS_SUCCESS;
}

#endif // ML_KERNEL_MODE

NTSTATUS
LoadPeImageWorker(
	PCWSTR      FullDllPath,
	PVOID*      DllBaseAddress,
	PVOID       OldBaseAddress,
	ULONG_PTR   Flags
)
{
	ULONG_PTR                   SizeOfHeaders, FileAlignment, SectionAlignment, SizeOfImage;
	PVOID64                     ImageBase;
	NTSTATUS                    Status;
	NtFileDisk                  file;
	IMAGE_DOS_HEADER            DosHeader;
	PIMAGE_OPTIONAL_HEADER      OptionalHeader32;
	PIMAGE_OPTIONAL_HEADER64    OptionalHeader64;
	PIMAGE_SECTION_HEADER       SectionHeader, Section;
	PIMAGE_IMPORT_DESCRIPTOR    ImportDescriptor;
	PIMAGE_DATA_DIRECTORY       DataDirectory;
	PBYTE                       Buffer;
	PVOID                       BaseAddress;

	union
	{
		IMAGE_NT_HEADERS            NtHeaders;
		IMAGE_NT_HEADERS64          NtHeaders64;
	};

	*DllBaseAddress = NULL;

	Status = file.Open(FullDllPath, FLAG_ON(Flags, LOAD_PE_NOT_RESOLVE_PATH) ? NFD_NOT_RESOLVE_PATH : 0);

	if (!NT_SUCCESS(Status))
		return Status;

	Status = file.Read(&DosHeader, sizeof(DosHeader));
	if (!NT_SUCCESS(Status))
		return Status;

	if (DosHeader.e_magic != IMAGE_DOS_SIGNATURE)
		return STATUS_INVALID_IMAGE_FORMAT;

	Status = file.Seek(DosHeader.e_lfanew, FILE_BEGIN);
	if (!NT_SUCCESS(Status))
		return Status;

	Status = file.Read(&NtHeaders, sizeof(NtHeaders) - sizeof(NtHeaders.OptionalHeader));
	if (!NT_SUCCESS(Status))
		return Status;

	if (NtHeaders.Signature != IMAGE_NT_SIGNATURE)
		return STATUS_INVALID_IMAGE_FORMAT;

	switch (NtHeaders.FileHeader.Machine)
	{
	case IMAGE_FILE_MACHINE_I386:
		OptionalHeader32 = (PIMAGE_OPTIONAL_HEADER)AllocStack(NtHeaders.FileHeader.SizeOfOptionalHeader);
		OptionalHeader64 = NULL;
		Status = file.Read(OptionalHeader32, NtHeaders.FileHeader.SizeOfOptionalHeader);
		break;

	case IMAGE_FILE_MACHINE_AMD64:
		OptionalHeader32 = NULL;
		OptionalHeader64 = (PIMAGE_OPTIONAL_HEADER64)AllocStack(NtHeaders64.FileHeader.SizeOfOptionalHeader);
		Status = file.Read(OptionalHeader64, NtHeaders64.FileHeader.SizeOfOptionalHeader);
		break;

	default:
		return STATUS_INVALID_IMAGE_FORMAT;
	}

	if (!NT_SUCCESS(Status))
		return Status;

	SectionHeader = (PIMAGE_SECTION_HEADER)AllocStack(NtHeaders.FileHeader.NumberOfSections * sizeof(*SectionHeader));
	Status = file.Read(SectionHeader, NtHeaders.FileHeader.NumberOfSections * sizeof(*SectionHeader));
	if (!NT_SUCCESS(Status))
		return Status;

	if (OptionalHeader32 != NULL)
	{
		SizeOfImage = OptionalHeader32->SizeOfImage;
		SizeOfHeaders = OptionalHeader32->SizeOfHeaders;
		FileAlignment = OptionalHeader32->FileAlignment;
		SectionAlignment = OptionalHeader32->SectionAlignment;
		ImageBase = (PVOID64)OptionalHeader32->ImageBase;
		DataDirectory = OptionalHeader32->DataDirectory;
	}
	else
	{
		SizeOfImage = OptionalHeader64->SizeOfImage;
		SizeOfHeaders = OptionalHeader64->SizeOfHeaders;
		FileAlignment = OptionalHeader64->FileAlignment;
		SectionAlignment = OptionalHeader64->SectionAlignment;
		ImageBase = (PVOID64)OptionalHeader64->ImageBase;
		DataDirectory = OptionalHeader64->DataDirectory;
	}

#if ML_KERNEL_MODE

	PMDL                Mdl;
	PHYSICAL_ADDRESS    LowAddress, HighAddress, SkipBytes;

	LowAddress.QuadPart = 0;
	HighAddress.QuadPart = -1;
	SkipBytes.QuadPart = 0;

	BaseAddress = MmAllocateMappingAddress(SizeOfImage, LOAD_PE_MEMORY_TAG);
	if (BaseAddress == NULL)
		return STATUS_NO_MEMORY;

	Mdl = MmAllocatePagesForMdl(LowAddress, HighAddress, SkipBytes, SizeOfImage);
	if (Mdl == NULL)
	{
		MmFreeMappingAddress(BaseAddress, LOAD_PE_MEMORY_TAG);
		return STATUS_NO_MEMORY;
	}

	BaseAddress = MmMapLockedPagesWithReservedMapping(BaseAddress, LOAD_PE_MEMORY_TAG, Mdl, MmCached);
	if (BaseAddress == NULL)
	{
		MmFreePagesFromMdl(Mdl);
		ExFreePool(Mdl);
		MmFreeMappingAddress(BaseAddress, LOAD_PE_MEMORY_TAG);

		return STATUS_NO_MEMORY;
	}

	*(PMDL *)&DosHeader.e_res2 = Mdl;

	*DllBaseAddress = BaseAddress;

	Status = MmProtectMdlSystemAddress(Mdl, PAGE_EXECUTE_READWRITE);
	FAIL_RETURN(Status);

#else // r3

	Status = Mm::AllocVirtualMemory(DllBaseAddress, SizeOfImage, PAGE_EXECUTE_READWRITE, MEM_RESERVE | MEM_COMMIT | (FLAG_ON(Flags, LOAD_PE_TOP_TO_DOWN) ? MEM_TOP_DOWN : 0));
	if (!NT_SUCCESS(Status))
		return Status;

	BaseAddress = *DllBaseAddress;

#endif

	FAIL_RETURN(file.Seek(0ll, FILE_BEGIN));
	FAIL_RETURN(file.Read(BaseAddress, SizeOfHeaders));

	Section = SectionHeader;
	for (ULONG_PTR NumberOfSection = NtHeaders.FileHeader.NumberOfSections; NumberOfSection; ++Section, --NumberOfSection)
	{
		ULONG_PTR PointerToRawData, SizeOfRawData, VirtualAddress;

		SizeOfRawData = Section->SizeOfRawData;
		SizeOfRawData = ROUND_UP(SizeOfRawData, FileAlignment);
		if (SizeOfRawData == 0)
			continue;

		PointerToRawData = Section->PointerToRawData;
		PointerToRawData = ROUND_DOWN(PointerToRawData, FileAlignment);
		Status = file.Seek(PointerToRawData, FILE_BEGIN);
		if (!NT_SUCCESS(Status))
			return Status;

		VirtualAddress = Section->VirtualAddress;
		VirtualAddress = ROUND_DOWN(VirtualAddress, SectionAlignment);
		Buffer = (PBYTE)BaseAddress + VirtualAddress;
		Status = file.Read(Buffer, SizeOfRawData);
		if (!NT_SUCCESS(Status))
			return Status;
	}

	Buffer = (PBYTE)BaseAddress;

	if (!FLAG_ON(Flags, LOAD_PE_IGNORE_RELOC)) LOOP_ONCE
	{
		if (!ValidateDataDirectory(&DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC], SizeOfImage))
		break;

		Status = RelocPeImage(Buffer, ImageBase, OldBaseAddress);
	}

#if !ML_KERNEL_MODE

		if (!FLAG_ON(Flags, LOAD_PE_IGNORE_IAT) &&
			ValidateDataDirectory(&DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT], SizeOfImage))
		{
			ImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)(DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress + Buffer);

			switch (NtHeaders.FileHeader.Machine)
			{
			case IMAGE_FILE_MACHINE_I386:
				Status = ProcessStaticImport32(ImportDescriptor, (ULONG_PTR)Buffer, Flags);
				break;

			case IMAGE_FILE_MACHINE_AMD64:
				Status = ProcessStaticImport64(ImportDescriptor, (ULONG_PTR)Buffer, Flags);
				break;
			}
		}

#else

	Status = STATUS_SUCCESS;

#endif // r3

	return Status;
}

NTSTATUS UnloadPeImage(PVOID DllBase)
{
	if (DllBase == NULL)
		return STATUS_INVALID_ADDRESS;

#if ML_KERNEL_MODE

	PIMAGE_DOS_HEADER   DosHeader;
	PMDL                Mdl;

	DosHeader = (PIMAGE_DOS_HEADER)DllBase;
	Mdl = *(PMDL *)&DosHeader->e_res2;

	if (Mdl == NULL)
		return STATUS_UNSUCCESSFUL;

	MmUnmapReservedMapping(DllBase, LOAD_PE_MEMORY_TAG, Mdl);
	MmFreePagesFromMdl(Mdl);
	ExFreePool(Mdl);
	MmFreeMappingAddress(DllBase, LOAD_PE_MEMORY_TAG);

	return STATUS_SUCCESS;

#else // r3

	return Mm::FreeVirtualMemory(DllBase);

#endif // ML_KERNEL_MODE
}

NTSTATUS
LoadPeImage(
	PCWSTR      FullDllPath,
	PVOID*      DllBaseAddress,
	PVOID       OldBaseAddress,
	ULONG_PTR   Flags
)
{
	NTSTATUS Status;

	Status = LoadPeImageWorker(FullDllPath, DllBaseAddress, OldBaseAddress, Flags);
	if (!NT_SUCCESS(Status) && *DllBaseAddress != NULL)
		UnloadPeImage(*DllBaseAddress);

	return Status;
}

PVOID FASTCALL GetRoutineAddress(PVOID ModuleBase, LPCSTR RoutineName)
{

#if ML_USER_MODE

	PVOID       ProcAddress;
	LONG        Ordinal;
	NTSTATUS    Status;
	ANSI_STRING ProcString, *ProcName;

	if ((ULONG_PTR)RoutineName > 0xFFFF)
	{
		RtlInitAnsiString(&ProcString, RoutineName);
		ProcName = &ProcString;
		Ordinal = 0;
	}
	else
	{
		ProcName = NULL;
		Ordinal = (LONG)(LONG_PTR)RoutineName;
	}

	Status = LdrGetProcedureAddress(ModuleBase, ProcName, (USHORT)Ordinal, &ProcAddress);
	if (!NT_SUCCESS(Status))
		return NULL;

	return ProcAddress == (PVOID)ModuleBase ? NULL : (PVOID)ProcAddress;

#else

	return 0;

#endif

}

#if ML_USER_MODE

PVOID
LoadDll(
	PCWSTR ModuleFileName
)
{
	PVOID           ModuleBase;
	NTSTATUS        Status;
	UNICODE_STRING  ModuleFile;

	RtlInitUnicodeString(&ModuleFile, ModuleFileName);

	Status = LdrLoadDll(NULL, NULL, &ModuleFile, &ModuleBase);

	return NT_SUCCESS(Status) ? ModuleBase : NULL;
}

NTSTATUS
UnloadDll(
	PVOID DllHandle
)
{
	return LdrUnloadDll(DllHandle);
}

#endif // r3

ML_NAMESPACE_END_(Ldr);

ML_NAMESPACE_BEGIN(Ps);

using namespace Mm;
using namespace Ldr;

ULONG_PTR HandleToProcessId(HANDLE Process)
{
#if ML_USER_MODE

	NTSTATUS Status;
	PROCESS_BASIC_INFORMATION Basic;

	Status = NtQueryInformationProcess(Process, ProcessBasicInformation, &Basic, sizeof(Basic), nullptr);

	return NT_SUCCESS(Status) ? (ULONG_PTR)Basic.UniqueProcessId : INVALID_CLIENT_ID;

#else // r0
#endif
}

ULONG_PTR HandleToThreadId(HANDLE Thread)
{
#if ML_USER_MODE

	NTSTATUS Status;
	THREAD_BASIC_INFORMATION Basic;

	Status = NtQueryInformationThread(Thread, ThreadBasicInformation, &Basic, sizeof(Basic), nullptr);

	return NT_SUCCESS(Status) ? (ULONG_PTR)Basic.ClientId.UniqueThread : INVALID_CLIENT_ID;

#else // r0
#endif
}

NTSTATUS ProcessIdToHandleEx(PHANDLE ProcessHandle, ULONG_PTR ProcessId, ULONG_PTR Access /* = PROCESS_ALL_ACCESS */)
{
#if ML_USER_MODE

	NTSTATUS            Status;
	OBJECT_ATTRIBUTES   ObjectAttributes;
	CLIENT_ID           CliendID;

	CliendID.UniqueThread = nullptr;
	CliendID.UniqueProcess = (HANDLE)ProcessId;
	InitializeObjectAttributes(&ObjectAttributes, nullptr, 0, nullptr, nullptr);

	Status = NtOpenProcess(ProcessHandle, Access, &ObjectAttributes, &CliendID);

	return Status;

#else // r0

	PEPROCESS   Process;
	NTSTATUS    Status;

	Status = PsLookupProcessByProcessId((HANDLE)ProcessId, &Process);
	FAIL_RETURN(Status);

	Status = ObOpenObjectByPointer(Process, 0, nullptr, Access, *PsProcessType, KernelMode, ProcessHandle);
	ObDereferenceObject(Process);

	return Status;

#endif
}

HANDLE ProcessIdToHandle(ULONG_PTR ProcessId, ULONG_PTR Access /* = PROCESS_ALL_ACCESS */)
{
	NTSTATUS    Status;
	HANDLE      ProcessHandle;

	Status = ProcessIdToHandleEx(&ProcessHandle, ProcessId, Access);
	return NT_SUCCESS(Status) ? ProcessHandle : nullptr;
}

NTSTATUS ThreadIdToHandleEx(PHANDLE ThreadHandle, ULONG_PTR ThreadId, ULONG_PTR Access)
{
#if ML_USER_MODE

	NTSTATUS            Status;
	OBJECT_ATTRIBUTES   ObjectAttributes;
	CLIENT_ID           CliendID;

	CliendID.UniqueProcess = NULL;
	CliendID.UniqueThread = (HANDLE)ThreadId;
	InitializeObjectAttributes(&ObjectAttributes, NULL, NULL, NULL, NULL);

	return NtOpenThread(ThreadHandle, Access, &ObjectAttributes, &CliendID);

#else // r0

	PETHREAD    Thread;
	HANDLE      Handle;
	NTSTATUS    Status;

	Status = PsLookupThreadByThreadId((HANDLE)ThreadId, &Thread);
	FAIL_RETURN(Status);

	Status = ObOpenObjectByPointer(Thread, 0, nullptr, Access, *PsThreadType, KernelMode, ThreadHandle);
	ObDereferenceObject(Thread);

	return Status;

#endif
}

HANDLE ThreadIdToHandle(ULONG_PTR ThreadId, ULONG_PTR Access)
{
	NTSTATUS Status;
	HANDLE Handle;

	Status = ThreadIdToHandleEx(&Handle, ThreadId, Access);
	return NT_SUCCESS(Status) ? Handle : nullptr;
}

ULONG_PTR GetSessionId(HANDLE Process)
{
	NTSTATUS Status;
	PROCESS_SESSION_INFORMATION Session;

#if ML_KERNEL_MODE

	Status = ZwQueryInformationProcess(Process, ProcessSessionInformation, &Session, sizeof(Session), NULL);

#else

	Status = NtQueryInformationProcess(Process, ProcessSessionInformation, &Session, sizeof(Session), NULL);

#endif

	return NT_FAILED(Status) ? INVALID_SESSION_ID : Session.SessionId;
}

ULONG_PTR GetSessionId(ULONG_PTR ProcessId)
{
	ULONG_PTR   SessionId;
	HANDLE      Process;

	Process = PidToHandle(ProcessId);
	if (Process == NULL)
		return INVALID_SESSION_ID;

	SessionId = GetSessionId(Process);

#if ML_KERNEL_MODE

	ZwClose(Process);

#else

	NtClose(Process);

#endif

	return SessionId;
}

BOOL IsWow64Process(HANDLE Process)
{
	PVOID       Wow64Information;
	NTSTATUS    Status;

	API_POINTER(ZwQueryInformationProcess)  XQueryInformationProcess;

#if ML_KERNEL_MODE

	XQueryInformationProcess = ZwQueryInformationProcess;

#else

	XQueryInformationProcess = NtQueryInformationProcess;

#endif

	Status = XQueryInformationProcess(
		Process,
		ProcessWow64Information,
		&Wow64Information,
		sizeof(Wow64Information),
		NULL
	);

	return NT_SUCCESS(Status) && Wow64Information != NULL;
}

PSYSTEM_PROCESS_INFORMATION QuerySystemProcesses()
{
	ULONG                       Size, Length;
	NTSTATUS                    Status;
	PSYSTEM_PROCESS_INFORMATION ProcessInfo;

	ProcessInfo = NULL;
	Size = 0;

	LOOP_FOREVER
	{
#if ML_KERNEL_MODE

		Status = ZwQuerySystemInformation(SystemProcessInformation, ProcessInfo, Size, &Length);
		if (Status != STATUS_INFO_LENGTH_MISMATCH)
			break;

		FreeMemoryP(ProcessInfo);
		ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)AllocateMemoryP(Length);

#else
		Status = NtQuerySystemInformation(SystemProcessInformation, ProcessInfo, Size, &Length);
		if (Status != STATUS_INFO_LENGTH_MISMATCH)
			break;

		ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)ReAllocateMemoryP(ProcessInfo, Length);

#endif

		if (ProcessInfo == NULL)
			return NULL;

		Size = Length;
	}

		if (!NT_SUCCESS(Status))
		{
			FreeMemoryP(ProcessInfo);
			ProcessInfo = NULL;
		}

	return ProcessInfo;
}

#if ML_X86

NAKED VOID CDECL CaptureRegisters(PML_THREAD_CONTEXT Context)
{
	INLINE_ASM
	{
		push    ebx;
		push    ecx;
		mov     ebx,[esp + 0Ch];
		mov     ecx,[ebx]ML_THREAD_CONTEXT.Flags;
		pushad;

		mov     edx,[esp + 020h];
		mov     eax,[esp + 024h];
		mov[esp]PUSHAD_REGISTER.Rcx, edx;
		mov[esp]PUSHAD_REGISTER.Rbx, eax;

		lea     edi,[ebx]ML_THREAD_CONTEXT.Rdi;
		lea     esi,[esp];

		and     ecx, (1 << 8) - 1;

		jecxz   LOOP_END;

	_LOOP:
		shr     ecx, 1;
		jnc     FLAG_NOT_SET;

		mov     eax,[esi];
		mov[edi], eax;

	FLAG_NOT_SET:
		lea     esi,[esi + 04h];
		lea     edi,[edi + 04h];
		jnz     _LOOP;

	LOOP_END:

		add[ebx]ML_THREAD_CONTEXT.Rsp, 010h;

		popad;
		pop     ecx;
		pop     ebx;
		ret;
	}
}

VOID CDECL RegistersContext(PML_THREAD_CONTEXT Context)
{
	;
}

#endif // x86

VOID Sleep(ULONG_PTR Milliseconds, BOOL Alertable)
{
	NTSTATUS Status;
	LARGE_INTEGER Timeout;

	FormatTimeOut(&Timeout, Milliseconds);

	LOOP_FOREVER
	{

#if ML_USER_MODE
		Status = NtDelayExecution((BOOLEAN)Alertable, &Timeout);
#else
		Status = KeDelayExecutionThread(KernelMode, Alertable, &Timeout);
#endif

		if (Milliseconds != INFINITE)
			break;

		if (Alertable == ThreadNoAlertable)
			break;

		if (Status != STATUS_USER_APC && Status != STATUS_ALERTED)
			break;
	}
}

#if ML_USER_MODE

PTEB_ACTIVE_FRAME
FindThreadFrame(
	ULONG_PTR Context
)
{
	PTEB_ACTIVE_FRAME Frame;

	Frame = RtlGetFrame();
	while (Frame != NULL && Frame->Flags != Context)
		Frame = Frame->Previous;

	return Frame;
}

PTEB_ACTIVE_FRAME
FindThreadFrameEx(
	ULONG_PTR Context,
	ULONG_PTR Data
)
{
	PTEB_ACTIVE_FRAME Frame;

	Frame = RtlGetFrame();
	while (Frame != nullptr && Frame->Flags != Context && (ULONG_PTR)Frame->Context != Data)
		Frame = Frame->Previous;

	return Frame;
}


NTSTATUS
CreateProcess(
	PCWSTR                  ApplicationName,
	PWSTR                   CommandLine,
	PCWSTR                  CurrentDirectory,
	ULONG                   CreationFlags,
	LPSTARTUPINFOW          StartupInfo,
	LPPROCESS_INFORMATION   ProcessInformation,
	LPSECURITY_ATTRIBUTES   ProcessAttributes,
	LPSECURITY_ATTRIBUTES   ThreadAttributes,
	PVOID                   Environment,
	HANDLE                  Token,
	BOOL                    InheritHandles
)
{
	BOOL                Result;
	STARTUPINFOW        DefaultStartupInfo;
	PROCESS_INFORMATION DefaultProcessInformation;

	if (StartupInfo == NULL)
	{
		ZeroMemory(&DefaultStartupInfo, sizeof(DefaultStartupInfo));
		DefaultStartupInfo.cb = sizeof(DefaultStartupInfo);
		StartupInfo = &DefaultStartupInfo;
	}

	if (ProcessInformation == NULL)
		ProcessInformation = &DefaultProcessInformation;

	API_POINTER(CreateProcessInternalW) StubCreateProcessInternalW = (API_POINTER(CreateProcessInternalW))
		EATLookupRoutineByHashPNoFix(
			GetKernel32Handle(),
			KERNEL32_CreateProcessInternalW
		);

	if (StubCreateProcessInternalW)
	{
		Result = StubCreateProcessInternalW(
			Token,
			ApplicationName,
			CommandLine,
			ProcessAttributes,
			ThreadAttributes,
			InheritHandles,
			CreationFlags,
			Environment,
			CurrentDirectory,
			StartupInfo,
			ProcessInformation,
			NULL
		);
	}
	else
	{
		Result = FALSE;
	}

	if (!Result)
		return ML_NTSTATUS_FROM_WIN32(RtlGetLastWin32Error());

	if (ProcessInformation == &DefaultProcessInformation)
	{
		NtClose(DefaultProcessInformation.hProcess);
		NtClose(DefaultProcessInformation.hThread);
	}

	return STATUS_SUCCESS;
}

#if ML_X86

NTSTATUS InjectDllBeforeKernel32Loaded(PML_PROCESS_INFORMATION ProcInfo, PCWSTR DllPath, ULONG CreationFlags)
{
	NTSTATUS                Status;
	ULONG_PTR               DllLoadCount, Offset, UndefinedInstruction, LdrLoadDllFirstDword;
	LDR_MODULE              *Ntdll;
	PVOID                   InjectBuffer, ReloadedNtdll, LdrLoadDll;
	HANDLE                  Debuggee;
	DBGUI_WAIT_STATE_CHANGE DbgState;
	PEXCEPTION_RECORD       ExceptionRecord;

	Ntdll = GetNtdllLdrModule();
	Status = LoadPeImage(Ntdll->FullDllName.Buffer, &ReloadedNtdll, NULL, LOAD_PE_IGNORE_RELOC);
	FAIL_RETURN(Status);

	LdrLoadDll = PtrAdd(EATLookupRoutineByHashPNoFix(ReloadedNtdll, NTDLL_LdrLoadDll), PtrOffset(Ntdll->DllBase, ReloadedNtdll));

	UnloadPeImage(ReloadedNtdll);

	Debuggee = ProcInfo->hProcess;

	Status = ReadMemory(Debuggee, LdrLoadDll, &LdrLoadDllFirstDword, sizeof(LdrLoadDllFirstDword));
	FAIL_RETURN(Status);

	UndefinedInstruction = 0x0B0F;   // ud2
	Status = WriteProtectMemory(Debuggee, LdrLoadDll, &UndefinedInstruction, 2);
	FAIL_RETURN(Status);

	if (FLAG_ON(CreationFlags, CREATE_SUSPENDED))
		NtResumeProcess(Debuggee);

	Status = DbgUiWaitStateChange(&DbgState, NULL);
	FAIL_RETURN(Status);

	DllLoadCount = 0;

	auto DbgX = [&]() -> NTSTATUS
	{
		NTSTATUS Status;

		Status = DbgUiContinue(&DbgState.AppClientId, DBG_CONTINUE);
		FAIL_RETURN(Status);

		Status = DbgUiWaitStateChange(&DbgState, NULL);
		return Status;
	};

	for (;; Status = DbgX())
	{
		FAIL_RETURN(Status);

		switch (DbgState.NewState)
		{
		case DbgExceptionStateChange:
			break;

		case DbgCreateThreadStateChange:
			if (DbgState.StateInfo.CreateThread.HandleToThread != NULL)
				NtClose(DbgState.StateInfo.CreateThread.HandleToThread);

			continue;

		case DbgCreateProcessStateChange:
			if (DbgState.StateInfo.CreateProcessInfo.HandleToProcess != NULL)
				NtClose(DbgState.StateInfo.CreateProcessInfo.HandleToProcess);

			if (DbgState.StateInfo.CreateProcessInfo.HandleToThread != NULL)
				NtClose(DbgState.StateInfo.CreateProcessInfo.HandleToThread);

			if (DbgState.StateInfo.CreateProcessInfo.NewProcess.FileHandle != NULL)
				NtClose(DbgState.StateInfo.CreateProcessInfo.NewProcess.FileHandle);

			continue;

		case DbgLoadDllStateChange:
			LOOP_ONCE
			{
				if (DbgState.StateInfo.LoadDll.FileHandle != NULL)
				NtClose(DbgState.StateInfo.LoadDll.FileHandle);

				if (DllLoadCount != 0)
					break;

				++DllLoadCount;

				BOOLEAN                 BeingDebugged;
				PPEB                    Peb;
				PVOID                   FsBase;
				CONTEXT                 Context;
				HANDLE                  Thread;
				DESCRIPTOR_TABLE_ENTRY  Descriptor;

				Thread = TidToHandle(DbgState.AppClientId.UniqueThread);
				if (Thread == NULL)
					break;

				Context.ContextFlags = CONTEXT_SEGMENTS;
				Status = NtGetContextThread(Thread, &Context);
				if (NT_FAILED(Status))
				{
					NtClose(Thread);
					break;
				}

				Descriptor.Selector = Context.SegFs;
				Status = NtQueryInformationThread(Thread, ThreadDescriptorTableEntry, &Descriptor, sizeof(Descriptor), NULL);
				NtClose(Thread);
				FAIL_BREAK(Status);

				FsBase = (PVOID)((Descriptor.Descriptor.HighWord.Bits.BaseHi << 24) |
					(Descriptor.Descriptor.HighWord.Bits.BaseMid << 16) |
					(Descriptor.Descriptor.BaseLow));

				if (FsBase == NULL)
					break;

				Peb = (PPEB)PtrAdd(FsBase, PEB_OFFSET);

				Status = ReadMemory(Debuggee, Peb, &Peb, sizeof(Peb));
				FAIL_BREAK(Status);

				// ReadMemory(Debuggee, &Peb->BeingDebugged, &BeingDebugged, sizeof(BeingDebugged));

				BeingDebugged = FALSE;
				WriteMemory(Debuggee, &Peb->BeingDebugged, &BeingDebugged, sizeof(BeingDebugged));
			}
			continue;

		default:
			continue;
		}

		ExceptionRecord = &DbgState.StateInfo.Exception.ExceptionRecord;
		if (ExceptionRecord->ExceptionCode != STATUS_ILLEGAL_INSTRUCTION)
			continue;

		if (ExceptionRecord->ExceptionAddress != LdrLoadDll)
			continue;

		break;
	}

	Status = WriteProtectMemory(Debuggee, LdrLoadDll, &LdrLoadDllFirstDword, sizeof(LdrLoadDllFirstDword));
	FAIL_RETURN(Status);

	PVOID           CallIp, CallNextIp;
	CONTEXT         Context;
	UNICODE_STRING  Dll;
	BYTE            CallOp;

	Context.ContextFlags = CONTEXT_CONTROL;
	Status = NtGetContextThread(ProcInfo->hThread, &Context);
	FAIL_RETURN(Status);


#if ML_X86

	Status = ReadMemory(Debuggee, (PVOID)Context.Esp, &CallNextIp, sizeof(ProcInfo->FirstCallLdrLoadDll));

#elif ML_AMD64

	Status = ReadMemory(Debuggee, (PVOID)Context.Rsp, &CallNextIp, sizeof(ProcInfo->FirstCallLdrLoadDll));

#endif

	FAIL_RETURN(Status);

	CallIp = PtrSub(CallNextIp, 5);
	Status = ReadMemory(Debuggee, CallIp, &CallOp, sizeof(CallOp));
	FAIL_RETURN(Status);

	if (CallOp != 0xE8)
		return STATUS_NOT_SUPPORTED;

	ProcInfo->FirstCallLdrLoadDll = CallIp;

	RtlInitUnicodeString(&Dll, DllPath);

	Status = InjectDllToRemoteProcess(
		Debuggee,
		ProcInfo->hThread,
		&Dll,
		INJECT_THREAD_SUSPENDED,
		&InjectBuffer
	);

	FAIL_RETURN(Status);

	Status = DbgUiContinue(&DbgState.AppClientId, DBG_CONTINUE);
	FAIL_RETURN(Status);

	if (FLAG_ON(CreationFlags, CREATE_SUSPENDED))
		NtSuspendProcess(Debuggee);

	Status = DbgUiStopDebugging(Debuggee);

	return Status;
}

NTSTATUS
CreateProcessWithDll(
	ULONG_PTR               Flags,
	PCWSTR                  DllPath,
	PCWSTR                  ApplicationName,
	PWSTR                   CommandLine,
	PCWSTR                  CurrentDirectory,
	ULONG                   CreationFlags,
	LPSTARTUPINFOW          StartupInfo,
	PML_PROCESS_INFORMATION ProcessInformation,
	LPSECURITY_ATTRIBUTES   ProcessAttributes,
	LPSECURITY_ATTRIBUTES   ThreadAttributes,
	PVOID                   Environment,
	HANDLE                  Token
)
{
	if (Flags == CPWD_NORMAL)
		return CreateProcess(ApplicationName, CommandLine, CurrentDirectory, CreationFlags, StartupInfo, ProcessInformation, ProcessAttributes, ThreadAttributes, Environment, Token);

	NTSTATUS                Status;
	ML_PROCESS_INFORMATION  ProcInfo;

	Status = STATUS_SUCCESS;

	switch (Flags)
	{
	case CPWD_BEFORE_KERNEL32:
		Status = CreateProcess(ApplicationName, CommandLine, CurrentDirectory, CreationFlags | DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS, StartupInfo, &ProcInfo, ProcessAttributes, ThreadAttributes, Environment, Token);
		FAIL_RETURN(Status);

		Status = InjectDllBeforeKernel32Loaded(&ProcInfo, DllPath, CreationFlags);
		break;

	default:
		return STATUS_INVALID_PARAMETER;
	}

	if (NT_FAILED(Status))
	{
		NtTerminateProcess(ProcInfo.hProcess, Status);
		NtClose(ProcInfo.hProcess);
		NtClose(ProcInfo.hThread);
	}
	else if (ProcessInformation == NULL)
	{
		NtClose(ProcInfo.hProcess);
		NtClose(ProcInfo.hThread);
	}
	else
	{
		*ProcessInformation = ProcInfo;
	}

	return Status;
}

#endif // AMD64

NTSTATUS
CreateThread(
	PVOID                   StartAddress,
	PVOID                   StartParameter,
	BOOL                    CreateSuspended,
	HANDLE                  ProcessHandle,
	PHANDLE                 ThreadHandle,
	PCLIENT_ID              ClientID,
	ULONG                   ZeroBits,
	ULONG_PTR               MaximumStackSize,
	ULONG_PTR               CommittedStackSize,
	PSECURITY_DESCRIPTOR    SecurityDescriptor
)
{
	NTSTATUS    Status;
	CLIENT_ID   ThreadId;
	HANDLE      NewThreadHandle;

	Status = RtlCreateUserThread(
		ProcessHandle,
		SecurityDescriptor,
		(BOOLEAN)CreateSuspended,
		ZeroBits,
		MaximumStackSize,
		CommittedStackSize,
		(PUSER_THREAD_START_ROUTINE)StartAddress,
		StartParameter,
		&NewThreadHandle,
		&ThreadId
	);
	if (!NT_SUCCESS(Status))
		return Status;

	if (ClientID != NULL)
		*ClientID = ThreadId;

	if (ThreadHandle != NULL)
	{
		*ThreadHandle = NewThreadHandle;
	}
	else
	{
		NtClose(NewThreadHandle);
	}

	return Status;
}

DECL_NORETURN
VOID
ExitProcess(
	NTSTATUS ExitStatus
)
{
	PVOID       LoaderLockCookie;
	NTSTATUS    Status;
	HANDLE      ProcessHeap = CurrentPeb()->ProcessHeap;

	Status = LdrLockLoaderLock(0, NULL, &LoaderLockCookie);
	RtlAcquirePebLock();
	RtlLockHeap(ProcessHeap);

	NtTerminateProcess(NULL, ExitStatus);

	RtlUnlockHeap(ProcessHeap);
	RtlReleasePebLock();

	if (NT_SUCCESS(Status))
		LdrUnlockLoaderLock(0, LoaderLockCookie);

	LdrShutdownProcess();
	NtTerminateProcess(CurrentProcess, ExitStatus);
}

#endif // r3

ML_NAMESPACE_END_(Ps);

ML_NAMESPACE_BEGIN(Mm)

NTSTATUS
AllocVirtualMemory(
	PVOID*      BaseAddress,
	ULONG_PTR   Size,
	ULONG       Protect,        /* = PAGE_EXECUTE_READWRITE */
	ULONG       AllocationType, /* = MEM_RESERVE | MEM_COMMIT */
	HANDLE      ProcessHandle   /* = Ps::CurrentProcess */
)
{
	return NtAllocateVirtualMemory(ProcessHandle, BaseAddress, 0, &Size, AllocationType, Protect);
}

NTSTATUS
AllocVirtualMemoryEx(
	HANDLE      ProcessHandle,
	PVOID*      BaseAddress,
	ULONG_PTR   Size,
	ULONG       Protect,
	ULONG       AllocationType
)
{
	return AllocVirtualMemory(BaseAddress, Size, Protect, AllocationType, ProcessHandle);
}

NTSTATUS
FreeVirtualMemory(
	PVOID   BaseAddress,
	HANDLE  ProcessHandle,  /* = Ps::CurrentProcess */
	ULONG   FreeType        /* = MEM_RELEASE */
)
{
	SIZE_T Size = 0;

	return NtFreeVirtualMemory(ProcessHandle, &BaseAddress, &Size, MEM_RELEASE);
}

#if ML_USER_MODE

NTSTATUS
ProtectVirtualMemory(
	PVOID       BaseAddress,
	ULONG_PTR   Size,
	ULONG       NewProtect,
	PULONG      OldProtect,
	HANDLE      ProcessHandle   /* = Ps::CurrentProcess */
)
{
	ULONG _OldProtect;
	NTSTATUS Status;

	Status = NtProtectVirtualMemory(ProcessHandle, &BaseAddress, &Size, NewProtect, &_OldProtect);
	if (NT_SUCCESS(Status) && OldProtect != NULL)
		*OldProtect = _OldProtect;

	return Status;
}

NTSTATUS
ProtectMemory(
	HANDLE      ProcessHandle,
	PVOID       BaseAddress,
	ULONG_PTR   Size,
	ULONG       NewProtect,
	PULONG      OldProtect
)
{
	return ProtectVirtualMemory(BaseAddress, Size, NewProtect, OldProtect, ProcessHandle);
}

NTSTATUS
ReadMemory(
	HANDLE      ProcessHandle,
	PVOID       BaseAddress,
	PVOID       Buffer,
	ULONG_PTR   Size,
	PULONG_PTR  BytesRead /* = NULL */
)
{
	return NtReadVirtualMemory(ProcessHandle, BaseAddress, Buffer, Size, BytesRead);
}

NTSTATUS
WriteMemory(
	HANDLE      ProcessHandle,
	PVOID       BaseAddress,
	PVOID       Buffer,
	ULONG_PTR   Size,
	PULONG_PTR  BytesWritten /* = NULL */
)
{
	return NtWriteVirtualMemory(ProcessHandle, BaseAddress, Buffer, Size, BytesWritten);
}

NTSTATUS
WriteProtectMemory(
	HANDLE      ProcessHandle,
	PVOID       BaseAddress,
	PVOID       Buffer,
	ULONG_PTR   Size,
	PULONG_PTR  BytesWritten /* = NULL */
)
{
	ULONG       Protect;
	NTSTATUS    Status;

	//Status = ProtectVirtualMemory(BaseAddress, Size, PAGE_WRITECOPY, &Protect, ProcessHandle);
	//FAIL_RETURN(Status);

	Status = ProtectVirtualMemory(BaseAddress, Size, PAGE_EXECUTE_READWRITE, &Protect, ProcessHandle);
	FAIL_RETURN(Status);

	Status = WriteMemory(ProcessHandle, BaseAddress, Buffer, Size, BytesWritten);

	if (Protect != PAGE_EXECUTE_READWRITE)
		ProtectVirtualMemory(BaseAddress, Size, Protect, NULL, ProcessHandle);

	return Status;
}

NTSTATUS QueryMappedImageName(HANDLE ProcessHandle, PVOID ImageBase, String& ImageName)
{
	NTSTATUS                            Status;
	ULONG_PTR                           Size;
	UNICODE_STRING                      DosPath;
	PMEMORY_MAPPED_FILENAME_INFORMATION MappedFileName;
	BYTE                                LocalBuffer[sizeof(*MappedFileName)];

	MappedFileName = (PMEMORY_MAPPED_FILENAME_INFORMATION)LocalBuffer;
	Size = sizeof(LocalBuffer);

	Status = NtQueryVirtualMemory(ProcessHandle, ImageBase, MemoryMappedFilenameInformation, MappedFileName, Size, &Size);
	if (Status != STATUS_BUFFER_OVERFLOW && Status != STATUS_INFO_LENGTH_MISMATCH)
		return Status;

	MappedFileName = (PMEMORY_MAPPED_FILENAME_INFORMATION)AllocStack(Size);
	Status = NtQueryVirtualMemory(ProcessHandle, ImageBase, MemoryMappedFilenameInformation, MappedFileName, Size, &Size);
	FAIL_RETURN(Status);

	Status = Io::QueryDosPathFromNtDeviceName(&DosPath, &MappedFileName->Name);
	FAIL_RETURN(Status);

	ImageName = DosPath;
	RtlFreeUnicodeString(&DosPath);

	return Status;
}

#endif // r3

ML_NAMESPACE_END_(Mm)

ML_NAMESPACE_BEGIN(Rtl);

NTSTATUS
DosPathNameToNtPathName(
	PCWSTR                  DosName,
	PUNICODE_STRING         NtName,
	PCWSTR*                 DosFilePath,
	PRTL_RELATIVE_NAME_U    FileName
)
{
#if ML_USER_MODE

	BOOL Success;

	Success = RtlDosPathNameToNtPathName_U(DosName, NtName, DosFilePath, FileName);

	return Success ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;

#elif ML_KERNEL_MODE

	return STATUS_NOT_IMPLEMENTED;

#endif // r
}

NTSTATUS GetSystemDirectory(PUNICODE_STRING Buffer, BOOL Wow64NoRedirect)
{
	BOOL                IsWow64;
	WCHAR               LocalBuffer[0x100];
	NTSTATUS            Status;
	HANDLE              KnownDllDirectoryHandle, KnownDllPathSymbolicLink;
	OBJECT_ATTRIBUTES   ObjectAttributes;
	UNICODE_STRING      LocalString;
	PUNICODE_STRING     KnownDlls;

	static UNICODE_STRING KnownDllsName = RTL_CONSTANT_STRING(L"\\KnownDlls");
	static UNICODE_STRING KnownDlls32Name = RTL_CONSTANT_STRING(L"\\KnownDlls32");
	static UNICODE_STRING KnownDllPathName = RTL_CONSTANT_STRING(L"KnownDllPath");

	KnownDlls = &KnownDllsName;

	if (Wow64NoRedirect == FALSE && Ps::IsWow64Process() != FALSE)
		KnownDlls = &KnownDlls32Name;

#if ML_USER_MODE

	InitializeObjectAttributes(&ObjectAttributes, KnownDlls, OBJ_CASE_INSENSITIVE, nullptr, nullptr);
	Status = NtOpenDirectoryObject(&KnownDllDirectoryHandle, DIRECTORY_QUERY | DIRECTORY_TRAVERSE, &ObjectAttributes);
	FAIL_RETURN(Status);

	InitializeObjectAttributes(&ObjectAttributes, &KnownDllPathName, OBJ_CASE_INSENSITIVE, KnownDllDirectoryHandle, nullptr);
	Status = NtOpenSymbolicLinkObject(&KnownDllPathSymbolicLink, SYMBOLIC_LINK_QUERY, &ObjectAttributes);
	NtClose(KnownDllDirectoryHandle);
	FAIL_RETURN(Status);

	RtlInitEmptyString(&LocalString, LocalBuffer, countof(LocalBuffer));

	Status = NtQuerySymbolicLinkObject(KnownDllPathSymbolicLink, &LocalString, nullptr);
	NtClose(KnownDllPathSymbolicLink);
	FAIL_RETURN(Status);

	RtlAppendUnicodeToString(&LocalString, L"\\");

	return RtlDuplicateUnicodeString(RTL_DUPSTR_ADD_NULL, &LocalString, Buffer);

#else // r0

	InitializeObjectAttributes(&ObjectAttributes, KnownDlls, OBJ_CASE_INSENSITIVE, nullptr, nullptr);
	Status = ZwOpenDirectoryObject(&KnownDllDirectoryHandle, DIRECTORY_QUERY | DIRECTORY_TRAVERSE, &ObjectAttributes);
	FAIL_RETURN(Status);

	InitializeObjectAttributes(&ObjectAttributes, &KnownDllPathName, OBJ_CASE_INSENSITIVE, KnownDllDirectoryHandle, nullptr);
	Status = ZwOpenSymbolicLinkObject(&KnownDllPathSymbolicLink, SYMBOLIC_LINK_QUERY, &ObjectAttributes);
	NtClose(KnownDllDirectoryHandle);
	FAIL_RETURN(Status);

	RtlInitEmptyString(&LocalString, LocalBuffer, countof(LocalBuffer));

	Status = ZwQuerySymbolicLinkObject(KnownDllPathSymbolicLink, &LocalString, nullptr);
	ZwClose(KnownDllPathSymbolicLink);
	FAIL_RETURN(Status);

	RtlAppendUnicodeToString(&LocalString, L"\\");

	return RtlDuplicateUnicodeString(RTL_DUPSTR_ADD_NULL, &LocalString, Buffer);

#endif
}

#if ML_USER_MODE

NTSTATUS GetModuleDirectory(ml::String &Path, PVOID ModuleBase)
{
	PWSTR           FileName;
	LDR_MODULE      *Self;
	UNICODE_STRING  SelfPath;

	Self = FindLdrModuleByHandle(ModuleBase);
	SelfPath = Self->FullDllName;
	FileName = findnamew(SelfPath.Buffer, SelfPath.Length / sizeof(SelfPath.Buffer[0]));
	SelfPath.Length = (USHORT)PtrOffset(FileName, SelfPath.Buffer);

	Path = SelfPath;

	return STATUS_SUCCESS;
}

NTSTATUS GetWorkingDirectory(PUNICODE_STRING Buffer)
{
	return RtlDuplicateUnicodeString(
		RTL_DUPSTR_ADD_NULL,
		&Ps::CurrentPeb()->ProcessParameters->CurrentDirectory.DosPath,
		Buffer
	);
}

NTSTATUS SetWorkingDirectory(PCWSTR PathName)
{
	UNICODE_STRING Path;

	RtlInitUnicodeString(&Path, PathName);
	return SetWorkingDirectory(&Path);
}

NTSTATUS SetWorkingDirectory(PUNICODE_STRING PathName)
{
	return RtlSetCurrentDirectory_U(PathName);
}

NTSTATUS SetExeDirectoryAsCurrent()
{
	UNICODE_STRING  ExePath;
	LDR_MODULE*     LdrModule;
	PWSTR           Buffer; // for stupid XP

	LdrModule = Ldr::FindLdrModuleByHandle(NULL);
	ExePath = LdrModule->FullDllName;
	ExePath.Length -= LdrModule->BaseDllName.Length;

	Buffer = (PWSTR)AllocStack(ExePath.Length + sizeof(ExePath.Buffer[0]));
	CopyMemory(Buffer, ExePath.Buffer, ExePath.Length);
	*PtrAdd(Buffer, ExePath.Length) = 0;
	ExePath.Buffer = Buffer;

	return RtlSetCurrentDirectory_U(&ExePath);
}

NTSTATUS NtPathNameToDosPathName(PUNICODE_STRING DosPath, PUNICODE_STRING NtPath)
{
	NTSTATUS                    Status;
	ULONG_PTR                   BufferSize;
	PWSTR                       Buffer;
	RTL_UNICODE_STRING_BUFFER   UnicodeBuffer;

	BufferSize = NtPath->MaximumLength + MAX_NTPATH * sizeof(WCHAR);

	Buffer = (PWSTR)AllocStack(BufferSize);

	ZeroMemory(&UnicodeBuffer, sizeof(UnicodeBuffer));

	UnicodeBuffer.String = *NtPath;
	UnicodeBuffer.String.Buffer = Buffer;
	UnicodeBuffer.String.MaximumLength = (USHORT)BufferSize;
	UnicodeBuffer.ByteBuffer.Buffer = (PUCHAR)Buffer;
	UnicodeBuffer.ByteBuffer.Size = BufferSize;

	CopyMemory(Buffer, NtPath->Buffer, NtPath->Length);

	Status = RtlNtPathNameToDosPathName(0, &UnicodeBuffer, NULL, NULL);
	FAIL_RETURN(Status);

	return RtlDuplicateUnicodeString(RTL_DUPSTR_ADD_NULL, &UnicodeBuffer.String, DosPath);
}

#endif // r3


BOOL
DoesNameContainWildCards(
	IN PUNICODE_STRING Name
)

/*++

Routine Description:

This routine simply scans the input Name string looking for any Nt
wild card characters.

Arguments:

Name - The string to check.

Return Value:

BOOLEAN - TRUE if one or more wild card characters was found.

--*/
{
	PWSTR p;

	//
	//  Check each character in the name to see if it's a wildcard
	//  character.
	//

#if ML_USER_MODE

	static const UCHAR LEGAL_ANSI_CHARACTER_ARRAY[128] =
	{
		0x00, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
		0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
		0x17, 0x07, 0x18, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x18, 0x16, 0x16, 0x17, 0x07, 0x00,
		0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x04, 0x16, 0x18, 0x16, 0x18, 0x18,
		0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17,
		0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x16, 0x00, 0x16, 0x17, 0x17,
		0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17,
		0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x10, 0x17, 0x17, 0x17
	};

#endif // r3

	if (Name->Length) {
		for (p = (PWSTR)Name->Buffer + (Name->Length / sizeof(WCHAR)) - 1;
			p >= Name->Buffer && *p != L'\\';
			p--) {

			//
			//  check for a wild card character
			//

			if (FsRtlIsUnicodeCharacterWild(*p)) {

				//
				//  Tell caller that this name contains wild cards
				//

				return TRUE;
			}
		}
	}

	//
	//  No wildcard characters were found, so return to our caller
	//

	return FALSE;
}

#define MATCHES_ARRAY_SIZE 16

//
//  Local support routine prototypes
//

BOOL
IsNameInExpressionPrivate(
	IN PUNICODE_STRING Expression,
	IN PUNICODE_STRING Name,
	IN BOOL IgnoreCase,
	IN PWCH UpcaseTable
)

/*++

Routine Description:

This routine compares a Dbcs name and an expression and tells the caller
if the name is in the language defined by the expression.  The input name
cannot contain wildcards, while the expression may contain wildcards.

Expression wild cards are evaluated as shown in the nondeterministic
finite automatons below.  Note that ~* and ~? are DOS_STAR and DOS_QM.


~* is DOS_STAR, ~? is DOS_QM, and ~. is DOS_DOT


S
<-----<
X  |     |  e       Y
X * Y ==       (0)----->-(1)->-----(2)-----(3)


S-.
<-----<
X  |     |  e       Y
X ~* Y ==      (0)----->-(1)->-----(2)-----(3)



X     S     S     Y
X ?? Y ==      (0)---(1)---(2)---(3)---(4)



X     .        .      Y
X ~.~. Y ==    (0)---(1)----(2)------(3)---(4)
|      |________|
|           ^   |
|_______________|
^EOF or .^


X     S-.     S-.     Y
X ~?~? Y ==    (0)---(1)-----(2)-----(3)---(4)
|      |________|
|           ^   |
|_______________|
^EOF or .^



where S is any single character

S-. is any single character except the final .

e is a null character transition

EOF is the end of the name string

In words:

* matches 0 or more characters.

? matches exactly 1 character.

DOS_STAR matches 0 or more characters until encountering and matching
the final . in the name.

DOS_QM matches any single character, or upon encountering a period or
end of name string, advances the expression to the end of the
set of contiguous DOS_QMs.

DOS_DOT matches either a . or zero characters beyond name string.

Arguments:

Expression - Supplies the input expression to check against
(Caller must already upcase if passing CaseInsensitive TRUE.)

Name - Supplies the input name to check for.

CaseInsensitive - TRUE if Name should be Upcased before comparing.

Return Value:

BOOLEAN - TRUE if Name is an element in the set of strings denoted
by the input Expression and FALSE otherwise.

--*/

{
	USHORT NameOffset;
	USHORT ExprOffset;

	ULONG SrcCount;
	ULONG DestCount;
	ULONG PreviousDestCount;
	ULONG MatchesCount;

	WCHAR NameChar = 0, ExprChar;

	USHORT LocalBuffer[MATCHES_ARRAY_SIZE * 2];

	USHORT *AuxBuffer = NULL;
	USHORT *PreviousMatches;
	USHORT *CurrentMatches;

	USHORT MaxState;
	USHORT CurrentState;

	BOOL NameFinished = FALSE;

	//
	//  The idea behind the algorithm is pretty simple.  We keep track of
	//  all possible locations in the regular expression that are matching
	//  the name.  If when the name has been exhausted one of the locations
	//  in the expression is also just exhausted, the name is in the language
	//  defined by the regular expression.
	//

	//
	//  If one string is empty return FALSE.  If both are empty return TRUE.
	//

	if ((Name->Length == 0) || (Expression->Length == 0)) {

		return (BOOL)(!(Name->Length + Expression->Length));
	}

	//
	//  Special case by far the most common wild card search of *
	//

	if ((Expression->Length == 2) && (Expression->Buffer[0] == L'*')) {

		return TRUE;
	}

	//
	//  Also special case expressions of the form *X.  With this and the prior
	//  case we have covered virtually all normal queries.
	//

	if (Expression->Buffer[0] == L'*') {

		UNICODE_STRING LocalExpression;

		LocalExpression = *Expression;

		LocalExpression.Buffer += 1;
		LocalExpression.Length -= 2;

		//
		//  Only special case an expression with a single *
		//

		if (!DoesNameContainWildCards(&LocalExpression)) {

			ULONG StartingNameOffset;

			if (Name->Length < (USHORT)(Expression->Length - sizeof(WCHAR))) {

				return FALSE;
			}

			StartingNameOffset = (Name->Length -
				LocalExpression.Length) / sizeof(WCHAR);

			//
			//  Do a simple memory compare if case sensitive, otherwise
			//  we have got to check this one character at a time.
			//

			if (!IgnoreCase) {

				return (BOOL)RtlEqualMemory(LocalExpression.Buffer,
					Name->Buffer + StartingNameOffset,
					LocalExpression.Length);

			}
			else {

				for (ExprOffset = 0;
					ExprOffset < (USHORT)(LocalExpression.Length / sizeof(WCHAR));
					ExprOffset += 1) {

					NameChar = Name->Buffer[StartingNameOffset + ExprOffset];
					NameChar = UpcaseTable[NameChar];

					ExprChar = LocalExpression.Buffer[ExprOffset];

					if (NameChar != ExprChar) {

						return FALSE;
					}
				}

				return TRUE;
			}
		}
	}

	//
	//  Walk through the name string, picking off characters.  We go one
	//  character beyond the end because some wild cards are able to match
	//  zero characters beyond the end of the string.
	//
	//  With each new name character we determine a new set of states that
	//  match the name so far.  We use two arrays that we swap back and forth
	//  for this purpose.  One array lists the possible expression states for
	//  all name characters up to but not including the current one, and other
	//  array is used to build up the list of states considering the current
	//  name character as well.  The arrays are then switched and the process
	//  repeated.
	//
	//  There is not a one-to-one correspondence between state number and
	//  offset into the expression.  This is evident from the NFAs in the
	//  initial comment to this function.  State numbering is not continuous.
	//  This allows a simple conversion between state number and expression
	//  offset.  Each character in the expression can represent one or two
	//  states.  * and DOS_STAR generate two states: ExprOffset*2 and
	//  ExprOffset*2 + 1.  All other expression characters can produce only
	//  a single state.  Thus ExprOffset = State/2.
	//
	//
	//  Here is a short description of the variables involved:
	//
	//  NameOffset  - The offset of the current name char being processed.
	//
	//  ExprOffset  - The offset of the current expression char being processed.
	//
	//  SrcCount    - Prior match being investigated with current name char
	//
	//  DestCount   - Next location to put a matching assuming current name char
	//
	//  NameFinished - Allows one more iteration through the Matches array
	//                 after the name is exhausted (to come *s for example)
	//
	//  PreviousDestCount - This is used to prevent entry duplication, see coment
	//
	//  PreviousMatches   - Holds the previous set of matches (the Src array)
	//
	//  CurrentMatches    - Holds the current set of matches (the Dest array)
	//
	//  AuxBuffer, LocalBuffer - the storage for the Matches arrays
	//

	//
	//  Set up the initial variables
	//

	PreviousMatches = &LocalBuffer[0];
	CurrentMatches = &LocalBuffer[MATCHES_ARRAY_SIZE];

	PreviousMatches[0] = 0;
	MatchesCount = 1;

	NameOffset = 0;

	MaxState = (USHORT)(Expression->Length * 2);

	while (!NameFinished) {

		if (NameOffset < Name->Length) {

			NameChar = Name->Buffer[NameOffset / sizeof(WCHAR)];

			NameOffset += sizeof(WCHAR);;

		}
		else {

			NameFinished = TRUE;

			//
			//  if we have already exhausted the expression, cool.  Don't
			//  continue.
			//

			if (PreviousMatches[MatchesCount - 1] == MaxState) {

				break;
			}
		}


		//
		//  Now, for each of the previous stored expression matches, see what
		//  we can do with this name character.
		//

		SrcCount = 0;
		DestCount = 0;
		PreviousDestCount = 0;

		while (SrcCount < MatchesCount) {

			USHORT Length;

			//
			//  We have to carry on our expression analysis as far as possible
			//  for each character of name, so we loop here until the
			//  expression stops matching.  A clue here is that expression
			//  cases that can match zero or more characters end with a
			//  continue, while those that can accept only a single character
			//  end with a break.
			//

			ExprOffset = (USHORT)((PreviousMatches[SrcCount++] + 1) / 2);


			Length = 0;

			LOOP_FOREVER
			{

				if (ExprOffset == Expression->Length) {

					break;
				}

			//
			//  The first time through the loop we don't want
			//  to increment ExprOffset.
			//

			ExprOffset = (USHORT)(ExprOffset + Length);
			Length = sizeof(WCHAR);

			CurrentState = (USHORT)(ExprOffset * 2);

			if (ExprOffset == Expression->Length) {

				CurrentMatches[DestCount++] = MaxState;
				break;
			}

			ExprChar = Expression->Buffer[ExprOffset / sizeof(WCHAR)];

			//
			//  Before we get started, we have to check for something
			//  really gross.  We may be about to exhaust the local
			//  space for ExpressionMatches[][], so we have to allocate
			//  some pool if this is the case.  Yuk!
			//

			if ((DestCount >= MATCHES_ARRAY_SIZE - 2) &&
				(AuxBuffer == NULL)) {

				ULONG ExpressionChars;

				ExpressionChars = Expression->Length / sizeof(WCHAR);

				AuxBuffer = (PUSHORT)AllocateMemoryP((ExpressionChars + 1) * sizeof(USHORT) * 2 * 2);

				RtlCopyMemory(AuxBuffer,
					CurrentMatches,
					MATCHES_ARRAY_SIZE * sizeof(USHORT));

				CurrentMatches = AuxBuffer;

				RtlCopyMemory(AuxBuffer + (ExpressionChars + 1) * 2,
					PreviousMatches,
					MATCHES_ARRAY_SIZE * sizeof(USHORT));

				PreviousMatches = AuxBuffer + (ExpressionChars + 1) * 2;
			}

			//
			//  * matches any character zero or more times.
			//

			if (ExprChar == L'*') {

				CurrentMatches[DestCount++] = CurrentState;
				CurrentMatches[DestCount++] = CurrentState + 3;
				continue;
			}

			//
			//  DOS_STAR matches any character except . zero or more times.
			//

			if (ExprChar == DOS_STAR) {

				BOOL ICanEatADot = FALSE;

				//
				//  If we are at a period, determine if we are allowed to
				//  consume it, ie. make sure it is not the last one.
				//

				if (!NameFinished && (NameChar == '.')) {

					USHORT Offset;

					for (Offset = NameOffset;
						Offset < Name->Length;
						Offset = (USHORT)(Offset + Length)) {

						if (Name->Buffer[Offset / sizeof(WCHAR)] == L'.') {

							ICanEatADot = TRUE;
							break;
						}
					}
				}

				if (NameFinished || (NameChar != L'.') || ICanEatADot) {

					CurrentMatches[DestCount++] = CurrentState;
					CurrentMatches[DestCount++] = CurrentState + 3;
					continue;

				}
				else {

					//
					//  We are at a period.  We can only match zero
					//  characters (ie. the epsilon transition).
					//

					CurrentMatches[DestCount++] = CurrentState + 3;
					continue;
				}
			}

			//
			//  The following expression characters all match by consuming
			//  a character, thus force the expression, and thus state
			//  forward.
			//

			CurrentState += (USHORT)(sizeof(WCHAR) * 2);

			//
			//  DOS_QM is the most complicated.  If the name is finished,
			//  we can match zero characters.  If this name is a '.', we
			//  don't match, but look at the next expression.  Otherwise
			//  we match a single character.
			//

			if (ExprChar == DOS_QM) {

				if (NameFinished || (NameChar == L'.')) {

					continue;
				}

				CurrentMatches[DestCount++] = CurrentState;
				break;
			}

			//
			//  A DOS_DOT can match either a period, or zero characters
			//  beyond the end of name.
			//

			if (ExprChar == DOS_DOT) {

				if (NameFinished) {

					continue;
				}

				if (NameChar == L'.') {

					CurrentMatches[DestCount++] = CurrentState;
					break;
				}
			}

			//
			//  From this point on a name character is required to even
			//  continue, let alone make a match.
			//

			if (NameFinished) {

				break;
			}

			//
			//  If this expression was a '?' we can match it once.
			//

			if (ExprChar == L'?') {

				CurrentMatches[DestCount++] = CurrentState;
				break;
			}

			//
			//  Finally, check if the expression char matches the name char
			//

			if (ExprChar == (WCHAR)(IgnoreCase ?
				UpcaseTable[NameChar] : NameChar)) {

				CurrentMatches[DestCount++] = CurrentState;
				break;
			}

			//
			//  The expression didn't match so go look at the next
			//  previous match.
			//

			break;
			}


				//
				//  Prevent duplication in the destination array.
				//
				//  Each of the arrays is monotonically increasing and non-
				//  duplicating, thus we skip over any source element in the src
				//  array if we just added the same element to the destination
				//  array.  This guarantees non-duplication in the dest. array.
				//

				while ((SrcCount < MatchesCount) &&
					(PreviousDestCount < DestCount)) {

					while ((SrcCount < MatchesCount) &&
						(PreviousMatches[SrcCount] <
							CurrentMatches[PreviousDestCount])) {

						SrcCount += 1;
					}

					PreviousDestCount += 1;
				}
		}

		//
		//  If we found no matches in the just finished iteration, it's time
		//  to bail.
		//

		if (DestCount == 0)
		{
			FreeMemoryP(AuxBuffer);
			return FALSE;
		}

		//
		//  Swap the meaning the two arrays
		//

		{
			USHORT *Tmp;

			Tmp = PreviousMatches;

			PreviousMatches = CurrentMatches;

			CurrentMatches = Tmp;
		}

		MatchesCount = DestCount;
	}


	CurrentState = PreviousMatches[MatchesCount - 1];

	FreeMemoryP(AuxBuffer);


	return (BOOL)(CurrentState == MaxState);
}

BOOL
IsNameInExpression(
	IN PUNICODE_STRING  Expression,
	IN PUNICODE_STRING  Name,
	IN BOOL             IgnoreCase,
	IN PWSTR            UpcaseTable OPTIONAL
)
{
	BOOL Result = FALSE;
	UNICODE_STRING LocalExpression, LocalName;

	//
	//  If we weren't given an upcase table, we have to upcase the names
	//  ourselves.
	//

	if (IgnoreCase && UpcaseTable == NULL)
	{

		NTSTATUS Status;

		Status = RtlUpcaseUnicodeString(&LocalName, Name, TRUE);
		if (NT_FAILED(Status))
			return FALSE;

		Status = RtlUpcaseUnicodeString(&LocalExpression, Expression, TRUE);
		if (NT_FAILED(Status))
		{
			RtlFreeUnicodeString(&LocalName);
			return FALSE;
		}

		Name = &LocalName;
		Expression = &LocalExpression;

		IgnoreCase = FALSE;

	}
	else
	{
		LocalName.Buffer = NULL;
		LocalExpression.Buffer = NULL;
	}

	//
	//  Now call the main routine, remembering to free the upcased string
	//  if we allocated one.
	//

	SEH_TRY
	{
		Result = IsNameInExpressionPrivate(Expression, Name, IgnoreCase, UpcaseTable);
	}
		SEH_FINALLY
	{
		if (LocalName.Buffer != NULL)
		RtlFreeUnicodeString(&LocalName);

		if (LocalExpression.Buffer != NULL)
			RtlFreeUnicodeString(&LocalExpression);
	}

	return Result;
}

NTSTATUS EnvironmentAppend(PUNICODE_STRING Key, PUNICODE_STRING Value)
{
	PWSTR           EnvBuffer, QuoteBuffer, Sub;
	ULONG           Length;
	UNICODE_STRING  Env, QuoteKey;

	Length = Key->Length + sizeof(*QuoteBuffer) * 2;
	QuoteBuffer = (PWSTR)AllocStack(Length);
	RtlInitEmptyString(&QuoteKey, QuoteBuffer, Length);

	QuoteBuffer[0] = '%';
	QuoteBuffer[Length / sizeof(*QuoteBuffer) - 1] = '%';
	CopyMemory(&QuoteBuffer[1], Key->Buffer, Key->Length);
	QuoteKey.Length = QuoteKey.MaximumLength;

	RtlInitEmptyString(&Env);
	RtlExpandEnvironmentStrings_U(nullptr, &QuoteKey, &Env, &Length);

	Length += Value->MaximumLength + sizeof(*EnvBuffer) * 3;
	EnvBuffer = (PWSTR)AllocStack(Length);
	RtlInitEmptyString(&Env, EnvBuffer, Length);

	RtlExpandEnvironmentStrings_U(nullptr, &QuoteKey, &Env, nullptr);

	((PWSTR)Env.Buffer)[Env.Length / sizeof(Env.Buffer[0])] = 0;
	Sub = (PWSTR)wcsstr(Env.Buffer, Value->Buffer);
	if (Sub != nullptr)
	{
		switch (Sub[Value->Length / sizeof(Value->Buffer[0])])
		{
		case 0:
		case ';':
			return STATUS_SUCCESS;
		}
	}

	RtlAppendUnicodeToString(&Env, L";");
	RtlAppendUnicodeStringToString(&Env, Value);
	RtlAppendUnicodeToString(&Env, L";");

	return RtlSetEnvironmentVariable(nullptr, Key, &Env);
}

ML_NAMESPACE_END_(Rtl);


BOOL
IsNameInExpression2(
	IN PUNICODE_STRING  Expression,
	IN PUNICODE_STRING  Name,
	IN BOOL             DEF_VAL(IgnoreCase, TRUE),
	IN PWSTR            DEF_VAL(UpcaseTable, NULL) OPTIONAL
)
{
	return Rtl::IsNameInExpression(Expression, Name, IgnoreCase, UpcaseTable);
}


ULONG_PTR GetOpCodeSize32_2(PVOID Buffer)
{
	return LdeGetOpCodeSize32(Buffer);
}

ULONG_PTR GetOpCodeSize64_2(PVOID Buffer)
{
	return LdeGetOpCodeSize64(Buffer);
}

ULONG_PTR GetOpCodeSize_2(PVOID Buffer)
{
#if ML_AMD64
	return GetOpCodeSize64(Buffer);
#elif ML_X86
	return GetOpCodeSize32(Buffer);
#endif
}


ML_NAMESPACE_BEGIN(Nls);

NTSTATUS
FASTCALL
AnsiToUnicode(
	PWCHAR      UnicodeBuffer,
	ULONG_PTR   BufferCount,
	LPCSTR      AnsiString,
	LONG_PTR    AnsiLength,
	PULONG_PTR  BytesInUnicode
)
{
	ULONG       Length;
	NTSTATUS    Status;

	if (AnsiLength == -1)
		AnsiLength = StrLengthA(AnsiString);

	Status = RtlMultiByteToUnicodeN(
		UnicodeBuffer,
		(ULONG)(BufferCount != -1 ? (BufferCount * sizeof(WCHAR)) : -1),
		&Length,
		(PCHAR)AnsiString,
		(ULONG)AnsiLength
	);
	if (!NT_SUCCESS(Status))
		return Status;

	if (Length / sizeof(WCHAR) < BufferCount)
		UnicodeBuffer[Length / sizeof(WCHAR)] = 0;

	if (BytesInUnicode != NULL)
		*BytesInUnicode = Length;

	return Status;
}

NTSTATUS
UnicodeToAnsi(
	PSTR        AnsiBuffer,
	ULONG_PTR   BufferCount,
	PCWSTR      UnicodeString,
	LONG_PTR    UnicodeLength,
	PULONG_PTR  BytesInAnsi
)
{
	ULONG       Length;
	NTSTATUS    Status;

	if (UnicodeLength == -1)
		UnicodeLength = StrLengthW(UnicodeString);

	Status = RtlUnicodeToMultiByteN(
		AnsiBuffer,
		(ULONG)BufferCount,
		&Length,
		(PWCHAR)UnicodeString,
		(ULONG)(UnicodeLength * sizeof(WCHAR))
	);

	if (!NT_SUCCESS(Status))
		return Status;

	if (Length < BufferCount)
		AnsiBuffer[Length] = 0;

	if (BytesInAnsi != NULL)
		*BytesInAnsi = Length;

	return Status;
}

NTSTATUS
AnsiToUnicodeString(
	PUNICODE_STRING Unicode,
	PCSTR           AnsiString,
	LONG_PTR        AnsiLength,
	BOOL            AllocateDestinationString
)
{
	ANSI_STRING Ansi;

	Ansi.Buffer = (PSTR)AnsiString;
	Ansi.Length = (USHORT)(AnsiLength == -1 ? StrLengthA(AnsiString) : AnsiLength);
	Ansi.MaximumLength = Ansi.Length;

	return RtlAnsiStringToUnicodeString(Unicode, &Ansi, (BOOLEAN)AllocateDestinationString);
}

NTSTATUS
UnicodeToAnsiString(
	PANSI_STRING    Ansi,
	LPCWSTR         UnicodeString,
	LONG_PTR        UnicodeLength,
	BOOL            AllocateDestinationString
)
{
	UNICODE_STRING Unicode;

	Unicode.Buffer = (PWSTR)UnicodeString;
	Unicode.Length = (USHORT)((UnicodeLength == -1 ? StrLengthW(UnicodeString) : UnicodeLength) * sizeof(WCHAR));
	Unicode.MaximumLength = Unicode.Length;

	return RtlUnicodeStringToAnsiString(Ansi, &Unicode, (BOOLEAN)AllocateDestinationString);
}

ML_NAMESPACE_END_(Nls);

ML_NAMESPACE_BEGIN(Str)

/************************************************************************
UNICODE_STRING helper class
************************************************************************/

UnicodeString::UnicodeString()
{
	RtlInitEmptyUnicodeString(this, NULL, 0);
}

UnicodeString::~UnicodeString()
{
	RtlFreeUnicodeString(this);
}

NTSTATUS UnicodeString::Create(PCWSTR Buffer, ULONG_PTR Length)
{
	return RtlSetUnicodeString(this, Buffer, Length);
}

NTSTATUS UnicodeString::Create(PCUNICODE_STRING Buffer)
{
	return Create(Buffer->Buffer, Buffer->Length / sizeof(WCHAR));
}

BOOL UnicodeString::operator==(PCWSTR Buffer) const
{
	UNICODE_STRING String;

	RtlInitUnicodeString(&String, Buffer);

	return RtlEqualUnicodeString((PUNICODE_STRING)this, &String, FALSE);
}

BOOL UnicodeString::operator==(PCUNICODE_STRING String) const
{
	return RtlEqualUnicodeString((PUNICODE_STRING)this, (PUNICODE_STRING)String, FALSE);
}

BOOL UnicodeString::operator==(const UnicodeString &String) const
{
	return RtlEqualUnicodeString((PUNICODE_STRING)this, (PUNICODE_STRING)&String, FALSE);
}

UnicodeString& UnicodeString::operator=(PCWSTR Buffer)
{
	if (*this == Buffer)
		return *this;

	Create(Buffer);

	return *this;
}

UnicodeString& UnicodeString::operator=(PCUNICODE_STRING Buffer)
{
	if (*this == Buffer)
		return *this;

	Create(Buffer);

	return *this;
}

/************************************************************************
UNICODE_STRING helper class end
************************************************************************/

ML_NAMESPACE_END_(Str);

ML_NAMESPACE_BEGIN(Reg)

PVOID AllocateKeyInfo(ULONG_PTR Size)
{
	return AllocateMemoryP(Size);
}

VOID FreeKeyInfo(PVOID Info)
{
	FreeMemoryP(Info);
}

NTSTATUS OpenPredefinedKeyHandle(PHANDLE KeyHandle, HANDLE PredefinedKey, ACCESS_MASK DesiredAccess)
{
	ULONG_PTR           Length;
	NTSTATUS            Status;
	OBJECT_ATTRIBUTES   ObjectAttributes;
	UNICODE_STRING      AttributeValue;
	UNICODE_STRING      KeyPath;
	PWSTR               LocalKeyBuffer;

	API_POINTER(ZwOpenKey)  XOpenKey;

	static WCHAR LocalMachineSuffix[] = L"_Classes";
	static WCHAR LocalSettingsSuffix[] = L"\\Local Settings";

	if ((LONG)(LONG_PTR)PredefinedKey >= 0)
		return STATUS_INVALID_HANDLE;

#if ML_KERNEL_MODE

	XOpenKey = ZwOpenKey;

#else

	XOpenKey = NtOpenKey;

#endif


#pragma push_macro("KEY_INDEX")
#define KEY_INDEX(_Key) ((ULONG_PTR)(_Key) & ~(1 << (bitsof(LONG) - 1)))

	Status = STATUS_INVALID_HANDLE;
	switch (KEY_INDEX(PredefinedKey))
	{
	case KEY_INDEX(HKEY_CLASSES_ROOT):
		Status = RtlFormatCurrentUserKeyPath(&KeyPath);
		FAIL_BREAK(Status);

		Length = KeyPath.Length;
		LocalKeyBuffer = (PWSTR)AllocStack(KeyPath.Length + sizeof(LocalMachineSuffix));
		CopyMemory(LocalKeyBuffer, KeyPath.Buffer, Length);
		CopyMemory(PtrAdd(LocalKeyBuffer, Length), LocalMachineSuffix, sizeof(LocalMachineSuffix));

		RtlFreeUnicodeString(&KeyPath);

		KeyPath.Buffer = LocalKeyBuffer;
		KeyPath.Length = (USHORT)(Length + CONST_STRLEN(LocalMachineSuffix) * sizeof(WCHAR));
		KeyPath.MaximumLength = KeyPath.Length;

		InitializeObjectAttributes(&ObjectAttributes, &KeyPath, OBJ_CASE_INSENSITIVE, NULL, NULL);
		Status = XOpenKey(KeyHandle, DesiredAccess, &ObjectAttributes);

		if (NT_SUCCESS(Status))
			break;

		NO_BREAK;

	case KEY_INDEX(HKEY_MACHINE_CLASS):
		RtlInitUnicodeString(&AttributeValue, L"\\Registry\\Machine\\Software\\Classes");
		InitializeObjectAttributes(&ObjectAttributes, &AttributeValue, OBJ_CASE_INSENSITIVE, NULL, NULL);
		Status = XOpenKey(KeyHandle, DesiredAccess, &ObjectAttributes);
		break;

#if ML_USER_MODE

	case KEY_INDEX(HKEY_CURRENT_USER):
		Status = RtlOpenCurrentUser(DesiredAccess, KeyHandle);
		break;

#endif // r3

	case KEY_INDEX(HKEY_LOCAL_MACHINE):
		RtlInitUnicodeString(&AttributeValue, L"\\Registry\\Machine");
		InitializeObjectAttributes(&ObjectAttributes, &AttributeValue, OBJ_CASE_INSENSITIVE, NULL, NULL);
		Status = XOpenKey(KeyHandle, DesiredAccess, &ObjectAttributes);
		break;

	case KEY_INDEX(HKEY_USERS):
		RtlInitUnicodeString(&AttributeValue, L"\\Registry\\User");
		InitializeObjectAttributes(&ObjectAttributes, &AttributeValue, OBJ_CASE_INSENSITIVE, NULL, NULL);
		Status = XOpenKey(KeyHandle, DesiredAccess, &ObjectAttributes);
		break;

	case KEY_INDEX(HKEY_PERFORMANCE_DATA):
	case KEY_INDEX(HKEY_PERFORMANCE_TEXT):
	case KEY_INDEX(HKEY_PERFORMANCE_NLSTEXT):
		*KeyHandle = (HANDLE)PredefinedKey;
		Status = STATUS_SUCCESS;
		break;

	case KEY_INDEX(HKEY_CURRENT_CONFIG):
		RtlInitUnicodeString(&AttributeValue, L"\\Registry\\Machine\\System\\CurrentControlSet\\Hardware Profiles\\Current");
		InitializeObjectAttributes(&ObjectAttributes, &AttributeValue, OBJ_CASE_INSENSITIVE, NULL, NULL);
		Status = XOpenKey(KeyHandle, DesiredAccess, &ObjectAttributes);
		break;

	case KEY_INDEX(HKEY_CURRENT_USER_LOCAL_SETTINGS):
		Status = RtlFormatCurrentUserKeyPath(&KeyPath);
		FAIL_BREAK(Status);

		Length = KeyPath.Length;
		LocalKeyBuffer = (PWSTR)AllocStack(KeyPath.Length + sizeof(LocalMachineSuffix) + sizeof(LocalSettingsSuffix));
		CopyMemory(LocalKeyBuffer, KeyPath.Buffer, Length);
		CopyMemory(PtrAdd(LocalKeyBuffer, Length), LocalMachineSuffix, CONST_STRLEN(LocalMachineSuffix) * sizeof(WCHAR));
		Length += CONST_STRLEN(LocalMachineSuffix) * sizeof(WCHAR);
		CopyMemory(PtrAdd(LocalKeyBuffer, Length), LocalSettingsSuffix, sizeof(LocalSettingsSuffix));
		Length += CONST_STRLEN(LocalSettingsSuffix) * sizeof(WCHAR);

		RtlFreeUnicodeString(&KeyPath);

		KeyPath.Buffer = LocalKeyBuffer;
		KeyPath.Length = (USHORT)Length;
		KeyPath.MaximumLength = KeyPath.Length;

		InitializeObjectAttributes(&ObjectAttributes, &KeyPath, OBJ_CASE_INSENSITIVE, NULL, NULL);
		Status = XOpenKey(KeyHandle, DesiredAccess, &ObjectAttributes);
		break;

	case KEY_INDEX(HKEY_DYN_DATA):
		Status = STATUS_NOT_IMPLEMENTED;
		break;
	}

#pragma pop_macro("KEY_INDEX")

	return Status;
}

ForceInline NTSTATUS CloseKeyHandle(HANDLE KeyHandle)
{
	if (KeyHandle == nullptr)
		return STATUS_INVALID_HANDLE;

#if ML_KERNEL_MODE
	return ZwClose(KeyHandle);
#else
	return NtClose(KeyHandle);
#endif
}

ForceInline NTSTATUS CloseMappedPredefinedKeyHandle(HANDLE KeyHandle)
{
	return CloseKeyHandle(KeyHandle);
}

NTSTATUS
OpenKey(
	PHANDLE     KeyHandle,
	HANDLE      hKey,
	ACCESS_MASK DesiredAccess,
	PCWSTR      SubKey
)
{
	BOOL                Retry;
	NTSTATUS            Status;
	HANDLE              RootKey;
	UNICODE_STRING      SubKeyString;
	OBJECT_ATTRIBUTES   ObjectAttributes;

	API_POINTER(ZwOpenKey)  XOpenKey;

#if ML_KERNEL_MODE

	XOpenKey = ZwOpenKey;

#else

	XOpenKey = NtOpenKey;

#endif

	RtlInitUnicodeString(&SubKeyString, SubKey);
	Retry = FALSE;

	LOOP_FOREVER
	{
		Status = OpenPredefinedKeyHandle(&RootKey, hKey, DesiredAccess);
		FAIL_RETURN(Status);

		InitializeObjectAttributes(&ObjectAttributes, &SubKeyString, OBJ_CASE_INSENSITIVE, RootKey, nullptr);
		Status = NtOpenKey(KeyHandle, DesiredAccess, &ObjectAttributes);

		CloseKeyHandle(RootKey);

		if (Retry == FALSE && Status == STATUS_OBJECT_NAME_NOT_FOUND && hKey == HKEY_CLASSES_ROOT)
		{
			Retry = TRUE;
			hKey = HKEY_MACHINE_CLASS;
			continue;
		}

		break;
	}

	return Status;
}

NTSTATUS
GetKeyValue(
	HANDLE                          hKey,
	PCWSTR                          SubKey,
	PCWSTR                          ValueName,
	PKEY_VALUE_PARTIAL_INFORMATION* Value
)
{
	NTSTATUS    Status;
	ULONG       Length;
	PKEY_VALUE_PARTIAL_INFORMATION Buffer;

	Buffer = NULL;
	Length = 0;

	LOOP_FOREVER
	{
		Status = GetKeyValue(hKey, SubKey, ValueName, KeyValuePartialInformation, Buffer, Length, &Length, 0);
		if (Status != STATUS_BUFFER_TOO_SMALL)
			break;

		FreeKeyInfo(Buffer);
		Buffer = (PKEY_VALUE_PARTIAL_INFORMATION)AllocateKeyInfo(Length + sizeof(WCHAR));
		if (Buffer == NULL)
			return STATUS_NO_MEMORY;

		*(PWSTR)PtrAdd(Buffer, Length) = 0;
	}

		if (NT_FAILED(Status))
		{
			FreeKeyInfo(Buffer);
		}
		else
		{
			*Value = Buffer;
		}

	return Status;
}

NTSTATUS
GetKeyValue(
	HANDLE                      hKey,
	PCWSTR                      SubKey,
	PCWSTR                      ValueName,
	KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
	PVOID                       KeyValueInformation,
	ULONG                       Length,
	PULONG                      ResultLength,
	ULONG                       Flags
)
{
	ULONG               ReturnLength;
	HANDLE              KeyHandle;
	NTSTATUS            Status;
	UNICODE_STRING      SubKeyString, ValueNameString;

	API_POINTER(ZwCreateKey)        XCreateKey;
	API_POINTER(ZwOpenKey)          XOpenKey;
	API_POINTER(ZwQueryValueKey)    XQueryValueKey;

#if ML_KERNEL_MODE

	XCreateKey = ZwCreateKey;
	XOpenKey = ZwOpenKey;
	XQueryValueKey = ZwQueryValueKey;

#else

	XCreateKey = NtCreateKey;
	XOpenKey = NtOpenKey;
	XQueryValueKey = NtQueryValueKey;

#endif

	Status = OpenKey(&KeyHandle, hKey, KEY_QUERY_VALUE, SubKey);
	FAIL_RETURN(Status);

	ReturnLength = 0;
	RtlInitUnicodeString(&ValueNameString, ValueName);
	Status = XQueryValueKey(
		KeyHandle,
		&ValueNameString,
		KeyValueInformationClass,
		KeyValueInformation,
		Length,
		&ReturnLength
	);

	if (KeyHandle != (HANDLE)hKey)
		CloseKeyHandle(KeyHandle);

	if ((NT_SUCCESS(Status) || Status == STATUS_BUFFER_TOO_SMALL) && ResultLength != NULL)
		*ResultLength = ReturnLength;

	return Status;
}

NTSTATUS
SetKeyValue(
	HANDLE      hKey,
	PCWSTR      SubKey,
	PCWSTR      ValueName,
	ULONG       ValueType,
	LPCVOID     ValueData,
	ULONG       ValueDataLength,
	ULONG_PTR   Flags
)
{
	HANDLE              KeyHandle, RootKey;
	NTSTATUS            Status;
	UNICODE_STRING      SubKeyString, ValueNameString;
	OBJECT_ATTRIBUTES   ObjectAttributes;

	API_POINTER(ZwCreateKey)    XCreateKey;
	API_POINTER(ZwSetValueKey)  XSetValueKey;

#if ML_KERNEL_MODE
	XCreateKey = ZwCreateKey;
	XSetValueKey = ZwSetValueKey;
#else
	XCreateKey = NtCreateKey;
	XSetValueKey = NtSetValueKey;
#endif

	RootKey = NULL;

	if (SubKey == NULL || SubKey[0] == 0)
	{
		KeyHandle = (HANDLE)hKey;
	}
	else
	{
		Status = OpenPredefinedKeyHandle(&RootKey, (HKEY)hKey);
		if (!NT_SUCCESS(Status))
			return Status;

		RtlInitUnicodeString(&SubKeyString, SubKey);
		InitializeObjectAttributes(
			&ObjectAttributes,
			&SubKeyString,
			OBJ_CASE_INSENSITIVE,
			RootKey,
			NULL
		);

		Status = XCreateKey(
			&KeyHandle,
			KEY_SET_VALUE | Flags,
			&ObjectAttributes,
			0,
			NULL,
			0,
			NULL
		);

		if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
		{
			PWSTR Name, End;

			Name = (PWSTR)ObjectAttributes.ObjectName->Buffer;
			End = PtrAdd(Name, ObjectAttributes.ObjectName->Length);

			while (Name < End)
			{
				if (*Name != '\\')
				{
					++Name;
					continue;
				}

				ObjectAttributes.ObjectName->Length = (USHORT)PtrOffset(Name, ObjectAttributes.ObjectName->Buffer);
				++Name;

				Status = XCreateKey(
					&KeyHandle,
					0x020000000 | Flags,    // MAXIMUM_ALLOWED ?
					&ObjectAttributes,
					0,
					NULL,
					0,
					NULL
				);

				if (!NT_SUCCESS(Status))
					goto _EXIT;

				CloseKeyHandle(KeyHandle);
			}

			ObjectAttributes.ObjectName->Length = (USHORT)PtrOffset(End, ObjectAttributes.ObjectName->Buffer);
			Status = XCreateKey(
				&KeyHandle,
				KEY_SET_VALUE | Flags,
				&ObjectAttributes,
				0,
				NULL,
				0,
				NULL
			);
		}

		if (!NT_SUCCESS(Status))
			goto _EXIT;
	}

	RtlInitUnicodeString(&ValueNameString, ValueName);

	Status = XSetValueKey(
		KeyHandle,
		&ValueNameString,
		0,
		ValueType,
		(PVOID)ValueData,
		ValueDataLength
	);

	if (KeyHandle != (HANDLE)hKey)
		CloseKeyHandle(KeyHandle);

_EXIT:

	if (RootKey != NULL)
		CloseMappedPredefinedKeyHandle(RootKey);

	return Status;
}

NTSTATUS
DeleteKey(
	HANDLE      hKey,
	PCWSTR      SubKey,
	ULONG_PTR   Flags
)
{
	HANDLE              KeyHandle, RootKey;
	NTSTATUS            Status;
	UNICODE_STRING      SubKeyString;
	OBJECT_ATTRIBUTES   ObjectAttributes;

	RootKey = NULL;
	if (SubKey == NULL || SubKey[0] == 0)
	{
		return STATUS_INVALID_PARAMETER;
	}

	Status = OpenPredefinedKeyHandle(&RootKey, (HKEY)hKey);
	FAIL_RETURN(Status);

	LOOP_ONCE
	{
		RtlInitUnicodeString(&SubKeyString, SubKey);
		InitializeObjectAttributes(&ObjectAttributes, &SubKeyString, OBJ_CASE_INSENSITIVE, RootKey, NULL);
		Status = ZwOpenKey(&KeyHandle, KEY_ALL_ACCESS | Flags, &ObjectAttributes);
		if (Status == STATUS_OBJECT_NAME_NOT_FOUND && hKey == HKEY_CLASSES_ROOT)
		{
			Status = DeleteKey(HKEY_MACHINE_CLASS, SubKey);
			break;
		}
		else if (NT_FAILED(Status))
		{
			break;
		}

		Status = ZwDeleteKey(KeyHandle);

		if (KeyHandle != (HANDLE)hKey)
			CloseKeyHandle(KeyHandle);
	}

		if (RootKey != NULL)
			CloseMappedPredefinedKeyHandle(RootKey);

	return Status;
}

NTSTATUS
DeleteKeyValue(
	HANDLE      hKey,
	PCWSTR      SubKey,
	PCWSTR      ValueName,
	ULONG_PTR   Flags
)
{
	HANDLE              KeyHandle, RootKey;
	NTSTATUS            Status;
	UNICODE_STRING      SubKeyString;
	OBJECT_ATTRIBUTES   ObjectAttributes;

	RootKey = NULL;
	if (SubKey == NULL || SubKey[0] == 0)
	{
		KeyHandle = (HANDLE)hKey;
	}
	else
	{
		Status = OpenPredefinedKeyHandle(&RootKey, (HKEY)hKey);
		if (!NT_SUCCESS(Status))
			return Status;

		RtlInitUnicodeString(&SubKeyString, SubKey);
		InitializeObjectAttributes(&ObjectAttributes, &SubKeyString, OBJ_CASE_INSENSITIVE, RootKey, NULL);
		Status = ZwOpenKey(&KeyHandle, KEY_SET_VALUE | Flags, &ObjectAttributes);
		if (NT_FAILED(Status))
			goto _EXIT;
	}

	RtlInitUnicodeString(&SubKeyString, ValueName);
	Status = ZwDeleteValueKey(KeyHandle, &SubKeyString);

	if (KeyHandle != (HANDLE)hKey)
		CloseKeyHandle(KeyHandle);

_EXIT:

	if (RootKey != NULL)
		CloseMappedPredefinedKeyHandle(RootKey);

	return Status;
}

ML_NAMESPACE_END_(Reg);

ML_NAMESPACE_BEGIN(Exp);

NTSTATUS
ExceptionBox(
	PCWSTR      Text,
	PCWSTR      Title,
	PULONG_PTR  Response,
	ULONG_PTR   Type
)
{
	UNICODE_STRING  HardErrorTitle, HardErrorText;
	ULONG_PTR       HardErrorParameters[3];
	ULONG           LocalResponse;
	NTSTATUS        Status;

	RtlInitUnicodeString(&HardErrorTitle, Title);
	RtlInitUnicodeString(&HardErrorText, Text);

	HardErrorParameters[0] = (ULONG_PTR)&HardErrorText;
	HardErrorParameters[1] = (ULONG_PTR)&HardErrorTitle;
	HardErrorParameters[2] = Type;

#if ML_USER_MODE

	Status = NtRaiseHardError(STATUS_SERVICE_NOTIFICATION, countof(HardErrorParameters), 1 | 2, HardErrorParameters, 0, &LocalResponse);

#elif ML_KERNEL_MODE

	Status = ExRaiseHardError(STATUS_SERVICE_NOTIFICATION, countof(HardErrorParameters), 1 | 2, HardErrorParameters, 0, &LocalResponse);

#endif

	if (NT_SUCCESS(Status) && Response != NULL)
		*Response = LocalResponse;

	return Status;
}

ML_NAMESPACE_END_(Exp);

ML_NAMESPACE_BEGIN(Lpc);

#if ML_USER_MODE

/************************************************************************
InterProcessLpcServer
************************************************************************/

#define VERIFY_IPC_TYPE(_type) if (this->IpcType != (_type)) return STATUS_OBJECT_TYPE_MISMATCH
#define VERIFY_IPC_TYPE_(_obj, _type) if ((_obj)->IpcType != (_type)) return STATUS_OBJECT_TYPE_MISMATCH

InterProcessLpcServer::InterProcessLpcServer()
{
	Reset();
}

InterProcessLpcServer::~InterProcessLpcServer()
{
	ReleaseAll();
}

VOID InterProcessLpcServer::Reset()
{
	IpcType = IPCType_None;
	PortHandle = nullptr;

	ZeroMemory(&Connected.ServerView, sizeof(Connected.ServerView));
	ZeroMemory(&Connected.ClientView, sizeof(Connected.ClientView));
}

IPC_STATUS InterProcessLpcServer::ReleaseAll()
{
	if (PortHandle != nullptr)
	{
		ZwClose(PortHandle);
	}

	if (IpcType == IPCType_ServerConnected && Connected.ServerView.SectionHandle != nullptr)
	{
		ZwClose(Connected.ServerView.SectionHandle);
	}

	Reset();

	return STATUS_SUCCESS;
}


/*++

listening

--*/

IPC_STATUS
InterProcessLpcServer::
Create(
	PWSTR Name,
	ULONG MaxConnectionInfoLength,
	ULONG MaxMessageLength
)
{
	NTSTATUS            Status;
	OBJECT_ATTRIBUTES   ObjectAttributes;
	UNICODE_STRING      PortName;

	VERIFY_IPC_TYPE(IPCType_None);

	RtlInitUnicodeString(&PortName, Name);
	InitializeObjectAttributes(&ObjectAttributes, &PortName, 0, nullptr, nullptr);

	Status = NtCreatePort(&LpcPort, &ObjectAttributes, MaxConnectionInfoLength, MaxMessageLength, 0);
	FAIL_RETURN(Status);

	IpcType = IPCType_ServerListen;

	Listening.MaxConnectionInfoLength = MaxConnectionInfoLength;
	Listening.MaxMessageLength = MaxMessageLength;
	Listening.MaxSmallDataLength = MaxMessageLength - sizeof(PORT_MESSAGE) - IPC_MESSAGE::kOverhead;

	return Status;
}

IPC_STATUS InterProcessLpcServer::Listen(PIPC_MESSAGE ConnectionRequest, ULONG Timeout)
{
	LARGE_INTEGER LocalTimeout;

	FormatTimeOut(&LocalTimeout, Timeout);
	return Listen(ConnectionRequest, Timeout == INFINITE ? nullptr : &LocalTimeout);
}

IPC_STATUS InterProcessLpcServer::Listen(PIPC_MESSAGE ConnectionRequest, PLARGE_INTEGER Timeout)
{
	NTSTATUS Status;

	VERIFY_IPC_TYPE(IPCType_ServerListen);

	ZeroMemory(ConnectionRequest, sizeof(*ConnectionRequest));

	LOOP_FOREVER
	{
		Status = NtReplyWaitReceivePortEx(LpcPort, nullptr, nullptr, ConnectionRequest, Timeout);
		if (Status != STATUS_SUCCESS ||
			(ConnectionRequest->u2.s2.Type & ~LPC_KERNELMODE_MESSAGE) == LPC_CONNECTION_REQUEST)
		{
			break;
		}
	}

	return Status;
}

IPC_STATUS
InterProcessLpcServer::
Accept(
	InterProcessLpcServer&  ConnectedPort,
	PIPC_MESSAGE            ConnectionRequest,
	BOOL                    AcceptConnection,
	PVOID                   Context
)
{
	NTSTATUS    Status;
	ULONG64     MaxMessageLength;

	VERIFY_IPC_TYPE(IPCType_ServerListen);
	VERIFY_IPC_TYPE_(&ConnectedPort, IPCType_None);

	if ((ConnectionRequest->u2.s2.Type & ~LPC_KERNELMODE_MESSAGE) != LPC_CONNECTION_REQUEST)
		return STATUS_UNSUCCESSFUL;

	MaxMessageLength = ConnectionRequest->MessageData.ConnectionRequest.MaxMessageLength;

	if (MaxMessageLength > Listening.MaxSmallDataLength)
	{
		OBJECT_ATTRIBUTES   ObjectAttributes;
		LARGE_INTEGER       MaximumSize;

		MaximumSize.QuadPart = MaxMessageLength;
		InitializeObjectAttributes(&ObjectAttributes, nullptr, 0, nullptr, nullptr);

		Status = ZwCreateSection(
			&ConnectedPort.Connected.ServerView.SectionHandle,
			SECTION_ALL_ACCESS,
			&ObjectAttributes,
			&MaximumSize,
			PAGE_READWRITE,
			SEC_COMMIT,
			nullptr
		);

		if (NT_FAILED(Status))
			return Status;

		ConnectedPort.Connected.ServerView.Length = sizeof(Connected.ServerView);
		ConnectedPort.Connected.ServerView.SectionOffset = 0;
		ConnectedPort.Connected.ServerView.ViewSize = (LPC_SIZE_T)MaximumSize.QuadPart;

		ConnectedPort.Connected.ClientView.Length = sizeof(Connected.ClientView);

		Status = NtAcceptConnectPort(
			&ConnectedPort.ConnectedPort,
			&ConnectedPort,
			ConnectionRequest,
			(BOOLEAN)AcceptConnection,
			&ConnectedPort.Connected.ServerView,
			&ConnectedPort.Connected.ClientView
		);
	}
	else
	{
		Status = NtAcceptConnectPort(
			&ConnectedPort.ConnectedPort,
			&ConnectedPort,
			ConnectionRequest,
			(BOOLEAN)AcceptConnection,
			nullptr,
			nullptr
		);
	}

	if (NT_SUCCESS(Status))
		Status = NtCompleteConnectPort(ConnectedPort.ConnectedPort);

	if (NT_FAILED(Status))
	{
		ConnectedPort.ReleaseAll();

		return Status;
	}

	ConnectedPort.IpcType = IPCType_ServerConnected;
	ConnectedPort.Connected.Context = Context;

	return Status;
}


/*++

listening end

--*/


/*++

connected

--*/

IPC_STATUS
InterProcessLpcServer::
Receive(
	OUT PVOID           Buffer,
	IN  ULONG_PTR       Size,
	OUT PULONG_PTR      BytesTransfered,
	OUT PVOID*          Context,
	IN  ULONG           Timeout
)
{
	LARGE_INTEGER LocalTimeout;

	FormatTimeOut(&LocalTimeout, Timeout);
	return Receive(Buffer, Size, BytesTransfered, Context, &LocalTimeout);
}

IPC_STATUS
InterProcessLpcServer::
Receive(
	OUT PVOID           Buffer,
	IN  ULONG_PTR       Size,
	OUT PULONG_PTR      BytesTransfered,
	OUT PVOID*          Context,
	IN  PLARGE_INTEGER  Timeout
)
{
	NTSTATUS                Status;
	PVOID                   DataBuffer;
	InterProcessLpcServer*  AssociatedPort;
	IPC_MESSAGE             ReceiveMessage;

	Status = NtReplyWaitReceivePortEx(ConnectedPort, (PVOID *)&AssociatedPort, nullptr, &ReceiveMessage, Timeout);
	FAIL_RETURN(Status);

	if (AssociatedPort != this)
	{
		// what the fuck
	}

	DataBuffer = AssociatedPort->GetDataBuffer(&ReceiveMessage);

	Size = (ULONG_PTR)ML_MIN(Size, ReceiveMessage.MessageDataLength);

	if (Buffer != nullptr)
		CopyMemory(Buffer, DataBuffer, Size);

	if (BytesTransfered != nullptr)
		*BytesTransfered = Size;

	if (Context != nullptr)
		*Context = AssociatedPort->Connected.Context;

	return Status;
}


/*++

connected end

--*/


/************************************************************************
InterProcessLpcClient
************************************************************************/

InterProcessLpcClient::InterProcessLpcClient()
{
	Reset();
}

InterProcessLpcClient::~InterProcessLpcClient()
{
	ReleaseAll();
}

VOID InterProcessLpcClient::Reset()
{
	IpcType = IPCType_None;
	ConnetionPort = nullptr;

	ZeroMemory(&ServerView, sizeof(ServerView));
	ZeroMemory(&ClientView, sizeof(ClientView));
}

IPC_STATUS InterProcessLpcClient::ReleaseAll()
{
	if (ConnetionPort != nullptr)
	{
		ZwClose(ConnetionPort);
	}

	Reset();

	return STATUS_SUCCESS;
}

LPC_STATUS
InterProcessLpcClient::
Connect(
	PWSTR Name,
	ULONG MaxMessageLength
)
{
	ULONG           ConnectionInformationLength;
	NTSTATUS        Status;
	UNICODE_STRING  PortName;
	IPC_MESSAGE     ConnectionMessage;

	VERIFY_IPC_TYPE(IPCType_None);

	RtlInitUnicodeString(&PortName, Name);

	ConnectionMessage.MessageType = IpcMessageType_ConnectionRequest;
	ConnectionMessage.MessageDataLength = sizeof(ConnectionMessage.MessageData.ConnectionRequest);
	ConnectionMessage.MessageData.ConnectionRequest.MaxMessageLength = MaxMessageLength;

	ConnectionInformationLength = (ULONG)ConnectionMessage.MessageDataLength;

	Status = NtConnectPort(
		&ConnetionPort,
		&PortName,
		nullptr,
		&ClientView,
		&ServerView,
		&MaxMessageLength,
		PtrAdd(&ConnectionMessage, sizeof(PORT_MESSAGE)),
		&ConnectionInformationLength
	);

	FAIL_RETURN(Status);

	IpcType = IPCType_Client;

	MaxSmallDataLength = MaxMessageLength - sizeof(PORT_MESSAGE) - IPC_MESSAGE::kOverhead;

	return Status;
}

#endif // r3

ML_NAMESPACE_END_(Lpc);
#include "mountmgr.h"

ML_NAMESPACE_BEGIN(Io);

VOID IopCopyFindData(PML_FIND_DATA FindFileData, PFILE_BOTH_DIR_INFORMATION FileInfo)
{
	ULONG Length;

	FindFileData->FileAttributes = FileInfo->FileAttributes;

	FindFileData->CreationTime = FileInfo->CreationTime;
	FindFileData->LastAccessTime = FileInfo->LastAccessTime;
	FindFileData->LastWriteTime = FileInfo->LastWriteTime;
	FindFileData->FileSize = FileInfo->EndOfFile;

	if (FLAG_ON(FileInfo->FileAttributes, FILE_ATTRIBUTE_REPARSE_POINT))
		FindFileData->Reserved0 = FileInfo->EaSize;

	Length = ML_MIN(FileInfo->FileNameLength, sizeof(FindFileData->FileName));
	CopyMemory(FindFileData->FileName, FileInfo->FileName, Length);
	if (Length < sizeof(FindFileData->FileName))
		FindFileData->FileName[Length / sizeof(WCHAR)] = 0;

	Length = ML_MIN(FileInfo->ShortNameLength, sizeof(FindFileData->AlternateFileName));
	CopyMemory(FindFileData->AlternateFileName, FileInfo->ShortName, Length);
	if (Length < sizeof(FindFileData->AlternateFileName))
		FindFileData->AlternateFileName[Length / sizeof(WCHAR)] = 0;
}

NTSTATUS
IopQueryFileBothDirectoryInformation(
	HANDLE                      DirectoryHandle,
	BOOL                        RestartScan,
	PUNICODE_STRING             FileName,
	PFILE_BOTH_DIR_INFORMATION* FileBothInformation
)
{
	NTSTATUS                    Status;
	ULONG_PTR                   FileInfoSize;
	IO_STATUS_BLOCK             IoStatus;
	PFILE_BOTH_DIR_INFORMATION  LocalFileBothInformation;

	API_POINTER(ZwQueryDirectoryFile) XZwQueryDirectoryFile;

	FileInfoSize = MAX_NTPATH * sizeof(WCHAR) + sizeof(*LocalFileBothInformation);
	LocalFileBothInformation = (PFILE_BOTH_DIR_INFORMATION)AllocateMemory(FileInfoSize);
	if (LocalFileBothInformation == NULL)
		return STATUS_NO_MEMORY;

#if ML_KERNEL_MODE

	XZwQueryDirectoryFile = ZwQueryDirectoryFile;

#elif ML_USER_MODE

	XZwQueryDirectoryFile = NtQueryDirectoryFile;

#endif

	LOOP_FOREVER
	{
		Status = XZwQueryDirectoryFile(
		DirectoryHandle,
		NULL,
		NULL,
		NULL,
		&IoStatus,
		LocalFileBothInformation,
		FileInfoSize,
		FileBothDirectoryInformation,
		TRUE,
		FileName,
		(BOOLEAN)RestartScan
		);

		if (Status != STATUS_BUFFER_OVERFLOW)
			break;

		FreeMemory(LocalFileBothInformation);
		FileInfoSize *= 2;
		LocalFileBothInformation = (PFILE_BOTH_DIR_INFORMATION)AllocateMemory(FileInfoSize);

		if (LocalFileBothInformation == NULL)
		{
			Status = STATUS_NO_MEMORY;
			break;
		}
	}

		if (NT_SUCCESS(Status))
		{
			*FileBothInformation = LocalFileBothInformation;
		}
		else
		{
			FreeMemory(LocalFileBothInformation);
		}

	return Status;
}

NTSTATUS QueryFirstFile(PHANDLE Handle, PCWSTR FileName, PML_FIND_DATA FindFileData)
{
	NTSTATUS                    Status;
	ULONG_PTR                   Length, FirstChar;
	HANDLE                      DirectoryHandle;
	PWSTR                       LocalFileName, FileNameBuffer;
	UNICODE_STRING              NtPath, BaseFileName;
	IO_STATUS_BLOCK             IoStatus;
	PFILE_BOTH_DIR_INFORMATION  FileBothInformation;

	Length = StrLengthW(FileName);
	Length -= FileName[Length - 1] == '\\';

	FileNameBuffer = (PWSTR)AllocateMemory(Length * sizeof(WCHAR) + sizeof(WCHAR));
	if (FileNameBuffer == NULL)
		return STATUS_NO_MEMORY;

	CopyMemory(FileNameBuffer, FileName, Length * sizeof(WCHAR));
	FileNameBuffer[Length] = 0;

	Status = DosPathNameToNtPathName(FileNameBuffer, &NtPath, (PCWSTR *)&BaseFileName.Buffer);
	FreeMemory(FileNameBuffer);
	FAIL_RETURN(Status);

	LocalFileName = (PWSTR)BaseFileName.Buffer;
	if (LocalFileName == NULL)
	{
		LocalFileName = PtrAdd((PWSTR)NtPath.Buffer, NtPath.Length);
		BaseFileName.Buffer = LocalFileName;
	}

	BaseFileName.Length = (USHORT)(NtPath.Length - (LocalFileName - NtPath.Buffer) * sizeof(WCHAR));
	BaseFileName.MaximumLength = BaseFileName.Length;

	FirstChar = *LocalFileName;
	*LocalFileName = 0;

	Status = NtFileDisk::OpenDirectory(&DirectoryHandle, NtPath.Buffer, NFD_NOT_RESOLVE_PATH);
	if (NT_FAILED(Status))
	{
		RtlFreeUnicodeString(&NtPath);
		return Status;
	}

	*LocalFileName = (WCHAR)FirstChar;
	if (BaseFileName.Length == 6 && *(PULONG64)BaseFileName.Buffer == TAG3W('*.*'))
	{
		BaseFileName.Length = 2;
	}
	else
	{
		for (ULONG Length = BaseFileName.Length / sizeof(WCHAR); Length; --Length)
		{
			switch (LocalFileName[0])
			{
			case '?':
				LocalFileName[0] = DOS_QM;
				break;

			case '*':
				if (LocalFileName[1] != '.')
					break;

				LocalFileName[0] = DOS_STAR;
				break;

			case '.':
				switch (LocalFileName[1])
				{
				case '?':
				case '*':
					LocalFileName[0] = DOS_DOT;
					break;
				}

				break;
			}
			++LocalFileName;
		}
	}

	Status = IopQueryFileBothDirectoryInformation(DirectoryHandle, TRUE, &BaseFileName, &FileBothInformation);

	RtlFreeUnicodeString(&NtPath);

	if (NT_FAILED(Status))
	{
		QueryClose(DirectoryHandle);
		return Status;
	}

	IopCopyFindData(FindFileData, FileBothInformation);
	FreeMemory(FileBothInformation);

	*Handle = DirectoryHandle;

	return Status;
}

NTSTATUS QueryNextFile(HANDLE DirectoryHandle, PML_FIND_DATA FindFileData)
{
	NTSTATUS                    Status;
	PFILE_BOTH_DIR_INFORMATION  FileBothInformation;

	Status = IopQueryFileBothDirectoryInformation(DirectoryHandle, FALSE, NULL, &FileBothInformation);

	if (NT_SUCCESS(Status))
	{
		IopCopyFindData(FindFileData, FileBothInformation);
		FreeMemory(FileBothInformation);
	}

	return Status;
}

ULONG_PTR QueryFileAttributes(PCWSTR FileName)
{
	NTSTATUS    Status;
	ULONG_PTR   FileAttributes;

	Status = QueryFileAttributesEx(FileName, &FileAttributes);

	return NT_SUCCESS(Status) ? FileAttributes : INVALID_FILE_ATTRIBUTES;
}

NTSTATUS QueryFileAttributesEx(PCWSTR FileName, PULONG_PTR FileAttributes)
{
#if ML_USER_MODE

	NTSTATUS                Status;
	UNICODE_STRING          NtPath;
	OBJECT_ATTRIBUTES       ObjectAttributes;
	FILE_BASIC_INFORMATION  FileBasic;

	Status = NtFileDisk::QueryFullNtPath(FileName, &NtPath);
	FAIL_RETURN(Status);

	InitializeObjectAttributes(&ObjectAttributes, &NtPath, OBJ_CASE_INSENSITIVE, NULL, 0);
	Status = NtQueryAttributesFile(&ObjectAttributes, &FileBasic);
	RtlFreeUnicodeString(&NtPath);
	FAIL_RETURN(Status);

	*FileAttributes = FileBasic.FileAttributes;

	return Status;

#elif ML_KERNEL_MODE

	return STATUS_NOT_IMPLEMENTED;

#endif

}

NTSTATUS ApplyFileAttributes(PCWSTR FileName, ULONG_PTR Attributes)
{
	ULONG                   Mask;
	NTSTATUS                Status;
	HANDLE                  FileHandle;
	IO_STATUS_BLOCK         IoStatus;
	FILE_BASIC_INFORMATION  FileInfo;
	OBJECT_ATTRIBUTES       ObjectAttributes;
	UNICODE_STRING          FilePath;

	Status = NtFileDisk::QueryFullNtPath(FileName, &FilePath);
	FAIL_RETURN(Status);

	InitializeObjectAttributes(&ObjectAttributes, &FilePath, OBJ_CASE_INSENSITIVE, nullptr, nullptr);
	Status = NtOpenFile(
		&FileHandle,
		SYNCHRONIZE | FILE_WRITE_ATTRIBUTES,
		&ObjectAttributes,
		&IoStatus,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		FILE_OPEN_REPARSE_POINT | FILE_OPEN_FOR_BACKUP_INTENT | FILE_SYNCHRONOUS_IO_NONALERT
	);

	RtlFreeUnicodeString(&FilePath);

	if (NT_FAILED(Status))
		return Status;

	Mask = FILE_ATTRIBUTE_NO_SCRUB_DATA |
		FILE_ATTRIBUTE_OFFLINE | FILE_ATTRIBUTE_NOT_CONTENT_INDEXED |
		FILE_ATTRIBUTE_TEMPORARY |
		FILE_ATTRIBUTE_ARCHIVE |
		FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM;

	ZeroMemory(&FileInfo, sizeof(FileInfo));
	FileInfo.FileAttributes = (Attributes & Mask) | FILE_ATTRIBUTE_NORMAL;

	Status = NtSetInformationFile(FileHandle, &IoStatus, &FileInfo, sizeof(FileInfo), FileBasicInformation);
	NtClose(FileHandle);

	return Status;
}

NTSTATUS MoveFile(PCWSTR ExistingFileName, PCWSTR NewFileName, BOOL ReplaceIfExists)
{
	NTSTATUS                    Status;
	HANDLE                      FileHandle;
	UNICODE_STRING              New;
	PFILE_RENAME_INFORMATION    FileRename;
	IO_STATUS_BLOCK             IoStatus;

	Status = NtFileDisk::Open(
		&FileHandle,
		ExistingFileName,
		0,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		SYNCHRONIZE | DELETE | FILE_READ_ATTRIBUTES
	);
	if (NT_FAILED(Status))
		return Status;

	Status = NtFileDisk::QueryFullNtPath(NewFileName, &New);
	if (NT_FAILED(Status))
	{
		NtClose(FileHandle);
		return Status;
	}

	FileRename = (PFILE_RENAME_INFORMATION)AllocStack(New.Length + sizeof(*FileRename));
	FileRename->ReplaceIfExists = ReplaceIfExists;
	FileRename->FileNameLength = New.Length;
	FileRename->RootDirectory = NULL;
	CopyMemory(FileRename->FileName, New.Buffer, New.Length);
	RtlFreeUnicodeString(&New);

	Status = NtSetInformationFile(
		FileHandle,
		&IoStatus,
		FileRename,
		FileRename->FileNameLength + sizeof(*FileRename),
		FileRenameInformation
	);
	NtClose(FileHandle);

	return Status;
}

NTSTATUS CopyFile(PCWSTR ExistingFileName, PCWSTR NewFileName, BOOL FailIfExists)
{
	if (FailIfExists && IsPathExists(NewFileName))
		return STATUS_OBJECT_NAME_EXISTS;

	PVOID                   Buffer;
	ULONG                   BufferSize;
	LARGE_INTEGER           BytesTransfered;
	NTSTATUS                Status;
	LARGE_INTEGER           FileSize;
	FILE_BASIC_INFORMATION  FileInformation;
	IO_STATUS_BLOCK         IoStatus;
	NtFileDisk             SourceFile, DestinationFile;

	Status = SourceFile.Open(ExistingFileName);
	if (NT_FAILED(Status))
		return Status;

	Status = DestinationFile.Create(NewFileName);
	if (NT_FAILED(Status))
		return Status;

	Status = ZwQueryInformationFile(SourceFile, &IoStatus, &FileInformation, sizeof(FileInformation), FileBasicInformation);
	if (NT_FAILED(Status))
	{
		DestinationFile.Delete();
		return Status;
	}

	Status = ZwSetInformationFile(DestinationFile, &IoStatus, &FileInformation, sizeof(FileInformation), FileBasicInformation);
	if (NT_FAILED(Status))
	{
		DestinationFile.Delete();
		return Status;
	}

	BufferSize = 0x1000;
	Buffer = AllocStack(BufferSize);
	SourceFile.GetSize(&FileSize);
	for (; FileSize.QuadPart; FileSize.QuadPart -= BytesTransfered.QuadPart)
	{
		BytesTransfered.LowPart = MY_MIN(FileSize.LowPart, BufferSize);
		Status = SourceFile.Read(Buffer, BytesTransfered.LowPart, &BytesTransfered);
		if (NT_FAILED(Status))
			break;

		Status = DestinationFile.Write(Buffer, BytesTransfered.LowPart, &BytesTransfered);
		if (NT_FAILED(Status))
			break;
	}

	if (NT_FAILED(Status))
		DestinationFile.Delete();

	return Status;
}

NTSTATUS DeleteFile(PCWSTR FileName)
{
	HANDLE      FileHandle;
	NTSTATUS    Status;

	Status = NtFileDisk::Open(
		&FileHandle,
		FileName,
		0,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		DELETE,
		FILE_OPEN,
		FILE_ATTRIBUTE_NORMAL,
		FILE_NON_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REPARSE_POINT
	);
	if (NT_FAILED(Status))
		return Status;

	Status = NtFileDisk::Delete(FileHandle);
	NtFileDisk::Close(FileHandle);

	return Status;
}

NTSTATUS DeleteDirectory(PCWSTR DirectoryName)
{
	HANDLE      FileHandle;
	NTSTATUS    Status;

	Status = NtFileDisk::Open(
		&FileHandle,
		DirectoryName,
		0,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		DELETE,
		FILE_OPEN,
		FILE_ATTRIBUTE_NORMAL,
		FILE_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REPARSE_POINT
	);
	if (NT_FAILED(Status))
		return Status;

	Status = NtFileDisk::Delete(FileHandle);
	NtFileDisk::Close(FileHandle);

	return Status;
}

BOOL IsPathExists(PCWSTR Path)
{
	return QueryFileAttributes(Path) != INVALID_FILE_ATTRIBUTES;
}

NTSTATUS
QueryDosLetterFromVolumeDeviceName(
	PUNICODE_STRING DosLetter,
	PUNICODE_STRING NtDeviceName
)
{
	NtFileDisk              MountPointManager;
	NTSTATUS                Status;
	PMOUNTMGR_VOLUME_PATHS  VolumePath;
	ULONG_PTR               DosVolLength, ReturnLength;
	PMOUNTMGR_TARGET_NAME   TargetName;

	Status = MountPointManager.OpenDevice(MOUNTMGR_DEVICE_NAME);
	FAIL_RETURN(Status);

	DosVolLength = MAX_NTPATH * sizeof(WCHAR) + sizeof(VolumePath->MultiSzLength);
	VolumePath = (PMOUNTMGR_VOLUME_PATHS)AllocStack(DosVolLength);

	TargetName = (PMOUNTMGR_TARGET_NAME)AllocStack(NtDeviceName->Length + sizeof(TargetName->DeviceNameLength));
	TargetName->DeviceNameLength = NtDeviceName->Length;
	CopyMemory(TargetName->DeviceName, NtDeviceName->Buffer, NtDeviceName->Length);

	LOOP_FOREVER
	{
		Status = MountPointManager.DeviceIoControl(
		IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATH,
		TargetName,
		TargetName->DeviceNameLength + sizeof(TargetName->DeviceNameLength),
		VolumePath,
		DosVolLength,
		&ReturnLength
		);
		if (Status != STATUS_MORE_PROCESSING_REQUIRED)
			break;

		VolumePath = (PMOUNTMGR_VOLUME_PATHS)AllocStack(ReturnLength - DosVolLength + sizeof(WCHAR));
		DosVolLength = ReturnLength;
	}

	FAIL_RETURN(Status);

	*PtrAdd(VolumePath->MultiSz, (ULONG_PTR)VolumePath->MultiSzLength) = 0;
	return RtlCreateUnicodeString(DosLetter, VolumePath->MultiSz) ? STATUS_SUCCESS : STATUS_NO_MEMORY;
}

NTSTATUS QueryDosPathFromHandle(PUNICODE_STRING DosPath, HANDLE FileHandle)
{
	NTSTATUS                    Status;
	OBJECT_NAME_INFORMATION2    ObjectName;
	FILE_NAME_INFORMATION2      FileName;
	IO_STATUS_BLOCK             IoStatus;
	UNICODE_STRING              DosLetter;
	ULONG_PTR                   Length;

	API_POINTER(ZwQueryObject)              XQueryObject;
	API_POINTER(ZwQueryInformationFile)     XQueryInformationFile;

#if ML_KERNEL_MODE

	XQueryObject = ZwQueryObject;
	XQueryInformationFile = ZwQueryInformationFile;

#else

	XQueryObject = NtQueryObject;
	XQueryInformationFile = NtQueryInformationFile;

#endif

	RtlInitEmptyUnicodeString(DosPath, NULL, 0);

	Status = XQueryObject(FileHandle, ObjectNameInformation, &ObjectName, sizeof(ObjectName), NULL);
	FAIL_RETURN(Status);

	Status = XQueryInformationFile(FileHandle, &IoStatus, &FileName, sizeof(FileName), FileNameInformation);
	FAIL_RETURN(Status);

	ObjectName.Name.Length -= (USHORT)FileName.FileNameLength;
	Status = QueryDosLetterFromVolumeDeviceName(&DosLetter, &ObjectName.Name);
	FAIL_RETURN(Status);

	Length = DosLetter.Length;
	CopyMemory(ObjectName.Buffer, DosLetter.Buffer, Length);

	CopyMemory(PtrAdd(ObjectName.Buffer, Length), FileName.FileName, FileName.FileNameLength);

	RtlFreeUnicodeString(&DosLetter);

	Length += FileName.FileNameLength;
	Length /= sizeof(WCHAR);
	ObjectName.Buffer[Length] = 0;

	if (FLAG_ON(QueryFileAttributes(ObjectName.Buffer), FILE_ATTRIBUTE_DIRECTORY))
	{
		if (ObjectName.Buffer[Length - 1] != '\\')
		{
			ObjectName.Buffer[Length++] = '\\';
		}
	}

	ObjectName.Name.Length = (USHORT)(Length * sizeof(WCHAR));

	return RtlDuplicateUnicodeString(RTL_DUPSTR_ADD_NULL, &ObjectName.Name, DosPath);
}

NTSTATUS QueryDosDevice(PCWSTR DeviceName, PUNICODE_STRING TargetPath)
{
	ULONG               Length;
	NTSTATUS            Status;
	HANDLE              Directory, Link;
	OBJECT_ATTRIBUTES   ObjectAttributes;
	UNICODE_STRING      Device, Target;
	PWSTR               TargetBuffer;

#if ML_USER_MODE

	Device = USTR(L"\\??");

	InitializeObjectAttributes(&ObjectAttributes, &Device, OBJ_CASE_INSENSITIVE, NULL, NULL);
	Status = NtOpenDirectoryObject(&Directory, DIRECTORY_QUERY, &ObjectAttributes);
	FAIL_RETURN(Status);

	RtlInitUnicodeString(&Device, DeviceName);
	InitializeObjectAttributes(&ObjectAttributes, &Device, OBJ_CASE_INSENSITIVE, Directory, NULL);
	Status = NtOpenSymbolicLinkObject(&Link, SYMBOLIC_LINK_QUERY, &ObjectAttributes);
	NtClose(Directory);
	FAIL_RETURN(Status);

	RtlInitEmptyUnicodeString(&Target, NULL, 0);

	Status = NtQuerySymbolicLinkObject(Link, &Target, &Length);

	if (Status != STATUS_BUFFER_TOO_SMALL)
	{
		NtClose(Link);
		return Status;
	}

	Length += sizeof(WCHAR);
	TargetBuffer = (PWSTR)AllocStack(Length);
	RtlInitEmptyUnicodeString(&Target, TargetBuffer, (USHORT)Length);

	Status = NtQuerySymbolicLinkObject(Link, &Target, &Length);
	NtClose(Link);

	FAIL_RETURN(Status);

	Length = Target.Length / sizeof(WCHAR);
	if (Target.Buffer[Length - 1] != '\\')
	{
		((PWSTR)Target.Buffer)[Length++] = '\\';
		((PWSTR)Target.Buffer)[Length]   = 0;
		Target.Length = (USHORT)(Length * sizeof(WCHAR));
	}

	return RtlDuplicateUnicodeString(RTL_DUPSTR_ADD_NULL, &Target, TargetPath);

#else

	Device = USTR(L"\\??");

	InitializeObjectAttributes(&ObjectAttributes, &Device, OBJ_CASE_INSENSITIVE, NULL, NULL);
	Status = ZwOpenDirectoryObject(&Directory, DIRECTORY_QUERY, &ObjectAttributes);
	FAIL_RETURN(Status);

	RtlInitUnicodeString(&Device, DeviceName);
	InitializeObjectAttributes(&ObjectAttributes, &Device, OBJ_CASE_INSENSITIVE, Directory, NULL);
	Status = ZwOpenSymbolicLinkObject(&Link, SYMBOLIC_LINK_QUERY, &ObjectAttributes);
	ZwClose(Directory);
	FAIL_RETURN(Status);

	RtlInitEmptyUnicodeString(&Target, NULL, 0);

	Status = ZwQuerySymbolicLinkObject(Link, &Target, &Length);

	if (Status != STATUS_BUFFER_TOO_SMALL)
	{
		ZwClose(Link);
		return Status;
	}

	Length += sizeof(WCHAR);
	TargetBuffer = (PWSTR)AllocStack(Length);
	RtlInitEmptyUnicodeString(&Target, TargetBuffer, Length);

	Status = ZwQuerySymbolicLinkObject(Link, &Target, &Length);
	ZwClose(Link);

	FAIL_RETURN(Status);

	Length = Target.Length / sizeof(WCHAR);
	if (Target.Buffer[Length - 1] != '\\')
	{
		Target.Buffer[Length++] = '\\';
		Target.Buffer[Length] = 0;
		Target.Length = (USHORT)(Length * sizeof(WCHAR));
	}

	return RtlDuplicateUnicodeString(RTL_DUPSTR_ADD_NULL, &Target, TargetPath);

#endif
}

NTSTATUS
QueryDosPathFromNtDeviceName(
	PUNICODE_STRING DosPath,
	PUNICODE_STRING NtDeviceName
)
{
	NTSTATUS        Status;
	ULONG_PTR       Length;
	WCHAR           DosLetter[4];
	PWSTR           DosPathBuffer;
	UNICODE_STRING  DosDevice;

	*(PULONG)&DosLetter[0] = TAG2W('A:');
	*(PULONG)&DosLetter[2] = 0;

	for (ULONG_PTR Count = 'Z' - 'A' + 1; Count; ++DosLetter[0], --Count)
	{
		Status = QueryDosDevice(DosLetter, &DosDevice);
		FAIL_CONTINUE(Status);

		if (RtlCompareMemory(NtDeviceName->Buffer, DosDevice.Buffer, DosDevice.Length) != DosDevice.Length)
		{
			RtlFreeUnicodeString(&DosDevice);
			continue;
		}

		Length = NtDeviceName->Length - DosDevice.Length;
		DosPathBuffer = (PWSTR)AllocStack(Length + sizeof(DosLetter) + sizeof(WCHAR));
		*(PULONG)DosPathBuffer = *(PULONG)DosLetter;
		DosPathBuffer[2] = '\\';

		CopyMemory(DosPathBuffer + 3, PtrAdd(NtDeviceName->Buffer, DosDevice.Length), Length);
		*PtrAdd(DosPathBuffer, Length + 6) = 0;

		RtlFreeUnicodeString(&DosDevice);

		return RtlCreateUnicodeString(DosPath, DosPathBuffer) ? STATUS_SUCCESS : STATUS_NO_MEMORY;
	}

	return STATUS_OBJECT_PATH_NOT_FOUND;
}

NTSTATUS
CreateNamedPipe(
	PHANDLE                 ReadPipe,
	PHANDLE                 WritePipe,
	PUNICODE_STRING         PipeName,
	PSECURITY_ATTRIBUTES    PipeAttributes,
	ULONG_PTR               BufferSize
)
{
	NTSTATUS            Status;
	HANDLE              LocalReadPipe, LocalWritePipe;
	NtFileDisk          NamedPipe;
	OBJECT_ATTRIBUTES   ObjectAttributes;
	IO_STATUS_BLOCK     IoStatus;
	LARGE_INTEGER       DefaultTimeout;
	UNICODE_STRING      PipeObjectName;

	if (BufferSize == 0)
		BufferSize = 0x1000;

	Status = NamedPipe.OpenDevice(
		L"\\Device\\NamedPipe\\",
		0,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		GENERIC_READ | SYNCHRONIZE,
		FILE_OPEN,
		0,
		FILE_SYNCHRONOUS_IO_NONALERT
	);

	FAIL_RETURN(Status);

	if (PipeName != nullptr)
	{
		PipeObjectName = *PipeName;
	}
	else
	{
		RtlInitEmptyString(&PipeObjectName);
	}

	InitializeObjectAttributes(&ObjectAttributes, &PipeObjectName, OBJ_CASE_INSENSITIVE, NamedPipe, nullptr);
	if (PipeAttributes != nullptr)
	{
		ObjectAttributes.SecurityDescriptor = PipeAttributes->lpSecurityDescriptor;
		SET_FLAG(ObjectAttributes.Attributes, PipeAttributes->bInheritHandle ? OBJ_INHERIT : 0);
	}

	FormatTimeOut(&DefaultTimeout, 120000);

	Status = NtCreateNamedPipeFile(
		&LocalReadPipe,
		GENERIC_READ | SYNCHRONIZE | FILE_WRITE_ATTRIBUTES,
		&ObjectAttributes,
		&IoStatus,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		FILE_CREATE,
		FILE_SYNCHRONOUS_IO_NONALERT,
		FILE_PIPE_BYTE_STREAM_TYPE,
		FILE_PIPE_BYTE_STREAM_MODE,
		FILE_PIPE_QUEUE_OPERATION,
		1,
		BufferSize,
		BufferSize,
		&DefaultTimeout
	);
	FAIL_RETURN(Status);

	ObjectAttributes.RootDirectory = LocalReadPipe;

	Status = NtOpenFile(
		&LocalWritePipe,
		GENERIC_WRITE | SYNCHRONIZE | FILE_READ_ATTRIBUTES,
		&ObjectAttributes,
		&IoStatus,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE
	);

	if (NT_FAILED(Status))
	{
		NtClose(LocalReadPipe);
		return Status;
	}

	*ReadPipe = LocalReadPipe;
	*WritePipe = LocalWritePipe;

	return Status;
}

ML_NAMESPACE_END_(Io);

ML_NAMESPACE_BEGIN(Ob)

#if ML_KERNEL_MODE

NTSTATUS
CreateObjectType(
	IN  PUNICODE_STRING             TypeName,
	IN  POBJECT_TYPE_INITIALIZER    ObjectTypeInitializer,
	OUT POBJECT_TYPE*               ObjectType
)
{
	NTSTATUS Status;

	*ObjectType = nullptr;

	Status = ObCreateObjectType(TypeName, ObjectTypeInitializer, nullptr, ObjectType);
	if (Status != STATUS_OBJECT_NAME_COLLISION)
		return Status;

	BYTE ObjectHack[0x100];
	POBJECT_TYPE Type;
	PVOID Object;
	ML_OBJECT_HEADER_DYNAMIC ObjectHeader;

	ZeroMemory(ObjectHack, sizeof(ObjectHack));

	Object = &ObjectHack[countof(ObjectHack) / 2];
	ObjectHeader = Object;

	for (ULONG_PTR TypeIndex = ML_OBJECT_TYPE(ObGetObjectType(*TmResourceManagerObjectType))->Index; TypeIndex != 0x100; ++TypeIndex)
	{
		ObjectHeader->TypeIndex = TypeIndex;
		Type = ObGetObjectType(Object);
		if (Type == nullptr || (ULONG_PTR)Type == 1)
			continue;

		if (RtlEqualUnicodeString(&ML_OBJECT_TYPE(Type)->Name, TypeName, TRUE) == FALSE)
			continue;

		*ObjectType = Type;

		return STATUS_SUCCESS;
	}

	return Status;
}

#endif

ML_NAMESPACE_END_(Ob);

ML_NAMESPACE_END_(Native);

ML_NAMESPACE_END;    // ml


PIMAGE_RESOURCE_DATA_ENTRY
Nt_FindResource(
	PVOID   Module,
	PCWSTR  Name,
	PCWSTR  Type
)
{
	return ml::Native::Ldr::FindResource(Module, Name, Type);
}

PVOID
Nt_LoadResource(
	PVOID                       Module,
	PIMAGE_RESOURCE_DATA_ENTRY  ResourceDataEntry,
	PULONG                      Size
)
{
	return ml::Native::Ldr::LoadResource(Module, ResourceDataEntry, Size);
}

NTSTATUS
Nt_AllocateMemory(
	HANDLE      ProcessHandle,
	PVOID*      BaseAddress,
	ULONG_PTR   Size,
	ULONG       Protect,        /* = PAGE_EXECUTE_READWRITE */
	ULONG       AllocationType  /* = MEM_RESERVE | MEM_COMMIT */
)
{
	return ml::Native::Mm::AllocVirtualMemory(BaseAddress, Size, Protect, AllocationType, ProcessHandle);
}

NTSTATUS
Nt_FreeMemory(
	HANDLE  ProcessHandle,
	PVOID   BaseAddress
)
{
	return ml::Native::Mm::FreeVirtualMemory(BaseAddress, ProcessHandle);
}

#endif // CPP_DEFINED

ML_NAMESPACE

LONG_PTR MlInitialize()
{
	if (MemoryAllocator::GetGlobalHeap() == NULL && MemoryAllocator::CreateGlobalHeap() == nullptr)
		return STATUS_NO_MEMORY;

	return STATUS_SUCCESS;
}

LONG_PTR MlUnInitialize()
{
	MemoryAllocator::DestroyGlobalHeap();

	return 0;
}

MY_NAMESPACE_END

#endif

