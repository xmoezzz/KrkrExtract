#pragma once

#include "my.h"
#include <string>

///[-] Remove this feature?
///[+] 1.Can I create two tjs compilers in the same process?
///[+] 2.Some useless interfaces(make code too complex and hard to debug)

NTSTATUS FASTCALL TjsDecompileStorage(IStream* Stream, std::wstring& FileName);

