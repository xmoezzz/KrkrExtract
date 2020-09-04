#include "DebuggerHandler.h"
#include <Utf.Convert.h>

std::shared_ptr<ArgumentParser> CommandParser::ParseCommandLine(PCWSTR CommandLine, std::wstring& Error)
{
	LONG_PTR               Argc;
	ArgumentParser::Result Result;
	ArgumentParser*        Parser;

	Argc  = 0;
	auto Argv  = CmdLineToArgvWToA(CommandLine, &Argc);
	Error = {};
		
	Parser = PrepareParser();
	if (!Parser)
		return nullptr;

	Result = Parser->parse(Argc, (const char**)Argv.get());
	if (Result)
	{
		Error = Utf8ToUtf16(Result.what());
		FreeArgv(Parser);
		return nullptr;
	}

	return std::shared_ptr<ArgumentParser>(
		Parser,
		[this](ArgumentParser* Ptr)
		{
			FreeParser(Ptr);
		}
	);
}

std::wstring CommandParser::ShowHelp()
{
	ArgumentParser* Parser = PrepareParser();

	if (Parser) 
	{
		auto Help = Utf8ToUtf16(Parser->print_help());
		FreeParser(Parser);
		return Help;
	}

	return {};
}

