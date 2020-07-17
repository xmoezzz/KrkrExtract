#ifndef _PackerInfo_
#define _PackerInfo_

#include "tp_stub.h"
#include <Windows.h>
#include <string>
#include <vector>
#include <Shlobj.h>
#include "CRC64.h"
#include "WinFile.h"
#include "XP3Info.h"
#include "XP3Offset.h"
#include "GenMD5Code.h"
#include "Sha1.h"
#include "Console.h"
#include "zlib.h"
#include "DecryptionCode.h"
#include "GlobalInit.h"
#include "Sha1.h"
#include "resource.h"

using std::wstring;
using std::vector;


#define TempPackage L"TempPackage.xp3"

#ifndef MagicLength
#define MagicLength 4
#endif

class PackInfo
{
public:

	enum {KrkrZ, KrkrZ_V2, NormalPack, UnknownPack/*Error Type*/};

	PackInfo(HWND _WinMain, const WCHAR* WinText);
	~PackInfo();

	HRESULT WINAPI DoPack(const WCHAR* BasePack, const WCHAR* GuessPackage, const WCHAR* OutName = L"patch_out.xp3");
	HRESULT WINAPI ForceInit(HWND _WinMain, const WCHAR* WinText);
	HRESULT WINAPI Reset();

//private:
	/*In File : XP3 File or Krkr's executable file*/
	HRESULT WINAPI DetactPackFormat(const WCHAR* lpFileName);
	ULONG   WINAPI InitIndex(PBYTE pDecompress, ULONG Size);
	ULONG   WINAPI InitIndexForM2(PBYTE pDecompress, ULONG Size);
	HRESULT WINAPI GetM2SingleEncryptionCode(const WCHAR* lpFileName);

	HRESULT WINAPI DoNormalPack(
		const WCHAR* lpBasePack,
		const WCHAR* lpGuessPack,
		const WCHAR* OutName);

	HRESULT WINAPI DoM2Pack(
		const WCHAR* lpBasePack,
		const WCHAR* GuessPackage,
		const WCHAR* OutName,
		ULONG PackType);

	//通过建立Dummy Package进行封包
	//Todo:使用remove删去对pack的引用，以便及时删除
	
	//Version1 
	HRESULT WINAPI DoM2PackV2(
		const WCHAR* lpBasePack,
		const WCHAR* GuessPackage,
		const WCHAR* OutName,
		ULONG PackType);

	HRESULT WINAPI DoM2PackV2_Version2(
		const WCHAR* lpBasePack,
		const WCHAR* GuessPackage,
		const WCHAR* OutName,
		ULONG PackType);

	HRESULT WINAPI DoM2DummyPackFirst(
		const WCHAR* lpBasePack);

	//Since nekopara vol2
	HRESULT WINAPI DoM2DummyPackFirst_Version2(
		const WCHAR* lpBasePack);

	HRESULT WINAPI IsM2SingleDecryption(
		BYTE* pOriginalBuffer,
		ULONG OriginalSize,
		BYTE* pDecodeBuffer,
		ULONG DecodeSize);

	HRESULT WINAPI IterFiles(wstring srcPath);

	VOID    WINAPI DecryptWorkerM2(
		ULONG EncryptOffset,
		PBYTE pBuffer,
		ULONG BufferSize,
		ULONG Hash, BYTE Key);

	VOID WINAPI DecryptWorker(
		ULONG EncryptOffset, 
		PBYTE pBuffer, 
		ULONG BufferSize, 
		ULONG Hash);

	//Since nekopara vol2
	//separately special sub-chunk
	BOOL WINAPI IsCompatXP3(
		PBYTE Data, 
		ULONG Size, 
		PBYTE pMagic = nullptr);

	VOID    WINAPI SetProcess(ULONG iPos);
	HRESULT WINAPI DisableAll(HWND hWnd);
	HRESULT WINAPI EnableAll(HWND hWnd);

//private:
	ULONG               PackInformation;
	vector<wstring>     FileList;
	ULONG M2Hash;
	BYTE  DecryptionKey;
	UCHAR M2ChunkMagic[4];
	tTVPXP3ArchiveExtractionFilter pfProc;
	BOOL  XP3EncryptionFlag;

	PBYTE pIndexBuffer;
	vector<XP3Index> M2ChunkList;

	vector<SMyXP3IndexM2> PackChunkList;
	HWND   MainWindow;
	HANDLE hThread;
	DWORD  ThreadId;

	wstring mBasePack;
	wstring mGuessPackage;
	wstring mOutName;

	wstring mWinText;

	BOOL InfoNameZeroEnd;
	BOOL M2NameZeroEnd;
};


#endif
