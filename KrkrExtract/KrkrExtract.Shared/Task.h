#pragma once

#include <Windows.h>

class Xp3TaskExporter
{
public:

	virtual bool NTAPI TaskIsBeingTerminated() = 0;
};


