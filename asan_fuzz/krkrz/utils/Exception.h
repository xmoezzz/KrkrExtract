
#ifndef __EXCEPTION_H__
#define __EXCEPTION_H__


class Exception /*: public std::exception*/ {
	std::wstring message_;
public:
	Exception( const std::wstring& mes ) : message_(mes) {
	}
	virtual const wchar_t* what() const {
		return message_.c_str();
	}
};

class EAbort : public Exception {
public:
	EAbort( const wchar_t* mes ) : Exception(std::wstring(mes)) {
	}
};

#endif // __EXCEPTION_H__
