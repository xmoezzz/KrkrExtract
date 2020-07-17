#pragma once

#include "my.h"

#define local static

#define BASE 65521UL    /* largest prime smaller than 65536 */
#define NMAX 5552
/* NMAX is the largest n such that 255n(n+1)/2 + (n+1)(BASE-1) <= 2^32-1 */

#define DO1(buf,i)  {adler += (buf)[i]; sum2 += adler;}
#define DO2(buf,i)  DO1(buf,i); DO1(buf,i+1);
#define DO4(buf,i)  DO2(buf,i); DO2(buf,i+2);
#define DO8(buf,i)  DO4(buf,i); DO4(buf,i+4);
#define DO16(buf)   DO8(buf,0); DO8(buf,8);

#  define MOD(a) a %= BASE
#  define MOD4(a) a %= BASE

/* ========================================================================= */
ULONG adler32IStream(ULONG adler, IStream *Stream, ULARGE_INTEGER len)
{
	unsigned long sum2;
	unsigned n;

	Byte    buffer[16];

	/* split Adler-32 into component sums */
	sum2 = (adler >> 16) & 0xffff;
	adler &= 0xffff;

	/* in case user likes doing a byte at a time, keep it fast */
	if (len.QuadPart == 1) {
		Stream->Read(buffer, 1, NULL);
		adler += buffer[0];
		if (adler >= BASE)
			adler -= BASE;
		sum2 += adler;
		if (sum2 >= BASE)
			sum2 -= BASE;
		return adler | (sum2 << 16);
	}

	/* initial Adler-32 value (deferred check for len == 1 speed) */
	if (Stream == NULL)
		return 1L;

	/* in case short lengths are provided, keep it somewhat fast */
	if (len.QuadPart < 16) {
		while (len.QuadPart--)
		{
			Stream->Read(buffer, 1, NULL);
			adler += buffer[0];
			sum2 += adler;
		}
		if (adler >= BASE)
			adler -= BASE;
		MOD4(sum2);             /* only added so many BASE's */
		return adler | (sum2 << 16);
	}

	/* do length NMAX blocks -- requires just one modulo operation */
	while (len.QuadPart >= NMAX) {
		len.QuadPart -= NMAX;
		n = NMAX / 16;          /* NMAX is divisible by 16 */
		do {
			Stream->Read(buffer, 16, NULL);
			DO16(buffer);          /* 16 sums unrolled */
		} while (--n);
		MOD(adler);
		MOD(sum2);
	}

	/* do remaining bytes (less than NMAX, still just one modulo) */
	if (len.QuadPart) {                  /* avoid modulos if none remaining */
		while (len.QuadPart >= 16) {
			len.QuadPart -= 16;
			Stream->Read(buffer, 16, NULL);
			DO16(buffer);
		}
		while (len.QuadPart--) {
			Stream->Read(buffer, 1, NULL);
			adler += buffer[0];
			sum2 += adler;
		}
		MOD(adler);
		MOD(sum2);
	}

	/* return recombined sums */
	return adler | (sum2 << 16);
}
