#pragma once
#include "psb_cc_base.h"


class psb_cc_object :
	public psb_cc_base
{
public:
	psb_cc_object(psb_cc* cc,Json::Value& sourceCode);
	~psb_cc_object();

	virtual bool compile();

protected:
	map<string, psb_cc_base*> _objects;
};

