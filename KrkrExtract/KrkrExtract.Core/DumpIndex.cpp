#include "DebuggerHandler.h"
#include "XP3Parser.h"
#include <Utf.Convert.h>

import Xp3Parser;

DumpIndexCommandPlugin::DumpIndexCommandPlugin(KrkrClientProxyer* Proxyer):
	m_Proxyer(Proxyer)
{
}

DumpIndexCommandPlugin::~DumpIndexCommandPlugin()
{
	m_Proxyer = nullptr;
}

PCWSTR NTAPI DumpIndexCommandPlugin::GetCommandName()
{
	return L"di";
}

ArgumentParser* DumpIndexCommandPlugin::PrepareParser()
{
	ArgumentParser* Parser;

	Parser = new (std::nothrow) ArgumentParser("di", "dump archive index");
	if (!Parser)
		return nullptr;
	
	Parser->add_argument("-n", "--name", "name", true)
		.position(ArgumentParser::Argument::Position::LAST);

	return Parser;
}


void DumpIndexCommandPlugin::FreeParser(ArgumentParser* Parser)
{
	if (Parser) {
		delete Parser;
	}
}

BOOL NTAPI DumpIndexCommandPlugin::Exec(PWSTR Command)
{
	NTSTATUS        Status;
	LONG_PTR        Argc;
	Xp3WalkerProxy  Xp3Proxy;
	DWORD           M2ChunkMagic;
	std::wstring    Error, ArchiveName;


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
		if (!Parser->exists("name"))
			break;

		ArchiveName = Utf8ToUtf16(Parser->get<std::string>("name"));

		Status = WalkXp3ArchiveIndex(
			m_Proxyer,
			ArchiveName.c_str(),
			Xp3Proxy,
			M2ChunkMagic,

			//
			// Unknown
			//

			WalkXp3IndexCallbackM(KrkrProxyer, Buffer, Size, File, Magic, Proxyer)
			{
				return SaveIndex(ArchiveName.c_str(), Buffer, (ULONG)Size);
			},

			//
			// Normal
			//

			WalkXp3IndexCallbackM(KrkrProxyer, Buffer, Size, File, Magic, Proxyer)
			{
				return SaveIndex(ArchiveName.c_str(), Buffer, (ULONG)Size);
			},

			//
			// sb
			//

			WalkXp3IndexCallbackM(KrkrProxyer, Buffer, Size, File, Magic, Proxyer)
			{
				return SaveIndex(ArchiveName.c_str(), Buffer, (ULONG)Size);
			},

			//
			// sb V2
			//

			WalkXp3IndexCallbackM(KrkrProxyer, Buffer, Size, File, Magic, Proxyer)
			{
				return SaveIndex(ArchiveName.c_str(), Buffer, (ULONG)Size);
			},

			//
			// Neko vol0
			//

			WalkXp3IndexCallbackM(KrkrProxyer, Buffer, Size, File, Magic, Proxyer)
			{
				return SaveIndex(ArchiveName.c_str(), Buffer, (ULONG)Size);
			},

			//
			// M2 Krkrz
			//

			WalkXp3IndexCallbackM(KrkrProxyer, Buffer, Size, File, Magic, Proxyer)
			{
				return SaveIndex(ArchiveName.c_str(), Buffer, (ULONG)Size);
			}
		);
	}

	return NT_SUCCESS(Status);
}

BOOL NTAPI DumpIndexCommandPlugin::Help(StringView* HelperString)
{
	if (HelperString) {
		HelperString->Assign(ShowHelp().c_str());
	}

	return HelperString && HelperString->Length() ? TRUE : FALSE;
}


NTSTATUS DumpIndexCommandPlugin::SaveIndex(PCWSTR FileName, PBYTE Buffer, ULONG Size)
{
	NTSTATUS   Status;
	NtFileDisk File;
	WCHAR      IndexName[MAX_NTPATH];

	wnsprintfW(IndexName, countof(IndexName), L"%s.index", FileName);
	Status = File.Create(IndexName);
	if (NT_FAILED(Status))
		return Status;

	Status = File.Write(Buffer, Size);
	if (NT_FAILED(Status))
		return Status;

	return File.Close();
}

