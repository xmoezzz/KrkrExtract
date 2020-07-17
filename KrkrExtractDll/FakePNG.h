#pragma once

#include "my.h"
#include "tp_stub.h"
#include <string>

using std::wstring;

Void WINAPI InitLayer();
Void WINAPI SavePng(LPCWSTR FileName, LPCWSTR Path);

HRESULT WINAPI ImageWorkerV2Link(iTVPFunctionExporter *exporter);
