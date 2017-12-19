
#include "psb_cc_string.h"



psb_cc_string::psb_cc_string(psb_cc* cc, string s) :
	psb_cc_base(cc, psb_value_t::TYPE_STRING), _value(s), _index(-1)
{

}
psb_cc_string::psb_cc_string(psb_cc* cc) :
	psb_cc_base(cc, psb_value_t::TYPE_STRING), _index(0)
{
}


psb_cc_string::~psb_cc_string()
{

}

bool psb_cc_string::compile()
{
	cc_stream s;
	
	reset_data();

	_index = _cc->add_string(_value);
	uint8_t type = s.calc_integer_size(_index);

	switch (type)
	{
	case 1: s.write_integer(psb_value_t::TYPE_STRING_N1, 1); break;
	case 2: s.write_integer(psb_value_t::TYPE_STRING_N2, 1); break;
	case 3: s.write_integer(psb_value_t::TYPE_STRING_N3, 1); break;
	case 4: s.write_integer(psb_value_t::TYPE_STRING_N4, 1); break;
	}

	s.write_integer(_index, type);

	_data = s.copy_buffer();
	_length = s.get_length();
	return true;
}

