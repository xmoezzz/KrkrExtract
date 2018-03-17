#pragma once
#include "psb_cc_base.h"


class psb_cc_resource :
	public psb_cc_base
{
public:
	psb_cc_resource(uint32_t index);
	~psb_cc_resource();

	virtual bool compile();
	uint32_t get_index();
protected:
	uint32_t _index;
};

