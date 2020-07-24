#pragma once

#include <my.h>

enum class Signals : ULONG_PTR
{
	//
	// nop
	//

	Signal_Nop = 0,

	//
	// Hooks
	//

	Signal_Unhook_LoadLibraryA = 1,
	Signal_Unhook_LoadLibraryW = 2,
	Signal_Unhook_TVPGetFunctionExporter = 3,
	Signal_Unhook_V2Link = 4,
	Signal_Unhook_TVPSetXP3ArchiveExtractionFilter = 5,
	Signal_Unhook_GetProcAddress = 6,
	Signal_Unhook_CreateProcessInternalW = 7,
	Signal_Unhook_MultiByteToWideChar = 8,
	Signal_Unhook_CreateFileW = 9,
	Signal_Unhook_ReadFile = 10,
	Signal_Unhook_IsDebuggerPresent = 11,

	//
	// Hardware
	//

	Signal_Remove_AllHardwareBreakPoint = 100,
	Signal_Remove_HardwareBreakPoint_0  = 101,
	Signal_Remove_HardwareBreakPoint_1  = 102,
	Signal_Remove_HardwareBreakPoint_2  = 103,
	Signal_Remove_HardwareBreakPoint_3  = 104,

	Signal_Remove_PrivateProcHandler = 200,


	//
	// Systems
	//
	Signal_Terminal = 5000,

	//
	// Continuous
	//

	Signal_DetectEngineVersion_Dyn = 6000,
	

	//
	// Last
	//
	_Signal_Invalid
};

namespace Signal {
	
struct Request 
{
	Signals Op     = Signals::Signal_Nop;
	HANDLE  Handle = INVALID_HANDLE_VALUE;
	PVOID   PrivateData = nullptr;
};

DWORD NTAPI SignalThread(PVOID* Param);

};
