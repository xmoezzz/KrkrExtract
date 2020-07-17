#pragma once
#include "psb_cc_base.h"
#include "psb_cc_array.h"
#include <string>
#include <map>

using std::map;
using std::string;

struct tree_desc
{
	unsigned char c;		//current char
	uint32_t node_id;	//tree id with _tree
	struct tree_desc* prev;	//parent char
	vector<struct tree_desc*> childs;//next chars


	uint32_t pos_beg;		//childs begin of array
	uint32_t pos_end;		//childs end of array
	uint32_t first_char;	//childs min char

	tree_desc()
	{
		node_id = -1;
		c = 0;
		pos_beg = 0;
		pos_end = 0;
		first_char = 0;
		prev = nullptr;
	}
	~tree_desc()
	{
		for (uint32_t i = 0; i < childs.size();i++)
		{
			delete childs[i];
		}
		childs.clear();
	}
};

class psb_cc_btree : public psb_cc_base
{
public:
	psb_cc_btree();
	~psb_cc_btree();
	
	void insert_tree(string value);
	static tree_desc *get_desc(tree_desc *desc, unsigned char c, bool is_end = false);

	void insert(string value);
	virtual bool compile();

	static bool min_char(tree_desc *desc, uint32_t &c);
	static bool max_char(tree_desc *desc, uint32_t &c);

	string get_name(tree_desc *desc);
	string get_name(uint32_t tid);
	uint32_t get_key_index(string& name);

	void make_tree(tree_desc *desc);
	void make_offset(tree_desc *desc);
	void make_branch(tree_desc *desc);
	void make_link();

	vector<string> values;
	map<string, uint32_t> results;
protected:
	tree_desc _desc;
	psb_cc_array _names;
	psb_cc_array _tree;
	psb_cc_array _offsets;
};

