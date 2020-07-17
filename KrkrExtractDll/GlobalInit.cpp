#include "GlobalInit.h"
#include "Console.h"


HRESULT WINAPI InitExporter(iTVPFunctionExporter *exporter)
{
	bool result = false;
	result = TVPInitImportStub(exporter);
	if (result)
	{
		GlobalData::GetGlobalData()->InitedByModule = InitedByDllModule;
	}
	return result ? S_OK : S_FALSE;
}

typedef iTVPFunctionExporter*   (__stdcall *TVPGetFunctionExporterFunc)();
HRESULT WINAPI InitExporterByExeModule()
{
	HMODULE hModule = GetModuleHandleW(NULL);
	
	FARPROC pTVPMainForm = GetProcAddress(hModule, "_TVPMainForm");
	TVPGetFunctionExporterFunc pfTVPGetFunctionExporter = (TVPGetFunctionExporterFunc)GetProcAddress(hModule, "TVPGetFunctionExporter");

	if (pfTVPGetFunctionExporter)
	{
		//Just Hook It, Do Not Call it!
		iTVPFunctionExporter* e = pfTVPGetFunctionExporter();
		if (e)
		{
			WCHAR Info[260] = { 0 };
			wsprintfW(Info, L"[exporter:exe]0x%08x\n", (DWORD)e);
			OutputString(Info);

			HRESULT result = InitExporter(e);
			
			if (result == S_OK)
			{
				GlobalData::GetGlobalData()->InitedByModule = InitedByExeModule;
				return S_OK;
			}
			else
			{
				return S_FALSE;
			}
		}
	}
	return S_FALSE;
}


//============================================

GlobalData* GlobalData::Handle = nullptr;

//============================================
//GlobalData

GlobalData::GlobalData() :
	PngFlag(PNG_RAW),
	TlgFlag(TLG_RAW),
	TextFlag(TEXT_RAW),
	PsbFlag(PSB_RAW),
	hImageModule(nullptr),
	InitedByModule(InitedByDllModule),
	hSelfModule(nullptr), //At DllMain
	hHostModule(nullptr),
	pfGlobalXP3Filter(nullptr),
	TVPFunctionExporter(nullptr),
	HasConsole(FALSE),
	isRunning(FALSE)
{
	hHostModule = GetModuleHandleW(nullptr);
}

GlobalData* GlobalData::GetGlobalData(HMODULE hSelf)
{
	if (Handle == nullptr)
	{
		Handle = new GlobalData;
		Handle->hSelfModule = hSelf; //ÀÁµÃCheckPEÁË

		RtlZeroMemory(Handle->SelfPath, MAX_PATH * 2);
		GetModuleFileNameW(hSelf, Handle->SelfPath, MAX_PATH);
	}
	return Handle;
}

ULONG GlobalData::SetTextFlag(ULONG Flag)
{
	ULONG OldFlag = Flag;
	this->TextFlag = Flag;
	return OldFlag;
}

ULONG GlobalData::GetTextFlag()
{
	return this->TextFlag;
}

ULONG GlobalData::SetTlgFlag(ULONG Flag)
{
	ULONG OldFlag = this->TlgFlag;
	this->TlgFlag = Flag;
	return OldFlag;
}

ULONG GlobalData::GetTlgFlag()
{
	return this->TlgFlag;
}


ULONG GlobalData::SetPngFlag(ULONG Flag)
{
	ULONG OldFlag = Flag;
	this->PngFlag = Flag;
	return OldFlag;
}

ULONG GlobalData::GetPngFlag()
{
	return this->PngFlag;
}

ULONG GlobalData::AddPsbFlag(ULONG Flag)
{
	ULONG OldFlag = this->PsbFlag;
	SET_FLAG(this->PsbFlag, Flag);
	return OldFlag;
}

ULONG GlobalData::DeletePsbFlag(ULONG Flag)
{
	ULONG OldFlag = this->PsbFlag;
	CLEAR_FLAG(this->PsbFlag, Flag);
	return OldFlag;
}

BOOL GlobalData::PsbFlagOn(ULONG Flag)
{
	if (Flag == PSB_ALL)
	{
		return FLAG_ON(this->PsbFlag, PSB_RAW) &&
			FLAG_ON(this->PsbFlag, PSB_TEXT) &&
			FLAG_ON(this->PsbFlag, PSB_DECOM) &&
			FLAG_ON(this->PsbFlag, PSB_IMAGE) &&
			FLAG_ON(this->PsbFlag, PSB_ANM);
	}
	else
	{
		return FLAG_ON(this->PsbFlag, Flag);
	}
}
;
VOID GlobalData::DebugPsbFlag()
{
	wstring Info;
	Info += L"Raw:";
	Info += FLAG_ON(PsbFlag, PSB_RAW) ? L"true " : L"false ";
	Info += L"Text:";
	Info += FLAG_ON(PsbFlag, PSB_TEXT) ? L"true " : L"false ";
	Info += L"Decom:";
	Info += FLAG_ON(PsbFlag, PSB_DECOM) ? L"true " : L"false ";
	Info += L"Image:";
	Info += FLAG_ON(PsbFlag, PSB_IMAGE) ? L"true " : L"false ";
	Info += L"Anm:";
	Info += FLAG_ON(PsbFlag, PSB_ANM) ? L"true " : L"false ";
	Info += L"\n";
	OutputString(Info.c_str(), TRUE);
}

void GlobalData::SetFolder(wstring& Name)
{
	this->Folder = Name;
}

void GlobalData::SetGuessPack(wstring& Name)
{
	this->GuessPack = Name;
}

void GlobalData::SetOutputPack(wstring& Name)
{
	this->OutPack = Name;
}

wstring GlobalData::GetFolder()
{
	return this->Folder;
}

wstring GlobalData::GetGuessPack()
{
	return this->GuessPack;
}

wstring GlobalData::GetOutputPack()
{
	return this->OutPack;
}


bool GlobalData::FindCodeSlow(const PCHAR start, ULONG size, const PCHAR Pattern, ULONG PatternLen)
{

	size_t Strlen = PatternLen;
	register DWORD iPos = 0;
	register DWORD zPos = 0;
	bool Found = false;
	while (iPos < size)
	{
		if (zPos == Strlen - 1)
		{
			Found = true;
			break;
		}
		if (start[iPos] == Pattern[zPos])
		{
			iPos++;
			zPos++;
		}
		else
		{
			iPos++;
			zPos = 0;
		}
	}
	if (Found)
	{
		return true;
	}
	return false;
}


ULONG GlobalData::GetFileLen(LPVOID pBaseaddr, LPVOID pReadBuf)
{
	LPBYTE pBase = (LPBYTE)pBaseaddr;
	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)pReadBuf;
	ULONG uSize = PIMAGE_OPTIONAL_HEADER((pBase + pDosHeader->e_lfanew + 4 + 20))->SizeOfHeaders;
	PIMAGE_SECTION_HEADER    pSec = (PIMAGE_SECTION_HEADER)(pBase + pDosHeader->e_lfanew + sizeof(IMAGE_NT_HEADERS));
	for (int i = 0; i<PIMAGE_FILE_HEADER(pBase + pDosHeader->e_lfanew + 4)->NumberOfSections; ++i)
	{
		uSize += pSec[i].SizeOfRawData;
	}
	return uSize;
}


WCHAR* GlobalData::GetCompiledDate()
{
	if (hSelfModule == nullptr)
	{
		return nullptr;
	}
	PIMAGE_DOS_HEADER pDosHeader;
	pDosHeader = (PIMAGE_DOS_HEADER)hSelfModule;
	PIMAGE_NT_HEADERS32 pNtHeader = (PIMAGE_NT_HEADERS32)((DWORD)pDosHeader + pDosHeader->e_lfanew);
	return _wctime((time_t*)&pNtHeader->FileHeader.TimeDateStamp);
}

