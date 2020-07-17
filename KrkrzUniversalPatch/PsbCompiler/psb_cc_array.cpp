#include "psb_cc_array.h"

psb_cc_array::psb_cc_array() :psb_cc_base(psb_value_t::TYPE_ARRAY)
{

}


psb_cc_array::~psb_cc_array()
{
}


void psb_cc_array::set_value(uint32_t index, int64_t value)
{
	while (size() <= index) {
		resize(index + 1);
	}

	operator[](index) = value;
}

uint8_t psb_cc_array::get_data_align_bytes()
{
	uint8_t max_bytes = 1;

	for (uint32_t i = 0; i < size(); i++)
	{
		uint8_t bytes = cc_stream::calc_integer_size(operator[](i));
		if (bytes > max_bytes) max_bytes = bytes;
	}

	return max_bytes;
}

bool psb_cc_array::compile()
{
	uint8_t items_bytes;
	uint32_t count_bytes = cc_stream::calc_integer_size(size());
	cc_stream s;

	reset_data();

	items_bytes = get_data_align_bytes();

	//type_id
	cc_create_hdr(s, count_bytes);
	//items count
	s.write_integer(size(), count_bytes);
	//items byte,start of 1
	s.write_integer((items_bytes - 1) + psb_value_t::TYPE_ARRAY_N1, 1);

	//fillable value
	for (uint32_t i = 0; i < size(); i++) s.write_integer(operator[](i), items_bytes);

	_data = s.copy_buffer();
	_length = s.get_length();
	return true;
}

void psb_cc_array::cc_create_hdr(cc_stream& s, uint8_t bytes)
{
	switch (bytes)
	{
	case 1:s.write_integer(psb_value_t::TYPE_ARRAY_N1); _type = psb_value_t::TYPE_ARRAY_N1; break;
	case 2:s.write_integer(psb_value_t::TYPE_ARRAY_N2); _type = psb_value_t::TYPE_ARRAY_N2; break;
	case 3:s.write_integer(psb_value_t::TYPE_ARRAY_N3); _type = psb_value_t::TYPE_ARRAY_N3; break;
	case 4:s.write_integer(psb_value_t::TYPE_ARRAY_N4); _type = psb_value_t::TYPE_ARRAY_N4; break;
	case 5:s.write_integer(psb_value_t::TYPE_ARRAY_N5); _type = psb_value_t::TYPE_ARRAY_N5; break;
	case 6:s.write_integer(psb_value_t::TYPE_ARRAY_N6); _type = psb_value_t::TYPE_ARRAY_N6; break;
	case 7:s.write_integer(psb_value_t::TYPE_ARRAY_N7); _type = psb_value_t::TYPE_ARRAY_N7; break;
	case 8:s.write_integer(psb_value_t::TYPE_ARRAY_N8); _type = psb_value_t::TYPE_ARRAY_N8; break;
	}
}