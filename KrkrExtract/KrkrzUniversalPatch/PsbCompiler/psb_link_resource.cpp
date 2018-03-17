#include "psb_link_resource.h"
#include <fstream>

using std::fstream;


psb_link_resource::psb_link_resource()
{
}


psb_link_resource::~psb_link_resource()
{
	for (auto& Item : resource_table)
	{
		if (Item.data)
			FreeMemoryZ(Item.data);
	}
}

bool psb_link_resource::compile()
{
	for (uint32_t i = 0; i < resource_table.size();i++)
	{
		chunk_offsets.push_back(chunk_data.get_length());
		chunk_lengths.push_back(resource_table[i].length);
		chunk_data.append(resource_table[i].data, resource_table[i].length);
	}

	chunk_offsets.compile();
	chunk_lengths.compile();

	return true;
}

bool psb_link_resource::load_resource(std::vector<ResourcePair>& resource_code, wstring _res_path, API_POINTER(QueryFilePrototype) QueryFileFunc,
	API_POINTER(FreeMemoryPrototype) FreeMemoryFunc)
{
	QueryFile   = QueryFileFunc;
	FreeMemoryZ = FreeMemoryFunc;

	for (auto& Item : resource_code)
	{
		if (!load_file(Item.Index, _res_path + L"\\" + Item.FileName))
			return false;
	}
	return true;
}

bool psb_link_resource::load_file(uint32_t i, wstring filename)
{
	PBYTE        Buffer;
	ULONG        Size;
	psb_resource item;

	Buffer = NULL;
	Size = 0;

	if (NT_FAILED(QueryFile(filename.c_str(), Buffer, Size)))
		return false;

	item.data = Buffer;
	item.length = Size;
	item.id = i;

	resource_table.push_back(item);
	return true;
}
