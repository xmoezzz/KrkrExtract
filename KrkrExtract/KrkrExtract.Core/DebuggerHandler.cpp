#include "DebuggerHandler.h"
#include "KrkrExtract.h"
#include <Utf.Convert.h>

NTSTATUS KrkrExtractCore::InitializeCommandUtils()
{
	std::string       KeyName;
	std::vector<char> KeyList;

	static auto BuiltinCommandUtils = 
	{ 
		&DumpIndexCommandPlugin::CreateInstance, 
		&DumpFileCommandPlugin::CreateInstance 
	};

	for (auto CreateInstanceBuilder : BuiltinCommandUtils)
	{
		auto Plugin = CreateInstanceBuilder(this);
		if (!Plugin)
			continue;

		KeyName = Utf16ToUtf8(Plugin->GetCommandName());
		KeyList.clear();
		std::copy(KeyName.begin(), KeyName.end(), std::back_inserter(KeyList));

		m_CommandBuilder.add(
			KeyList, 
			std::shared_ptr<CommandPlugin>(
				Plugin
				)
			);
	}
	
	return STATUS_SUCCESS;
}


NTSTATUS KrkrExtractCore::ParseUtilCommand(PWSTR Command)
{
	LONG_PTR              Argc;
	std::vector<char>     KeyList;

	auto Argv = CommandWcharToUt8::CmdLineToArgvWToA(Command, &Argc);
	if (!Argv)
		return STATUS_NO_MEMORY;

	if (Argv <= 0)
		return STATUS_INVALID_PARAMETER;

	auto KeyName = std::string(Argv.get()[0]);
	std::copy(KeyName.begin(), KeyName.end(), std::back_inserter(KeyList));

	LOOP_ONCE
	{
		auto PluginNode = m_CommandBuilder.get(KeyList);
		if (!PluginNode)
			break;

		auto Plugin = PluginNode.get();
		if (!Plugin)
			break;

		if (!Plugin->get())
			break;

		return Plugin->get()->Exec(Command) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
	};

	TellServerCommandResultOutput(
		CommandStatus::COMMAND_WARN,
		L"Unsupported command : %s",
		Utf8ToUtf16(KeyName).c_str()
	);

	return STATUS_UNSUCCESSFUL;
}

