#include "CRC64.h"

uint64_t crc64(uint64_t crc, const unsigned char *s, uint64_t l)
{
	uint64_t j;
	for (j = 0; j < l; j++) {
		uint8_t byte = s[j];
		crc = crc64_tab[(uint8_t)crc ^ byte] ^ (crc >> 8);

	}
	return crc;

}


ULONG64 WINAPI WideStringTo64BitHash(WCHAR* Str)
{
	LARGE_INTEGER Result;
	Result.QuadPart = (ULONG64)0;
	Result.QuadPart = crc64(0, (BYTE*)Str, lstrlenW(Str));
	return Result.QuadPart;
}



ULONG64 WINAPI WideStringTo64BitHashConst(const WCHAR* Str)
{
	LARGE_INTEGER Result;
	Result.QuadPart = (ULONG64)0;
	Result.QuadPart = crc64(0, (BYTE*)Str, lstrlenW(Str));
	return Result.QuadPart;
}
