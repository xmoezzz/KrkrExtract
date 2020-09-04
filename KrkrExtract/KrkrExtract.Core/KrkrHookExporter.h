#pragma once

#include <my.h>

class KrkrExtractCore;

class KrkrHookExporter
{
public:
	virtual KrkrExtractCore* GetInstanceForHook() = 0;
};


enum class HookMode : ULONG
{
	HOOK_NATIVE = 0,
	HOOK_EPT    = 1
};


