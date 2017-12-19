#include <Windows.h>
#include <stdio.h>
#include <string>
#include <vector>


using std::wstring;
using std::vector;


const WCHAR MY_SEPARATOR = L'?';

typedef struct FileInfoChunk
{
	wstring FileName;
	ULONG   Offset;
	ULONG64 Hash;
	ULONG   Size;

	FileInfoChunk& operator = (const FileInfoChunk& o)
	{
		FileName = o.FileName;
		Offset = o.Offset;
		Hash = o.Hash;
		Size = o.Size;
		return *this;
	}
}FileInfoChunk;

unsigned long long MurmurHash64B(const void * key, int len, unsigned int seed = 0xEE6B27EB)
{
	const unsigned int m = 0x5bd1e995;
	const int r = 24;

	unsigned int h1 = seed ^ len;
	unsigned int h2 = 0;

	const unsigned int * data = (const unsigned int *)key;

	while (len >= 8)
	{
		unsigned int k1 = *data++;
		k1 *= m; k1 ^= k1 >> r; k1 *= m;
		h1 *= m; h1 ^= k1;
		len -= 4;

		unsigned int k2 = *data++;
		k2 *= m; k2 ^= k2 >> r; k2 *= m;
		h2 *= m; h2 ^= k2;
		len -= 4;
	}

	if (len >= 4)
	{
		unsigned int k1 = *data++;
		k1 *= m; k1 ^= k1 >> r; k1 *= m;
		h1 *= m; h1 ^= k1;
		len -= 4;
	}

	switch (len)
	{
	case 3: h2 ^= ((unsigned char*)data)[2] << 16;
	case 2: h2 ^= ((unsigned char*)data)[1] << 8;
	case 1: h2 ^= ((unsigned char*)data)[0];
		h2 *= m;
	};

	h1 ^= h2 >> 18; h1 *= m;
	h2 ^= h1 >> 22; h2 *= m;
	h1 ^= h2 >> 17; h1 *= m;
	h2 ^= h1 >> 19; h2 *= m;

	unsigned long long h = h1;

	h = (h << 32) | h2;

	return h;
}


wstring FileNameToLower(LPCWSTR FileName)
{
	wstring Result;

	for (ULONG i = 0; i < lstrlenW(FileName); i++)
	{
		if (FileName[i] <= 'Z' && FileName[i] >= 'A')
			Result += towlower(FileName[i]);
		else
			Result += FileName[i];
	}
	return Result;
}


vector<wstring>  FileList;

void TravelDir(std::wstring& Path)
{
	WIN32_FIND_DATAW FindData;
	HANDLE           Handle;
	std::wstring     CurrentPath, FullPath;

	CurrentPath = Path + L"\\*.*";
	Handle = FindFirstFileW(CurrentPath.c_str(), &FindData);
	if (Handle == INVALID_HANDLE_VALUE)
		return;
	
	while (FindNextFileW(Handle, &FindData))
	{
		if (lstrcmpW(FindData.cFileName, L".") == 0 || 
		    lstrcmpW(FindData.cFileName, L"..") == 0)
		{
			continue;
		}

		FullPath = Path + L"\\";
		FullPath += FindData.cFileName;
		if (!(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			FileList.push_back(FullPath);
		}
		else
		{
			TravelDir(FullPath);
		}
	}
}

int wmain(int argc, WCHAR* argv[])
{
	DWORD                 Attributes, iPos;
	FILE*                 Writer;
	FILE*                 Reader;
	PBYTE                 Buffer;
	FileInfoChunk         Chunk;
	vector<FileInfoChunk> ChunkList;
	vector<BYTE>          ChunkData;

	if (argc != 2)
		return 0;

	Attributes = GetFileAttributesW(argv[1]);
	if (Attributes == 0xffffffff && !(Attributes & FILE_ATTRIBUTE_DIRECTORY))
		return 0;

	TravelDir(wstring(argv[1]));

	if (FileList.size() == 0)
		return 0;

	Writer = _wfopen(L"Game.bin", L"wb");
	iPos = 0;
	fwrite(&iPos, 1, 4, Writer);
	iPos += 4;

	for (auto& FileName : FileList)
	{
		wprintf(L"Packing : %s\n", FileName.c_str());
		Reader = _wfopen(FileName.c_str(), L"rb");
		if (!Reader)
			continue;

		fseek(Reader, 0, SEEK_END);
		Chunk.Size = ftell(Reader);
		rewind(Reader);

		Buffer = new BYTE[Chunk.Size];
		if (!Buffer)
		{
			fclose(Reader);
			continue;
		}

		Chunk.Offset = iPos;
		iPos += Chunk.Size;
		fread(Buffer, 1, Chunk.Size, Reader);

		fwrite(Buffer, 1, Chunk.Size, Writer);
		fclose(Reader);
		delete[] Buffer;

		Chunk.FileName = FileName;
		for (auto& CharItem : Chunk.FileName)
		{
			if (CharItem == L'\\' || CharItem == L'/')
				CharItem = MY_SEPARATOR;
		}

		Chunk.FileName = FileNameToLower(Chunk.FileName.c_str());
		Chunk.Hash = MurmurHash64B(Chunk.FileName.c_str(), Chunk.FileName.length() * 2);

		ChunkList.push_back(Chunk);
	}

	for (auto& it : ChunkList)
	{
		for (ULONG i = 0; i < it.FileName.length(); i++)
		{
			PBYTE CurFileName = (PBYTE)&(it.FileName[i]);
			ChunkData.push_back(CurFileName[0]);
			ChunkData.push_back(CurFileName[1]);
		}
		ChunkData.push_back(NULL);
		ChunkData.push_back(NULL);
		BYTE   Info[16];
		memcpy(Info, &it.Offset, 4);
		memcpy(Info + 4, &it.Size, 4);
		memcpy(Info + 8, &it.Hash, 8);

		for (ULONG i = 0; i < 16; i++)
		{
			ChunkData.push_back(Info[i]);
		}
	}
	fwrite(&ChunkData[0], 1, ChunkData.size(), Writer);
	fseek(Writer, 0, SEEK_SET);
	fwrite((PBYTE)&iPos, 1, 4, Writer);
	fclose(Writer);
	return 0;
}

