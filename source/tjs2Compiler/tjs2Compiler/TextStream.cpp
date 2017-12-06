#include "TextStream.h"
#include "TVPStream.h"


static tjs_int inline TVPWideCharToUtf8(tjs_char in, char * out)
{
	// convert a wide character 'in' to utf-8 character 'out'
	if (in < (1 << 7))
	{
		if (out)
		{
			out[0] = (char)in;
		}
		return 1;
	}
	else if (in < (1 << 11))
	{
		if (out)
		{
			out[0] = (char)(0xc0 | (in >> 6));
			out[1] = (char)(0x80 | (in & 0x3f));
		}
		return 2;
	}
	else if (in < (1 << 16))
	{
		if (out)
		{
			out[0] = (char)(0xe0 | (in >> 12));
			out[1] = (char)(0x80 | ((in >> 6) & 0x3f));
			out[2] = (char)(0x80 | (in & 0x3f));
		}
		return 3;
	}
#if 1
	else
	{
		MessageBoxW(NULL, TJS_W("UTF-16 to UTF-8"), L"Error", MB_OK);
	}
#else
	// 以下オリジナルのコードだけど、通らないはず。
	else if (in < (1 << 21))
	{
		if (out)
		{
			out[0] = (char)(0xf0 | (in >> 18));
			out[1] = (char)(0x80 | ((in >> 12) & 0x3f));
			out[2] = (char)(0x80 | ((in >> 6) & 0x3f));
			out[3] = (char)(0x80 | (in & 0x3f));
		}
		return 4;
	}
	else if (in < (1 << 26))
	{
		if (out)
		{
			out[0] = (char)(0xf8 | (in >> 24));
			out[1] = (char)(0x80 | ((in >> 16) & 0x3f));
			out[2] = (char)(0x80 | ((in >> 12) & 0x3f));
			out[3] = (char)(0x80 | ((in >> 6) & 0x3f));
			out[4] = (char)(0x80 | (in & 0x3f));
		}
		return 5;
	}
	else if (in < (1 << 31))
	{
		if (out)
		{
			out[0] = (char)(0xfc | (in >> 30));
			out[1] = (char)(0x80 | ((in >> 24) & 0x3f));
			out[2] = (char)(0x80 | ((in >> 18) & 0x3f));
			out[3] = (char)(0x80 | ((in >> 12) & 0x3f));
			out[4] = (char)(0x80 | ((in >> 6) & 0x3f));
			out[5] = (char)(0x80 | (in & 0x3f));
		}
		return 6;
	}
#endif
	return -1;
}
//---------------------------------------------------------------------------
tjs_int TVPWideCharToUtf8String(const tjs_char *in, char * out)
{
	// convert input wide string to output utf-8 string
	int count = 0;
	while (*in)
	{
		tjs_int n;
		if (out)
		{
			n = TVPWideCharToUtf8(*in, out);
			out += n;
		}
		else
		{
			n = TVPWideCharToUtf8(*in, NULL);
			/*
			in this situation, the compiler's inliner
			will collapse all null check parts in
			TVPWideCharToUtf8.
			*/
		}
		if (n == -1) return -1; // invalid character found
		count += n;
		in++;
	}
	return count;
}
//---------------------------------------------------------------------------
static bool inline TVPUtf8ToWideChar(const char * & in, tjs_char *out)
{
	// convert a utf-8 charater from 'in' to wide charater 'out'
	const unsigned char * & p = (const unsigned char * &)in;
	if (p[0] < 0x80)
	{
		if (out) *out = (tjs_char)in[0];
		in++;
		return true;
	}
	else if (p[0] < 0xc2)
	{
		// invalid character
		return false;
	}
	else if (p[0] < 0xe0)
	{
		// two bytes (11bits)
		if ((p[1] & 0xc0) != 0x80) return false;
		if (out) *out = ((p[0] & 0x1f) << 6) + (p[1] & 0x3f);
		in += 2;
		return true;
	}
	else if (p[0] < 0xf0)
	{
		// three bytes (16bits)
		if ((p[1] & 0xc0) != 0x80) return false;
		if ((p[2] & 0xc0) != 0x80) return false;
		if (out) *out = ((p[0] & 0x1f) << 12) + ((p[1] & 0x3f) << 6) + (p[2] & 0x3f);
		in += 3;
		return true;
	}
	else if (p[0] < 0xf8)
	{
		// four bytes (21bits)
		if ((p[1] & 0xc0) != 0x80) return false;
		if ((p[2] & 0xc0) != 0x80) return false;
		if ((p[3] & 0xc0) != 0x80) return false;
		if (out) *out = ((p[0] & 0x07) << 18) + ((p[1] & 0x3f) << 12) +
			((p[2] & 0x3f) << 6) + (p[3] & 0x3f);
		in += 4;
		return true;
	}
	else if (p[0] < 0xfc)
	{
		// five bytes (26bits)
		if ((p[1] & 0xc0) != 0x80) return false;
		if ((p[2] & 0xc0) != 0x80) return false;
		if ((p[3] & 0xc0) != 0x80) return false;
		if ((p[4] & 0xc0) != 0x80) return false;
		if (out) *out = ((p[0] & 0x03) << 24) + ((p[1] & 0x3f) << 18) +
			((p[2] & 0x3f) << 12) + ((p[3] & 0x3f) << 6) + (p[4] & 0x3f);
		in += 5;
		return true;
	}
	else if (p[0] < 0xfe)
	{
		// six bytes (31bits)
		if ((p[1] & 0xc0) != 0x80) return false;
		if ((p[2] & 0xc0) != 0x80) return false;
		if ((p[3] & 0xc0) != 0x80) return false;
		if ((p[4] & 0xc0) != 0x80) return false;
		if ((p[5] & 0xc0) != 0x80) return false;
		if (out) *out = ((p[0] & 0x01) << 30) + ((p[1] & 0x3f) << 24) +
			((p[2] & 0x3f) << 18) + ((p[3] & 0x3f) << 12) +
			((p[4] & 0x3f) << 6) + (p[5] & 0x3f);
		in += 6;
		return true;
	}
	return false;
}
//---------------------------------------------------------------------------
tjs_int TVPUtf8ToWideCharString(const char * in, tjs_char *out)
{
	// convert input utf-8 string to output wide string
	int count = 0;
	while (*in)
	{
		tjs_char c;
		if (out)
		{
			if (!TVPUtf8ToWideChar(in, &c))
				return -1; // invalid character found
			*out++ = c;
		}
		else
		{
			if (!TVPUtf8ToWideChar(in, NULL))
				return -1; // invalid character found
		}
		count++;
	}
	return count;
}

ttstr TVPStringFromBMPUnicode(const tjs_uint16 *src, tjs_int maxlen = -1)
{
	// convert to ttstr from BMP unicode
	if (sizeof(tjs_char) == 2)
	{
		// sizeof(tjs_char) is 2 (windows native)
		if (maxlen == -1)
			return ttstr((const tjs_char*)src);
		else
			return ttstr((const tjs_char*)src, maxlen);
	}
	else if (sizeof(tjs_char) == 4)
	{
		// sizeof(tjs_char) is 4 (UCS32)
		// FIXME: NOT TESTED CODE
		tjs_int len = 0;
		const tjs_uint16 *p = src;
		while (*p) len++, p++;
		if (maxlen != -1 && len > maxlen) len = maxlen;
		ttstr ret((tTJSStringBufferLength)(len));
		tjs_char *dest = ret.Independ();
		p = src;
		while (len && *p)
		{
			*dest = *p;
			dest++;
			p++;
			len--;
		}
		*dest = 0;
		ret.FixLen();
		return ret;
	}
	MessageBoxW(NULL, L"Unicode Text Must Be 2 or 4", L"Error", MB_OK);
	return ttstr(NULL);
}

static ttstr DefaultReadEncoding = TJS_W("UTF-8");

class tTVPTextReadStream : public iTJSTextReadStream
{
	tTJSBinaryStream * Stream;
	bool DirectLoad;
	tjs_char *Buffer;
	size_t BufferLen;
	tjs_char *BufferPtr;
	tjs_int CryptMode;

public:
	tTVPTextReadStream(const ttstr  & name, const ttstr & modestr, const ttstr &encoding)
	{
		// following syntax of modestr is currently supported.
		// oN: read from binary offset N (in bytes)

		Stream = NULL;
		Buffer = NULL;
		DirectLoad = false;
		CryptMode = -1;

		// check o mode
		Stream = TVPCreateStream(name, TJS_BS_READ);

		tjs_uint64 ofs = 0;
		const tjs_char * o_ofs;
		o_ofs = TJS_strchr(modestr.c_str(), TJS_W('o'));
		if (o_ofs != NULL)
		{
			// seek to offset
			o_ofs++;
			tjs_char buf[256];
			int i;
			for (i = 0; i < 255; i++)
			{
				if (o_ofs[i] >= TJS_W('0') && o_ofs[i] <= TJS_W('9'))
					buf[i] = o_ofs[i];
				else break;
			}
			buf[i] = 0;
			ofs = ttstr(buf).AsInteger();
			Stream->SetPosition(ofs);
		}

		// check first of the file - whether the file is unicode
		try
		{
			tjs_uint8 mark[2] = { 0, 0 };
			Stream->Read(mark, 2);
			if (mark[0] == 0xff && mark[1] == 0xfe)
			{
				// unicode
				DirectLoad = true;
			}
			else
			{
				// check UTF-8 BOM
				tjs_uint8 mark2[1] = { 0 };
				Stream->Read(mark2, 1);
				if (mark[0] == 0xef && mark[1] == 0xbb && mark2[0] == 0xbf) {
					// UTF-8 BOM
					tjs_uint size = (tjs_uint)(Stream->GetSize() - 3);
					tjs_uint8 *nbuf = new tjs_uint8[size + 1];
					try
					{
						Stream->ReadBuffer(nbuf, size);
						nbuf[size] = 0; // terminater
						BufferLen = TVPUtf8ToWideCharString((const char*)nbuf, NULL);
						if (BufferLen == (size_t)-1)
						{
							MessageBoxW(NULL, L"NarrowToWideConversionError", L"Error", MB_OK);
							return;
						}
						Buffer = new tjs_char[BufferLen + 1];
						TVPUtf8ToWideCharString((const char*)nbuf, Buffer);
					}
					catch (...)
					{
						delete[] nbuf;
						throw;
					}
					delete[] nbuf;
					Buffer[BufferLen] = 0;
					BufferPtr = Buffer;
				}
				else {
					// ansi/mbcs
					// read whole and hold it
					Stream->SetPosition(ofs);
					tjs_uint size = (tjs_uint)(Stream->GetSize());
					tjs_uint8 *nbuf = new tjs_uint8[size + 1];
					try
					{
						Stream->ReadBuffer(nbuf, size);
						nbuf[size] = 0; // terminater
						if (encoding == TJS_W("UTF-8")) {
							BufferLen = TVPUtf8ToWideCharString((const char*)nbuf, NULL);
							if (BufferLen == (size_t)-1)
							{
								MessageBoxW(NULL, L"NarrowToWideConversionError - UTF-8", L"Error", MB_OK);
							}
							Buffer = new tjs_char[BufferLen + 1];
							TVPUtf8ToWideCharString((const char*)nbuf, Buffer);
						}
						else if (encoding == TJS_W("Shift_JIS")) {
							BufferLen = TJS_narrowtowidelen((tjs_nchar*)nbuf);
							if (BufferLen == (size_t)-1)
							{
								MessageBoxW(NULL, L"NarrowToWideConversionError - SJIS", L"Error", MB_OK);
							}
							Buffer = new tjs_char[BufferLen + 1];
							TJS_narrowtowide(Buffer, (tjs_nchar*)nbuf, BufferLen);
						}
						else {
							MessageBoxW(NULL, L"Unsupported encoding", L"Error", MB_OK);
						}
					}
					catch (...)
					{
						delete[] nbuf;
						throw;
					}
					delete[] nbuf;
					Buffer[BufferLen] = 0;
					BufferPtr = Buffer;
				}
			}
		}
		catch (...)
		{
			delete Stream; Stream = NULL;
			throw;
		}
	}


	~tTVPTextReadStream()
	{
		if (Stream) delete Stream;
		if (Buffer) delete[] Buffer;
	}

	tjs_uint TJS_INTF_METHOD Read(tTJSString & targ, tjs_uint size)
	{
		if (DirectLoad)
		{
			if (sizeof(tjs_char) == 2)
			{
				if (size == 0) size = static_cast<tjs_uint>(Stream->GetSize() - Stream->GetPosition());
				if (!size)
				{
					targ.Clear();
					return 0;
				}
				tjs_char *buf = targ.AllocBuffer(size);
				tjs_uint read = Stream->Read(buf, size * 2); // 2 = BMP unicode size
				read /= 2;

				if (CryptMode == 0)
				{
					// simple crypt
					for (tjs_uint i = 0; i<read; i++)
					{
						tjs_char ch = buf[i];
						if (ch >= 0x20) buf[i] = ch ^ (((ch & 0xfe) << 8) ^ 1);
					}
				}
				else if (CryptMode == 1)
				{
					// simple crypt
					for (tjs_uint i = 0; i<read; i++)
					{
						tjs_char ch = buf[i];
						ch = ((ch & 0xaaaaaaaa) >> 1) | ((ch & 0x55555555) << 1);
						buf[i] = ch;
					}
				}
				buf[read] = 0;
				targ.FixLen();
				return read;
			}
			else
			{
				// sizeof(tjs_char) is 4
				// FIXME: NOT TESTED CODE
				if (size == 0) size = static_cast<tjs_uint>(Stream->GetSize() - Stream->GetPosition());
				tjs_uint16 *buf = new tjs_uint16[size / 2];
				tjs_uint read;
				try
				{
					read = Stream->Read(buf, size * 2); // 2 = BMP unicode size
					read /= 2;

					if (CryptMode == 0)
					{
						// simple crypt (buggy version)
						for (tjs_uint i = 0; i<read; i++)
						{
							tjs_char ch = buf[i];
							if (ch >= 0x20) buf[i] = ch ^ (((ch & 0xfe) << 8) ^ 1);
						}
					}
					else if (CryptMode == 1)
					{
						// simple crypt
						for (tjs_uint i = 0; i<read; i++)
						{
							tjs_char ch = buf[i];
							ch = ((ch & 0xaaaaaaaa) >> 1) | ((ch & 0x55555555) << 1);
							buf[i] = ch;
						}
					}
					buf[read] = 0;
				}
				catch (...)
				{
					delete[] buf;
					throw;
				}
				targ = TVPStringFromBMPUnicode(buf);
				delete[] buf;
				return read;
			}
		}
		else
		{
			if (size == 0) size = BufferLen;
			if (size)
			{
				tjs_char *buf = targ.AllocBuffer(size);
				TJS_strncpy(buf, BufferPtr, size);
				buf[size] = 0;
				BufferPtr += size;
				BufferLen -= size;
				targ.FixLen();
			}
			else
			{
				targ.Clear();
			}
			return size;
		}
	}

	void TJS_INTF_METHOD Destruct() { delete this; }

};

void TVPSetDefaultReadEncoding(const ttstr& encoding)
{
	DefaultReadEncoding = encoding;
}


const tjs_char* TVPGetDefaultReadEncoding()
{
	return DefaultReadEncoding.c_str();
}


iTJSTextReadStream * TVPCreateTextStreamForRead(const ttstr & name,
	const ttstr & modestr)
{
	return new tTVPTextReadStream(name, modestr, DefaultReadEncoding);
}
//---------------------------------------------------------------------------
iTJSTextReadStream * TVPCreateTextStreamForReadByEncoding(const ttstr & name,
	const ttstr & modestr, const ttstr & encoding)
{
	return new tTVPTextReadStream(name, modestr, encoding);
}
