#include "StatusMatcher.h"
#include "Helper.h"
#include "KrkrHeaders.h"

ChunkNodeKind NTAPI Xp3M2CompressedChunkNodeValidator::GetKind()
{
	return ChunkNodeKind::M2_COMPRESSED_NODE;
}


PCWSTR NTAPI Xp3M2CompressedChunkNodeValidator::GetName()
{
	return L"Xp3M2CompressedChunkNodeValidator";
}


BOOL NTAPI Xp3M2CompressedChunkNodeValidator::Validate(PBYTE Buffer, ULONG Size, DWORD& M2Magic)
{
	LARGE_INTEGER ChunkSize;
	LARGE_INTEGER InFileOffset;
	ULONG         Offset;
	USHORT        FileNameLength;
	SIZE_T        FileNameSize;
	DWORD         OriginalSize;
	DWORD         ArchiveSize;

	M2Magic = 0;
	Offset  = 0;

	Size += sizeof(KRKR2_INDEX_CHUNK_GENERAL);

	if (Offset + sizeof(M2Magic) > Size)
		return FALSE;

	Offset += sizeof(M2Magic);

	if (Offset += sizeof(ChunkSize) > Size)
		return FALSE;

	ChunkSize.QuadPart = *(PULONG64)(Buffer + Offset);
	Offset += sizeof(ChunkSize);

	if (ChunkSize.QuadPart != Size)
		return FALSE;

	if (Offset + FIELD_SIZE(KRKRZ_XP3_INDEX_CHUNK_COMPRESSED, Offset) > Size)
		return FALSE;

	InFileOffset = *(PLARGE_INTEGER)(Buffer + Offset);
	Offset += FIELD_SIZE(KRKRZ_XP3_INDEX_CHUNK_COMPRESSED, Offset);
	
	if (Offset + sizeof(OriginalSize) + sizeof(ArchiveSize) > Size)
		return FALSE;

	Offset += sizeof(OriginalSize) + sizeof(ArchiveSize);

	if (Offset += sizeof(FileNameLength) > Size)
		return FALSE;

	FileNameLength = *(PUSHORT)(Offset + Buffer);
	Offset += sizeof(FileNameLength);

	FileNameSize = 0;
	if (!SafeStringLength<WCHAR>((PCWSTR)(Offset + Buffer), FileNameLength, FileNameSize))
		return FALSE;

	if (FileNameSize != (USHORT)FileNameSize)
		return FALSE;

	if (FIELD_OFFSET(KRKRZ_XP3_INDEX_CHUNK_COMPRESSED, ProductName) + sizeof(WCHAR) * FileNameLength > Size)
		return FALSE;

	M2Magic = *(PDWORD)Buffer;
	return TRUE;
}


