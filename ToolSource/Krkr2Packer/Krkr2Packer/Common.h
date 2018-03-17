#ifndef _Common_
#define _Common_

#include <Windows.h>
#include <string>

using std::wstring;

typedef unsigned char tjs_uint8;
typedef unsigned int  tjs_uint;
typedef unsigned long long   tjs_uint64;
typedef long long   tjs_int64;
typedef int           tjs_int;
typedef unsigned long tjs_uint32;
typedef char          tjs_char;
typedef unsigned __int16 tjs_uint16;


#define ttstr wstring;

#define TVP_XP3_INDEX_ENCODE_METHOD_MASK 0x07
#define TVP_XP3_INDEX_ENCODE_RAW      0
#define TVP_XP3_INDEX_ENCODE_ZLIB     1

#define TVP_XP3_INDEX_CONTINUE   0x80

#define TVP_XP3_FILE_PROTECTED (1<<31)

#define TVP_XP3_SEGM_ENCODE_METHOD_MASK  0x07
#define TVP_XP3_SEGM_ENCODE_RAW       0
#define TVP_XP3_SEGM_ENCODE_ZLIB      1


#pragma pack(1)


typedef struct PACK_FILE_INFO
{
	LONG    Encrypt;
	LONG    Compress;
	WCHAR   FileName[MAX_PATH];
} PACK_FILE_INFO;


typedef struct KRKR2_XP3_DATA_HEADER
{
	BYTE            bZlib;
	LARGE_INTEGER   ArchiveSize;
	LARGE_INTEGER   OriginalSize;
} KRKR2_XP3_DATA_HEADER;

typedef struct KRKR2_XP3_INDEX
{
	KRKR2_XP3_DATA_HEADER   DataHeader;
	BYTE                    Index[1];
} KRKR2_XP3_INDEX;


typedef struct KRKR2_XP3_HEADER
{
	BYTE          Magic[0xB];
	LARGE_INTEGER IndexOffset;
} KRKR2_XP3_HEADER;


typedef struct KRKRZ_XP3_INDEX_CHUNK_Yuzu
{
	char Magic[4]; //Yuzu
	LARGE_INTEGER ChunkSize;
	ULONG Hash;
	USHORT Len;
	wstring Name;

	KRKRZ_XP3_INDEX_CHUNK_Yuzu() : Hash(0){}
} KRKRZ_XP3_INDEX_CHUNK_Yuzu;


typedef struct KRKR2_XP3_INDEX_CHUNK_INFO2
{
	char            Magic[4];     // 'info'
	LARGE_INTEGER   ChunkSize;
	ULONG           EncryptedFlag;
	LARGE_INTEGER   OriginalSize;
	LARGE_INTEGER   ArchiveSize;
	USHORT          FileNameLength;
	wstring         FileName;
} KRKR2_XP3_INDEX_CHUNK_INFO2;

typedef struct KRKR2_XP3_INDEX_CHUNK_SEGM_DATA
{
	BOOL            bZlib;     // bZlib & 7 -> 1: compressed  0: raw  other: error
	LARGE_INTEGER   Offset;
	LARGE_INTEGER   OriginalSize;
	LARGE_INTEGER   ArchiveSize;
} KRKR2_XP3_INDEX_CHUNK_SEGM_DATA;

typedef struct KRKR2_XP3_INDEX_CHUNK_SEGM
{
	char                            Magic[4];     // 'segm'
	LARGE_INTEGER                   ChunkSize;
	KRKR2_XP3_INDEX_CHUNK_SEGM_DATA segm[1];
} KRKR2_XP3_INDEX_CHUNK_SEGM;

typedef struct KRKR2_XP3_INDEX_CHUNK_ADLR
{
	char            Magic[4]; // 'adlr'
	LARGE_INTEGER   ChunkSize;
	ULONG           Hash;
} KRKR2_XP3_INDEX_CHUNK_ADLR;

typedef struct KRKR2_XP3_INDEX_CHUNK_TIME
{
	char            Magic[4];    // TAG4('time')
	LARGE_INTEGER   ChunkSize;
	FILETIME        FileTime;
} KRKR2_XP3_INDEX_CHUNK_TIME;


typedef struct KRKR2_XP3_INDEX_CHUNK_FILE
{
	char            Magic[4];     // 'File'
	LARGE_INTEGER   ChunkSize;
} KRKR2_XP3_INDEX_CHUNK_FILE;

typedef struct
{
	KRKR2_XP3_INDEX_CHUNK_FILE  file;
	KRKR2_XP3_INDEX_CHUNK_TIME  time;
	KRKR2_XP3_INDEX_CHUNK_ADLR  adlr;
	KRKR2_XP3_INDEX_CHUNK_SEGM  segm;
	KRKR2_XP3_INDEX_CHUNK_INFO2 info;
} SMyXP3Index;


#pragma pack(4)

#define CHUNK_MAGIC_FILE    "File"
#define CHUNK_MAGIC_INFO    "info"
#define CHUNK_MAGIC_SEGM    "segm"
#define CHUNK_MAGIC_ADLR    "adlr"
#define CHUNK_MAGIC_TIME    "time"

#define MagicLength 4


#endif
