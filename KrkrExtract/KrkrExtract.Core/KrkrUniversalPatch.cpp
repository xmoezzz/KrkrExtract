#include "CoreTask.h"
#include "tp_stub.h"
#include "PEFile.h"
#include "Helper.h"
#include "resource.h"
#include "TaskAutoUnlocker.h"

CoUniversalPatchTask::CoUniversalPatchTask(KrkrClientProxyer* Proxyer, BOOL ApplyIcon, BOOL ApplyProtection) :
	m_Protection(ApplyProtection),
	m_CopyIcon(ApplyIcon)
{
	m_Proxyer = Proxyer;
}

CoUniversalPatchTask::~CoUniversalPatchTask()
{
	SendKill();
	Ps::Sleep(500);
	Stop();

	m_Proxyer = nullptr;
}



ULONG_PTR Nt_GetModuleFileBaseName(PVOID ModuleBase, LPWSTR Filename, ULONG_PTR BufferCount)
{
	ULONG_PTR               Length;
	PEB                    *Peb;
	PLDR_DATA_TABLE_ENTRY   LdrModule, FirstLdrModule;

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

	Length = LdrModule->BaseDllName.Length;
	RtlCopyMemory(Filename, LdrModule->BaseDllName.Buffer, (LdrModule->BaseDllName.Length + 1) * 2);

	return Length;
}


INT CoUniversalPatchTaskThreadEpFilter(ULONG ExceptionCode, EXCEPTION_POINTERS* Ep)
{
	switch (ExceptionCode)
	{
	case EXCEPTION_ACCESS_VIOLATION:
		PrintConsoleW(L"CoUniversalPatchTaskThread : EXCEPTION_ACCESS_VIOLATION\n");
		PrintConsoleW(L"Eip : %p, Address : %p\n", Ep->ContextRecord->Eip, Ep->ExceptionRecord->ExceptionAddress);
		return EXCEPTION_EXECUTE_HANDLER;
	}

	return EXCEPTION_CONTINUE_SEARCH;
}

template<class CallbackFunc>
NTSTATUS
WriteOrPatchResource(
	_In_ CoUniversalPatchTask* Task,
	_In_ HMODULE      Module,
	_In_ INT          ResourceId,
	_In_ PCWSTR       TypeName,
	_In_ wstring&     OutputName,
	_In_ CallbackFunc Callback
)
{
	NTSTATUS      Status;
	HRESULT       HResult;
	STATSTG       Stat;
	ULONG         Size;
	IStream*      Stream;
	NtFileDisk    File;

	Stream = LoadFromResource(
		Module,
		ResourceId,
		TypeName,
		Callback
	);

	if (!Stream)
	{
		Task->SetLastMessage(FALSE, L"KrkrExtract couldn't open current executable file!");
		return STATUS_NOT_FOUND;
	}

	LOOP_ONCE
	{
		HResult = Stream->Stat(&Stat, STATFLAG_DEFAULT);
		Status  = FAILED(HResult) ? STATUS_UNSUCCESSFUL : STATUS_SUCCESS;
		if (FAILED(HResult))
			break;

		Size = Stat.cbSize.LowPart;
		auto Buffer = AllocateMemorySafeP<BYTE>(Size);
		if (!Buffer) {
			Status = STATUS_NO_MEMORY;
			break;
		}

		HResult = Stream->Read(Buffer.get(), Size, nullptr);
		Status  = FAILED(HResult) ? STATUS_UNSUCCESSFUL : STATUS_SUCCESS;
		if (FAILED(HResult))
			break;

		Status = File.Create(OutputName.c_str());
		if (NT_FAILED(Status)) {
			Task->SetLastMessage(FALSE, L"Universal patch :Couldn't write file");
			break;
		}

		File.Write(Buffer.get(), Size);
		File.Close();
		Stream->Release();
	}

	return Status;
}


void CoUniversalPatchTask::ThreadFunction()
{
	NTSTATUS              Status;
	NtFileDisk            File;
	ULONG                 Size;
	IStream*              Stream;
	STATSTG               Stat;
	ULONG                 BytesRead;
	TaskAutoUnlocker      Unlocker(this, m_Proxyer);

	if (m_Proxyer->GetTVPCreateBStream() == NULL)  {
		SetLastMessage(FALSE, L"Unknown module type");
		return;
	}

	if (Nt_GetFileAttributes(m_Proxyer->GetModulePath()) == 0xFFFFFFFF)
	{
		SetLastMessage(FALSE, L"KrkrExtract couldn't open current executable file!");
		return;
	}

	Status = WriteOrPatchResource(
		this,
		m_Proxyer->GetHostModule(),
		IDR_EXE1,
		L"EXE",
		ReplaceFileNameExtension(std::wstring(m_Proxyer->GetModulePath()), L"_patch.exe"),
		WALK_RESOURCE_BUFFERM(Buffer, Size)
		{
			WCHAR  ExeFileBaseName[MAX_PATH];
			PVOID  PtrToLoaderStruct;
			static CHAR KeyWord[] = "KrkrInfoXmoeAnzu";

			SEH_TRY
			{
				LOOP_ONCE
				{
					PtrToLoaderStruct = KMP(Buffer, Size, KeyWord, CONST_STRLEN(KeyWord));
					if (!PtrToLoaderStruct)
						break;

					Nt_GetModuleFileBaseName(GetModuleHandleW(NULL), ExeFileBaseName, countof(ExeFileBaseName));

					RtlZeroMemory(PtrToLoaderStruct, 0x10);
					RtlZeroMemory(ExeFileBaseName, sizeof(ExeFileBaseName));
					RtlCopyMemory((PBYTE)PtrToLoaderStruct + 0x10, ExeFileBaseName, lstrlenW(ExeFileBaseName) * sizeof(ExeFileBaseName[0]));
				}

				if (!PtrToLoaderStruct) {
					m_Proxyer->TellServerLogOutput(LogLevel::LOG_ERROR, L"Universal patch : Couldn't find signature.");
					return STATUS_NOT_FOUND;
				}
			}
			SEH_EXCEPT(CoUniversalPatchTaskThreadEpFilter(GetExceptionCode(), GetExceptionInformation()))
			{
				m_Proxyer->TellServerLogOutput(LogLevel::LOG_ERROR, L"!! Exception !!");
				return STATUS_UNSUCCESSFUL;
			}
			return STATUS_SUCCESS;
		}
	);

	if (NT_FAILED(Status))
	{
		SetLastMessage(FALSE, L"Failed to write exe");
		return;
	}

	Status = WriteOrPatchResource(
		this,
		m_Proxyer->GetHostModule(),
		IDR_DLL1, 
		L"DLL",
		wstring(L"KrkrUniversalPatch.dll"),
		WALK_RESOURCE_BUFFERM(Buffer, Size)
		{
			//TODO...
			return STATUS_SUCCESS;
		}
	);

	if (NT_FAILED(Status))
	{
		SetLastMessage(FALSE, L"Failed to write dll");
		return;
	}

	SetLastMessage(TRUE, L"Universal patch : Successful");

	LOOP_ONCE
	{
		if (!IsCopyIcon())
			break;

		Status = CopyExeIcon(ReplaceFileNameExtension(std::wstring(m_Proxyer->GetModulePath()), L"_patch.exe").c_str(), 512);
		if (NT_SUCCESS(Status))
			break;

		SetLastMessage(TRUE, L"Universal patch : Successful.\nBut KrkrExtract couldn't copy icon to patch.");
	}
}




