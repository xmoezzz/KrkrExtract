#pragma once

#include <string>
#include <my.h>

class ResourcePair
{
public:
	ULONG Index;
	std::wstring FileName;

	ResourcePair() :
		Index(0)
	{

	}

	ResourcePair(ULONG vIndex, std::wstring vFileName) :
		Index(vIndex),
		FileName(vFileName)
	{

	}

	ResourcePair& operator = (const ResourcePair& o)
	{
		this->Index = o.Index;
		this->FileName = o.FileName;

		return *this;
	}
};
