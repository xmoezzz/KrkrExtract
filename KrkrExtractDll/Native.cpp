#include "Native.h"

NTSTATUS
WriteMemory(
HANDLE      ProcessHandle,
PVOID       BaseAddress,
PVOID       Buffer,
ULONG_PTR   Size,
PULONG_PTR  BytesWritten  = NULL
)
{
	return NtWriteVirtualMemory(ProcessHandle, BaseAddress, Buffer, Size, BytesWritten);
}


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
ReadMemory(
HANDLE      ProcessHandle,
PVOID       BaseAddress,
PVOID       Buffer,
ULONG_PTR   Size,
PULONG_PTR  BytesRead = NULL
)
{
	return NtReadVirtualMemory(ProcessHandle, BaseAddress, Buffer, Size, BytesRead);
}

NTSTATUS
AllocVirtualMemoryEx(
HANDLE      ProcessHandle,
PVOID*      BaseAddress,
ULONG_PTR   Size,
ULONG       Protect = PAGE_EXECUTE_READWRITE,
ULONG       AllocationType = MEM_RESERVE | MEM_COMMIT
)
{
	return AllocVirtualMemory(BaseAddress, Size, Protect, AllocationType, ProcessHandle);
}

NTSTATUS
FreeVirtualMemory(
PVOID   BaseAddress,
HANDLE  ProcessHandle,
ULONG   FreeType = MEM_RELEASE
)
{
	SIZE_T Size = 0;

	return NtFreeVirtualMemory(ProcessHandle, &BaseAddress, &Size, MEM_RELEASE);
}

FORCEINLINE
VOID
RtlInitEmptyUnicodeString(
OUT PUNICODE_STRING UnicodeString,
IN  PCWSTR          Buffer,
IN  ULONG_PTR       BufferSize
)
{
	UnicodeString->Length = 0;
	UnicodeString->MaximumLength = (USHORT)BufferSize;
	UnicodeString->Buffer = (PWSTR)Buffer;
}

BOOL ValidateDataDirectory(PIMAGE_DATA_DIRECTORY DataDirectory, ULONG_PTR SizeOfImage)
{
	if (DataDirectory->Size == 0)
		return FALSE;

	if ((ULONG64)DataDirectory->VirtualAddress + DataDirectory->Size > ULONG_MAX)
		return FALSE;

	return DataDirectory->VirtualAddress <= SizeOfImage && DataDirectory->VirtualAddress + DataDirectory->Size <= SizeOfImage;
}

enum
{
	IMAGE_VALID_EXPORT_ADDRESS_TABLE = 0x00000001,
	IMAGE_VALID_IMPORT_ADDRESS_TABLE = 0x00000002,
	IMAGE_VALID_RESOURCE = 0x00000004,
	IMAGE_VALID_RELOC = 0x00000008,
};

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
		lea     ecx, [ecx + 14];
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
		mov     ecx, [esi];
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
		Status = AllocVirtualMemoryEx(ProcessHandle, &Buffer, MEMORY_PAGE_SIZE);
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
				DllFullPath->Buffer,
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
			FreeVirtualMemory(Buffer, ProcessHandle);

	return Status;
}
