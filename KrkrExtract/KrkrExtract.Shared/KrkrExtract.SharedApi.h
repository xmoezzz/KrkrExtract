#pragma once

#include "KrkrClientProxyer.h"
#include <tp_stub.h>


class StringView
{
public:
	virtual void  Assign(PCWSTR Buffer) = 0;
	virtual ULONG Length() = 0;
};

class CommandPlugin
{
public:
	virtual PCWSTR NTAPI GetCommandName()     = 0;
	virtual BOOL   NTAPI Exec(PWSTR Command) = 0;
	virtual BOOL   NTAPI Help(StringView* HelperString) = 0;
};




