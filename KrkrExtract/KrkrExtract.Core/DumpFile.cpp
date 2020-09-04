#include "DebuggerHandler.h"
#include <Utf.Convert.h>

DumpFileCommandPlugin::DumpFileCommandPlugin(KrkrClientProxyer* Proxyer) :
	m_Proxyer(Proxyer)
{
}

DumpFileCommandPlugin::~DumpFileCommandPlugin()
{
	m_Proxyer = nullptr;
}

PCWSTR NTAPI DumpFileCommandPlugin::GetCommandName()
{
	return L"df";
}

ArgumentParser* DumpFileCommandPlugin::PrepareParser()
{
	ArgumentParser* Parser;

	Parser = new (std::nothrow) ArgumentParser("df", "dump file");
	if (!Parser)
		return nullptr;

	Parser->add_argument("-n", "--name", "name")
		.position(ArgumentParser::Argument::Position::LAST);
	Parser->add_argument("-l", "--list", "list");

	return Parser;
}


void DumpFileCommandPlugin::FreeParser(ArgumentParser* Parser)
{
	if (Parser) {
		delete Parser;
	}
}


BOOL NTAPI DumpFileCommandPlugin::Exec(PWSTR Command)
{
	NTSTATUS        Status;
	std::wstring    Error;

	auto Parser = ParseCommandLine(Command, Error);
	if (!Parser)
		return FALSE;

	if (Error.length())
	{
		m_Proxyer->TellServerCommandResultOutput(
			CommandStatus::COMMAND_ERROR,
			Error.c_str()
		);

		return FALSE;
	}

	LOOP_ONCE
	{
		Status = STATUS_UNSUCCESSFUL;

		if (Parser->exists("name")) {
			Status = DumpWithFileName(Utf8ToUtf16(Parser->get<std::string>("name")).c_str());
		}
		else if (Parser->exists("list")) {
			Status = DumpWithFileName(Utf8ToUtf16(Parser->get<std::string>("list")).c_str());
		}
	}

	return NT_SUCCESS(Status);
}

BOOL NTAPI DumpFileCommandPlugin::Help(StringView* HelperString)
{
	if (HelperString) {
		HelperString->Assign(ShowHelp().c_str());
	}

	return HelperString && HelperString->Length() ? TRUE : FALSE;
}


void DumpFileCommandPlugin::ReleaseStreamSafe(IStream* Stream)
{
	SEH_TRY {
		Stream->Release();
	}
	SEH_EXCEPT(EXCEPTION_CONTINUE_EXECUTION) {
		m_Proxyer->TellServerLogOutput(LogLevel::LOG_ERROR, L"memory corrupted during releasing stream");
	}
}


NTSTATUS DumpFileCommandPlugin::DumpWithFileName(PCWSTR FileName)
{
	NTSTATUS          Status;
	HRESULT           Success;
	tTJSBinaryStream* BStream;
	IStream*          Stream;
	NtFileDisk        File;
	IPlugin*          DefaultPlugin;
	wstring           OutFilePath;
	

	LOOP_ONCE
	{
		BStream = NULL;
		BStream = m_Proxyer->HostTVPCreateStream(FileName);
		if (BStream == NULL)
		{
			m_Proxyer->TellServerLogOutput(
				LogLevel::LOG_ERROR,
				L"No such file : %s",
				FileName
			);

			Status = STATUS_NO_SUCH_FILE;
			break;
		}

		Stream = m_Proxyer->HostConvertBStreamToIStream(BStream);
		if (!Stream)
		{
			m_Proxyer->TellServerLogOutput(
				LogLevel::LOG_ERROR,
				L"no memory : %s",
				FileName
			);

			Status = STATUS_NO_MEMORY;
			break;
		}


		DefaultPlugin = m_Proxyer->GetDefaultPlugin();
		if (DefaultPlugin)
		{
			OutFilePath += m_Proxyer->GetWorkerDir();
			OutFilePath += L"\\KrkrExtract_Output\\";

			if (Nt_GetFileAttributes(FileName) == (ULONG)-1) 
			{
				Status = File.CreateDirectoryW(FileName, NULL);
				if (NT_FAILED(Status)) {
					break;
				}
			}

			OutFilePath += FileName;
			Success = DefaultPlugin->Unpack(OutFilePath.c_str(), Stream);
			if (Stream) {
				ReleaseStreamSafe(Stream);
			}

			if (FAILED(Success)) {
				Status = STATUS_UNSUCCESSFUL;
			}

			break;
		}

		Status = STATUS_UNSUCCESSFUL;
	}

	return Status;
}

NTSTATUS DumpFileCommandPlugin::DumpWithFileList(PCWSTR FileListName)
{
	BOOL            Success;
	vector<wstring> NameList;

	Success = ReadFileList(FileListName, NameList);
	if (!Success) {
		return STATUS_UNSUCCESSFUL;
	}

	for (auto& FileName : NameList) {
		DumpWithFileName(FileName.c_str());
	}
	
	return STATUS_SUCCESS;
}


BOOL GetLines(std::vector<std::wstring>& NameList, PWCHAR Buffer, ULONG Size)
{
	std::wstring ReadLine;
	ULONG        Offset;


	ReadLine.clear();
	Offset = 0;

	LOOP_FOREVER
	{
		if (Offset >= Size)
			break;

		if (Buffer[Offset] == L'\r')
		{
			if (ReadLine.length()) {
				NameList.push_back(ReadLine);
			}

			ReadLine.clear();
			Offset++;

			if (Offset >= Size)
				break;

			if (Buffer[Offset] == L'\n') {
				Offset++;
			}
		}

		if (Buffer[Offset] == '\n')
		{
			NameList.push_back(ReadLine);
			ReadLine.clear();
			Offset++;
		}

		ReadLine += Buffer[Offset];
		Offset++;
	}

	if (ReadLine.length()) {
		NameList.push_back(ReadLine);
	}

	return NameList.size() != 0;
};

BOOL DumpFileCommandPlugin::ReadFileList(PCWSTR FileListName, std::vector<std::wstring>& FileList)
{
	NTSTATUS   Status;
	NtFileDisk File;
	ULONG      Size;
	Encode     TextEncoding;
	ULONG      BytesInUnicodeString;
	ULONG      AllocatedBytes;
	BYTE       CurrentByte;

	Status = File.Open(FileListName);
	if (NT_FAILED(Status))
		return NT_SUCCESS(Status);

	Size = File.GetSize32();
	if (Size == 0)
		return NT_SUCCESS(Status);

	auto Buffer = AllocateMemorySafeP<BYTE>(Size);
	if (!Buffer)
		return FALSE;

	Status = File.Read(Buffer.get(), Size);
	if (NT_FAILED(Status))
		return NT_SUCCESS(Status);

	Status = DetectEncode(Buffer.get(), Size, TextEncoding);
	if (NT_FAILED(Status))
		return NT_SUCCESS(Status);

	switch (TextEncoding)
	{
	case Encode::ANSI:
	{
		AllocatedBytes       = ROUND_UP((Size + 1) * sizeof(WCHAR), sizeof(SIZE_T));
		BytesInUnicodeString = 0;

		auto UnicodeFile = AllocateMemorySafeP<WCHAR>(ROUND_UP((Size + 1) * sizeof(WCHAR), sizeof(SIZE_T)));
		RtlMultiByteToUnicodeN(UnicodeFile.get(), AllocatedBytes, &BytesInUnicodeString, (PSTR)Buffer.get(), Size);
		GetLines(FileList, UnicodeFile.get(), BytesInUnicodeString / 2);
	}
	break;

	case Encode::UNICODE_LE:
	{
		GetLines(FileList, (PWSTR)(Buffer.get() + 2), (Size - 2) / 2);
	}
	break;

	case Encode::UNICODE_BE:
	{
		for (ULONG i = 0; i < Size / 2 - 1; i++)
		{
			CurrentByte         = Buffer.get()[i];
			Buffer.get()[i]     = Buffer.get()[i + 1];
			Buffer.get()[i + 1] = CurrentByte;
		}

		GetLines(FileList, (PWSTR)(Buffer.get() + 2), (Size - 2) / 2);
	}
	break;

	case Encode::UTF8:
	{
		AllocatedBytes       = ROUND_UP((Size + 1) * sizeof(WCHAR), sizeof(SIZE_T));
		BytesInUnicodeString = 0;
		if (Size <= 3)
			return STATUS_BUFFER_TOO_SMALL;

		auto UnicodeFile  = AllocateMemorySafeP<WCHAR>(AllocatedBytes);
		RtlUTF8ToUnicodeN(UnicodeFile.get(), AllocatedBytes, &BytesInUnicodeString, (PSTR)Buffer.get() + 3, Size - 3);
		GetLines(FileList, UnicodeFile.get(), BytesInUnicodeString / 2);
	}
	break;

	case Encode::UTF8_NOBOM:
	{
		PWSTR UnicodeFile = (PWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ROUND_UP((Size + 1) * 2, 4));
		ULONG UnicodeStringActualByteCount = 0;
		RtlUTF8ToUnicodeN(UnicodeFile, ROUND_UP((Size + 1) * 2, 4), &UnicodeStringActualByteCount, (PSTR)Buffer.get(), Size);
		GetLines(FileList, (PWSTR)UnicodeFile, UnicodeStringActualByteCount / 2);
	}
	break;
	}

	return TRUE;
}


NTSTATUS DumpFileCommandPlugin::DetectEncode(PBYTE Buffer, ULONG Size, Encode& TextEncode)
{
	NTSTATUS Status;

	
	if (Size >= 2 && Buffer[0] == 0xFF && Buffer[1] == 0xFE) 
	{
		TextEncode = Encode::UNICODE_LE;
		return STATUS_SUCCESS;
	}

	if (Size >= 2 && Buffer[0] == 0xFE && Buffer[1] == 0xFF) {
		TextEncode = Encode::UNICODE_LE;
		return STATUS_SUCCESS;
	}

	if (Size >= 3 && Buffer[0] == 0xEF && Buffer[1] == 0xBB && Buffer[2] == 0xBF)
	{
		TextEncode = Encode::UTF8;
		return STATUS_SUCCESS;
	}

	Status = CheckUtf8WithoutBOM(Buffer, Size);
	if (NT_SUCCESS(Status))
	{
		TextEncode = Encode::UTF8_NOBOM;
		return Status;
	}

	TextEncode = Encode::ANSI;
	return STATUS_SUCCESS;
}


NTSTATUS DumpFileCommandPlugin::CheckUtf8WithoutBOM(PBYTE Buffer, ULONG Size)
{
	if (UtilUtf8ToWideCharString((const char*)Buffer, nullptr) == -1)
		return STATUS_NO_MATCH;
	
	return STATUS_SUCCESS;
}

