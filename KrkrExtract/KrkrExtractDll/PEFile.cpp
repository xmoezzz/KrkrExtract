#include "PEFile.h"
#include <math.h>

PEFile::PEFile() {
	init();
}
//==============================================================================
PEFile::PEFile(LPCWSTR filePath) {
	init();
	loadFromFile(filePath);
}
//==============================================================================
PEFile::~PEFile() {
	unloadFile();
}
//==============================================================================
void PEFile::init() {
	peMemory = NULL;
	ZeroMemory(&newImports, sizeof(PE_IMPORT_DLL));
}
//==============================================================================
bool PEFile::readFileData(LPCWSTR filePath) {
	// open the file for read
	HANDLE fileHandle = CreateFileW(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (fileHandle == INVALID_HANDLE_VALUE) {
		return false;
	}

	// get the file size
	DWORD fileSize = GetFileSize(fileHandle, 0);
	if (fileSize == 0) {
		CloseHandle(fileHandle);
		return false;
	}

	// allocate memory to read the pe file (note that we used VirtualAlloc not GlobalAlloc!)
	peMemory = (PBYTE)VirtualAlloc(NULL, fileSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (peMemory == NULL) {
		CloseHandle(fileHandle);
		return false;
	}

	DWORD bytesRead;
	// read whole file data
	if (!ReadFile(fileHandle, peMemory, fileSize, &bytesRead, NULL) || bytesRead != fileSize) {
		CloseHandle(fileHandle);
		return false;
	}

	// close the file
	CloseHandle(fileHandle);

	return true;
}
//==============================================================================
bool PEFile::checkValidity() {
	// 'dosHeader.Signature' must be "MZ" && 'peHeaders.Signature' must be "PE\0\0"
	if (dosHeader.Signature != IMAGE_DOS_SIGNATURE || peHeaders.Signature != IMAGE_NT_SIGNATURE) {
		unloadFile();
		return false;
	}

	if (peHeaders.FileHeader.NumberOfSections > MAX_SECTION_COUNT) {
		unloadFile();
		return false;
	}

	return true;
}
//==============================================================================
bool PEFile::readHeaders() {
	// read dos/pe headers
	CopyMemory(&dosHeader, peMemory, sizeof(PE_DOS_HEADER));
	dosStub.RawData = peMemory + sizeof(PE_DOS_HEADER);
	dosStub.Size = dosHeader.PEHeaderOffset - sizeof(PE_DOS_HEADER);
	CopyMemory(&peHeaders, peMemory + dosHeader.PEHeaderOffset, sizeof(PE_NT_HEADERS));

	// check validity of the file to ensure that we loaded a "PE File" not another thing!
	if (!checkValidity()) {
		return false;
	}

	// read section table
	ZeroMemory(sectionTable, sizeof(sectionTable));
	CopyMemory(sectionTable, peMemory + dosHeader.PEHeaderOffset + sizeof(PE_NT_HEADERS),
		peHeaders.FileHeader.NumberOfSections * sizeof(PE_SECTION_HEADER));

	return true;
}
//==============================================================================
bool PEFile::readBody() {
	// read reserved data
	DWORD reservedDataOffset = dosHeader.PEHeaderOffset + sizeof(PE_NT_HEADERS) +
		peHeaders.FileHeader.NumberOfSections * sizeof(PE_SECTION_HEADER);

	reservedData.Offset = reservedDataOffset;
	reservedData.RawData = peMemory + reservedDataOffset;
	/*reservedData.Size = peHeaders.OptionalHeader.SizeOfHeaders - reservedDataOffset;*/
	if (sectionTable[0].PointerToRawData > 0) {
		reservedData.Size = sectionTable[0].PointerToRawData - reservedDataOffset;
	}
	else {
		reservedData.Size = sectionTable[0].VirtualAddress - reservedDataOffset;
	}

	// read sections
	for (int i = 0; i < peHeaders.FileHeader.NumberOfSections; i++) {
		sections[i].Offset = sectionTable[i].PointerToRawData;
		sections[i].RawData = peMemory + sectionTable[i].PointerToRawData;
		sections[i].Size = sectionTable[i].SizeOfRawData;
	}

	return true;
}
//==============================================================================
bool PEFile::readImportTable() {
	DWORD tableRVA = peHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
	DWORD tableOffset = rvaToOffset(tableRVA);
	if (tableOffset == 0) {
		return false;
	}

	ZeroMemory(&importTable, sizeof(PE_IMPORT_DLL));

	IMAGE_IMPORT_DESCRIPTOR* importDesc = (IMAGE_IMPORT_DESCRIPTOR*)(peMemory + tableOffset);
	IMAGE_THUNK_DATA* importThunk;
	PE_IMPORT_DLL* importDll = &this->importTable;
	PE_IMPORT_FUNCTION* importFunction;

	while (true) {
		importDll->DllName = (char*)(peMemory + rvaToOffset(importDesc->Name));
		if (importDesc->OriginalFirstThunk > 0) {
			importThunk = (IMAGE_THUNK_DATA*)(peMemory + rvaToOffset(importDesc->OriginalFirstThunk));
		}
		else {
			importThunk = (IMAGE_THUNK_DATA*)(peMemory + rvaToOffset(importDesc->FirstThunk));
		}

		importDll->Functions = new PE_IMPORT_FUNCTION();
		ZeroMemory(importDll->Functions, sizeof(PE_IMPORT_FUNCTION));
		importFunction = importDll->Functions;
		while (true) {
			if ((importThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG32) == IMAGE_ORDINAL_FLAG32) {
				importFunction->FunctionId = IMAGE_ORDINAL32(importThunk->u1.Ordinal);
			}
			else {
				DWORD nameOffset = rvaToOffset(importThunk->u1.AddressOfData);
				importFunction->FunctionName = (char*)(peMemory + nameOffset + 2);
			}

			importThunk = (IMAGE_THUNK_DATA*)((char*)importThunk + sizeof(IMAGE_THUNK_DATA));
			if (importThunk->u1.AddressOfData == 0) {
				break;
			}
			importFunction->Next = new PE_IMPORT_FUNCTION();
			ZeroMemory(importFunction->Next, sizeof(PE_IMPORT_FUNCTION));
			importFunction = importFunction->Next;
		}

		importDesc = (IMAGE_IMPORT_DESCRIPTOR*)((char*)importDesc + sizeof(IMAGE_IMPORT_DESCRIPTOR));
		if (importDesc->Name == 0) {
			break;
		}
		importDll->Next = new PE_IMPORT_DLL();
		ZeroMemory(importDll->Next, sizeof(PE_IMPORT_DLL));
		importDll = importDll->Next;
	}

	return true;
}
//==============================================================================
bool PEFile::loadFromFile(LPCWSTR filePath) {
	unloadFile();

	return readFileData(filePath) &&
		readHeaders() &&
		readBody() &&
		readImportTable();
}
//==============================================================================
bool PEFile::loadFromMemory(PBYTE memoryAddress) {
	unloadFile();

	peMemory = memoryAddress;

	return readHeaders()/* &&
						readBody() &&
						readImportTable()*/;
}
//==============================================================================
bool PEFile::saveToFile(LPCWSTR filePath) {
	commit();
	buildImportTable();

	// create the output file
	HANDLE fileHandle = CreateFileW(filePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (fileHandle == INVALID_HANDLE_VALUE) {
		return false;
	}

	DWORD bytesWritten;

	WriteFile(fileHandle, &dosHeader, sizeof(PE_DOS_HEADER), &bytesWritten, NULL);
	WriteFile(fileHandle, dosStub.RawData, dosStub.Size, &bytesWritten, NULL);
	writePadding(fileHandle, dosHeader.PEHeaderOffset - sizeof(PE_DOS_HEADER) - dosStub.Size);
	WriteFile(fileHandle, &peHeaders, sizeof(PE_NT_HEADERS), &bytesWritten, NULL);
	WriteFile(fileHandle, &sectionTable, peHeaders.FileHeader.NumberOfSections * sizeof(PE_SECTION_HEADER), &bytesWritten, NULL);
	WriteFile(fileHandle, reservedData.RawData, reservedData.Size, &bytesWritten, NULL);

	for (int i = 0; i < peHeaders.FileHeader.NumberOfSections; i++) {
		writePadding(fileHandle, sectionTable[i].PointerToRawData - GetFileSize(fileHandle, NULL));
		WriteFile(fileHandle, sections[i].RawData, sections[i].Size, &bytesWritten, NULL);
	}

	CloseHandle(fileHandle);

	return true;
}
//==============================================================================
bool PEFile::writePadding(HANDLE fileHandle, long paddingSize) {
	if (paddingSize <= 0)
		return false;

	DWORD bytesWritten;
	char* padding = new char[paddingSize];
	memset(padding, 0, paddingSize);
	WriteFile(fileHandle, padding, paddingSize, &bytesWritten, NULL);
	delete padding;

	return (bytesWritten == paddingSize);
}
//==============================================================================
void PEFile::unloadFile() {
	if (peMemory != NULL) {
		VirtualFree(peMemory, 0, MEM_RELEASE);
		peMemory = NULL;
	}
}
//==============================================================================
void PEFile::buildImportTable() {
	DWORD sizeDlls = 0;
	DWORD sizeFunctions = 0;
	DWORD sizeStrings = 0;
	DWORD newImportsSize = calcNewImportsSize(sizeDlls, sizeFunctions, sizeStrings);

	// we'll move the old dll list to the new import table, so we'll calc its size
	DWORD oldImportDllsSize = 0;
	PE_IMPORT_DLL* importDll = &this->importTable;
	while (importDll != NULL) {
		oldImportDllsSize += sizeof(IMAGE_IMPORT_DESCRIPTOR);
		importDll = importDll->Next;
	}

	// add a new section to handle the new import table
	int index = addSection(SECTION_IMPORT, oldImportDllsSize + newImportsSize, false);

	// copy old import dll list
	DWORD oldImportTableRVA = peHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
	DWORD oldImportTableOffset = rvaToOffset(oldImportTableRVA);
	CopyMemory(sections[index].RawData, peMemory + oldImportTableOffset, oldImportDllsSize);

	// copy new imports
	char* newImportsData = buildNewImports(sectionTable[index].VirtualAddress + oldImportDllsSize);
	CopyMemory(sections[index].RawData + oldImportDllsSize, newImportsData, newImportsSize);

	peHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = sectionTable[index].VirtualAddress;
	peHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size = sectionTable[index].SizeOfRawData;
	peHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress = 0;
	peHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].Size = 0;
}
//==============================================================================
char* PEFile::buildNewImports(DWORD baseRVA) {
	commit();

	IMAGE_IMPORT_DESCRIPTOR importDesc;
	IMAGE_THUNK_DATA importThunk;
	PE_IMPORT_DLL* importDll;
	PE_IMPORT_FUNCTION* importFunction;

	DWORD sizeDlls = 0;
	DWORD sizeFunctions = 0;
	DWORD sizeStrings = 0;
	DWORD newImportsSize = calcNewImportsSize(sizeDlls, sizeFunctions, sizeStrings);
	DWORD offsetDlls = 0;
	DWORD offsetFunctions = sizeDlls;
	DWORD offsetStrings = sizeDlls + 2 * sizeFunctions;

	char* buffer = new char[newImportsSize];
	ZeroMemory(buffer, newImportsSize);

	importDll = &newImports;
	while (importDll != NULL) {
		ZeroMemory(&importDesc, sizeof(IMAGE_IMPORT_DESCRIPTOR));
		importDesc.OriginalFirstThunk = baseRVA + offsetFunctions;
		importDesc.FirstThunk = baseRVA + offsetFunctions + sizeFunctions;
		importDesc.Name = baseRVA + offsetStrings;
		CopyMemory(buffer + offsetStrings, importDll->DllName, strlen(importDll->DllName));
		offsetStrings += alignNumber((DWORD)strlen(importDll->DllName) + 1, 2);

		CopyMemory(buffer + offsetDlls, &importDesc, sizeof(IMAGE_IMPORT_DESCRIPTOR));
		offsetDlls += sizeof(IMAGE_IMPORT_DESCRIPTOR);

		importFunction = importDll->Functions;
		while (importFunction != NULL) {
			ZeroMemory(&importThunk, sizeof(IMAGE_THUNK_DATA));
			if (importFunction->FunctionId != 0) {
				importThunk.u1.Ordinal = importFunction->FunctionId | IMAGE_ORDINAL_FLAG32;
			}
			else {
				importThunk.u1.AddressOfData = baseRVA + offsetStrings;
				CopyMemory(buffer + offsetStrings + 2, importFunction->FunctionName, strlen(importFunction->FunctionName));
				offsetStrings += 2 + alignNumber((DWORD)strlen(importFunction->FunctionName) + 1, 2);
			}

			CopyMemory(buffer + offsetFunctions, &importThunk, sizeof(IMAGE_THUNK_DATA));
			CopyMemory(buffer + offsetFunctions + sizeFunctions, &importThunk, sizeof(IMAGE_THUNK_DATA));
			offsetFunctions += sizeof(IMAGE_THUNK_DATA);

			importFunction = importFunction->Next;
		}
		offsetFunctions += sizeof(IMAGE_THUNK_DATA);

		importDll = importDll->Next;
	}

	return buffer;
}
//==============================================================================
DWORD PEFile::calcNewImportsSize(DWORD &sizeDlls, DWORD &sizeFunctions, DWORD &sizeStrings) {
	PE_IMPORT_DLL* importDll = &this->newImports;
	PE_IMPORT_FUNCTION* importFunction;

	// calc added imports size
	while (importDll != NULL) {
		sizeDlls += sizeof(IMAGE_IMPORT_DESCRIPTOR);
		sizeStrings += alignNumber((DWORD)strlen(importDll->DllName) + 1, 2);
		importFunction = importDll->Functions;
		while (importFunction != NULL) {
			sizeFunctions += sizeof(IMAGE_THUNK_DATA);
			if (importFunction->FunctionId == 0) {
				sizeStrings += 2 + alignNumber((DWORD)strlen(importFunction->FunctionName) + 1, 2);
			}
			importFunction = importFunction->Next;
		}
		sizeFunctions += sizeof(IMAGE_THUNK_DATA); // for the terminator thunk data
		importDll = importDll->Next;
	}
	sizeDlls += sizeof(IMAGE_IMPORT_DESCRIPTOR); // for the terminator import descriptor

	return sizeDlls + 2 * sizeFunctions + sizeStrings;
}
//==============================================================================
int PEFile::addSection(char* name, DWORD size, bool isExecutable) {
	if (peHeaders.FileHeader.NumberOfSections == MAX_SECTION_COUNT) {
		return -1;
	}

	PE_SECTION_DATA &newSection = sections[peHeaders.FileHeader.NumberOfSections];
	PE_SECTION_HEADER &newSectionHeader = sectionTable[peHeaders.FileHeader.NumberOfSections];
	PE_SECTION_HEADER &lastSectionHeader = sectionTable[peHeaders.FileHeader.NumberOfSections - 1];

	DWORD sectionSize = alignNumber(size, peHeaders.OptionalHeader.FileAlignment);
	DWORD virtualSize = alignNumber(sectionSize, peHeaders.OptionalHeader.SectionAlignment);

	DWORD sectionOffset = alignNumber(lastSectionHeader.PointerToRawData + lastSectionHeader.SizeOfRawData, peHeaders.OptionalHeader.FileAlignment);
	DWORD virtualOffset = alignNumber(lastSectionHeader.VirtualAddress + lastSectionHeader.Misc.VirtualSize, peHeaders.OptionalHeader.SectionAlignment);

	ZeroMemory(&newSectionHeader, sizeof(IMAGE_SECTION_HEADER));
	CopyMemory(newSectionHeader.Name, name, (strlen(name) > 8 ? 8 : strlen(name)));

	newSectionHeader.PointerToRawData = sectionOffset;
	newSectionHeader.VirtualAddress = virtualOffset;
	newSectionHeader.SizeOfRawData = sectionSize;
	newSectionHeader.Misc.VirtualSize = virtualSize;
	newSectionHeader.Characteristics = //0xC0000040; 
		IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_CNT_INITIALIZED_DATA;

	if (isExecutable) {
		newSectionHeader.Characteristics |= IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE;
	}

	newSection.RawData = (PBYTE)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, sectionSize);
	newSection.Size = sectionSize;

	peHeaders.FileHeader.NumberOfSections++;
	if (reservedData.Size > 0) {
		reservedData.Size -= sizeof(IMAGE_SECTION_HEADER);
	}

	// return new section index
	return peHeaders.FileHeader.NumberOfSections - 1;
}
//==============================================================================
void PEFile::addImport(char* dllName, char** functions, int functionCount) {
	PE_IMPORT_DLL* importDll = &this->newImports;
	PE_IMPORT_FUNCTION* importFunction;

	if (newImports.DllName != NULL) {
		while (importDll->Next != NULL) {
			importDll = importDll->Next;
		}
		importDll->Next = new PE_IMPORT_DLL();
		importDll = importDll->Next;
	}
	importDll->DllName = dllName;
	importDll->Functions = new PE_IMPORT_FUNCTION();
	importDll->Next = NULL;

	importFunction = importDll->Functions;
	importFunction->FunctionName = functions[0];
	for (int i = 1; i < functionCount; i++) {
		importFunction->Next = new PE_IMPORT_FUNCTION();
		importFunction = importFunction->Next;
		importFunction->FunctionName = functions[i];
	}
	importFunction->Next = NULL;
}
//==============================================================================
DWORD PEFile::alignNumber(DWORD number, DWORD alignment) {
	return (DWORD)(ceil(number / (alignment + 0.0)) * alignment);
}
//==============================================================================
DWORD PEFile::rvaToOffset(DWORD rva) {
	for (int i = 0; i < peHeaders.FileHeader.NumberOfSections; i++) {
		if (rva >= sectionTable[i].VirtualAddress &&
			rva < sectionTable[i].VirtualAddress + sectionTable[i].Misc.VirtualSize) {
			return sectionTable[i].PointerToRawData + (rva - sectionTable[i].VirtualAddress);
		}
	}
	return 0;
}
//==============================================================================
DWORD PEFile::offsetToRVA(DWORD offset) {
	for (int i = 0; i < peHeaders.FileHeader.NumberOfSections; i++) {
		if (offset >= sectionTable[i].PointerToRawData &&
			offset < sectionTable[i].PointerToRawData + sectionTable[i].SizeOfRawData) {
			return sectionTable[i].VirtualAddress + (offset - sectionTable[i].PointerToRawData);
		}
	}
	return 0;
}
//==============================================================================
void PEFile::commit() {
	fixReservedData();
	fixHeaders();
	fixSectionTable();
}
//==============================================================================
void PEFile::fixReservedData() {
	DWORD dirIndex = 0;
	for (dirIndex = 0; dirIndex < peHeaders.OptionalHeader.NumberOfRvaAndSizes; dirIndex++) {
		if (peHeaders.OptionalHeader.DataDirectory[dirIndex].VirtualAddress > 0 &&
			peHeaders.OptionalHeader.DataDirectory[dirIndex].VirtualAddress >= reservedData.Offset &&
			peHeaders.OptionalHeader.DataDirectory[dirIndex].VirtualAddress < reservedData.Size) {
			break;
		}
	}

	if (dirIndex == peHeaders.OptionalHeader.NumberOfRvaAndSizes) {
		return;
	}

	int sectionIndex = addSection(SECTION_RESERV, reservedData.Size, false);
	CopyMemory(sections[sectionIndex].RawData, reservedData.RawData, reservedData.Size);

	for (dirIndex = 0; dirIndex < peHeaders.OptionalHeader.NumberOfRvaAndSizes; dirIndex++) {
		if (peHeaders.OptionalHeader.DataDirectory[dirIndex].VirtualAddress > 0 &&
			peHeaders.OptionalHeader.DataDirectory[dirIndex].VirtualAddress >= reservedData.Offset &&
			peHeaders.OptionalHeader.DataDirectory[dirIndex].VirtualAddress < reservedData.Size) {
			peHeaders.OptionalHeader.DataDirectory[dirIndex].VirtualAddress +=
				sectionTable[sectionIndex].VirtualAddress - reservedData.Offset;
		}
	}

	reservedData.Size = 0;
}
//==============================================================================
void PEFile::fixHeaders() {
	peHeaders.OptionalHeader.SizeOfHeaders = alignNumber(dosHeader.PEHeaderOffset + peHeaders.FileHeader.SizeOfOptionalHeader +
		peHeaders.FileHeader.NumberOfSections * sizeof(PE_SECTION_HEADER), peHeaders.OptionalHeader.FileAlignment);

	DWORD imageSize = peHeaders.OptionalHeader.SizeOfHeaders;
	for (int i = 0; i < peHeaders.FileHeader.NumberOfSections; i++) {
		imageSize += alignNumber(sectionTable[i].Misc.VirtualSize, peHeaders.OptionalHeader.SectionAlignment);
	}
	peHeaders.OptionalHeader.SizeOfImage = alignNumber(imageSize, peHeaders.OptionalHeader.SectionAlignment);

	peHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress = 0;
	peHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].Size = 0;
}
//==============================================================================
void PEFile::fixSectionTable() {
	DWORD offset = peHeaders.OptionalHeader.SizeOfHeaders;
	for (int i = 0; i < peHeaders.FileHeader.NumberOfSections; i++) {
		sectionTable[i].Characteristics |= IMAGE_SCN_MEM_WRITE;
		offset = alignNumber(offset, peHeaders.OptionalHeader.FileAlignment);
		sectionTable[i].PointerToRawData = offset;
		//sectionTable[i].SizeOfRawData = alignNumber(offset + sectionTable[i].Misc.VirtualSize, peHeaders.OptionalHeader.FileAlignment);
		offset += sectionTable[i].SizeOfRawData;
	}
}
//==============================================================================