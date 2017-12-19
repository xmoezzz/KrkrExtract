#pragma once
#include "psb_cc_base.h"

class psb_cc_collection : public psb_cc_base, vector<psb_cc_base*>
{
public:
	psb_cc_collection(psb_cc* cc, Json::Value sourceCode);
	~psb_cc_collection();

	virtual bool compile();

};

