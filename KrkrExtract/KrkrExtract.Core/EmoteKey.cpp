#include "KrkrExtract.h"
#include <capstone/capstone.h>

enum class EMOTEKEY : ULONG
{
	 EMOTE_EMOTEDRIVER = 0,
	 EMOTE_EMOTEPLAYER = 1
};

PVOID ProbeModuleBase(PCWSTR GameRoot, PCWSTR PluginName)
{
	WCHAR   FullPath[MAX_NTPATH];
	PVOID   Module;

	RtlZeroMemory(FullPath, sizeof(FullPath));
	Module = Nt_GetModuleHandle(PluginName);
	if (Module)
		return Module;
	
	FormatStringW(FullPath, L"%s%s", GameRoot, PluginName);
	Module = Nt_LoadLibrary(FullPath);
	if (Module)
		return Module;

	FormatStringW(FullPath, L"%splugin\\%s", GameRoot, PluginName);
	return Nt_LoadLibrary(FullPath);
}


BOOL ProboReadOnlyPage(PVOID Address)
{
	NTSTATUS                 Status;
	MEMORY_BASIC_INFORMATION Info;

	if (IsBadReadPtr(Address, 1))
		return FALSE;

	Status = NtQueryVirtualMemory(NtCurrentProcess(), Address, MemoryBasicInformation, &Info, sizeof(Info), nullptr);
	if (NT_FAILED(Status))
	return FALSE;

	return Info.Protect == PAGE_READONLY ? TRUE : FALSE;
}


BOOL ProboExecutablePage(PVOID Address)
{
	NTSTATUS                 Status;
	MEMORY_BASIC_INFORMATION Info;

	if (IsBadReadPtr(Address, 1))
		return FALSE;

	Status = NtQueryVirtualMemory(NtCurrentProcess(), Address, MemoryBasicInformation, &Info, sizeof(Info), nullptr);
	if (NT_FAILED(Status))
		return FALSE;

	return Info.Protect & PAGE_EXECUTE ? TRUE : FALSE;
}


//
//  who can write a tiny static analysis engine for me :(
//
//  This first push offset instruction within the first basic block
//
// .text:10002F33 8B 4D 10                                mov     ecx, [ebp+arg_8]
// .text:10002F36 8B 5D 08                                mov     ebx, [ebp + arg_0]
// .text:10002F39 68 50 33 07 10                          push    offset a742877301; "742877301"
//
// const StructCryptFilter::`vftable'
// 

NTSTATUS ParseEmoteDriver(PVOID Module, ULONG& PrivateKey)
{
	ULONG_PTR   OpSize, InstructionCount, Offset;
	PBYTE       ExportedFunc;
	ULONG_PTR   Address;


	if (Module == nullptr)
		return STATUS_INVALID_PARAMETER;

	InstructionCount = 0;
	PrivateKey = 0;

	ExportedFunc = (PBYTE)Nt_GetProcAddress(Module, "?EmoteFilterTexture@@YAXPAEKP6AX0K@Z@Z");
	if (!ExportedFunc)
		return STATUS_NOT_FOUND;

	Offset = 0;
	LOOP_FOREVER
	{
		if (InstructionCount >= 0x20)
			break;

		OpSize = GetOpCodeSize32((PBYTE)ExportedFunc + Offset);
		if (OpSize == 0)
			break;

		//
		// push offset
		//
		if (((PBYTE)ExportedFunc + Offset)[0] == 0x68)
		{
			if (ProboReadOnlyPage((PVOID)*(PULONG)(ExportedFunc + Offset + 1)))
			{
				Address = *(PULONG)(ExportedFunc + Offset + 1);
				PrivateKey = (ULONG)StringToInt32A((PCSTR)Address);
				return STATUS_SUCCESS;
			}
		}

		InstructionCount++;
		Offset += OpSize;
	}

	if (InstructionCount >= 0x20)
		return STATUS_BAD_DATA;

	return STATUS_UNSUCCESSFUL;
}

//
//
// #c#r#y#p#t#k#e#y#
// const StructCryptFilter::`vftable'
//
// .text : 10057A0F 68 5C 9A 12 10                                               push    offset a149203383 ; "149203383"
// .text : 10057A14 89 B5 78 FF FF FF                                            mov[ebp + var_88], esi
// .text : 10057A1A E8 4D 65 08 00                                               call    _atoi
// .text : 10057A1F C7 85 34 FF FF FF F0 D5 12 10                                mov[ebp + var_CC], offset ? ? _7StructCryptFilter@@6B@; const StructCryptFilter::`vftable'
// .text : 10057A29 C7 85 38 FF FF FF 15 CD 5B 07                                mov[ebp + var_C8], 75BCD15h
// .text : 10057A33 C7 85 3C FF FF FF E5 55 9A 15                                mov[ebp + var_C4], 159A55E5h
// .text : 10057A3D C7 85 40 FF FF FF B5 3B 12 1F                                mov[ebp + var_C0], 1F123BB5h
// .text : 10057A47 89 85 44 FF FF FF                                            mov[ebp + var_BC], eax
// .text : 10057A4D 89 B5 48 FF FF FF                                            mov[ebp + var_B8], esi
// .text : 10057A53 89 B5 4C FF FF FF                                            mov[ebp + var_B4], esi
//
//


NTSTATUS MatchEmotePlayerConstants(PBYTE Address, PBYTE EndAddress, BOOL& Found, ULONG_PTR FollowedThreshold)
{
	ULONG_PTR Offset, OpSize;
	ULONG     MatchedCount, InstructionCount;

	MatchedCount     = 0;
	InstructionCount = 0;
	Found            = FALSE;

	while (Address < EndAddress)
	{
		if (MatchedCount > FollowedThreshold)
			return STATUS_NOT_FOUND;

		OpSize = GetOpCodeSize32(Address);
		if (OpSize == 0)
			return STATUS_BAD_DATA;

		if (OpSize != 10)
			goto _next;

		if (Address[0] != 0xC7 || Address[1] != 0x85)
			goto _next;

		switch (*(PULONG)(Address + 6))
		{
		case 0x75BCD15:
		case 0x159A55E5:
		case 0x1F123BB5:
			MatchedCount++;
			break;
		}

	_next:
		InstructionCount++;
		Address += OpSize;
	}

	if (MatchedCount >= 3)
	{
		Found = TRUE;
		return STATUS_SUCCESS;
	}

	return STATUS_NOT_FOUND;
}


NTSTATUS ParseEmotePlayerInternal(PBYTE Address, SIZE_T Size, ULONG& PrivateKey)
{
	NTSTATUS  Status;
	ULONG_PTR Offset, OpSize;
	BOOL      Found;
	
	Offset = 0;
	while (Offset < Size)
	{
		OpSize = GetOpCodeSize32(Address + Offset);
		if (OpSize == 0)
			return STATUS_BAD_DATA;

		if (OpSize != 5)
			goto _next;

		if ((Address + Offset)[0] != 0x68)
			goto _next;

		if (!ProboReadOnlyPage((PVOID)*(PULONG)(Address + Offset + 1)))
			goto _next;

		Found = FALSE;
		Status = MatchEmotePlayerConstants(Address + Offset + OpSize, Address + Size, Found, 10);
		if (NT_FAILED(Status) || Found == FALSE)
			goto _next;

		PrivateKey = (ULONG)StringToInt32A((PCSTR)*(PULONG)(Address + Offset + 1));
		return STATUS_SUCCESS;
			

	_next:
		Offset += Offset;
	}

	return STATUS_NOT_FOUND;
}

NTSTATUS ParseEmotePlayer(PVOID Module, ULONG& PrivateKey)
{
	NTSTATUS             Status;
	IMAGE_SECTION_HEADER SectionTable[64];
	PIMAGE_DOS_HEADER    DosHeader;
	PIMAGE_NT_HEADERS32  NtHeader;

	DosHeader = (PIMAGE_DOS_HEADER)Module;
	NtHeader = (PIMAGE_NT_HEADERS32)((PBYTE)Module + DosHeader->e_lfanew);
	RtlZeroMemory(SectionTable, sizeof(SectionTable));
	RtlCopyMemory(SectionTable, (PBYTE)Module + sizeof(IMAGE_NT_HEADERS32) + DosHeader->e_lfanew,
		NtHeader->FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER));

	for (ULONG_PTR i = 0; i < NtHeader->FileHeader.NumberOfSections; i++)
	{
		if (!ProboExecutablePage((PBYTE)Module + SectionTable[i].VirtualAddress))
			continue;

		Status = ParseEmotePlayerInternal((PBYTE)Module + SectionTable[i].VirtualAddress, SectionTable[i].SizeOfRawData, PrivateKey);
		if (NT_SUCCESS(Status))
			return Status;
	}

	return STATUS_NOT_FOUND;
}



NTSTATUS NTAPI KrkrExtractCore::GetEmotePrivateKey(PULONG EmoteKey)
{
	NTSTATUS Status;
	

	auto CheckEmoteKey = [](KrkrClientProxyer* Proxyer, PULONG EmoteKey, PCWSTR WorkerDir, EMOTEKEY EmoteKind)->NTSTATUS
	{
		NTSTATUS Status;
		PVOID    Module;
		
		Module = nullptr;
		switch (EmoteKind)
		{
		case EMOTEKEY::EMOTE_EMOTEDRIVER:
			Module = ProbeModuleBase(WorkerDir, L"emotedriver.dll");
			break;

		case EMOTEKEY::EMOTE_EMOTEPLAYER:
			Module = ProbeModuleBase(WorkerDir, L"emoteplayer.dll");
			break;

		default:
			Proxyer->TellServerLogOutput(
				LogLevel::LOG_ERROR,
				L"Unknown emote kind : %d",
				(ULONG)EmoteKind
			);
			break;
		}

		if (!Module) {
			Proxyer->TellServerLogOutput(
				LogLevel::LOG_ERROR,
				L"no module available"
			);

			return STATUS_NOT_FOUND;
		}

		Status = STATUS_UNSUCCESSFUL;
		switch (EmoteKind)
		{
		case EMOTEKEY::EMOTE_EMOTEDRIVER:
			Status = ParseEmoteDriver(Module, *EmoteKey);
			break;

		case EMOTEKEY::EMOTE_EMOTEPLAYER:
			Status = ParseEmotePlayer(Module, *EmoteKey);
			break;
		}
		
		if (NT_FAILED(Status))
		{
			switch (Status)
			{
			case STATUS_NOT_FOUND:
				Proxyer->TellServerLogOutput(
					LogLevel::LOG_ERROR,
					L"Signature not available"
				);
				break;

			case STATUS_BAD_DATA:
				Proxyer->TellServerLogOutput(
					LogLevel::LOG_ERROR,
					L"No module file available"
				);
				break;

			default:
				Proxyer->TellServerLogOutput(
					LogLevel::LOG_ERROR,
					L"Unable to parse emote private key"
				);
				break;
			}
		}

		return Status;
	};

	if (m_EmoteKeyInitialized)
	{
		if (EmoteKey) {
			*EmoteKey = m_EmotePrivateKey;
		}
		return STATUS_SUCCESS;
	}

	Status = CheckEmoteKey(this, &m_EmotePrivateKey, m_WorkerDir.c_str(), EMOTEKEY::EMOTE_EMOTEDRIVER);
	if (NT_SUCCESS(Status))
		return Status;

	Status = CheckEmoteKey(this, &m_EmotePrivateKey, m_WorkerDir.c_str(), EMOTEKEY::EMOTE_EMOTEPLAYER);
	if (NT_SUCCESS(Status))
		return Status;

	return STATUS_UNSUCCESSFUL;
}

