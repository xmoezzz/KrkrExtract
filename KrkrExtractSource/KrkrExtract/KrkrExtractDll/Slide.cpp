//---------------------------------------------------------------------------

#include "Slide.h"
//---------------------------------------------------------------------------
SlideCompressor::SlideCompressor()
{
	S = 0;
	for (int i = 0; i < SLIDE_N + SLIDE_M; i++) Text[i] = 0;
	for (int i = 0; i < 256 * 256; i++)
		Map[i] = -1;
	for (int i = 0; i < SLIDE_N; i++)
		Chains[i].Prev = Chains[i].Next = -1;
	for (int i = SLIDE_N - 1; i >= 0; i--)
		AddMap(i);
}
//---------------------------------------------------------------------------
SlideCompressor::~SlideCompressor()
{
}
//---------------------------------------------------------------------------
int SlideCompressor::GetMatch(const unsigned char*cur, int curlen, int &pos, int s)
{
	// get match length
	if (curlen < 3) return 0;

	int place = cur[0] + ((int)cur[1] << 8);

	int maxlen = 0;
	if ((place = Map[place]) != -1)
	{
		int place_org;
		curlen -= 1;
		do
		{
			place_org = place;
			if (s == place || s == ((place + 1) & (SLIDE_N - 1))) continue;
			place += 2;
			int lim = (SLIDE_M < curlen ? SLIDE_M : curlen) + place_org;
			const unsigned char *c = cur + 2;
			if (lim >= SLIDE_N)
			{
				if (place_org <= s && s < SLIDE_N)
					lim = s;
				else if (s < (lim&(SLIDE_N - 1)))
					lim = s + SLIDE_N;
			}
			else
			{
				if (place_org <= s && s < lim)
					lim = s;
			}
			while (Text[place] == *(c++) && place < lim) place++;
			int matchlen = place - place_org;
			if (matchlen > maxlen) pos = place_org, maxlen = matchlen;
			if (matchlen == SLIDE_M) return maxlen;

		} while ((place = Chains[place_org].Next) != -1);
	}
	return maxlen;
}
//---------------------------------------------------------------------------
void SlideCompressor::AddMap(int p)
{
	int place = Text[p] + ((int)Text[(p + 1) & (SLIDE_N - 1)] << 8);

	if (Map[place] == -1)
	{
		// first insertion
		Map[place] = p;
	}
	else
	{
		// not first insertion
		int old = Map[place];
		Map[place] = p;
		Chains[old].Prev = p;
		Chains[p].Next = old;
		Chains[p].Prev = -1;
	}
}
//---------------------------------------------------------------------------
void SlideCompressor::DeleteMap(int p)
{
	int n;
	if ((n = Chains[p].Next) != -1)
		Chains[n].Prev = Chains[p].Prev;

	if ((n = Chains[p].Prev) != -1)
	{
		Chains[n].Next = Chains[p].Next;
	}
	else if (Chains[p].Next != -1)
	{
		int place = Text[p] + ((int)Text[(p + 1) & (SLIDE_N - 1)] << 8);
		Map[place] = Chains[p].Next;
	}
	else
	{
		int place = Text[p] + ((int)Text[(p + 1) & (SLIDE_N - 1)] << 8);
		Map[place] = -1;
	}

	Chains[p].Prev = -1;
	Chains[p].Next = -1;
}
//---------------------------------------------------------------------------
void SlideCompressor::Encode(const unsigned char *in, long inlen,
	unsigned char *out, long & outlen)
{
	unsigned char code[40], codeptr, mask;

	if (inlen == 0) return;

	outlen = 0;
	code[0] = 0;
	codeptr = mask = 1;

	int s = S;
	while (inlen > 0)
	{
		int pos = 0;
		int len = GetMatch(in, inlen, pos, s);
		if (len >= 3)
		{
			code[0] |= mask;
			if (len >= 18)
			{
				code[codeptr++] = pos & 0xff;
				code[codeptr++] = ((pos & 0xf00) >> 8) | 0xf0;
				code[codeptr++] = len - 18;
			}
			else
			{
				code[codeptr++] = pos & 0xff;
				code[codeptr++] = ((pos & 0xf00) >> 8) | ((len - 3) << 4);
			}
			while (len--)
			{
				unsigned char c = 0[in++];
				DeleteMap((s - 1) & (SLIDE_N - 1));
				DeleteMap(s);
				if (s < SLIDE_M - 1) Text[s + SLIDE_N] = c;
				Text[s] = c;
				AddMap((s - 1) & (SLIDE_N - 1));
				AddMap(s);
				s++;
				inlen--;
				s &= (SLIDE_N - 1);
			}
		}
		else
		{
			unsigned char c = 0[in++];
			DeleteMap((s - 1) & (SLIDE_N - 1));
			DeleteMap(s);
			if (s < SLIDE_M - 1) Text[s + SLIDE_N] = c;
			Text[s] = c;
			AddMap((s - 1) & (SLIDE_N - 1));
			AddMap(s);
			s++;
			inlen--;
			s &= (SLIDE_N - 1);
			code[codeptr++] = c;
		}
		mask <<= 1;

		if (mask == 0)
		{
			for (int i = 0; i < codeptr; i++)
				out[outlen++] = code[i];
			mask = codeptr = 1;
			code[0] = 0;
		}
	}

	if (mask != 1)
	{
		for (int i = 0; i < codeptr; i++)
			out[outlen++] = code[i];
	}

	S = s;
}
//---------------------------------------------------------------------------
void SlideCompressor::Store()
{
	S2 = S;
	int i;
	for (i = 0; i < SLIDE_N + SLIDE_M - 1; i++)
		Text2[i] = Text[i];

	for (i = 0; i < 256 * 256; i++)
		Map2[i] = Map[i];

	for (i = 0; i < SLIDE_N; i++)
		Chains2[i] = Chains[i];
}
//---------------------------------------------------------------------------
void SlideCompressor::Restore()
{
	S = S2;
	int i;
	for (i = 0; i < SLIDE_N + SLIDE_M - 1; i++)
		Text[i] = Text2[i];

	for (i = 0; i < 256 * 256; i++)
		Map[i] = Map2[i];

	for (i = 0; i < SLIDE_N; i++)
		Chains[i] = Chains2[i];
}
//---------------------------------------------------------------------------

