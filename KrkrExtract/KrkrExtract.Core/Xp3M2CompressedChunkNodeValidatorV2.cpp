#include "StatusMatcher.h"
#include "XP3Parser.h"

ChunkNodeKind NTAPI Xp3M2CompressedChunkNodeValidatorV2::GetKind()
{
	return ChunkNodeKind::M2_COMPRESSED_NODE;
}

PCWSTR NTAPI Xp3M2CompressedChunkNodeValidatorV2::GetName()
{
	return L"Xp3M2CompressedChunkNodeValidatorV2";
}

BOOL NTAPI Xp3M2CompressedChunkNodeValidatorV2::Validate(PBYTE Buffer, ULONG Size, DWORD& Magic)
{
	KRKRZ_XP3_INDEX_CHUNK_COMPRESSED_V2 Chunk;
	
	Magic = 0;
	if (Size != sizeof(Chunk) - sizeof(KRKR2_INDEX_CHUNK_GENERAL))
		return FALSE;

	RtlCopyMemory(&Chunk, Buffer, sizeof(Chunk));
	if (IsBuiltinChunk(Chunk.Magic))
		return FALSE;

	if (Chunk.Offset.HighPart != 0)
		return FALSE;

	//
	// not likely
	// Information entropy detection?
	//

	if (Chunk.ArchiveSize > Chunk.OriginalSize)
		return FALSE;

	Magic = Chunk.Magic;
	return TRUE;
}


