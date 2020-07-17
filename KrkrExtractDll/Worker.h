#pragma once

#include "my.h"

HANDLE NTAPI StartPacker();
HANDLE NTAPI StartDumper(LPCWSTR lpFileName);
HANDLE NTAPI StartMiniDumper(LPCWSTR lpFileName);
HANDLE NTAPI StartExtDumper();

NTSTATUS NTAPI AddUnpackFile(LPCWSTR FileName);
