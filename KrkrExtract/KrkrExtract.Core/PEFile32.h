/*******************************************************************************
********************************   Team AT4RE   ********************************
********************************************************************************
*******************  PLEASE DON'T CHANGE/REMOVE THIS HEADER  *******************
********************************************************************************
**                                                                            **
**	Title:		PEFile class.                                                 **
**	Desc:		A handy class to manipulate pe files.                         **
**	Author:		MohammadHi [ in4matics at hotmail dot com ]                   **
**	WwW:		AT4RE      [ http://www.at4re.com ]                           **
**	Date:		2008-01-28                                                    **
**                                                                            **
********************************************************************************
*******************************************************************************/

/*
[  PE File Format   ]
---------------------
|    DOS Header     |
---------------------
|     DOS Stub      |
---------------------
|     PE Header     |
---------------------
|   Section Table   |
---------------------
|      Padding      |
---------------------
|     Section 1     |
---------------------
|     Section 2     |
---------------------
|	     ...        |
---------------------
|     Section n     |
---------------------*/

//==============================================================================
#pragma once

#include <windows.h>

#pragma pack(1)
//==============================================================================
//==============================================================================
#define MAX_SECTION_COUNT		64
#define SECTION_IMPORT			"@.import"
#define SECTION_RESERV			"@.reserv"
//==============================================================================
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
	char* RawData;
	DWORD   Size;
};
struct PE_SECTION_DATA {
	DWORD   Offset;
	char* RawData;
	DWORD   Size;
};
struct PE_IMPORT_FUNCTION {
	char* FunctionName;
	int					FunctionId;
	PE_IMPORT_FUNCTION* Next;
};
struct PE_IMPORT_DLL {
	char* DllName;
	PE_IMPORT_FUNCTION* Functions;
	PE_IMPORT_DLL* Next;
};
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
	PEFile(WCHAR* filePath);
	~PEFile();
	bool				loadFromFile(WCHAR* filePath);
	bool				loadFromMemory(char* memoryAddress);
	bool				saveToFile(WCHAR* filePath);
	int					addSection(char* name, DWORD size, bool isExecutable);
	void				addImport(char* dllName, char** functions, int functionCount);
	void				commit();

private:
	char* peMemory;

	void				init();
	bool				readFileData(WCHAR* filePath);
	bool				checkValidity();
	bool				readHeaders();
	bool				readBody();
	bool				readImportTable();
	bool				writePadding(HANDLE fileHandle, long paddingSize);
	void				unloadFile();

	void				buildImportTable();
	char* buildNewImports(DWORD baseRVA);
	DWORD				calcNewImportsSize(DWORD& sizeDlls, DWORD& sizeFunctions, DWORD& sizeStrings);

	DWORD				alignNumber(DWORD number, DWORD alignment);
	DWORD				rvaToOffset(DWORD rva);
	DWORD				offsetToRVA(DWORD offset);

	void				fixReservedData();
	void				fixHeaders();
	void				fixSectionTable();
	void* VaToPtr(DWORD dwVA);
};
//==============================================================================
