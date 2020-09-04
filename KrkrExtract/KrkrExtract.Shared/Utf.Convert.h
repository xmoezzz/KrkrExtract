#pragma once

#include <stdint.h>
#include <string>

#pragma warning(disable : 4333)

inline int UtilWideCharToUtf8(wchar_t in, char * out)
{
	// convert a wide character 'in' to utf-8 character 'out'
	if (in < (1 << 7))
	{
		if (out)
		{
			out[0] = (char)in;
		}
		return 1;
	}
	else if (in < (1 << 11))
	{
		if (out)
		{
			out[0] = (char)(0xc0 | (in >> 6));
			out[1] = (char)(0x80 | (in & 0x3f));
		}
		return 2;
	}
	else if (in < (1 << 16))
	{
		if (out)
		{
			out[0] = (char)(0xe0 | (in >> 12));
			out[1] = (char)(0x80 | ((in >> 6) & 0x3f));
			out[2] = (char)(0x80 | (in & 0x3f));
		}
		return 3;
	}
	else if (in < (1 << 21))
	{
		if (out)
		{
			out[0] = (char)(0xf0 | (in >> 18));
			out[1] = (char)(0x80 | ((in >> 12) & 0x3f));
			out[2] = (char)(0x80 | ((in >> 6) & 0x3f));
			out[3] = (char)(0x80 | (in & 0x3f));
		}
		return 4;
	}
	else if (in < (1 << 26))
	{
		if (out)
		{
			out[0] = (char)(0xf8 | (in >> 24));
			out[1] = (char)(0x80 | ((in >> 16) & 0x3f));
			out[2] = (char)(0x80 | ((in >> 12) & 0x3f));
			out[3] = (char)(0x80 | ((in >> 6) & 0x3f));
			out[4] = (char)(0x80 | (in & 0x3f));
		}
		return 5;
	}
	else if (in < (1 << 31))
	{
		if (out)
		{
			out[0] = (char)(0xfc | (in >> 30));
			out[1] = (char)(0x80 | ((in >> 24) & 0x3f));
			out[2] = (char)(0x80 | ((in >> 18) & 0x3f));
			out[3] = (char)(0x80 | ((in >> 12) & 0x3f));
			out[4] = (char)(0x80 | ((in >> 6) & 0x3f));
			out[5] = (char)(0x80 | (in & 0x3f));
		}
		return 6;
	}
	return -1;
}

//---------------------------------------------------------------------------
inline int UtilWideCharToUtf8String(const wchar_t *in, char * out)
{
	// convert input wide string to output utf-8 string
	int count = 0;
	while (*in)
	{
		int n;
		if (out)
		{
			n = UtilWideCharToUtf8(*in, out);
			out += n;
		}
		else
		{
			n = UtilWideCharToUtf8(*in, NULL);
			/*
				in this situation, the compiler's inliner
				will collapse all null check parts in
				TVPWideCharToUtf8.
			*/
		}
		if (n == -1) return -1; // invalid character found
		count += n;
		in++;
	}
	return count;
}

//---------------------------------------------------------------------------
inline bool UtilUtf8ToWideChar(const char * & in, wchar_t *out)
{
	// convert a utf-8 charater from 'in' to wide charater 'out'
	const unsigned char * & p = (const unsigned char * &)in;
	if (p[0] < 0x80)
	{
		if (out) *out = (wchar_t)in[0];
		in++;
		return true;
	}
	else if (p[0] < 0xc2)
	{
		// invalid character
		return false;
	}
	else if (p[0] < 0xe0)
	{
		// two bytes (11bits)
		if ((p[1] & 0xc0) != 0x80) return false;
		if (out) *out = ((p[0] & 0x1f) << 6) + (p[1] & 0x3f);
		in += 2;
		return true;
	}
	else if (p[0] < 0xf0)
	{
		// three bytes (16bits)
		if ((p[1] & 0xc0) != 0x80) return false;
		if ((p[2] & 0xc0) != 0x80) return false;
		if (out) *out = ((p[0] & 0x1f) << 12) + ((p[1] & 0x3f) << 6) + (p[2] & 0x3f);
		in += 3;
		return true;
	}
	else if (p[0] < 0xf8)
	{
		// four bytes (21bits)
		if ((p[1] & 0xc0) != 0x80) return false;
		if ((p[2] & 0xc0) != 0x80) return false;
		if ((p[3] & 0xc0) != 0x80) return false;
		if (out) *out = ((p[0] & 0x07) << 18) + ((p[1] & 0x3f) << 12) +
			((p[2] & 0x3f) << 6) + (p[3] & 0x3f);
		in += 4;
		return true;
	}
	else if (p[0] < 0xfc)
	{
		// five bytes (26bits)
		if ((p[1] & 0xc0) != 0x80) return false;
		if ((p[2] & 0xc0) != 0x80) return false;
		if ((p[3] & 0xc0) != 0x80) return false;
		if ((p[4] & 0xc0) != 0x80) return false;
		if (out) *out = ((p[0] & 0x03) << 24) + ((p[1] & 0x3f) << 18) +
			((p[2] & 0x3f) << 12) + ((p[3] & 0x3f) << 6) + (p[4] & 0x3f);
		in += 5;
		return true;
	}
	else if (p[0] < 0xfe)
	{
		// six bytes (31bits)
		if ((p[1] & 0xc0) != 0x80) return false;
		if ((p[2] & 0xc0) != 0x80) return false;
		if ((p[3] & 0xc0) != 0x80) return false;
		if ((p[4] & 0xc0) != 0x80) return false;
		if ((p[5] & 0xc0) != 0x80) return false;
		if (out) *out = ((p[0] & 0x01) << 30) + ((p[1] & 0x3f) << 24) +
			((p[2] & 0x3f) << 18) + ((p[3] & 0x3f) << 12) +
			((p[4] & 0x3f) << 6) + (p[5] & 0x3f);
		in += 6;
		return true;
	}
	return false;
}
//---------------------------------------------------------------------------

inline int UtilUtf8ToWideCharString(const char * in, wchar_t *out)
{
	// convert input utf-8 string to output wide string
	int count = 0;
	while (*in)
	{
		wchar_t c;
		if (out)
		{
			if (!UtilUtf8ToWideChar(in, &c))
				return -1; // invalid character found
			*out++ = c;
		}
		else
		{
			if (!UtilUtf8ToWideChar(in, NULL))
				return -1; // invalid character found
		}
		count++;
	}
	return count;
}

//---------------------------------------------------------------------------

inline int UtilUtf8ToWideCharString(const char * in, unsigned int length, wchar_t *out)
{
	// convert input utf-8 string to output wide string
	int count = 0;
	const char *end = in + length;
	while (*in && in < end)
	{
		if (in + 6 > end)
		{
			// fetch utf-8 character length
			const unsigned char ch = *(const unsigned char *)in;

			if (ch >= 0x80)
			{
				unsigned int len = 0;

				if (ch < 0xc2) return -1;
				else if (ch < 0xe0) len = 2;
				else if (ch < 0xf0) len = 3;
				else if (ch < 0xf8) len = 4;
				else if (ch < 0xfc) len = 5;
				else if (ch < 0xfe) len = 6;
				else return -1;

				if (in + len > end) return -1;
			}
		}

		wchar_t c;
		if (out)
		{
			if (!UtilUtf8ToWideChar(in, &c))
				return -1; // invalid character found
			*out++ = c;
		}
		else
		{
			if (!UtilUtf8ToWideChar(in, NULL))
				return -1; // invalid character found
		}
		count++;
	}
	return count;
}


#pragma warning(default : 4333)

inline std::string Utf16ToUtf8(const std::wstring& String) 
{
	int          Size;
	std::string  Utf8String;

	Size = UtilWideCharToUtf8String(String.c_str(), nullptr);
	if (Size == (int)-1)
		return {};

	try {
		Utf8String.resize(Size);
	}
	catch (...) {
		return {};
	}

	memset((void*)Utf8String.data(), 0, Utf8String.size() * sizeof(char));
	UtilWideCharToUtf8String(String.c_str(), (char*)Utf8String.data());
	return Utf8String;
}

inline std::string Utf16ToUtf8(const wchar_t* String)
{
	int          Size;
	std::string  Utf8String;

	Size = UtilWideCharToUtf8String(String, nullptr);
	if (Size == (int)-1)
		return {};

	try {
		Utf8String.resize(Size);
	}
	catch (...) {
		return {};
	}

	memset((void*)Utf8String.data(), 0, Utf8String.size() * sizeof(char));
	UtilWideCharToUtf8String(String, (char*)Utf8String.data());
	return Utf8String;
}

inline std::wstring Utf8ToUtf16(const std::string& String)
{
	int          Size;
	std::wstring Utf16String;

	Size = UtilUtf8ToWideCharString(String.c_str(), String.length(), nullptr);
	if (Size == (int)-1)
		return {};

	try {
		Utf16String.resize(Size);
	}
	catch (...) {
		return {};
	}

	memset((void*)Utf16String.data(), 0, Utf16String.size() * sizeof(wchar_t));
	UtilUtf8ToWideCharString(String.c_str(), String.length(), (wchar_t*)Utf16String.data());
	return Utf16String;
}


inline std::wstring Utf8ToUtf16(const char* String)
{
	int          Size;
	std::wstring Utf16String;

	if (!String)
		return {};

	Size = UtilUtf8ToWideCharString(String, nullptr);
	if (Size == (int)-1)
		return {};

	try {
		Utf16String.resize(Size);
	}
	catch (...) {
		return {};
	}

	memset((void*)Utf16String.data(), 0, Utf16String.size() * sizeof(wchar_t));
	UtilUtf8ToWideCharString(String, lstrlenA(String), (wchar_t*)Utf16String.data());
	return Utf16String;
}




