#pragma once

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

typedef uint32_t ULONG;
typedef ULONG *PULONG;
typedef unsigned short USHORT;
typedef USHORT *PUSHORT;
typedef unsigned char UCHAR;
typedef UCHAR *PUCHAR;


typedef int32_t             LONG_PTR;
typedef char                CHAR;
typedef uint32_t            DWORD;
typedef int                 BOOL;
typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef float               FLOAT;
typedef FLOAT               *PFLOAT;
typedef BOOL            *PBOOL;
typedef BOOL            *LPBOOL;
typedef BYTE            *PBYTE;
typedef BYTE            *LPBYTE;
typedef int             *PINT;
typedef int             *LPINT;
typedef WORD            *PWORD;
typedef WORD            *LPWORD;
typedef int32_t         *LPLONG;
typedef DWORD           *PDWORD;
typedef DWORD           *LPDWORD;
typedef void            *LPVOID;
typedef void            *PVOID;
typedef const void      *LPCVOID;
typedef int32_t          LONG;
typedef unsigned short   UINT16;
typedef unsigned int     UINT;
typedef uint32_t         UINT32;
typedef int32_t          INT32;


typedef int                 INT;
typedef unsigned int        *PUINT;

#define TRUE 1
#define FALSE 0

#define HEAP_ZERO_MEMORY 0
#define UNREFERENCED_PARAMETER(x) x;
#define CopyMemory(d, s, size) memcpy(d, s, size)


#define BI_RGB        0L
#define BI_RLE8       1L
#define BI_RLE4       2L
#define BI_BITFIELDS  3L
#define BI_JPEG       4L
#define BI_PNG        5L

inline void* GetProcessHeap() { return 0; }
inline void* HeapAlloc(void* Heap, int flags, size_t size) 
{
    return calloc(1, size);
}


inline void HeapFree(void* Heap, int flags, void* ptr)
{
    free(ptr);
}


#pragma pack(1)
typedef struct tagBITMAPFILEHEADER {
        WORD    bfType;
        DWORD   bfSize;
        WORD    bfReserved1;
        WORD    bfReserved2;
        DWORD   bfOffBits;
} BITMAPFILEHEADER, *LPBITMAPFILEHEADER, *PBITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER{
        DWORD      biSize;
        LONG       biWidth;
        LONG       biHeight;
        WORD       biPlanes;
        WORD       biBitCount;
        DWORD      biCompression;
        DWORD      biSizeImage;
        LONG       biXPelsPerMeter;
        LONG       biYPelsPerMeter;
        DWORD      biClrUsed;
        DWORD      biClrImportant;
} BITMAPINFOHEADER, *LPBITMAPINFOHEADER, *PBITMAPINFOHEADER;



typedef struct
{
	UINT16 Tag;                   // 0x00
	UINT32 FileSize;             // 0x02
	UINT32 Reserve;              // 0x06
	UINT32 RawOffset;            // 0x0A
	struct
	{
		UINT32 InfoHeaderSize;   // 0x0E
		INT32  Width;            // 0x12
		INT32  Height;           // 0x16
		UINT16 Layer;             // 0x1A
		UINT16 Bit;               // 0x1C
		UINT32 Compressed;       // 0x1E
		UINT32 SizeImage;        // 0x22
		INT32  XPelsPerMeter;     // 0x26
		INT32  YPelsPerMeter;     // 0x2A
		UINT32 ClrUsed;          // 0x2E
		UINT32 ClrImportant;     // 0x32
	} Info;
} IMAGE_BITMAP_HEADER, *PIMAGE_BITMAP_HEADER;


#pragma pack()



inline
BOOL
InitBitmapHeader(
	IMAGE_BITMAP_HEADER*Header,
	INT32               Width,
	INT32               Height,
	INT32               BitsPerPixel,
	LONG_PTR*           Stride
)
{
	UINT32 LocalStride, PaletteSize;

	if (Header == NULL)
		return FALSE;

	memset(Header, 0, sizeof(*Header));

	PaletteSize = BitsPerPixel == 8 ? 256 * 4 : 0;

	Header->RawOffset = sizeof(*Header) + PaletteSize;
	Header->Info.Height = Height;
	Header->Info.Width = Width;
	Header->Tag = 0x4D42;
	Header->Info.InfoHeaderSize = sizeof(Header->Info);
	Header->Info.Layer = 1;
	Header->Info.Bit = (UINT16)BitsPerPixel;
	//    pHeader->Info.dwClrUsed = 1;
	LocalStride = (Width * BitsPerPixel / 8 + 3) & ~3;
	if (Stride)
		*Stride = LocalStride;

	Header->FileSize = Height * LocalStride + sizeof(*Header) + PaletteSize;

	return TRUE;
}