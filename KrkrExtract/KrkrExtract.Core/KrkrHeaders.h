#pragma once

#include "my.h"
#include "ml.h"
#include <Xp3Access.h>

static BYTE XP3Mark1[] =
{ 0x58/*'X'*/, 0x50/*'P'*/, 0x33/*'3'*/, 0x0d/*'\r'*/,
0x0a/*'\n'*/, 0x20/*' '*/, 0x0a/*'\n'*/, 0x1a/*EOF*/,
0xff /* sentinel */ };

static BYTE XP3Mark2[] =
{ 0x8b, 0x67, 0x01, 0xff/* sentinel */ };

const static unsigned char StaticXP3V2Magic[0xB] = { 0x58, 0x50, 0x33, 0x0d, 0x0a, 0x20, 0x0a, 0x1a, 0x8b, 0x67, 0x01 };


#pragma pack(1)


typedef struct KRKR2_XP3_DATA_HEADER
{
	BYTE            bZlib;
	LARGE_INTEGER   ArchiveSize;
	LARGE_INTEGER   OriginalSize;
} KRKR2_XP3_DATA_HEADER;


struct KRKR2_XP3_HEADER
{
	BYTE          Magic[0xB];
	LARGE_INTEGER IndexOffset;

	KRKR2_XP3_HEADER()
	{
		IndexOffset.QuadPart = (ULONG64)0;
		CopyMemory(Magic, XP3Mark1, 8);
		CopyMemory(Magic + 8, XP3Mark2, 3);
	}
	KRKR2_XP3_HEADER(PBYTE Mark, ULONG64 Offset)
	{
		CopyMemory(Magic, Mark, 0xB);
		IndexOffset.QuadPart = Offset;
	}

};

typedef struct KRKR2_INDEX_CHUNK_GENERAL
{
	DWORD          Magic;
	LARGE_INTEGER  ChunkSize;
} KRKR2_INDEX_CHUNK_GENERAL;


typedef struct KRKRZ_XP3_INDEX_CHUNK_YUZU
{
	DWORD                  Magic; //Yuzu,etc...
	LARGE_INTEGER          ChunkSize;
	ULONG                  Hash;
	USHORT                 FileNameLength;
	WCHAR                  FileName[0x400];
} KRKRZ_XP3_INDEX_CHUNK_YUZU;


typedef struct KRKR2_XP3_INDEX_CHUNK_INFO
{
	DWORD                  Magic;     // 'info'
	LARGE_INTEGER          ChunkSize;
	ULONG                  EncryptedFlag;
	LARGE_INTEGER          OriginalSize;
	LARGE_INTEGER          ArchiveSize;
	USHORT                 FileNameLength;
	WCHAR                  FileName[0x400];

} KRKR2_XP3_INDEX_CHUNK_INFO;



typedef struct KRKR2_XP3_INDEX_CHUNK_SEGM_DATA
{
	BOOL            bZlib;     // bZlib & 7 -> 1: compressed  0: raw  other: error
	LARGE_INTEGER   Offset;
	LARGE_INTEGER   OriginalSize;
	LARGE_INTEGER   ArchiveSize;
} KRKR2_XP3_INDEX_CHUNK_SEGM_DATA;

typedef struct KRKR2_XP3_INDEX_CHUNK_SEGM
{
	DWORD                           Magic;
	LARGE_INTEGER                   ChunkSize;
	KRKR2_XP3_INDEX_CHUNK_SEGM_DATA segm[1];
} KRKR2_XP3_INDEX_CHUNK_SEGM;

typedef struct KRKR2_XP3_INDEX_CHUNK_ADLR
{
	DWORD           Magic;
	LARGE_INTEGER   ChunkSize;
	ULONG           Hash;
} KRKR2_XP3_INDEX_CHUNK_ADLR;

typedef struct KRKR2_XP3_INDEX_CHUNK_TIME
{
	DWORD           Magic;
	LARGE_INTEGER   ChunkSize;
	FILETIME        FileTime;
} KRKR2_XP3_INDEX_CHUNK_TIME;


typedef struct KRKR2_XP3_INDEX_CHUNK_FILE
{
	DWORD           Magic;
	LARGE_INTEGER   ChunkSize;
} KRKR2_XP3_INDEX_CHUNK_FILE;


typedef struct KRKRZ_XP3_INDEX_CHUNK_COMPRESSED
{
	DWORD          Magic;
	LARGE_INTEGER  ChunkSize;
	LARGE_INTEGER  Offset;
	DWORD          OriginalSize;
	DWORD          ArchiveSize;
	USHORT         LengthOfProduct;
	WCHAR          ProductName[MAX_PATH]; //Zero End
} KRKRZ_XP3_INDEX_CHUNK_COMPRESSED;


//
// without product name
//

typedef struct KRKRZ_XP3_INDEX_CHUNK_COMPRESSED_V2
{
	DWORD          Magic;
	LARGE_INTEGER  ChunkSize;
	LARGE_INTEGER  Offset;
	DWORD          OriginalSize;
	DWORD          ArchiveSize;
} KRKRZ_XP3_INDEX_CHUNK_COMPRESSED_V2;


typedef struct
{
	KRKR2_XP3_INDEX_CHUNK_FILE  file;
	KRKR2_XP3_INDEX_CHUNK_TIME  time;
	KRKR2_XP3_INDEX_CHUNK_ADLR  adlr;
	KRKR2_XP3_INDEX_CHUNK_SEGM  segm;
	KRKR2_XP3_INDEX_CHUNK_INFO  info;
} SMyXP3IndexNormal;

typedef struct
{
	KRKRZ_XP3_INDEX_CHUNK_YUZU  yuzu;

	union
	{
		SMyXP3IndexNormal           krkr2;
		struct {
			KRKR2_XP3_INDEX_CHUNK_FILE  file;
			KRKR2_XP3_INDEX_CHUNK_TIME  time;
			KRKR2_XP3_INDEX_CHUNK_ADLR  adlr;
			KRKR2_XP3_INDEX_CHUNK_SEGM  segm;
			KRKR2_XP3_INDEX_CHUNK_INFO  info;
		};
	};

} SMyXP3IndexM2;

#pragma pack()

class XP3Index : public SMyXP3IndexM2
{
public:
	XP3Index() {}

	XP3Index(const XP3Index& Other)
	{
		RtlCopyMemory(this, &Other, sizeof(Other));
	}

	XP3Index(Xp3ItemAccess* Other)
	{
		InternalReset();
		
		info.FileNameLength = (USHORT)StrLengthW(Other->GetFileName());
		(void) lstrcpynW(info.FileName, Other->GetFileName(), min(info.FileNameLength + 1, countof(info.FileName)));
		yuzu.FileNameLength = (USHORT)StrLengthW(Other->GetM2FileName());
		(void) lstrcpynW(yuzu.FileName, Other->GetM2FileName(), min(yuzu.FileNameLength + 1, countof(yuzu.FileName)));

		m_IsInfoNullTerminated   = Other->GetFileNameNullTerminated();
		m_IsM2InfoNullTerminated = Other->GetM2GFileNameNullTerminated();
		m_IsM2Format = Other->IsM2Item();
	}

	XP3Index& operator=(XP3Index& Other)
	{
		RtlCopyMemory(this, &Other, sizeof(Other));
		return *this;
	}

private:
	void InternalReset()
	{
		RtlZeroMemory(this, sizeof(*this));
		m_IsM2Format = FALSE;
		m_IsInfoNullTerminated = TRUE;
		m_IsM2InfoNullTerminated = TRUE;
		m_IsFakeItem = FALSE;
	}

public:
	BOOL m_IsM2Format                   = FALSE;
	BOOL m_IsInfoNullTerminated         = TRUE;
	BOOL m_IsM2InfoNullTerminated       = TRUE;
	BOOL m_IsFakeItem                   = FALSE;
};



#define CHUNK_MAGIC_FILE TAG4('File')
#define CHUNK_MAGIC_INFO TAG4('info')
#define CHUNK_MAGIC_SEGM TAG4('segm')
#define CHUNK_MAGIC_ADLR TAG4('adlr')
#define CHUNK_MAGIC_TIME TAG4('time')

#define CHUNK_MAGIC_YUZU       TAG4('Yuzu')
#define CHUNK_MAGIC_KRKRZ_M2   TAG4('eliF')
#define CHUNK_MAGIC_KRKRZ_NEKO TAG4('neko')
#define CHUNK_MAGIC_KRKRZ_FENG TAG4('feng')

