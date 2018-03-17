#pragma once

#include "my.h"
#include "tp_stub.h"
#include <Shlobj.h>
#include "MyLib.h"
#include "zlib.h"
#include "KrkrExtract.h"
#include "resource.h"


#ifndef MagicLength
#define MagicLength 4
#endif

class KrkrPacker
{
public:

	KrkrPacker();
	~KrkrPacker();

	Void   NTAPI Init();
	Void   NTAPI InternalReset();


	NTSTATUS NTAPI DetactPackFormat(LPCWSTR lpFileName);

	NTSTATUS NTAPI DoNormalPack(LPCWSTR lpBasePack,LPCWSTR lpGuessPack, LPCWSTR OutName);

	//魔女的夜宴
	NTSTATUS NTAPI DoM2Pack(LPCWSTR lpBasePack, LPCWSTR GuessPackage, LPCWSTR OutName);
	//Nekopara VOL2
	NTSTATUS NTAPI DoM2Pack_Version2(LPCWSTR lpBasePack, LPCWSTR GuessPackage, LPCWSTR OutName);
	//SenrenBanka
	NTSTATUS NTAPI DoM2Pack_SenrenBanka(LPCWSTR lpBasePack, LPCWSTR GuessPackage, LPCWSTR OutName);
	//生命のスペア
	NTSTATUS NTAPI DoNormalPackEx(LPCWSTR lpBasePack, LPCWSTR GuessPackage, LPCWSTR OutName);
	//魔女
	NTSTATUS NTAPI DoM2DummyPackFirst(LPCWSTR lpBasePack);
	//Since nekopara vol2
	NTSTATUS NTAPI DoM2DummyPackFirst_Version2(LPCWSTR lpBasePack);
	//SenrenBanka
	NTSTATUS NTAPI DoM2DummyPackFirst_SenrenBanka(LPCWSTR lpBasePack);
	//生命のスペア
	NTSTATUS NTAPI DoDummyNormalPackExFirst(LPCWSTR lpBasePack);

	NTSTATUS NTAPI GetSenrenBankaPackInfo(PBYTE IndexData, ULONG IndexSize, NtFileDisk& file);

	NTSTATUS NTAPI IterFiles(LPCWSTR lpPath);

	Void NTAPI DecryptWorker(ULONG EncryptOffset, PBYTE pBuffer, ULONG BufferSize,  ULONG Hash);

	ULONG                             KrkrPackType;
	vector<wstring>                   FileList;
	ULONG                             M2Hash;
	BYTE                              DecryptionKey;
	tTVPXP3ArchiveExtractionFilter    pfProc;
	BOOL                              XP3EncryptionFlag;
	vector<XP3Index>                  M2ChunkList;
	vector<SMyXP3IndexM2>             PackChunkList;
	BOOL                              InfoNameZeroEnd;
	BOOL                              M2NameZeroEnd;
	KRKRZ_M2_Senrenbanka_HEADER       SenrenBankaInfo;
	DWORD                             M2SubChunkMagic;

	HANDLE                            hThread;
	DWORD                             ThreadId;
};



