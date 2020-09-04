#pragma once

#include <my.h>
#include <unordered_set>
#include <string>
#include <atomic>
#include <functional>
#include "tp_stub.h"
#include "XP3Parser.h"
#include "NativeThread.h"
#include "KrkrExtract.SharedApi.h"
#include "CoTaskBase.h"
#include "KrkrClientProxyer.h"

class CoPackerTask final : public CoTaskBase, public Xp3TaskExporter
{
public:
	CoPackerTask() = delete;
	CoPackerTask(const CoPackerTask&) = delete;
	~CoPackerTask();

	CoPackerTask(KrkrClientProxyer* Proxyer, PCWSTR BaseDir, PCWSTR OriginalArchiveName, PCWSTR OutputArchiveName);
	bool NTAPI TaskIsBeingTerminated() { return IsBeingTerminated(); };

private:
	
	enum class KrkrPackerPass
	{
		NORMAL   = 0,
		ONE_PASS = 1,
		TWO_PASS = 2,
	};

	void ThreadFunction();

	NTSTATUS DetactArchiveFormat(PCWSTR FileName, Xp3WalkerProxy& Xp3Proxyer);
	NTSTATUS MakeKrkrCompatibleArchive();

	//
	// kirikiri 2
	//

	NTSTATUS MakeKrkr2CompatibleArchiveGeneral(KrkrPackerPass PassStatus, std::wstring& OutputArchive);

	//
	// sabbat of witch
	//

	NTSTATUS MakeKrkrZCompatibleArchiveGeneral(KrkrPackerPass PassStatus, std::wstring& OutputArchive);
	
	//
	// nekopara vol0
	//

	NTSTATUS MakeKrkrNekoCompatibleArchiveGeneral(KrkrPackerPass PassStatus, std::wstring& OutputArchive);

	//
	// SenrenBanka
	//

	NTSTATUS MakeKrkrSenrenBankaCompatibleArchiveGeneral(KrkrPackerPass PassStatus, std::wstring& OutputArchive);

	//
	// CafeStella
	//

	NTSTATUS MakeKrkrSenrenBankaCompatibleArchiveGeneralV2(KrkrPackerPass PassStatus, std::wstring& OutputArchive);
	

	NTSTATUS GetSenrenBankaPackInfo  (PBYTE IndexData, ULONG IndexSize, NtFileDisk& File);
	NTSTATUS GetSenrenBankaPackInfoV2(PBYTE IndexData, ULONG IndexSize, NtFileDisk& File);
	NTSTATUS IterFiles(LPCWSTR Path);

	NTSTATUS InitializeTimeIndex(KRKR2_XP3_INDEX_CHUNK_TIME& TimeChunk);
	NTSTATUS InitializeAdlrIndex(KRKR2_XP3_INDEX_CHUNK_ADLR& AldrChunk);
	NTSTATUS InitializeSegmIndex(KRKR2_XP3_INDEX_CHUNK_SEGM& SegmChunk);
	NTSTATUS InitializeInfoIndex(KRKR2_XP3_INDEX_CHUNK_INFO& InfoChunk);
	NTSTATUS InitializeFileIndex(KRKR2_XP3_INDEX_CHUNK_FILE& FileChunk);
	NTSTATUS InitializeYuzuIndex(KRKRZ_XP3_INDEX_CHUNK_YUZU& YuzuChunk);
	NTSTATUS InitializeCompIndex(KRKRZ_XP3_INDEX_CHUNK_COMPRESSED& CompChunk);
	NTSTATUS InitializeCompIndex(KRKRZ_XP3_INDEX_CHUNK_COMPRESSED_V2& CompChunk);

	NTSTATUS ReadWriteEncrypt(NtFileDisk& Source, NtFileDisk& Dest);
	NTSTATUS ReadWriteEncrypt(std::wstring& Source, NtFileDisk& Dest, LARGE_INTEGER& Size);
	
	
	ULONG64 CalculateChunkSize(KRKR2_XP3_INDEX_CHUNK_TIME& TimeChunk);
	ULONG64 CalculateChunkSize(KRKR2_XP3_INDEX_CHUNK_ADLR& AdlrChunk);
	ULONG64 CalculateChunkSize(KRKR2_XP3_INDEX_CHUNK_SEGM& SegmChunk);
	ULONG64 CalculateChunkSize(KRKR2_XP3_INDEX_CHUNK_INFO& InfoChunk);
	ULONG64 CalculateChunkSize(KRKRZ_XP3_INDEX_CHUNK_YUZU& YuzuChunk);
	ULONG64 CalculateFileSize(
		KRKR2_XP3_INDEX_CHUNK_TIME Time,
		KRKR2_XP3_INDEX_CHUNK_ADLR Adlr,
		KRKR2_XP3_INDEX_CHUNK_SEGM Segm,
		KRKR2_XP3_INDEX_CHUNK_INFO Info
	);

	ULONG64 CalculateHeaderSize(KRKRZ_XP3_INDEX_CHUNK_COMPRESSED& Header);

	constexpr ULONG64 CalculateCommonHeaderSize();

	std::vector<BYTE> SerializeToArray(SMyXP3IndexNormal& FileChunk);
	std::vector<BYTE> SerializeToArray(SMyXP3IndexM2&     FileChunk);
	std::vector<BYTE> SerializeToArray(KRKRZ_XP3_INDEX_CHUNK_YUZU& M2Chunk);
	std::vector<BYTE> SerializeToArray(KRKRZ_XP3_INDEX_CHUNK_COMPRESSED& M2CompressedChunk);
	std::vector<BYTE> SerializeToArray(KRKRZ_XP3_INDEX_CHUNK_COMPRESSED_V2& M2CompressedChunk);

	ttstr GetAutoSearchPath(std::wstring& FileName);

	void DecryptWorker(ULONG64 EncryptOffset, PBYTE Buffer, ULONG BufferSize, ULONG Hash);

public:
	PCWSTR GetBaseDir()             { return m_BaseDir.c_str(); };
	PCWSTR GetOriginalArchivePath() { return m_OriginalArchivePath.c_str(); };
	PCWSTR GetOutputArchivePath()   { return m_OutputArchivePath.c_str();  }
	KrkrClientProxyer* GetKrkrClientProxyer() { return m_Proxyer; }

private:
	wstring                             m_BaseDir;
	wstring                             m_OriginalArchivePath;
	wstring                             m_OutputArchivePath;
	wstring                             m_TemplateFileName;
	PackInfo                            m_ArchiveType = PackInfo::UnknownPack;
	vector<wstring>                     m_FileList;
	BYTE                                m_DecryptionKey;
	DWORD                               m_M2ChunkMagic     = 0;
	BOOL                                m_InfoNameZeroEnd  = TRUE;
	BOOL                                m_M2NameZeroEnd    = TRUE;
	BOOL                                m_M2ProductZeroEnd = TRUE;
	KRKRZ_XP3_INDEX_CHUNK_COMPRESSED    m_OriginalCompressedChunk;
	KRKRZ_XP3_INDEX_CHUNK_COMPRESSED_V2 m_OriginalCompressedChunkV2;
	KrkrClientProxyer*                  m_Proxyer = nullptr;
};



class CoDumperTask final : public CoTaskBase, public Xp3TaskExporter
{
public:
	CoDumperTask() = delete;
	CoDumperTask(const CoDumperTask&) = delete;
	~CoDumperTask();

	CoDumperTask(KrkrClientProxyer* Proxyer, PCWSTR FilePath);
	NTSTATUS ExtractFiles(Xp3WalkerProxy& Proxyer);
	bool NTAPI TaskIsBeingTerminated() { return IsBeingTerminated(); };

private:
	void     ThreadFunction();
	NTSTATUS DumpFileWithIStream(ttstr M2Prefix, ttstr NormalPrefix, Xp3WalkerProxy& Proxyer);
	NTSTATUS DumpFileWithArchive(Xp3WalkerProxy& Proxyer);
	void     DecryptWorker(ULONG64 EncryptOffset, PBYTE Buffer, ULONG BufferSize, ULONG Hash);
	wstring  GetPossibleName(XP3Index& Item);

	HRESULT  DumpStreamSafe(IStream* Stream, PCWSTR ExtName, PCWSTR OutputFileName);

	KrkrClientProxyer* m_Proxyer = nullptr;
	std::wstring       m_FilePath;
};


class CoUniversalPatchTask final : public CoTaskBase
{
public:
	CoUniversalPatchTask() = delete;
	CoUniversalPatchTask(const CoUniversalPatchTask&) = delete;
	~CoUniversalPatchTask();

	CoUniversalPatchTask(KrkrClientProxyer* Proxyer, BOOL ApplyIcon, BOOL ApplyProtection);

	void ThreadFunction();

	BOOL IsProtection() { return m_Protection; };
	BOOL IsCopyIcon()   { return m_CopyIcon;   };

	bool NTAPI TaskIsBeingTerminated() { return IsBeingTerminated(); };

private:
	BOOL               m_Protection = FALSE;
	BOOL               m_CopyIcon   = FALSE;
	KrkrClientProxyer* m_Proxyer    = nullptr;
};


class CoUniversalDumperTask final : public CoTaskBase
{
public:
	CoUniversalDumperTask(const CoUniversalDumperTask&) = delete;
	CoUniversalDumperTask() = delete;
	~CoUniversalDumperTask();

	CoUniversalDumperTask(KrkrClientProxyer* Proxyer);

	bool NTAPI TaskIsBeingTerminated() { return IsBeingTerminated(); };

private:
	void     ThreadFunction();
	NTSTATUS LoadFiles(std::unordered_set<wstring>& FileList);

	void IdentifyFileNameSafe(PVOID Address, SIZE_T Size, std::unordered_set<wstring>& FileList);
	void DumpStreamSafe(IStream* Stream, PCWSTR ExtName, PCWSTR OutputFileName);

private:
	std::unordered_set<std::wstring> m_FileList;
	std::wstring                     m_LastMessage;
	KrkrClientProxyer*               m_Proxyer = nullptr;
};

