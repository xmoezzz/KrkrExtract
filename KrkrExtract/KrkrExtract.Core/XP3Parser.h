#pragma once

#include "KrkrHeaders.h"
#include "StatusMatcher.h"
#include <KrkrClientProxyer.h>
#include <zlib.h>

//
// TODO : fix this bad algorithm
//

NTSTATUS 
FindEmbededXp3OffsetSlow(
	_In_  NtFileDisk&    File, 
	_Out_ PLARGE_INTEGER Offset
);

BOOL IsBuiltinChunk(DWORD ChunkMagic);

class Xp3WalkerProxy : public Xp3IndexAccess
{
public:
	Xp3WalkerProxy() {} ;
	Xp3WalkerProxy(const Xp3WalkerProxy& Other) :
		m_IsM2Krkr(Other.IsM2Krkr()),
		m_ExtractFile(Other.IsExtractFile())
	{
		m_Items = GetItems();
	}

	void AppendItem(XP3Index& Item) { m_Items.push_back(Item); };
	void SetM2Krkr()      { m_IsM2Krkr = TRUE; };
	void SetNormalKrkr()  { m_IsM2Krkr = FALSE; };
	void SetExtractFile() { m_ExtractFile = TRUE; };
	void SetPackInfo(PackInfo Info) { m_PackInfo = Info; };
	bool IsM2Krkr() const { return m_IsM2Krkr; };
	bool IsExtractFile() const        { return m_ExtractFile; };
	std::vector<XP3Index>& GetItems() { return m_Items; };

	void  NTAPI SetSpecialChunkM2(ULONG M2SecondaryMagic) { m_M2SpecialChunkMagic = M2SecondaryMagic; };
	ULONG NTAPI GetSpecialChunkM2() { return m_M2SpecialChunkMagic; };
	ULONG NTAPI GetCount() { return m_Items.size(); };
	PackInfo NTAPI GetPackInfo() { return m_PackInfo; };
	void  NTAPI Push(Xp3ItemAccess* Item) 
	{
		XP3Index Xp3Item(Item);
		return m_Items.push_back(Xp3Item); 
	};

	void  NTAPI SetM2(BOOL IsM2) { SetM2Krkr(); };
	
private:
	std::vector<XP3Index> m_Items;
	BOOL                  m_IsM2Krkr            = FALSE;
	BOOL                  m_ExtractFile         = FALSE;
	BOOL                  m_M2SpecialChunkMagic = 0;
	PackInfo              m_PackInfo = PackInfo::NormalPack;
};

NTSTATUS
ConvertToGeneralXp3Chunk(
	_In_    PBYTE Data,
	_In_    ULONG64 Size,
	_Inout_ std::vector<Xp3ItemGeneral>& Items,
	_Out_   DWORD& Magic
);

PackInfo MatchXp3ArchiveKind(std::vector<Xp3ItemGeneral>& Chunks);

#define WalkXp3IndexCallbackM(KrkrProxyer, Buffer, Size, File, Magic, Proxyer) \
	[&] (KrkrClientProxyer* KrkrProxyer, PBYTE Buffer, ULONG64 Size, NtFileDisk& File, DWORD Magic, Xp3WalkerProxy& Proxyer) -> NTSTATUS


#define WalkXp3IndexCallbackDummy \
	WalkXp3IndexCallbackM(KrkrProxyer, Buffer, Size, File, Magic, Proxyer) { return STATUS_SUCCESS; }


NTSTATUS
IsXp3File(
	_In_ NtFileDisk& File,
	_Out_ BOOL& IsXp3
);

NTSTATUS 
IsXp3File(
	_In_ PCWSTR FileName,  
	_Out_ BOOL& IsXp3
);


enum class WalkerCallbackStatus : ULONG
{
	STATUS_SKIP     = 0,
	STATUS_CONTINUE = 1,
	STATUS_ERROR    = 2,
};

typedef WalkerCallbackStatus (*SENRENBANKACALLBACK)(
	KrkrClientProxyer* Proxyer, 
	std::shared_ptr<BYTE> CompressedIndexBuffer, 
	ULONG CompressedSize,
	ULONG DecompressedSize,
	NtFileDisk& File,
	PVOID UserContext
	);

NTSTATUS
WalkSenrenBankaIndexBuffer(
	_In_ KrkrClientProxyer* Proxyer,
	_In_ PBYTE Decompress,
	_In_ ULONG Size,
	_In_ NtFileDisk& File,
	_In_ DWORD M2ChunkMagic,
	_In_ Xp3WalkerProxy& Proxy,
	_In_ SENRENBANKACALLBACK Callback = nullptr,
	_In_ PVOID CallbackPrivateContext = nullptr
);


NTSTATUS
WalkSenrenBankaIndexV2Buffer(
	_In_ KrkrClientProxyer* Proxyer,
	_In_ PBYTE Decompress,
	_In_ ULONG Size,
	_In_ NtFileDisk& File,
	_In_ DWORD M2ChunkMagic,
	_In_ Xp3WalkerProxy& Proxy,
	_In_ SENRENBANKACALLBACK Callback = nullptr,
	_In_ PVOID CallbackPrivateContext = nullptr
);


NTSTATUS
WalkNormalIndexBuffer(
	_In_ KrkrClientProxyer* Proxyer,
	_In_ PBYTE Decompress,
	_In_ ULONG Size,
	_In_ NtFileDisk& File,
	_In_ DWORD M2ChunkMagic,
	_In_ Xp3WalkerProxy& Proxy
);


NTSTATUS
WalkNekoVol0IndexBuffer(
	_In_ KrkrClientProxyer* Proxyer,
	_In_ PBYTE Decompress,
	_In_ ULONG Size,
	_In_ NtFileDisk& File,
	_In_ DWORD M2ChunkMagic,
	_In_ Xp3WalkerProxy& Proxy
);


NTSTATUS
WalkKrkrZIndexBuffer(
	_In_ KrkrClientProxyer* Proxyer,
	_In_ PBYTE Decompress,
	_In_ ULONG Size,
	_In_ NtFileDisk& File,
	_In_ DWORD M2ChunkMagic,
	_In_ Xp3WalkerProxy& Proxy
);



NTSTATUS
ReadXp3FileChunk(
	PBYTE Buffer,
	ULONG Size,
	ULONG Offset,
	KRKR2_XP3_INDEX_CHUNK_FILE& FileChunk,
	ULONG& ByteTransferred
);

NTSTATUS
ReadXp3AdlrChunk(
	PBYTE Buffer,
	ULONG Size,
	ULONG Offset,
	KRKR2_XP3_INDEX_CHUNK_ADLR& AdlrChunk,
	ULONG& ByteTransferred
);

NTSTATUS
ReadXp3TimeChunk(
	PBYTE Buffer,
	ULONG Size,
	ULONG Offset,
	KRKR2_XP3_INDEX_CHUNK_TIME& TimeChunk,
	ULONG& ByteTransferred
);

NTSTATUS
ReadXp3SegmChunk(
	PBYTE Buffer,
	ULONG Size,
	ULONG Offset,
	KRKR2_XP3_INDEX_CHUNK_SEGM& SegmChunk,
	ULONG& ByteTransferred
);

NTSTATUS
ReadXp3InfoChunk(
	PBYTE Buffer,
	ULONG Size,
	ULONG Offset,
	KRKR2_XP3_INDEX_CHUNK_INFO& InfoChunk,
	ULONG& ByteTransferred,
	BOOL&  NullTerminated
);

NTSTATUS
ReadXp3M2InfoChunk(
	PBYTE Buffer,
	ULONG Size,
	ULONG Offset,
	KRKRZ_XP3_INDEX_CHUNK_YUZU& M2InfoChunk,
	ULONG& ByteTransferred,
	BOOL&  NullTerminated
);


NTSTATUS
ReadXp3M2CompressedChunk(
	PBYTE Buffer,
	ULONG Size,
	ULONG Offset,
	KRKRZ_XP3_INDEX_CHUNK_COMPRESSED& CompressedChunk,
	ULONG& ByteTransferred,
	BOOL&  NullTerminated
);

NTSTATUS
ReadXp3M2CompressedChunk(
	PBYTE Buffer,
	ULONG Size,
	ULONG Offset,
	KRKRZ_XP3_INDEX_CHUNK_COMPRESSED_V2& CompressedChunk,
	ULONG& ByteTransferred,
	BOOL&  NullTerminated
);

NTSTATUS
ReadXp3UnknownChunk(
	PBYTE Buffer,
	ULONG Size,
	ULONG Offset,
	ULONG& ByteTransferred
);


BOOL CheckItem(XP3Index& Item);
BOOL CheckItem(KRKR2_XP3_INDEX_CHUNK_INFO& Item);


