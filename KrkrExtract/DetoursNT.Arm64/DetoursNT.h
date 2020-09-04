#pragma once

//
// Workaround for VS2017 to force this library to
// be compilable on ARM.
//

#define _ARM_WINAPI_PARTITION_DESKTOP_SDK_AVAILABLE 1
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma region CRT

//
// __ImageBase is automatically provided by the linker.
//

extern IMAGE_DOS_HEADER __ImageBase;

#pragma intrinsic(strlen)

void* __cdecl
memset(
  void* dest,
  int ch,
  size_t count
  );

void* __cdecl
memcpy(
  void* dest,
  const void* src,
  size_t count
  );

// void* __cdecl
// malloc(
//   size_t size
//   );
//
// void* __cdecl
// calloc(
//   size_t count,
//   size_t size
//   );
//
// void* __cdecl
// realloc(
//   void* ptr,
//   size_t new_size
//   );
//
// void __cdecl
// free(
//   void* ptr
//   );

#pragma endregion

#pragma region Detours/module.cpp

ULONG WINAPI DetourGetModuleSize(_In_opt_ HMODULE hModule);

#pragma endregion

#pragma region NTDLL.DLL

#define NtCurrentProcess()        ((HANDLE)(LONG_PTR)-1)
#define ZwCurrentProcess()        NtCurrentProcess()
#define NtCurrentThread()         ((HANDLE)(LONG_PTR)-2)
#define ZwCurrentThread()         NtCurrentThread()

typedef enum _MEMORY_INFORMATION_CLASS
{
  MemoryBasicInformation,
  MemoryWorkingSetList,
  MemorySectionName,
  MemoryBasicVlmInformation,
  MemoryWorkingSetExList
} MEMORY_INFORMATION_CLASS;

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAllocateVirtualMemory(
  _In_ HANDLE ProcessHandle,
  _Inout_ _At_(*BaseAddress, _Readable_bytes_(*RegionSize) _Writable_bytes_(*RegionSize) _Post_readable_byte_size_(*RegionSize)) PVOID *BaseAddress,
  _In_ ULONG_PTR ZeroBits,
  _Inout_ PSIZE_T RegionSize,
  _In_ ULONG AllocationType,
  _In_ ULONG Protect
  );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtFreeVirtualMemory(
  _In_ HANDLE ProcessHandle,
  _Inout_ PVOID *BaseAddress,
  _Inout_ PSIZE_T RegionSize,
  _In_ ULONG FreeType
  );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtProtectVirtualMemory(
  _In_ HANDLE ProcessHandle,
  _Inout_ PVOID *BaseAddress,
  _Inout_ PSIZE_T RegionSize,
  _In_ ULONG NewProtect,
  _Out_ PULONG OldProtect
  );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryVirtualMemory(
  _In_ HANDLE ProcessHandle,
  _In_opt_ PVOID BaseAddress,
  _In_ MEMORY_INFORMATION_CLASS MemoryInformationClass,
  _Out_writes_bytes_(MemoryInformationLength) PVOID MemoryInformation,
  _In_ SIZE_T MemoryInformationLength,
  _Out_opt_ PSIZE_T ReturnLength
  );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtReadVirtualMemory(
  _In_ HANDLE ProcessHandle,
  _In_opt_ PVOID BaseAddress,
  _Out_writes_bytes_(BufferSize) PVOID Buffer,
  _In_ SIZE_T BufferSize,
  _Out_opt_ PSIZE_T NumberOfBytesRead
  );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtWriteVirtualMemory(
  _In_ HANDLE ProcessHandle,
  _In_opt_ PVOID BaseAddress,
  _In_reads_bytes_(BufferSize) PVOID Buffer,
  _In_ SIZE_T BufferSize,
  _Out_opt_ PSIZE_T NumberOfBytesWritten
  );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtFlushInstructionCache(
  _In_ HANDLE ProcessHandle,
  _In_opt_ PVOID BaseAddress,
  _In_ SIZE_T Length
  );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSuspendThread(
  _In_ HANDLE ThreadHandle,
  _Out_opt_ PULONG PreviousSuspendCount
  );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtResumeThread(
  _In_ HANDLE ThreadHandle,
  _Out_opt_ PULONG PreviousSuspendCount
  );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtGetContextThread(
  _In_ HANDLE ThreadHandle,
  _Inout_ PCONTEXT ThreadContext
  );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetContextThread(
  _In_ HANDLE ThreadHandle,
  _In_ PCONTEXT ThreadContext
  );

#pragma endregion

#pragma region KERNEL32.DLL

#define GetLastError                Mock_GetLastError
#define SetLastError                Mock_SetLastError

#define GetCurrentProcess()         NtCurrentProcess()
#define GetCurrentThread()          NtCurrentThread()
#define GetCurrentThreadId          Mock_GetCurrentThreadId

#define VirtualAlloc                Mock_VirtualAlloc
#define VirtualAllocEx              Mock_VirtualAllocEx
#define VirtualFree                 Mock_VirtualFree
#define VirtualFreeEx               Mock_VirtualFreeEx
#define VirtualProtect              Mock_VirtualProtect
#define VirtualProtectEx            Mock_VirtualProtectEx
#define VirtualQuery                Mock_VirtualQuery
#define VirtualQueryEx              Mock_VirtualQueryEx
#define ReadProcessMemory           Mock_ReadProcessMemory
#define WriteProcessMemory          Mock_WriteProcessMemory
#define FlushInstructionCache       Mock_FlushInstructionCache
#define SuspendThread               Mock_SuspendThread
#define ResumeThread                Mock_ResumeThread
#define GetThreadContext            Mock_GetThreadContext
#define SetThreadContext            Mock_SetThreadContext
#define DebugBreak                  __debugbreak

DWORD
WINAPI
GetLastError(
  VOID
  );

VOID
WINAPI
SetLastError(
  IN DWORD dwErrCode
  );

DWORD
WINAPI
GetCurrentThreadId(
  VOID
  );

LPVOID
NTAPI
VirtualAlloc(
  IN LPVOID lpAddress,
  IN SIZE_T dwSize,
  IN DWORD flAllocationType,
  IN DWORD flProtect
  );

LPVOID
NTAPI
VirtualAllocEx(
  IN HANDLE hProcess,
  IN LPVOID lpAddress,
  IN SIZE_T dwSize,
  IN DWORD flAllocationType,
  IN DWORD flProtect
  );

BOOL
NTAPI
VirtualFree(
  IN LPVOID lpAddress,
  IN SIZE_T dwSize,
  IN DWORD dwFreeType
  );

BOOL
NTAPI
VirtualFreeEx(
  IN HANDLE hProcess,
  IN LPVOID lpAddress,
  IN SIZE_T dwSize,
  IN DWORD dwFreeType
  );

BOOL
NTAPI
VirtualProtect(
  IN LPVOID lpAddress,
  IN SIZE_T dwSize,
  IN DWORD flNewProtect,
  OUT PDWORD lpflOldProtect
  );

BOOL
NTAPI
VirtualProtectEx(
  IN HANDLE hProcess,
  IN LPVOID lpAddress,
  IN SIZE_T dwSize,
  IN DWORD flNewProtect,
  OUT PDWORD lpflOldProtect
  );

SIZE_T
NTAPI
VirtualQuery(
  IN LPCVOID lpAddress,
  OUT PMEMORY_BASIC_INFORMATION lpBuffer,
  IN SIZE_T dwLength
  );

SIZE_T
NTAPI
VirtualQueryEx(
  IN HANDLE hProcess,
  IN LPCVOID lpAddress,
  OUT PMEMORY_BASIC_INFORMATION lpBuffer,
  IN SIZE_T dwLength
  );

BOOL
NTAPI
ReadProcessMemory(
  IN HANDLE hProcess,
  IN LPCVOID lpBaseAddress,
  IN LPVOID lpBuffer,
  IN SIZE_T nSize,
  OUT SIZE_T* lpNumberOfBytesRead
  );

BOOL
WINAPI
FlushInstructionCache(
  IN HANDLE hProcess,
  IN LPCVOID lpBaseAddress,
  IN SIZE_T nSize
  );

DWORD
WINAPI
SuspendThread(
  IN HANDLE hThread
  );

DWORD
WINAPI
ResumeThread(
  IN HANDLE hThread
  );

BOOL
WINAPI
GetThreadContext(
  IN HANDLE hThread,
  OUT LPCONTEXT lpContext
  );

BOOL
WINAPI
SetThreadContext(
  IN HANDLE hThread,
  IN CONST CONTEXT *lpContext
  );

#ifdef __cplusplus
}
#endif
