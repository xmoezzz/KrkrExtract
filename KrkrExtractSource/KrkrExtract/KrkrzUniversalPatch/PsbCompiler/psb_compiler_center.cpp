#include "psb_compiler_center.h"
#include "../Tool.h"


psb_compiler_center::psb_compiler_center()
{
}


psb_compiler_center::~psb_compiler_center()
{
}

bool psb_compiler_center::require_compile(wstring FileName,
	PBYTE SrcBuffer, 
	ULONG SrcSize, 
	API_POINTER(AllocateMemoryPrototype) AllocateMemoryFunc, 
	API_POINTER(FreeMemoryPrototype) FreeMemoryFunc,
	API_POINTER(QueryFilePrototype) QueryFileFunc)
{
	_src_file = FileName;

	AllocateMemoryRoutine = AllocateMemoryFunc;
	FreeMemoryRoutine = FreeMemoryFunc;
	QueryFileRoutine = QueryFileFunc;
	
	if (!source_reader.parse((LPCSTR)SrcBuffer, (LPCSTR)(SrcBuffer + SrcSize), source_code))
	{
		FreeMemoryFunc(SrcBuffer);
		return false;
	}

	return true;
}

bool psb_compiler_center::compile()
{
	return _compiler.cc(source_code);
}

bool psb_compiler_center::link()
{
	return _link.link(_compiler, _compiler.get_resource_code(), (_src_file + L".dir"), QueryFileRoutine, FreeMemoryRoutine);
}

bool psb_compiler_center::write_file(PBYTE& Buffer, ULONG& Size)
{
	Buffer = NULL;
	Size = 0;

	Buffer = (PBYTE)AllocateMemoryRoutine(_link.length());
	if (!Buffer)
		return false;

	Size = _link.length();
	RtlCopyMemory(Buffer, _link.data(), Size);
	return true;
}
