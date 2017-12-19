#include "psb_cc_resource.h"



psb_cc_resource::psb_cc_resource(uint32_t index) :
	psb_cc_base(psb_value_t::TYPE_RESOURCE), _index(index)
{

}


psb_cc_resource::~psb_cc_resource()
{

}

uint32_t psb_cc_resource::get_index()
{
	return _index;
}

bool psb_cc_resource::compile()
{
	cc_stream s;
	reset_data();

	int type = cc_stream::calc_integer_size(_index);
	switch (type) {
	case 1: s.write_integer(psb_value_t::TYPE_RESOURCE_N1, 1); _type = psb_value_t::TYPE_RESOURCE_N1; break;
	case 2: s.write_integer(psb_value_t::TYPE_RESOURCE_N2, 1); _type = psb_value_t::TYPE_RESOURCE_N2; break;
	case 3: s.write_integer(psb_value_t::TYPE_RESOURCE_N3, 1); _type = psb_value_t::TYPE_RESOURCE_N3; break;
	case 4: s.write_integer(psb_value_t::TYPE_RESOURCE_N4, 1); _type = psb_value_t::TYPE_RESOURCE_N4; break;
	}

	s.write_integer(_index, type);


	_data = s.copy_buffer();
	_length = s.get_length();
	return true;
}
