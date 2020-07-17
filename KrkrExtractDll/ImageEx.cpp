#include "ImageEx.h"


BOOL
__fastcall
InitBitmapHeader(
IMG_BITMAP_HEADER  *pHeader,
INT32               Width,
INT32               Height,
INT32               BitsPerPixel,
PINT32              pStride
)
{
	INT32 Stride;

	if (pHeader == NULL)
		return FALSE;

	ZeroMemory(pHeader, sizeof(*pHeader));
	pHeader->dwRawOffset = sizeof(*pHeader);
	pHeader->Info.dwHeight = Height;
	pHeader->Info.dwWidth = Width;
	pHeader->wTag = 0x4D42;
	pHeader->Info.dwInfoHeaderSize = sizeof(pHeader->Info);
	pHeader->Info.wLayer = 1;
	pHeader->Info.wBit = (UINT16)BitsPerPixel;
	//    pHeader->Info.dwClrUsed = 1;
	Stride = (Width * BitsPerPixel / 8 + 3) & ~3;
	if (pStride)
		*pStride = Stride;
	pHeader->dwFileSize = Height * Stride + sizeof(*pHeader);

	return TRUE;
}
