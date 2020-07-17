#include "KrkrUniversalPatch.h"
#include "tp_stub.h"
#include "KrkrExtract.h"
#include "PEFile.h"

//none thread base
NTSTATUS NTAPI MakePatch()
{
	NTSTATUS       Status;
	FuncHostAlloc  Allocater;
	ULONG_PTR      IStreamAdapterVtable;
	GlobalData*    Handle;
	NtFileDisk     ExeFile;
	WCHAR          ExeFileName[MAX_PATH];

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

	Nt_GetModuleFileName(Nt_GetExeModuleHandle(), ExeFileName, countof(ExeFileName));
	
	LOOP_ONCE
	{
		if (Nt_GetFileAttributes(ExeFileName) == 0xFFFFFFFF)
		{
			MessageBoxW(Handle->MainWindow, L"KrkrExtract couldn't open current executable file!", L"KrkrExtract", MB_OK);
			Status = STATUS_UNSUCCESSFUL;
			break;
		}

		CHAR* Functions[] = { "XmoeLinker" };

		PEFile PeReader(ExeFileName);
		PeReader.addImport("KrkrUniversalPatch.dll", Functions, 1);
		if (!PeReader.saveToFile(GetNameFileName(ExeFileName).c_str()))
		{
			MessageBoxW(Handle->MainWindow, L"KrkrExtract couldn't added import information to the executable file!", L"KrkrExtract", MB_OK);
			Status = STATUS_UNSUCCESSFUL;
			break;
		}

		MessageBoxW(Handle->MainWindow, L"Universal patch : Successful", L"KrkrExtract", MB_OK);
	}

	return Status;
}
