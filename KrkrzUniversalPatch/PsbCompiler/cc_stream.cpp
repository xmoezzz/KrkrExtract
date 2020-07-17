#include "cc_stream.h"
#include <memory>


cc_stream::cc_stream() :_position(0), _write_pos(0)
{
	_data = (unsigned char *)malloc(0);
	_length = 0;
}

cc_stream::cc_stream(unsigned char *data, uint32_t length) :_data(data), _length(length), _position(0), _write_pos(0)
{

}

cc_stream::~cc_stream()
{
	if (_data) {
		free(_data);
	}
}

uint8_t cc_stream::calc_integer_size(int64_t value)
{
	uint8_t bytes = 0;
	
	if(value < 0){
		value = ~value + 1;
	}
	if(value ==0) return 1;
	for(uint8_t i=0;i < sizeof(value);i++)
	{
		int64_t mask = (int64_t)1 << ((i * 8) - 1);

		if(value >> (i * 8) == 0){
			if(value & mask) {
				bytes++;
			}
			break;
		}
		bytes++;
	}

	return bytes;
}

unsigned char *cc_stream::copy_buffer()
{
	unsigned char *buff = new unsigned char[_write_pos];

	memcpy(buff, _data, _write_pos);

	return buff;
}

uint32_t cc_stream::get_length() {
	return _write_pos;
}

uint32_t cc_stream::get_position() {
	return _position;
}

unsigned char *cc_stream::get_data()
{
	return _data;
}

uint32_t cc_stream::get_private_length()
{
	return _length;
}

void cc_stream::write_integer(int64_t value, uint8_t size)
{
	if (size == 0) {
		size = calc_integer_size(value);
	}

	append((unsigned char *)&value, size);
}


void cc_stream::alloc(uint32_t size)
{
	if (_length > _position + size) {
		return;
	}

	_data = (unsigned char *)realloc(_data, _position + size + 0x100);
	_length = _position + size + 0x100;
}

void cc_stream::append(unsigned char *data, uint32_t size)
{
	alloc(size);

	_position = _write_pos;
	memcpy(&_data[_position], data, size);
	_position += size;
	_write_pos = _position;
}

void cc_stream::seek(uint32_t pos)
{
	if (_position < pos) {
		alloc(pos - _position);
	}

	_position = pos;
	if (_position > _write_pos) {
		_write_pos = _position;
	}
}
void cc_stream::move_next(uint32_t offset)
{
	seek(_position + offset);
}
void cc_stream::replace(uint32_t pos, const unsigned char *data, uint32_t length)
{
	if (pos + length > _length) {
		alloc(_length - (pos + length));
	}

	memcpy(&_data[pos], data, length);
}