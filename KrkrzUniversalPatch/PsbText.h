#ifndef _PSB_TEXT_
#define _PSB_TEXT_

#include <Windows.h>
#include <vector>
#include <algorithm>
#include <string>
#include "TextPlugin.h"
//#include "PsbTextHeader.h"
EXTERN_C
{
#include "zlib.h"
}
#include "Shlobj.h"

using std::wstring;

#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "zlib.lib")
#pragma comment(lib, "MyLibrary_x86_static.lib")

#define		PSBVALTYPE_ARRAY	0x20
#define		PSBVALTYPE_DICT		0x21

struct PsbHeader
{
	DWORD	dwMagic;
	DWORD	nVersion;
	DWORD	unk;
	DWORD	nNameTree;
	DWORD	nStrOffList;
	DWORD	nStrRes;
	DWORD	nDibOffList;
	DWORD	nDibSizeList;
	DWORD	nDibRes;
	DWORD	nResIndexTree;
};

struct TreeNode
{
	int nBranch;
	std::vector<int> pSub;
};

struct PsbInfo
{
	BOOL	bIsCompressed;
	DWORD*	lpStrOffList;
	int		nStrs;
	char*	lpStrRes;
	int		nTotalStrLen;
	int		nOriTotalStrLen;
	DWORD*	lpTree;
	DWORD*	lpVerifyTree;
	int		nTreeSize;
	//char**	lpNamesTable;
	int		nNames;
};

int compare1(char* arg1, char* arg2);
int compare2(WORD* arg1, WORD* arg2);
int compare3(char* arg1, char* arg2);
int compare4(DWORD* arg1, DWORD* arg2);

int WINAPI ExtractPsbText(IStream* Stream, PBYTE& OutBuffer, ULONG& OutSize, LPCWSTR FileName);

#endif

