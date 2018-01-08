#pragma once

#include "my.h"
#include <string>


NTSTATUS FASTCALL TjsDecompileStorage(IStream* Stream, std::wstring& FileName);

