#pragma once

#include <my.h>
#include <Xp3Access.h>
#include <unordered_map>
#include "ChunkReader.h"


class Xp3FileNodeValidator : public Xp3StatusValidator
{
public:
	ChunkNodeKind NTAPI GetKind();
	PCWSTR        NTAPI GetName();
	BOOL          NTAPI Validate(PBYTE Buffer, ULONG Size, DWORD& Magic);
};


class Xp3FileProtectedNodeValidator : public Xp3StatusValidator
{
public:
	ChunkNodeKind NTAPI GetKind();
	PCWSTR        NTAPI GetName();
	BOOL          NTAPI Validate(PBYTE Buffer, ULONG Size, DWORD& Magic);
};


class Xp3M2ChunkNodeValidator : public Xp3StatusValidator
{
public:
	ChunkNodeKind NTAPI GetKind();
	PCWSTR        NTAPI GetName();
	BOOL          NTAPI Validate(PBYTE Buffer, ULONG Size, DWORD& Magic);
};

class Xp3M2CompressedChunkNodeValidator : public Xp3StatusValidator
{
public:
	ChunkNodeKind NTAPI GetKind();
	PCWSTR        NTAPI GetName();
	BOOL          NTAPI Validate(PBYTE Buffer, ULONG Size, DWORD& Magic);
};

//
// without product name
//

class Xp3M2CompressedChunkNodeValidatorV2 : public Xp3StatusValidator
{
public:
	ChunkNodeKind NTAPI GetKind();
	PCWSTR        NTAPI GetName();
	BOOL          NTAPI Validate(PBYTE Buffer, ULONG Size, DWORD& Magic);
};

class Xp3M2CompressedChunkNodeValidatorV3 : public Xp3StatusValidator
{
public:
	ChunkNodeKind NTAPI GetKind();
	PCWSTR        NTAPI GetName();
	BOOL          NTAPI Validate(PBYTE Buffer, ULONG Size, DWORD& Magic);
};
