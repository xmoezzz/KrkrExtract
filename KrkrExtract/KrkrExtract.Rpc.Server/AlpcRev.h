#pragma once

#include <my.h>

class AlpcRev
{
public:
	virtual BOOL ParseMessage(PVOID MessageBuffer, SIZE_T MessageSize) = 0;
};


