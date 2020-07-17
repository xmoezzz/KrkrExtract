#pragma once
#include "psb_cc_base.h"
#include "cc_stream.h"

class psb_cc_decimal : public psb_cc_base
{
public:
	psb_cc_decimal();
	psb_cc_decimal(float value);
	psb_cc_decimal(double value);
	~psb_cc_decimal();

	bool is_double();
	float get_value_float();
	double get_value_double();


	virtual bool compile();

protected:

	struct
	{
		float f;
		double d;
	}_value;
	
	bool _double_flag;
	bool _zero_flag;


private:
	void cc_create_hdr(cc_stream& s);
	void cc_double(cc_stream& s);
	void cc_float(cc_stream& s);
};

