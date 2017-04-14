#include <Windows.h>

PVOID NTAPI AllocateMemoryRoutine(ULONG_PTR Size);
VOID  NTAPI FreeMemoryRoutine(PVOID Mem);

decltype(&AllocateMemoryRoutine) XmoeAllocateMemory = NULL;
decltype(&FreeMemoryRoutine)     XmoeFreeMemory     = NULL;

#pragma comment(linker, "/EXPORT:XmoeInitFileSystem=_XmoeInitFileSystem@8,PRIVATE")
extern "C" __declspec(dllexport)
NTSTATUS NTAPI XmoeInitFileSystem(decltype(&AllocateMemoryRoutine) AllocateRoutine, decltype(&FreeMemoryRoutine) FreeRoutine)
{
	if (!AllocateRoutine || !FreeRoutine)
		return STATUS_INVALID_PARAMETER;

	XmoeAllocateMemory = AllocateRoutine;
	XmoeFreeMemory     = FreeRoutine;

	return 0;
}

#pragma comment(linker, "/EXPORT:XmoeQueryFile=_XmoeQueryFile@12,PRIVATE")
extern "C" __declspec(dllexport)
NTSTATUS NTAPI XmoeQueryFile(LPCWSTR FileName, PBYTE* FileBuffer, ULONG* FileSize)
{
	if (!FileName || !FileBuffer || !FileSize)
		return STATUS_INVALID_PARAMETER;

	return 0;
}

BOOL NTAPI DllMain(HMODULE hModule, DWORD Reason, LPVOID lpReserved)
{
	return TRUE;
}

