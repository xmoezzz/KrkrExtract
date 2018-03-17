#include "psb_compiler_center.h"
#include <my.h>

PVOID NTAPI AllocateMemoryInternal(ULONG_PTR Size)
{
	return AllocateMemoryP(Size);
}

VOID  NTAPI FreeMemoryInternal(PVOID Mem)
{
	FreeMemoryP(Mem);
}


NTSTATUS NTAPI QueryFilePrototype(LPCWSTR FileName, PBYTE& Buffer, ULONG& Size);

NTSTATUS NTAPI CompilePsbFull(wstring& FileName, PBYTE& OutBuffer, ULONG& OutSize, API_POINTER(QueryFilePrototype) QueryFileFunc)
{
	psb_compiler_center pcc;
	PBYTE               SrcBuffer;
	ULONG               SrcSize;

	SrcBuffer = NULL;
	SrcSize   = 0;
	OutBuffer = NULL;
	OutSize   = 0;

	if (NT_FAILED(QueryFileFunc(FileName.c_str(), SrcBuffer, SrcSize)))
		return false;

	if (!pcc.require_compile(FileName, SrcBuffer, SrcSize, AllocateMemoryInternal, FreeMemoryInternal, QueryFileFunc))
	{
		FreeMemoryP(SrcBuffer);
		return false;
	}

	if (!pcc.compile())
	{
		FreeMemoryP(SrcBuffer);
		return false;
	}

	if(!pcc.link())
	{
		FreeMemoryP(SrcBuffer);
		return false;
	}

	if (!pcc.write_file(OutBuffer, OutSize))
	{
		FreeMemoryP(SrcBuffer);
		return false;
	}

	FreeMemoryP(SrcBuffer);
	return true;
}
