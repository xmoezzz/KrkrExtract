#include "DebuggerHandler.h"
#include <Utf.Convert.h>


std::shared_ptr<PSTR> CommandWcharToUt8::CmdLineToArgvWToA(PCWSTR CommandLine, PLONG_PTR Argc)
{
	LONG_PTR argc, CmdLineLength;
	LPSTR *argv;

	auto CommandLineUtf8 = Utf16ToUtf8(CommandLine);
	auto pszCmdLine = (PSTR)CommandLineUtf8.c_str();

	argc = CmdLineToArgvWorkerA(pszCmdLine, NULL, &CmdLineLength);
	argv = (LPSTR *)AllocateMemory(CmdLineLength * sizeof(*pszCmdLine) + (argc + 1) * 4);
	if (argv == NULL)
		return NULL;

	CopyMemory(&argv[argc + 1], pszCmdLine, CmdLineLength * sizeof(*pszCmdLine));
	argv[argc] = 0;
	pszCmdLine = (LPSTR)&argv[argc + 1];

	argc = CmdLineToArgvWorkerA(pszCmdLine, argv, NULL);
	if (Argc) {
		*Argc = argc;
	}

	return std::shared_ptr<PSTR>(
		argv,
		[](void* Ptr)
	{
		FreeArgv(Ptr);
	});
}


VOID CommandWcharToUt8::FreeArgv(void* Argv)
{
	if (Argv) {
		FreeMemory(Argv);
	}
}
