#pragma once

#include <Windows.h>
#include "Task.h"

class KrkrClientProxyer;

enum class PackInfo : ULONG
{
	KrkrZ                 = 0,
	KrkrZ_V2              = 1,
	NormalPack            = 2,
	NormalPack_NoExporter = 3,
	KrkrZ_SenrenBanka     = 4,
	KrkrZ_SenrenBanka_V2  = 5,
	Krkr_Custom           = 100,
	UnknownPack           = 0xFFFFFFFF
};


enum class ChunkNodeKind : ULONG
{
	FILE_CHUNK_NODE           = 0,
	M2_CHUNK_NODE             = 1,
	M2_COMPRESSED_NODE        = 2,
	M2_COMPRESSED_NODE_V2     = 3,
	FILE_CHUNK_NODE_PROTECTED = 4,
	OTHER_NODE                = 80,
	USER_CHUNK_NODE           = 100
};

class Xp3StatusNode
{
public:
	virtual ChunkNodeKind NTAPI GetKind()   = 0;
	virtual PBYTE         NTAPI GetBuffer() = 0;
	virtual ULONG         NTAPI GetSize()   = 0;
};


class Xp3StatusValidator
{
public:
	virtual ChunkNodeKind NTAPI GetKind() = 0;
	virtual PCWSTR        NTAPI GetName() = 0;
	virtual BOOL          NTAPI Validate(PBYTE Buffer, ULONG Size, DWORD& Magic) = 0;
};



class Xp3ItemAccess
{
public:

	virtual bool NTAPI SetFileName  (PCWSTR FileName) = 0;
	virtual bool NTAPI SetM2FileName(PCWSTR FileName) = 0;
	virtual bool NTAPI SetFileNameNullTerminated(BOOL State)   = 0;
	virtual bool NTAPI SetM2FileNameNullTerminated(BOOL State) = 0;
	virtual bool NTAPI SetM2Item() = 0;
	virtual PCWSTR NTAPI GetFileName()   = 0;
	virtual PCWSTR NTAPI GetM2FileName() = 0;
	virtual bool NTAPI   GetFileNameNullTerminated()    = 0;
	virtual bool NTAPI   GetM2GFileNameNullTerminated() = 0;
	virtual bool NTAPI   IsM2Item() = 0;
};


class Xp3IndexAccess
{
public:
	typedef bool (CALLBACK* ENUMXP3INDEXPROC)(Xp3ItemAccess*, void*);

	virtual ULONG NTAPI GetCount() = 0;
	virtual void  NTAPI Push(Xp3ItemAccess* Item) = 0;
	virtual void  NTAPI SetM2(BOOL IsM2) = 0;
	virtual void  NTAPI SetSpecialChunkM2(ULONG M2SecondaryMagic) = 0;
	virtual ULONG NTAPI GetSpecialChunkM2() = 0;
};


class Xp3ArchivePlugin
{
public:
	virtual bool NTAPI Register(_In_ PCWSTR keVersion, _Out_ PWSTR PluginName) = 0;
	virtual bool NTAPI WalkXp3Index(Xp3TaskExporter* Exporter, Xp3IndexAccess* Xp3Index) = 0;
	virtual bool NTAPI MakeArchive(
		Xp3TaskExporter*   Exporter,
		KrkrClientProxyer* Proxyer, 
		PCWSTR             BaseDir, 
		PCWSTR             OriginalArchive, 
		PCWSTR             OutputArchive
	) = 0;

	virtual void* NTAPI GetPluginModuleHandle() = 0;
};

//
// TODO
//
class Xp3ArchivePlugin2;



