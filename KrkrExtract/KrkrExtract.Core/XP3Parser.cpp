#include "XP3Parser.h"
#include "KrkrExtract.h"
#include <Xp3Access.h>
#include <zlib.h>
#include "magic_enum.hpp"
#include "trie.h"
#include "SafeMemory.h"
#include "ChunkReader.h"
#include "StatusMatcher.h"

static WCHAR* ProtectionInfo = L"$$$ This is a protected archive. $$$ 著作者はこのアーカイブが正規の利用方法以外の方法で展開されることを望んでいません。 $$$ This is a protected archive. $$$ 著作者はこのアーカイブが正規の利用方法以外の方法で展開されることを望んでいません。 $$$ This is a protected archive. $$$ 著作者はこのアーカイブが正規の利用方法以外の方法で展開されることを望んでいません。 $$$ Warning! Extracting this archive may infringe on author's rights. 警告 このアーカイブを展開することにより、あなたは著作者の権利を侵害するおそれがあります。.txt";

NTSTATUS IsXp3File(PCWSTR FileName, BOOL& IsXp3)
{
	NTSTATUS   Status;
	BOOL       Success;
	NtFileDisk File;

	IsXp3  = FALSE;
	Status = File.Open(FileName);
	if (NT_FAILED(Status))
		return Status;

	Status = IsXp3File(File, IsXp3);
	if (NT_FAILED(Status))
		return Status;

	File.Close();
	return STATUS_SUCCESS;
}


NTSTATUS FindTheFirstUnknownMagic(PBYTE Buffer, SIZE_T Size, DWORD& M2Magic)
{
	ULONG64        Offset;
	LARGE_INTEGER  ChunkSize;

	M2Magic = 0;
	Offset  = 0;

	if (Size < sizeof(TAG4('File')))
		return STATUS_BUFFER_TOO_SMALL;

	while (Offset < Size)
	{
		switch (*(PDWORD)(Buffer + Offset))
		{
		case CHUNK_MAGIC_FILE:
			Offset += sizeof(CHUNK_MAGIC_FILE);
			if (Offset + FIELD_SIZE(KRKR2_XP3_INDEX_CHUNK_FILE, ChunkSize) > Size)
				return STATUS_BUFFER_TOO_SMALL;

			ChunkSize = *(PLARGE_INTEGER)(Buffer + Offset);
			Offset += sizeof(ChunkSize);
			if (Offset > Size)
				return STATUS_BUFFER_TOO_SMALL;

			Offset += ChunkSize.QuadPart;
			if (Offset > Size)
				return STATUS_BUFFER_TOO_SMALL;

			break;


		default:
			M2Magic = *(PDWORD)(Buffer + Offset);
			return STATUS_SUCCESS;
		}
	}

	return STATUS_SUCCESS;
}



PackInfo MatchXp3ArchiveKind(std::vector<Xp3ItemGeneral>& Chunks)
{
	BOOL                          Success, HitCache;
	DWORD                         Magic;
	PackInfo                      ArchiveKind;
	Trie<ChunkNodeKind, PackInfo> ArchiveChunkBuilder;
	std::wstring                  HashName;
	std::vector<ChunkNodeKind>    Characteristic;
	std::unordered_map<ChunkNodeKind, std::shared_ptr<Xp3StatusValidator>> Xp3Validators;

	auto Krkr2Pattern =
	{
		ChunkNodeKind::FILE_CHUNK_NODE,
		ChunkNodeKind::FILE_CHUNK_NODE
	};

	auto KrkrZCompatiblePattern1 = 
	{
		ChunkNodeKind::FILE_CHUNK_NODE,
		ChunkNodeKind::M2_CHUNK_NODE
	};

	static auto KrkrZCompatiblePattern2 =
	{
		ChunkNodeKind::M2_CHUNK_NODE,
		ChunkNodeKind::FILE_CHUNK_NODE,
		ChunkNodeKind::M2_CHUNK_NODE
	};

	auto KrkrZNekoPattern = 
	{
		ChunkNodeKind::M2_CHUNK_NODE,
		ChunkNodeKind::M2_CHUNK_NODE,
	};

	auto KrkrZWithCompressPattern =
	{
		ChunkNodeKind::M2_COMPRESSED_NODE
	};

	auto KrkrZWithCompressPatternV2 =
	{
		ChunkNodeKind::M2_COMPRESSED_NODE_V2
	};

	ArchiveChunkBuilder.add(Krkr2Pattern,               PackInfo::NormalPack);
	ArchiveChunkBuilder.add(KrkrZCompatiblePattern1,    PackInfo::KrkrZ);
	ArchiveChunkBuilder.add(KrkrZCompatiblePattern2,    PackInfo::KrkrZ);
	ArchiveChunkBuilder.add(KrkrZNekoPattern,           PackInfo::KrkrZ_V2);
	ArchiveChunkBuilder.add(KrkrZWithCompressPattern,   PackInfo::KrkrZ_SenrenBanka);
	ArchiveChunkBuilder.add(KrkrZWithCompressPatternV2, PackInfo::KrkrZ_SenrenBanka_V2);

	Xp3Validators[ChunkNodeKind::FILE_CHUNK_NODE]       = std::shared_ptr<Xp3StatusValidator>(new Xp3FileNodeValidator());
	Xp3Validators[ChunkNodeKind::M2_CHUNK_NODE]         = std::shared_ptr<Xp3StatusValidator>(new Xp3M2ChunkNodeValidator());
	Xp3Validators[ChunkNodeKind::M2_COMPRESSED_NODE]    = std::shared_ptr<Xp3StatusValidator>(new Xp3M2CompressedChunkNodeValidator());
	Xp3Validators[ChunkNodeKind::M2_COMPRESSED_NODE_V2] = std::shared_ptr<Xp3StatusValidator>(new Xp3M2CompressedChunkNodeValidatorV2());


#define CHUNK_RETRY_THRESHOLD 3

	if (Chunks.size() < CHUNK_RETRY_THRESHOLD) 
	{
		PrintConsoleW(L"We need more chunks to detect archive format...\n");
		return PackInfo::UnknownPack;
	}

	//
	// Better optimization?
	// this chunk belongs to which kind
	//

	for (ULONG i = 0; i < CHUNK_RETRY_THRESHOLD; i++)
	{
		HitCache = FALSE;
		for (auto& Validator : Xp3Validators) 
		{
			Magic = 0;
			if (Validator.second->Validate(Chunks[i].Buffer.get(), Chunks[i].ChunkSize.LowPart, Magic) && Magic == Chunks[i].Magic) {
				Characteristic.push_back(Validator.first);
				HitCache = TRUE;
				continue;
			}

			//PrintConsoleW(L"chunk magic : %08x %08x\n", Magic, Chunks[i].Magic);
		}
		
		//
		// For debugging purposes
		//

		if (!HitCache) 
		{
			HashName.clear();

			GenMD5Code(
				Chunks[i].Buffer.get(), 
				Chunks[i].ChunkSize.LowPart + sizeof(KRKR2_INDEX_CHUNK_GENERAL),
				HashName
			);

			PrintConsoleW(L"Unrecognized chunk : %s\n", HashName.c_str());
			Chunks[i].Dump();
		}
	}

	//
	// gather some chunk characteristics
	//

	ArchiveKind = PackInfo::UnknownPack;
	if (Characteristic.size() == 0)
	{
		PrintConsoleW(L"Chunk characteristic array is empty...\n");
		return ArchiveKind;
	}

	PrintConsoleW(L"Chunk characteristic : \n========================\n");
	for (auto& ChunkKind : Characteristic)
	{
		auto&& KindName = magic_enum::enum_name(ChunkKind);
		PrintConsoleA("%s\n", std::string(KindName).c_str());
	}

	PrintConsoleW(L"========================\n");

	//
	// try to match archive kind until reach chunk threshold
	//


	for (ULONG i = 1; i <= CHUNK_RETRY_THRESHOLD; i++)
	{
		std::vector<ChunkNodeKind> SubCharacteristic;
		SubCharacteristic.insert(SubCharacteristic.end(), Characteristic.begin(), Characteristic.begin() + i);
		auto CurrentKind = ArchiveChunkBuilder.get(SubCharacteristic);
		if (CurrentKind.get() != nullptr && *CurrentKind.get() != PackInfo::UnknownPack) {

			//
			// Algorithm : the last fit
			//

			ArchiveKind = *CurrentKind.get();
		}
	}

	auto&& ArchiveKindName = magic_enum::enum_name(ArchiveKind);
	PrintConsoleA("Archive kind : %s\n", std::string(ArchiveKindName).c_str());

	return ArchiveKind;
}


NTSTATUS IsXp3File(NtFileDisk& File, BOOL& IsXp3)
{
	NTSTATUS        Status;
	BYTE            Buffer[0x10];
	static BYTE     XP3Header[] = { 0x58, 0x50, 0x33, 0x0D, 0x0A, 0x20, 0x0A, 0x1A, 0x8B, 0x67, 0x01 };

	IsXp3  = FALSE;
	Status = File.Seek((LONG64)0, FILE_BEGIN);
	if (!NT_SUCCESS(Status))
		return Status;

	Status = File.Read(Buffer, sizeof(XP3Header));
	if (!NT_SUCCESS(Status))
		return Status;

	if (RtlCompareMemory(Buffer, XP3Header, sizeof(XP3Header)) == sizeof(XP3Header))
	{
		IsXp3 = TRUE;
		return STATUS_SUCCESS;
	}

	Status = FindEmbededXp3OffsetSlow(File, NULL);
	if (NT_SUCCESS(Status))
	{
		IsXp3 = TRUE;
		return STATUS_SUCCESS;
	}

	return Status;
}

NTSTATUS FindEmbededXp3OffsetSlow(NtFileDisk &File, PLARGE_INTEGER Offset)
{
	NTSTATUS        Status;
	BYTE            Buffer[0x10000], *Xp3Signature;
	LARGE_INTEGER   BytesRead;

	static BYTE     XP3Header[] = { 0x58, 0x50, 0x33, 0x0D, 0x0A, 0x20, 0x0A, 0x1A, 0x8B, 0x67, 0x01 };

	Status = File.Seek((LONG64)0, FILE_BEGIN);
	if (!NT_SUCCESS(Status))
		return Status;

	Status = File.Read(Buffer, sizeof(IMAGE_DOS_HEADER));
	if (!NT_SUCCESS(Status))
		return Status;

	if (((PIMAGE_DOS_HEADER)Buffer)->e_magic != IMAGE_DOS_SIGNATURE)
		return STATUS_INVALID_IMAGE_WIN_32;

	Status = File.Seek(0x10, FILE_BEGIN);
	if (!NT_SUCCESS(Status))
		return Status;

	for (LONG64 FileSize = File.GetSize64(); FileSize > 0; FileSize -= sizeof(Buffer))
	{
		Status = File.Read(Buffer, sizeof(Buffer), &BytesRead);
		if (!NT_SUCCESS(Status))
			return Status;

		if (BytesRead.QuadPart < 0x10)
			return STATUS_NOT_FOUND;

		Xp3Signature = Buffer;
		for (ULONG_PTR Count = sizeof(Buffer) / 0x10; Count; Xp3Signature += 0x10, --Count)
		{
			if (!memcmp(Xp3Signature, XP3Header, sizeof(XP3Header)))
			{
				if (Offset)
					Offset->QuadPart = File.GetCurrentPos64() - sizeof(Buffer) + (Xp3Signature - Buffer);

				return STATUS_SUCCESS;
			}
		}
	}

	return STATUS_NOT_FOUND;
}


BOOL IsBuiltinChunk(DWORD ChunkMagic)
{
	switch (ChunkMagic)
	{
	case CHUNK_MAGIC_FILE:
	case CHUNK_MAGIC_INFO:
	case CHUNK_MAGIC_SEGM:
	case CHUNK_MAGIC_ADLR:
	case CHUNK_MAGIC_TIME:
		return TRUE;
	}

	return FALSE;
};


NTSTATUS
ConvertToGeneralXp3Chunk(
	_In_  PBYTE   Data,
	_In_  ULONG64 Size,
	_Inout_ std::vector<Xp3ItemGeneral>& Items,
	_Out_ DWORD&  M2Magic
)
{
	ULONG64        PtrOffset;

	PtrOffset = 0;
	M2Magic = 0;

	while (PtrOffset < Size)
	{
		Xp3ItemGeneral Chunk;

		if (PtrOffset + FIELD_SIZE(KRKR2_INDEX_CHUNK_GENERAL, Magic) > Size) {
			PrintConsoleW(L"ConvertToGeneralXp3Chunk failed : oob read magic, offset : %08x\n", PtrOffset);
			return STATUS_BUFFER_OVERFLOW;
		}
		
		Chunk.Magic = *(PDWORD)(Data + PtrOffset);
		PtrOffset += FIELD_SIZE(KRKR2_INDEX_CHUNK_GENERAL, Magic);
		if (PtrOffset + FIELD_SIZE(KRKR2_INDEX_CHUNK_GENERAL, ChunkSize) > Size) {
			PrintConsoleW(L"ConvertToGeneralXp3Chunk failed : oob read chunk size, offset : %08x\n", PtrOffset);
			return STATUS_BUFFER_OVERFLOW;
		}
		
		Chunk.ChunkSize = *(PLARGE_INTEGER)(Data + PtrOffset);
		PtrOffset += FIELD_SIZE(KRKR2_INDEX_CHUNK_GENERAL, ChunkSize);
		if (PtrOffset + Chunk.ChunkSize.QuadPart > Size) {
			PrintConsoleW(L"ConvertToGeneralXp3Chunk failed : oob read chunk data, offset : %08x\n", PtrOffset);
			return STATUS_BUFFER_OVERFLOW;
		}

		Chunk.Buffer = AllocateMemorySafeP<BYTE>(
			Chunk.ChunkSize.LowPart + 
			sizeof(KRKR2_INDEX_CHUNK_GENERAL)
			);

		if (!Chunk.Buffer) {
			PrintConsoleW(L"ConvertToGeneralXp3Chunk failed, oom, buffer size = 0x%08x\n", Chunk.ChunkSize.LowPart);
			return STATUS_NO_MEMORY;
		}

		RtlCopyMemory(
			Chunk.Buffer.get(),
			Data + PtrOffset - sizeof(KRKR2_INDEX_CHUNK_GENERAL),
			Chunk.ChunkSize.LowPart + sizeof(KRKR2_INDEX_CHUNK_GENERAL)
		);

		if (IsBuiltinChunk(Chunk.Magic) == FALSE && M2Magic == 0) {
			M2Magic = Chunk.Magic;
		}

		Items.push_back(Chunk);
		PtrOffset += Chunk.ChunkSize.QuadPart;
	}

	return STATUS_SUCCESS;
}


ForceInline BOOL CheckItem(XP3Index& Item)
{
	switch (Item.m_IsM2Format)
	{
	case FALSE:
		if (!StrNCompareW(
			Item.info.FileName,
			ProtectionInfo,
			min(CONST_STRLEN(ProtectionInfo), CONST_STRLEN(Item.info.FileName)))) {
			return FALSE;
		}
		break;

	default:
		if (!StrNCompareW(
			Item.yuzu.FileName,
			ProtectionInfo,
			min(CONST_STRLEN(ProtectionInfo), CONST_STRLEN(Item.yuzu.FileName)))) {
			return FALSE;
		}
		break;
	}

	return TRUE;
}


NTSTATUS
ReadXp3FileChunk(
	PBYTE Buffer,
	ULONG Size,
	ULONG Offset,
	KRKR2_XP3_INDEX_CHUNK_FILE& FileChunk,
	ULONG& ByteTransferred
)
{
	ByteTransferred = 0;
	RtlZeroMemory(&FileChunk, sizeof(FileChunk));

	if (ByteTransferred + sizeof(FileChunk) + Offset > Size)
		return STATUS_BUFFER_OVERFLOW;

	ByteTransferred = sizeof(FileChunk);
	RtlCopyMemory(&FileChunk, Buffer + Offset, sizeof(FileChunk));
	return STATUS_SUCCESS;
}

NTSTATUS
ReadXp3AdlrChunk(
	PBYTE Buffer,
	ULONG Size,
	ULONG Offset,
	KRKR2_XP3_INDEX_CHUNK_ADLR& AdlrChunk,
	ULONG& ByteTransferred
)
{
	ByteTransferred = 0;

	RtlZeroMemory(&AdlrChunk, sizeof(AdlrChunk));
	if (ByteTransferred + sizeof(AdlrChunk) + Offset > Size)
		return STATUS_BUFFER_OVERFLOW;
	
	ByteTransferred = sizeof(AdlrChunk);
	RtlCopyMemory(&AdlrChunk, Buffer + Offset, sizeof(AdlrChunk));
	return STATUS_SUCCESS;
}


NTSTATUS
ReadXp3TimeChunk(
	PBYTE Buffer,
	ULONG Size,
	ULONG Offset,
	KRKR2_XP3_INDEX_CHUNK_TIME& TimeChunk,
	ULONG& ByteTransferred
)
{
	ByteTransferred = 0;

	RtlZeroMemory(&TimeChunk, sizeof(TimeChunk));
	if (ByteTransferred + sizeof(TimeChunk) + Offset > Size)
		return STATUS_BUFFER_OVERFLOW;

	ByteTransferred = sizeof(TimeChunk);
	RtlCopyMemory(&TimeChunk, Buffer + Offset, sizeof(TimeChunk));
	return STATUS_SUCCESS;
}

NTSTATUS
ReadXp3SegmChunk(
	PBYTE Buffer,
	ULONG Size,
	ULONG Offset,
	KRKR2_XP3_INDEX_CHUNK_SEGM& SegmChunk,
	ULONG& ByteTransferred
)
{
	ByteTransferred = 0;
	RtlZeroMemory(&SegmChunk, sizeof(SegmChunk));
	
	if (ByteTransferred + FIELD_OFFSET(KRKR2_XP3_INDEX_CHUNK_SEGM, segm) + Offset > Size)
		return STATUS_BUFFER_OVERFLOW;

	RtlCopyMemory(&SegmChunk, Buffer + Offset, FIELD_OFFSET(KRKR2_XP3_INDEX_CHUNK_SEGM, segm));
	ByteTransferred += FIELD_OFFSET(KRKR2_XP3_INDEX_CHUNK_SEGM, segm);

	if (ByteTransferred + Offset + SegmChunk.ChunkSize.LowPart > Size)
		return STATUS_BUFFER_OVERFLOW;

	ByteTransferred += SegmChunk.ChunkSize.LowPart;
	return STATUS_SUCCESS;
}


NTSTATUS
ReadXp3InfoChunk(
	PBYTE Buffer,
	ULONG Size,
	ULONG Offset,
	KRKR2_XP3_INDEX_CHUNK_INFO& InfoChunk,
	ULONG& ByteTransferred,
	BOOL&  NullTerminated
)
{
	ByteTransferred = 0;
	NullTerminated  = TRUE;
	RtlZeroMemory(&InfoChunk, sizeof(InfoChunk));

	if (ByteTransferred + FIELD_OFFSET(KRKR2_XP3_INDEX_CHUNK_INFO, FileName) + Offset > Size)
		return STATUS_BUFFER_OVERFLOW;
	
	RtlCopyMemory(&InfoChunk, Buffer + Offset, FIELD_OFFSET(KRKR2_XP3_INDEX_CHUNK_INFO, FileName));
	ByteTransferred += FIELD_OFFSET(KRKR2_XP3_INDEX_CHUNK_INFO, FileName);

	if (ByteTransferred + Offset + InfoChunk.ChunkSize.LowPart > Size)
		return STATUS_BUFFER_OVERFLOW;

	if (FIELD_OFFSET(KRKR2_XP3_INDEX_CHUNK_INFO, FileName) - sizeof(KRKR2_INDEX_CHUNK_GENERAL) + 
		InfoChunk.FileNameLength * sizeof(WCHAR) == InfoChunk.ChunkSize.LowPart) {
		NullTerminated = FALSE;
	}

	lstrcpynW(
		InfoChunk.FileName, 
		(PCWSTR)(Buffer + Offset + FIELD_OFFSET(KRKR2_XP3_INDEX_CHUNK_INFO, FileName)), 
		min(InfoChunk.FileNameLength + 1, FIELD_SIZE(KRKR2_XP3_INDEX_CHUNK_INFO, FileName)) * sizeof(WCHAR)
	);

	ByteTransferred = sizeof(KRKR2_INDEX_CHUNK_GENERAL) + InfoChunk.ChunkSize.LowPart;
	
	return STATUS_SUCCESS;
}


NTSTATUS
ReadXp3M2InfoChunk(
	PBYTE Buffer,
	ULONG Size,
	ULONG Offset,
	KRKRZ_XP3_INDEX_CHUNK_YUZU& M2InfoChunk,
	ULONG& ByteTransferred,
	BOOL&  NullTerminated
)
{
	ByteTransferred = 0;
	NullTerminated = TRUE;
	RtlZeroMemory(&M2InfoChunk, sizeof(M2InfoChunk));

	if (ByteTransferred + FIELD_OFFSET(KRKRZ_XP3_INDEX_CHUNK_YUZU, FileName) + Offset > Size) 
	{
		PrintConsoleW(
			L"ReadXp3M2InfoChunk : Buffer overflow in pre-check, Field = %08x, Offset, Size = %08x\n",
			FIELD_OFFSET(KRKRZ_XP3_INDEX_CHUNK_YUZU, FileName),
			Offset,
			Size
			);

		return STATUS_BUFFER_OVERFLOW;
	}

	RtlCopyMemory(&M2InfoChunk, Buffer + Offset, FIELD_OFFSET(KRKRZ_XP3_INDEX_CHUNK_YUZU, FileName));
	ByteTransferred += FIELD_OFFSET(KRKRZ_XP3_INDEX_CHUNK_YUZU, FileName);

	if (sizeof(KRKR2_INDEX_CHUNK_GENERAL) + Offset + M2InfoChunk.ChunkSize.LowPart > Size)
	{
		PrintConsoleW(
			L"ReadXp3M2InfoChunk : Buffer overflow, Copied : %08x, Offset = %08x, Chunk = %08x\n",
			ByteTransferred,
			Offset,
			M2InfoChunk.ChunkSize.LowPart
		);

		return STATUS_BUFFER_OVERFLOW;
	}

	if (FIELD_OFFSET(KRKRZ_XP3_INDEX_CHUNK_YUZU, FileName) - sizeof(KRKR2_INDEX_CHUNK_GENERAL) + 
		M2InfoChunk.FileNameLength * sizeof(WCHAR) == M2InfoChunk.ChunkSize.LowPart) {
		NullTerminated = FALSE;
	}

	lstrcpynW(
		M2InfoChunk.FileName, 
		(PCWSTR)(Buffer + Offset + FIELD_OFFSET(KRKRZ_XP3_INDEX_CHUNK_YUZU, FileName)), 
		min(M2InfoChunk.FileNameLength + 1, FIELD_SIZE(KRKRZ_XP3_INDEX_CHUNK_YUZU, FileName)) * sizeof(WCHAR)
	);

	ByteTransferred = sizeof(KRKR2_INDEX_CHUNK_GENERAL) + M2InfoChunk.ChunkSize.LowPart;

	return STATUS_SUCCESS;
}


NTSTATUS
ReadXp3M2CompressedChunk(
	PBYTE Buffer,
	ULONG Size,
	ULONG Offset,
	KRKRZ_XP3_INDEX_CHUNK_COMPRESSED& CompressedChunk,
	ULONG& ByteTransferred,
	BOOL&  NullTerminated
)
{
	ByteTransferred = 0;
	NullTerminated = TRUE;
	RtlZeroMemory(&CompressedChunk, sizeof(CompressedChunk));

	if (ByteTransferred + FIELD_OFFSET(KRKRZ_XP3_INDEX_CHUNK_COMPRESSED, ProductName) + Offset > Size)
		return STATUS_BUFFER_OVERFLOW;

	RtlCopyMemory(&CompressedChunk, Buffer + Offset, FIELD_OFFSET(KRKRZ_XP3_INDEX_CHUNK_COMPRESSED, ProductName));
	ByteTransferred += FIELD_OFFSET(KRKRZ_XP3_INDEX_CHUNK_COMPRESSED, ProductName);

	if (ByteTransferred + Offset + CompressedChunk.ChunkSize.LowPart > Size)
		return STATUS_BUFFER_OVERFLOW;

	if (FIELD_OFFSET(KRKRZ_XP3_INDEX_CHUNK_COMPRESSED, ProductName) + CompressedChunk.LengthOfProduct * sizeof(WCHAR) == CompressedChunk.ChunkSize.LowPart) {
		NullTerminated = FALSE;
	}

	lstrcpynW(
		CompressedChunk.ProductName, 
		(PCWSTR)(Buffer + Offset + FIELD_OFFSET(KRKRZ_XP3_INDEX_CHUNK_COMPRESSED, ProductName)), 
		min(CompressedChunk.LengthOfProduct + 1, FIELD_OFFSET(KRKRZ_XP3_INDEX_CHUNK_COMPRESSED, ProductName))
	);

	ByteTransferred = sizeof(KRKR2_INDEX_CHUNK_GENERAL) + CompressedChunk.ChunkSize.LowPart;

	return STATUS_SUCCESS;
}


NTSTATUS
ReadXp3M2CompressedChunk(
	PBYTE Buffer,
	ULONG Size,
	ULONG Offset,
	KRKRZ_XP3_INDEX_CHUNK_COMPRESSED_V2& CompressedChunk,
	ULONG& ByteTransferred,
	BOOL&  NullTerminated
)
{
	ByteTransferred = 0;
	NullTerminated = TRUE;
	RtlZeroMemory(&CompressedChunk, sizeof(CompressedChunk));

	if (ByteTransferred + sizeof(KRKRZ_XP3_INDEX_CHUNK_COMPRESSED_V2) + Offset > Size)
		return STATUS_BUFFER_OVERFLOW;

	RtlCopyMemory(&CompressedChunk, Buffer + Offset, sizeof(KRKRZ_XP3_INDEX_CHUNK_COMPRESSED_V2));
	ByteTransferred += sizeof(KRKRZ_XP3_INDEX_CHUNK_COMPRESSED_V2);

	return STATUS_SUCCESS;
}


NTSTATUS
ReadXp3UnknownChunk(
	PBYTE Buffer,
	ULONG Size,
	ULONG Offset,
	ULONG& ByteTransferred
)
{
	KRKR2_INDEX_CHUNK_GENERAL Chunk;

	ByteTransferred = 0;
	RtlZeroMemory(&Chunk, sizeof(Chunk));

	if (ByteTransferred + sizeof(Chunk) + Offset > Size)
		return STATUS_BUFFER_OVERFLOW;

	RtlCopyMemory(&Chunk, Buffer + Offset, sizeof(Chunk));
	ByteTransferred += sizeof(Chunk) + Chunk.ChunkSize.LowPart;

	return STATUS_SUCCESS;
}



NTSTATUS
WalkKrkrZIndexBuffer(
	_In_ KrkrClientProxyer* Proxyer,
	_In_ PBYTE Decompress,
	_In_ ULONG Size,
	_In_ NtFileDisk& File,
	_In_ DWORD M2ChunkMagic,
	_In_ Xp3WalkerProxy& Proxy
)
{
	NTSTATUS           Status;
	ULONG              PtrOffset, InChunkPtrOffset;
	ULONG              ByteTransferred, FileChunkSize;
	XP3Index           Item;


	PtrOffset = 0;
	while (PtrOffset < Size)
	{
		if (PtrOffset + sizeof(CHUNK_MAGIC_YUZU) > Size)
			return STATUS_BUFFER_TOO_SMALL;

		switch (*(PDWORD)(Decompress + PtrOffset))
		{
		case CHUNK_MAGIC_FILE:
			ByteTransferred = 0;
			Item.m_IsM2Format                   = FALSE;
			Item.m_IsM2InfoNullTerminated       = FALSE;

			Status = ReadXp3FileChunk(Decompress, Size, PtrOffset, Item.file, ByteTransferred);
			if (NT_FAILED(Status))
				return Status;

			FileChunkSize = ByteTransferred;
			PtrOffset += sizeof(Item.file);

			if (PtrOffset + sizeof(CHUNK_MAGIC_YUZU))
				return STATUS_BUFFER_TOO_SMALL;

			InChunkPtrOffset = 0;
			while (InChunkPtrOffset < FileChunkSize)
			{
				switch (*(PDWORD)(Decompress + PtrOffset + InChunkPtrOffset))
				{
				case CHUNK_MAGIC_ADLR:
					ByteTransferred = 0;
					Status = ReadXp3AdlrChunk(Decompress, Size, PtrOffset + InChunkPtrOffset, Item.adlr, ByteTransferred);
					if (NT_FAILED(Status))
						return Status;

					InChunkPtrOffset += ByteTransferred;
					break;

				case CHUNK_MAGIC_TIME:
					ByteTransferred = 0;
					Status = ReadXp3TimeChunk(Decompress, Size, PtrOffset + InChunkPtrOffset, Item.time, ByteTransferred);
					if (NT_FAILED(Status))
						return Status;

					InChunkPtrOffset += ByteTransferred;
					break;

				case CHUNK_MAGIC_SEGM:
					ByteTransferred = 0;
					Status = ReadXp3SegmChunk(Decompress, Size, PtrOffset + InChunkPtrOffset, Item.segm, ByteTransferred);
					if (NT_FAILED(Status))
						return Status;

					InChunkPtrOffset += ByteTransferred;
					break;

				case CHUNK_MAGIC_INFO:
					ByteTransferred = 0;
					Status = ReadXp3InfoChunk(Decompress, Size, PtrOffset + InChunkPtrOffset, Item.info, ByteTransferred, Item.m_IsInfoNullTerminated);
					if (NT_FAILED(Status))
						return Status;

					InChunkPtrOffset += ByteTransferred;
					break;

				default:
					Proxyer->TellServerLogOutput(
						LogLevel::LOG_WARN, L"Unknown chunk in file chunk : %08x",
						*(PDWORD)(Decompress + PtrOffset + InChunkPtrOffset)
					);

					ByteTransferred = 0;
					Status = ReadXp3UnknownChunk(Decompress, Size, PtrOffset + InChunkPtrOffset, ByteTransferred);
					if (NT_FAILED(Status))
						return Status;

					InChunkPtrOffset += ByteTransferred;
					break;
				}
			}

			if (CheckItem(Item)) {
				Proxy.AppendItem(Item);
			}

			PtrOffset += ByteTransferred;
			break;

		default:
			if (*(PDWORD)(Decompress + PtrOffset) == M2ChunkMagic)
			{
				ByteTransferred   = 0;
				Item.m_IsM2Format = TRUE;
				Proxy.SetM2Krkr();
				Status = ReadXp3M2InfoChunk(Decompress, Size, PtrOffset, Item.yuzu, ByteTransferred, Item.m_IsM2InfoNullTerminated);
				if (NT_FAILED(Status))
					return Status;

				PtrOffset += ByteTransferred;
			}
			else
			{
				Proxyer->TellServerLogOutput(
					LogLevel::LOG_WARN, L"Unknown chunk in root chunk : %08x",
					*(PDWORD)(Decompress + PtrOffset)
				);

				ByteTransferred = 0;
				Status = ReadXp3UnknownChunk(Decompress, Size, PtrOffset, ByteTransferred);
				if (NT_FAILED(Status))
					return Status;

				PtrOffset += ByteTransferred;
			}
			break;
		}

	}
	return STATUS_SUCCESS;
}


NTSTATUS
WalkNormalIndexBuffer(
	_In_ KrkrClientProxyer* Proxyer,
	_In_ PBYTE Decompress, 
	_In_ ULONG Size, 
	_In_ NtFileDisk& File, 
	_In_ DWORD Unused, 
	_In_ Xp3WalkerProxy& Proxy
)
/*++

Routine Description:

	Iterate all file chunks within provided index buffer


Arguments:

	Decompress - Index buffer.
	Size - Size of index buffer
	File - The current file
	Unused - Unused
	Proxy - Data provider

Return Value:

	Status of iteration.

--*/
{
	NTSTATUS           Status;
	ULONG              PtrOffset, InChunkPtrOffset;
	ULONG              ByteTransferred, FileChunkSize;
	XP3Index           Item;
	

	PtrOffset = 0;
	while (PtrOffset < Size)
	{
		if (PtrOffset + sizeof(CHUNK_MAGIC_YUZU) > Size)
			return STATUS_BUFFER_TOO_SMALL;

		switch (*(PDWORD)(Decompress + PtrOffset))
		{
		case CHUNK_MAGIC_FILE:
			ByteTransferred = 0;
			Item.m_IsM2Format                   = FALSE;
			Item.m_IsM2InfoNullTerminated       = FALSE;

			Status = ReadXp3FileChunk(Decompress, Size, PtrOffset, Item.file, ByteTransferred);
			if (NT_FAILED(Status))
				return Status;

			FileChunkSize = ByteTransferred;
			PtrOffset += sizeof(Item.file);

			if (PtrOffset + sizeof(CHUNK_MAGIC_YUZU))
				return STATUS_BUFFER_TOO_SMALL;

			InChunkPtrOffset = 0;
			while (InChunkPtrOffset < FileChunkSize)
			{
				switch (*(PDWORD)(Decompress + PtrOffset + InChunkPtrOffset))
				{
				case CHUNK_MAGIC_ADLR:
					ByteTransferred = 0;
					Status = ReadXp3AdlrChunk(Decompress, Size, PtrOffset + InChunkPtrOffset, Item.adlr, ByteTransferred);
					if (NT_FAILED(Status))
						return Status;

					InChunkPtrOffset += ByteTransferred;
					break;

				case CHUNK_MAGIC_TIME:
					ByteTransferred = 0;
					Status = ReadXp3TimeChunk(Decompress, Size, PtrOffset + InChunkPtrOffset, Item.time, ByteTransferred);
					if (NT_FAILED(Status))
						return Status;

					InChunkPtrOffset += ByteTransferred;
					break;

				case CHUNK_MAGIC_SEGM:
					ByteTransferred = 0;
					Status = ReadXp3SegmChunk(Decompress, Size, PtrOffset + InChunkPtrOffset, Item.segm, ByteTransferred);
					if (NT_FAILED(Status))
						return Status;

					InChunkPtrOffset += ByteTransferred;
					break;

				case CHUNK_MAGIC_INFO:
					ByteTransferred = 0;
					Status = ReadXp3InfoChunk(Decompress, Size, PtrOffset + InChunkPtrOffset, Item.info, ByteTransferred, Item.m_IsInfoNullTerminated);
					if (NT_FAILED(Status))
						return Status;

					InChunkPtrOffset += ByteTransferred;
					break;

				default:
					Proxyer->TellServerLogOutput(
						LogLevel::LOG_WARN, L"Unknown chunk in file chunk : %08x",
						*(PDWORD)(Decompress + PtrOffset + InChunkPtrOffset)
					);

					ByteTransferred = 0;
					Status = ReadXp3UnknownChunk(Decompress, Size, PtrOffset + InChunkPtrOffset, ByteTransferred);
					if (NT_FAILED(Status))
						return Status;

					InChunkPtrOffset += ByteTransferred;
					break;
				}
			}

			if (CheckItem(Item)) {
				Proxy.AppendItem(Item);
			}

			PtrOffset += ByteTransferred;
			break;

		default:
			Proxyer->TellServerLogOutput(
				LogLevel::LOG_WARN, L"Unknown chunk in root chunk : %08x",
				*(PDWORD)(Decompress + PtrOffset)
			);

			ByteTransferred = 0;
			Status = ReadXp3UnknownChunk(Decompress, Size, PtrOffset, ByteTransferred);
			if (NT_FAILED(Status))
				return Status;

			PtrOffset += ByteTransferred;
			break;
		}

	}
	return STATUS_SUCCESS;
}



NTSTATUS 
WalkSenrenBankaIndexBuffer(
	_In_ KrkrClientProxyer* Proxyer,
	_In_ PBYTE Decompress,
	_In_ ULONG Size,
	_In_ NtFileDisk& File,
	_In_ DWORD M2ChunkMagic,
	_In_ Xp3WalkerProxy& Proxy,
	_In_ SENRENBANKACALLBACK Callback,
	_In_ PVOID CallbackPrivateContext
)
{
	NTSTATUS                           Status;
	KRKRZ_XP3_INDEX_CHUNK_COMPRESSED   CompressedChunk;
	ULONG                              PtrOffset, DecodeSize, DecompSize;
	ULONG                              ByteTransferred, FileChunkSize;
	BOOL                               NullTerminated, DecryptionFailed;
	XP3Index                           Item;
	Prototype::SpecialChunkDecoderFunc Decoder;
	WalkerCallbackStatus               CallbackStatus;

	Decoder = Proxyer->GetSpecialChunkDecoder();
	
	if (FIELD_OFFSET(KRKRZ_XP3_INDEX_CHUNK_COMPRESSED, ProductName) > Size)
		return STATUS_BUFFER_OVERFLOW;

	NullTerminated = TRUE;
	Status = ReadXp3M2CompressedChunk(Decompress, Size, 0, CompressedChunk, ByteTransferred, NullTerminated);
	if (NT_FAILED(Status))
		return Status;

	Status = File.Seek(CompressedChunk.Offset.LowPart, FILE_BEGIN);
	if (NT_FAILED(Status))
		return Status;

	auto IndexBuffer      = AllocateMemorySafeP<BYTE>(CompressedChunk.OriginalSize);
	auto CompressedBuffer = AllocateMemorySafeP<BYTE>(CompressedChunk.ArchiveSize);

	if (!IndexBuffer || !CompressedBuffer)
		return STATUS_NO_MEMORY;

	Status = File.Read(CompressedBuffer.get(), CompressedChunk.ArchiveSize);
	if (NT_FAILED(Status))
		return Status;

	if (Callback) 
	{
		CallbackStatus = Callback(
			Proxyer, 
			CompressedBuffer, 
			CompressedChunk.ArchiveSize,
			CompressedChunk.OriginalSize, 
			File, 
			CallbackPrivateContext
		);

		switch (CallbackStatus)
		{
		case WalkerCallbackStatus::STATUS_SKIP:
			return STATUS_SUCCESS;
			
		case WalkerCallbackStatus::STATUS_CONTINUE:
			break;

		default:
		case WalkerCallbackStatus::STATUS_ERROR:
			return STATUS_UNSUCCESSFUL;
		}
	}

	DecompSize       = CompressedChunk.OriginalSize;
	DecryptionFailed = FALSE;
	switch (Proxyer->GetIsSpcialChunkEncrypted())
	{
	case FALSE:
		if (uncompress(IndexBuffer.get(), (PULONG)&DecompSize, CompressedBuffer.get(), CompressedChunk.ArchiveSize) != Z_OK ||
			DecompSize != CompressedChunk.ArchiveSize) {
			DecryptionFailed = TRUE;
		}
		break;

	default:
		if (Decoder)
		{
			//
			// ??_7CxFilterDecrypt@@6B@ (hidden module)
			// .text:026D6315 loc_26D6315:                            ; CODE XREF: sub_26D62A0+71↑j
			// .text:026D6315                 test    al, al
			// .text : 026D6317               setz    al
			// .text : 026D631A               mov     byte ptr[ebp + arg_8 + 3], al
			// .text : 026D631D               test    al, al
			// .text : 026D631F               jnz     short loc_26D6375
			// .text : 026D6321               mov     eax, 100h
			// .text : 026D6326               cmp     esi, 100h
			// .text : 026D632C               ja      short loc_26D6330
			// .text : 026D632E               mov     eax, esi
			//

			DecodeSize = 0x100;
			if (CompressedChunk.ArchiveSize < 0x100) {
				DecodeSize = CompressedChunk.ArchiveSize;
			}

			Decoder(CompressedBuffer.get(), CompressedBuffer.get(), DecodeSize);
			DecompSize = CompressedChunk.OriginalSize;

			if (uncompress(IndexBuffer.get(), (PULONG)&DecompSize, CompressedBuffer.get(), CompressedChunk.ArchiveSize) != Z_OK ||
				DecompSize != CompressedChunk.OriginalSize) {
				DecryptionFailed = TRUE;
			}
		}
		break;
	}

	if (DecryptionFailed)
	{
		Proxyer->TellServerLogOutput(LogLevel::LOG_ERROR, L"Failed to decompress special chunk");
		return STATUS_DECRYPTION_FAILED;
	}

	Proxy.SetM2Krkr();

	PtrOffset = 0;
	while (PtrOffset < CompressedChunk.OriginalSize)
	{
		if (PtrOffset + sizeof(CHUNK_MAGIC_YUZU) > CompressedChunk.OriginalSize)
			return STATUS_BUFFER_TOO_SMALL;

		if (*(PDWORD)(IndexBuffer.get() + PtrOffset) == M2ChunkMagic)
		{
			ByteTransferred = 0;
			Item.m_IsM2Format = TRUE;
			Status = ReadXp3M2InfoChunk(
				IndexBuffer.get(), 
				CompressedChunk.OriginalSize, 
				PtrOffset, Item.yuzu, 
				ByteTransferred, 
				Item.m_IsM2InfoNullTerminated
			);

			if (NT_FAILED(Status))
				return Status;

			PtrOffset += ByteTransferred;
		}
		else
		{
			Proxyer->TellServerLogOutput(
				LogLevel::LOG_WARN, L"Unknown chunk in root chunk : %08x",
				*(PDWORD)(IndexBuffer.get() + PtrOffset)
			);

			ByteTransferred = 0;
			Status = ReadXp3UnknownChunk(IndexBuffer.get(), Size, PtrOffset, ByteTransferred);
			if (NT_FAILED(Status))
				return Status;

			PtrOffset += ByteTransferred;
			break;
		}

		Item.m_IsM2Format = TRUE;

		if (CheckItem(Item)) {
			Proxy.AppendItem(Item);
		}
	}

	Proxy.SetM2Krkr();
	return STATUS_SUCCESS;
}



NTSTATUS
WalkSenrenBankaIndexV2Buffer(
	_In_ KrkrClientProxyer* Proxyer,
	_In_ PBYTE Decompress,
	_In_ ULONG Size,
	_In_ NtFileDisk& File,
	_In_ DWORD M2ChunkMagic,
	_In_ Xp3WalkerProxy& Proxy,
	_In_ SENRENBANKACALLBACK Callback,
	_In_ PVOID CallbackPrivateContext
)
{
	NTSTATUS                             Status;
	KRKRZ_XP3_INDEX_CHUNK_COMPRESSED_V2  CompressedChunk;
	ULONG                                PtrOffset, DecodeSize, DecompSize;
	ULONG                                ByteTransferred, FileChunkSize;
	BOOL                                 NullTerminated, DecryptionFailed;
	DWORD                                ChunkMagic;
	XP3Index                             Item;
	Prototype::SpecialChunkDecoderFunc   Decoder;
	WalkerCallbackStatus                 CallbackStatus;

	Decoder = Proxyer->GetSpecialChunkDecoder();

	if (sizeof(KRKRZ_XP3_INDEX_CHUNK_COMPRESSED_V2) > Size)
	{
		PrintConsoleW(
			L"WalkSenrenBankaIndexV2Buffer failed : Size = %08x, header : %08x\n",
			Size,
			sizeof(KRKRZ_XP3_INDEX_CHUNK_COMPRESSED_V2)
		);

		return STATUS_BUFFER_OVERFLOW;
	}

	NullTerminated = TRUE;
	Status = ReadXp3M2CompressedChunk(Decompress, Size, 0, CompressedChunk, ByteTransferred, NullTerminated);
	if (NT_FAILED(Status)) {
		PrintConsoleW(L"WalkSenrenBankaIndexV2Buffer failed : ReadXp3M2CompressedChunk, %08x\n", Status);
		return Status;
	}

	Status = File.Seek(CompressedChunk.Offset.LowPart, FILE_BEGIN);
	if (NT_FAILED(Status)) {
		PrintConsoleW(L"WalkSenrenBankaIndexV2Buffer failed : File.Seek, %08x\n", Status);
		return Status;
	}

	auto IndexBuffer = AllocateMemorySafeP<BYTE>(CompressedChunk.OriginalSize);
	auto CompressedBuffer = AllocateMemorySafeP<BYTE>(CompressedChunk.ArchiveSize);

	if (!IndexBuffer || !CompressedBuffer)
		return STATUS_NO_MEMORY;

	Status = File.Read(CompressedBuffer.get(), CompressedChunk.ArchiveSize);
	if (NT_FAILED(Status))
		return Status;

	if (Callback)
	{
		CallbackStatus = Callback(
			Proxyer,
			CompressedBuffer,
			CompressedChunk.ArchiveSize,
			CompressedChunk.OriginalSize,
			File,
			CallbackPrivateContext
		);

		switch (CallbackStatus)
		{
		case WalkerCallbackStatus::STATUS_SKIP:
			return STATUS_SUCCESS;

		case WalkerCallbackStatus::STATUS_CONTINUE:
			break;

		default:
		case WalkerCallbackStatus::STATUS_ERROR:
			return STATUS_UNSUCCESSFUL;
		}
	}

	DecompSize = CompressedChunk.OriginalSize;
	DecryptionFailed = FALSE;
	switch (Proxyer->GetIsSpcialChunkEncrypted())
	{
	case FALSE:
		if (uncompress(IndexBuffer.get(), (PULONG)&DecompSize, CompressedBuffer.get(), CompressedChunk.ArchiveSize) != Z_OK ||
			DecompSize != CompressedChunk.ArchiveSize) {
			DecryptionFailed = TRUE;
		}
		break;

	default:
		if (Decoder)
		{
			//
			// ??_7CxFilterDecrypt@@6B@ (hidden module)
			// .text:026D6315 loc_26D6315:                            ; CODE XREF: sub_26D62A0+71↑j
			// .text:026D6315                 test    al, al
			// .text : 026D6317               setz    al
			// .text : 026D631A               mov     byte ptr[ebp + arg_8 + 3], al
			// .text : 026D631D               test    al, al
			// .text : 026D631F               jnz     short loc_26D6375
			// .text : 026D6321               mov     eax, 100h
			// .text : 026D6326               cmp     esi, 100h
			// .text : 026D632C               ja      short loc_26D6330
			// .text : 026D632E               mov     eax, esi
			//

			DecodeSize = 0x100;
			if (CompressedChunk.ArchiveSize < 0x100) {
				DecodeSize = CompressedChunk.ArchiveSize;
			}

			Decoder(CompressedBuffer.get(), CompressedBuffer.get(), DecodeSize);
			DecompSize = CompressedChunk.OriginalSize;

			if (uncompress(IndexBuffer.get(), (PULONG)&DecompSize, CompressedBuffer.get(), CompressedChunk.ArchiveSize) != Z_OK ||
				DecompSize != CompressedChunk.OriginalSize) {
				DecryptionFailed = TRUE;
			}
		}
		break;
	}

	if (DecryptionFailed) {
		Proxyer->TellServerLogOutput(LogLevel::LOG_ERROR, L"Failed to decompress special chunk");
		return STATUS_DECRYPTION_FAILED;
	}

	if (CompressedChunk.OriginalSize < sizeof(M2ChunkMagic)) {
		Proxyer->TellServerLogOutput(LogLevel::LOG_ERROR, L"Size of spcial chunk is too small");
		return STATUS_BUFFER_TOO_SMALL;
	}

	Proxy.SetM2Krkr();

	ChunkMagic = *(PDWORD)IndexBuffer.get();
	if (ChunkMagic != M2ChunkMagic) {
		Proxy.SetSpecialChunkM2(ChunkMagic);
	}
	else {
		ChunkMagic = M2ChunkMagic;
	}

	PtrOffset = 0;
	while (PtrOffset < CompressedChunk.OriginalSize)
	{
		if (PtrOffset + sizeof(CHUNK_MAGIC_YUZU) > CompressedChunk.OriginalSize)
			return STATUS_BUFFER_TOO_SMALL;

		if (*(PDWORD)(IndexBuffer.get() + PtrOffset) == ChunkMagic)
		{
			ByteTransferred = 0;
			Item.m_IsM2Format = TRUE;

			Status = ReadXp3M2InfoChunk(
				IndexBuffer.get(), 
				CompressedChunk.OriginalSize, 
				PtrOffset, 
				Item.yuzu, 
				ByteTransferred, 
				Item.m_IsM2InfoNullTerminated
			);

			if (NT_FAILED(Status))
				return Status;

			PtrOffset += ByteTransferred;
		}
		else
		{
			Proxyer->TellServerLogOutput(
				LogLevel::LOG_WARN, L"Unknown chunk in root chunk : %08x",
				*(PDWORD)(IndexBuffer.get() + PtrOffset)
			);

			ByteTransferred = 0;
			Status = ReadXp3UnknownChunk(IndexBuffer.get(), CompressedChunk.OriginalSize, PtrOffset, ByteTransferred);
			if (NT_FAILED(Status))
				return Status;

			PtrOffset += ByteTransferred;
			break;
		}

		Item.m_IsM2Format = TRUE;

		if (CheckItem(Item)) {
			Proxy.AppendItem(Item);
		}
	}

	Proxy.SetM2Krkr();
	return STATUS_SUCCESS;
}


NTSTATUS
WalkNekoVol0IndexBuffer(
	_In_ KrkrClientProxyer* Proxyer,
	_In_ PBYTE Decompress,
	_In_ ULONG Size,
	_In_ NtFileDisk& File,
	_In_ DWORD M2ChunkMagic,
	_In_ Xp3WalkerProxy& Proxy
)
{
	NTSTATUS          Status;
	ULONG             PtrOffset, ByteTransferred;
	XP3Index          Item;

	if (Size < sizeof(CHUNK_MAGIC_FILE))
		return STATUS_BUFFER_TOO_SMALL;

	Proxy.SetM2Krkr();

	PtrOffset = 0;
	while (PtrOffset < Size)
	{
		if (PtrOffset + sizeof(CHUNK_MAGIC_YUZU) > Size)
			return STATUS_BUFFER_TOO_SMALL;

		if (*(PDWORD)(Decompress + PtrOffset) == M2ChunkMagic)
		{
			ByteTransferred = 0;
			Item.m_IsM2Format = TRUE;
			Status = ReadXp3M2InfoChunk(Decompress, Size, PtrOffset, Item.yuzu, ByteTransferred, Item.m_IsM2InfoNullTerminated);
			if (NT_FAILED(Status))
				return Status;

			PtrOffset += ByteTransferred;

			Item.m_IsM2Format = TRUE;

			if (CheckItem(Item)) {
				Proxy.AppendItem(Item);
			}
		}
		else {
			break;
		}
	}

	Proxy.SetM2Krkr();
	return STATUS_SUCCESS;
}

