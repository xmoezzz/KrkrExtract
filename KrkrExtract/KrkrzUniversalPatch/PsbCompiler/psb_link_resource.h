#pragma once
#include "psb_cc_base.h"
#include "psb_cc_array.h"
#include "ResourcePair.h"
#include <vector>
#include <my.h>

NTSTATUS NTAPI QueryFilePrototype(LPCWSTR FileName, PBYTE& Buffer, ULONG& Size);
VOID  NTAPI FreeMemoryPrototype(PVOID Mem);

struct psb_resource
{
	uint32_t id;
	unsigned char *data;
	uint32_t length;

	psb_resource()
	{
		id = 0;
		data = NULL;
		length = 0;
	}
};

class psb_link_resource
{
public:
	psb_link_resource();
	~psb_link_resource();


	bool load_resource(std::vector<ResourcePair>& resource_code, wstring _res_path, API_POINTER(QueryFilePrototype) QueryFileFunc,
		API_POINTER(FreeMemoryPrototype) FreeMemoryFunc);
	bool load_file(uint32_t i, wstring filename);
	virtual bool compile();


	API_POINTER(QueryFilePrototype) QueryFile;
	API_POINTER(FreeMemoryPrototype) FreeMemoryZ;

	vector<psb_resource> resource_table;
	psb_cc_array chunk_offsets;
	psb_cc_array chunk_lengths;
	cc_stream chunk_data;
};

