#include "StatusMatcher.h"
#include "XP3Parser.h"

ChunkNodeKind NTAPI Xp3M2CompressedChunkNodeValidatorV3::GetKind()
{
	return ChunkNodeKind::M2_COMPRESSED_NODE;
}

PCWSTR NTAPI Xp3M2CompressedChunkNodeValidatorV3::GetName()
{
	return L"Xp3M2CompressedChunkNodeValidatorV3";
}

BOOL NTAPI Xp3M2CompressedChunkNodeValidatorV3::Validate(PBYTE Buffer, ULONG Size, DWORD& Magic)
{
	KRKRZ_XP3_INDEX_CHUNK_COMPRESSED_V3 Chunk;

	Magic = 0;
	if (Size != sizeof(Chunk) - sizeof(KRKR2_INDEX_CHUNK_GENERAL))
		return FALSE;

	RtlCopyMemory(&Chunk, Buffer, sizeof(Chunk));
	if (IsBuiltinChunk(Chunk.Magic))
		return FALSE;

	if (Chunk.Offset.HighPart != 0)
		return FALSE;

	switch (Chunk.Kind)
	{
	case 0:
	case 1:
		break;

	default:
		return FALSE;
	}

	Magic = Chunk.Magic;
	return TRUE;
}


