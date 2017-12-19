#include <my.h>

#pragma comment(linker, "/ENTRY:MainEntry")
#pragma comment(lib, "MyLibrary_x64.lib")

typedef
BOOL
(WINAPI
*FuncCreateProcessInternalW)(
HANDLE                  hToken,
LPCWSTR                 lpApplicationName,
LPWSTR                  lpCommandLine,
LPSECURITY_ATTRIBUTES   lpProcessAttributes,
LPSECURITY_ATTRIBUTES   lpThreadAttributes,
BOOL                    bInheritHandles,
ULONG                   dwCreationFlags,
LPVOID                  lpEnvironment,
LPCWSTR                 lpCurrentDirectory,
LPSTARTUPINFOW          lpStartupInfo,
LPPROCESS_INFORMATION   lpProcessInformation,
PHANDLE                 phNewToken
);

BOOL
(WINAPI
*StubCreateProcessInternalW)(
HANDLE                  hToken,
LPCWSTR                 lpApplicationName,
LPWSTR                  lpCommandLine,
LPSECURITY_ATTRIBUTES   lpProcessAttributes,
LPSECURITY_ATTRIBUTES   lpThreadAttributes,
BOOL                    bInheritHandles,
ULONG                   dwCreationFlags,
LPVOID                  lpEnvironment,
LPCWSTR                 lpCurrentDirectory,
LPSTARTUPINFOW          lpStartupInfo,
LPPROCESS_INFORMATION   lpProcessInformation,
PHANDLE                 phNewToken
);


Void InjectDllToRemoteProcess64(HANDLE ProcessHandle, HANDLE ThreadHandle, LPCWSTR DllPath)
{
	CONTEXT context;
	DWORD   len;
	LPVOID  mem;
	DWORD64 LLW;
	
	union
	{
		PBYTE pB;
		PDWORD64 pL;
	} ip;

#define CODESIZE 92
	static BYTE code[CODESIZE + 2 * (MAX_PATH)] = 
	{
		0, 0, 0, 0, 0, 0, 0, 0,	// original rip
		0, 0, 0, 0, 0, 0, 0, 0,	// LoadLibraryW
		0x9C,				// pushfq
		0x50,				// push  rax
		0x51,				// push  rcx
		0x52,				// push  rdx
		0x53,				// push  rbx
		0x55,				// push  rbp
		0x56,				// push  rsi
		0x57,				// push  rdi
		0x41, 0x50,			// push  r8
		0x41, 0x51,			// push  r9
		0x41, 0x52,			// push  r10
		0x41, 0x53,			// push  r11
		0x41, 0x54,			// push  r12
		0x41, 0x55,			// push  r13
		0x41, 0x56,			// push  r14
		0x41, 0x57,			// push  r15
		0x48, 0x83, 0xEC, 0x28,		// sub   rsp, 40
		0x48, 0x8D, 0x0D, 41, 0, 0, 0,	// lea   ecx, L"path\to\procrewriter.x64.dll"
		0xFF, 0x15, -49, -1, -1, -1,		// call  LoadLibraryW
		0x48, 0x83, 0xC4, 0x28,		// add   rsp, 40
		0x41, 0x5F,			// pop r15
		0x41, 0x5E,			// pop r14
		0x41, 0x5D,			// pop r13
		0x41, 0x5C,			// pop r12
		0x41, 0x5B,			// pop r11
		0x41, 0x5A,			// pop r10
		0x41, 0x59,			// pop r9
		0x41, 0x58,			// pop r8
		0x5F,				// pop rdi
		0x5E,				// pop rsi
		0x5D,				// pop rbp
		0x5B,				// pop rbx
		0x5A,				// pop rdx
		0x59,				// pop rcx
		0x58,				// pop rax
		0x9D,				// popfq
		0xFF, 0x25, -91, -1, -1, -1,    // jmp   original Rip
		0x0,			   // DWORD alignment for LLW, fwiw
	};

	len = 2 * (StrLengthW(DllPath) + 1);
	if (len > 2 * (MAX_PATH))
		return;

	RtlCopyMemory(code + CODESIZE, DllPath, len);
	len += CODESIZE;

	context.ContextFlags = CONTEXT_CONTROL;
	GetThreadContext(ThreadHandle, &context);
	mem = VirtualAllocEx(ProcessHandle, NULL, len, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	LLW = (DWORD64)LoadLibraryW;

	ip.pB = code;
	*ip.pL++ = context.Rip;
	*ip.pL++ = LLW;

	WriteProcessMemory(ProcessHandle, mem, code, len, NULL);
	FlushInstructionCache(ProcessHandle, mem, len);
	context.Rip = (DWORD64)mem + 16;
	SetThreadContext(ThreadHandle, &context);
}


BOOL
WINAPI
VMeCreateProcess(
HANDLE                  hToken,
LPCWSTR                 lpApplicationName,
LPWSTR                  lpCommandLine,
LPCWSTR                 lpDllPath,
LPSECURITY_ATTRIBUTES   lpProcessAttributes,
LPSECURITY_ATTRIBUTES   lpThreadAttributes,
BOOL                    bInheritHandles,
ULONG                   dwCreationFlags,
LPVOID                  lpEnvironment,
LPCWSTR                 lpCurrentDirectory,
LPSTARTUPINFOW          lpStartupInfo,
LPPROCESS_INFORMATION   lpProcessInformation,
PHANDLE                 phNewToken
)
{
	BOOL             Result, IsSuspended;

	StubCreateProcessInternalW = (FuncCreateProcessInternalW)EATLookupRoutineByHashPNoFix(GetKernel32Handle(), KERNEL32_CreateProcessInternalW);

	IsSuspended = !!(dwCreationFlags & CREATE_SUSPENDED);
	dwCreationFlags |= CREATE_SUSPENDED;
	Result = StubCreateProcessInternalW(
		hToken,
		lpApplicationName,
		lpCommandLine,
		lpProcessAttributes,
		lpThreadAttributes,
		bInheritHandles,
		dwCreationFlags,
		lpEnvironment,
		lpCurrentDirectory,
		lpStartupInfo,
		lpProcessInformation,
		phNewToken);

	if (!Result)
		return Result;

	InjectDllToRemoteProcess64(
		lpProcessInformation->hProcess,
		lpProcessInformation->hThread,
		lpDllPath);

	ResumeThread(lpProcessInformation->hThread);

	return TRUE;
}



BOOL FASTCALL CheckAndCreateProcess(LPCWSTR ProcessName)
{
	STARTUPINFOW        si;
	PROCESS_INFORMATION pi;

	RtlZeroMemory(&si, sizeof(si));
	RtlZeroMemory(&pi, sizeof(pi));
	si.cb = sizeof(si);

	return VMeCreateProcess(NULL, ProcessName, NULL, L"KrkrHelper.dll", NULL, NULL, FALSE, NULL, NULL, NULL, &si, &pi, NULL);
}


BOOL FASTCALL CheckAndCreateProcessInit(LPCWSTR ProcessName)
{
	STARTUPINFOW        si;
	PROCESS_INFORMATION pi;

	RtlZeroMemory(&si, sizeof(si));
	RtlZeroMemory(&pi, sizeof(pi));
	si.cb = sizeof(si);

	return VMeCreateProcess(NULL, ProcessName, NULL, L"KrkrExtract64.dll", NULL, NULL, FALSE, NULL, NULL, NULL, &si, &pi, NULL);
}


VOID CDECL MainEntry()
{
	BOOL     Success;
	LPWSTR*  Args;
	INT      Argc;

	ml::MlInitialize();
	Args = CommandLineToArgvW(::GetCommandLineW(), &Argc);
	if (Argc != 3)
	{
		ReleaseArgv(Args);
		Ps::ExitProcess(0);
	}
	
	if (Args[1][0] == L'>' && Args[1][1] == L'<')
		Success = CheckAndCreateProcess(Args[2]);
	else if (Args[1][0] == L'&')
		Success = CheckAndCreateProcessInit(Args[2]); //x64 native
	else
		Success = 0;
	
	LocalFree(Args);
	Ps::ExitProcess(Success);
}

