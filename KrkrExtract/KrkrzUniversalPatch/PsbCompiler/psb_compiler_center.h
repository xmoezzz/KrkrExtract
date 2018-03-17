#pragma once

#include "psb_link.h"
#include <my.h>

PVOID NTAPI AllocateMemoryPrototype(ULONG_PTR Size);
VOID  NTAPI FreeMemoryPrototype(PVOID Mem);
NTSTATUS NTAPI QueryFilePrototype(LPCWSTR FileName, PBYTE& Buffer, ULONG& Size);

class psb_compiler_center
{
public:
	psb_compiler_center();
	~psb_compiler_center();

	bool require_compile(wstring FileName,
		PBYTE SrcBuffer,
		ULONG SrcSize,
		API_POINTER(AllocateMemoryPrototype) AllocateMemoryFunc,
		API_POINTER(FreeMemoryPrototype) FreeMemoryFunc,
		API_POINTER(QueryFilePrototype) QueryFileFunc);

	bool compile();
	bool link();
	bool write_file(PBYTE& Buffer, ULONG& Size);

protected:
	wstring _src_file;

	API_POINTER(AllocateMemoryPrototype) AllocateMemoryRoutine;
	API_POINTER(FreeMemoryPrototype) FreeMemoryRoutine;
	API_POINTER(QueryFilePrototype) QueryFileRoutine;

	Json::Value source_code;
	Json::Reader source_reader;

	psb_cc _compiler;
	psb_link _link;

public:
	wstring src_file() const
	{
		return _src_file;
	}
};
