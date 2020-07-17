#ifndef _DecryptionCode_
#define _DecryptionCode_

#include <Windows.h>
#include "tp_stub.h"
#include "CMem.h"
#include <cctype>
#include <string>

using std::wstring;

tTVPXP3ArchiveExtractionFilter GetExtractionFilter();

wstring ToLowerString(const WCHAR* lpString);

#endif
