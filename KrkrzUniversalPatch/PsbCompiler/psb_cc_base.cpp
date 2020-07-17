
#include "psb_cc_base.h"

psb_cc_base::psb_cc_base(psb_value_t::type_t type) : _data(NULL), _length(0), _type(type), _cc(NULL)
{

}
psb_cc_base::psb_cc_base(psb_cc* cc, psb_value_t::type_t type) :_cc(cc), _data(NULL), _length(0), _type(type)
{

}

psb_cc_base::psb_cc_base(psb_cc* cc, Json::Value &src, psb_value_t::type_t type) :_cc(cc), _src(src), _data(NULL), _length(0), _type(type)
{

}

psb_cc_base::~psb_cc_base()
{
	if (_data)
	{
		delete[] _data;
	}
}

unsigned char* psb_cc_base::get_data()
{
	return _data;
}
void psb_cc_base::reset_data()
{
	if (_data) {
		delete[] _data;
		_data = NULL;
	}

	_length = 0;
}

uint32_t psb_cc_base::get_length()
{
	return _length;
}

psb_value_t::type_t psb_cc_base::get_type()
{
	return _type;
}

Json::Value psb_cc_base::get_source()
{
	return _src;
}
