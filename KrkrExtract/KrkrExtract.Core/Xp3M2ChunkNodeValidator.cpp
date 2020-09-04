#include "StatusMatcher.h"
#include "KrkrHeaders.h"
#include "Helper.h"

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
	DWORD         Hash;
	LARGE_INTEGER ChunkSize;
	ULONG         Offset;
	USHORT        FileNameLength;
	SIZE_T        FileNameSize;

	UNREFERENCED_PARAMETER(Hash);

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

	if (Offset + sizeof(Hash) > Size)
		return FALSE;
	
	Offset += sizeof(Hash);

	if (Offset += sizeof(FileNameLength) > Size)
		return FALSE;

	FileNameLength = *(PUSHORT)(Offset + Buffer);
	Offset += sizeof(FileNameLength);

	FileNameSize = 0;
	if (!SafeStringLength<WCHAR>((PCWSTR)(Offset + Buffer), FileNameLength, FileNameSize))
		return FALSE;

	if (FileNameSize != (USHORT)FileNameSize)
		return FALSE;

	if (FIELD_OFFSET(KRKRZ_XP3_INDEX_CHUNK_YUZU, FileName) + sizeof(WCHAR) * FileNameLength > Size)
		return FALSE;

	M2Magic = *(PDWORD)Buffer;
	return TRUE;
}



