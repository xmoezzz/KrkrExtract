#include "psb_cc_integer.h"

psb_cc_integer::psb_cc_integer(int64_t number):
	psb_cc_base(psb_value_t::TYPE_INTEGER),_number(number)
{

}

psb_cc_integer::psb_cc_integer():
	psb_cc_base(psb_value_t::TYPE_INTEGER), _number(0)
{
}

psb_cc_integer::~psb_cc_integer()
{

}


int64_t psb_cc_integer::get_number()
{
	return _number;
}

void psb_cc_integer::set_number(int64_t number)
{
	_number = number;
}

uint8_t psb_cc_integer::cc_create_hdr(cc_stream& s)
{
	uint8_t type = cc_stream::calc_integer_size(_number);
	if (_number == 0) {
		type = 0;
	}
	switch (type)
	{
	case 0: s.write_integer(psb_value_t::TYPE_NUMBER_N0); _type = psb_value_t::TYPE_NUMBER_N0; break;
	case 1: s.write_integer(psb_value_t::TYPE_NUMBER_N1); _type = psb_value_t::TYPE_NUMBER_N1; break;
	case 2: s.write_integer(psb_value_t::TYPE_NUMBER_N2); _type = psb_value_t::TYPE_NUMBER_N2; break;
	case 3: s.write_integer(psb_value_t::TYPE_NUMBER_N3); _type = psb_value_t::TYPE_NUMBER_N3; break;
	case 4: s.write_integer(psb_value_t::TYPE_NUMBER_N4); _type = psb_value_t::TYPE_NUMBER_N4; break;
	case 5: s.write_integer(psb_value_t::TYPE_NUMBER_N5); _type = psb_value_t::TYPE_NUMBER_N5; break;
	case 6: s.write_integer(psb_value_t::TYPE_NUMBER_N6); _type = psb_value_t::TYPE_NUMBER_N6; break;
	case 7: s.write_integer(psb_value_t::TYPE_NUMBER_N7); _type = psb_value_t::TYPE_NUMBER_N7; break;
	case 8: s.write_integer(psb_value_t::TYPE_NUMBER_N8); _type = psb_value_t::TYPE_NUMBER_N8; break;
	}
	return type;
}
bool psb_cc_integer::compile()
{
	cc_stream cc;
	reset_data();

	uint8_t type = cc_create_hdr(cc);

	if (type > 0)
	{
		cc.write_integer(_number, type);
	}
	
	_data = cc.copy_buffer();
	_length = cc.get_length();
	return true;
}
