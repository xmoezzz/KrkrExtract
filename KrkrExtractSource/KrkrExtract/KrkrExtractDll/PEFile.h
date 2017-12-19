#pragma once

//==============================================================================
#include <windows.h>
//==============================================================================
#define MAX_SECTION_COUNT		64
#define SECTION_IMPORT			".Xmoe\0\0\0"
#define SECTION_RESERV			"@.reserv"
//==============================================================================
#pragma pack(1)
struct PE_DOS_HEADER {
	WORD   Signature;
	WORD   LastPageBytes;
	WORD   NumberOfPages;
	WORD   Relocations;
	WORD   HeaderSize;
	WORD   MinMemory;
	WORD   MaxMemory;
	WORD   InitialSS;
	WORD   InitialSP;
	WORD   Checksum;
	WORD   InitialIP;
	WORD   InitialCS;
	WORD   RelocTableOffset;
	WORD   Overlay;
	WORD   Reserved1[4];
	WORD   OemId;
	WORD   OemInfo;
	WORD   Reserved2[10];
	LONG   PEHeaderOffset;
};
struct PE_DOS_STUB {
	PBYTE   RawData;
	DWORD   Size;
};
struct PE_SECTION_DATA {
	DWORD   Offset;
	PBYTE   RawData;
	DWORD   Size;
};
struct PE_IMPORT_FUNCTION {
	PCHAR				FunctionName;
	int					FunctionId;
	PE_IMPORT_FUNCTION*	Next;
};
struct PE_IMPORT_DLL {
	char*				DllName;
	PE_IMPORT_FUNCTION* Functions;
	PE_IMPORT_DLL*	Next;
};

#pragma pack()
//==============================================================================
typedef IMAGE_NT_HEADERS PE_NT_HEADERS;
typedef IMAGE_SECTION_HEADER PE_SECTION_HEADER;
//==============================================================================
class PEFile {
public:
	PE_DOS_HEADER		dosHeader;
	PE_DOS_STUB			dosStub;
	PE_NT_HEADERS		peHeaders;
	PE_SECTION_HEADER	sectionTable[MAX_SECTION_COUNT];
	PE_SECTION_DATA		reservedData;
	PE_SECTION_DATA		sections[MAX_SECTION_COUNT];
	PE_IMPORT_DLL		importTable;
	PE_IMPORT_DLL		newImports;

	PEFile();
	PEFile(LPCWSTR filePath);
	~PEFile();
	bool				loadFromFile(LPCWSTR filePath);
	bool				loadFromMemory(PBYTE memoryAddress);
	bool				saveToFile(LPCWSTR filePath);
	int					addSection(char* name, DWORD size, bool isExecutable);
	void				addImport(char* dllName, char** functions, int functionCount);
	void				commit();

private:
	PBYTE				peMemory;

	void				init();
	bool				readFileData(LPCWSTR filePath);
	bool				checkValidity();
	bool				readHeaders();
	bool				readBody();
	bool				readImportTable();
	bool				writePadding(HANDLE fileHandle, long paddingSize);
	void				unloadFile();

	void				buildImportTable();
	char*				buildNewImports(DWORD baseRVA);
	DWORD				calcNewImportsSize(DWORD &sizeDlls, DWORD &sizeFunctions, DWORD &sizeStrings);

	DWORD				alignNumber(DWORD number, DWORD alignment);
	DWORD				rvaToOffset(DWORD rva);
	DWORD				offsetToRVA(DWORD offset);

	void				fixReservedData();
	void				fixHeaders();
	void				fixSectionTable();

};
