#include "UniversalShellCode.h"
#include <vector>

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





#ifndef CONTEXT_AMD64
#define CONTEXT_AMD64 0x100000
#endif

#define CONTEXT64_CONTROL (CONTEXT_AMD64 | 0x1L)
#define CONTEXT64_INTEGER (CONTEXT_AMD64 | 0x2L)
#define CONTEXT64_SEGMENTS (CONTEXT_AMD64 | 0x4L)
#define CONTEXT64_FLOATING_POINT  (CONTEXT_AMD64 | 0x8L)
#define CONTEXT64_DEBUG_REGISTERS (CONTEXT_AMD64 | 0x10L)
#define CONTEXT64_FULL (CONTEXT64_CONTROL | CONTEXT64_INTEGER | CONTEXT64_FLOATING_POINT)
#define CONTEXT64_ALL (CONTEXT64_CONTROL | CONTEXT64_INTEGER | CONTEXT64_SEGMENTS | CONTEXT64_FLOATING_POINT | CONTEXT64_DEBUG_REGISTERS)
#define CONTEXT64_XSTATE (CONTEXT_AMD64 | 0x20L)


#define EMIT(a) __asm __emit (a)

#define X64_Start_with_CS(_cs) \
    { \
    EMIT(0x6A) EMIT(_cs)                         /*  push   _cs             */ \
    EMIT(0xE8) EMIT(0) EMIT(0) EMIT(0) EMIT(0)   /*  call   $+5             */ \
    EMIT(0x83) EMIT(4) EMIT(0x24) EMIT(5)        /*  add    dword [esp], 5  */ \
    EMIT(0xCB)                                   /*  retf                   */ \
    }

#define X64_End_with_CS(_cs) \
    { \
    EMIT(0xE8) EMIT(0) EMIT(0) EMIT(0) EMIT(0)                                 /*  call   $+5                   */ \
    EMIT(0xC7) EMIT(0x44) EMIT(0x24) EMIT(4) EMIT(_cs) EMIT(0) EMIT(0) EMIT(0) /*  mov    dword [rsp + 4], _cs  */ \
    EMIT(0x83) EMIT(4) EMIT(0x24) EMIT(0xD)                                    /*  add    dword [rsp], 0xD      */ \
    EMIT(0xCB)                                                                 /*  retf                         */ \
    }

#define X64_Start() X64_Start_with_CS(0x33)
#define X64_End() X64_End_with_CS(0x23)

#define _RAX  0
#define _RCX  1
#define _RDX  2
#define _RBX  3
#define _RSP  4
#define _RBP  5
#define _RSI  6
#define _RDI  7
#define _R8   8
#define _R9   9
#define _R10 10
#define _R11 11
#define _R12 12
#define _R13 13
#define _R14 14
#define _R15 15

#define X64_Push(r) EMIT(0x48 | ((r) >> 3)) EMIT(0x50 | ((r) & 7))
#define X64_Pop(r) EMIT(0x48 | ((r) >> 3)) EMIT(0x58 | ((r) & 7))

#define REX_W EMIT(0x48) __asm


#pragma pack(push)
#pragma pack(1)
template <class T>
struct _LIST_ENTRY_T
{
	T Flink;
	T Blink;
};

template <class T>
struct _UNICODE_STRING_T
{
	union
	{
		struct
		{
			WORD Length;
			WORD MaximumLength;
		};
		T dummy;
	};
	T Buffer;
};

template <class T>
struct _NT_TIB_T
{
	T ExceptionList;
	T StackBase;
	T StackLimit;
	T SubSystemTib;
	T FiberData;
	T ArbitraryUserPointer;
	T Self;
};

template <class T>
struct _CLIENT_ID_T
{
	T UniqueProcess;
	T UniqueThread;
};

template <class T>
struct _TEB_T_
{
	_NT_TIB_T<T> NtTib;
	T EnvironmentPointer;
	_CLIENT_ID_T<T> ClientId;
	T ActiveRpcHandle;
	T ThreadLocalStoragePointer;
	T ProcessEnvironmentBlock;
	DWORD LastErrorValue;
	DWORD CountOfOwnedCriticalSections;
	T CsrClientThread;
	T Win32ThreadInfo;
	DWORD User32Reserved[26];
	//rest of the structure is not defined for now, as it is not needed
};

template <class T>
struct _LDR_DATA_TABLE_ENTRY_T
{
	_LIST_ENTRY_T<T> InLoadOrderLinks;
	_LIST_ENTRY_T<T> InMemoryOrderLinks;
	_LIST_ENTRY_T<T> InInitializationOrderLinks;
	T DllBase;
	T EntryPoint;
	union
	{
		DWORD SizeOfImage;
		T dummy01;
	};
	_UNICODE_STRING_T<T> FullDllName;
	_UNICODE_STRING_T<T> BaseDllName;
	DWORD Flags;
	WORD LoadCount;
	WORD TlsIndex;
	union
	{
		_LIST_ENTRY_T<T> HashLinks;
		struct
		{
			T SectionPointer;
			T CheckSum;
		};
	};
	union
	{
		T LoadedImports;
		DWORD TimeDateStamp;
	};
	T EntryPointActivationContext;
	T PatchInformation;
	_LIST_ENTRY_T<T> ForwarderLinks;
	_LIST_ENTRY_T<T> ServiceTagLinks;
	_LIST_ENTRY_T<T> StaticLinks;
	T ContextInformation;
	T OriginalBase;
	_LARGE_INTEGER LoadTime;
};

template <class T>
struct _PEB_LDR_DATA_T
{
	DWORD Length;
	DWORD Initialized;
	T SsHandle;
	_LIST_ENTRY_T<T> InLoadOrderModuleList;
	_LIST_ENTRY_T<T> InMemoryOrderModuleList;
	_LIST_ENTRY_T<T> InInitializationOrderModuleList;
	T EntryInProgress;
	DWORD ShutdownInProgress;
	T ShutdownThreadId;

};

template <class T, class NGF, int A>
struct _PEB_T
{
	union
	{
		struct
		{
			BYTE InheritedAddressSpace;
			BYTE ReadImageFileExecOptions;
			BYTE BeingDebugged;
			BYTE BitField;
		};
		T dummy01;
	};
	T Mutant;
	T ImageBaseAddress;
	T Ldr;
	T ProcessParameters;
	T SubSystemData;
	T ProcessHeap;
	T FastPebLock;
	T AtlThunkSListPtr;
	T IFEOKey;
	T CrossProcessFlags;
	T UserSharedInfoPtr;
	DWORD SystemReserved;
	DWORD AtlThunkSListPtr32;
	T ApiSetMap;
	T TlsExpansionCounter;
	T TlsBitmap;
	DWORD TlsBitmapBits[2];
	T ReadOnlySharedMemoryBase;
	T HotpatchInformation;
	T ReadOnlyStaticServerData;
	T AnsiCodePageData;
	T OemCodePageData;
	T UnicodeCaseTableData;
	DWORD NumberOfProcessors;
	union
	{
		DWORD NtGlobalFlag;
		NGF dummy02;
	};
	LARGE_INTEGER CriticalSectionTimeout;
	T HeapSegmentReserve;
	T HeapSegmentCommit;
	T HeapDeCommitTotalFreeThreshold;
	T HeapDeCommitFreeBlockThreshold;
	DWORD NumberOfHeaps;
	DWORD MaximumNumberOfHeaps;
	T ProcessHeaps;
	T GdiSharedHandleTable;
	T ProcessStarterHelper;
	T GdiDCAttributeList;
	T LoaderLock;
	DWORD OSMajorVersion;
	DWORD OSMinorVersion;
	WORD OSBuildNumber;
	WORD OSCSDVersion;
	DWORD OSPlatformId;
	DWORD ImageSubsystem;
	DWORD ImageSubsystemMajorVersion;
	T ImageSubsystemMinorVersion;
	T ActiveProcessAffinityMask;
	T GdiHandleBuffer[A];
	T PostProcessInitRoutine;
	T TlsExpansionBitmap;
	DWORD TlsExpansionBitmapBits[32];
	T SessionId;
	ULARGE_INTEGER AppCompatFlags;
	ULARGE_INTEGER AppCompatFlagsUser;
	T pShimData;
	T AppCompatInfo;
	_UNICODE_STRING_T<T> CSDVersion;
	T ActivationContextData;
	T ProcessAssemblyStorageMap;
	T SystemDefaultActivationContextData;
	T SystemAssemblyStorageMap;
	T MinimumStackCommit;
	T FlsCallback;
	_LIST_ENTRY_T<T> FlsListHead;
	T FlsBitmap;
	DWORD FlsBitmapBits[4];
	T FlsHighIndex;
	T WerRegistrationData;
	T WerShipAssertPtr;
	T pContextData;
	T pImageHeaderHash;
	T TracingFlags;
};

typedef _LDR_DATA_TABLE_ENTRY_T<DWORD64> LDR_DATA_TABLE_ENTRY64;
typedef _TEB_T_<DWORD64> TEB64;
typedef _PEB_LDR_DATA_T<DWORD64> PEB_LDR_DATA64;
typedef _PEB_T<DWORD64, DWORD, 30> PEB64;

struct _XSAVE_FORMAT64
{
	WORD ControlWord;
	WORD StatusWord;
	BYTE TagWord;
	BYTE Reserved1;
	WORD ErrorOpcode;
	DWORD ErrorOffset;
	WORD ErrorSelector;
	WORD Reserved2;
	DWORD DataOffset;
	WORD DataSelector;
	WORD Reserved3;
	DWORD MxCsr;
	DWORD MxCsr_Mask;
	_M128A FloatRegisters[8];
	_M128A XmmRegisters[16];
	BYTE Reserved4[96];
};

struct _CONTEXT64
{
	DWORD64 P1Home;
	DWORD64 P2Home;
	DWORD64 P3Home;
	DWORD64 P4Home;
	DWORD64 P5Home;
	DWORD64 P6Home;
	DWORD ContextFlags;
	DWORD MxCsr;
	WORD SegCs;
	WORD SegDs;
	WORD SegEs;
	WORD SegFs;
	WORD SegGs;
	WORD SegSs;
	DWORD EFlags;
	DWORD64 Dr0;
	DWORD64 Dr1;
	DWORD64 Dr2;
	DWORD64 Dr3;
	DWORD64 Dr6;
	DWORD64 Dr7;
	DWORD64 Rax;
	DWORD64 Rcx;
	DWORD64 Rdx;
	DWORD64 Rbx;
	DWORD64 Rsp;
	DWORD64 Rbp;
	DWORD64 Rsi;
	DWORD64 Rdi;
	DWORD64 R8;
	DWORD64 R9;
	DWORD64 R10;
	DWORD64 R11;
	DWORD64 R12;
	DWORD64 R13;
	DWORD64 R14;
	DWORD64 R15;
	DWORD64 Rip;
	_XSAVE_FORMAT64 FltSave;
	_M128A Header[2];
	_M128A Legacy[8];
	_M128A Xmm0;
	_M128A Xmm1;
	_M128A Xmm2;
	_M128A Xmm3;
	_M128A Xmm4;
	_M128A Xmm5;
	_M128A Xmm6;
	_M128A Xmm7;
	_M128A Xmm8;
	_M128A Xmm9;
	_M128A Xmm10;
	_M128A Xmm11;
	_M128A Xmm12;
	_M128A Xmm13;
	_M128A Xmm14;
	_M128A Xmm15;
	_M128A VectorRegister[26];
	DWORD64 VectorControl;
	DWORD64 DebugControl;
	DWORD64 LastBranchToRip;
	DWORD64 LastBranchFromRip;
	DWORD64 LastExceptionToRip;
	DWORD64 LastExceptionFromRip;
};

#pragma pack(pop)


class CMemPtr
{
private:
	void** m_ptr;
	bool watchActive;

public:
	CMemPtr(void** ptr) : m_ptr(ptr), watchActive(true) {}

	~CMemPtr()
	{
		if (*m_ptr && watchActive)
		{
			free(*m_ptr);
			*m_ptr = 0;
		}
	}

	void disableWatch() { watchActive = false; }
};

#define WATCH(ptr) \
    CMemPtr watch_##ptr((void**)&ptr)

#define DISABLE_WATCH(ptr) \
    watch_##ptr.disableWatch()


union reg64
{
	DWORD64 v;
	DWORD dw[2];
};

#if defined(ML_I386)

#pragma message("x86_32 version")

#pragma warning(push)
#pragma warning(disable : 4409)
DWORD64 __cdecl X64Call(DWORD64 func, int argC, ...)
{
	va_list args;
	va_start(args, argC);
	reg64 _rcx = { (argC > 0) ? argC--, va_arg(args, DWORD64) : 0 };
	reg64 _rdx = { (argC > 0) ? argC--, va_arg(args, DWORD64) : 0 };
	reg64 _r8 = { (argC > 0) ? argC--, va_arg(args, DWORD64) : 0 };
	reg64 _r9 = { (argC > 0) ? argC--, va_arg(args, DWORD64) : 0 };
	reg64 _rax = { 0 };

	reg64 restArgs = { (DWORD64)&va_arg(args, DWORD64) };

	// conversion to QWORD for easier use in inline assembly
	reg64 _argC = { (DWORD64)argC };
	DWORD back_esp = 0;
	WORD back_fs = 0;

	__asm
	{
		;// reset FS segment, to properly handle RFG
		mov    back_fs, fs
			mov    eax, 0x2B
			mov    fs, ax

			;// keep original esp in back_esp variable
		mov    back_esp, esp

			;// align esp to 0x10, without aligned stack some syscalls may return errors !
		;// (actually, for syscalls it is sufficient to align to 8, but SSE opcodes 
		;// requires 0x10 alignment), it will be further adjusted according to the
		;// number of arguments above 4
		and esp, 0xFFFFFFF0

			X64_Start();

		;// below code is compiled as x86 inline asm, but it is executed as x64 code
		;// that's why it need sometimes REX_W() macro, right column contains detailed
		;// transcription how it will be interpreted by CPU

		;// fill first four arguments
		REX_W mov    ecx, _rcx.dw[0];// mov     rcx, qword ptr [_rcx]
		REX_W mov    edx, _rdx.dw[0];// mov     rdx, qword ptr [_rdx]
		push   _r8.v;// push    qword ptr [_r8]
		X64_Pop(_R8); ;// pop     r8
		push   _r9.v;// push    qword ptr [_r9]
		X64_Pop(_R9); ;// pop     r9
		;//
		REX_W mov    eax, _argC.dw[0];// mov     rax, qword ptr [_argC]
		;// 
		;// final stack adjustment, according to the    ;//
		;// number of arguments above 4                 ;// 
		test   al, 1;// test    al, 1
		jnz    _no_adjust;// jnz     _no_adjust
		sub    esp, 8;// sub     rsp, 8
	_no_adjust:;//
		;// 
		push   edi;// push    rdi
		REX_W mov    edi, restArgs.dw[0];// mov     rdi, qword ptr [restArgs]
		;// 
		;// put rest of arguments on the stack          ;// 
		REX_W test   eax, eax;// test    rax, rax
		jz     _ls_e;// je      _ls_e
		REX_W lea    edi, dword ptr[edi + 8 * eax - 8];// lea     rdi, [rdi + rax*8 - 8]
		;// 
	_ls:;// 
		REX_W test   eax, eax;// test    rax, rax
		jz     _ls_e;// je      _ls_e
		push   dword ptr[edi];// push    qword ptr [rdi]
		REX_W sub    edi, 8;// sub     rdi, 8
		REX_W sub    eax, 1;// sub     rax, 1
		jmp    _ls;// jmp     _ls
	_ls_e:;// 
		;// 
		;// create stack space for spilling registers   ;// 
		REX_W sub    esp, 0x20;// sub     rsp, 20h
		;// 
		call   func;// call    qword ptr [func]
		;// 
		;// cleanup stack                               ;// 
		REX_W mov    ecx, _argC.dw[0];// mov     rcx, qword ptr [_argC]
		REX_W lea    esp, dword ptr[esp + 8 * ecx + 0x20];// lea     rsp, [rsp + rcx*8 + 20h]
		;// 
		pop    edi;// pop     rdi
		;// 
// set return value                             ;// 
		REX_W mov    _rax.dw[0], eax;// mov     qword ptr [_rax], rax

		X64_End();

		mov    ax, ds
			mov    ss, ax
			mov    esp, back_esp

			;// restore FS segment
		mov    ax, back_fs
			mov    fs, ax
	}
	return _rax.v;
}
#pragma warning(pop)

void getMem64(void* dstMem, DWORD64 srcMem, size_t sz)
{
	if ((nullptr == dstMem) || (0 == srcMem) || (0 == sz))
		return;

	reg64 _src = { srcMem };

	__asm
	{
		X64_Start();

		;// below code is compiled as x86 inline asm, but it is executed as x64 code
		;// that's why it need sometimes REX_W() macro, right column contains detailed
		;// transcription how it will be interpreted by CPU

		push   edi;// push     rdi
		push   esi;// push     rsi
		;//
		mov    edi, dstMem;// mov      edi, dword ptr [dstMem]        ; high part of RDI is zeroed
		REX_W mov    esi, _src.dw[0];// mov      rsi, qword ptr [_src]
		mov    ecx, sz;// mov      ecx, dword ptr [sz]            ; high part of RCX is zeroed
		;//
		mov    eax, ecx;// mov      eax, ecx
		and eax, 3;// and      eax, 3
		shr    ecx, 2;// shr      ecx, 2
		;//
		rep    movsd;// rep movs dword ptr [rdi], dword ptr [rsi]
		;//
		test   eax, eax;// test     eax, eax
		je     _move_0;// je       _move_0
		cmp    eax, 1;// cmp      eax, 1
		je     _move_1;// je       _move_1
		;//
		movsw;// movs     word ptr [rdi], word ptr [rsi]
		cmp    eax, 2;// cmp      eax, 2
		je     _move_0;// je       _move_0
		;//
	_move_1:;//
		movsb;// movs     byte ptr [rdi], byte ptr [rsi]
		;//
	_move_0:;//
		pop    esi;// pop      rsi
		pop    edi;// pop      rdi

		X64_End();
	}
}

bool cmpMem64(void* dstMem, DWORD64 srcMem, size_t sz)
{
	if ((nullptr == dstMem) || (0 == srcMem) || (0 == sz))
		return false;

	bool result = false;
	reg64 _src = { srcMem };
	__asm
	{
		X64_Start();

		;// below code is compiled as x86 inline asm, but it is executed as x64 code
		;// that's why it need sometimes REX_W() macro, right column contains detailed
		;// transcription how it will be interpreted by CPU

		push   edi;// push      rdi
		push   esi;// push      rsi
		;//           
		mov    edi, dstMem;// mov       edi, dword ptr [dstMem]       ; high part of RDI is zeroed
		REX_W mov    esi, _src.dw[0];// mov       rsi, qword ptr [_src]
		mov    ecx, sz;// mov       ecx, dword ptr [sz]           ; high part of RCX is zeroed
		;//           
		mov    eax, ecx;// mov       eax, ecx
		and eax, 3;// and       eax, 3
		shr    ecx, 2;// shr       ecx, 2
		;// 
		repe   cmpsd;// repe cmps dword ptr [rsi], dword ptr [rdi]
		jnz     _ret_false;// jnz       _ret_false
		;// 
		test   eax, eax;// test      eax, eax
		je     _move_0;// je        _move_0
		cmp    eax, 1;// cmp       eax, 1
		je     _move_1;// je        _move_1
		;// 
		cmpsw;// cmps      word ptr [rsi], word ptr [rdi]
		jnz     _ret_false;// jnz       _ret_false
		cmp    eax, 2;// cmp       eax, 2
		je     _move_0;// je        _move_0
		;// 
	_move_1:;// 
		cmpsb;// cmps      byte ptr [rsi], byte ptr [rdi]
		jnz     _ret_false;// jnz       _ret_false
		;// 
	_move_0:;// 
		mov    result, 1;// mov       byte ptr [result], 1
		;// 
	_ret_false:;// 
		pop    esi;// pop      rsi
		pop    edi;// pop      rdi

		X64_End();
	}

	return result;
}


DWORD64 getTEB64()
{
	reg64 reg;
	reg.v = 0;

	X64_Start();
	// R12 register should always contain pointer to TEB64 in WoW64 processes
	X64_Push(_R12);
	// below pop will pop QWORD from stack, as we're in x64 mode now
	__asm pop reg.dw[0]
		X64_End();

	return reg.v;
}


DWORD64 NTAPI GetModuleHandle64(PCWSTR lpModuleName)
{
	TEB64 teb64;
	getMem64(&teb64, getTEB64(), sizeof(TEB64));

	PEB64 peb64;
	getMem64(&peb64, teb64.ProcessEnvironmentBlock, sizeof(PEB64));
	PEB_LDR_DATA64 ldr;
	getMem64(&ldr, peb64.Ldr, sizeof(PEB_LDR_DATA64));

	DWORD64 LastEntry = peb64.Ldr + offsetof(PEB_LDR_DATA64, InLoadOrderModuleList);
	LDR_DATA_TABLE_ENTRY64 head;
	head.InLoadOrderLinks.Flink = ldr.InLoadOrderModuleList.Flink;
	do
	{
		getMem64(&head, head.InLoadOrderLinks.Flink, sizeof(LDR_DATA_TABLE_ENTRY64));

		wchar_t* tempBuf = (wchar_t*)malloc(head.BaseDllName.MaximumLength);
		if (nullptr == tempBuf)
			return 0;
		WATCH(tempBuf);
		getMem64(tempBuf, head.BaseDllName.Buffer, head.BaseDllName.MaximumLength);

		if (0 == _wcsicmp(lpModuleName, tempBuf))
			return head.DllBase;
	} while (head.InLoadOrderLinks.Flink != LastEntry);

	return 0;
}

DWORD64 getNTDLL64()
{
	static DWORD64 ntdll64 = 0;
	if (0 != ntdll64)
		return ntdll64;

	ntdll64 = GetModuleHandle64(L"ntdll.dll");
	return ntdll64;
}


DWORD64 getLdrGetProcedureAddress()
{
	DWORD64 modBase = getNTDLL64();
	if (0 == modBase)
		return 0;

	IMAGE_DOS_HEADER idh;
	getMem64(&idh, modBase, sizeof(idh));

	IMAGE_NT_HEADERS64 inh;
	getMem64(&inh, modBase + idh.e_lfanew, sizeof(IMAGE_NT_HEADERS64));

	IMAGE_DATA_DIRECTORY& idd = inh.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];

	if (0 == idd.VirtualAddress)
		return 0;

	IMAGE_EXPORT_DIRECTORY ied;
	getMem64(&ied, modBase + idd.VirtualAddress, sizeof(ied));

	DWORD* rvaTable = (DWORD*)malloc(sizeof(DWORD) * ied.NumberOfFunctions);
	if (nullptr == rvaTable)
		return 0;
	WATCH(rvaTable);
	getMem64(rvaTable, modBase + ied.AddressOfFunctions, sizeof(DWORD) * ied.NumberOfFunctions);

	WORD* ordTable = (WORD*)malloc(sizeof(WORD) * ied.NumberOfFunctions);
	if (nullptr == ordTable)
		return 0;
	WATCH(ordTable);
	getMem64(ordTable, modBase + ied.AddressOfNameOrdinals, sizeof(WORD) * ied.NumberOfFunctions);

	DWORD* nameTable = (DWORD*)malloc(sizeof(DWORD) * ied.NumberOfNames);
	if (nullptr == nameTable)
		return 0;
	WATCH(nameTable);
	getMem64(nameTable, modBase + ied.AddressOfNames, sizeof(DWORD) * ied.NumberOfNames);

	// lazy search, there is no need to use binsearch for just one function
	for (DWORD i = 0; i < ied.NumberOfFunctions; i++)
	{
		if (!cmpMem64((PVOID)"LdrGetProcedureAddress", modBase + nameTable[i], sizeof("LdrGetProcedureAddress")))
			continue;
		else
			return modBase + rvaTable[ordTable[i]];
	}
	return 0;
}

VOID NTAPI SetLastErrorFromX64Call(DWORD64 status)
{
	typedef ULONG(WINAPI* RtlNtStatusToDosError_t)(NTSTATUS Status);
	typedef ULONG(WINAPI* RtlSetLastWin32Error_t)(NTSTATUS Status);

	static RtlNtStatusToDosError_t RtlNtStatusToDosError = nullptr;
	static RtlSetLastWin32Error_t RtlSetLastWin32Error = nullptr;

	if ((nullptr == RtlNtStatusToDosError) || (nullptr == RtlSetLastWin32Error))
	{
		HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
		RtlNtStatusToDosError = (RtlNtStatusToDosError_t)GetProcAddress(ntdll, "RtlNtStatusToDosError");
		RtlSetLastWin32Error = (RtlSetLastWin32Error_t)GetProcAddress(ntdll, "RtlSetLastWin32Error");
	}

	if ((nullptr != RtlNtStatusToDosError) && (nullptr != RtlSetLastWin32Error))
	{
		RtlSetLastWin32Error(RtlNtStatusToDosError((DWORD)status));
	}
}



DWORD64 NTAPI GetProcAddress64(DWORD64 hModule, PCSTR funcName)
{
	static DWORD64 _LdrGetProcedureAddress = 0;
	if (0 == _LdrGetProcedureAddress)
	{
		_LdrGetProcedureAddress = getLdrGetProcedureAddress();
		if (0 == _LdrGetProcedureAddress)
			return 0;
	}

	_UNICODE_STRING_T<DWORD64> fName = { 0 };
	fName.Buffer = (DWORD64)funcName;
	fName.Length = (WORD)strlen(funcName);
	fName.MaximumLength = fName.Length + 1;
	DWORD64 funcRet = 0;
	X64Call(_LdrGetProcedureAddress, 4, (DWORD64)hModule, (DWORD64)&fName, (DWORD64)0, (DWORD64)&funcRet);
	return funcRet;
}


SIZE_T NTAPI VirtualQueryEx64(HANDLE hProcess, DWORD64 lpAddress, MEMORY_BASIC_INFORMATION64* lpBuffer, SIZE_T dwLength)
{
	static DWORD64 ntqvm = 0;
	if (0 == ntqvm)
	{
		ntqvm = GetProcAddress64(getNTDLL64(), "NtQueryVirtualMemory");
		if (0 == ntqvm)
			return 0;
	}
	DWORD64 ret = 0;
	DWORD64 status = X64Call(ntqvm, 6, (DWORD64)hProcess, lpAddress, (DWORD64)0, (DWORD64)lpBuffer, (DWORD64)dwLength, (DWORD64)&ret);
	if (STATUS_SUCCESS != status)
		SetLastErrorFromX64Call(status);
	return (SIZE_T)ret;
}


DWORD64 NTAPI VirtualAllocEx64(HANDLE hProcess, DWORD64 lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect)
{
	static DWORD64 ntavm = 0;
	if (0 == ntavm)
	{
		ntavm = GetProcAddress64(getNTDLL64(), "NtAllocateVirtualMemory");
		if (0 == ntavm)
			return 0;
	}

	DWORD64 tmpAddr = lpAddress;
	DWORD64 tmpSize = dwSize;
	DWORD64 ret = X64Call(ntavm, 6, (DWORD64)hProcess, (DWORD64)&tmpAddr, (DWORD64)0, (DWORD64)&tmpSize, (DWORD64)flAllocationType, (DWORD64)flProtect);

	if (STATUS_SUCCESS != ret) {
		SetLastErrorFromX64Call(ret);
		return FALSE;
	}

	return tmpAddr;
}



BOOL NTAPI VirtualFreeEx64(HANDLE hProcess, DWORD64 lpAddress, SIZE_T dwSize, DWORD dwFreeType)
{
	static DWORD64 ntfvm = 0;
	if (0 == ntfvm)
	{
		ntfvm = GetProcAddress64(getNTDLL64(), "NtFreeVirtualMemory");
		if (0 == ntfvm)
			return 0;
	}

	DWORD64 tmpAddr = lpAddress;
	DWORD64 tmpSize = dwSize;
	DWORD64 ret = X64Call(ntfvm, 4, (DWORD64)hProcess, (DWORD64)&tmpAddr, (DWORD64)&tmpSize, (DWORD64)dwFreeType);
	if (STATUS_SUCCESS != ret)
	{
		SetLastErrorFromX64Call(ret);
		return FALSE;
	}

	return TRUE;
}


BOOL NTAPI VirtualProtectEx64(HANDLE hProcess, DWORD64 lpAddress, SIZE_T dwSize, DWORD flNewProtect, DWORD* lpflOldProtect)
{
	static DWORD64 ntpvm = 0;
	if (0 == ntpvm)
	{
		ntpvm = GetProcAddress64(getNTDLL64(), "NtProtectVirtualMemory");
		if (0 == ntpvm)
			return 0;
	}

	DWORD64 tmpAddr = lpAddress;
	DWORD64 tmpSize = dwSize;
	DWORD64 ret = X64Call(ntpvm, 5, (DWORD64)hProcess, (DWORD64)&tmpAddr, (DWORD64)&tmpSize, (DWORD64)flNewProtect, (DWORD64)lpflOldProtect);
	if (STATUS_SUCCESS != ret)
	{
		SetLastErrorFromX64Call(ret);
		return FALSE;
	}

	return TRUE;
}


BOOL NTAPI ReadProcessMemory64(HANDLE hProcess, DWORD64 lpBaseAddress, LPVOID lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesRead)
{
	static DWORD64 nrvm = 0;
	if (0 == nrvm)
	{
		nrvm = GetProcAddress64(getNTDLL64(), "NtReadVirtualMemory");
		if (0 == nrvm)
			return 0;
	}
	DWORD64 numOfBytes = lpNumberOfBytesRead ? *lpNumberOfBytesRead : 0;
	DWORD64 ret = X64Call(nrvm, 5, (DWORD64)hProcess, lpBaseAddress, (DWORD64)lpBuffer, (DWORD64)nSize, (DWORD64)&numOfBytes);
	if (STATUS_SUCCESS != ret)
	{
		SetLastErrorFromX64Call(ret);
		return FALSE;
	}

	if (lpNumberOfBytesRead) {
		*lpNumberOfBytesRead = (SIZE_T)numOfBytes;
	}

	return TRUE;
}


BOOL NTAPI WriteProcessMemory64(HANDLE hProcess, DWORD64 lpBaseAddress, LPVOID lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesWritten)
{
	static DWORD64 nrvm = 0;
	if (0 == nrvm)
	{
		nrvm = GetProcAddress64(getNTDLL64(), "NtWriteVirtualMemory");
		if (0 == nrvm)
			return 0;
	}
	DWORD64 numOfBytes = lpNumberOfBytesWritten ? *lpNumberOfBytesWritten : 0;
	DWORD64 ret = X64Call(nrvm, 5, (DWORD64)hProcess, lpBaseAddress, (DWORD64)lpBuffer, (DWORD64)nSize, (DWORD64)&numOfBytes);
	if (STATUS_SUCCESS != ret)
	{
		SetLastErrorFromX64Call(ret);
		return FALSE;
	}

	if (lpNumberOfBytesWritten) {
		*lpNumberOfBytesWritten = (SIZE_T)numOfBytes;
	}

	return TRUE;
}


BOOL NTAPI GetThreadContext64(HANDLE hThread, _CONTEXT64* lpContext)
{
	static DWORD64 gtc = 0;
	if (0 == gtc)
	{
		gtc = GetProcAddress64(getNTDLL64(), "NtGetContextThread");
		if (0 == gtc)
			return 0;
	}
	DWORD64 ret = X64Call(gtc, 2, (DWORD64)hThread, (DWORD64)lpContext);
	if (STATUS_SUCCESS != ret)
	{
		SetLastErrorFromX64Call(ret);
		return FALSE;
	}

	return TRUE;
}


BOOL NTAPI SetThreadContext64(HANDLE hThread, _CONTEXT64* lpContext)
{
	static DWORD64 stc = 0;
	if (0 == stc)
	{
		stc = GetProcAddress64(getNTDLL64(), "NtSetContextThread");
		if (0 == stc)
			return 0;
	}
	DWORD64 ret = X64Call(stc, 2, (DWORD64)hThread, (DWORD64)lpContext);
	if (STATUS_SUCCESS != ret)
	{
		SetLastErrorFromX64Call(ret);
		return FALSE;
	}


	return TRUE;
}


#elif defined(ML_AMD64) || defined(__ARM64__)

#pragma message("x86_64 or arm64 version")

DWORD64 NTAPI GetModuleHandle64(PCWSTR lpModuleName)
{
	return (DWORD64)GetModuleHandleW(lpModuleName);
}


DWORD64 NTAPI getNTDLL64()
{
	static DWORD64 ntdll64 = 0;
	if (0 != ntdll64)
		return ntdll64;

	ntdll64 = GetModuleHandle64(L"ntdll.dll");
	return ntdll64;
}

DWORD64 getLdrGetProcedureAddress()
{
	return (DWORD64)LdrGetProcedureAddress;
}

VOID NTAPI SetLastErrorFromX64Call(DWORD64 status)
{
	SetLastError(status);
}

DWORD64 NTAPI GetProcAddress64(DWORD64 hModule, PCSTR funcName)
{
	return (DWORD64)GetProcAddress((HMODULE)hModule, funcName);
}

SIZE_T NTAPI VirtualQueryEx64(HANDLE hProcess, DWORD64 lpAddress, MEMORY_BASIC_INFORMATION64* lpBuffer, SIZE_T dwLength)
{
	SIZE_T                   Size;
	MEMORY_BASIC_INFORMATION Info;

	if (!lpBuffer) {
		return 0;
	}

	RtlZeroMemory(&Info, sizeof(Info));
	RtlZeroMemory(lpBuffer, sizeof(*lpBuffer));

	Size = VirtualQueryEx(hProcess, (PVOID)lpAddress, &Info, dwLength);
	lpBuffer->AllocationBase = (ULONGLONG)Info.AllocationBase;
	lpBuffer->AllocationProtect = Info.AllocationProtect;
	lpBuffer->BaseAddress = (ULONGLONG)Info.BaseAddress;
	lpBuffer->Protect = Info.Protect;
	lpBuffer->RegionSize = Info.RegionSize;
	lpBuffer->State = Info.State;
	lpBuffer->Type = Info.Type;

	return Size;
}


DWORD64 NTAPI VirtualAllocEx64(HANDLE hProcess, DWORD64 lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect)
{
	return (DWORD64)VirtualAllocEx(hProcess, (PVOID)lpAddress, dwSize, flAllocationType, flProtect);
}


BOOL NTAPI VirtualFreeEx64(HANDLE hProcess, DWORD64 lpAddress, SIZE_T dwSize, DWORD dwFreeType)
{
	return VirtualFreeEx(hProcess, (PVOID)lpAddress, dwSize, dwFreeType);
}


BOOL NTAPI VirtualProtectEx64(HANDLE hProcess, DWORD64 lpAddress, SIZE_T dwSize, DWORD flNewProtect, DWORD* lpflOldProtect)
{
	return VirtualProtectEx(hProcess, (PVOID)lpAddress, dwSize, flNewProtect, lpflOldProtect);
}


BOOL NTAPI ReadProcessMemory64(HANDLE hProcess, DWORD64 lpBaseAddress, LPVOID lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesRead)
{
	return ReadProcessMemory(hProcess, (PVOID)lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesRead);
}


BOOL NTAPI WriteProcessMemory64(HANDLE hProcess, DWORD64 lpBaseAddress, LPVOID lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesWritten)
{
	return WriteProcessMemory(hProcess, (PVOID)lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesWritten);
}


BOOL NTAPI GetThreadContext64(HANDLE hThread, _CONTEXT64* lpContext)
{
	return GetThreadContext(hThread, (PCONTEXT)lpContext);
}

BOOL NTAPI SetThreadContext64(HANDLE hThread, _CONTEXT64* lpContext)
{
	return SetThreadContext(hThread, (PCONTEXT)lpContext);
}

#endif



BOOL PrepareShellCode_AMD64(HANDLE Process, HANDLE Thread, PCWSTR DllPath)
{
	BOOL               Status;
	_CONTEXT64         Context;
	SIZE_T             BytesTransferred;
	DWORD64            VirtualMemory, DllNameBase;
	std::vector<BYTE>  ShellCode;


	SuspendThread(Thread);

	RtlZeroMemory(&Context, sizeof(Context));
	Context.ContextFlags = CONTEXT_FULL;

	Status = GetThreadContext64(Thread, &Context);
	if (!Status) {
		return FALSE;
	}

	VirtualMemory = VirtualAllocEx64(Process, NULL, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (!VirtualMemory) {
		return FALSE;
	}

	if (StrLengthW(DllPath) > 0x240) {
		return FALSE;
	}

	DllNameBase = (DWORD64)VirtualMemory + 0x500;

	BytesTransferred = 0;
	Status = WriteProcessMemory64(
		Process,
		DllNameBase,
		(PVOID)DllPath,
		(StrLengthW(DllPath) + 1) * 2,
		&BytesTransferred
	);

	if (!Status) {
		VirtualFreeEx64(Process, VirtualMemory, 0x1000, MEM_RELEASE);
		return FALSE;
	}


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

#define SIN(...) ShellCode.insert(ShellCode.end(), {__VA_ARGS__})
#define SIN2(x) ShellCode.insert(ShellCode.end(), x.begin(), x.end())

	SIN(0x48, 0xB8); //mov rax, imm

	auto&& Imm = MakeImm(Context.Rip);
	SIN2(Imm); // the imm
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

	SIN(0xE8, 0x1C, 0x00, 0x00, 0x00); // call 0x21 (offset : 0x1C)

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

	SIN(0x48, 0xB9); // mov rcx, imm (dll name)
	auto Imm2 = MakeImm(DllNameBase);
	SIN2(Imm2); // the imm
	SIN(0x48, 0x8B, 0x7C, 0x24, 0x30); //mov     rdi, [rsp+38h+var_8]
	SIN(0x48, 0x8B, 0x74, 0x24, 0x48); //mov     rsi, [rsp+38h+arg_8]
	SIN(0x48, 0x8B, 0x5C, 0x24, 0x40); //mov     rbx, [rsp+38h+arg_0]
	SIN(0x48, 0x83, 0xC4, 0x38); //add     rsp, 38h
	SIN(0x48, 0xFF, 0xE2); //jmp     rdx

#undef SIN
#undef SIN2

	Status = WriteProcessMemory64(
		Process,
		VirtualMemory,
		ShellCode.data(),
		ShellCode.size(),
		&BytesTransferred
	);

	if (!Status) {
		return FALSE;
	}

	Context.Rip = (DWORD64)VirtualMemory;
	SetThreadContext64(Thread, &Context);
	return TRUE;
}




BOOL RemoteProcessIs64Bits(PCWSTR FilePath, BOOL& Is64Bit)
{
	NTSTATUS          Status;
	NtFileDisk        File;
	PIMAGE_NT_HEADERS NtHeader;
	static BYTE Buffer[0x1000];

	Is64Bit = FALSE;
	Status = File.Open(FilePath);
	if (NT_FAILED(Status))
		return FALSE;
	
	Status = File.Read(Buffer, sizeof(Buffer));
	if (NT_FAILED(Status))
		return FALSE;

	NtHeader = PIMAGE_NT_HEADERS(PCHAR(Buffer) + PIMAGE_DOS_HEADER(Buffer)->e_lfanew);
	if (PBYTE(NtHeader) >= Buffer + sizeof(Buffer)) {
		PrintConsoleW(L"RemoteProcessIs64Bits : Buffer overflow\n");
		return FALSE;
	}

	switch (NtHeader->FileHeader.Machine)
	{
	case IMAGE_FILE_MACHINE_ALPHA64:
	case IMAGE_FILE_MACHINE_ARM64:
	case IMAGE_FILE_MACHINE_AMD64:
		Is64Bit = TRUE;
		break;
	}

	return TRUE;
}




BOOL WINAPI CreateProcessWithDllW_ANY_TO_I386(
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





BOOL WINAPI CreateProcessWithDllW_ANYX86_TO_AMD64(
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



