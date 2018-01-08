#pragma once

#include "psb_cc_base.h"

class psb_cc_integer : public psb_cc_base
{
public:
	psb_cc_integer(int64_t number);
	psb_cc_integer();
	~psb_cc_integer();

	virtual bool compile();

	uint8_t cc_create_hdr(cc_stream& s);

	int64_t get_number();
	void set_number(int64_t number);
protected:

	int64_t _number;
};

