
#ifndef __STRING_UTILITY_H__
#define __STRING_UTILITY_H__

#include <string>
#include <iostream>
#include <algorithm>
#include <locale>

struct equal_char_ignorecase {
	inline bool operator()(char x, char y) const {
		std::locale loc;
		return std::tolower(x,loc) == std::tolower(y,loc);
	}
};

inline bool icomp( const std::string& x, const std::string& y ) {
	return x.size() == y.size() && std::equal(x.begin(), x.end(), y.begin(), equal_char_ignorecase());
}

struct equal_wchar_ignorecase {
	inline bool operator()(wchar_t x, wchar_t y) const {
		std::locale loc;
		return std::tolower(x,loc) == std::tolower(y,loc);
	}
};
inline bool icomp( const std::wstring& x, const std::wstring& y ) {
	return x.size() == y.size() && std::equal(x.begin(), x.end(), y.begin(), equal_wchar_ignorecase());
}

inline std::string Trim( const std::string& val ) {
	static const char* TRIM_STR=" \01\02\03\04\05\06\a\b\t\n\v\f\r\x0E\x0F\x7F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F";
	std::string::size_type pos = val.find_first_not_of( TRIM_STR );
	std::string::size_type lastpos = val.find_last_not_of( TRIM_STR );
	if( pos == lastpos ) {
		if( pos == std::string::npos ) {
			return val;
		} else {
			return val.substr(pos,1);
		}
	} else {
		std::string::size_type len = lastpos - pos + 1;
		return val.substr(pos,len);
	}
}
inline std::wstring Trim( const std::wstring& val ) {
	static const wchar_t* TRIM_STR=L" \01\02\03\04\05\06\a\b\t\n\v\f\r\x0E\x0F\x7F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F";
	std::wstring::size_type pos = val.find_first_not_of( TRIM_STR );
	std::wstring::size_type lastpos = val.find_last_not_of( TRIM_STR );
	if( pos == lastpos ) {
		if( pos == std::wstring::npos ) {
			return val;
		} else {
			return val.substr(pos,1);
		}
	} else {
		std::wstring::size_type len = lastpos - pos + 1;
		return val.substr(pos,len);
	}
}

#endif // __STRING_UTILITY_H__
