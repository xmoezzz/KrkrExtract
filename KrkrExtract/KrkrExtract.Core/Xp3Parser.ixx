#include "XP3Parser.h"

export module Xp3Parser;

export
template<
	class IndexCallback0,
	class IndexCallback1,
	class IndexCallback2,
	class IndexCallback3,
	class IndexCallback4,
	class IndexCallback5
>
inline NTSTATUS
WalkXp3ArchiveIndex(
	_In_ KrkrClientProxyer* Proxyer,
	_In_ PCWSTR FilePath,
	_In_ Xp3WalkerProxy& Xp3Proxy,
	_Out_ DWORD& M2ChunkMagic,
	_In_ IndexCallback0 cbUnknownIndex,
	_In_ IndexCallback1 cbNormalIndex,
	_In_ IndexCallback2 cbSbOrEncryptedIndex,
	_In_ IndexCallback3 cbSbOrEncryptedV2Index,
	_In_ IndexCallback4 cbNekoVol0Index,
	_In_ IndexCallback5 cbKrkrZGeneralIndex
)
{
	NTSTATUS                    Status;
	NtFileDisk                  File, IndexFile;
	ULONG                       CountOfEntry;
	WCHAR                       IndexFileName[MAX_NTPATH];
	KRKR2_XP3_HEADER            XP3Header;
	KRKR2_XP3_DATA_HEADER       DataHeader;
	LARGE_INTEGER               BeginOffset, Offset;
	PackInfo                    ArchiveKind;
	ULONG64                     CompresseBufferSize;
	ULONG64                     DecompressBufferSize;
	std::vector<Xp3ItemGeneral> Xp3Chunks;

	M2ChunkMagic = 0;

	Status = File.Open(FilePath);
	if (NT_FAILED(Status))
		return Status;

	BeginOffset.QuadPart = 0;

	Status = File.Read(&XP3Header, sizeof(XP3Header));
	if (NT_FAILED(Status))
		return Status;

	if ((*(PUSHORT)XP3Header.Magic) == IMAGE_DOS_SIGNATURE)
	{
		Status = FindEmbededXp3OffsetSlow(File, &BeginOffset);
		if (NT_FAILED(Status))
			return Status;

		File.Seek(BeginOffset, FILE_BEGIN);
		File.Read(&XP3Header, sizeof(XP3Header));
	}
	else {
		BeginOffset.QuadPart = 0;
	}

	Status = STATUS_UNSUCCESSFUL;
	if (RtlCompareMemory(StaticXP3V2Magic, XP3Header.Magic, sizeof(StaticXP3V2Magic)) != sizeof(StaticXP3V2Magic))
		return Status;

	CompresseBufferSize = 0x1000;
	DecompressBufferSize = 0x1000;

	auto    Compress = AllocateMemorySafeP<BYTE>((SIZE_T)CompresseBufferSize);
	auto    Decompress = AllocateMemorySafeP<BYTE>((SIZE_T)DecompressBufferSize);

	CountOfEntry = 0;
	DataHeader.OriginalSize = XP3Header.IndexOffset;

	do
	{
		Offset.QuadPart = DataHeader.OriginalSize.QuadPart + BeginOffset.QuadPart;
		File.Seek(Offset, FILE_BEGIN);
		if (NT_FAILED(File.Read(&DataHeader, sizeof(DataHeader))))
		{
			File.Close();
			return STATUS_BAD_DATA;
		}

		if (DataHeader.ArchiveSize.HighPart != 0 || DataHeader.ArchiveSize.LowPart == 0)
			continue;

		if (DataHeader.ArchiveSize.LowPart > CompresseBufferSize)
		{
			CompresseBufferSize = DataHeader.ArchiveSize.LowPart;
			Compress = AllocateMemorySafeP<BYTE>((SIZE_T)CompresseBufferSize);
		}

		if ((DataHeader.bZlib & 7) == 0)
		{
			Offset.QuadPart = -8;
			File.Seek(Offset, FILE_CURRENT);
		}

		Status = File.Read(Compress.get(), DataHeader.ArchiveSize.LowPart);
		if (NT_FAILED(Status))
			return STATUS_BAD_DATA;

		CountOfEntry++;

		switch (DataHeader.bZlib & 7)
		{
		case FALSE:
			if (DataHeader.ArchiveSize.LowPart > DecompressBufferSize)
			{
				DecompressBufferSize = DataHeader.ArchiveSize.LowPart;
				Decompress = AllocateMemorySafeP<BYTE>((SIZE_T)DecompressBufferSize);
				if (!Decompress)
					return STATUS_NO_MEMORY;
			}

			RtlCopyMemory(Decompress.get(), Compress.get(), DataHeader.ArchiveSize.LowPart);
			DataHeader.OriginalSize.LowPart = DataHeader.ArchiveSize.LowPart;
			break;

		default:
			if (DataHeader.OriginalSize.LowPart > DecompressBufferSize)
			{
				DecompressBufferSize = DataHeader.OriginalSize.LowPart;
				Decompress = AllocateMemorySafeP<BYTE>((SIZE_T)DecompressBufferSize);
				if (!Decompress)
					return STATUS_NO_MEMORY;
			}

			DataHeader.OriginalSize.HighPart = DataHeader.OriginalSize.LowPart;
			if (uncompress(Decompress.get(), (PULONG)&DataHeader.OriginalSize.HighPart,
				Compress.get(), DataHeader.ArchiveSize.LowPart) != Z_OK)
			{
				return STATUS_COMPRESSED_FILE_NOT_SUPPORTED;
			}
			DataHeader.OriginalSize.LowPart = DataHeader.OriginalSize.HighPart;
			break;
		}

		Status = ConvertToGeneralXp3Chunk(Decompress.get(), DataHeader.OriginalSize.LowPart, Xp3Chunks, M2ChunkMagic);
		if (NT_FAILED(Status))
			return Status;

		LOOP_ONCE
		{
			FormatStringW(IndexFileName, L"%s.index", FilePath);
			Status = IndexFile.Create(IndexFileName);
			if (NT_FAILED(Status))
				break;

			IndexFile.Write(Decompress.get(), DataHeader.OriginalSize.LowPart);
			IndexFile.Close();
		}

		ArchiveKind = MatchXp3ArchiveKind(Xp3Chunks);
		switch (ArchiveKind)
		{
		case PackInfo::KrkrZ:
			Status = cbKrkrZGeneralIndex(
				Proxyer,
				Decompress.get(),
				DataHeader.OriginalSize.LowPart,
				File,
				M2ChunkMagic,
				Xp3Proxy
			);

			if (NT_FAILED(Status))
				return Status;

			break;

		case PackInfo::KrkrZ_V2:
			Status = cbNekoVol0Index(
				Proxyer,
				Decompress.get(),
				DataHeader.OriginalSize.LowPart,
				File,
				M2ChunkMagic,
				Xp3Proxy
			);

			if (NT_FAILED(Status))
				return Status;

			break;

		case PackInfo::KrkrZ_SenrenBanka:
			Status = cbSbOrEncryptedIndex(
				Proxyer,
				Decompress.get(),
				DataHeader.OriginalSize.LowPart,
				File,
				M2ChunkMagic,
				Xp3Proxy
			);

			if (NT_FAILED(Status))
				return Status;

			break;

		case PackInfo::KrkrZ_SenrenBanka_V2:
			Status = cbSbOrEncryptedV2Index(
				Proxyer,
				Decompress.get(),
				DataHeader.OriginalSize.LowPart,
				File,
				M2ChunkMagic,
				Xp3Proxy
			);

			if (NT_FAILED(Status))
				return Status;

			break;

		case PackInfo::NormalPack:
		case PackInfo::NormalPack_NoExporter:
			Status = cbNormalIndex(
				Proxyer,
				Decompress.get(),
				DataHeader.OriginalSize.LowPart,
				File,
				M2ChunkMagic,
				Xp3Proxy
			);

			if (NT_FAILED(Status))
				return Status;

			break;

		case PackInfo::UnknownPack:
		default:
			PrintConsoleW(L"WalkXp3ArchiveIndex : Unknown archive\n");

			Status = cbUnknownIndex(
				Proxyer,
				Decompress.get(),
				DataHeader.OriginalSize.LowPart,
				File,
				M2ChunkMagic,
				Xp3Proxy
			);

			if (NT_FAILED(Status))
				return Status;

			break;
		}

	} while (DataHeader.bZlib & 0x80);

	File.Close();
	return CountOfEntry ? Status : STATUS_NOT_FOUND;
}