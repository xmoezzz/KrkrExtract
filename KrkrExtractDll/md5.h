/*
* This is the header file for the MD5 message-digest algorithm.
* The algorithm is due to Ron Rivest.  This code was
* written by Colin Plumb in 1993, no copyright is claimed.
* This code is in the public domain; do with it what you wish.
*
* Equivalent code is available from RSA Data Security, Inc.
* This code has been tested against that, and is equivalent,
* except that you don't need to include two pages of legalese
* with every copy.
*
* To compute the message digest of a chunk of bytes, declare an
* MD5Context structure, pass it to MD5Init, call MD5Update as
* needed on buffers full of bytes, and then call MD5Final, which
* will fill a supplied 16-byte array with the digest.
*
* Changed so as no longer to depend on Colin Plumb's `usual.h'
* header definitions; now uses stuff from dpkg's config.h
*  - Ian Jackson <ijackson@nyx.cs.du.edu>.
* Still in the public domain.
*
* This copy has been re-released as GPL by
*   Wesley W. Terpstra <terpstra@users.sourceforge.net>
* for inclusion in the lurker project. It is now c++ code without autoconf.
*
*    This program is free software; you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation; version 2.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not, write to the Free Software
*    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef MD5_H
#define MD5_H

#include <climits>

#if UINT_MAX == 4294967295U
typedef unsigned int UWORD32;
#elif ULONG_MAX = 4294967295U
typedef unsigned long UWORD32;
#else
#error "No 32 bit type"
#endif

#define md5byte unsigned char

struct MD5Context {
	UWORD32 buf[4];
	UWORD32 bytes[2];
	UWORD32 in[16];
};

void MD5Init(struct MD5Context *context);
void MD5Update(struct MD5Context *context, md5byte const *buf, unsigned len);
void MD5Final(unsigned char digest[16], struct MD5Context *context);
void MD5Transform(UWORD32 buf[4], UWORD32 const in[16]);

#endif /* !MD5_H */
