#include "psb.hpp"
#include "psb_cc_collection.h"
#include "psb_cc_array.h"


psb_cc_collection::psb_cc_collection(psb_cc* cc, Json::Value sourceCode) :
	psb_cc_base(cc, sourceCode, psb_value_t::TYPE_COLLECTION)
{
	for (uint32_t i = 0; i < sourceCode.size(); i++)
	{
		push_back(cc->pack(sourceCode[i]));
	}
}


psb_cc_collection::~psb_cc_collection()
{
}

bool psb_cc_collection::compile()
{
	cc_stream s;

	reset_data();

	s.write_integer(psb_value_t::TYPE_COLLECTION);

	psb_cc_array* offsets = new psb_cc_array();
	offsets->resize(size());

	cc_stream entries;

	for (uint32_t i = 0; i < size(); i++)
	{
		//cout << "[" << i << "]" << endl;

		operator[](i)->compile();
		//operator[](i)->dump();

		//begin of buffer offset
		offsets->operator[](i) = entries.get_length();
		entries.append(operator[](i)->get_data(), operator[](i)->get_length());
	}

	//replace offsets table
	offsets->compile();

	s.append(offsets->get_data(), offsets->get_length());
	s.append(entries.get_data(), entries.get_length());

	//set compile result
	_data = s.copy_buffer();
	_length = s.get_length();
	return true;
}
