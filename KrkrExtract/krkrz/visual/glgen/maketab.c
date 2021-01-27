/*-----------------------------------------------------------------*/
unsigned char TVPDivTable[256*256];
unsigned char TVPOpacityOnOpacityTable[256*256];
unsigned char TVPNegativeMulTable[256*256];
/* following two are for 65-level anti-aliased letter drawing */
/* ( TVPApplyColorMap65_d and TVPApplyColorMap65_do ) */
unsigned char TVPOpacityOnOpacityTable65[65*256];
unsigned char TVPNegativeMulTable65[65*256];
unsigned char TVPDitherTable_5_6[8][4][2][256];
unsigned char TVPDitherTable_676[3][4][4][256];
unsigned char TVP252DitherPalette[3][256];
tjs_uint32 TVPRecipTable256[256]; /* 1/x  table  ( 65536 ) multiplied */
tjs_uint16 TVPRecipTable256_16[256]; /* 1/x  table  ( 65536 ) multiplied,
	but limitted to 32767 (signed 16bits) */
static const tjs_uint8 TVPDither4x4[4][4] = {
 {   0, 12,  2, 14   },
 {   8,  4, 10,  6   },
 {   3, 15,  1, 13   },
 {  11,  7,  9,  5   }};

#define TVP_TLG6_GOLOMB_HALF_THRESHOLD 8


#define TVP_TLG6_GOLOMB_N_COUNT  4
#define TVP_TLG6_LeadingZeroTable_BITS 12
#define TVP_TLG6_LeadingZeroTable_SIZE  (1<<TVP_TLG6_LeadingZeroTable_BITS)
tjs_uint8 TVPTLG6LeadingZeroTable[TVP_TLG6_LeadingZeroTable_SIZE];
short int TVPTLG6GolombCompressed[TVP_TLG6_GOLOMB_N_COUNT][9] = {
		{3,7,15,27,63,108,223,448,130,},
		{3,5,13,24,51,95,192,384,257,},
		{2,5,12,21,39,86,155,320,384,},
		{2,3,9,18,33,61,129,258,511,},
	/* Tuned by W.Dee, 2004/03/25 */
};
char TVPTLG6GolombBitLengthTable
	[TVP_TLG6_GOLOMB_N_COUNT*2*128][TVP_TLG6_GOLOMB_N_COUNT] =
	{ { 0 } };


static void TVPPsMakeTable(void);

static void TVPTLG6InitLeadingZeroTable(void)
{
	/* table which indicates first set bit position + 1. */
	/* this may be replaced by BSF (IA32 instrcution). */

	int i;
	for(i = 0; i < TVP_TLG6_LeadingZeroTable_SIZE; i++)
	{
		int cnt = 0;
		int j;
		for(j = 1; j != TVP_TLG6_LeadingZeroTable_SIZE && !(i & j);
			j <<= 1, cnt++);
		cnt ++;
		if(j == TVP_TLG6_LeadingZeroTable_SIZE) cnt = 0;
		TVPTLG6LeadingZeroTable[i] = cnt;
	}
}

void TVPTLG6InitGolombTable(void)
{
	int n, i, j;
	for(n = 0; n < TVP_TLG6_GOLOMB_N_COUNT; n++)
	{
		int a = 0;
		for(i = 0; i < 9; i++)
		{
			for(j = 0; j < TVPTLG6GolombCompressed[n][i]; j++)
				TVPTLG6GolombBitLengthTable[a++][n] = (char)i;
		}
		if(a != TVP_TLG6_GOLOMB_N_COUNT*2*128)
			*(char*)0 = 0;   /* THIS MUST NOT BE EXECUETED! */
				/* (this is for compressed table data check) */
	}
}


static void TVPInitDitherTable(void)
{
	/* create an ordered dither table for conversion of 8bit->6bit and 8bit->5bit and */
	/* RGB ( 256*256*256 ) -> palettized 252 colors ( 6*7*6 ) */
	tjs_int j, i, r, g, b, c;

	for(j = 0; j < 4; j ++)
	{
		for(i = 0; i < 4; i ++)
		{
			double v1 = TVPDither4x4[j][i] / 16.0;
			double v2 = TVPDither4x4[((j+1)%2)][((i+1)%2)] / 16.0;
			double v3 = TVPDither4x4[j][((i+1)%2)] / 16.0;

			int n;

			for(n = 0; n < 256; n++)
			{
				double nt = n / 255.0;
				double frac;
				int main;

				/* for 5bit */
				main = (int)(nt * 31.0);
				frac = nt * 31.0 - (int)(nt * 31.0);
				TVPDitherTable_5_6[j][i][0][n] = main + ((v1 < frac)?1:0);
				TVPDitherTable_5_6[j+4][i][0][n] = TVPDitherTable_5_6[j][i][0][n];

				/* for 6bit */
				main = (int)(nt * 63.0);
				frac = nt * 63.0 - (int)(nt * 63.0);
				TVPDitherTable_5_6[j][i][1][n] = main + ((v2 < frac)?1:0);
				TVPDitherTable_5_6[j+4][i][1][n] = TVPDitherTable_5_6[j][i][1][n];

				/* 256 level -> 6 level R, B */
				main = (int)(nt * 5);
				frac = nt * 5 - (int)(nt * 5);
				TVPDitherTable_676[2][i][j][n] = (main + ((v1 < frac)?1:0)) * (6 * 7);
				TVPDitherTable_676[0][i][j][n] = (main + ((v2 < frac)?1:0));

				/* 256 level -> 7 level G */
				main = (int)(nt * 6);
				frac = nt * 6 - (int)(nt * 6);
				TVPDitherTable_676[1][i][j][n] = (main + ((v3 < frac)?1:0)) * (6);
			}
		}
	}

	/* create 256 colors dither palette table */
	/* ( 252 colors are used ) */
	c = 0;
	for(r = 0; r < 6; r++)
	{
		for(g = 0; g < 7; g++)
		{
			for(b = 0; b < 6; b++)
			{
				TVP252DitherPalette[0][c] = r * 255 / 5;
				TVP252DitherPalette[1][c] = g * 255 / 6;
				TVP252DitherPalette[2][c] = b * 255 / 5;
				c ++;
			}
		}
	}
	for(; c < 256; c++)
	{
		TVP252DitherPalette[0][c] =
		TVP252DitherPalette[1][c] =
		TVP252DitherPalette[2][c] = 0;
	}

	/* create TVPRecipTable256 */
	TVPRecipTable256[0] = 65536;
	TVPRecipTable256_16[0] = 0x7fff;
	for(i = 1; i < 256; i++)
	{
		TVPRecipTable256[i] = 65536/i;
		TVPRecipTable256_16[i] = TVPRecipTable256[i] > 0x7fff ?
									0x7fff : TVPRecipTable256[i];
	}
}


static void TVPCreateTable(void)
{
	int a,b;

	for(a=0; a<256; a++)
	{
		for(b=0; b<256; b++)
		{
			float c;
			int ci;
			int addr = b*256+ a;

			if(a)
			{
				float at = (float)(a/255.0), bt = (float)(b/255.0);
				c = bt / at;
				c /= (float)( (1.0 - bt + c) );
				ci = (int)(c*255);
				if(ci>=256) ci = 255; /* will not overflow... */
			}
			else
			{
				ci=255;
			}

			TVPOpacityOnOpacityTable[addr]=(unsigned char)ci;
				/* higher byte of the index is source opacity */
				/* lower byte of the index is destination opacity */
		
			TVPNegativeMulTable[addr] = (unsigned char)
				( 255 - (255-a)*(255-b)/ 255 ); 
		}
	}

	for(a=0; a<256; a++)
	{
		for(b=0; b<65; b++)
		{
			float c;
			int ci;
			int addr = b*256+ a;
			int bb;

			if(a)
			{
				float at = (float)(a / 255.0), bt = (float)(b / 64.0);
				c = bt / at;
				c /= (float)( (1.0 - bt + c) );
				ci = (int)(c*255);
				if(ci>=256) ci = 255; /* will not overflow... */
			}
			else
			{
				ci=255;
			}

			TVPOpacityOnOpacityTable65[addr]=(unsigned char)ci;
				/* higher byte of the index is source opacity */
				/* lower byte of the index is destination opacity */

			bb = b * 4;
			if(bb > 255) bb = 255;
			TVPNegativeMulTable65[addr] = (unsigned char)
				( 255 - (255-a)*(255-bb)/ 255 ); 
		}
	}

	for(b=0; b<256; b++)
	{
		TVPDivTable[(0<<8)+b] = 0;
		for(a=1; a<256; a++)
		{
			tjs_int tmp = (tjs_int)(b*255/a);
			if(tmp > 255) tmp = 255;
			TVPDivTable[(a<<8)+b] = (tjs_uint8)(tmp);
		}
	}


	TVPInitDitherTable();
	TVPTLG6InitLeadingZeroTable();
	TVPTLG6InitGolombTable();
	TVPPsMakeTable();
}

static void TVPDestroyTable(void)
{
	/* nothing to do ... */
}


