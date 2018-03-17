#pragma once
#include "psb_cc_base.h"


class psb_cc_string :
	public psb_cc_base
{
public:
	psb_cc_string(psb_cc* cc, string s);
	psb_cc_string(psb_cc* cc);
	~psb_cc_string();

	virtual bool compile();

protected:
	string _value;
	uint32_t _index;

};

