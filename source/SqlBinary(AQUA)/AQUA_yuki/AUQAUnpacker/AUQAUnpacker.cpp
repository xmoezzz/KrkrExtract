#include <Windows.h>
#include <WinFile.h>
#include <zlib.h>
#include <string>
#include <vector>
#include "CMem.h"
#include "Krkr2.h"
#include "Cxdec.h"

#pragma comment(lib, "zlib.lib")
#pragma comment(lib, "ntdll.lib")

using std::wstring;
using std::vector;

static const WCHAR* ProtectionInfo = L"$$$ This is a protected archive. $$$ 著作者はこのアーカイブが正規の利用方法以外の方法で展開されることを望んでいません。 $$$ This is a protected archive. $$$ 著作者はこのアーカイブが正規の利用方法以外の方法で展開されることを望んでいません。 $$$ This is a protected archive. $$$ 著作者はこのアーカイブが正規の利用方法以外の方法で展開されることを望んでいません。 $$$ Warning! Extracting this archive may infringe on author's rights. 警告 このアーカイブを展開することにより、あなたは著作者の権利を侵害するおそれがあります。.txt";


wstring GetPackageName(wstring& fileName)
{
	wstring temp(fileName);
	wstring::size_type pos = temp.find_last_of(L"\\");

	if (pos != wstring::npos)
	{
		temp = temp.substr(pos + 1, temp.length());
	}

	wstring temp2(temp);
	wstring::size_type pos2 = temp2.find_last_of(L"/");
	if (pos2 != wstring::npos)
	{
		temp2 = temp2.substr(pos + 1, temp2.length());
	}
	return temp2;
}

VOID WINAPI AddPath(const wchar_t* FileName)
{
	int iPos = 0;
	while (FileName[iPos] != NULL)
	{
		if (FileName[iPos] == L'/' || FileName[iPos] == L'\\')
		{
			wchar_t iPath[260] = { 0 };
			wchar_t  CurrPath[260] = { 0 };
			GetCurrentDirectoryW(260, CurrPath);
			lstrcpynW(iPath, FileName, iPos + 1);
			wstring CreateP(CurrPath);
			CreateP += L"\\";
			CreateP += iPath;
			CreateDirectoryW(CreateP.c_str(), NULL);
		}
		iPos++;
	}
}

std::vector<XP3Index> ItemVector;

#define PtrInc(x) PtrOffset += x
#define iPosAdd(x) iPos += x

BOOL WINAPI InitIndex(PBYTE pDecompress, ULONG Size)
{
	ULONG PtrOffset = 0;

	while (PtrOffset < Size)
	{
		BOOL     NotAdd = FALSE;
		XP3Index item;

		if (RtlCompareMemory(pDecompress + PtrOffset, CHUNK_MAGIC_YUZU, 4) == 4)
		{
			PtrOffset += 4;
			PtrOffset += 8;
			item.isM2Format = TRUE;
			ULONG HashInfo = 0;
			CopyMemory(&HashInfo, (pDecompress + PtrOffset), 4);
			item.yuzu.Hash = HashInfo;
			PtrOffset += 4;
			USHORT FileNameLen = 0;
			CopyMemory(&FileNameLen, (pDecompress + PtrOffset), 2);
			item.yuzu.Len = FileNameLen;
			PtrOffset += 2;
			wstring M2FileName((WCHAR*)(pDecompress + PtrOffset), FileNameLen);
			item.yuzu.Name = M2FileName;

			WCHAR ZeroEnd = *(WCHAR*)(pDecompress + PtrOffset + FileNameLen * 2);
			if (!ZeroEnd)
			{
				PtrOffset += (FileNameLen + 1) * 2;
			}
			else
			{
				PtrOffset += (FileNameLen)* 2;
			}

			if (!wcscmp(item.yuzu.Name.c_str(), ProtectionInfo))
			{
				NotAdd = TRUE;
			}
		}
		else
		{
			if (RtlCompareMemory(pDecompress + PtrOffset, CHUNK_MAGIC_KRKRZ_M2, 4) == 4 ||
				RtlCompareMemory(pDecompress + PtrOffset, CHUNK_MAGIC_KRKRZ_NEKO, 4) == 4 ||
				RtlCompareMemory(pDecompress + PtrOffset, CHUNK_MAGIC_KRKRZ_FENG, 4) == 4)
			{
				PtrOffset += 4;
				PtrOffset += 8;
				item.isM2Format = TRUE;
				ULONG HashInfo = 0;
				CopyMemory(&HashInfo, (pDecompress + PtrOffset), 4);
				item.yuzu.Hash = HashInfo;
				PtrOffset += 4;
				USHORT FileNameLen = 0;
				CopyMemory(&FileNameLen, (pDecompress + PtrOffset), 2);
				item.yuzu.Len = FileNameLen;
				PtrOffset += 2;
				wstring M2FileName((WCHAR*)(pDecompress + PtrOffset), FileNameLen);
				item.yuzu.Name = M2FileName;

				WCHAR ZeroEnd = *(WCHAR*)(pDecompress + PtrOffset + FileNameLen * 2);
				if (!ZeroEnd)
				{
					PtrOffset += (FileNameLen + 1) * 2;
				}
				else
				{
					PtrOffset += (FileNameLen)* 2;
				}

				if (!wcscmp(item.yuzu.Name.c_str(), ProtectionInfo))
				{
					NotAdd = TRUE;
				}
			}
			else
			{
				item.isM2Format = FALSE;
			}
		}


		if (RtlCompareMemory(pDecompress + PtrOffset, CHUNK_MAGIC_FILE, 4) != 4)
		{
			return S_FALSE;
		}
		//File Maigc
		PtrInc(4);
		ULONG64 FileChunkSize64 = 0;
		CopyMemory(&FileChunkSize64, (PtrOffset + pDecompress), 8);
		PtrInc(8);

		ULONG SavePtrOffset = PtrOffset;
		ULONG32 FileChunkSize32 = (ULONG32)FileChunkSize64;
		ULONG iPos = 0;

		while (iPos < FileChunkSize32)
		{
			if (RtlCompareMemory((PtrOffset + pDecompress), CHUNK_MAGIC_INFO, 4) == 4)
			{
				PtrInc(4);
				iPosAdd(4);
				ULONG64 InfoChunkSize = 0;
				RtlCopyMemory(&InfoChunkSize, (PtrOffset + pDecompress), sizeof(ULONG64));

				PtrInc(8);
				iPosAdd(8);
				CopyMemory(&(item.info.EncryptedFlag), (PtrOffset + pDecompress), 4);
				PtrInc(4);
				iPosAdd(4);
				CopyMemory(&(item.info.OriginalSize), (PtrOffset + pDecompress), 8);
				PtrInc(8);
				iPosAdd(8);
				CopyMemory(&(item.info.ArchiveSize), (PtrOffset + pDecompress), 8);
				PtrInc(8);
				iPosAdd(8);
				CopyMemory(&(item.info.FileNameLength), (PtrOffset + pDecompress), 2);
				PtrInc(2);
				iPosAdd(2);


				//某高考 的 支持
				WCHAR* FakeName = (WCHAR*)(PtrOffset + pDecompress);
				if ((item.info.FileNameLength > MAX_PATH && (ULONG)InfoChunkSize == 0x20 &&
					(FakeName[0] != L'$' && FakeName[1] != L'$' && FakeName[2] != L'$'))
					|| FALSE)
					//并不稳定，最好是对齐比较，自动获取到下一个Sign的长度
				{
					PtrOffset -= 2;
					iPos -= 2;
					item.info.FileNameLength = 0xFFFF;
					ULONG FakeNameSegm1, FakeNameSegm2, FakeNameSegm3;

					FakeNameSegm1 = *(PULONG)(PtrOffset + pDecompress);
					PtrInc(4);
					iPosAdd(4);
					FakeNameSegm2 = *(PULONG)(PtrOffset + pDecompress);
					PtrInc(4);
					iPosAdd(4);
					FakeNameSegm3 = *(PULONG)(PtrOffset + pDecompress);
					PtrInc(4);
					iPosAdd(4);

					WCHAR FakeNameStr[60] = { 0 };
					wsprintfW(FakeNameStr, L"%08X-%08X-%08X", FakeNameSegm1, FakeNameSegm2, FakeNameSegm3);
					item.info.FileName = FakeNameStr;
				}
				else
				{
					wstring WideFileName((WCHAR*)(PtrOffset + pDecompress), item.info.FileNameLength);
					WCHAR ZeroEnd = *(WCHAR*)(PtrOffset + pDecompress + item.info.FileNameLength * 2);
					if (!ZeroEnd)
					{
						PtrInc((item.info.FileNameLength + 1) * 2);
						iPosAdd((item.info.FileNameLength + 1) * 2);
					}
					else
					{
						PtrInc((item.info.FileNameLength) * 2);
						iPosAdd((item.info.FileNameLength) * 2);
					}

					item.info.FileName = WideFileName;
					if (WideFileName[0] == L'$' && WideFileName[1] == L'$' && WideFileName[2] == L'$')
					{
						//FileChunkSize32
						PtrOffset = SavePtrOffset;
						PtrOffset += FileChunkSize32;
						iPos = FileChunkSize32;
						NotAdd = TRUE;
					}
					else if (item.info.FileNameLength > MAX_PATH)
					{
						PtrOffset = SavePtrOffset;
						PtrOffset += FileChunkSize32;
						iPos = FileChunkSize32;
						NotAdd = TRUE;
					}

					if (item.info.OriginalSize.LowPart == 0xFFFFFFFF)
					{
						PtrOffset = SavePtrOffset;
						PtrOffset += FileChunkSize32;
						iPos = FileChunkSize32;
						NotAdd = TRUE;
					}

					if (!wcscmp(ProtectionInfo, WideFileName.c_str()))
					{
						NotAdd = TRUE;
					}
				}
			}
			else if (RtlCompareMemory((PtrOffset + pDecompress), CHUNK_MAGIC_SEGM, 4) == 4)
			{
				PtrInc(4);
				iPosAdd(4);
				ULONG64 TempChunkSize = 0;
				CopyMemory(&TempChunkSize, (PtrOffset + pDecompress), 8);
				item.segm.ChunkSize.QuadPart = TempChunkSize;
				PtrInc(8);
				iPosAdd(8);
				CopyMemory(&(item.segm.segm[0].bZlib), (PtrOffset + pDecompress), sizeof(BOOL));
				CopyMemory(&(item.segm.segm[0].Offset), (PtrOffset + pDecompress + sizeof(BOOL)), sizeof(ULONG64));
				CopyMemory(&(item.segm.segm[0].OriginalSize), (PtrOffset + pDecompress + sizeof(BOOL) + sizeof(ULONG64)), sizeof(ULONG64));
				CopyMemory(&(item.segm.segm[0].ArchiveSize), (PtrOffset + pDecompress + sizeof(BOOL) + sizeof(ULONG64) * 2), sizeof(ULONG64));
				PtrInc(TempChunkSize);
				iPosAdd(TempChunkSize);
			}
			else if (RtlCompareMemory((PtrOffset + pDecompress), CHUNK_MAGIC_ADLR, 4) == 4)
			{
				PtrInc(4);
				iPosAdd(4);

				ULONG64 TempChunkSize = 0;
				CopyMemory(&TempChunkSize, (PtrOffset + pDecompress), 8);
				PtrInc(8);
				iPosAdd(8);
				CopyMemory(&(item.adlr.Hash), (PtrOffset + pDecompress), 4);
				PtrInc(4);
				iPosAdd(4);
			}
			else if (RtlCompareMemory((PtrOffset + pDecompress), CHUNK_MAGIC_TIME, 4) == 4)
			{
				PtrInc(4);
				iPosAdd(4);
				ULONG64 TempChunkSize = 0;
				CopyMemory(&TempChunkSize, (PtrOffset + pDecompress), 8);

				PtrInc(8);
				iPosAdd(8);
				PtrInc(TempChunkSize);
				iPosAdd(TempChunkSize);
			}
			else
			{
				UCHAR UnknownMagic[4];
				RtlCopyMemory(UnknownMagic, (PtrOffset + pDecompress), 4);
				//Magic
				PtrInc(4);
				iPosAdd(4);
				ULONG64 TempChunkSize = 0;
				CopyMemory(&TempChunkSize, (PtrOffset + pDecompress), 8);

				//ChunkSize
				PtrInc(8);
				iPosAdd(8);
				PtrInc(TempChunkSize);
				iPosAdd(TempChunkSize);
			}
		}

		if (!NotAdd)
		{
			ItemVector.push_back(item);
		}
	}
	return TRUE;
}


HRESULT WINAPI ProcessXP3Archive(std::wstring xp3, HANDLE st)
{
	ULONG                   Count = 0;
	KRKR2_XP3_HEADER        XP3Header;
	KRKR2_XP3_DATA_HEADER   DataHeader;
	DWORD                   nRet = 0;
	ULONG                   IndexSectionSize = 0;
	HRESULT                 Status;
	LARGE_INTEGER           BeginOffset;
	WCHAR                   OutputInfo[MAX_PATH] = { 0 };

	BeginOffset.QuadPart = (ULONG64)0;

	if (ReadFile(st, &XP3Header, sizeof(XP3Header), &nRet, NULL) != TRUE)
	{
		return S_FALSE;
	}

	BeginOffset.QuadPart = 0;

	if (RtlCompareMemory(StaticXP3V2Magic, XP3Header.Magic, sizeof(StaticXP3V2Magic)) != sizeof(StaticXP3V2Magic))
	{
		return S_FALSE;
	}

	ULONG64 CompresseBufferSize = 0x1000;
	ULONG64 DecompressBufferSize = 0x1000;
	PBYTE pCompress = (PBYTE)HeapAlloc(GetProcessHeap(), 0, (ULONG)CompresseBufferSize);
	PBYTE pDecompress = (PBYTE)HeapAlloc(GetProcessHeap(), 0, (ULONG)DecompressBufferSize);
	DataHeader.OriginalSize = XP3Header.IndexOffset;


	BOOL Result = FALSE;
	do
	{
		LARGE_INTEGER Offset;

		Offset.QuadPart = DataHeader.OriginalSize.QuadPart + BeginOffset.QuadPart;
		SetFilePointerEx(st, Offset, NULL, FILE_BEGIN);
		if (!USER32_SUCCESS(ReadFile(st, &DataHeader, sizeof(DataHeader), &nRet, NULL)))
		{
			return S_FALSE;
		}

		if (DataHeader.ArchiveSize.HighPart != 0 || DataHeader.ArchiveSize.LowPart == 0)
		{
			continue;
		}

		if (DataHeader.ArchiveSize.LowPart > CompresseBufferSize)
		{
			CompresseBufferSize = DataHeader.ArchiveSize.LowPart;
			pCompress = (PBYTE)HeapReAlloc(GetProcessHeap(), 0, pCompress, (ULONG)CompresseBufferSize);
		}

		if ((DataHeader.bZlib & 7) == 0)
		{
			Offset.QuadPart = -8;
			SetFilePointerEx(st, Offset, NULL, FILE_CURRENT);
		}

		ReadFile(st, pCompress, DataHeader.ArchiveSize.LowPart, &nRet, NULL);

		BOOL EncodeMark = DataHeader.bZlib & 7;

		if (EncodeMark == FALSE)
		{
			if (DataHeader.ArchiveSize.LowPart > DecompressBufferSize)
			{
				DecompressBufferSize = DataHeader.ArchiveSize.LowPart;
				pDecompress = (PBYTE)HeapReAlloc(GetProcessHeap(), 0, pDecompress, (ULONG)DecompressBufferSize);
			}
			CopyMemory(pDecompress, pCompress, DataHeader.ArchiveSize.LowPart);
			DataHeader.OriginalSize.LowPart = DataHeader.ArchiveSize.LowPart;
		}
		else if (EncodeMark == TRUE)
		{
			if (DataHeader.OriginalSize.LowPart > DecompressBufferSize)
			{
				DecompressBufferSize = DataHeader.OriginalSize.LowPart;
				pDecompress = (PBYTE)HeapReAlloc(GetProcessHeap(), 0, pDecompress, (ULONG)DecompressBufferSize);
			}

			DataHeader.OriginalSize.HighPart = DataHeader.OriginalSize.LowPart;
			if (uncompress((PBYTE)pDecompress, (PULONG)&DataHeader.OriginalSize.HighPart,
				(PBYTE)pCompress, DataHeader.ArchiveSize.LowPart) == Z_OK)
			{
				DataHeader.OriginalSize.LowPart = DataHeader.OriginalSize.HighPart;
			}
		}
		else
		{
			DataHeader.bZlib = 0;
			return S_FALSE;
		}


		Result = InitIndex(pDecompress, DataHeader.OriginalSize.LowPart);

		if (Result)
			break;

	} while (DataHeader.bZlib & 0x80);

	HeapFree(GetProcessHeap(), 0, pCompress);
	HeapFree(GetProcessHeap(), 0, pDecompress);

	return S_OK;
}


static cxdec_callback AQUACallBack =
{
	"AQUA-X'moe",
	{ 0x1A8, 0x776 },
	xcode_building_stage1
};


VOID DecryptWorker(ULONG EncryptOffset, PBYTE pBuffer, ULONG BufferSize, ULONG Hash)
{
	cxdec_decode(&AQUACallBack, Hash, EncryptOffset, pBuffer, BufferSize);
}

void Extract(const WCHAR* wFileName)
{
	WinFile FakeWorker;
	HRESULT FakeWorkerFlag = FakeWorker.Open(wFileName, WinFile::FileRead);
	for (auto it : ItemVector)
	{
		ULONG os = 0;
		wstring outFilePath = L"outPath\\";
		outFilePath += GetPackageName(wstring(wFileName));
		outFilePath += L"\\";
		outFilePath += it.info.FileName.c_str();
		AddPath(outFilePath.c_str());
		HANDLE hFile = CreateFileW(outFilePath.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

		if (it.info.EncryptedFlag & 7)
		{
			PBYTE OutBuffer = nullptr, WriteBuffer = nullptr;
			OutBuffer = (PBYTE)CMem::Alloc(it.info.ArchiveSize.LowPart);
			WriteBuffer = (PBYTE)CMem::Alloc(it.info.OriginalSize.LowPart);
			FakeWorker.Seek(it.segm.segm[0].Offset.LowPart, FILE_BEGIN);
			FakeWorker.Read(OutBuffer, it.info.ArchiveSize.LowPart);
			ULONG OriSize = it.info.OriginalSize.LowPart, ArcSize = it.info.ArchiveSize.LowPart;
			uncompress(WriteBuffer, &OriSize, OutBuffer, ArcSize);

			DecryptWorker(0, WriteBuffer, it.info.ArchiveSize.LowPart, it.adlr.Hash);
			WriteFile(hFile, WriteBuffer, it.info.OriginalSize.LowPart, &os, NULL);

			CMem::Free(WriteBuffer);
			CMem::Free(OutBuffer);
		}
		else
		{
			PBYTE WriteBuffer = nullptr;
			WriteBuffer = (PBYTE)CMem::Alloc(it.info.OriginalSize.LowPart);
			FakeWorker.Seek(it.segm.segm[0].Offset.LowPart, FILE_BEGIN);
			FakeWorker.Read(WriteBuffer, it.info.OriginalSize.LowPart);

			DecryptWorker(0, WriteBuffer, it.info.ArchiveSize.LowPart, it.adlr.Hash);
			WriteFile(hFile, WriteBuffer, it.info.OriginalSize.LowPart, &os, NULL);

			CMem::Free(WriteBuffer);
		}
		CloseHandle(hFile);
	}
	FakeWorker.Release();
}



int wmain(int argc, WCHAR* argv[])
{
	if (argc != 2)
		return 0;

	//cxdec_init();

	HANDLE hFile = CreateFileW(argv[1], GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	
	ProcessXP3Archive(wstring(argv[1]), hFile);
	Extract(GetPackageName(wstring(argv[1])).c_str());
	CloseHandle(hFile);
	return 0;
}

