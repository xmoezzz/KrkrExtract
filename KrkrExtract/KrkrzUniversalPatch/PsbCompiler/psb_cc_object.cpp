
#include "psb_cc_object.h"
#include "psb_cc_array.h"
#include <algorithm>

psb_cc_object::psb_cc_object(psb_cc* cc, Json::Value& sourceCode) :
	psb_cc_base(cc, sourceCode, psb_value_t::TYPE_OBJECTS)
{
	vector<string> members = sourceCode.getMemberNames();

	for (uint32_t i = 0; i < members.size(); i++)
	{
		string key_name = members[i];
		_objects[key_name] = cc->pack(sourceCode[key_name]);
	}
}


psb_cc_object::~psb_cc_object()
{
}

bool psb_cc_object::compile()
{
	cc_stream s;
	reset_data();

	s.write_integer(psb_value_t::TYPE_OBJECTS, 0x1);

	psb_cc_array *names = new psb_cc_array();
	psb_cc_array *offsets = new psb_cc_array();

	names->resize(_objects.size());
	offsets->resize(_objects.size());

	vector <string> src_names = _src.getMemberNames();

	cc_stream entries;
	vector<uint32_t> names_vec;
	for (uint32_t i = 0; i < src_names.size(); i++)
	{
		string key_name = src_names[i];
		names_vec.push_back(_cc->add_names(key_name));
	}

	sort(names_vec.begin(), names_vec.end());
	
	for (uint32_t i = 0; i < names_vec.size(); i++)
	{
		string key_name = _cc->get_names(names_vec[i]);
		names->operator[](i) = names_vec[i];
		psb_cc_base *base = _objects[key_name];
		base->compile();

		offsets->operator[](i) = entries.get_length();
		entries.append(base->get_data(), base->get_length());
	}

	names->compile();
	offsets->compile();

	s.append(names->get_data(), names->get_length());
	s.append(offsets->get_data(), offsets->get_length());
	s.append(entries.get_data(), entries.get_length());

	_data = s.copy_buffer();
	_length = s.get_length();
	return true;
}

