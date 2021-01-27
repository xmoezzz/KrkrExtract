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
	switch ((NTSTATUS)ExceptionCode)
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
	_In_ HMODULE          Module,
	_In_ INT              ResourceId,
	_In_ PCWSTR           TypeName,
	_In_ const wstring&   OutputName,
	_In_ CallbackFunc     Callback
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



#define P2ALIGNDOWN(x, align) ((x) & -(align))
// align x up to the nearest multiple of align. align must be a power of 2.
#define P2ALIGNUP(x, align) (-(-(x) & -(align)))


BOOL CompilePayload32(PBYTE Payload, ULONG OldOepOffset, ULONG DllNameBase)
{
	std::vector<BYTE> ShellCode;

#define SIN(...) ShellCode.insert(ShellCode.end(), {__VA_ARGS__})
#define SIN2(x) ShellCode.insert(ShellCode.end(), x.begin(), x.end())

	auto MovEax_XX = [](DWORD XX)->std::vector<BYTE>
	{
		std::vector<BYTE> Inst;
		union
		{
			BYTE  Bytes[4];
			DWORD Dword;
		};

		Inst.push_back(0xB8);
		Dword = XX;

		for (ULONG i = 0; i < _countof(Bytes); i++) {
			Inst.push_back(Bytes[i]);
		}

		return Inst;
	};

	auto Const_XX = [](DWORD XX)->std::vector<BYTE>
	{
		std::vector<BYTE> Inst;
		union
		{
			BYTE  Bytes[4];
			DWORD Dword;
		};

		Dword = XX;
		for (ULONG i = 0; i < _countof(Bytes); i++) {
			Inst.push_back(Bytes[i]);
		}

		return Inst;
	};

	SIN(0x64, 0xA1, 0x30, 0x00, 0x00, 0x00); // mov     eax, large fs:30h
	SIN(0x22, 0x33, 0x44);
	SIN(0x8B, 0x40, 0x08); // mov     eax, [eax+8]
	SIN(0x05); // add eax, imm
	auto&& OffsetImm = Const_XX(OldOepOffset);
	SIN2(OffsetImm);
	SIN(0x50); // push eax ; save oep
	SIN(0x64, 0xA1, 0x30, 0x00, 0x00, 0x00); // mov     eax, large fs:30h
	SIN(0x8B, 0x40, 0x08); // mov     eax, [eax+8]
	SIN(0x05); // add eax, imm
	auto&& DllNameImm = Const_XX(DllNameBase);
	SIN2(DllNameImm);
	SIN(0x9C); // pushfd
	SIN(0x60); // pushad
	SIN(0x50); // push eax; DllName.vaddr
	SIN(0xE8, 0x08, 0x00, 0x00, 0x00); // call eip + 0x13
	SIN(0x83, 0xC4, 0x04); // add esp, 0x4
	SIN(0x61); // popad
	SIN(0x9D); // popfd
	SIN(0x58); // pop eax
	SIN(0xFF, 0xE0); // jmp eax

	// LoadDll

	SIN(0x55);       // push    ebp
	SIN(0x8B, 0xEC); // mov     ebp, esp
	SIN(0x64, 0xA1, 0x30, 0x00, 0x00, 0x00); // mov     eax, large fs:30h
	SIN(0x83, 0xEC, 0x08); // sub     esp, 8
	SIN(0x8B, 0x40, 0x0C); // mov     eax, [eax+0Ch]
	SIN(0x53); // push    ebx
	SIN(0x56); // push    esi
	SIN(0x57); // push    edi
	SIN(0x8B, 0x48, 0x1C); // mov     ecx, [eax+1Ch]
	SIN(0x83, 0xE9, 0x10); // sub     ecx, 10h
	SIN(0x8B, 0xF9); // mov     edi, ecx
	SIN(0x66, 0x0F, 0x1F, 0x44, 0x00, 0x00); // nop     word ptr [eax+eax+00h]

	// loc_401030:
	SIN(0x8B, 0x49, 0x10); // mov     ecx, [ecx+10h]
	SIN(0x83, 0xE9, 0x10); // sub     ecx, 10h
	SIN(0x8B, 0x51, 0x30); // mov     edx, [ecx+30h]
	SIN(0x85, 0xD2); // test    edx, edx
	SIN(0x74, 0x46); // jz      short loc_401083
	SIN(0x8B, 0x02); // mov     eax, [edx]
	SIN(0x8B, 0x72, 0x04); // mov     esi, [edx+4]
	SIN(0x25, 0xDF, 0xFF, 0xDF, 0xFF); // and     eax, 0FFDFFFDFh
	SIN(0x81, 0xE6, 0xDF, 0xFF, 0xDF, 0xFF); // and     esi, 0FFDFFFDFh
	SIN(0x3D, 0x4B, 0x00, 0x45, 0x00); // cmp     eax, 45004Bh
	SIN(0x75, 0x2F); // jnz     short loc_401083
	SIN(0x81, 0xFE, 0x52, 0x00, 0x4E, 0x00); // cmp     esi, 4E0052h
	SIN(0x75, 0x27); // jnz     short loc_401083
	SIN(0x8B, 0x42, 0x08); // mov     eax, [edx+8]
	SIN(0x8B, 0x72, 0x0C); // mov     esi, [edx+0Ch]
	SIN(0x25, 0xDF, 0xFF, 0xDF, 0xFF); // and     eax, 0FFDFFFDFh
	SIN(0x81, 0xE6, 0xDF, 0xFF, 0xDF, 0xFF); // and     esi, 0FFDFFFDFh
	SIN(0x3D, 0x45, 0x00, 0x4C, 0x00); // cmp     eax, 4C0045h
	SIN(0x75, 0x0F); // jnz     short loc_401083
	SIN(0x81, 0xFE, 0x13, 0x00, 0x12, 0x00); // cmp     esi, 120013h
	SIN(0x75, 0x07); // jnz     short loc_401083
	SIN(0x66, 0x83, 0x7A, 0x10, 0x2E); // cmp     word ptr [edx+10h], 2Eh
	SIN(0x74, 0x0D); // jz      short loc_401090

	// loc_401083
	SIN(0x3B, 0xF9); // cmp     edi, ecx
	SIN(0x75, 0xA9); // jnz     short loc_401030

	// loc_401087
	SIN(0x5F); // pop     edi
	SIN(0x5E); // pop     esi
	SIN(0x33, 0xC0); //  xor     eax, eax
	SIN(0x5B); // pop     ebx
	SIN(0x8B, 0xE5); // mov     esp, ebp
	SIN(0x5D); // pop     ebp
	SIN(0xC3); // retn

	// loc_401090
	SIN(0x8B, 0x59, 0x18); // mov     ebx, [ecx+18h]
	SIN(0xB8, 0x4D, 0x5A, 0x00, 0x00); // mov     eax, 5A4Dh
	SIN(0x66, 0x39, 0x03);
	SIN(0x75, 0xEA); // jnz     short loc_401087
	SIN(0x8B, 0x53, 0x3C);  // mov     edx, [ebx+3Ch]
	SIN(0x03, 0xD3); // add     edx, ebx
	SIN(0x81, 0x3A, 0x50, 0x45, 0x00, 0x00); // cmp     dword ptr [edx], 4550h
	SIN(0x75, 0xDD); // jnz     short loc_401087
	SIN(0x0F, 0xB7, 0x42, 0x18); // movzx   eax, word ptr [edx+18h]
	SIN(0xBE, 0x0B, 0x01, 0x00, 0x00); // mov     esi, 10Bh
	SIN(0x8B, 0xC8); // mov     ecx, eax
	SIN(0xBF, 0x0B, 0x02, 0x00, 0x00); // mov     edi, 20Bh
	SIN(0x89, 0x4D, 0xF8); // mov     [ebp+var_8], ecx
	SIN(0x66, 0x3B, 0xCE); // cmp     cx, si
	SIN(0x74, 0x0C); // jz      short loc_4010CE
	SIN(0x66, 0x3B, 0xC7); // cmp     ax, di
	SIN(0x75, 0xC0); // jnz     short loc_401087
	SIN(0xB8, 0x88, 0x00, 0x00, 0x00); // mov     eax, 88h
	SIN(0xEB, 0x05); // jmp     short loc_4010D3

	// loc_4010CE
	SIN(0xB8, 0x78, 0x00, 0x00, 0x00); // mov     eax, 78h

	// loc_4010D3
	SIN(0x8B, 0x7C, 0x10, 0x04); // mov     edi, [eax+edx+4]
	SIN(0x8B, 0x4A, 0x50); // mov     ecx, [edx+50h]
	SIN(0x85, 0xFF); // test    edi, edi
	SIN(0x74, 0xA9); // jz      short loc_401087
	SIN(0x8B, 0x34, 0x10); // mov     esi, [eax+edx]
	SIN(0x33, 0xC0); // xor     eax, eax
	SIN(0x89, 0x75, 0xFC); // mov     [ebp+var_4], esi
	SIN(0x01, 0x7D, 0xFC); // add     [ebp+var_4], edi
	SIN(0x13, 0xC0); // adc     eax, eax
	SIN(0x85, 0xC0); // test    eax, eax
	SIN(0x77, 0x98); // ja      short loc_401087
	SIN(0x72, 0x06); // jb      short loc_4010F7
	SIN(0x83, 0x7D, 0xFC, 0xFF); // cmp     [ebp+var_4], 0FFFFFFFFh
	SIN(0x77, 0x90); // ja      short loc_401087

	// loc_4010F7
	SIN(0x3B, 0xF1); // cmp     esi, ecx
	SIN(0x77, 0x8C); // ja      short loc_401087
	SIN(0x8D, 0x04, 0x3E); // lea     eax, [esi+edi]
	SIN(0x3B, 0xC1); // cmp     eax, ecx
	SIN(0x77, 0x85); // ja      short loc_401087
	SIN(0x39, 0x4C, 0x1E, 0x1C); // cmp     [esi+ebx+1Ch], ecx
	SIN(0x0F, 0x83, 0x7B, 0xFF, 0xFF, 0xFF); // jnb     loc_401087
	SIN(0xB8, 0x0B, 0x02, 0x00, 0x00); // mov     eax, 20Bh
	SIN(0x66, 0x39, 0x45, 0xF8); // cmp     word ptr [ebp+var_8], ax
	SIN(0x74, 0x5B); // jz      short loc_401172
	SIN(0x8B, 0x42, 0x78); // mov     eax, [edx+78h]
	SIN(0x03, 0xC3); // add     eax, ebx
	SIN(0x8B, 0x48, 0x18); // mov     ecx, [eax+18h]
	SIN(0x8B, 0x70, 0x20); // mov     esi, [eax+20h]
	SIN(0x8B, 0x78, 0x24); // mov     edi, [eax+24h]
	SIN(0x03, 0xF3); // add     esi, ebx
	SIN(0x89, 0x4D, 0xFC);  // mov     [ebp+var_4], ecx
	SIN(0x8B, 0x48, 0x1C); // mov     ecx, [eax+1Ch]
	SIN(0x03, 0xCB); // add     ecx, ebx
	SIN(0x03, 0xFB); // add     edi, ebx
	SIN(0x89, 0x4D, 0xF8); // mov     [ebp+var_8], ecx

	// loc_401134
	SIN(0x8B, 0x16); // mov     edx, [esi]
	SIN(0x33, 0xC9); // xor     ecx, ecx
	SIN(0x03, 0xD3); // add     edx, ebx
	SIN(0x8A, 0x02); // mov     al, [edx]
	SIN(0x84, 0xC0); // test    al, al
	SIN(0x74, 0x1D); // jz      short loc_40115D

	// loc_401140
	SIN(0xC1, 0xC1, 0x0D); // rol     ecx, 0Dh
	SIN(0x8D, 0x52, 0x01); // lea     edx, [edx+1]
	SIN(0x0F, 0xB6, 0xC0); // movzx   eax, al
	SIN(0x33, 0xC8); // xor     ecx, eax
	SIN(0x8A, 0x02); // mov     al, [edx]
	SIN(0x84, 0xC0); // test    al, al
	SIN(0x75, 0xEF); // jnz     short loc_401140
	SIN(0x81, 0xF9, 0xAC, 0x35, 0x33, 0xA3); // cmp     ecx, 0A33335ACh
	SIN(0x0F, 0x84, 0xBC, 0x00, 0x00, 0x00); // jz      loc_401219

	// loc_40115D
	SIN(0x83, 0xC7, 0x02); // add     edi, 2
	SIN(0x83, 0xC6, 0x04); // add     esi, 4
	SIN(0x83, 0x6D, 0xFC, 0x01); // sub     [ebp+var_4], 1
	SIN(0x75, 0xCB); // jnz     short loc_401134
	SIN(0x5F); // pop     edi
	SIN(0x5E); // pop     esi
	SIN(0x33, 0xC0); // xor     eax, eax
	SIN(0x5B); // pop     ebx
	SIN(0x8B, 0xE5); // mov     esp, ebp
	SIN(0x5D); // pop     ebp
	SIN(0xC3); // retn

	// loc_401172
	SIN(0x8B, 0xB2, 0x8C, 0x00, 0x00, 0x00); // mov     esi, [edx+8Ch]
	SIN(0x85, 0xF6); // test    esi, esi
	SIN(0x0F, 0x84, 0x77, 0x00, 0x00, 0x00); // jz      loc_4011F7
	SIN(0x8B, 0xBA, 0x88, 0x00, 0x00, 0x00); // mov     edi, [edx+88h]
	SIN(0x33, 0xC0); // xor     eax, eax
	SIN(0x8B, 0xD7); // mov     edx, edi
	SIN(0x03, 0xD6); // add     edx, esi
	SIN(0x13, 0xC0); // adc     eax, eax
	SIN(0x85, 0xC0); // test    eax, eax
	SIN(0x77, 0x65); // ja      short loc_4011F7
	SIN(0x72, 0x05); // jb      short loc_401199
	SIN(0x83, 0xFA, 0xFF); // cmp     edx, 0FFFFFFFFh
	SIN(0x77, 0x5E); // ja      short loc_4011F7

	// loc_401199
	SIN(0x3B, 0xF9); // cmp     edi, ecx
	SIN(0x77, 0x5A); // ja      short loc_4011F7
	SIN(0x8D, 0x04, 0x37); // lea     eax, [edi+esi]
	SIN(0x3B, 0xC1); // cmp     eax, ecx
	SIN(0x77, 0x53); // ja      short loc_4011F7
	SIN(0x39, 0x4C, 0x1F, 0x1C); // cmp     [edi+ebx+1Ch], ecx
	SIN(0x73, 0x4D); // jnb     short loc_4011F7
	SIN(0x8B, 0x44, 0x3B, 0x18); // mov     eax, [ebx+edi+18h]
	SIN(0x8B, 0x74, 0x3B, 0x20); // mov     esi, [ebx+edi+20h]
	SIN(0x89, 0x45, 0xFC); // mov     [ebp+var_4], eax
	SIN(0x03, 0xF3); // add     esi, ebx
	SIN(0x8B, 0x44, 0x3B, 0x1C); // mov     eax, [ebx+edi+1Ch]
	SIN(0x8B, 0x7C, 0x3B, 0x24); // mov     edi, [ebx+edi+24h]
	SIN(0x03, 0xC3); // add     eax, ebx
	SIN(0x89, 0x45, 0xF8); // mov     [ebp+var_8], eax
	SIN(0x03, 0xFB); // add     edi, ebx

	// loc_4011C6
	SIN(0x8B, 0x16); // mov     edx, [esi]
	SIN(0x33, 0xC9); // xor     ecx, ecx
	SIN(0x03, 0xD3); // add     edx, ebx
	SIN(0x8A, 0x02); // mov     al, [edx]
	SIN(0x84, 0xC0); // test    al, al
	SIN(0x74, 0x19); // jz      short loc_4011EB

	// loc_4011D2
	SIN(0xC1, 0xC1, 0x0D); // rol     ecx, 0Dh
	SIN(0x8D, 0x52, 0x01); // lea     edx, [edx+1]
	SIN(0x0F, 0xB6, 0xC0); // movzx   eax, al
	SIN(0x33, 0xC8); // xor     ecx, eax
	SIN(0x8A, 0x02); // mov     al, [edx]
	SIN(0x84, 0xC0); // test    al, al
	SIN(0x75, 0xEF); // jnz     short loc_4011D2
	SIN(0x81, 0xF9, 0xAC, 0x35, 0x33, 0xA3); // cmp     ecx, 0A33335ACh
	SIN(0x74, 0x2E); // jz      short loc_401219

	// loc_4011EB
	SIN(0x83, 0xC7, 0x02); // add     edi, 2
	SIN(0x83, 0xC6, 0x04); // add     esi, 4
	SIN(0x83, 0x6D, 0xFC, 0x01); // sub     [ebp+var_4], 1
	SIN(0x75, 0xCF); // jnz     short loc_4011C6

	// loc_4011F7
	SIN(0x33, 0xC0); // xor     eax, eax

	// loc_4011F9
	SIN(0x85, 0xC0); // test    eax, eax
	SIN(0x0F, 0x84, 0x86, 0xFE, 0xFF, 0xFF); // jz      loc_401087
	SIN(0x8B, 0x00); // mov     eax, [eax]
	SIN(0x03, 0xC3); // add     eax, ebx
	SIN(0x0F, 0x84, 0x7C, 0xFE, 0xFF, 0xFF); // jz      loc_401087
	SIN(0xff, 0x75, 0x08); // push DWORD PTR [ebp+0x8] ; the arg
	SIN(0x90); // nop ; padding, sizeof(push imm) = 5
	SIN(0x90); // nop
	SIN(0xFF, 0xD0); // call    eax
	SIN(0x5F); // pop     edi
	SIN(0x5E); // pop     esi
	SIN(0x5B); // pop     ebx
	SIN(0x8B, 0xE5); // mov     esp, ebp
	SIN(0x5D); // pop     ebp
	SIN(0xC3); // retn

	// loc_401219
	SIN(0x0F, 0xB7, 0x07); // movzx   eax, word ptr [edi]
	SIN(0x8B, 0x4D, 0xF8); // mov     ecx, [ebp+var_8]
	SIN(0x8D, 0x04, 0x81); // lea     eax, [ecx+eax*4]
	SIN(0xEB, 0xD5); // jmp     short loc_4011F9

#undef SIN
#undef SIN2


	RtlCopyMemory(Payload, ShellCode.data(), ShellCode.size());
	return TRUE;
}


BOOL CompilePayload64(PBYTE Payload, ULONG OldOepOffset, ULONG DllNameBase)
{
	std::vector<BYTE> ShellCode;

	auto MakeImm = [](DWORD64 Value)->std::vector<BYTE>
	{
		std::vector<BYTE> Inst;

		union {
			DWORD64 Qword;
			BYTE    Bytes[8];
		};

		Qword = Value;
		for (SIZE_T i = 0; i < sizeof(Bytes); i++) {
			Inst.push_back(Bytes[i]);
		}

		return Inst;
	};


	auto MakeImm32 = [](DWORD Value)->std::vector<BYTE>
	{
		std::vector<BYTE> Inst;

		union {
			DWORD Dword;
			BYTE  Bytes[4];
		};

		Dword = Value;
		for (SIZE_T i = 0; i < sizeof(Bytes); i++) {
			Inst.push_back(Bytes[i]);
		}

		return Inst;
	};


#define SIN(...) ShellCode.insert(ShellCode.end(), {__VA_ARGS__})
#define SIN2(x) ShellCode.insert(ShellCode.end(), x.begin(), x.end())

	SIN(0x65, 0x48, 0x8B, 0x04, 0x25, 0x60, 0x00, 0x00, 0x00); //mov     rax, gs:60h
	SIN(0x48, 0x8B, 0x40, 0x10); //mov     rax, [rax+10h]
	SIN(0x48, 0x05); // add rax, imm
	auto&& Imm = MakeImm32(OldOepOffset);
	SIN2(Imm);
	SIN(0x50); // push rax

	SIN(0x9C); // pushfq
	SIN(0x50); // push rax
	SIN(0x53); // push rbx
	SIN(0x51); // push   rcx
	SIN(0x52); // push   rdx
	SIN(0x56); // push   rsi
	SIN(0x57); // push   rdi
	SIN(0x55); // push   rbp
	SIN(0x54); // push   rsp
	SIN(0x41, 0x50); // push   r8
	SIN(0x41, 0x51); // push   r9
	SIN(0x41, 0x52); // push   r10
	SIN(0x41, 0x53); // push   r11
	SIN(0x41, 0x54); // push   r12
	SIN(0x41, 0x55); // push   r13
	SIN(0x41, 0x56); // push   r14
	SIN(0x41, 0x57); // push   r15

	SIN(0x65, 0x48, 0x8B, 0x04, 0x25, 0x60, 0x00, 0x00, 0x00); //mov     rax, gs:60h
	SIN(0x48, 0x8B, 0x40, 0x10); //mov     rax, [rax+10h]
	SIN(0x48, 0x05); // add rax, imm
	auto&& Imm2 = MakeImm32(DllNameBase);
	SIN2(Imm2);
	SIN(0x49, 0x89, 0xC7); // mov r15, rax
	SIN(0xE8, 0x1C, 0x00, 0x00, 0x00); // call 0x21 (offset : 0x1C)
	SIN(0x41, 0x5f); // pop    r15
	SIN(0x41, 0x5e); // pop    r14
	SIN(0x41, 0x5d); // pop    r13
	SIN(0x41, 0x5c); // pop    r12
	SIN(0x41, 0x5b); // pop    r11
	SIN(0x41, 0x5a); // pop    r10
	SIN(0x41, 0x59); // pop    r9
	SIN(0x41, 0x58); // pop    r8
	SIN(0x5c); // pop    rsp
	SIN(0x5d); // pop    rbp
	SIN(0x5f); // pop    rdi
	SIN(0x5e); // pop    rsi
	SIN(0x5a); // pop    rdx
	SIN(0x59); // pop    rcx
	SIN(0x5b); // pop    rbx
	SIN(0x58); // pop    rax
	SIN(0x9D); // popfq
	SIN(0x58); // pop rax
	SIN(0xFF, 0xE0); //jmp rax

	// LoadDll
	SIN(0x48, 0x83, 0xEC, 0x38); //sub     rsp, 38h
	SIN(0x65, 0x48, 0x8B, 0x04, 0x25, 0x60, 0x00, 0x00, 0x00); //mov     rax, gs:60h
	SIN(0x49, 0xB9, 0xDF, 0xFF, 0xDF, 0xFF, 0xDF, 0xFF, 0xDF, 0xFF); //mov     r9, 0FFDFFFDFFFDFFFDFh
	SIN(0x48, 0x89, 0x5C, 0x24, 0x40); //mov     [rsp+38h+arg_0], rbx
	SIN(0x49, 0xBA, 0x4B, 0x00, 0x45, 0x00, 0x52, 0x00, 0x4E, 0x00); //mov     r10, 4E00520045004Bh
	SIN(0x48, 0x89, 0x74, 0x24, 0x48); //mov     [rsp+38h+arg_8], rsi
	SIN(0x49, 0xBB, 0x45, 0x00, 0x4C, 0x00, 0x13, 0x00, 0x12, 0x00); //mov     r11, 120013004C0045h
	SIN(0x48, 0x89, 0x7C, 0x24, 0x30); //mov     [rsp+38h+var_8], rdi
	SIN(0x48, 0x8B, 0x48, 0x18); //mov     rcx, [rax+18h]
	SIN(0x48, 0x8B, 0x51, 0x30); //mov     rdx, [rcx+30h]
	SIN(0x48, 0x83, 0xEA, 0x20); //sub     rdx, 20h ; ' '
	SIN(0x4C, 0x8B, 0xC2); //mov     r8, rdx
	SIN(0x0F, 0x1F, 0x80, 0x00, 0x00, 0x00, 0x00); //nop     dword ptr [rax+00000000h]
	SIN(0x48, 0x8B, 0x52, 0x20); //mov     rdx, [rdx+20h]
	SIN(0x48, 0x83, 0xEA, 0x20); //sub     rdx, 20h ; ' '
	SIN(0x48, 0x8B, 0x4A, 0x60); //mov     rcx, [rdx+60h]
	SIN(0x48, 0x85, 0xC9); //test    rcx, rcx
	SIN(0x74, 0x1E); //jz      short loc_14000108F
	SIN(0x48, 0x8B, 0x01); //mov     rax, [rcx]
	SIN(0x49, 0x23, 0xC1); //and     rax, r9
	SIN(0x49, 0x3B, 0xC2); //cmp     rax, r10
	SIN(0x75, 0x13); //jnz     short loc_14000108F
	SIN(0x48, 0x8B, 0x41, 0x08); //mov     rax, [rcx+8]
	SIN(0x49, 0x23, 0xC1); //and     rax, r9
	SIN(0x49, 0x3B, 0xC3); //cmp     rax, r11
	SIN(0x75, 0x07); //jnz     short loc_14000108F
	SIN(0x66, 0x83, 0x79, 0x10, 0x2E); //cmp     word ptr [rcx+10h], 2Eh ; '.'
	SIN(0x74, 0x1B); //jz      short loc_1400010AA
	SIN(0x4C, 0x3B, 0xC2); //cmp     r8, rdx
	SIN(0x75, 0xCC); //jnz     short loc_140001060
	SIN(0x33, 0xC0); //xor     eax, eax
	SIN(0x48, 0x8B, 0x7C, 0x24, 0x30); //mov     rdi, [rsp+38h+var_8]
	SIN(0x48, 0x8B, 0x74, 0x24, 0x48); //mov     rsi, [rsp+38h+arg_8]
	SIN(0x48, 0x8B, 0x5C, 0x24, 0x40); //mov     rbx, [rsp+38h+arg_0]
	SIN(0x48, 0x83, 0xC4, 0x38); //add     rsp, 38h
	SIN(0xC3); //retn
	SIN(0x4C, 0x8B, 0x5A, 0x30); //mov     r11, [rdx+30h]
	SIN(0xB8, 0x4D, 0x5A, 0x00, 0x00); //mov     eax, 5A4Dh
	SIN(0x66, 0x41, 0x39, 0x03); //cmp     [r11], ax
	SIN(0x75, 0xDB); //jnz     short loc_140001094
	SIN(0x49, 0x63, 0x53, 0x3C); //movsxd  rdx, dword ptr [r11+3Ch]
	SIN(0x49, 0x03, 0xD3); //add     rdx, r11
	SIN(0x81, 0x3A, 0x50, 0x45, 0x00, 0x00); //cmp     dword ptr [rdx], 4550h
	SIN(0x75, 0xCC); //jnz     short loc_140001094
	SIN(0x0F, 0xB7, 0x5A, 0x18); //movzx   ebx, word ptr [rdx+18h]
	SIN(0xB8, 0x0B, 0x01, 0x00, 0x00); //mov     eax, 10Bh
	SIN(0xBF, 0x0B, 0x02, 0x00, 0x00); //mov     edi, 20Bh
	SIN(0x66, 0x3B, 0xD8); //cmp     bx, ax
	SIN(0x74, 0x05); //jz      short loc_1400010E0
	SIN(0x66, 0x3B, 0xDF); //cmp     bx, di
	SIN(0x75, 0xB4); //jnz     short loc_140001094
	SIN(0x44, 0x8B, 0x92, 0x8C, 0x00, 0x00, 0x00); //mov     r10d, [rdx+8Ch]
	SIN(0x44, 0x8B, 0x4A, 0x50); //mov     r9d, [rdx+50h]
	SIN(0x45, 0x85, 0xD2); //test    r10d, r10d
	SIN(0x74, 0xA4); //jz      short loc_140001094
	SIN(0x44, 0x8B, 0x82, 0x88, 0x00, 0x00, 0x00); //mov     r8d, [rdx+88h]
	SIN(0xBE, 0xFF, 0xFF, 0xFF, 0xFF); //mov     esi, 0FFFFFFFFh
	SIN(0x4B, 0x8D, 0x04, 0x10); //lea     rax, [r8+r10]
	SIN(0x48, 0x3B, 0xC6); //cmp     rax, rsi
	SIN(0x77, 0x8F); //ja      short loc_140001094
	SIN(0x4D, 0x3B, 0xC1); //cmp     r8, r9
	SIN(0x77, 0x8A); //ja      short loc_140001094
	SIN(0x43, 0x8D, 0x0C, 0x10); //lea     ecx, [r8+r10]
	SIN(0x49, 0x3B, 0xC9); //cmp     rcx, r9
	SIN(0x77, 0x81); //ja      short loc_140001094
	SIN(0x43, 0x8B, 0x4C, 0x18, 0x1C); //mov     ecx, [r8+r11+1Ch]
	SIN(0x49, 0x3B, 0xC9); //cmp     rcx, r9
	SIN(0x0F, 0x83, 0x73, 0xFF, 0xFF, 0xFF); //jnb     loc_140001094
	SIN(0x66, 0x3B, 0xDF); //cmp     bx, di
	SIN(0x0F, 0x84, 0xA0, 0x00, 0x00, 0x00); //jz      loc_1400011CA
	SIN(0x45, 0x33, 0xC9); //xor     r9d, r9d
	SIN(0x4C, 0x89, 0x4C, 0x24, 0x28); //mov     [rsp+38h+var_10], r9
	SIN(0x44, 0x89, 0x44, 0x24, 0x28); //mov     dword ptr [rsp+38h+var_10], r8d
	SIN(0x48, 0x8B, 0x4C, 0x24, 0x28); //mov     rcx, [rsp+38h+var_10]
	SIN(0x49, 0x03, 0xCB); //add     rcx, r11
	SIN(0x4C, 0x89, 0x4C, 0x24, 0x28); //mov     [rsp+38h+var_10], r9
	SIN(0x8B, 0x41, 0x1C); //mov     eax, [rcx+1Ch]
	SIN(0x8B, 0x59, 0x18); //mov     ebx, [rcx+18h]
	SIN(0x89, 0x44, 0x24, 0x28); //mov     dword ptr [rsp+38h+var_10], eax
	SIN(0x48, 0x8B, 0x7C, 0x24, 0x28); //mov     rdi, [rsp+38h+var_10]
	SIN(0x8B, 0x41, 0x20); //mov     eax, [rcx+20h]
	SIN(0x49, 0x03, 0xFB); //add     rdi, r11
	SIN(0x4C, 0x89, 0x4C, 0x24, 0x28); //mov     [rsp+38h+var_10], r9
	SIN(0x89, 0x44, 0x24, 0x28); //mov     dword ptr [rsp+38h+var_10], eax
	SIN(0x4C, 0x8B, 0x44, 0x24, 0x28); //mov     r8, [rsp+38h+var_10]
	SIN(0x8B, 0x41, 0x24); //mov     eax, [rcx+24h]
	SIN(0x4D, 0x03, 0xC3); //add     r8, r11
	SIN(0x4C, 0x89, 0x4C, 0x24, 0x28); //mov     [rsp+38h+var_10], r9
	SIN(0x89, 0x44, 0x24, 0x28); //mov     dword ptr [rsp+38h+var_10], eax
	SIN(0x4C, 0x8B, 0x54, 0x24, 0x28); //mov     r10, [rsp+38h+var_10]
	SIN(0x4D, 0x03, 0xD3); //add     r10, r11
	SIN(0x66, 0x90); //xchg    ax, ax
	SIN(0x49, 0x8B, 0x10); //mov     rdx, [r8]
	SIN(0x41, 0x8B, 0xC1); //mov     eax, r9d
	SIN(0x49, 0x03, 0xD3); //add     rdx, r11
	SIN(0x0F, 0xB6, 0x0A); //movzx   ecx, byte ptr [rdx]
	SIN(0x84, 0xC9); //test    cl, cl
	SIN(0x74, 0x1A); //jz      short loc_1400011AA
	SIN(0xC1, 0xC0, 0x0D); //rol     eax, 0Dh
	SIN(0x48, 0x8D, 0x52, 0x01); //lea     rdx, [rdx+1]
	SIN(0x0F, 0xB6, 0xC9); //movzx   ecx, cl
	SIN(0x33, 0xC1); //xor     eax, ecx
	SIN(0x0F, 0xB6, 0x0A); //movzx   ecx, byte ptr [rdx]
	SIN(0x84, 0xC9); //test    cl, cl
	SIN(0x75, 0xED); //jnz     short loc_140001190
	SIN(0x3D, 0xAC, 0x35, 0x33, 0xA3); //cmp     eax, 0A33335ACh
	SIN(0x74, 0x13); //jz      short loc_1400011BD
	SIN(0x49, 0x83, 0xC2, 0x02); //add     r10, 2
	SIN(0x49, 0x83, 0xC0, 0x08); //add     r8, 8
	SIN(0x48, 0x83, 0xEB, 0x01); //sub     rbx, 1
	SIN(0x75, 0xC8); //jnz     short loc_140001180
	SIN(0xE9, 0xD7, 0xFE, 0xFF, 0xFF); //jmp     loc_140001094
	SIN(0x41, 0x0F, 0xB7, 0x02); //movzx   eax, word ptr [r10]
	SIN(0x48, 0x8D, 0x0C, 0xC7); //lea     rcx, [rdi+rax*8]
	SIN(0xE9, 0xC1, 0x00, 0x00, 0x00); //jmp     loc_14000128B
	SIN(0x81, 0x3A, 0x50, 0x45, 0x00, 0x00); //cmp     dword ptr [rdx], 4550h
	SIN(0x0F, 0x85, 0xAF, 0x00, 0x00, 0x00); //jnz     loc_140001285
	SIN(0x4B, 0x8D, 0x04, 0x10); //lea     rax, [r8+r10]
	SIN(0x48, 0x3B, 0xC6); //cmp     rax, rsi
	SIN(0x0F, 0x87, 0xA2, 0x00, 0x00, 0x00); //ja      loc_140001285
	SIN(0x4D, 0x3B, 0xC1); //cmp     r8, r9
	SIN(0x0F, 0x87, 0x99, 0x00, 0x00, 0x00); //ja      loc_140001285
	SIN(0x43, 0x8D, 0x0C, 0x02); //lea     ecx, [r10+r8]
	SIN(0x49, 0x3B, 0xC9); //cmp     rcx, r9
	SIN(0x0F, 0x87, 0x8C, 0x00, 0x00, 0x00); //ja      loc_140001285
	SIN(0x45, 0x33, 0xC9); //xor     r9d, r9d
	SIN(0x4C, 0x89, 0x4C, 0x24, 0x28); //mov     [rsp+38h+var_10], r9
	SIN(0x44, 0x89, 0x44, 0x24, 0x28); //mov     dword ptr [rsp+38h+var_10], r8d
	SIN(0x48, 0x8B, 0x44, 0x24, 0x28); //mov     rax, [rsp+38h+var_10]
	SIN(0x41, 0x8B, 0x7C, 0x03, 0x1C); //mov     edi, [r11+rax+1Ch]
	SIN(0x45, 0x8B, 0x44, 0x03, 0x20); //mov     r8d, [r11+rax+20h]
	SIN(0x49, 0x03, 0xFB); //add     rdi, r11
	SIN(0x45, 0x8B, 0x54, 0x03, 0x24); //mov     r10d, [r11+rax+24h]
	SIN(0x4D, 0x03, 0xC3); //add     r8, r11
	SIN(0x41, 0x8B, 0x5C, 0x03, 0x18); //mov     ebx, [r11+rax+18h]
	SIN(0x4D, 0x03, 0xD3); //add     r10, r11
	SIN(0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00); //nop     dword ptr [rax+rax+00000000h]
	SIN(0x41, 0x8B, 0x00); //mov     eax, [r8]
	SIN(0x41, 0x8B, 0xC9); //mov     ecx, r9d
	SIN(0x4C, 0x89, 0x4C, 0x24, 0x28); //mov     [rsp+38h+var_10], r9
	SIN(0x89, 0x44, 0x24, 0x28); //mov     dword ptr [rsp+38h+var_10], eax
	SIN(0x48, 0x8B, 0x54, 0x24, 0x28); //mov     rdx, [rsp+38h+var_10]
	SIN(0x49, 0x03, 0xD3); //add     rdx, r11
	SIN(0x0F, 0xB6, 0x02); //movzx   eax, byte ptr [rdx]
	SIN(0x84, 0xC0); //test    al, al
	SIN(0x74, 0x1D); //jz      short loc_14000126B
	SIN(0x66, 0x90); //xchg    ax, ax
	SIN(0xC1, 0xC1, 0x0D); //rol     ecx, 0Dh
	SIN(0x48, 0x8D, 0x52, 0x01); //lea     rdx, [rdx+1]
	SIN(0x0F, 0xB6, 0xC0); //movzx   eax, al
	SIN(0x33, 0xC8); //xor     ecx, eax
	SIN(0x0F, 0xB6, 0x02); //movzx   eax, byte ptr [rdx]
	SIN(0x84, 0xC0); //test    al, al
	SIN(0x75, 0xED); //jnz     short loc_140001250
	SIN(0x81, 0xF9, 0xAC, 0x35, 0x33, 0xA3); //cmp     ecx, 0A33335ACh
	SIN(0x74, 0x10); //jz      short loc_14000127B
	SIN(0x49, 0x83, 0xC2, 0x02); //add     r10, 2
	SIN(0x49, 0x83, 0xC0, 0x04); //add     r8, 4
	SIN(0x48, 0x83, 0xEB, 0x01); //sub     rbx, 1
	SIN(0x75, 0xB7); //jnz     short loc_140001230
	SIN(0xEB, 0x0D); //jmp     short loc_140001288
	SIN(0x41, 0x0F, 0xB7, 0x02); //movzx   eax, word ptr [r10]
	SIN(0x48, 0x8D, 0x0C, 0x87); //lea     rcx, [rdi+rax*4]
	SIN(0xEB, 0x06); //jmp     short loc_14000128B
	SIN(0x45, 0x33, 0xC9); //xor     r9d, r9d
	SIN(0x49, 0x8B, 0xC9); //mov     rcx, r9
	SIN(0x48, 0x85, 0xC9); //test    rcx, rcx
	SIN(0x0F, 0x84, 0x00, 0xFE, 0xFF, 0xFF); //jz      loc_140001094
	SIN(0x8B, 0x01); //mov     eax, [rcx]
	SIN(0x4C, 0x89, 0x4C, 0x24, 0x28); //mov     [rsp+38h+var_10], r9
	SIN(0x89, 0x44, 0x24, 0x28); //mov     dword ptr [rsp+38h+var_10], eax
	SIN(0x48, 0x8B, 0x54, 0x24, 0x28); //mov     rdx, [rsp+38h+var_10]
	SIN(0x49, 0x03, 0xD3); //add     rdx, r11
	SIN(0x0F, 0x84, 0xE7, 0xFD, 0xFF, 0xFF); //jz      loc_140001094

	SIN(0x4C, 0x89, 0xF9); // mov rcx, r15 ; dllname.vaddr
	SIN(0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90); // nop; padding
	SIN(0x48, 0x8B, 0x7C, 0x24, 0x30); //mov     rdi, [rsp+38h+var_8]
	SIN(0x48, 0x8B, 0x74, 0x24, 0x48); //mov     rsi, [rsp+38h+arg_8]
	SIN(0x48, 0x8B, 0x5C, 0x24, 0x40); //mov     rbx, [rsp+38h+arg_0]
	SIN(0x48, 0x83, 0xC4, 0x38); //add     rsp, 38h
	SIN(0x48, 0xFF, 0xE2); //jmp     rdx

#undef SIN
#undef SIN2

	RtlCopyMemory(Payload, ShellCode.data(), ShellCode.size());

	return TRUE;
}

BOOL AddSection64(PBYTE Payload, PVOID Base, PIMAGE_NT_HEADERS64 Header, PCSTR SectionName, PCWSTR DllName)
{
	ULONG Size = Header->OptionalHeader.SizeOfImage;
	ULONG NumberOfSections = Header->FileHeader.NumberOfSections;

	if (NumberOfSections >= 64) {
		return FALSE;
	}

	if (NumberOfSections <= 0) {
		return FALSE;
	}

	if (StrLengthW(DllName) > MAX_PATH) {
		return FALSE;
	}

	PIMAGE_SECTION_HEADER SectionHeaders = IMAGE_FIRST_SECTION(Header);
	if ((ULONG64)&SectionHeaders[NumberOfSections - 1] - (ULONG64)Base + sizeof(IMAGE_SECTION_HEADER) > SectionHeaders[0].PointerToRawData) {
		PrintConsoleW(L"No room for the new section...\n");
		return FALSE;
	}

	ZeroMemory(&SectionHeaders[NumberOfSections], sizeof(IMAGE_SECTION_HEADER));
	CopyMemory(&SectionHeaders[NumberOfSections].Name, SectionName, 8);

	SectionHeaders[NumberOfSections].Misc.VirtualSize = P2ALIGNUP(0x1000, Header->OptionalHeader.SectionAlignment);
	SectionHeaders[NumberOfSections].VirtualAddress = P2ALIGNUP(SectionHeaders[NumberOfSections - 1].Misc.VirtualSize + SectionHeaders[NumberOfSections - 1].VirtualAddress, Header->OptionalHeader.SectionAlignment);
	SectionHeaders[NumberOfSections].SizeOfRawData = P2ALIGNUP(0x1000, Header->OptionalHeader.FileAlignment);
	SectionHeaders[NumberOfSections].PointerToRawData = SectionHeaders[NumberOfSections - 1].SizeOfRawData + SectionHeaders[NumberOfSections - 1].PointerToRawData;
	SectionHeaders[NumberOfSections].Characteristics = IMAGE_SCN_CNT_CODE |
		IMAGE_SCN_MEM_EXECUTE |
		IMAGE_SCN_MEM_READ;

	CompilePayload64(
		Payload,
		Header->OptionalHeader.AddressOfEntryPoint,
		SectionHeaders[NumberOfSections].VirtualAddress + 0x500
	);

	RtlCopyMemory((PBYTE)Payload + 0x500, DllName, (StrLengthW(DllName) + 1) * sizeof(DllName[0]));

	Header->OptionalHeader.SizeOfImage = P2ALIGNUP(SectionHeaders[NumberOfSections].VirtualAddress + SectionHeaders[NumberOfSections].Misc.VirtualSize, Header->OptionalHeader.SectionAlignment);
	Header->OptionalHeader.AddressOfEntryPoint = SectionHeaders[NumberOfSections].VirtualAddress;
	Header->FileHeader.NumberOfSections = NumberOfSections + 1;

	return TRUE;
}

BOOL AddSection32(PBYTE Payload, PVOID Base, PIMAGE_NT_HEADERS32 Header, PCSTR SectionName, PCWSTR DllName)
{
	ULONG Size = Header->OptionalHeader.SizeOfImage;
	ULONG NumberOfSections = Header->FileHeader.NumberOfSections;

	if (NumberOfSections >= 64) {
		return FALSE;
	}

	if (NumberOfSections <= 0) {
		return FALSE;
	}

	if (StrLengthW(DllName) > MAX_PATH) {
		return FALSE;
	}


	PIMAGE_SECTION_HEADER SectionHeaders = IMAGE_FIRST_SECTION(Header);
	if ((ULONG64)&SectionHeaders[NumberOfSections - 1] - (ULONG64)Base + sizeof(IMAGE_SECTION_HEADER) > SectionHeaders[0].PointerToRawData) {
		PrintConsoleW(L"No room for the new section...\n");
		return FALSE;
	}

	ZeroMemory(&SectionHeaders[NumberOfSections], sizeof(IMAGE_SECTION_HEADER));
	CopyMemory(&SectionHeaders[NumberOfSections].Name, SectionName, StrLengthA(SectionName) > 8 ? 8 : StrLengthA(SectionName));

	SectionHeaders[NumberOfSections].Misc.VirtualSize = P2ALIGNUP(0x1000, Header->OptionalHeader.SectionAlignment);
	SectionHeaders[NumberOfSections].VirtualAddress = P2ALIGNUP(SectionHeaders[NumberOfSections - 1].Misc.VirtualSize + SectionHeaders[NumberOfSections - 1].VirtualAddress, Header->OptionalHeader.SectionAlignment);
	SectionHeaders[NumberOfSections].SizeOfRawData = P2ALIGNUP(0x1000, Header->OptionalHeader.FileAlignment);
	SectionHeaders[NumberOfSections].PointerToRawData = SectionHeaders[NumberOfSections - 1].SizeOfRawData + SectionHeaders[NumberOfSections - 1].PointerToRawData;
	SectionHeaders[NumberOfSections].Characteristics = IMAGE_SCN_CNT_CODE |
		IMAGE_SCN_MEM_EXECUTE |
		IMAGE_SCN_MEM_READ;

	CompilePayload32(
		Payload,
		Header->OptionalHeader.AddressOfEntryPoint,
		SectionHeaders[NumberOfSections].VirtualAddress + 0x500
	);

	RtlCopyMemory((PBYTE)Payload + 0x500, DllName, (StrLengthW(DllName) + 1) * sizeof(DllName[0]));

	Header->OptionalHeader.SizeOfImage = P2ALIGNUP(SectionHeaders[NumberOfSections].VirtualAddress + SectionHeaders[NumberOfSections].Misc.VirtualSize, Header->OptionalHeader.SectionAlignment);
	Header->OptionalHeader.AddressOfEntryPoint = SectionHeaders[NumberOfSections].VirtualAddress;
	Header->FileHeader.NumberOfSections = NumberOfSections + 1;

	return TRUE;
}


BOOL CalcPeSize(PIMAGE_NT_HEADERS NtHeader, ULONG_PTR Version)
{
	ULONG_PTR Size;
	PIMAGE_SECTION_HEADER SectionHeaders;

	switch (Version)
	{
	case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
		Size = ((PIMAGE_NT_HEADERS32)NtHeader)->OptionalHeader.SizeOfHeaders;
		SectionHeaders = (PIMAGE_SECTION_HEADER)((PBYTE)NtHeader + sizeof(IMAGE_NT_HEADERS32));
		for (ULONG_PTR i = 0; i < ((PIMAGE_NT_HEADERS32)NtHeader)->FileHeader.NumberOfSections; i++) {
			Size += SectionHeaders[i].SizeOfRawData;
		}
		return Size;

	case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
		Size = ((PIMAGE_NT_HEADERS64)NtHeader)->OptionalHeader.SizeOfHeaders;
		SectionHeaders = (PIMAGE_SECTION_HEADER)((PBYTE)NtHeader + sizeof(IMAGE_NT_HEADERS64));
		for (ULONG_PTR i = 0; i < ((PIMAGE_NT_HEADERS64)NtHeader)->FileHeader.NumberOfSections; i++) {
			Size += SectionHeaders[i].SizeOfRawData;
		}
		return Size;
	}

	return 0;
}


HANDLE MapFile(NtFileDisk& File, PVOID* Base)
{
	HANDLE Handle;

	if (Base == NULL) {
		return NULL;
	}

	Handle = CreateFileMappingW(File.GetHandle(), NULL, PAGE_READONLY, 0, 0, NULL);
	if (Handle == INVALID_HANDLE_VALUE || Handle == 0) {
		return NULL;
	}

	*Base = MapViewOfFile(Handle, FILE_MAP_READ, 0, 0, 0);

	return Handle;
}


BOOL UnMap(PVOID Base, HANDLE Handle)
{
	BOOL b1 = UnmapViewOfFile(Base);
	BOOL b2 = CloseHandle(Handle);

	if (b1 && b2) return TRUE;
	return FALSE;
}


BOOL AddSection(PCWSTR FileName, PCWSTR NewFileName, PCSTR SectionName, PCWSTR DllName)
{
	NTSTATUS      Status;
	NtFileDisk    File, NewFile;
	ULONG         Size;
	ULONG64       FileSize;
	PVOID         Base;
	PBYTE         Buffer;
	ULONG_PTR     Version;
	LARGE_INTEGER ByteTransferred;
	BOOL          Success;
	HANDLE        Handle;

	static BYTE Payload[0x1000];

	//
	// TODO:
	// how to bypass Patch.Gen2 (fake libc_main vs control flow obfuscate)
	// Or signed by user:)
	//

	Status = File.Open(FileName);
	if (NT_FAILED(Status))
		return FALSE;

	Base = 0;
	Size = File.GetSize64();
	Handle = MapFile(File, &Base);
	if (!Handle) {
		return FALSE;
	}

	PIMAGE_DOS_HEADER DosHeader = (PIMAGE_DOS_HEADER)Base;
	if (DosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
		UnMap(Base, Handle);
		return FALSE;
	}

	PIMAGE_NT_HEADERS NtHeader = ImageFileNtHeaders(Base, &Version);
	if (!NtHeader) {
		UnMap(Base, Handle);
		return FALSE;
	}

	Size = CalcPeSize(NtHeader, Version);
	Buffer = (PBYTE)AllocateMemory(Size);
	if (!Buffer) {
		UnMap(Base, Handle);
		return FALSE;
	}

	RtlCopyMemory(Buffer, Base, Size);
	UnMap(Base, Handle);
	File.Seek(Size, FILE_BEGIN);

	NtHeader = ImageFileNtHeaders(Buffer, &Version);

	switch (Version)
	{
	case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
		Success = AddSection32(Payload, Buffer, ((PIMAGE_NT_HEADERS32)NtHeader), SectionName, DllName);
		break;

	case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
		Success = AddSection64(Payload, Buffer, ((PIMAGE_NT_HEADERS64)NtHeader), SectionName, DllName);
		break;

	default:
		Success = FALSE;
		break;
	}

	if (!Success) {
		return FALSE;
	}

	FileSize = File.GetSize64();

	Status = NewFile.Create(NewFileName);
	if (NT_FAILED(Status))
		return FALSE;

	NewFile.Write(Buffer, Size);
	NewFile.Write(Payload, sizeof(Payload));

	RtlZeroMemory(Payload, sizeof(Payload));
	while (NT_SUCCESS(File.Read(Payload, sizeof(Payload), &ByteTransferred)) && ByteTransferred.QuadPart != 0) {
		NewFile.Write(Payload, ByteTransferred.LowPart);
	}

	File.Close();
	NewFile.Close();
	return TRUE;
}


static WCHAR Startup[] =
	L"Scripts.createWithDll32(\"%s\", \"KrkrUniversalPatch.dll\");\r\n"
	L"System.exit();\r\n";


static WCHAR Startup64[] =
	L"Scripts.createWithDll64(\"%s\", \"KrkrUniversalPatch.dll\");\r\n"	
	L"System.exit();\r\n";

NTSTATUS CreateStartupArchiveAfterExe(NtFileDisk& File, BOOL Is32Bit, PCWSTR ExeName)
{
	NTSTATUS                Status;
	PBYTE                   pbIndex;
	ULONG                   CompressedSize, BytesTransfered;
	WCHAR                   ScriptContent[0x200];
	LARGE_INTEGER           Size, Offset, FileOffset;
	SMyXP3IndexNormal       Index;
	KRKR2_XP3_DATA_HEADER   IndexHeader;
	KRKR2_XP3_HEADER        XP3Header;


	static BYTE Bom[]    = { 0xFF, 0xFE };
	static BYTE Header[] = { 0x58, 0x50, 0x33, 0x0D, 0x0A, 0x20, 0x0A, 0x1A, 0x8B, 0x67, 0x01 };

	RtlCopyMemory(XP3Header.Magic, Header, sizeof(Header));
	XP3Header.IndexOffset.QuadPart = 0;

	RtlZeroMemory(ScriptContent, sizeof(ScriptContent));
	if (Is32Bit) {
		wnsprintfW(ScriptContent, countof(ScriptContent), Startup, ExeName);
	}
	else {
		wnsprintfW(ScriptContent, countof(ScriptContent), Startup64, ExeName);
	}
	Size.QuadPart = lstrlenW(ScriptContent) * sizeof(WCHAR) + sizeof(Bom);

	Offset.QuadPart = 0;
	FileOffset.QuadPart = 0;
	Status = File.Seek(Offset, FILE_END, &FileOffset);
	if (NT_FAILED(Status)) {
		return Status;
	}

	Status = File.Write(&XP3Header, sizeof(XP3Header), &Offset);
	if (NT_FAILED(Status)) {
		return Status;
	}

	if (Offset.QuadPart <= FileOffset.QuadPart) {
		return STATUS_DATA_ERROR;
	}
	Offset.QuadPart -= FileOffset.QuadPart;

	RtlZeroMemory(&Index, sizeof(Index));
	Index.file.Magic = CHUNK_MAGIC_FILE;
	Index.info.Magic = CHUNK_MAGIC_INFO;
	Index.time.Magic = CHUNK_MAGIC_TIME;
	Index.segm.Magic = CHUNK_MAGIC_SEGM;
	Index.adlr.Magic = CHUNK_MAGIC_ADLR;
	Index.segm.ChunkSize.QuadPart = (ULONG64)0x1C; //sizeof(pIndex->segm.segm);
	Index.adlr.ChunkSize.QuadPart = (ULONG64)0x04;
	Index.info.ChunkSize.QuadPart = (ULONG64)0x58;
	Index.file.ChunkSize.QuadPart = (ULONG64)0xB0;
	Index.time.ChunkSize.QuadPart = (ULONG64)0x08;

	static const WCHAR FileName[] = L"startup.tjs";

	Index.segm.segm->Offset = Offset;
	lstrcpyW(Index.info.FileName, FileName);
	Index.info.FileNameLength = lstrlenW(FileName);

	BytesTransfered = Size.LowPart;

	Index.adlr.Hash = 0;
	Index.adlr.Hash = adler32(1/*adler32(0, 0, 0)*/, (Bytef*)ScriptContent, BytesTransfered);

	Index.segm.segm->OriginalSize.LowPart = BytesTransfered;
	Index.info.OriginalSize.LowPart = BytesTransfered;
	Index.info.EncryptedFlag = 0x00000000;

	ULONG FileNameSize = (lstrlenW(Index.info.FileName) + 1) * 2;

	Index.file.ChunkSize.QuadPart =
		sizeof(Index.time) +
		sizeof(Index.segm) +
		sizeof(Index.adlr) +
		FIELD_OFFSET(KRKR2_XP3_INDEX_CHUNK_INFO, FileName) +
		FileNameSize;


	Index.segm.segm->ArchiveSize.LowPart = BytesTransfered;
	Index.info.ArchiveSize.LowPart = BytesTransfered;
	Offset.QuadPart += BytesTransfered;

	Size.QuadPart = BytesTransfered;
	Status = File.Write((PVOID)ScriptContent, BytesTransfered, &Offset);
	if (NT_FAILED(Status)) {
		return Status;
	}

	Offset.QuadPart -= FileOffset.QuadPart;
	XP3Header.IndexOffset = Offset;

	// generate index, calculate index size first
	Size.LowPart = 0;
	Size.LowPart +=
		sizeof(Index.time) +
		sizeof(Index.adlr) +
		sizeof(Index.segm) +
		sizeof(Index.file) +
		FIELD_OFFSET(KRKR2_XP3_INDEX_CHUNK_INFO, FileName) +
		FileNameSize;

	auto Buffer = AllocateMemorySafe<BYTE>(Size.LowPart);
	auto CompressedBuffer = AllocateMemorySafe<BYTE>(compressBound(Size.LowPart));

	if (!Buffer || !CompressedBuffer) {
		return STATUS_NO_MEMORY;
	}

	// generate index to lpCompressBuffer
	pbIndex = (PBYTE)Buffer.get();

	RtlCopyMemory(pbIndex, &Index.file, sizeof(Index.file));
	pbIndex += sizeof(Index.file);
	RtlCopyMemory(pbIndex, &Index.time, sizeof(Index.time));
	pbIndex += sizeof(Index.time);
	RtlCopyMemory(pbIndex, &Index.adlr, sizeof(Index.adlr));
	pbIndex += sizeof(Index.adlr);
	RtlCopyMemory(pbIndex, &Index.segm, sizeof(Index.segm));
	pbIndex += sizeof(Index.segm);
	RtlCopyMemory(pbIndex, &Index.info, FIELD_OFFSET(KRKR2_XP3_INDEX_CHUNK_INFO, FileName));
	pbIndex += FIELD_OFFSET(KRKR2_XP3_INDEX_CHUNK_INFO, FileName);

	
	RtlCopyMemory(pbIndex, Index.info.FileName, FileNameSize);
	pbIndex += FileNameSize;

	IndexHeader.bZlib = 1;
	IndexHeader.OriginalSize.QuadPart = Size.QuadPart;
	IndexHeader.ArchiveSize.QuadPart = 0;
	compress2(Buffer.get(), &IndexHeader.ArchiveSize.LowPart, CompressedBuffer.get(), Size.LowPart, Z_BEST_COMPRESSION);
	
	Status = File.Write(&IndexHeader, sizeof(IndexHeader));
	if (Status) {
		return Status;
	}

	Status = File.Write(CompressedBuffer.get(), IndexHeader.ArchiveSize.LowPart);
	if (Status) {
		return Status;
	}

	Status = File.Seek(FileOffset, FILE_BEGIN);
	if (Status) {
		return Status;
	}

	Status = File.Write(&XP3Header, sizeof(XP3Header));
	if (Status) {
		return Status;
	}

	return STATUS_SUCCESS;
}



void CoUniversalPatchTask::ThreadFunction()
{
	NTSTATUS              Status;
	BOOL                  Success;
	NtFileDisk            File, Tvpwin32File;
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

	//
	// Plan (A) 反客为主
	// fuck anti-virus engines
	//
	LOOP_ONCE
	{
		Status = WriteOrPatchResource(
			this,
			m_Proxyer->GetHostModule(),
			IDR_EXE1,
			L"EXE",
			ReplaceFileNameExtension(std::wstring(m_Proxyer->GetModulePath()), L"_patch.exe"),
			WALK_RESOURCE_BUFFERM(Buffer, Size)
			{
				return STATUS_SUCCESS;
			}
		);

		if (NT_FAILED(Status)) {
			SetLastMessage(FALSE, L"Failed to extract tvpwin32.exe");
			break;
		}

		if (IsCopyIcon())
		{
			Status = CopyExeIcon(
				ReplaceFileNameExtension(
					std::wstring(m_Proxyer->GetModulePath()), L"_patch.exe").c_str(),
				512
			);

			if (NT_SUCCESS(Status)) {
				SetLastMessage(FALSE, L"Failed to apply icon");
				break;
			}
		}

		Status = Tvpwin32File.Append(ReplaceFileNameExtension(
			std::wstring(m_Proxyer->GetModulePath()), L"_patch.exe").c_str()
		);

		if (NT_FAILED(Status)) {
			Tvpwin32File.Close();
			SetLastMessage(FALSE, L"Failed to open tvpwin32.exe for writing payload.");
			break;
		}

		Status = CreateStartupArchiveAfterExe(
			Tvpwin32File, 
			sizeof(void*) == 32, 
			ReplaceFileNameExtension(
				std::wstring(m_Proxyer->GetModulePath()), L"_patch.exe").c_str()
		);

		if (NT_FAILED(Status)) {
			Tvpwin32File.Close();
			SetLastMessage(FALSE, L"Failed to write payload.");
			break;
		}
		Tvpwin32File.Close();
	}

	if (NT_FAILED(Status)) {

		Io::DeleteFileW(ReplaceFileNameExtension(
			std::wstring(m_Proxyer->GetModulePath()), L"_patch.exe").c_str()
		);

		//
		// failed due to space issue ?
		// checksum error --, like NSIS Error
		// Plan (B) 暗度陈仓
		//

		Success = AddSection(
			m_Proxyer->GetModulePath(),
			ReplaceFileNameExtension(std::wstring(m_Proxyer->GetModulePath()), L"_patch.exe").c_str(),
			".text0",
			L"KrkrUniversalPatch.dll"
		);

		if (!Success) {
			Status = STATUS_UNSUCCESSFUL;
			SetLastMessage(FALSE, L"Failed to create universal patch (plan B).");
		}
	}

	if (NT_SUCCESS(Status))
	{
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
	}

	if (NT_FAILED(Status)) {
		SetLastMessage(FALSE, L"Failed to write dll");

		Io::DeleteFileW(ReplaceFileNameExtension(
			std::wstring(m_Proxyer->GetModulePath()), L"_patch.exe").c_str()
		);

		return;
	}

	SetLastMessage(TRUE, L"Universal patch : Successful");
}




