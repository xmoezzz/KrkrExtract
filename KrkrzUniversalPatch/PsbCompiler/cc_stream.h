#pragma once

#include <stdint.h>

class cc_stream
{
public:
	cc_stream();
	cc_stream(unsigned char *data, uint32_t length);
	~cc_stream();

	static uint8_t calc_integer_size(int64_t value);

	//get_buffer return value need delete[]
	unsigned char *copy_buffer();
	unsigned char *get_data();
	uint32_t get_length();
	uint32_t get_position();

	void seek(uint32_t pos);
	void move_next(uint32_t offset);

	void replace(uint32_t pos, const unsigned char *data, uint32_t length);

	void write_integer(int64_t value, uint8_t size = 0);

	//template struct...
	template <class T>
	void inline write_t(T value) {
		append((unsigned char *)&value, sizeof(value));
	}

	void append(unsigned char *data, uint32_t size);

protected:
	void alloc(uint32_t size);
	
	
	uint32_t get_private_length();

private:
	unsigned char *_data;
	uint32_t _length;
	uint32_t _position;
	uint32_t _write_pos;
};

