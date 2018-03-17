
#include "psb_cc_btree.h"



psb_cc_btree::psb_cc_btree() :psb_cc_base(psb_value_t::TYPE_BTREE)
{

}


psb_cc_btree::~psb_cc_btree()
{
}

void psb_cc_btree::insert(string value)
{
	values.push_back(value);
	insert_tree(value);
}

tree_desc *psb_cc_btree::get_desc(tree_desc *desc, unsigned char c, bool is_end)
{
	for (uint32_t i = 0; i < desc->childs.size(); i++)
	{
		if (desc->childs[i]->c == c) return desc->childs[i];
	}

	tree_desc* my_desc = new tree_desc();
	my_desc->c = c;
	my_desc->prev = desc;
	desc->childs.push_back(my_desc);
	return my_desc;
}

void psb_cc_btree::insert_tree(string value)
{
	tree_desc* prev = &_desc;
	for (uint32_t i = 0; i < value.length(); i++)
	{
		unsigned char c = (unsigned char)value[i];
		prev = get_desc(prev, c);
	}

	prev = get_desc(prev, 0, true);
}
uint32_t psb_cc_btree::get_key_index(string& name)
{
	for (uint32_t i = 0; i < values.size();i++)
	{
		if (values[i] == name) return i;
	}
	return -1;
}

string psb_cc_btree::get_name(tree_desc *desc)
{
	string accum;
	tree_desc* a = desc->prev;

	while (a->prev) {
		accum = (char)a->c + accum;
		a = a->prev;
	}
	return accum;
}


bool psb_cc_btree::compile()
{
	cc_stream s;

	_names.resize(values.size());
	_offsets.push_back(1);		//set default char offsets
	_desc.node_id = 0;

	make_branch(&_desc);
	make_link();

	_names.compile();
	_offsets.compile();
	_tree.compile();

	s.append(_offsets.get_data(), _offsets.get_length());
	s.append(_tree.get_data(), _tree.get_length());
	s.append(_names.get_data(), _names.get_length());

	_data = s.copy_buffer();
	_length = s.get_length();

	return true;
}
string psb_cc_btree::get_name(uint32_t tid)
{
	string accum;
	uint32_t a = (uint32_t)_names[tid];
	uint32_t b = (uint32_t)_tree[a];

	while (true) {
		uint32_t c = (uint32_t)_tree[b];
		uint32_t d = (uint32_t)_offsets[c];
		uint32_t e = (uint32_t)b - d;

		b = c;

		accum = (char)e + accum;

		if (!b) {
			break;
		}
	}
	return accum;
}

void psb_cc_btree::make_tree(tree_desc *desc)
{
	uint32_t node_id;
	uint32_t offset;

	if (desc->prev->first_char == 0)	//check is first char
	{
		//tree['a' + 0x1] = 0;
		//offsets[0] = 0x1;

		node_id = desc->c;							//'a'
		offset = (uint32_t)_offsets[desc->prev->node_id];		//first char offset = 1
		node_id += offset;							//'a' + 0x1
	}
	else
	{
		//[current char] - [range char min] + start_pos
		node_id = ((uint32_t)desc->c - desc->prev->first_char) + desc->prev->pos_beg;
	}

	//set parent value
	_tree.set_value(node_id, desc->prev->node_id);
	//set node id
	desc->node_id = node_id;
}

void psb_cc_btree::make_offset(tree_desc *desc)
{
	uint32_t m, n;

	max_char(desc, m);
	min_char(desc, n);

	uint32_t count = (m - n);

	uint32_t pos = _tree.size();
	if (pos <= m || pos <= n)
	{
		_tree.set_value(m, 0);
		pos = _tree.size();
	}

	uint32_t end_pos = pos + count;
	uint32_t offset = pos - n;
	_tree.set_value(end_pos, 0);
	_offsets.set_value(desc->node_id, offset);

	if (desc->c == 0)		//end point link to names table index
	{
		uint32_t index = get_key_index(get_name(desc));
		_offsets.set_value(desc->node_id, index);
		return;
	}

	desc->pos_beg = pos;
	desc->pos_end = end_pos;
	desc->first_char = n;
}
void psb_cc_btree::make_link()
{
	for (uint32_t i = 0; i < values.size(); i++)
	{
		uint32_t id = results[values[i]];
		_names.set_value(i, id);
	}
}

void psb_cc_btree::make_branch(tree_desc *desc)
{
	for (uint32_t i = 0; i < desc->childs.size(); i++)
	{
		make_tree(desc->childs[i]);
	}

	for (uint32_t i = 0; i < desc->childs.size(); i++)
	{
		make_offset(desc->childs[i]);
	}

	for (uint32_t i = 0; i < desc->childs.size(); i++)
	{
		make_branch(desc->childs[i]);
	}

	for (uint32_t i = 0; i < desc->childs.size(); i++)
	{
		if (desc->childs[i]->c == 0)
		{
			results[get_name(desc->childs[i])] = desc->childs[i]->node_id;
		}
	}
}
bool psb_cc_btree::min_char(tree_desc* desc, uint32_t& c)
{
	c = 0xff;

	if (desc->childs.empty())
	{
		c = 0;
		return false;
	}

	for (uint32_t i = 0; i < desc->childs.size(); i++)
	{
		if (uint32_t(desc->childs[i]->c) < c)
		{
			c = uint32_t(desc->childs[i]->c);
		}
	}

	return true;
}

bool psb_cc_btree::max_char(tree_desc * desc, uint32_t & c)
{
	c = 0;

	if (desc->childs.empty())
	{
		c = 0;
		return false;
	}

	for (uint32_t i = 0; i < desc->childs.size(); i++)
	{
		if (uint32_t(desc->childs[i]->c) > c)
		{
			c = uint32_t(desc->childs[i]->c);
		}
	}

	return true;
}
