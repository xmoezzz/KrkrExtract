#include "KrkrUniversalPatch.h"
#include "tp_stub.h"
#include "KrkrExtract.h"
#include "PEFile.h"
#include "resource.h"

NTSTATUS FASTCALL CopyExeIcon(PCWCHAR To, ULONG_PTR Check)
{
	HMODULE    hExe;
	HRSRC      hRes;
	HANDLE     hUpdateRes;

	hExe       = (HMODULE)Nt_GetExeModuleHandle();
	hUpdateRes = BeginUpdateResourceW(To, FALSE);
	if (hUpdateRes == NULL)
		return STATUS_UNSUCCESSFUL;


	for (ULONG i = 0; i<Check; i++)
	{
		hRes = FindResource(hExe, MAKEINTRESOURCEW(i), RT_GROUP_ICON);
		if (hRes != NULL)
		{
			if (!UpdateResourceW(hUpdateRes, RT_GROUP_ICON, MAKEINTRESOURCEW(i), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), LockResource(LoadResource(hExe, hRes)), SizeofResource(hExe, hRes))
				)
				break;
		}
	}
	hRes = NULL;


	for (ULONG i = 0; i<Check; i++)
	{
		if (FindResource(hExe, MAKEINTRESOURCEW(i), RT_ICON) != NULL)
		{
			i--;
			for (ULONG rCount = i; rCount<(i + 32); rCount++)
			{
				hRes = FindResource(hExe, MAKEINTRESOURCEW(rCount), RT_ICON);
				if (hRes != NULL)
				{
					UpdateResourceW(hUpdateRes, RT_ICON, MAKEINTRESOURCEW(rCount), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), LockResource(LoadResource(hExe, hRes)), SizeofResource(hExe, hRes));
				}
			}
			break;
		}

	}
	hRes = NULL;

	for (ULONG i = 0; i<Check; i++){

		hRes = FindResource(hExe, MAKEINTRESOURCEW(i), RT_VERSION);
		if (hRes != NULL)
		{
			if (!UpdateResourceW(hUpdateRes, RT_VERSION, MAKEINTRESOURCEW(i), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), LockResource(LoadResource(hExe, hRes)), SizeofResource(hExe, hRes))
				)
				break;
		}
	}

	EndUpdateResourceW(hUpdateRes, FALSE);
	return STATUS_SUCCESS;
}


EXTC ULONG
InternalCopyUnicodeString(
PUNICODE_STRING Unicode,
PWCHAR          Buffer,
ULONG_PTR       BufferCount,
BOOL            IsDirectory = FALSE
);



IStream* LoadFromResource(HMODULE Module, UINT nResID, LPCWSTR lpTyp, BOOL Inject, HWND Handle)
{
	WCHAR          ExeFileBaseName[MAX_PATH];


	auto Nt_GetModuleFileBaseName = [](PVOID ModuleBase, LPWSTR Filename, ULONG_PTR BufferCount)->ULONG_PTR
	{
		ULONG_PTR               Length;
		PEB_BASE               *Peb;
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
	};

	HRSRC hRsrc = ::FindResourceW(Module, MAKEINTRESOURCE(nResID), lpTyp);
	if (hRsrc == NULL)
		return NULL;

	HGLOBAL hImgData = ::LoadResource(Module, hRsrc);
	if (hImgData == NULL)
	{
		::FreeResource(hImgData);
		return NULL;
	}

	LPVOID lpVoid = ::LockResource(hImgData);

	LPSTREAM pStream = NULL;
	DWORD dwSize = ::SizeofResource(Module, hRsrc);
	HGLOBAL hNew = ::GlobalAlloc(GHND, dwSize);
	LPBYTE lpByte = (LPBYTE)::GlobalLock(hNew);
	RtlCopyMemory(lpByte, lpVoid, dwSize);


	if (Inject)
	{
		static CHAR KeyWord[] = "KrkrInfoXmoeAnzu";

		PVOID ShellData;

		ShellData = KMP(lpByte, dwSize, KeyWord, CONST_STRLEN(KeyWord));
		if (ShellData == NULL)
		{
			MessageBoxW(Handle, L"Universal patch : Internal Error", L"KrkrExtract", MB_OK);
			return NULL;
		}

		Nt_GetModuleFileBaseName(Nt_GetExeModuleHandle(), ExeFileBaseName, countof(ExeFileBaseName));

		RtlZeroMemory(ShellData, 0x10);
		RtlCopyMemory((PBYTE)ShellData + 0x10, ExeFileBaseName, StrLengthW(ExeFileBaseName) * sizeof(ExeFileBaseName[0]));

	}

	::GlobalUnlock(hNew);

	if (FAILED(CreateStreamOnHGlobal(hNew, TRUE, &pStream)))
	{
		GlobalFree(hNew);
		return NULL;
	}
	::FreeResource(hImgData);

	return pStream;
}


//none thread base
NTSTATUS NTAPI MakePatch()
{
	NTSTATUS       Status;
	FuncHostAlloc  Allocater;
	ULONG_PTR      IStreamAdapterVtable;
	GlobalData*    Handle;
	NtFileDisk     File;
	WCHAR          ExeFileName[MAX_PATH];
	PBYTE          Buffer;
	ULONG          Size;
	IStream*       Stream;
	STATSTG        Stat;
	ULONG          BytesRead;

	Handle = GlobalData::GetGlobalData();

	auto GetTVPCreateStreamCall = [&]()->PVOID
	{
		PVOID CallIStreamStub, CallIStream, CallTVPCreateStreamCall;
		ULONG OpSize, OpOffset;
		WORD  WordOpcode;

		static char funcname[] = "IStream * ::TVPCreateIStream(const ttstr &,tjs_uint32)";

		LOOP_ONCE
		{
			CallTVPCreateStreamCall = NULL;

			CallIStreamStub = TVPGetImportFuncPtr(funcname);
			if (!CallIStreamStub)
				break;

			CallIStream = NULL;
			OpOffset = 0;

			LOOP_FOREVER
			{
				if (((PBYTE)CallIStreamStub + OpOffset)[0] == 0xCC)
				break;

				WordOpcode = *(PWORD)((ULONG_PTR)CallIStreamStub + OpOffset);
				//mov edx,dword ptr [ebp+0xC]
				if (WordOpcode == 0x558B)
				{
					OpOffset += 2;
					if (((PBYTE)CallIStreamStub + OpOffset)[0] == 0xC)
					{
						OpOffset++;
						WordOpcode = *(PWORD)((ULONG_PTR)CallIStreamStub + OpOffset);
						//mov edx,dword ptr [ebp+0x8]
						if (WordOpcode == 0x4D8B)
						{
							OpOffset += 2;
							if (((PBYTE)CallIStreamStub + OpOffset)[0] == 0x8)
							{
								OpOffset++;
								if (((PBYTE)CallIStreamStub + OpOffset)[0] == CALL)
								{
									CallIStream = (PVOID)GetCallDestination(((ULONG_PTR)CallIStreamStub + OpOffset));
									OpOffset += 5;
									break;
								}
							}
						}
					}
				}
				//the next opcode
				OpSize = GetOpCodeSize32(((PBYTE)CallIStreamStub + OpOffset));
				OpOffset += OpSize;
			}

			if (!CallIStream)
				break;

			OpOffset = 0;
			LOOP_FOREVER
			{
				if (((PBYTE)CallIStream + OpOffset)[0] == 0xC3)
				break;

				//find the first call
				if (((PBYTE)CallIStream + OpOffset)[0] == CALL)
				{
					CallTVPCreateStreamCall = (PVOID)GetCallDestination(((ULONG_PTR)CallIStream + OpOffset));
					OpOffset += 5;
					break;
				}

				//the next opcode
				OpSize = GetOpCodeSize32(((PBYTE)CallIStream + OpOffset));
				OpOffset += OpSize;
			}

			LOOP_FOREVER
			{
				if (((PBYTE)CallIStream + OpOffset)[0] == 0xC3)
				break;

				if (((PBYTE)CallIStream + OpOffset)[0] == CALL)
				{
					//push 0xC
					//call HostAlloc
					//add esp, 0x4
					if (((PBYTE)CallIStream + OpOffset - 2)[0] == 0x6A &&
						((PBYTE)CallIStream + OpOffset - 2)[1] == 0x0C)
					{
						Allocater = (FuncHostAlloc)GetCallDestination(((ULONG_PTR)CallIStream + OpOffset));
						OpOffset += 5;
					}
					break;
				}

				//the next opcode
				OpSize = GetOpCodeSize32(((PBYTE)CallIStream + OpOffset));
				OpOffset += OpSize;
			}

			LOOP_FOREVER
			{
				if (((PBYTE)CallIStream + OpOffset)[0] == 0xC3)
				break;

				//mov eax, mem.offset
				if (((PBYTE)CallIStream + OpOffset)[0] == 0xC7 &&
					((PBYTE)CallIStream + OpOffset)[1] == 0x00)
				{
					OpOffset += 2;
					IStreamAdapterVtable = *(PULONG_PTR)((PBYTE)CallIStream + OpOffset);
					OpOffset += 4;
					break;
				}

				//the next opcode
				OpSize = GetOpCodeSize32(((PBYTE)CallIStream + OpOffset));
				OpOffset += OpSize;
			}
		}


		//Find virtual table offset
		//IStreamAdapter

		if (Allocater && IStreamAdapterVtable)
		{
			return CallTVPCreateStreamCall;
		}
		else
		{
			return NULL;
		}
	};

	if (Handle->ModuleType != ModuleVersion::Krkrz)
	{
		MessageBoxW(Handle->MainWindow, L"Sorry, Universal patch only support krkrz currently!",L"KrkrExtract", MB_OK);
		return STATUS_INVALID_SIGNATURE;
	}

	if (GetTVPCreateStreamCall() == NULL)
	{
		MessageBoxW(Handle->MainWindow, L"KrkrExtract couldn't analyze this game!", L"KrkrExtract", MB_OK);
		return STATUS_UNSUCCESSFUL;
	}

	auto GetNameFileName = [](LPCWSTR FileName)->wstring
	{
		wstring PeFileName(FileName);
		auto Index = PeFileName.find_last_of(L'.');
		if (Index == std::wstring::npos)
			return PeFileName + L"_Patch";
		else
			return PeFileName.substr(0, Index) + L"_Patch.exe";
	};

	RtlZeroMemory(ExeFileName, countof(ExeFileName));
	GetModuleFileNameW((HMODULE)Nt_GetExeModuleHandle(), ExeFileName, countof(ExeFileName));
	
	LOOP_ONCE
	{
		if (Nt_GetFileAttributes(ExeFileName) == 0xFFFFFFFF)
		{
			MessageBoxW(Handle->MainWindow, L"KrkrExtract couldn't open current executable file!", L"KrkrExtract", MB_OK);
			Status = STATUS_UNSUCCESSFUL;
			break;
		}

		Stream = LoadFromResource(Handle->hSelfModule, IDR_EXE1, L"EXE", TRUE, Handle->MainWindow);
		if (!Stream)
		{
			Status = STATUS_UNSUCCESSFUL;
			break;
		}

		Stream->Stat(&Stat, STATFLAG_DEFAULT);
		Size = Stat.cbSize.LowPart;
		Buffer = (PBYTE)AllocateMemoryP(Size);
		Stream->Read(Buffer, Size, &BytesRead);
		Status = File.Create(GetNameFileName(ExeFileName).c_str());
		if (NT_FAILED(Status))
		{
			MessageBoxW(Handle->MainWindow, L"Universal patch :Couldn't write file", L"KrkrExtract", MB_OK);
			FreeMemoryP(Buffer);
			break;
		}

		File.Write(Buffer, Size);
		File.Close();
		Stream->Release();


		Stream = LoadFromResource(Handle->hSelfModule, IDR_DLL1, L"DLL", FALSE, Handle->MainWindow);
		if (!Stream)
		{
			Stream->Release();
			Status = STATUS_UNSUCCESSFUL;
			break;
		}

		Status = File.Create(L"KrkrUniversalPatch.dll");
		if (NT_FAILED(Status))
		{
			MessageBoxW(Handle->MainWindow, L"Universal patch : Couldn't write file", L"SiglusExtract", MB_OK | MB_ICONERROR);
			Stream->Release();
			Status = STATUS_UNSUCCESSFUL;
			break;
		}

		Stream->Stat(&Stat, STATFLAG_DEFAULT);
		Size = Stat.cbSize.LowPart;
		Buffer = (PBYTE)ReAllocateMemoryP(Buffer, Size);
		Stream->Read(Buffer, Size, &BytesRead);
		File.Write(Buffer, Size);
		File.Close();
		Stream->Release();
		FreeMemoryP(Buffer);

		if (Handle->InheritIcon)
			Status = CopyExeIcon(GetNameFileName(ExeFileName).c_str(), 512);

		if (NT_FAILED(Status))
			MessageBoxW(Handle->MainWindow, L"Universal patch : Successful\nBut KrkrExtract couldn't apply icon to patch.", L"KrkrExtract", MB_OK | MB_ICONWARNING);
		else
			MessageBoxW(Handle->MainWindow, L"Universal patch : Successful", L"KrkrExtract", MB_OK);

		Status = STATUS_SUCCESS;
	}

	return Status;
}
