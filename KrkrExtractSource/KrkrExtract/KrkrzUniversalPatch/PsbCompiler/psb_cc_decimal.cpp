#include <my.h>
#include "psb_cc_decimal.h"


psb_cc_decimal::psb_cc_decimal() :
	psb_cc_base(psb_value_t::TYPE_DECIMAL), _zero_flag(true), _double_flag(false)
{
	RtlZeroMemory(&_value, sizeof(_value));
}


psb_cc_decimal::psb_cc_decimal(float value) :
	psb_cc_base(psb_value_t::TYPE_FLOAT), _double_flag(false), _zero_flag(false)
{
	if (value == 0.0f) {
		_zero_flag = true;
		_type = psb_value_t::TYPE_FLOAT0;
	}

	_value.f = value;
}
psb_cc_decimal::psb_cc_decimal(double value) :
	psb_cc_base(psb_value_t::TYPE_DOUBLE), _double_flag(true), _zero_flag(false)
{
	if (value == 0.0) {
		_zero_flag = true;
		_type = psb_value_t::TYPE_FLOAT0;
	}

	_value.d = value;
}
psb_cc_decimal::~psb_cc_decimal()
{

}

bool psb_cc_decimal::compile()
{
	cc_stream s;
	reset_data();

	cc_create_hdr(s);

	if (!_zero_flag)
	{
		if (_double_flag) {
			s.write_t(_value.d);
		}
		else {
			s.write_t(_value.f);
		}
	}

	_data = s.copy_buffer();
	_length = s.get_length();
	return true;
}


void psb_cc_decimal::cc_create_hdr(cc_stream& s)
{
	if (_double_flag) {
		if (_value.d == 0.0) {
			s.write_integer(psb_value_t::TYPE_FLOAT0, 1);
		}
		else {
			s.write_integer(psb_value_t::TYPE_DOUBLE);
		}
	}
	else {
		if (_value.f == 0.0f) {
			s.write_integer(psb_value_t::TYPE_FLOAT0, 1);
		}
		else {
			s.write_integer(psb_value_t::TYPE_FLOAT);
		}
	}
}

bool psb_cc_decimal::is_double()
{
	return _double_flag;
}
float psb_cc_decimal::get_value_float()
{
	return _value.f;
}
double psb_cc_decimal::get_value_double()
{
	return _value.d;
}