#pragma once

#include "SafeMemory.h"

std::shared_ptr<BYTE> GetDataFromMDF(_In_ PBYTE Buffer, _Inout_ ULONG &Size);
std::shared_ptr<BYTE> GetDataFromLZ4(_In_ PBYTE Buffer, _Inout_ ULONG &Size);
std::shared_ptr<BYTE> GetDataFromMDF(_In_ std::shared_ptr<BYTE> Buffer, _Inout_ ULONG &Size);
std::shared_ptr<BYTE> GetDataFromLZ4(_In_ std::shared_ptr<BYTE> Buffer, _Inout_ ULONG &Size);
