#include "psb_cc_base.h"
#include "cc_stream.h"
#include "psb_link.h"
#include "psb_link_resource.h"



psb_link::psb_link() :
	_data(nullptr), _length(0)
{
	_resource = NULL;
	names = NULL;
	_strings_cc = NULL;
	_data = NULL;
}


psb_link::~psb_link()
{
	if (_resource)
		delete _resource;

	if (names)
		delete names;

	if (_strings_cc)
		delete _strings_cc; 

	if (_data)
		delete[] _data;

	_resource = NULL;
}

bool psb_link::link(psb_cc &cc, std::vector<ResourcePair>& resource_code, wstring _res_path, API_POINTER(QueryFilePrototype) QueryFileFunc,
	API_POINTER(FreeMemoryPrototype) FreeMemoryFunc)
{
	cc_stream complete_data;
	PSBHDR hdr;
	_compiler = &cc;
	res_path = _res_path;

	psb_cc_base* entries = cc.get_entries();
	
	if(!link_string_table())
		return false;

	link_names_table();

	if (!link_resource_table(resource_code, _res_path, QueryFileFunc, FreeMemoryFunc))
		return false;

	memcpy(&hdr.signature, "PSB", 4);
	hdr.encrypt = 0;
	hdr.version = 2;
	hdr.offset_encrypt = 0;
	complete_data.append((unsigned char*)&hdr, sizeof(hdr));

	hdr.offset_names = complete_data.get_length();
	complete_data.append(names->get_data(), names->get_length());

	hdr.offset_entries = complete_data.get_length();
	complete_data.append(entries->get_data(), entries->get_length());

	hdr.offset_strings = complete_data.get_length();
	complete_data.append(_strings_cc->get_data(), _strings_cc->get_length());

	hdr.offset_strings_data = complete_data.get_length();
	complete_data.append(_string_data.get_data(), _string_data.get_length());

	hdr.offset_chunk_offsets = complete_data.get_length();
	complete_data.append(_resource->chunk_offsets.get_data(), _resource->chunk_offsets.get_length());
	hdr.offset_chunk_lengths = complete_data.get_length();
	complete_data.append(_resource->chunk_lengths.get_data(), _resource->chunk_lengths.get_length());
	hdr.offset_chunk_data = complete_data.get_length();
	complete_data.append(_resource->chunk_data.get_data(), _resource->chunk_data.get_length());

	complete_data.replace(0, (const unsigned char*)&hdr, sizeof(hdr));

	_data = complete_data.copy_buffer();
	_length = complete_data.get_length();

	return true;
}

bool psb_link::link_string_table()
{
	_strings_cc = new psb_cc_array();
	_strings_cc->resize(_compiler->_string_table.size());

	for (uint32_t i = 0; i < _compiler->_string_table.size(); i++)
	{
		_strings_cc->operator[](i) = _string_data.get_length();
		_string_data.append((unsigned char*)_compiler->_string_table[i].c_str(), _compiler->_string_table[i].length() + 1);
	}

	return _strings_cc->compile();
}

bool psb_link::link_names_table()
{
	names = new psb_cc_btree();
	
	for (uint32_t i = 0; i < _compiler->_name_table.size(); i++)
	{
		names->insert(_compiler->_name_table[i]);
	}

	return names->compile();

}

bool psb_link::link_resource_table(std::vector<ResourcePair>& resource_code, wstring _res_path, API_POINTER(QueryFilePrototype) QueryFileFunc,
	API_POINTER(FreeMemoryPrototype) FreeMemoryFunc)
{
	_resource = new psb_link_resource();

	if (!_resource->load_resource(resource_code, _res_path, QueryFileFunc, FreeMemoryFunc))
		return false;
	
	if (!_resource->compile())
		return false;

	return true;
}
