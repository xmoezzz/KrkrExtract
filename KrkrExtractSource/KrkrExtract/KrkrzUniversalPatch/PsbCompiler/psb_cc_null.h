#pragma once
#include "psb_cc_base.h"
class psb_cc_null :
	public psb_cc_base
{
public:
	psb_cc_null();
	~psb_cc_null();

	virtual bool compile();
};

