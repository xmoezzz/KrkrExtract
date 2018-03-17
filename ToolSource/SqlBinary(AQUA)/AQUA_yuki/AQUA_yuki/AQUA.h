#pragma once

#include <Windows.h>
#include "tp_stub.h"

/*

const tjs_uint SizeOfSelf; // structure size of tTVPXP3ExtractionFilterInfo itself
const tjs_uint64 Offset; // offset of the buffer data in uncompressed stream position  4
void * Buffer; // target data buffer                                       c
const tjs_uint BufferSize; // buffer size in bytes pointed by "Buffer"     10
const tjs_uint32 FileHash; // hash value of the file (since inteface v2)   14

1E001F20        83EC 08     sub esp,8
1E001F23        53          push ebx
1E001F24        55          push ebp
1E001F25        56          push esi
1E001F26        8B7424 18   mov esi,dword ptr [esp+18]; µ¹Ðò
1E001F2A        57          push edi
1E001F2B        33DB        xor ebx,ebx
1E001F2D        8B6E 14     mov ebp,dword ptr [esi+14];hash
1E001F30        8B46 08     mov eax,dword ptr [esi+8] ;offset2
1E001F33        8B56 04     mov edx,dword ptr [esi+4] ;offset1
1E001F36        8BFD        mov edi,ebp   ;;edi = hash
1E001F38        81E7 A80100 and edi,000001A8 ;;key[0]
1E001F3E        81C7 760700 add edi,776      ;;key[1]
1E001F44        3BC3        cmp eax,ebx
1E001F46        77 6A       ja short 1E001FB2
1E001F48        72 04       jb short 1E001F4E
1E001F4A        3BD7        cmp edx,edi
1E001F4C        73 56       jae short 1E001FA4
1E001F4E        8B5E 10     mov ebx,dword ptr [esi+10] ;Size
*/

void TVP_tTVPXP3ArchiveExtractionFilter_CONVENTION
TVPXP3ArchiveExtractionFilter(tTVPXP3ExtractionFilterInfo *info);
