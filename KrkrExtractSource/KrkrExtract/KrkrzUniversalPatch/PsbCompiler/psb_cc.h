#pragma once

#include <stdint.h>
#include "json.h"
#include <string>
#include <vector>
#include "ResourcePair.h"

using std::string;
using std::vector;

class psb_cc_base;

class psb_cc
{
	friend psb_cc_base;
public:
	psb_cc();
	~psb_cc();

	bool cc(Json::Value &src);
	void precache_name_all(Json::Value &source_code);
	uint32_t add_string(string value);
	uint32_t add_names(string value);
	string get_names(uint32_t index);
	psb_cc_base* pack(Json::Value& source_code);

	psb_cc_base* get_entries();


	std::vector<ResourcePair>& get_resource_code()
	{
		return resource_code;
	}

public:
	vector<string> _string_table;
	vector<string> _name_table;
	psb_cc_base *_entries;

	std::vector<ResourcePair> resource_code;
};
