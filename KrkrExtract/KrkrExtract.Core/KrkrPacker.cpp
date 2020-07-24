#include "my.h"
#include "ml.h"
#include "CoreTask.h"
#include "XP3Parser.h"
#include "KrkrHeaders.h"
#include "Adler32Stream.h"
#include "mt64.h"
#include "SafeMemory.h"
#include "XP3Parser.h"
#include "Helper.h"
#include "magic_enum.hpp"
#include <zlib.h>


class MemoryStream
{
public:
	SIZE_T GetSize()   { return m_Buffer.size(); };
	PBYTE  GetBuffer() { return &m_Buffer[0];    };

	void Append(PBYTE Buffer, SIZE_T Size)
	{
		if (!Buffer || !Size)
			return;
		
		m_Buffer.insert(m_Buffer.end(), Buffer, Buffer + Size);
	}
	
	void Append(std::vector<BYTE>& Buffer)
	{
		m_Buffer.insert(m_Buffer.end(), Buffer.begin(), Buffer.end());
	}


	void Append(std::vector<BYTE>&& Buffer)
	{
		m_Buffer.insert(m_Buffer.end(), Buffer.begin(), Buffer.end());
	}

	void Set(PBYTE Buffer, SIZE_T Size)
	{
		if (!Buffer || !Size)
			return;

		m_Buffer.resize(Size);
		RtlCopyMemory(&m_Buffer[0], Buffer, Size);
	}

	std::vector<BYTE>& GetStream()
	{
		return m_Buffer;
	}

private:
	std::vector<BYTE> m_Buffer;
};


CoPackerTask::~CoPackerTask()
{
	m_Proxyer = nullptr;
}


CoPackerTask::CoPackerTask(KrkrClientProxyer* Proxyer, PCWSTR BaseDir, PCWSTR OriginalArchiveName, PCWSTR OutputArchiveName) :
	m_BaseDir(BaseDir),
	m_OriginalArchivePath(OriginalArchiveName),
	m_OutputArchivePath(OutputArchiveName)
{
	m_Proxyer = Proxyer;
	m_ArchiveType = PackInfo::UnknownPack;
	m_DecryptionKey = 0;
	m_InfoNameZeroEnd = TRUE;
	m_M2NameZeroEnd = TRUE;

	RtlZeroMemory(&m_OriginalCompressedChunk,   sizeof(m_OriginalCompressedChunk));
	RtlZeroMemory(&m_OriginalCompressedChunkV2, sizeof(m_OriginalCompressedChunkV2));

	PrintConsoleW(L"Filter Addr : 0x%08x\n", m_Proxyer->GetXP3Filter());
}


void CoPackerTask::DecryptWorker(ULONG64 EncryptOffset, PBYTE pBuffer, ULONG BufferSize, ULONG Hash)
{
	tTVPXP3ArchiveExtractionFilter Filter;
	tTVPXP3ExtractionFilterInfo    Info(EncryptOffset, pBuffer, BufferSize, Hash);

	Filter = (tTVPXP3ArchiveExtractionFilter)m_Proxyer->GetXP3Filter();
	if (Filter != nullptr) {
		Filter(&Info);
	}
}

ULONG64 CoPackerTask::CalculateChunkSize(KRKR2_XP3_INDEX_CHUNK_TIME& TimeChunk)
{
	return sizeof(TimeChunk.FileTime);
}

ULONG64 CoPackerTask::CalculateChunkSize(KRKR2_XP3_INDEX_CHUNK_ADLR& AdlrChunk)
{
	return sizeof(AdlrChunk.Hash);
}

ULONG64 CoPackerTask::CalculateChunkSize(KRKR2_XP3_INDEX_CHUNK_SEGM& SegmChunk)
{
	return sizeof(SegmChunk.segm);
}

ULONG64 CoPackerTask::CalculateChunkSize(KRKR2_XP3_INDEX_CHUNK_INFO& InfoChunk)
{
	return FIELD_OFFSET(KRKR2_XP3_INDEX_CHUNK_INFO, FileName) +
		m_InfoNameZeroEnd ? InfoChunk.FileNameLength * sizeof(WCHAR) : (InfoChunk.FileNameLength + 1) * sizeof(WCHAR);
}

ULONG64 CoPackerTask::CalculateChunkSize(KRKRZ_XP3_INDEX_CHUNK_YUZU& YuzuChunk)
{
	return FIELD_OFFSET(KRKRZ_XP3_INDEX_CHUNK_YUZU, FileName) +
		m_M2NameZeroEnd ? YuzuChunk.FileNameLength * sizeof(WCHAR) : (YuzuChunk.FileNameLength + 1) * sizeof(WCHAR);
}


ULONG64 CoPackerTask::CalculateHeaderSize(KRKRZ_XP3_INDEX_CHUNK_COMPRESSED& Header)
{
	return FIELD_OFFSET(KRKRZ_XP3_INDEX_CHUNK_COMPRESSED, ProductName) +
		m_M2ProductZeroEnd ? Header.LengthOfProduct * sizeof(WCHAR) : (Header.LengthOfProduct + 1) * sizeof(WCHAR);
}

ULONG64 CoPackerTask::CalculateFileSize(
	KRKR2_XP3_INDEX_CHUNK_TIME Time,
	KRKR2_XP3_INDEX_CHUNK_ADLR Adlr,
	KRKR2_XP3_INDEX_CHUNK_SEGM Segm,
	KRKR2_XP3_INDEX_CHUNK_INFO Info
)
{
	ULONG64 Size = 0;

	Size += CalculateChunkSize(Time) + CalculateCommonHeaderSize();
	Size += CalculateChunkSize(Adlr) + CalculateCommonHeaderSize();
	Size += CalculateChunkSize(Segm) + CalculateCommonHeaderSize();
	Size += CalculateChunkSize(Info) + CalculateCommonHeaderSize();

	return Size;
}


constexpr ULONG64 CoPackerTask::CalculateCommonHeaderSize()
{
	return FIELD_SIZE(KRKR2_XP3_INDEX_CHUNK_TIME, Magic) + FIELD_SIZE(KRKR2_XP3_INDEX_CHUNK_TIME, ChunkSize);
}

NTSTATUS CoPackerTask::InitializeTimeIndex(KRKR2_XP3_INDEX_CHUNK_TIME& TimeChunk)
{
	TimeChunk.Magic              = CHUNK_MAGIC_TIME;
	TimeChunk.ChunkSize.QuadPart = CalculateChunkSize(TimeChunk);
	GetSystemTimeAsFileTime(&TimeChunk.FileTime);
	
	return STATUS_SUCCESS;
}

NTSTATUS CoPackerTask::InitializeAdlrIndex(KRKR2_XP3_INDEX_CHUNK_ADLR& AdlrChunk)
{
	AdlrChunk.Magic = CHUNK_MAGIC_ADLR;
	AdlrChunk.Hash  = 0x23333333;
	AdlrChunk.ChunkSize.QuadPart = CalculateChunkSize(AdlrChunk);

	return STATUS_SUCCESS;
}

NTSTATUS CoPackerTask::InitializeSegmIndex(KRKR2_XP3_INDEX_CHUNK_SEGM& SegmChunk)
{
	SegmChunk.Magic = CHUNK_MAGIC_SEGM;
	RtlZeroMemory(SegmChunk.segm, sizeof(SegmChunk.segm));
	SegmChunk.ChunkSize.QuadPart = CalculateChunkSize(SegmChunk);

	return STATUS_SUCCESS;
}


NTSTATUS CoPackerTask::InitializeInfoIndex(KRKR2_XP3_INDEX_CHUNK_INFO& InfoChunk)
{
	RtlZeroMemory(&InfoChunk, sizeof(InfoChunk));

	InfoChunk.Magic = CHUNK_MAGIC_INFO;
	return STATUS_SUCCESS;
}


NTSTATUS CoPackerTask::InitializeFileIndex(KRKR2_XP3_INDEX_CHUNK_FILE& FileChunk)
{
	RtlZeroMemory(&FileChunk, sizeof(FileChunk));

	FileChunk.Magic = CHUNK_MAGIC_FILE;
	return STATUS_SUCCESS;
}

NTSTATUS CoPackerTask::InitializeYuzuIndex(KRKRZ_XP3_INDEX_CHUNK_YUZU& YuzuChunk)
{
	RtlZeroMemory(&YuzuChunk, sizeof(YuzuChunk));

	YuzuChunk.Magic = m_M2ChunkMagic;
	return STATUS_SUCCESS;
}

NTSTATUS CoPackerTask::InitializeCompIndex(KRKRZ_XP3_INDEX_CHUNK_COMPRESSED& CompChunk)
{
	RtlCopyMemory(&CompChunk, &m_OriginalCompressedChunk, sizeof(CompChunk));
	
	CompChunk.ArchiveSize  = 0;
	CompChunk.OriginalSize = 0;
	CompChunk.Offset.QuadPart = 0;

	return STATUS_SUCCESS;
}

NTSTATUS CoPackerTask::InitializeCompIndex(KRKRZ_XP3_INDEX_CHUNK_COMPRESSED_V2& CompChunk)
{
	RtlCopyMemory(&CompChunk, &m_OriginalCompressedChunkV2, sizeof(CompChunk));

	CompChunk.ArchiveSize = 0;
	CompChunk.OriginalSize = 0;
	CompChunk.Offset.QuadPart = 0;

	return STATUS_SUCCESS;
}


NTSTATUS CoPackerTask::ReadWriteEncrypt(NtFileDisk& Source, NtFileDisk& Dest)
{
	NTSTATUS      Status;
	LARGE_INTEGER ByteTransferred, Size, ReadedSize;
	static BYTE   Buffer[0x1000];

	Status = Source.Seek((LONG64)0, SEEK_SET);
	if (NT_FAILED(Status))
		return Status;

	Status = Source.GetSize(&Size);
	if (NT_FAILED(Status))
		return Status;

	ReadedSize.QuadPart = 0;
	while (ReadedSize.QuadPart < Size.QuadPart)
	{
		ByteTransferred.QuadPart = 0;
		Source.Read(Buffer, sizeof(Buffer), &ByteTransferred);

		DecryptWorker(ReadedSize.QuadPart, Buffer, ByteTransferred.LowPart, 0x23333333);
		ReadedSize.QuadPart += ByteTransferred.QuadPart;

		Dest.Write(Buffer, ByteTransferred.LowPart);
	}

	return STATUS_SUCCESS;
}

NTSTATUS CoPackerTask::ReadWriteEncrypt(std::wstring& Source, NtFileDisk& Dest, LARGE_INTEGER& Size)
{
	HRESULT      Success;
	STATSTG      Stat;
	IStream*     Stream;
	LARGE_INTEGER ByteTransferred, ReadedSize;
	static BYTE   Buffer[0x1000];
	
	auto AutoPath = GetAutoSearchPath(Source);

	Stream = TVPCreateIStream(Source.c_str(), TJS_BS_READ);
	if (!Stream)
		return STATUS_NO_SUCH_FILE;
	
	ReadedSize.QuadPart = 0;
	while (ReadedSize.QuadPart < Size.QuadPart)
	{
		ByteTransferred.QuadPart = 0;
		Stream->Read(Buffer, sizeof(Buffer), &ByteTransferred.LowPart);

		DecryptWorker(ReadedSize.QuadPart, Buffer, ByteTransferred.LowPart, 0x23333333);
		ReadedSize.QuadPart += ByteTransferred.LowPart;

		Dest.Write(Buffer, ByteTransferred.LowPart);
	}

	Stream->Release();
	
	return STATUS_SUCCESS;
}


std::vector<BYTE> CoPackerTask::SerializeToArray(SMyXP3IndexNormal& FileChunk)
{
	std::vector<BYTE> Array;
	ULONG             Size;
	
	Size = FIELD_OFFSET(SMyXP3IndexNormal, info.FileName);
	if (m_InfoNameZeroEnd) {
		Size += (FileChunk.info.FileNameLength + 1) * sizeof(WCHAR);
	}
	else {
		Size += FileChunk.info.FileNameLength * sizeof(WCHAR);
	}

	Array.insert(Array.end(), (PBYTE)&FileChunk, (PBYTE)&FileChunk + Size);
	return Array;
}


std::vector<BYTE> CoPackerTask::SerializeToArray(SMyXP3IndexM2& FileChunk)
{
	std::vector<BYTE>  Array, Array2;
	ULONG              Size;

	Size = FIELD_OFFSET(SMyXP3IndexM2, yuzu.FileName);
	if (m_M2NameZeroEnd) {
		Size += (FileChunk.yuzu.FileNameLength + 1) * sizeof(WCHAR);
	}
	else {
		Size += FileChunk.yuzu.FileNameLength * sizeof(WCHAR);
	}

	Array.insert(Array.end(), (PBYTE)&FileChunk, (PBYTE)&FileChunk + Size);
	Array2 = SerializeToArray(FileChunk.krkr2);
	Array.insert(Array.end(), Array.begin(), Array.end());

	return Array;
}


std::vector<BYTE> CoPackerTask::SerializeToArray(KRKRZ_XP3_INDEX_CHUNK_YUZU& M2Chunk)
{
	std::vector<BYTE>  Array;
	ULONG              Size;

	Size = FIELD_OFFSET(KRKRZ_XP3_INDEX_CHUNK_YUZU, FileName);
	if (m_M2NameZeroEnd) {
		Size += (M2Chunk.FileNameLength + 1) * sizeof(WCHAR);
	}
	else {
		Size += M2Chunk.FileNameLength * sizeof(WCHAR);
	}

	Array.insert(Array.end(), (PBYTE)&M2Chunk, (PBYTE)&M2Chunk + Size);
	return Array;
}


std::vector<BYTE> CoPackerTask::SerializeToArray(KRKRZ_XP3_INDEX_CHUNK_COMPRESSED& M2CompressedChunk)
{
	std::vector<BYTE>  Array;
	ULONG              Size;

	Size = FIELD_OFFSET(KRKRZ_XP3_INDEX_CHUNK_COMPRESSED, ProductName);
	if (m_M2ProductZeroEnd) {
		Size += (M2CompressedChunk.LengthOfProduct + 1) * sizeof(WCHAR);
	}
	else {
		Size += M2CompressedChunk.LengthOfProduct * sizeof(WCHAR);
	}

	Array.insert(Array.end(), (PBYTE)&M2CompressedChunk, (PBYTE)&M2CompressedChunk + Size);
	return Array;
}


std::vector<BYTE> CoPackerTask::SerializeToArray(KRKRZ_XP3_INDEX_CHUNK_COMPRESSED_V2& M2CompressedChunk)
{
	std::vector<BYTE>  Array;
	ULONG              Size;

	Array.insert(
		Array.end(), 
		(PBYTE)&M2CompressedChunk, 
		(PBYTE)&M2CompressedChunk + sizeof(KRKRZ_XP3_INDEX_CHUNK_COMPRESSED_V2)
	);

	return Array;
}


ttstr CoPackerTask::GetAutoSearchPath(std::wstring& FileName)
{
	ttstr    Path = L"file://./";

	for (SIZE_T Index = 0; Index < m_BaseDir.length(); Index++)
	{
		if (m_BaseDir[Index] == L'\\' || m_BaseDir[Index] == L'/') {
			Path += L"/";
		}
		else if (m_BaseDir[Index] == L':') {
			Path += L"/";
		}
		else {
			Path += CHAR_LOWER(m_BaseDir[Index]);
		}
	}

	Path += L"/";
	Path += m_TemplateFileName.c_str();
	Path += L"/";
	Path += FileName.c_str();

	return Path;
}


NTSTATUS CoPackerTask::IterFiles(LPCWSTR Path)
{
	HANDLE           Handle;
	WCHAR            FilePath[MAX_NTPATH];
	WIN32_FIND_DATAW FindFileData;

	RtlZeroMemory(&FindFileData, sizeof(WIN32_FIND_DATAW));
	wsprintfW(FilePath, L"%s%s", Path, L"\\*.*");

	Handle = Nt_FindFirstFile(FilePath, &FindFileData);
	if (Handle == INVALID_HANDLE_VALUE)
		return STATUS_NO_SUCH_FILE;

	do
	{
		if (!lstrcmpW(FindFileData.cFileName, L".") || !lstrcmpW(FindFileData.cFileName, L".."))
			continue;

		if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;

		m_Proxyer->TellServerLogOutput(
			LogLevel::LOG_INFO,
			L"Found file : %s\n", FindFileData.cFileName
		);

		m_FileList.push_back(FindFileData.cFileName);

	} while (Nt_FindNextFile(Handle, &FindFileData));

	return NtClose(Handle);
}

NTSTATUS CoPackerTask::MakeKrkrCompatibleArchive()
{
	NTSTATUS            Status;
	ULONG64             RandNum;
	WCHAR               TemplateFileName[MAX_PATH];
	std::wstring        TemplateFilePath;
	tTJSVariant         ExecResult;
	WCHAR               Message[0x400];
	
	RandNum = genrand64_int64();
	wsprintfW(TemplateFileName, L"KrkrzTempWorker_%08x%08x.xp3", HiDword(RandNum), LoDword(RandNum));
	TemplateFilePath   = m_Proxyer->GetPackerBaseDir() + std::wstring(L"\\") + TemplateFileName;
	m_TemplateFileName = TemplateFileName;

	if (m_ArchiveType == PackInfo::UnknownPack) {
		m_Proxyer->EnumXp3Plugin(
			this, 
			[](void* Param, Xp3ArchivePlugin* Plugin)->BOOL
			{
				CoPackerTask* Task = (CoPackerTask*)Param;
				if (!Task || !Plugin)
					return FALSE;

				return Plugin->MakeArchive(
					Task,
					Task->GetKrkrClientProxyer(),
					Task->GetBaseDir(),
					Task->GetOriginalArchivePath(),
					Task->GetOutputArchivePath()
					) 
					? FALSE : TRUE;
			}
		);
	}

	auto MakeArchiveWithStatus = [this](std::wstring& OutputFilePath, PackInfo ArchiveType, KrkrPackerPass WhichPass)->NTSTATUS
	{
		NTSTATUS   NtStatus;
		PCWSTR     Phase;
		WCHAR      Message[0x400];
		PCWSTR     PassName;

		switch (WhichPass)
		{
		case CoPackerTask::KrkrPackerPass::NORMAL:
			PassName = L"KrkrPackerPass::NORMAL";
			break;

		case CoPackerTask::KrkrPackerPass::ONE_PASS:
			PassName = L"KrkrPackerPass::ONE_PASS";
			break;

		case CoPackerTask::KrkrPackerPass::TWO_PASS:
			PassName = L"KrkrPackerPass::ONE_PASS";
			break;

		default:
			PassName = L"(Unknown Pass)";
			break;
		}

		switch (ArchiveType)
		{
		case PackInfo::KrkrZ:
			NtStatus = MakeKrkrZCompatibleArchiveGeneral(WhichPass, OutputFilePath);
			Phase = L"MakeKrkrZCompatibleArchiveGeneral";
			break;

		case PackInfo::KrkrZ_V2:
			NtStatus = MakeKrkrNekoCompatibleArchiveGeneral(WhichPass, OutputFilePath);
			Phase = L"MakeKrkrNekoCompatibleArchiveGeneral";
			break;

		case PackInfo::KrkrZ_SenrenBanka:
			NtStatus = MakeKrkrSenrenBankaCompatibleArchiveGeneral(WhichPass, OutputFilePath);
			Phase = L"MakeKrkrSenrenBankaCompatibleArchiveGeneral";
			break;

		case PackInfo::KrkrZ_SenrenBanka_V2:
			NtStatus = MakeKrkrSenrenBankaCompatibleArchiveGeneralV2(WhichPass, OutputFilePath);
			Phase = L"MakeKrkrSenrenBankaCompatibleArchiveGeneralV2";
			break;

		case PackInfo::NormalPack:
			NtStatus = MakeKrkr2CompatibleArchiveGeneral(WhichPass, OutputFilePath);
			Phase = L"MakeKrkr2CompatibleArchiveGeneral";
			break;

		case PackInfo::NormalPack_NoExporter:
			NtStatus = MakeKrkr2CompatibleArchiveGeneral(WhichPass, OutputFilePath);
			Phase = L"MakeKrkr2CompatibleArchiveGeneral";
			break;

		default:
			NtStatus = STATUS_UNSUCCESSFUL;
			m_Proxyer->TellServerLogOutput(
				LogLevel::LOG_ERROR,
				L"CoPackerTask::MakeKrkrCompatibleArchive : should never reach here (switch : %d)",
				(ULONG)ArchiveType
			);
			break;
		}

		if (NT_FAILED(NtStatus))
		{
			wnsprintfW(
				Message,
				countof(Message),
				L"CoPackerTask::%s : %s %s",
				Phase,
				PassName,
				OutputFilePath.c_str()
			);

			SetLastMessage(FALSE, Message);
		}

		return NtStatus;
	};

	LOOP_ONCE
	{
		Status = IterFiles(m_BaseDir.c_str());
		if (NT_FAILED(Status))
		{
			SetLastMessage(FALSE, L"No file available");
			break;
		}

		if (!m_Proxyer->GetXP3Filter())
		{
			m_Proxyer->SetCurrentTempFileName(TemplateFileName);
			Status = MakeArchiveWithStatus(TemplateFilePath, m_ArchiveType, KrkrPackerPass::ONE_PASS);
			if (NT_FAILED(Status))
				break;

			TVPExecuteScript(ttstr(L"Storages.addAutoPath(System.exePath + \"" + ttstr(TemplateFilePath.c_str()) + L"\" + \">\");"), &ExecResult);
			
			Status = MakeArchiveWithStatus(TemplateFilePath, m_ArchiveType, KrkrPackerPass::TWO_PASS);
			if (NT_FAILED(Status))
				break;
			
			TVPExecuteScript(ttstr(L"Storages.removeAutoPath(System.exePath + \"" + ttstr(TemplateFilePath.c_str()) + L"\" + \">\");"));
			if (m_Proxyer->GetCurrentTempHandle() != 0 && m_Proxyer->GetCurrentTempHandle() != INVALID_HANDLE_VALUE)
			{
				Status = NtClose(m_Proxyer->GetCurrentTempHandle());
				if (NT_SUCCESS(Status)) 
				{
					m_Proxyer->SetCurrentTempHandle(INVALID_HANDLE_VALUE);
					m_Proxyer->SetCurrentTempFileName(nullptr);
				}
				else
				{
					wnsprintfW(
						Message,
						countof(Message),
						L"Repacker : Successful!\nDelete %s manually",
						TemplateFileName
					);

					SetLastMessage(
						TRUE,
						Message
					);
					
					Status = STATUS_FILE_NOT_AVAILABLE;
				}
			}
		}
		else {
			Status = MakeArchiveWithStatus(m_OutputArchivePath, m_ArchiveType, KrkrPackerPass::NORMAL);
		}
	}
	return Status;
}


NTSTATUS CoPackerTask::MakeKrkr2CompatibleArchiveGeneral(KrkrPackerPass PassStatus, std::wstring& OutputArchive)
{
	NTSTATUS                Status;
	NtFileDisk              File, FileXP3;
	MemoryStream            IndexStream;
	LARGE_INTEGER           Size, Offset, BytesTransfered, RawIndexSize;
	SMyXP3IndexNormal*      Xp3Index;
	KRKR2_XP3_DATA_HEADER   IndexHeader;
	SIZE_T                  RealCount;
	BYTE                    FirstMagic[11] = { 0x58, 0x50, 0x33, 0x0D, 0x0A, 0x20, 0x0A, 0x1A, 0x8B, 0x67, 0x01 };
	KRKR2_XP3_HEADER        XP3Header(FirstMagic, 0);

	FileXP3.Write(&XP3Header, sizeof(XP3Header), &BytesTransfered);
	Offset.QuadPart = BytesTransfered.QuadPart;

	auto XP3IndexBuffer = AllocateMemorySafeP<SMyXP3IndexNormal>(sizeof(SMyXP3IndexNormal) * m_FileList.size());
	if (!XP3IndexBuffer)
		return STATUS_NO_MEMORY;

	RawIndexSize.QuadPart = 0;
	RealCount = 0;
	Xp3Index  = XP3IndexBuffer.get();
	for (SIZE_T i = 0; i < m_FileList.size(); i++)
	{
		RtlZeroMemory(Xp3Index, sizeof(*Xp3Index));
		
		auto FullFilePath = m_BaseDir + L"\\" + m_FileList[i];
		Status = File.Open(FullFilePath.c_str());
		if (NT_FAILED(Status))
		{
			m_Proxyer->TellServerLogOutput(
				LogLevel::LOG_ERROR, 
				L"CoPackerTask::MakeKrkr2CompatibleArchiveGeneral : Open file failed = %s",
				FullFilePath.c_str());

			continue;
		}

		Status = File.GetSize(&Size);
		if (NT_FAILED(Status))
		{
			m_Proxyer->TellServerLogOutput(
				LogLevel::LOG_ERROR,
				L"CoPackerTask::MakeKrkr2CompatibleArchiveGeneral : Get file size failed = %s",
				FullFilePath.c_str());

			continue;
		}

		if (PassStatus == KrkrPackerPass::NORMAL || PassStatus == KrkrPackerPass::ONE_PASS) {
			Status = ReadWriteEncrypt(File, FileXP3);
		}
		else 
		{
			Size.QuadPart = 0;
			Status = ReadWriteEncrypt(m_FileList[i] + L".dummy", FileXP3, Size);
		}
		if (NT_FAILED(Status))
		{
			m_Proxyer->TellServerLogOutput(
				LogLevel::LOG_ERROR,
				L"CoPackerTask::MakeKrkr2CompatibleArchiveGeneral : ReadWriteEncrypt failed = %s",
				FullFilePath.c_str());

			continue;
		}


		InitializeTimeIndex(Xp3Index->time);
		InitializeAdlrIndex(Xp3Index->adlr);
		InitializeSegmIndex(Xp3Index->segm);
		InitializeInfoIndex(Xp3Index->info);
		InitializeFileIndex(Xp3Index->file);

		Xp3Index->segm.segm[0].Offset = Offset;
		Xp3Index->segm.segm[0].bZlib  = 0;
		Xp3Index->segm.segm[0].ArchiveSize.QuadPart  = Size.QuadPart;
		Xp3Index->segm.segm[0].OriginalSize.QuadPart = Size.QuadPart;

		if (PassStatus == KrkrPackerPass::NORMAL || PassStatus == KrkrPackerPass::TWO_PASS) 
		{
			if (m_FileList[i].length() >= countof(Xp3Index->info.FileName))
			{
				m_Proxyer->TellServerLogOutput(
					LogLevel::LOG_ERROR,
					L"CoPackerTask::MakeKrkr2CompatibleArchiveGeneral : "
					L"m_FileList[i].length() >= countof(Xp3Index->info.FileName), Buffer overflow"
				);

				return STATUS_UNSUCCESSFUL;
			}


			lstrcpynW(Xp3Index->info.FileName, m_FileList[i].c_str(), m_FileList[i].length() + 1);
		}
		else 
		{
			wnsprintfW(
				Xp3Index->info.FileName,
				countof(Xp3Index->info.FileName), 
				L"%s.dummy",
				m_FileList[i].c_str()
				);
		}

		Xp3Index->info.FileNameLength        = (USHORT)m_FileList[i].size();
		Xp3Index->info.OriginalSize.QuadPart = Size.QuadPart;
		Xp3Index->info.ArchiveSize.QuadPart  = Size.QuadPart;

		

		if (m_Proxyer->GetXP3Filter()) {
			Xp3Index->info.EncryptedFlag = 0x80000000;
		}
		else {
			Xp3Index->info.EncryptedFlag = 0;
		}

		Xp3Index->info.ChunkSize.QuadPart = CalculateChunkSize(Xp3Index->info);
		Xp3Index->file.ChunkSize.QuadPart = CalculateFileSize(
			Xp3Index->time,
			Xp3Index->adlr,
			Xp3Index->segm,
			Xp3Index->info
		);

		if (!m_InfoNameZeroEnd) {
			Xp3Index->file.ChunkSize.QuadPart -= sizeof(WCHAR);
		}

		IndexStream.Append(SerializeToArray(*Xp3Index));
		
		Offset.QuadPart += Size.QuadPart;
		Xp3Index++;
		RealCount++;
		RawIndexSize.QuadPart += Xp3Index->file.ChunkSize.QuadPart + sizeof(Xp3Index->info);
	}

	XP3Header.IndexOffset = Offset;
	IndexHeader.bZlib = 1;
	IndexHeader.OriginalSize.QuadPart = (ULONG64)IndexStream.GetSize();
	IndexHeader.ArchiveSize.QuadPart  = (ULONG64)compressBound(IndexStream.GetSize());

	LOOP_ONCE
	{
		auto CompressedBuffer = AllocateMemorySafeP<BYTE>(IndexHeader.ArchiveSize.LowPart);
		if (!CompressedBuffer)
		{
			m_Proxyer->TellServerLogOutput(
				LogLevel::LOG_WARN,
				L"CoPackerTask::MakeKrkr2CompatibleArchiveGeneral : failed to allocate memory for compress index buffer"
			);
			
			IndexHeader.bZlib = 0;
			IndexHeader.ArchiveSize.QuadPart = (ULONG64)IndexStream.GetSize();
			FileXP3.Write(&IndexHeader, sizeof(IndexHeader), &BytesTransfered);
			FileXP3.Write(IndexStream.GetBuffer(), IndexHeader.OriginalSize.LowPart);
			break;
		}

		if (Z_OK != compress2(
			CompressedBuffer.get(),
			&IndexHeader.ArchiveSize.LowPart,
			IndexStream.GetBuffer(),
			IndexStream.GetSize(),
			Z_BEST_COMPRESSION)
			)
		{
			m_Proxyer->TellServerLogOutput(
				LogLevel::LOG_WARN,
				L"CoPackerTask::MakeKrkr2CompatibleArchiveGeneral : failed to invoke `compress2` for index buffer"
			);

			IndexHeader.bZlib = 0;
			IndexHeader.ArchiveSize.QuadPart = (ULONG64)IndexStream.GetSize();
			FileXP3.Write(&IndexHeader, sizeof(IndexHeader), &BytesTransfered);
			FileXP3.Write(IndexStream.GetBuffer(), IndexHeader.OriginalSize.LowPart);
			break;
		}

		FileXP3.Write(&IndexHeader, sizeof(IndexHeader), &BytesTransfered);
		FileXP3.Write(CompressedBuffer.get(), IndexHeader.ArchiveSize.LowPart, &BytesTransfered);
	}

	FileXP3.Seek((LONG64)0, FILE_BEGIN);
	FileXP3.Write(&XP3Header, sizeof(XP3Header), &BytesTransfered);

	return STATUS_SUCCESS;
}

NTSTATUS CoPackerTask::MakeKrkrZCompatibleArchiveGeneral(KrkrPackerPass PassStatus, std::wstring& OutputArchive)
{
	NTSTATUS                Status;
	NtFileDisk              File, FileXP3;
	MemoryStream            IndexStream;
	LARGE_INTEGER           Size, Offset, BytesTransfered, RawIndexSize;
	SMyXP3IndexM2*          Xp3Index;
	KRKR2_XP3_DATA_HEADER   IndexHeader;
	SIZE_T                  RealCount;
	BYTE                    FirstMagic[11] = { 0x58, 0x50, 0x33, 0x0D, 0x0A, 0x20, 0x0A, 0x1A, 0x8B, 0x67, 0x01 };
	std::wstring            HashName;
	KRKR2_XP3_HEADER        XP3Header(FirstMagic, 0);

	FileXP3.Write(&XP3Header, sizeof(XP3Header), &BytesTransfered);
	Offset.QuadPart = BytesTransfered.QuadPart;

	auto XP3IndexBuffer = AllocateMemorySafeP<SMyXP3IndexM2>(sizeof(SMyXP3IndexM2) * m_FileList.size());
	if (!XP3IndexBuffer)
		return STATUS_NO_MEMORY;

	RawIndexSize.QuadPart = 0;
	RealCount = 0;
	Xp3Index = XP3IndexBuffer.get();
	for (SIZE_T i = 0; i < m_FileList.size(); i++)
	{
		RtlZeroMemory(Xp3Index, sizeof(*Xp3Index));

		auto FullFilePath = m_BaseDir + L"\\" + m_FileList[i];
		Status = File.Open(FullFilePath.c_str());
		if (NT_FAILED(Status))
		{
			m_Proxyer->TellServerLogOutput(
				LogLevel::LOG_ERROR,
				L"CoPackerTask::MakeKrkrZCompatibleArchiveGeneral : Open file failed = %s",
				FullFilePath.c_str());

			continue;
		}

		Status = File.GetSize(&Size);
		if (NT_FAILED(Status))
		{
			m_Proxyer->TellServerLogOutput(
				LogLevel::LOG_ERROR,
				L"CoPackerTask::MakeKrkrZCompatibleArchiveGeneral : Get file size failed = %s",
				FullFilePath.c_str());

			continue;
		}

		if (PassStatus == KrkrPackerPass::NORMAL || PassStatus == KrkrPackerPass::ONE_PASS) {
			Status = ReadWriteEncrypt(File, FileXP3);
		}
		else
		{
			Size.QuadPart = 0;
			Status = ReadWriteEncrypt(m_FileList[i] + L".dummy", FileXP3, Size);
		}
		if (NT_FAILED(Status))
		{
			m_Proxyer->TellServerLogOutput(
				LogLevel::LOG_ERROR,
				L"CoPackerTask::MakeKrkrZCompatibleArchiveGeneral : ReadWriteEncrypt failed = %s",
				FullFilePath.c_str());

			continue;
		}


		InitializeTimeIndex(Xp3Index->time);
		InitializeAdlrIndex(Xp3Index->adlr);
		InitializeSegmIndex(Xp3Index->segm);
		InitializeInfoIndex(Xp3Index->info);
		InitializeFileIndex(Xp3Index->file);
		InitializeYuzuIndex(Xp3Index->yuzu);

		Xp3Index->segm.segm[0].Offset = Offset;
		Xp3Index->segm.segm[0].bZlib = 0;
		Xp3Index->segm.segm[0].ArchiveSize.QuadPart = Size.QuadPart;
		Xp3Index->segm.segm[0].OriginalSize.QuadPart = Size.QuadPart;

		HashName.clear();
		if (PassStatus == KrkrPackerPass::NORMAL || PassStatus == KrkrPackerPass::TWO_PASS) 
		{
			if (m_FileList[i].length() >= countof(Xp3Index->info.FileName))
			{
				m_Proxyer->TellServerLogOutput(
					LogLevel::LOG_ERROR,
					L"CoPackerTask::MakeKrkrZCompatibleArchiveGeneral : "
					L"m_FileList[i].length() >= countof(Xp3Index->info.FileName), Buffer overflow"
				);

				return STATUS_UNSUCCESSFUL;
			}

			lstrcpynW(Xp3Index->info.FileName, m_FileList[i].c_str(), m_FileList[i].length() + 1);
		}
		else
		{
			wnsprintfW(
				Xp3Index->info.FileName,
				countof(Xp3Index->info.FileName),
				L"%s.dummy",
				m_FileList[i].c_str()
			);
		}

		GenMD5Code(ToLowerString(Xp3Index->info.FileName).c_str(), HashName);

		if (HashName.length() >= countof(Xp3Index->info.FileName))
		{
			m_Proxyer->TellServerLogOutput(
				LogLevel::LOG_ERROR,
				L"CoPackerTask::MakeKrkrZCompatibleArchiveGeneral : "
				L"HashName.length() >= countof(Xp3Index->info.FileName), Buffer overflow"
			);

			return STATUS_UNSUCCESSFUL;
		}

		lstrcpynW(Xp3Index->info.FileName, HashName.c_str(), HashName.length() + 1);
		Xp3Index->info.FileNameLength = (USHORT)m_FileList[i].size();

		Xp3Index->info.OriginalSize.QuadPart = Size.QuadPart;
		Xp3Index->info.ArchiveSize.QuadPart  = Size.QuadPart;

		if (m_Proxyer->GetXP3Filter()) {
			Xp3Index->info.EncryptedFlag = 0x80000000;
		}
		else {
			Xp3Index->info.EncryptedFlag = 0;
		}

		Xp3Index->info.ChunkSize.QuadPart = CalculateChunkSize(Xp3Index->info);
		Xp3Index->file.ChunkSize.QuadPart = CalculateFileSize(
			Xp3Index->time,
			Xp3Index->adlr,
			Xp3Index->segm,
			Xp3Index->info
		);

		if (!m_InfoNameZeroEnd) {
			Xp3Index->file.ChunkSize.QuadPart -= sizeof(WCHAR);
		}


		Xp3Index->yuzu.Hash = Xp3Index->adlr.Hash;
		Xp3Index->yuzu.FileNameLength = (USHORT)m_FileList[i].size();
		if (PassStatus == KrkrPackerPass::NORMAL || PassStatus == KrkrPackerPass::TWO_PASS)
		{
			if (m_FileList[i].length() >= countof(Xp3Index->yuzu.FileName))
			{
				m_Proxyer->TellServerLogOutput(
					LogLevel::LOG_ERROR,
					L"CoPackerTask::MakeKrkrZCompatibleArchiveGeneral : "
					L"m_FileList[i].length() >= countof(Xp3Index->yuzu.FileName), Buffer overflow"
				);

				return STATUS_UNSUCCESSFUL;
			}

			lstrcpynW(Xp3Index->yuzu.FileName, m_FileList[i].c_str(), m_FileList[i].length() + 1);
		}
		else
		{
			wnsprintfW(
				Xp3Index->yuzu.FileName,
				countof(Xp3Index->yuzu.FileName),
				L"%s.dummy",
				m_FileList[i].c_str()
			);
		}

		Xp3Index->yuzu.ChunkSize.QuadPart = CalculateChunkSize(Xp3Index->yuzu);

		IndexStream.Append(SerializeToArray(*Xp3Index));

		Offset.QuadPart += Size.QuadPart;
		Xp3Index++;
		RealCount++;
		RawIndexSize.QuadPart += Xp3Index->file.ChunkSize.QuadPart + sizeof(Xp3Index->info);
	}

	XP3Header.IndexOffset = Offset;
	IndexHeader.bZlib = 1;
	IndexHeader.OriginalSize.QuadPart = (ULONG64)IndexStream.GetSize();
	IndexHeader.ArchiveSize.QuadPart  = (ULONG64)compressBound(IndexStream.GetSize());

	LOOP_ONCE
	{
		auto CompressedBuffer = AllocateMemorySafeP<BYTE>(IndexHeader.ArchiveSize.LowPart);
		if (!CompressedBuffer)
		{
			m_Proxyer->TellServerLogOutput(
				LogLevel::LOG_WARN,
				L"CoPackerTask::MakeKrkrZCompatibleArchiveGeneral : failed to allocate memory for compress index buffer"
			);

			IndexHeader.bZlib = 0;
			IndexHeader.ArchiveSize.QuadPart = (ULONG64)IndexStream.GetSize();
			FileXP3.Write(&IndexHeader, sizeof(IndexHeader), &BytesTransfered);
			FileXP3.Write(IndexStream.GetBuffer(), IndexHeader.OriginalSize.LowPart);
			break;
		}

		if (Z_OK != compress2(
			CompressedBuffer.get(),
			&IndexHeader.ArchiveSize.LowPart,
			IndexStream.GetBuffer(),
			IndexStream.GetSize(),
			Z_BEST_COMPRESSION)
			)
		{
			m_Proxyer->TellServerLogOutput(
				LogLevel::LOG_WARN,
				L"CoPackerTask::MakeKrkrZCompatibleArchiveGeneral : failed to invoke `compress2` for index buffer"
			);

			IndexHeader.bZlib = 0;
			IndexHeader.ArchiveSize.QuadPart = (ULONG64)IndexStream.GetSize();
			FileXP3.Write(&IndexHeader, sizeof(IndexHeader), &BytesTransfered);
			FileXP3.Write(IndexStream.GetBuffer(), IndexHeader.OriginalSize.LowPart);
			break;
		}

		FileXP3.Write(&IndexHeader, sizeof(IndexHeader), &BytesTransfered);
		FileXP3.Write(CompressedBuffer.get(), IndexHeader.ArchiveSize.LowPart, &BytesTransfered);
	}

	FileXP3.Seek((LONG64)0, FILE_BEGIN);
	FileXP3.Write(&XP3Header, sizeof(XP3Header), &BytesTransfered);

	return STATUS_SUCCESS;
}


NTSTATUS CoPackerTask::MakeKrkrNekoCompatibleArchiveGeneral(KrkrPackerPass PassStatus, std::wstring& OutputArchive)
{
	NTSTATUS                Status;
	NtFileDisk              File, FileXP3;
	MemoryStream            IndexStreamKr, IndexStreamM2, IndexStream;
	LARGE_INTEGER           Size, Offset, BytesTransfered, RawIndexSize;
	SMyXP3IndexM2*          Xp3Index;
	KRKR2_XP3_DATA_HEADER   IndexHeader;
	SIZE_T                  RealCount;
	BYTE                    FirstMagic[11] = { 0x58, 0x50, 0x33, 0x0D, 0x0A, 0x20, 0x0A, 0x1A, 0x8B, 0x67, 0x01 };
	std::wstring            HashName;
	KRKR2_XP3_HEADER        XP3Header(FirstMagic, 0);

	FileXP3.Write(&XP3Header, sizeof(XP3Header), &BytesTransfered);
	Offset.QuadPart = BytesTransfered.QuadPart;

	auto XP3IndexBuffer = AllocateMemorySafeP<SMyXP3IndexM2>(sizeof(SMyXP3IndexM2) * m_FileList.size());
	if (!XP3IndexBuffer)
		return STATUS_NO_MEMORY;

	RawIndexSize.QuadPart = 0;
	RealCount = 0;
	Xp3Index = XP3IndexBuffer.get();
	for (SIZE_T i = 0; i < m_FileList.size(); i++)
	{
		RtlZeroMemory(Xp3Index, sizeof(*Xp3Index));

		auto FullFilePath = m_BaseDir + L"\\" + m_FileList[i];
		Status = File.Open(FullFilePath.c_str());
		if (NT_FAILED(Status))
		{
			m_Proxyer->TellServerLogOutput(
				LogLevel::LOG_ERROR,
				L"CoPackerTask::MakeKrkrNekoCompatibleArchiveGeneral : Open file failed = %s",
				FullFilePath.c_str());

			continue;
		}

		Status = File.GetSize(&Size);
		if (NT_FAILED(Status))
		{
			m_Proxyer->TellServerLogOutput(
				LogLevel::LOG_ERROR,
				L"CoPackerTask::MakeKrkrNekoCompatibleArchiveGeneral : Get file size failed = %s",
				FullFilePath.c_str());

			continue;
		}

		if (PassStatus == KrkrPackerPass::NORMAL || PassStatus == KrkrPackerPass::ONE_PASS) {
			Status = ReadWriteEncrypt(File, FileXP3);
		}
		else
		{
			Size.QuadPart = 0;
			Status = ReadWriteEncrypt(m_FileList[i] + L".dummy", FileXP3, Size);
		}
		if (NT_FAILED(Status))
		{
			m_Proxyer->TellServerLogOutput(
				LogLevel::LOG_ERROR,
				L"CoPackerTask::MakeKrkrNekoCompatibleArchiveGeneral : ReadWriteEncrypt failed = %s",
				FullFilePath.c_str());

			continue;
		}


		InitializeTimeIndex(Xp3Index->time);
		InitializeAdlrIndex(Xp3Index->adlr);
		InitializeSegmIndex(Xp3Index->segm);
		InitializeInfoIndex(Xp3Index->info);
		InitializeFileIndex(Xp3Index->file);
		InitializeYuzuIndex(Xp3Index->yuzu);

		Xp3Index->segm.segm[0].Offset = Offset;
		Xp3Index->segm.segm[0].bZlib = 0;
		Xp3Index->segm.segm[0].ArchiveSize.QuadPart = Size.QuadPart;
		Xp3Index->segm.segm[0].OriginalSize.QuadPart = Size.QuadPart;

		HashName.clear();
		if (PassStatus == KrkrPackerPass::NORMAL || PassStatus == KrkrPackerPass::TWO_PASS) 
		{
			if (m_FileList[i].length() >= countof(Xp3Index->info.FileName))
			{
				m_Proxyer->TellServerLogOutput(
					LogLevel::LOG_ERROR,
					L"CoPackerTask::MakeKrkrNekoCompatibleArchiveGeneral : "
					L"m_FileList[i].length() >= countof(Xp3Index->info.FileName), Buffer overflow"
				);

				return STATUS_UNSUCCESSFUL;
			}

			lstrcpynW(Xp3Index->info.FileName, m_FileList[i].c_str(), m_FileList[i].length() + 1);
		}
		else
		{
			wnsprintfW(
				Xp3Index->info.FileName,
				countof(Xp3Index->info.FileName),
				L"%s.dummy",
				m_FileList[i].c_str()
			);
		}

		GenMD5Code(ToLowerString(Xp3Index->info.FileName).c_str(), HashName);

		if (HashName.length() >= countof(Xp3Index->info.FileName))
		{
			m_Proxyer->TellServerLogOutput(
				LogLevel::LOG_ERROR,
				L"CoPackerTask::MakeKrkrNekoCompatibleArchiveGeneral : "
				L"HashName.length() >= countof(Xp3Index->info.FileName), Buffer overflow"
			);

			return STATUS_UNSUCCESSFUL;
		}

		lstrcpynW(Xp3Index->info.FileName, HashName.c_str(), HashName.length() + 1);
		Xp3Index->info.FileNameLength = (USHORT)m_FileList[i].size();

		Xp3Index->info.OriginalSize.QuadPart = Size.QuadPart;
		Xp3Index->info.ArchiveSize.QuadPart = Size.QuadPart;

		if (m_Proxyer->GetXP3Filter()) {
			Xp3Index->info.EncryptedFlag = 0x80000000;
		}
		else {
			Xp3Index->info.EncryptedFlag = 0;
		}

		Xp3Index->info.ChunkSize.QuadPart = CalculateChunkSize(Xp3Index->info);
		Xp3Index->file.ChunkSize.QuadPart = CalculateFileSize(
			Xp3Index->time,
			Xp3Index->adlr,
			Xp3Index->segm,
			Xp3Index->info
		);

		if (!m_InfoNameZeroEnd) {
			Xp3Index->file.ChunkSize.QuadPart -= sizeof(WCHAR);
		}


		Xp3Index->yuzu.Hash = Xp3Index->adlr.Hash;
		Xp3Index->yuzu.FileNameLength = (USHORT)m_FileList[i].size();
		if (PassStatus == KrkrPackerPass::NORMAL || PassStatus == KrkrPackerPass::TWO_PASS) 
		{
			if (m_FileList[i].length() >= countof(Xp3Index->yuzu.FileName))
			{
				m_Proxyer->TellServerLogOutput(
					LogLevel::LOG_ERROR,
					L"CoPackerTask::MakeKrkrNekoCompatibleArchiveGeneral : "
					L"m_FileList[i].length() >= countof(Xp3Index->yuzu.FileName), Buffer overflow"
				);

				return STATUS_UNSUCCESSFUL;
			}

			lstrcpynW(Xp3Index->yuzu.FileName, m_FileList[i].c_str(), m_FileList[i].length() + 1);
		}
		else
		{
			wnsprintfW(
				Xp3Index->yuzu.FileName,
				countof(Xp3Index->yuzu.FileName),
				L"%s.dummy",
				m_FileList[i].c_str()
			);
		}

		Xp3Index->yuzu.ChunkSize.QuadPart = CalculateChunkSize(Xp3Index->yuzu);

		IndexStreamM2.Append(SerializeToArray(Xp3Index->yuzu));
		IndexStreamKr.Append(SerializeToArray(Xp3Index->krkr2));

		Offset.QuadPart += Size.QuadPart;
		Xp3Index++;
		RealCount++;
		RawIndexSize.QuadPart += Xp3Index->file.ChunkSize.QuadPart + sizeof(Xp3Index->info);
	}

	IndexStream.GetStream().insert(
		IndexStream.GetStream().end(),
		IndexStreamM2.GetStream().begin(),
		IndexStreamM2.GetStream().end()
	);

	IndexStream.GetStream().insert(
		IndexStream.GetStream().end(),
		IndexStreamKr.GetStream().begin(),
		IndexStreamKr.GetStream().end()
	);

	XP3Header.IndexOffset = Offset;
	IndexHeader.bZlib = 1;
	IndexHeader.OriginalSize.QuadPart = (ULONG64)IndexStream.GetSize();
	IndexHeader.ArchiveSize.QuadPart = (ULONG64)compressBound(IndexStream.GetSize());

	LOOP_ONCE
	{
		auto CompressedBuffer = AllocateMemorySafeP<BYTE>(IndexHeader.ArchiveSize.LowPart);
		if (!CompressedBuffer)
		{
			m_Proxyer->TellServerLogOutput(
				LogLevel::LOG_WARN,
				L"CoPackerTask::MakeKrkrNekoCompatibleArchiveGeneral : failed to allocate memory for compress index buffer"
			);

			IndexHeader.bZlib = 0;
			IndexHeader.ArchiveSize.QuadPart = (ULONG64)IndexStream.GetSize();
			FileXP3.Write(&IndexHeader, sizeof(IndexHeader), &BytesTransfered);
			FileXP3.Write(IndexStream.GetBuffer(), IndexHeader.OriginalSize.LowPart);
			break;
		}

		if (Z_OK != compress2(
			CompressedBuffer.get(),
			&IndexHeader.ArchiveSize.LowPart,
			IndexStream.GetBuffer(),
			IndexStream.GetSize(),
			Z_BEST_COMPRESSION)
			)
		{
			m_Proxyer->TellServerLogOutput(
				LogLevel::LOG_WARN,
				L"CoPackerTask::MakeKrkrNekoCompatibleArchiveGeneral : failed to invoke `compress2` for index buffer"
			);

			IndexHeader.bZlib = 0;
			IndexHeader.ArchiveSize.QuadPart = (ULONG64)IndexStream.GetSize();
			FileXP3.Write(&IndexHeader, sizeof(IndexHeader), &BytesTransfered);
			FileXP3.Write(IndexStream.GetBuffer(), IndexHeader.OriginalSize.LowPart);
			break;
		}

		FileXP3.Write(&IndexHeader, sizeof(IndexHeader), &BytesTransfered);
		FileXP3.Write(CompressedBuffer.get(), IndexHeader.ArchiveSize.LowPart, &BytesTransfered);
	}

	FileXP3.Seek((LONG64)0, FILE_BEGIN);
	FileXP3.Write(&XP3Header, sizeof(XP3Header), &BytesTransfered);

	return STATUS_SUCCESS;
}


NTSTATUS CoPackerTask::MakeKrkrSenrenBankaCompatibleArchiveGeneral(KrkrPackerPass PassStatus, std::wstring& OutputArchive)
{
	NTSTATUS                          Status;
	NtFileDisk                        File, FileXP3;
	MemoryStream                      IndexStreamKr, IndexStreamM2, IndexStream;
	LARGE_INTEGER                     Size, Offset, BytesTransfered, RawIndexSize;
	SMyXP3IndexM2*                    Xp3Index;
	KRKR2_XP3_DATA_HEADER             IndexHeader;
	KRKRZ_XP3_INDEX_CHUNK_COMPRESSED  M2CompressedHeader;
	SIZE_T                            RealCount;
	BYTE                              FirstMagic[11] = { 0x58, 0x50, 0x33, 0x0D, 0x0A, 0x20, 0x0A, 0x1A, 0x8B, 0x67, 0x01 };
	std::wstring                      HashName;
	KRKR2_XP3_HEADER                  XP3Header(FirstMagic, 0);

	FileXP3.Write(&XP3Header, sizeof(XP3Header), &BytesTransfered);
	Offset.QuadPart = BytesTransfered.QuadPart;

	auto XP3IndexBuffer = AllocateMemorySafeP<SMyXP3IndexM2>(sizeof(SMyXP3IndexM2) * m_FileList.size());
	if (!XP3IndexBuffer)
		return STATUS_NO_MEMORY;

	RawIndexSize.QuadPart = 0;
	RealCount = 0;
	Xp3Index = XP3IndexBuffer.get();
	for (SIZE_T i = 0; i < m_FileList.size(); i++)
	{
		RtlZeroMemory(Xp3Index, sizeof(*Xp3Index));

		auto FullFilePath = m_BaseDir + L"\\" + m_FileList[i];
		Status = File.Open(FullFilePath.c_str());
		if (NT_FAILED(Status))
		{
			m_Proxyer->TellServerLogOutput(
				LogLevel::LOG_ERROR,
				L"CoPackerTask::MakeKrkrSenrenBankaCompatibleArchiveGeneral : Open file failed = %s",
				FullFilePath.c_str());

			continue;
		}

		Status = File.GetSize(&Size);
		if (NT_FAILED(Status))
		{
			m_Proxyer->TellServerLogOutput(
				LogLevel::LOG_ERROR,
				L"CoPackerTask::MakeKrkrSenrenBankaCompatibleArchiveGeneral : Get file size failed = %s",
				FullFilePath.c_str());

			continue;
		}

		if (PassStatus == KrkrPackerPass::NORMAL || PassStatus == KrkrPackerPass::ONE_PASS) {
			Status = ReadWriteEncrypt(File, FileXP3);
		}
		else
		{
			Size.QuadPart = 0;
			Status = ReadWriteEncrypt(m_FileList[i] + L".dummy", FileXP3, Size);
		}
		if (NT_FAILED(Status))
		{
			m_Proxyer->TellServerLogOutput(
				LogLevel::LOG_ERROR,
				L"CoPackerTask::MakeKrkrSenrenBankaCompatibleArchiveGeneral : ReadWriteEncrypt failed = %s",
				FullFilePath.c_str());

			continue;
		}


		InitializeTimeIndex(Xp3Index->time);
		InitializeAdlrIndex(Xp3Index->adlr);
		InitializeSegmIndex(Xp3Index->segm);
		InitializeInfoIndex(Xp3Index->info);
		InitializeFileIndex(Xp3Index->file);
		InitializeYuzuIndex(Xp3Index->yuzu);

		Xp3Index->segm.segm[0].Offset = Offset;
		Xp3Index->segm.segm[0].bZlib = 0;
		Xp3Index->segm.segm[0].ArchiveSize.QuadPart = Size.QuadPart;
		Xp3Index->segm.segm[0].OriginalSize.QuadPart = Size.QuadPart;

		HashName.clear();
		if (PassStatus == KrkrPackerPass::NORMAL || PassStatus == KrkrPackerPass::TWO_PASS)
		{
			if (m_FileList[i].length() >= countof(Xp3Index->info.FileName))
			{
				m_Proxyer->TellServerLogOutput(
					LogLevel::LOG_ERROR,
					L"CoPackerTask::MakeKrkrSenrenBankaCompatibleArchiveGeneral : "
					L"m_FileList[i].length() >= countof(Xp3Index->info.FileName), Buffer overflow"
				);

				return STATUS_UNSUCCESSFUL;
			}

			lstrcpynW(Xp3Index->info.FileName, m_FileList[i].c_str(), m_FileList[i].length() + 1);
		}
		else
		{
			wnsprintfW(
				Xp3Index->info.FileName,
				countof(Xp3Index->info.FileName),
				L"%s.dummy",
				m_FileList[i].c_str()
			);
		}

		GenMD5Code(ToLowerString(Xp3Index->info.FileName).c_str(), HashName);

		if (HashName.length() >= countof(Xp3Index->yuzu.FileName))
		{
			m_Proxyer->TellServerLogOutput(
				LogLevel::LOG_ERROR,
				L"CoPackerTask::MakeKrkrSenrenBankaCompatibleArchiveGeneral : "
				L"HashName.length() >= countof(Xp3Index->info.FileName), Buffer overflow"
			);

			return STATUS_UNSUCCESSFUL;
		}

		lstrcpynW(Xp3Index->info.FileName, HashName.c_str(), HashName.length() + 1);
		Xp3Index->info.FileNameLength = (USHORT)m_FileList[i].size();

		Xp3Index->info.OriginalSize.QuadPart = Size.QuadPart;
		Xp3Index->info.ArchiveSize.QuadPart  = Size.QuadPart;

		if (m_Proxyer->GetXP3Filter()) {
			Xp3Index->info.EncryptedFlag = 0x80000000;
		}
		else {
			Xp3Index->info.EncryptedFlag = 0;
		}

		Xp3Index->info.ChunkSize.QuadPart = CalculateChunkSize(Xp3Index->info);
		Xp3Index->file.ChunkSize.QuadPart = CalculateFileSize(
			Xp3Index->time,
			Xp3Index->adlr,
			Xp3Index->segm,
			Xp3Index->info
		);

		if (!m_InfoNameZeroEnd) {
			Xp3Index->file.ChunkSize.QuadPart -= sizeof(WCHAR);
		}


		Xp3Index->yuzu.Hash = Xp3Index->adlr.Hash;
		Xp3Index->yuzu.FileNameLength = (USHORT)m_FileList[i].size();
		if (PassStatus == KrkrPackerPass::NORMAL || PassStatus == KrkrPackerPass::TWO_PASS)
		{
			if (m_FileList[i].length() >= countof(Xp3Index->yuzu.FileName))
			{
				m_Proxyer->TellServerLogOutput(
					LogLevel::LOG_ERROR,
					L"CoPackerTask::MakeKrkrSenrenBankaCompatibleArchiveGeneral : "
					L"m_FileList[i].length() >= countof(Xp3Index->yuzu.FileName), Buffer overflow"
				);

				return STATUS_UNSUCCESSFUL;
			}

			lstrcpynW(Xp3Index->yuzu.FileName, m_FileList[i].c_str(), m_FileList[i].length() + 1);
		}
		else
		{
			wnsprintfW(
				Xp3Index->yuzu.FileName,
				countof(Xp3Index->yuzu.FileName),
				L"%s.dummy",
				m_FileList[i].c_str()
			);
		}

		Xp3Index->yuzu.ChunkSize.QuadPart = CalculateChunkSize(Xp3Index->yuzu);

		IndexStreamM2.Append(SerializeToArray(Xp3Index->yuzu));
		IndexStreamKr.Append(SerializeToArray(Xp3Index->krkr2));

		Offset.QuadPart += Size.QuadPart;
		Xp3Index++;
		RealCount++;
		RawIndexSize.QuadPart += Xp3Index->file.ChunkSize.QuadPart + sizeof(Xp3Index->info);
	}

	InitializeCompIndex(M2CompressedHeader);
	M2CompressedHeader.OriginalSize = IndexStreamM2.GetSize();
	M2CompressedHeader.ArchiveSize  = compressBound(M2CompressedHeader.OriginalSize);

	LOOP_ONCE
	{
		auto M2CompressedChunk = AllocateMemorySafeP<BYTE>(M2CompressedHeader.ArchiveSize);
		if (!M2CompressedChunk)
		{
			m_Proxyer->TellServerLogOutput(
				LogLevel::LOG_WARN,
				L"CoPackerTask::MakeKrkrSenrenBankaCompatibleArchiveGeneral : failed to allocate memory for compressing compressed chunk buffer"
			);

			M2CompressedHeader.ArchiveSize = M2CompressedHeader.OriginalSize;
			break;
		}

		if (Z_OK != compress2(
			M2CompressedChunk.get(),
			&M2CompressedHeader.ArchiveSize,
			IndexStreamM2.GetBuffer(),
			IndexStreamM2.GetSize(),
			Z_BEST_COMPRESSION)
			)
		{
			m_Proxyer->TellServerLogOutput(
				LogLevel::LOG_WARN,
				L"CoPackerTask::MakeKrkrSenrenBankaCompatibleArchiveGeneral : failed to invoke `compress2` for compressed chunk buffer"
			);

			M2CompressedHeader.ArchiveSize = M2CompressedHeader.OriginalSize;
			break;
		}

		IndexStreamM2.Set(M2CompressedChunk.get(), M2CompressedHeader.ArchiveSize);
	}

	//
	// end of files
	//

	M2CompressedHeader.Offset = Offset;
	Offset.QuadPart += M2CompressedHeader.ArchiveSize;

	if (m_Proxyer->GetIsSpcialChunkEncrypted() && m_Proxyer->GetSpecialChunkDecoder())
	{
		auto Decoder = m_Proxyer->GetSpecialChunkDecoder();
		Decoder(
			IndexStreamM2.GetBuffer(),
			IndexStreamM2.GetBuffer(),
			IndexStreamM2.GetSize() < 0x100 ? IndexStreamM2.GetSize() : 0x100
		);
	}
	File.Write(IndexStreamM2.GetBuffer(), IndexStreamM2.GetSize());

	auto IndexCompressedStream = SerializeToArray(M2CompressedHeader);

	IndexStream.GetStream().insert(
		IndexStream.GetStream().end(),
		IndexCompressedStream.begin(),
		IndexCompressedStream.end()
	);

	IndexStream.GetStream().insert(
		IndexStream.GetStream().end(),
		IndexStreamKr.GetStream().begin(),
		IndexStreamKr.GetStream().end()
	);

	XP3Header.IndexOffset = Offset;
	IndexHeader.bZlib = 1;
	IndexHeader.OriginalSize.QuadPart = (ULONG64)IndexStream.GetSize();
	IndexHeader.ArchiveSize.QuadPart  = (ULONG64)compressBound(IndexStream.GetSize());

	LOOP_ONCE
	{
		auto CompressedBuffer = AllocateMemorySafeP<BYTE>(IndexHeader.ArchiveSize.LowPart);
		if (!CompressedBuffer)
		{
			m_Proxyer->TellServerLogOutput(
				LogLevel::LOG_WARN,
				L"CoPackerTask::MakeKrkrSenrenBankaCompatibleArchiveGeneral : failed to allocate memory for compressing index buffer"
			);

			IndexHeader.bZlib = 0;
			IndexHeader.ArchiveSize.QuadPart = (ULONG64)IndexStream.GetSize();
			FileXP3.Write(&IndexHeader, sizeof(IndexHeader), &BytesTransfered);
			FileXP3.Write(IndexStream.GetBuffer(), IndexHeader.OriginalSize.LowPart);
			break;
		}

		if (Z_OK != compress2(
			CompressedBuffer.get(),
			&IndexHeader.ArchiveSize.LowPart,
			IndexStream.GetBuffer(),
			IndexStream.GetSize(),
			Z_BEST_COMPRESSION)
			)
		{
			m_Proxyer->TellServerLogOutput(
				LogLevel::LOG_WARN,
				L"CoPackerTask::MakeKrkrSenrenBankaCompatibleArchiveGeneral : failed to invoke `compress2` for index buffer"
			);

			IndexHeader.bZlib = 0;
			IndexHeader.ArchiveSize.QuadPart = (ULONG64)IndexStream.GetSize();
			FileXP3.Write(&IndexHeader, sizeof(IndexHeader), &BytesTransfered);
			FileXP3.Write(IndexStream.GetBuffer(), IndexHeader.OriginalSize.LowPart);
			break;
		}

		FileXP3.Write(&IndexHeader, sizeof(IndexHeader), &BytesTransfered);
		FileXP3.Write(CompressedBuffer.get(), IndexHeader.ArchiveSize.LowPart, &BytesTransfered);
	}

	FileXP3.Seek((LONG64)0, FILE_BEGIN);
	FileXP3.Write(&XP3Header, sizeof(XP3Header), &BytesTransfered);

	return STATUS_SUCCESS;
}



NTSTATUS CoPackerTask::MakeKrkrSenrenBankaCompatibleArchiveGeneralV2(KrkrPackerPass PassStatus, std::wstring& OutputArchive)
{
	NTSTATUS                             Status;
	NtFileDisk                           File, FileXP3;
	MemoryStream                         IndexStreamKr, IndexStreamM2, IndexStream;
	LARGE_INTEGER                        Size, Offset, BytesTransfered, RawIndexSize;
	SMyXP3IndexM2*                       Xp3Index;
	KRKR2_XP3_DATA_HEADER                IndexHeader;
	KRKRZ_XP3_INDEX_CHUNK_COMPRESSED_V2  M2CompressedHeader;
	SIZE_T                               RealCount;
	BYTE                                 FirstMagic[11] = { 0x58, 0x50, 0x33, 0x0D, 0x0A, 0x20, 0x0A, 0x1A, 0x8B, 0x67, 0x01 };
	std::wstring                         HashName;
	KRKR2_XP3_HEADER                     XP3Header(FirstMagic, 0);

	FileXP3.Write(&XP3Header, sizeof(XP3Header), &BytesTransfered);
	Offset.QuadPart = BytesTransfered.QuadPart;

	auto XP3IndexBuffer = AllocateMemorySafeP<SMyXP3IndexM2>(sizeof(SMyXP3IndexM2) * m_FileList.size());
	if (!XP3IndexBuffer)
		return STATUS_NO_MEMORY;

	RawIndexSize.QuadPart = 0;
	RealCount = 0;
	Xp3Index = XP3IndexBuffer.get();
	for (SIZE_T i = 0; i < m_FileList.size(); i++)
	{
		RtlZeroMemory(Xp3Index, sizeof(*Xp3Index));

		auto FullFilePath = m_BaseDir + L"\\" + m_FileList[i];
		Status = File.Open(FullFilePath.c_str());
		if (NT_FAILED(Status))
		{
			m_Proxyer->TellServerLogOutput(
				LogLevel::LOG_ERROR,
				L"CoPackerTask::MakeKrkrSenrenBankaCompatibleArchiveGeneralV2 : Open file failed = %s",
				FullFilePath.c_str());

			continue;
		}

		Status = File.GetSize(&Size);
		if (NT_FAILED(Status))
		{
			m_Proxyer->TellServerLogOutput(
				LogLevel::LOG_ERROR,
				L"CoPackerTask::MakeKrkrSenrenBankaCompatibleArchiveGeneralV2 : Get file size failed = %s",
				FullFilePath.c_str());

			continue;
		}

		if (PassStatus == KrkrPackerPass::NORMAL || PassStatus == KrkrPackerPass::ONE_PASS) {
			Status = ReadWriteEncrypt(File, FileXP3);
		}
		else
		{
			Size.QuadPart = 0;
			Status = ReadWriteEncrypt(m_FileList[i] + L".dummy", FileXP3, Size);
		}
		if (NT_FAILED(Status))
		{
			m_Proxyer->TellServerLogOutput(
				LogLevel::LOG_ERROR,
				L"CoPackerTask::MakeKrkrSenrenBankaCompatibleArchiveGeneralV2 : ReadWriteEncrypt failed = %s",
				FullFilePath.c_str());

			continue;
		}


		InitializeTimeIndex(Xp3Index->time);
		InitializeAdlrIndex(Xp3Index->adlr);
		InitializeSegmIndex(Xp3Index->segm);
		InitializeInfoIndex(Xp3Index->info);
		InitializeFileIndex(Xp3Index->file);
		InitializeYuzuIndex(Xp3Index->yuzu);

		Xp3Index->segm.segm[0].Offset = Offset;
		Xp3Index->segm.segm[0].bZlib = 0;
		Xp3Index->segm.segm[0].ArchiveSize.QuadPart = Size.QuadPart;
		Xp3Index->segm.segm[0].OriginalSize.QuadPart = Size.QuadPart;

		HashName.clear();
		if (PassStatus == KrkrPackerPass::NORMAL || PassStatus == KrkrPackerPass::TWO_PASS)
		{
			if (m_FileList[i].length() >= countof(Xp3Index->info.FileName))
			{
				m_Proxyer->TellServerLogOutput(
					LogLevel::LOG_ERROR,
					L"CoPackerTask::MakeKrkrSenrenBankaCompatibleArchiveGeneralV2 : "
					L"m_FileList[i].length() >= countof(Xp3Index->info.FileName), Buffer overflow"
				);

				return STATUS_UNSUCCESSFUL;
			}

			lstrcpynW(Xp3Index->info.FileName, m_FileList[i].c_str(), m_FileList[i].length() + 1);
		}
		else
		{
			wnsprintfW(
				Xp3Index->info.FileName,
				countof(Xp3Index->info.FileName),
				L"%s.dummy",
				m_FileList[i].c_str()
			);
		}

		GenMD5Code(ToLowerString(Xp3Index->info.FileName).c_str(), HashName);

		if (HashName.length() >= countof(Xp3Index->info.FileName))
		{
			m_Proxyer->TellServerLogOutput(
				LogLevel::LOG_ERROR,
				L"CoPackerTask::MakeKrkrSenrenBankaCompatibleArchiveGeneralV2 : "
				L"HashName.length() >= countof(Xp3Index->info.FileName), Buffer overflow"
			);

			return STATUS_UNSUCCESSFUL;
		}

		lstrcpynW(Xp3Index->info.FileName, HashName.c_str(), HashName.length() + 1);
		Xp3Index->info.FileNameLength = (USHORT)m_FileList[i].size();

		Xp3Index->info.OriginalSize.QuadPart = Size.QuadPart;
		Xp3Index->info.ArchiveSize.QuadPart = Size.QuadPart;

		if (m_Proxyer->GetXP3Filter()) {
			Xp3Index->info.EncryptedFlag = 0x80000000;
		}
		else {
			Xp3Index->info.EncryptedFlag = 0;
		}

		Xp3Index->info.ChunkSize.QuadPart = CalculateChunkSize(Xp3Index->info);
		Xp3Index->file.ChunkSize.QuadPart = CalculateFileSize(
			Xp3Index->time,
			Xp3Index->adlr,
			Xp3Index->segm,
			Xp3Index->info
		);

		if (!m_InfoNameZeroEnd) {
			Xp3Index->file.ChunkSize.QuadPart -= sizeof(WCHAR);
		}


		Xp3Index->yuzu.Hash = Xp3Index->adlr.Hash;
		Xp3Index->yuzu.FileNameLength = (USHORT)m_FileList[i].size();
		if (PassStatus == KrkrPackerPass::NORMAL || PassStatus == KrkrPackerPass::TWO_PASS) 
		{
			if (m_FileList[i].length() >= countof(Xp3Index->yuzu.FileName))
			{
				m_Proxyer->TellServerLogOutput(
					LogLevel::LOG_ERROR,
					L"CoPackerTask::MakeKrkrZCompatibleArchiveGeneralV2 : "
					L"m_FileList[i].length() >= countof(Xp3Index->yuzu.FileName), Buffer overflow"
				);

				return STATUS_UNSUCCESSFUL;
			}

			lstrcpynW(Xp3Index->yuzu.FileName, m_FileList[i].c_str(), m_FileList[i].length() + 1);
		}
		else
		{
			wnsprintfW(
				Xp3Index->yuzu.FileName,
				countof(Xp3Index->yuzu.FileName),
				L"%s.dummy",
				m_FileList[i].c_str()
			);
		}

		Xp3Index->yuzu.ChunkSize.QuadPart = CalculateChunkSize(Xp3Index->yuzu);

		IndexStreamM2.Append(SerializeToArray(Xp3Index->yuzu));
		IndexStreamKr.Append(SerializeToArray(Xp3Index->krkr2));

		Offset.QuadPart += Size.QuadPart;
		Xp3Index++;
		RealCount++;
		RawIndexSize.QuadPart += Xp3Index->file.ChunkSize.QuadPart + sizeof(Xp3Index->info);
	}

	InitializeCompIndex(M2CompressedHeader);
	M2CompressedHeader.OriginalSize = IndexStreamM2.GetSize();
	M2CompressedHeader.ArchiveSize = compressBound(M2CompressedHeader.OriginalSize);

	LOOP_ONCE
	{
		auto M2CompressedChunk = AllocateMemorySafeP<BYTE>(M2CompressedHeader.ArchiveSize);
		if (!M2CompressedChunk)
		{
			m_Proxyer->TellServerLogOutput(
				LogLevel::LOG_WARN,
				L"CoPackerTask::MakeKrkrSenrenBankaCompatibleArchiveGeneralV2 : failed to allocate memory for compressing compressed chunk buffer"
			);

			M2CompressedHeader.ArchiveSize = M2CompressedHeader.OriginalSize;
			break;
		}

		if (Z_OK != compress2(
			M2CompressedChunk.get(),
			&M2CompressedHeader.ArchiveSize,
			IndexStreamM2.GetBuffer(),
			IndexStreamM2.GetSize(),
			Z_BEST_COMPRESSION)
			)
		{
			m_Proxyer->TellServerLogOutput(
				LogLevel::LOG_WARN,
				L"CoPackerTask::MakeKrkrSenrenBankaCompatibleArchiveGeneralV2 : failed to invoke `compress2` for compressed chunk buffer"
			);

			M2CompressedHeader.ArchiveSize = M2CompressedHeader.OriginalSize;
			break;
		}

		IndexStreamM2.Set(M2CompressedChunk.get(), M2CompressedHeader.ArchiveSize);
	}

		//
		// end of files
		//

	M2CompressedHeader.Offset = Offset;
	Offset.QuadPart += M2CompressedHeader.ArchiveSize;

	if (m_Proxyer->GetIsSpcialChunkEncrypted() && m_Proxyer->GetSpecialChunkDecoder())
	{
		auto Decoder = m_Proxyer->GetSpecialChunkDecoder();
		Decoder(
			IndexStreamM2.GetBuffer(),
			IndexStreamM2.GetBuffer(),
			IndexStreamM2.GetSize() < 0x100 ? IndexStreamM2.GetSize() : 0x100
		);
	}
	File.Write(IndexStreamM2.GetBuffer(), IndexStreamM2.GetSize());

	auto IndexCompressedStream = SerializeToArray(M2CompressedHeader);

	IndexStream.GetStream().insert(
		IndexStream.GetStream().end(),
		IndexCompressedStream.begin(),
		IndexCompressedStream.end()
	);

	IndexStream.GetStream().insert(
		IndexStream.GetStream().end(),
		IndexStreamKr.GetStream().begin(),
		IndexStreamKr.GetStream().end()
	);

	XP3Header.IndexOffset = Offset;
	IndexHeader.bZlib = 1;
	IndexHeader.OriginalSize.QuadPart = (ULONG64)IndexStream.GetSize();
	IndexHeader.ArchiveSize.QuadPart = (ULONG64)compressBound(IndexStream.GetSize());

	LOOP_ONCE
	{
		auto CompressedBuffer = AllocateMemorySafeP<BYTE>(IndexHeader.ArchiveSize.LowPart);
		if (!CompressedBuffer)
		{
			m_Proxyer->TellServerLogOutput(
				LogLevel::LOG_WARN,
				L"CoPackerTask::MakeKrkrSenrenBankaCompatibleArchiveGeneralV2 : failed to allocate memory for compressing index buffer"
			);

			IndexHeader.bZlib = 0;
			IndexHeader.ArchiveSize.QuadPart = (ULONG64)IndexStream.GetSize();
			FileXP3.Write(&IndexHeader, sizeof(IndexHeader), &BytesTransfered);
			FileXP3.Write(IndexStream.GetBuffer(), IndexHeader.OriginalSize.LowPart);
			break;
		}

		if (Z_OK != compress2(
			CompressedBuffer.get(),
			&IndexHeader.ArchiveSize.LowPart,
			IndexStream.GetBuffer(),
			IndexStream.GetSize(),
			Z_BEST_COMPRESSION)
			)
		{
			m_Proxyer->TellServerLogOutput(
				LogLevel::LOG_WARN,
				L"CoPackerTask::MakeKrkrSenrenBankaCompatibleArchiveGeneralV2 : failed to invoke `compress2` for index buffer"
			);

			IndexHeader.bZlib = 0;
			IndexHeader.ArchiveSize.QuadPart = (ULONG64)IndexStream.GetSize();
			FileXP3.Write(&IndexHeader, sizeof(IndexHeader), &BytesTransfered);
			FileXP3.Write(IndexStream.GetBuffer(), IndexHeader.OriginalSize.LowPart);
			break;
		}

		FileXP3.Write(&IndexHeader, sizeof(IndexHeader), &BytesTransfered);
		FileXP3.Write(CompressedBuffer.get(), IndexHeader.ArchiveSize.LowPart, &BytesTransfered);
	}

	FileXP3.Seek((LONG64)0, FILE_BEGIN);
	FileXP3.Write(&XP3Header, sizeof(XP3Header), &BytesTransfered);

	return STATUS_SUCCESS;
}


NTSTATUS CoPackerTask::GetSenrenBankaPackInfo(PBYTE IndexData, ULONG IndexSize, NtFileDisk& File)
{
	NTSTATUS                Status;
	ULONG                   Offset, ByteTransferred;
	ULONG                   DecompSize, DecodeSize;
	BOOL                    RawFailed, NullTerminated;

	Offset          = 0;
	ByteTransferred = 0;
	NullTerminated  = FALSE;
	Status = ReadXp3M2CompressedChunk(IndexData, IndexSize, 0, m_OriginalCompressedChunk, ByteTransferred, NullTerminated);
	if (NT_FAILED(Status))
		return Status;

	DecompSize = m_OriginalCompressedChunk.OriginalSize;

	auto IndexBuffer      = AllocateMemorySafeP<BYTE>(m_OriginalCompressedChunk.OriginalSize);
	auto CompressedBuffer = AllocateMemorySafeP<BYTE>(m_OriginalCompressedChunk.ArchiveSize);

	if (!IndexBuffer || !CompressedBuffer)
	{
		SetLastMessage(FALSE, L"GetSenrenBankaPackInfo : Insufficient memory");
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	File.Seek(m_OriginalCompressedChunk.Offset, FILE_BEGIN);
	Status = File.Read(CompressedBuffer.get(), m_OriginalCompressedChunk.ArchiveSize);
	if (NT_FAILED(Status))
	{
		SetLastMessage(FALSE, L"GetSenrenBankaPackInfo : Couldn't decompress the special block");
		return Status;
	}


	RawFailed = FALSE;
	if (!m_Proxyer->GetIsSpcialChunkEncrypted())
	{
		if ((DecompSize = uncompress((PBYTE)IndexBuffer.get(), (PULONG)&DecompSize,
			(PBYTE)CompressedBuffer.get(), m_OriginalCompressedChunk.ArchiveSize)) != Z_OK)
		{
			RawFailed = TRUE;
		}
	}
	else
	{
		if (m_Proxyer->GetSpecialChunkDecoder())
		{
			DecodeSize = 0x100;
			if (DecodeSize < 0x100) {
				DecodeSize = m_OriginalCompressedChunk.ArchiveSize;
			}

			auto ChunkDeobfsFilter = m_Proxyer->GetSpecialChunkDecoder();
			if (ChunkDeobfsFilter) {
				ChunkDeobfsFilter(CompressedBuffer.get(), CompressedBuffer.get(), DecodeSize);
			}

			DecompSize = m_OriginalCompressedChunk.OriginalSize;
			if ((DecompSize = uncompress((PBYTE)IndexBuffer.get(), (PULONG)&DecompSize,
				(PBYTE)CompressedBuffer.get(), m_OriginalCompressedChunk.ArchiveSize)) != Z_OK) {
				RawFailed = TRUE;
			}
		}
		else {
			RawFailed = TRUE;
		}
	}

	if (RawFailed)
	{
		PrintConsoleW(L"Failed to gather information(at Compressed block)\n");
		SetLastMessage(FALSE, L"Failed to decompress special chunk");
		return STATUS_DATA_ERROR;
	}

	return STATUS_SUCCESS;
}



NTSTATUS CoPackerTask::GetSenrenBankaPackInfoV2(PBYTE IndexData, ULONG IndexSize, NtFileDisk& File)
{
	NTSTATUS            Status;
	ULONG               Offset, ByteTransferred;
	ULONG               DecompSize, DecodeSize;
	BOOL                RawFailed, NullTerminated;

	Offset = 0;
	ByteTransferred = 0;
	NullTerminated = FALSE;
	Status = ReadXp3M2CompressedChunk(IndexData, IndexSize, 0, m_OriginalCompressedChunkV2, ByteTransferred, NullTerminated);
	if (NT_FAILED(Status))
		return Status;

	DecompSize = m_OriginalCompressedChunkV2.OriginalSize;

	auto IndexBuffer = AllocateMemorySafeP<BYTE>(m_OriginalCompressedChunkV2.OriginalSize);
	auto CompressedBuffer = AllocateMemorySafeP<BYTE>(m_OriginalCompressedChunkV2.ArchiveSize);

	if (!IndexBuffer || !CompressedBuffer)
	{
		SetLastMessage(FALSE, L"GetSenrenBankaPackInfoV2 : Insufficient memory");
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	File.Seek(m_OriginalCompressedChunkV2.Offset, FILE_BEGIN);
	Status = File.Read(CompressedBuffer.get(), m_OriginalCompressedChunkV2.ArchiveSize);
	if (NT_FAILED(Status))
	{
		SetLastMessage(FALSE, L"GetSenrenBankaPackInfoV2 : Couldn't decompress the special block");
		return Status;
	}


	RawFailed = FALSE;
	if (!m_Proxyer->GetIsSpcialChunkEncrypted())
	{
		if ((DecompSize = uncompress((PBYTE)IndexBuffer.get(), (PULONG)&DecompSize,
			(PBYTE)CompressedBuffer.get(), m_OriginalCompressedChunkV2.ArchiveSize)) != Z_OK)
		{
			RawFailed = TRUE;
		}
	}
	else
	{
		if (m_Proxyer->GetSpecialChunkDecoder())
		{
			DecodeSize = 0x100;
			if (DecodeSize < 0x100) {
				DecodeSize = m_OriginalCompressedChunkV2.ArchiveSize;
			}

			auto ChunkDeobfsFilter = m_Proxyer->GetSpecialChunkDecoder();
			if (ChunkDeobfsFilter) {
				ChunkDeobfsFilter(CompressedBuffer.get(), CompressedBuffer.get(), DecodeSize);
			}

			DecompSize = m_OriginalCompressedChunkV2.OriginalSize;
			if ((DecompSize = uncompress((PBYTE)IndexBuffer.get(), (PULONG)&DecompSize,
				(PBYTE)CompressedBuffer.get(), m_OriginalCompressedChunkV2.ArchiveSize)) != Z_OK) {
				RawFailed = TRUE;
			}
		}
		else {
			RawFailed = TRUE;
		}
	}

	if (RawFailed)
	{
		PrintConsoleW(L"Failed to gather information(at Compressed block)\n");
		SetLastMessage(FALSE, L"Failed to decompress special chunk");
		return STATUS_DATA_ERROR;
	}

	return STATUS_SUCCESS;
}



NTSTATUS CoPackerTask::DetactArchiveFormat(PCWSTR FileName, Xp3WalkerProxy& Xp3Proxyer)
{
	NTSTATUS                Status;
	ULONG                   Count;
	NtFileDisk              File;
	BOOL                    IsXp3;
	KRKR2_XP3_HEADER        XP3Header;
	KRKR2_XP3_DATA_HEADER   DataHeader;
	LARGE_INTEGER           BeginOffset;
	ULONG                   OldHash, NewHash;
	IStream*                Stream;
	STATSTG                 Stat;
	WCHAR                   LastErrorMessage[0x200];

	Count = 0;
	IsXp3 = FALSE;
	m_ArchiveType = PackInfo::UnknownPack;

	Status = IsXp3File(FileName, IsXp3);
	if (NT_FAILED(Status))
	{
		wnsprintfW(
			LastErrorMessage, 
			countof(LastErrorMessage), 
			L"CoPackerTask::DetactPackFormat : failed %x", 
			Status
		);

		SetLastMessage(FALSE, LastErrorMessage);
		return Status;
	}

	if (!IsXp3) 
	{
		wnsprintfW(
			LastErrorMessage,
			countof(LastErrorMessage),
			L"CoPackerTask::DetactPackFormat : unsupported archive format"
		);

		SetLastMessage(FALSE, LastErrorMessage);
		return Status;
	}


	Status = WalkXp3ArchiveIndex(
		m_Proxyer,
		FileName,
		Xp3Proxyer,
		m_M2ChunkMagic,
		WalkXp3IndexCallbackM(KrkrProxyer, Buffer, Size, File, Magic, Proxyer)
		{
			m_ArchiveType = PackInfo::UnknownPack;
			return STATUS_SUCCESS;
		},
		WalkXp3IndexCallbackM(KrkrProxyer, Buffer, Size, File, Magic, Proxyer)
		{
			m_ArchiveType = PackInfo::NormalPack;

			return WalkNormalIndexBuffer(
					KrkrProxyer,
					Buffer,
					(ULONG)Size,
					File,
					Magic,
					Proxyer
				);
		},
		WalkXp3IndexCallbackM(KrkrProxyer, Buffer, Size, File, Magic, Proxyer)
		{
			NTSTATUS   Status;

			m_ArchiveType = PackInfo::KrkrZ_SenrenBanka;
			
			Status = WalkSenrenBankaIndexBuffer(
				KrkrProxyer,
				Buffer,
				(ULONG)Size,
				File,
				Magic,
				Proxyer
			);

			if (NT_FAILED(Status))
			{
				SetLastMessage(FALSE, L"WalkSenrenBankaIndexBuffer failed");
				return Status;
			}

			Status = GetSenrenBankaPackInfo(Buffer, Size, File);
			return Status;
		},
		WalkXp3IndexCallbackM(KrkrProxyer, Buffer, Size, File, Magic, Proxyer)
		{
			NTSTATUS   Status;

			m_ArchiveType = PackInfo::KrkrZ_SenrenBanka_V2;

			Status = WalkSenrenBankaIndexV2Buffer(
				KrkrProxyer,
				Buffer,
				(ULONG)Size,
				File,
				Magic,
				Proxyer
			);

			if (NT_FAILED(Status))
			{
				SetLastMessage(FALSE, L"WalkSenrenBankaIndexV2Buffer failed");
				return Status;
			}

			Status = GetSenrenBankaPackInfoV2(Buffer, Size, File);
			return Status;
		},
		WalkXp3IndexCallbackM(KrkrProxyer, Buffer, Size, File, Magic, Proxyer)
		{
			m_ArchiveType = PackInfo::KrkrZ_V2;

			return WalkNekoVol0IndexBuffer(
				KrkrProxyer,
				Buffer,
				(ULONG)Size,
				File,
				Magic,
				Proxyer
			);
		},
		WalkXp3IndexCallbackM(KrkrProxyer, Buffer, Size, File, Magic, Proxyer)
		{
			m_ArchiveType = PackInfo::KrkrZ;

			return WalkKrkrZIndexBuffer(
				KrkrProxyer,
				Buffer,
				(ULONG)Size,
				File,
				Magic,
				Proxyer
			);
		}
	);


	if (NT_FAILED(Status))
	{
		wnsprintfW(
			LastErrorMessage,
			countof(LastErrorMessage),
			L"CoPackerTask::DetactPackFormat : WalkXp3ArchiveIndex failed = %x",
			Status
		);

		SetLastMessage(FALSE, LastErrorMessage);

		m_ArchiveType = PackInfo::UnknownPack;
		return Status;
	}

	if (m_ArchiveType == PackInfo::NormalPack && Xp3Proxyer.IsM2Krkr() && m_M2ChunkMagic) {
		m_ArchiveType = PackInfo::KrkrZ;
	}

	if (m_ArchiveType == PackInfo::NormalPack && m_Proxyer->GetXP3Filter() == NULL)
	{
		LOOP_ONCE
		{
			auto Items = Xp3Proxyer.GetItems();

			if (Items.size() == 0)
				break;

			Stream = TVPCreateIStream(ttstr(Items[Items.size() / 2].info.FileName), TJS_BS_READ);
			if (Stream)
			{
				Stream->Stat(&Stat, STATFLAG_DEFAULT);
				NewHash = adler32IStream(0, Stream, Stat.cbSize);

				//
				// better algorithm?
				//

				OldHash = Items[Items.size() / 2].adlr.Hash;
				if (OldHash != NewHash) {
					m_ArchiveType = PackInfo::NormalPack_NoExporter;
				}

				Stream->Release();
			}
		}
	}

	return STATUS_SUCCESS;
}


VOID FormatPathNormal(wstring& PackageName, ttstr& out)
{
	out.Clear();
	out = L"file://./";
	for (ULONG iPos = 0; iPos < PackageName.length(); iPos++)
	{
		if (PackageName[iPos] == L':')
			continue;
		else if (PackageName[iPos] == L'\\')
			out += L'/';
		else
			out += PackageName[iPos];
	}
	out += L'>';
}




void CoPackerTask::ThreadFunction()
{
	NTSTATUS       Status;
	Xp3WalkerProxy Xp3Proxyer;

	if (IsBeingTerminated())
		return;

	Status = DetactArchiveFormat(m_OriginalArchivePath.c_str(), Xp3Proxyer);
	if (NT_FAILED(Status)) {
		m_Proxyer->TellServerLogOutput(LogLevel::LOG_ERROR, L"DetactArchiveFormat failed : %08x\n", Status);
		return;
	}

	Status = MakeKrkrCompatibleArchive();
	if (NT_FAILED(Status)) {
		m_Proxyer->TellServerLogOutput(LogLevel::LOG_ERROR, L"MakeKrkrCompatibleArchive failed : %08x\n", Status);
	}
}


