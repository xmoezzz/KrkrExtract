#ifndef _PSB_H_INCLUDED_
#define _PSB_H_INCLUDED_


#include <stdio.h>
#include <string>
#include <map>
#include <vector>
#include <stdint.h>

using namespace std;

#pragma pack(1)

struct PSBHDR {
	unsigned char signature[4];
	uint16_t version;		//version
	uint16_t encrypt;
	uint32_t offset_encrypt;	// = offset_names
	uint32_t offset_names;//
	uint32_t offset_strings;
	uint32_t offset_strings_data;
	uint32_t offset_chunk_offsets;
	uint32_t offset_chunk_lengths;
	uint32_t offset_chunk_data;
	uint32_t offset_entries;
};

struct PSBHDR3 {
	unsigned char signature[4];
	uint16_t version;		//version
	uint16_t encrypt;
	uint32_t offset_encrypt;	// = offset_names
	uint32_t offset_names;//
	uint32_t offset_strings;
	uint32_t offset_strings_data;
	uint32_t offset_chunk_offsets;
	uint32_t offset_chunk_lengths;
	uint32_t offset_chunk_data;
	uint32_t offset_entries;
	uint32_t offset_emote;
};

#pragma pack()


/***************************************************************************
* A whole bunch of packed data junk...
***************************************************************************/

class psb_t;

/***************************************************************************
* psb_value_t
*/
class psb_value_t {
public:
	// Probably this should actually be kind as in get_number below. Don't care.
	enum type_t {

		TYPE_NONE = 0x0,
		TYPE_NULL = 0x1, // 0
		TYPE_FALSE = 0x2, //??
		TYPE_TRUE = 0x3,  //??

		//int
		TYPE_NUMBER_N0 = 0x4,
		TYPE_NUMBER_N1 = 0x5,
		TYPE_NUMBER_N2 = 0x6,
		TYPE_NUMBER_N3 = 0x7,
		TYPE_NUMBER_N4 = 0x8,
		TYPE_NUMBER_N5 = 0x9,
		TYPE_NUMBER_N6 = 0xA,
		TYPE_NUMBER_N7 = 0xB,
		TYPE_NUMBER_N8 = 0xC,

		//array N(NUMBER) is count mask
		TYPE_ARRAY_N1 = 0xD,
		TYPE_ARRAY_N2 = 0xE,
		TYPE_ARRAY_N3 = 0xF,
		TYPE_ARRAY_N4 = 0x10,
		TYPE_ARRAY_N5 = 0x11,
		TYPE_ARRAY_N6 = 0x12,
		TYPE_ARRAY_N7 = 0x13,
		TYPE_ARRAY_N8 = 0x14,

		//index of strings table
		TYPE_STRING_N1 = 0x15,
		TYPE_STRING_N2 = 0x16,
		TYPE_STRING_N3 = 0x17,
		TYPE_STRING_N4 = 0x18,

		//resource of thunk
		TYPE_RESOURCE_N1 = 0x19,
		TYPE_RESOURCE_N2 = 0x1A,
		TYPE_RESOURCE_N3 = 0x1B,
		TYPE_RESOURCE_N4 = 0x1C,

		//fpu value
		TYPE_FLOAT0 = 0x1D,
		TYPE_FLOAT = 0x1E,
		TYPE_DOUBLE = 0x1F,

		//objects
		TYPE_COLLECTION = 0x20,	//object collection
		TYPE_OBJECTS = 0x21,	//object


		//used by compiler,it's fake
		TYPE_INTEGER = 0x80,
		TYPE_STRING = 0x81,
		TYPE_RESOURCE = 0x82,
		TYPE_DECIMAL = 0x83,
		TYPE_ARRAY = 0x84,
		TYPE_BOOLEAN = 0x85,
		TYPE_BTREE = 0x86,
	};

	psb_value_t(const psb_t&    psb,
		type_t          type,
		unsigned char*& p);
	psb_value_t(const psb_t&    psb,
		unsigned char*& p);


	virtual ~psb_value_t(void);

	type_t get_type(void) const;

protected:
	const psb_t& psb;
	type_t type;
};
/***************************************************************************
* psb_null_t
*/
class psb_null_t : public psb_value_t {
public:
	psb_null_t(const psb_t&    psb,
		unsigned char*& p,
		psb_value_t::type_t type);

private:
	unsigned char *buff;
};
/***************************************************************************
* psb_null_t
*/
class psb_boolean_t : public psb_value_t {
public:
	psb_boolean_t(const psb_t&    psb,
		unsigned char*& p,
		psb_value_t::type_t type);

	bool get_boolean();

private:
	unsigned char *buff;
	bool value;
};
/***************************************************************************
* psb_resource_t
*/
class psb_resource_t : public psb_value_t {
public:
	psb_resource_t(const psb_t&    psb,
		unsigned char*& p,
		psb_value_t::type_t type);

	unsigned char *get_buff();
	uint32_t get_length();
	uint32_t get_index();
protected:
	uint32_t chunk_index;
	unsigned char *chunk_buff;
	uint32_t chunk_len;
};
/***************************************************************************
* psb_number_t
*/
class psb_number_t : public psb_value_t {
public:
	union psb_number_value_t {
		float f;
		double d;
		int64_t i;
	};

	enum psb_number_type_t {
		INTEGER,
		FLOAT,
		DOUBLE
	};

	psb_number_t(const psb_t&    psb,
		unsigned char*& p, psb_value_t::type_t type);

	float get_float() const;
	double get_double() const;
	int64_t get_integer() const;
	psb_number_type_t get_number_type() const;

	static bool is_number_type(psb_value_t *value);

private:
	psb_number_value_t value;
	psb_number_type_t number_type;
	unsigned char *buff;
};
/***************************************************************************
* psb_array_t
*/
class psb_array_t : public psb_value_t {
public:
	psb_array_t(const psb_t&    psb,
		unsigned char*& p, psb_value_t::type_t type);

	uint32_t size(void) const;

	uint32_t get(uint32_t index) const;

	uint32_t  data_length;
	uint32_t  entry_count;
	uint32_t  entry_length;
	unsigned char* buff;

};
/***************************************************************************
* psb_string_t
*/
class psb_string_t : public psb_value_t {
public:
	psb_string_t(const psb_t&    psb,
		unsigned char*& p);

	uint32_t get_index() const;

	string get_string() const;

	unsigned char* buff;
};
/***************************************************************************
* psb_objects_t
*/
class psb_objects_t : public psb_value_t {
public:
	psb_objects_t(const psb_t&    psb,
		unsigned char*& p);

	~psb_objects_t(void);

	uint32_t size(void) const;

	string get_name(uint32_t index) const;

	unsigned char* get_data(uint32_t index) const;

	unsigned char* get_data(const string& name) const;

	template<class T> void unpack(T*& out, const string& name) const;

public:
	psb_array_t*   names;
	psb_array_t*   offsets;
	unsigned char* buff;
};

/***************************************************************************
* psb_collection_t
*/
class psb_collection_t : public psb_value_t {
public:
	psb_collection_t(const psb_t&    psb,
		unsigned char*& p);

	~psb_collection_t(void);

	uint32_t size(void) const;

	unsigned char* get(uint32_t index) const;

	template<class T> void unpack(T*& out, uint32_t index) const;

public:
	psb_array_t*   offsets;
	unsigned char* buff;
};

/***************************************************************************
* psb_t
*/
class psb_t {
public:
	psb_t(unsigned char* buff);

	~psb_t(void);

	string get_name(uint32_t index) const;

	bool get_number(unsigned char* p, psb_number_t::psb_number_value_t &value, psb_number_t::psb_number_type_t &number_type) const;

	string get_string(unsigned char* p) const;
	uint32_t get_string_index(unsigned char* p) const;

	const psb_objects_t* get_objects(void) const;

	unsigned char* get_chunk(unsigned char* p) const;

	uint32_t get_chunk_length(unsigned char* p) const;

	//string make_filename(const string& name) const;

	psb_value_t* unpack(unsigned char*& p) const;

	template<class T> void unpack(T*& out, unsigned char*& p) const;

public:
	uint32_t get_chunk_index(unsigned char* p) const;

	unsigned char* buff;
	PSBHDR*        hdr;
	psb_array_t*   str1;
	psb_array_t*   str2;
	psb_array_t*   str3;
	psb_array_t*   strings;
	char*          strings_data;
	psb_array_t*   chunk_offsets;
	psb_array_t*   chunk_lengths;
	unsigned char* chunk_data;

	psb_objects_t* objects;
	psb_collection_t* expire_suffix_list;

	string         extension;
};

template<class T>
void
psb_objects_t::
unpack(T*& out, const string& name) const {
	out = NULL;

	unsigned char* temp = get_data(name);

	if (temp) {
		psb.unpack(out, temp);
	}
}

template<class T>
void
psb_collection_t::
unpack(T*& out, uint32_t index) const {
	unsigned char* temp = get(index);

	psb.unpack(out, temp);
}

template<class T>
void
psb_t::
unpack(T*& out, unsigned char*& p) const {
	out = dynamic_cast<T*>(unpack(p));
}


#endif
