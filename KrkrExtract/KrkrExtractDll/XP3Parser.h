#pragma once

#include "KrkrHeaders.h"

namespace PackInfo{ enum { KrkrZ, KrkrZ_V2, KrkrZ_SenrenBanka, NormalPack, NormalPack_NoExporter, UnknownPack/*Error Type*/ }; };


NTSTATUS FindEmbededXp3OffsetSlow(NtFileDisk &file, PLARGE_INTEGER Offset);

BOOL WINAPI InitIndexFileFirst(PBYTE pDecompress, ULONG Size);
BOOL WINAPI DetectCompressedChunk(PBYTE pDecompress, ULONG Size);
BOOL WINAPI InitIndexFile_SenrenBanka(PBYTE pDecompress, ULONG Size, NtFileDisk& File);
BOOL WINAPI InitIndex_NekoVol0(PBYTE pDecompress, ULONG Size);
BOOL WINAPI IsCompatXP3(PBYTE Data, ULONG Size, DWORD* pMagic);

//Return package type
ULONG WINAPI FindChunkMagicFirst(PBYTE pDecompress, ULONG Size);
