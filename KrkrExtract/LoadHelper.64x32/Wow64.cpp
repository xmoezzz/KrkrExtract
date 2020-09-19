#include <Windows.h>
#include <Psapi.h>
#include <string>

using std::string;

//-----------------------------------------------------------------------------

HMODULE WINAPI GetRemoteModuleHandle(HANDLE hProcess, LPCSTR lpModuleName)
{
	HMODULE* ModuleArray = NULL;
	DWORD ModuleArraySize = 100;
	DWORD NumModules = 0;
	WCHAR lpModuleNameCopy[MAX_PATH] = { 0 };
	WCHAR ModuleNameBuffer[MAX_PATH] = { 0 };

	/* Make sure we didn't get a NULL pointer for the module name */
	if (lpModuleName == NULL)
		goto GRMH_FAIL_JMP;

	/* Convert lpModuleName to all lowercase so the comparison isn't case sensitive */
	for (size_t i = 0; lpModuleName[i] != '\0'; ++i)
	{
		if (lpModuleName[i] >= L'A' && lpModuleName[i] <= L'Z')
			lpModuleNameCopy[i] = lpModuleName[i] + 0x20; // 0x20 is the difference between uppercase and lowercase
		else
			lpModuleNameCopy[i] = lpModuleName[i];

		lpModuleNameCopy[i + 1] = '\0';
	}

	/* Allocate memory to hold the module handles */
	ModuleArray = new HMODULE[ModuleArraySize];

	/* Check if the allocation failed */
	if (ModuleArray == NULL)
		goto GRMH_FAIL_JMP;

	/* Get handles to all the modules in the target process */
	if (!::EnumProcessModulesEx(hProcess, ModuleArray,
		ModuleArraySize * sizeof(HMODULE), &NumModules, LIST_MODULES_ALL))
		goto GRMH_FAIL_JMP;

	/* We want the number of modules not the number of bytes */
	NumModules /= sizeof(HMODULE);

	/* Did we allocate enough memory for all the module handles? */
	if (NumModules > ModuleArraySize)
	{
		delete[] ModuleArray; // Deallocate so we can try again
		ModuleArray = NULL; // Set it to NULL se we can be sure if the next try fails
		ModuleArray = new HMODULE[NumModules]; // Allocate the right amount of memory

		/* Check if the allocation failed */
		if (ModuleArray == NULL)
			goto GRMH_FAIL_JMP;

		ModuleArraySize = NumModules; // Update the size of the array

		/* Get handles to all the modules in the target process */
		if (!::EnumProcessModulesEx(hProcess, ModuleArray,
			ModuleArraySize * sizeof(HMODULE), &NumModules, LIST_MODULES_ALL))
			goto GRMH_FAIL_JMP;

		/* We want the number of modules not the number of bytes */
		NumModules /= sizeof(HMODULE);
	}

	/* Iterate through all the modules and see if the names match the one we are looking for */
	for (DWORD i = 0; i <= NumModules; ++i)
	{
		/* Get the module's name */
		::GetModuleBaseName(hProcess, ModuleArray[i],
			ModuleNameBuffer, sizeof(ModuleNameBuffer));

		/* Convert ModuleNameBuffer to all lowercase so the comparison isn't case sensitive */
		for (size_t j = 0; ModuleNameBuffer[j] != '\0'; ++i)
		{
			if (ModuleNameBuffer[j] >= L'A' && ModuleNameBuffer[j] <= L'Z')
				ModuleNameBuffer[j] += 0x20; // 0x20 is the difference between uppercase and lowercase
		}

		/* Does the name match? */
		if (wcsstr(ModuleNameBuffer, lpModuleNameCopy) != NULL)
		{
			/* Make a temporary variable to hold return value*/
			HMODULE TempReturn = ModuleArray[i];

			/* Give back that memory */
			delete[] ModuleArray;

			/* Success */
			return TempReturn;
		}

		/* Wrong module let's try the next... */
	}

	/* Uh Oh... */
GRMH_FAIL_JMP:

	/* If we got to the point where we allocated memory we need to give it back */
	if (ModuleArray != NULL)
		delete[] ModuleArray;

	/* Failure... */
	return NULL;
}


//-----------------------------------------------------------------------------

FARPROC WINAPI GetRemoteProcAddress(HANDLE hProcess, HMODULE hModule, LPCSTR lpProcName, UINT Ordinal, BOOL UseOrdinal)
{
	BOOL Is64Bit = FALSE;
	MODULEINFO RemoteModuleInfo = { 0 };
	UINT_PTR RemoteModuleBaseVA = 0;
	IMAGE_DOS_HEADER DosHeader = { 0 };
	DWORD Signature = 0;
	IMAGE_FILE_HEADER FileHeader = { 0 };
	IMAGE_OPTIONAL_HEADER64 OptHeader64 = { 0 };
	IMAGE_OPTIONAL_HEADER32 OptHeader32 = { 0 };
	IMAGE_DATA_DIRECTORY ExportDirectory = { 0 };
	IMAGE_EXPORT_DIRECTORY ExportTable = { 0 };
	UINT_PTR ExportFunctionTableVA = 0;
	UINT_PTR ExportNameTableVA = 0;
	UINT_PTR ExportOrdinalTableVA = 0;
	DWORD* ExportFunctionTable = NULL;
	DWORD* ExportNameTable = NULL;
	WORD* ExportOrdinalTable = NULL;

	/* Temporary variables not used until much later but easier
	/* to define here than in all the the places they are used */
	CHAR TempChar;
	BOOL Done = FALSE;

	/* Check to make sure we didn't get a NULL pointer for the name unless we are searching by ordinal */
	if (lpProcName == NULL && !UseOrdinal)
		goto GRPA_FAIL_JMP;

	/* Get the base address of the remote module along with some other info we don't need */
	if (!::GetModuleInformation(hProcess, hModule, &RemoteModuleInfo, sizeof(RemoteModuleInfo)))
		goto GRPA_FAIL_JMP;
	RemoteModuleBaseVA = (UINT_PTR)RemoteModuleInfo.lpBaseOfDll;

	/* Read the DOS header and check it's magic number */
	if (!::ReadProcessMemory(hProcess, (LPCVOID)RemoteModuleBaseVA, &DosHeader,
		sizeof(DosHeader), NULL) || DosHeader.e_magic != IMAGE_DOS_SIGNATURE)
		goto GRPA_FAIL_JMP;

	/* Read and check the NT signature */
	if (!::ReadProcessMemory(hProcess, (LPCVOID)(RemoteModuleBaseVA + DosHeader.e_lfanew),
		&Signature, sizeof(Signature), NULL) || Signature != IMAGE_NT_SIGNATURE)
		goto GRPA_FAIL_JMP;

	/* Read the main header */
	if (!::ReadProcessMemory(hProcess,
		(LPCVOID)(RemoteModuleBaseVA + DosHeader.e_lfanew + sizeof(Signature)),
		&FileHeader, sizeof(FileHeader), NULL))
		goto GRPA_FAIL_JMP;

	/* Which type of optional header is the right size? */
	if (FileHeader.SizeOfOptionalHeader == sizeof(OptHeader64))
		Is64Bit = TRUE;
	else if (FileHeader.SizeOfOptionalHeader == sizeof(OptHeader32))
		Is64Bit = FALSE;
	else
		goto GRPA_FAIL_JMP;

	if (Is64Bit)
	{
		/* Read the optional header and check it's magic number */
		if (!::ReadProcessMemory(hProcess,
			(LPCVOID)(RemoteModuleBaseVA + DosHeader.e_lfanew + sizeof(Signature) + sizeof(FileHeader)),
			&OptHeader64, FileHeader.SizeOfOptionalHeader, NULL)
			|| OptHeader64.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
			goto GRPA_FAIL_JMP;
	}
	else
	{
		/* Read the optional header and check it's magic number */
		if (!::ReadProcessMemory(hProcess,
			(LPCVOID)(RemoteModuleBaseVA + DosHeader.e_lfanew + sizeof(Signature) + sizeof(FileHeader)),
			&OptHeader32, FileHeader.SizeOfOptionalHeader, NULL)
			|| OptHeader32.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
			goto GRPA_FAIL_JMP;
	}

	/* Make sure the remote module has an export directory and if it does save it's relative address and size */
	if (Is64Bit && OptHeader64.NumberOfRvaAndSizes >= IMAGE_DIRECTORY_ENTRY_EXPORT + 1)
	{
		ExportDirectory.VirtualAddress = (OptHeader64.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT]).VirtualAddress;
		ExportDirectory.Size = (OptHeader64.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT]).Size;
	}
	else if (OptHeader32.NumberOfRvaAndSizes >= IMAGE_DIRECTORY_ENTRY_EXPORT + 1)
	{
		ExportDirectory.VirtualAddress = (OptHeader32.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT]).VirtualAddress;
		ExportDirectory.Size = (OptHeader32.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT]).Size;
	}
	else
		goto GRPA_FAIL_JMP;

	/* Read the main export table */
	if (!::ReadProcessMemory(hProcess, (LPCVOID)(RemoteModuleBaseVA + ExportDirectory.VirtualAddress),
		&ExportTable, sizeof(ExportTable), NULL))
		goto GRPA_FAIL_JMP;

	/* Save the absolute address of the tables so we don't need to keep adding the base address */
	ExportFunctionTableVA = RemoteModuleBaseVA + ExportTable.AddressOfFunctions;
	ExportNameTableVA = RemoteModuleBaseVA + ExportTable.AddressOfNames;
	ExportOrdinalTableVA = RemoteModuleBaseVA + ExportTable.AddressOfNameOrdinals;

	/* Allocate memory for our copy of the tables */
	ExportFunctionTable = new DWORD[ExportTable.NumberOfFunctions];
	ExportNameTable = new DWORD[ExportTable.NumberOfNames];
	ExportOrdinalTable = new WORD[ExportTable.NumberOfNames];

	/* Check if the allocation failed */
	if (ExportFunctionTable == NULL || ExportNameTable == NULL || ExportOrdinalTable == NULL)
		goto GRPA_FAIL_JMP;

	/* Get a copy of the function table */
	if (!::ReadProcessMemory(hProcess, (LPCVOID)ExportFunctionTableVA,
		ExportFunctionTable, ExportTable.NumberOfFunctions * sizeof(DWORD), NULL))
		goto GRPA_FAIL_JMP;

	/* Get a copy of the name table */
	if (!::ReadProcessMemory(hProcess, (LPCVOID)ExportNameTableVA,
		ExportNameTable, ExportTable.NumberOfNames * sizeof(DWORD), NULL))
		goto GRPA_FAIL_JMP;

	/* Get a copy of the ordinal table */
	if (!::ReadProcessMemory(hProcess, (LPCVOID)ExportOrdinalTableVA,
		ExportOrdinalTable, ExportTable.NumberOfNames * sizeof(WORD), NULL))
		goto GRPA_FAIL_JMP;

	/* If we are searching for an ordinal we do that now */
	if (UseOrdinal)
	{
		/* NOTE:
		/* Microsoft's PE/COFF specification does NOT say we need to subtract the ordinal base
		/* from our ordinal but it seems to always give the wrong function if we don't */

		/* Make sure the ordinal is valid */
		if (Ordinal < ExportTable.Base || (Ordinal - ExportTable.Base) >= ExportTable.NumberOfFunctions)
			goto GRPA_FAIL_JMP;

		UINT FunctionTableIndex = Ordinal - ExportTable.Base;

		/* Check if the function is forwarded and if so get the real address*/
		if (ExportFunctionTable[FunctionTableIndex] >= ExportDirectory.VirtualAddress &&
			ExportFunctionTable[FunctionTableIndex] <= ExportDirectory.VirtualAddress + ExportDirectory.Size)
		{
			Done = FALSE;
			string TempForwardString;
			TempForwardString.clear(); // Empty the string so we can fill it with a new name

			/* Get the forwarder string one character at a time because we don't know how long it is */
			for (UINT_PTR i = 0; !Done; ++i)
			{
				/* Get next character */
				if (!::ReadProcessMemory(hProcess,
					(LPCVOID)(RemoteModuleBaseVA + ExportFunctionTable[FunctionTableIndex] + i),
					&TempChar, sizeof(TempChar), NULL))
					goto GRPA_FAIL_JMP;

				TempForwardString.push_back(TempChar); // Add it to the string

				/* If it's NUL we are done */
				if (TempChar == (CHAR)'\0')
					Done = TRUE;
			}

			/* Find the dot that seperates the module name and the function name/ordinal */
			size_t Dot = TempForwardString.find('.');
			if (Dot == string::npos)
				goto GRPA_FAIL_JMP;

			/* Temporary variables that hold parts of the forwarder string */
			string RealModuleName, RealFunctionId;
			RealModuleName = TempForwardString.substr(0, Dot - 1);
			RealFunctionId = TempForwardString.substr(Dot + 1, string::npos);

			HMODULE RealModule = GetRemoteModuleHandle(hProcess, RealModuleName.c_str());
			FARPROC TempReturn;// Make a temporary variable to hold return value 


			/* Figure out if the function was exported by name or by ordinal */
			if (RealFunctionId.at(0) == '#') // Exported by ordinal
			{
				UINT RealOrdinal = 0;
				RealFunctionId.erase(0, 1); // Remove '#' from string

				/* My version of atoi() because I was too lazy to use the real one... */
				for (size_t i = 0; i < RealFunctionId.size(); ++i)
				{
					if (RealFunctionId[i] >= '0' && RealFunctionId[i] <= '9')
					{
						RealOrdinal *= 10;
						RealOrdinal += RealFunctionId[i] - '0';
					}
					else
						break;
				}

				/* Recursively call this function to get return value */
				TempReturn = GetRemoteProcAddress(hProcess, RealModule, NULL, RealOrdinal, TRUE);
			}
			else // Exported by name
			{
				/* Recursively call this function to get return value */
				TempReturn = GetRemoteProcAddress(hProcess, RealModule, RealFunctionId.c_str(), 0, FALSE);
			}

			/* Give back that memory */
			delete[] ExportFunctionTable;
			delete[] ExportNameTable;
			delete[] ExportOrdinalTable;

			/* Success!!! */
			return TempReturn;
		}
		else // Not Forwarded
		{

			/* Make a temporary variable to hold return value*/
			FARPROC TempReturn = (FARPROC)(RemoteModuleBaseVA + ExportFunctionTable[FunctionTableIndex]);

			/* Give back that memory */
			delete[] ExportFunctionTable;
			delete[] ExportNameTable;
			delete[] ExportOrdinalTable;

			/* Success!!! */
			return TempReturn;
		}
	}


	/* Iterate through all the names and see if they match the one we are looking for */
	for (DWORD i = 0; i < ExportTable.NumberOfNames; ++i) {
		string TempFunctionName;

		Done = FALSE;// Reset for next name
		TempFunctionName.clear(); // Empty the string so we can fill it with a new name

		/* Get the function name one character at a time because we don't know how long it is */
		for (UINT_PTR j = 0; !Done; ++j)
		{
			/* Get next character */
			if (!::ReadProcessMemory(hProcess, (LPCVOID)(RemoteModuleBaseVA + ExportNameTable[i] + j),
				&TempChar, sizeof(TempChar), NULL))
				goto GRPA_FAIL_JMP;

			TempFunctionName.push_back(TempChar); // Add it to the string

			/* If it's NUL we are done */
			if (TempChar == (CHAR)'\0')
				Done = TRUE;
		}

		/* Does the name match? */
		if (TempFunctionName.find(lpProcName) != string::npos)
		{
			/* NOTE:
			/* Microsoft's PE/COFF specification says we need to subtract the ordinal base
			/*from the value in the ordinal table but that seems to always give the wrong function */

			/* Check if the function is forwarded and if so get the real address*/
			if (ExportFunctionTable[ExportOrdinalTable[i]] >= ExportDirectory.VirtualAddress &&
				ExportFunctionTable[ExportOrdinalTable[i]] <= ExportDirectory.VirtualAddress + ExportDirectory.Size)
			{
				Done = FALSE;
				string TempForwardString;
				TempForwardString.clear(); // Empty the string so we can fill it with a new name

				/* Get the forwarder string one character at a time because we don't know how long it is */
				for (UINT_PTR j = 0; !Done; ++j)
				{
					/* Get next character */
					if (!::ReadProcessMemory(hProcess,
						(LPCVOID)(RemoteModuleBaseVA + ExportFunctionTable[i] + j),
						&TempChar, sizeof(TempChar), NULL))
						goto GRPA_FAIL_JMP;

					TempForwardString.push_back(TempChar); // Add it to the string

					/* If it's NUL we are done */
					if (TempChar == (CHAR)'\0')
						Done = TRUE;
				}

				/* Find the dot that seperates the module name and the function name/ordinal */
				size_t Dot = TempForwardString.find('.');
				if (Dot == string::npos)
					goto GRPA_FAIL_JMP;

				/* Temporary variables that hold parts of the forwarder string */
				string RealModuleName, RealFunctionId;
				RealModuleName = TempForwardString.substr(0, Dot);
				RealFunctionId = TempForwardString.substr(Dot + 1, string::npos);

				HMODULE RealModule = GetRemoteModuleHandle(hProcess, RealModuleName.c_str());
				FARPROC TempReturn;// Make a temporary variable to hold return value 


				/* Figure out if the function was exported by name or by ordinal */
				if (RealFunctionId.at(0) == '#') // Exported by ordinal
				{
					UINT RealOrdinal = 0;
					RealFunctionId.erase(0, 1); // Remove '#' from string

					/* My version of atoi() because I was to lazy to use the real one... */
					for (size_t i = 0; i < RealFunctionId.size(); ++i)
					{
						if (RealFunctionId[i] >= '0' && RealFunctionId[i] <= '9')
						{
							RealOrdinal *= 10;
							RealOrdinal += RealFunctionId[i] - '0';
						}
						else
							break;
					}

					/* Recursively call this function to get return value */
					TempReturn = GetRemoteProcAddress(hProcess, RealModule, NULL, RealOrdinal, TRUE);
				}
				else // Exported by name
				{
					/* Recursively call this function to get return value */
					TempReturn = GetRemoteProcAddress(hProcess, RealModule, RealFunctionId.c_str(), 0, FALSE);
				}

				/* Give back that memory */
				delete[] ExportFunctionTable;
				delete[] ExportNameTable;
				delete[] ExportOrdinalTable;

				/* Success!!! */
				return TempReturn;
			}
			else // Not Forwarded
			{

				/* Make a temporary variable to hold return value*/
				FARPROC TempReturn;

				/* NOTE:
				/* Microsoft's PE/COFF specification says we need to subtract the ordinal base
				/*from the value in the ordinal table but that seems to always give the wrong function */
				//TempReturn = (FARPROC)(RemoteModuleBaseVA + ExportFunctionTable[ExportOrdinalTable[i] - ExportTable.Base]);

				/* So we do it this way instead */
				TempReturn = (FARPROC)(RemoteModuleBaseVA + ExportFunctionTable[ExportOrdinalTable[i]]);

				/* Give back that memory */
				delete[] ExportFunctionTable;
				delete[] ExportNameTable;
				delete[] ExportOrdinalTable;

				/* Success!!! */
				return TempReturn;
			}
		}

		/* Wrong function let's try the next... */
	}

	/* Uh Oh... */
GRPA_FAIL_JMP:

	/* If we got to the point where we allocated memory we need to give it back */
	if (ExportFunctionTable != NULL)
		delete[] ExportFunctionTable;
	if (ExportNameTable != NULL)
		delete[] ExportNameTable;
	if (ExportOrdinalTable != NULL)
		delete[] ExportOrdinalTable;

	/* Falure... */
	return NULL;
}
