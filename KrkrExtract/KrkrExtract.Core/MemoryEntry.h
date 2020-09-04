#pragma once

#include <my.h>
#include <memory>

class MemEntry
{
public:
	MemEntry() : Size(0), Hash(0) {}

	MemEntry& operator = (const MemEntry& Other)
	{
		Buffer = Other.Buffer;
		Size = Other.Size;
		Hash = Other.Hash;

		return *this;
	}

	std::shared_ptr<BYTE> Buffer;
	ULONG   Size;
	ULONG64 Hash;
};

