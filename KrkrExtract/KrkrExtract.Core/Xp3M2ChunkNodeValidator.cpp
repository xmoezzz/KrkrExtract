#include "StatusMatcher.h"
#include "KrkrHeaders.h"
#include "Helper.h"
#include "XP3Parser.h"

ChunkNodeKind NTAPI Xp3M2ChunkNodeValidator::GetKind()
{
	return ChunkNodeKind::M2_CHUNK_NODE;
}


PCWSTR NTAPI Xp3M2ChunkNodeValidator::GetName()
{
	return L"Xp3M2ChunkNodeValidator";
}


BOOL NTAPI Xp3M2ChunkNodeValidator::Validate(PBYTE Buffer, ULONG Size, DWORD& M2Magic)
{
	KRKRZ_XP3_INDEX_CHUNK_YUZU Chunk;
	ULONG                      Offset;
	SIZE_T                     FileNameSize;

	M2Magic = 0;
	Offset  = 0;

	Size += sizeof(KRKR2_INDEX_CHUNK_GENERAL);

	if (Offset + sizeof(M2Magic) > Size) {
		return FALSE;
	}

	if (IsBuiltinChunk(*(PDWORD)Buffer)) {
		return FALSE;
	}

	RtlZeroMemory(&Chunk, sizeof(Chunk));

	if (FIELD_OFFSET(KRKRZ_XP3_INDEX_CHUNK_YUZU, FileName) + Offset > Size) {
		return FALSE;
	}

	RtlCopyMemory(&Chunk, Buffer + Offset, FIELD_OFFSET(KRKRZ_XP3_INDEX_CHUNK_YUZU, FileName));
	Offset += FIELD_OFFSET(KRKRZ_XP3_INDEX_CHUNK_YUZU, FileName);

	if (Chunk.ChunkSize.LowPart + sizeof(KRKR2_INDEX_CHUNK_GENERAL) != Size) {
		return FALSE;
	}

	if (Chunk.FileNameLength == 0) {
		return FALSE;
	}

	if (Chunk.FileNameLength * sizeof(WCHAR) + Offset > Size) {
		return FALSE;
	}

	FileNameSize = 0;
	if (!SafeStringLength<WCHAR>((PCWSTR)(Offset + Buffer), Chunk.FileNameLength, FileNameSize)) {
		return FALSE;
	}

	if (FileNameSize != (USHORT)FileNameSize) {
		return FALSE;
	}

	if (FIELD_OFFSET(KRKRZ_XP3_INDEX_CHUNK_YUZU, FileName) + sizeof(WCHAR) * Chunk.FileNameLength > Size) {
		return FALSE;
	}

	M2Magic = *(PDWORD)Buffer;
	return TRUE;
}



