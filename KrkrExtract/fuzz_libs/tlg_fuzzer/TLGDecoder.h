#pragma once

#ifndef FUZZ_ME
#include "my.h"
#else
#include "NtToLinux.h"
#endif

#include <memory>

#pragma pack(1)

#define KRKR2_TLG5_MAGIC "TLG5.0"
#define KRKR2_TLG6_MAGIC "TLG6.0"
#define KRKR2_TLG0_MAGIC "TLG0.0"

typedef struct
{
	BYTE    Magic[0xB];
	BYTE    Colors;
	ULONG   Width;
	ULONG   Height;
	ULONG   BlockHeight;
	ULONG   BlockOffset[1];
} KRKR2_TLG5_HEADER;

typedef struct
{
	BYTE    Magic[0xB];
	BYTE    Colors;
	BYTE    DataFlags;
	BYTE    ColorType;             // currently always zero
	BYTE    ExternalGolombTable;   // currently always zero
	ULONG   Width;
	ULONG   Height;
	ULONG   MaxBitLength;
	BYTE    Data[1];
} KRKR2_TLG6_HEADER;

typedef struct
{
	BYTE MetaHeader[0xF];
	union
	{
		KRKR2_TLG5_HEADER tlg5;
		KRKR2_TLG6_HEADER tlg6;
	};
} KRKR2_TLG_WITH_META_HEADER;

//unsupported type : TLG0.0\x0sds

typedef struct
{
	CHAR   mark[11];		// "TLG6.0\x0raw\x1a"
	UCHAR  colors;
	UCHAR  flag;
	UCHAR  type;
	UCHAR  golomb_bit_length;
	ULONG  width;
	ULONG  height;
	ULONG  max_bit_length;
	ULONG  filter_length;
} tlg6_header_t;

#pragma pack()

BOOL DecodeTLG5(std::shared_ptr<BYTE> InBuffer, ULONG InSize, std::shared_ptr<BYTE>& OutBuffer, ULONG& OutSize);
BOOL DecodeTLG6(std::shared_ptr<BYTE> InBuffer, ULONG InSize, std::shared_ptr<BYTE>& OutBuffer, ULONG& OutSize);
LONG TVPTLG5DecompressSlide(UCHAR *out, const UCHAR *in, LONG insize, UCHAR *text, LONG initialr);

