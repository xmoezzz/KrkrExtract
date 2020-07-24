#pragma once

#include <my.h>
#include <Stubs.h>

enum class RaiseErrorType
{
	RAISE_ERROR_HEARTBEAT_TIMEOUT = 0,
	RAISE_ERROR_REMOTE_CRASH = 1,
	RAISE_ERROR_SECRET_DISMATCH = 2,
	RAISE_ERROR_INVALID_PID = 3,
	RAISE_ERROR_REMOTE_DEAD = 4,
	RAISE_ERROR_REMOTE_PRIVILEGED = 5,
	RAISE_ERROR_REMOTE_GENEROUS = 6
};

using NotifyServerProgressChangedCallback = void (NTAPI*)(PCWSTR, ULONG64, ULONG64);
using NotifyServerLogOutputCallback = void (NTAPI*)(LogLevel, PCWSTR, BOOL IsCmd);
using NotifyServerUIReadyCallback = void (NTAPI*)();
using NotifyServerMessageBoxCallback = void (NTAPI*)(PCWSTR, ULONG, BOOL);
using NotifyServerTaskStartAndDisableUICallback = void (NTAPI*)();
using NotifyServerTaskEndAndEnableUICallback = void (NTAPI*)(BOOL, PCWSTR);
using NotifyServerExitFromRemoteProcessCallback = void (NTAPI*)();

//
// Handle this error
// TRUE  : continue execution
// FALSE : exit now
//
using NotifyServerRaiseErrorCallback = BOOL(NTAPI*)(RaiseErrorType ErrorType, PCWSTR Error);




