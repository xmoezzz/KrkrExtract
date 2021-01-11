#include <my.h>
#include <vector>

#pragma pack(push, 1)
typedef struct SHELL_DATA
{
	ULONG64      Maigc1;
	ULONG64      Magic2;
	WCHAR        FileName[MAX_PATH];
}SHELL_DATA, *PSHELL_DATA;
#pragma pack(pop)

SHELL_DATA LinkerData
{
	(ULONG64)TAG8('Krkr', 'Info'),
	(ULONG64)TAG8('Xmoe', 'Anzu')
};


BOOL PrepareShellCode(HANDLE Process, HANDLE Thread, PCWSTR DllPath)
{
	BOOL              Status;
	PVOID             VirtualMemory;
	PVOID             DllNameBase;
	WOW64_CONTEXT     Context;
	SIZE_T            BytesTransferred;
	std::vector<BYTE> ShellCode;


	SuspendThread(Thread);

	RtlZeroMemory(&Context, sizeof(Context));
	Context.ContextFlags = CONTEXT_FULL;

	Status = Wow64GetThreadContext(Thread, &Context);
	if (!Status) {
		return FALSE;
	}

	VirtualMemory = VirtualAllocEx(Process, NULL, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (!VirtualMemory) {
		return FALSE;
	}

	if (StrLengthW(DllPath) > 0x240) {
		return FALSE;
	}

	DllNameBase = (PBYTE)VirtualMemory + 0x500;

	BytesTransferred = 0;
	Status = WriteProcessMemory(
		Process,
		DllNameBase,
		DllPath,
		(StrLengthW(DllPath) + 1) * 2,
		&BytesTransferred
	);

	if (!Status) {
		VirtualFreeEx(Process, VirtualMemory, 0x1000, MEM_RELEASE);
		return FALSE;
	}

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

		for (ULONG i = 0; i < countof(Bytes); i++) {
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
		for (ULONG i = 0; i < countof(Bytes); i++) {
			Inst.push_back(Bytes[i]);
		}

		return Inst;
	};

	auto&& MovInst = MovEax_XX(Context.Eip);
	SIN2(MovInst); // mov eax, Context.Eip
	SIN(0x50); // push eax
	SIN(0x9C); // pushfd
	SIN(0x60); // pushad
	SIN(0xE8, 0x05, 0x00, 0x00, 0x00); // call eip + 0x10
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
	SIN(0x68); // push DllName.vaddr <<< (push imm)

	auto&& Imm = Const_XX((DWORD)DllNameBase);
	SIN2(Imm); // imm 
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

	Status = WriteProcessMemory(
		Process,
		VirtualMemory,
		ShellCode.data(),
		ShellCode.size(),
		&BytesTransferred
	);

	if (!Status) {
		return FALSE;
	}

	Context.Eip = (DWORD)VirtualMemory;
	Wow64SetThreadContext(Thread, &Context);
	return TRUE;
}



BOOL
CreateProcessWithDllW_ANY_TO_I386(
	LPCWSTR lpApplicationName,
	LPWSTR lpCommandLine,
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL bInheritHandles,
	DWORD dwCreationFlags,
	LPVOID lpEnvironment,
	LPCWSTR lpCurrentDirectory,
	LPSTARTUPINFOW lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation,
	LPCWSTR lpDllFullPath
)
{
	BOOL  Status;
	DWORD CreaFlags;

	CreaFlags = dwCreationFlags | CREATE_SUSPENDED;

	Status = CreateProcessW(
		lpApplicationName,
		lpCommandLine,
		lpProcessAttributes,
		lpThreadAttributes,
		bInheritHandles,
		CreaFlags,
		lpEnvironment,
		lpCurrentDirectory,
		lpStartupInfo,
		lpProcessInformation
	);

	if (!Status) {
		return FALSE;
	}

	Status = PrepareShellCode(
		lpProcessInformation->hProcess,
		lpProcessInformation->hThread,
		lpDllFullPath
	);

	if (!Status) {
		TerminateProcess(lpProcessInformation->hProcess, -1);
		CloseHandle(lpProcessInformation->hProcess);
		CloseHandle(lpProcessInformation->hThread);
		return FALSE;
	}

	if (!(dwCreationFlags & CREATE_SUSPENDED)) {
		ResumeThread(lpProcessInformation->hThread);
	}

	CloseHandle(lpProcessInformation->hProcess);
	CloseHandle(lpProcessInformation->hThread);
	return TRUE;
}


BOOL FASTCALL CheckAndCreateProcess()
{
	STARTUPINFOW        si;
	PROCESS_INFORMATION pi;

	RtlZeroMemory(&si, sizeof(si));
	RtlZeroMemory(&pi, sizeof(pi));
	si.cb = sizeof(si);

	return CreateProcessWithDllW_ANY_TO_I386(
		NULL, 
		LinkerData.FileName, 
		NULL, 
		NULL, 
		FALSE, 
		NULL, 
		NULL, 
		NULL, 
		&si, 
		&pi, 
		L"KrkrUniversalPatch.dll"
	);
}


int WINAPI wWinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPWSTR    lpCmdLine,
	int       nShowCmd)
{
	PWSTR*              Argv;
	INT                 Argc;
	STARTUPINFOW        si;
	PROCESS_INFORMATION pi;
	ULONG               Length;

	RtlZeroMemory(&si, sizeof(si));
	RtlZeroMemory(&pi, sizeof(pi));
	si.cb = sizeof(si);

	Argv = CommandLineToArgvW(lpCmdLine, &Argc);
	if (Argv == NULL || Argc < 1)
	{
		LocalFree(Argv);
		return 0;
	}

	if (Nt_GetFileAttributes(L"KrkrUniversalPatch.dll") == (DWORD)-1) {
		MessageBoxW(NULL, L"Couldn't find KrkrUniversalPatch.dll", L"Krkr Universal Patch", MB_OK | MB_ICONERROR);
		LocalFree(Argv);
		return 0;
	}

	if (!CheckAndCreateProcess()) {
		MessageBoxW(NULL, L"Couldn't Launch Game", L"Krkr Universal Patch", MB_OK | MB_ICONERROR);
	}

	LocalFree(Argv);
	return 0;
}

