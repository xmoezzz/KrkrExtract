#include "StatusMatcher.h"
#include "KrkrHeaders.h"

ChunkNodeKind NTAPI Xp3FileNodeValidator::GetKind()
{
	return ChunkNodeKind::FILE_CHUNK_NODE;
}

PCWSTR NTAPI Xp3FileNodeValidator::GetName()
{
	return L"Xp3FileNodeValidator";
}

BOOL NTAPI Xp3FileNodeValidator::Validate(PBYTE Buffer, ULONG Size, DWORD& Magic)
{
	ULONG                      Offset;
	ULARGE_INTEGER             ChunkSize;
	KRKR2_XP3_INDEX_CHUNK_FILE File;

	Magic = 0;
	if (Buffer == NULL || Size < sizeof(File))
		return FALSE;

	RtlCopyMemory(&File, Buffer, sizeof(File));
	if (File.Magic != CHUNK_MAGIC_FILE)
	{
		PrintConsoleW(L"Xp3FileNodeValidator::Validate : Magic mismatch\n");
		return FALSE;
	}

	if (File.ChunkSize.QuadPart != Size)
	{
		PrintConsoleW(L"Xp3FileNodeValidator::Validate : Chunk size mismatch\n");
		return FALSE;
	}

	Offset = sizeof(File);

	//
	// Restore to raw buffer size
	//

	Size  += sizeof(File);
	
	while (Offset < Size)
	{
		if (Offset + sizeof(CHUNK_MAGIC_FILE) > Size)
		{
			PrintConsoleW(L"Xp3FileNodeValidator::Validate : Reach unexpected end when iterating subchunks in file chunk\n");
			PrintConsoleW(L"Offset : %08x , Size : %08x\n", Offset, Size);
			return FALSE;
		}

		switch (*(PDWORD)(Offset + Buffer))
		{
			//
			// normal chunk
			//

		case CHUNK_MAGIC_INFO:
		case CHUNK_MAGIC_SEGM:
		case CHUNK_MAGIC_ADLR:
		case CHUNK_MAGIC_TIME:
			Offset += sizeof(CHUNK_MAGIC_FILE);
			if (Offset + sizeof(ChunkSize) > Size) {
				PrintConsoleW(L"Xp3FileNodeValidator::Validate : Buffer overflow %08x\n", Offset);
				return FALSE;
			}

			ChunkSize.QuadPart = *(PULONG64)(Buffer + Offset);
			Offset += sizeof(ChunkSize);

			if (Offset + ChunkSize.LowPart > Size) {
				PrintConsoleW(L"Xp3FileNodeValidator::Validate : Buffer overflow %08x\n", Offset);
				return FALSE;
			}

			Offset += ChunkSize.LowPart;
			break;

		default:
			PrintConsoleW(L"Xp3FileNodeValidator::Validate : Found unknown chunk %08x\n", *(PDWORD)(Offset + Buffer));
			return FALSE;
		}
	}

	Magic = CHUNK_MAGIC_FILE;
	return TRUE;
}



