#pragma once

#include "my.h"

class tTJSCriticalSection
{
	CRITICAL_SECTION CS;
public:
	tTJSCriticalSection() { InitializeCriticalSection(&CS); }
	~tTJSCriticalSection() { DeleteCriticalSection(&CS); }

	void Enter() { EnterCriticalSection(&CS); }
	void Leave() { LeaveCriticalSection(&CS); }
};

class tTJSCriticalSectionHolder
{
	tTJSCriticalSection *Section;
public:
	tTJSCriticalSectionHolder(tTJSCriticalSection &cs)
	{
		Section = &cs;
		Section->Enter();
	}

	~tTJSCriticalSectionHolder()
	{
		Section->Leave();
	}

};



