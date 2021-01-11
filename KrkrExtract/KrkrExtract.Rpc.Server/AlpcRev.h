#pragma once

#include <my.h>
#include <RpcDefine.h>
#include "PrivateStub.h"

class EventMsg
{
public:
	enum class MsgKind : ULONG {
		M_Start = 0,
		M_NotifyServerProgressChanged = 1,
		M_NotifyServerLogOutput = 2,
		M_NotifyServerUIReady  = 3,
		M_NotifyServerMessageBox = 4,
		M_NotifyServerTaskStartAndDisableUI = 5,
		M_NotifyServerTaskEndAndEnableUI = 6,
		M_NotifyServerExitFromRemoteProcess = 7,
		M_NotifyServerRaiseError = 8,
		M_Last = 9
	};

	virtual MsgKind GetMessageKind() const = 0;
};

class NotifyServerProgressChangedMsg : public EventMsg
{
public:
	NotifyServerProgressChangedMsg() = delete;
	NotifyServerProgressChangedMsg(ULONGLONG Current, ULONGLONG Total, PCWSTR Task) noexcept {
		this->Current = Current;
		this->Total   = Total;
		this->TaskName = Task;
	}

	MsgKind GetMessageKind() const {
		return EventMsg::MsgKind::M_NotifyServerExitFromRemoteProcess;
	}

	ULONGLONG GetCurrent() const {
		return Current;
	}

	ULONGLONG GetTotal() const {
		return Total;
	}

	std::wstring GetTaskName() const {
		return TaskName;
	}

	NotifyServerProgressChangedMsg& operator = (const NotifyServerProgressChangedMsg& Other) {
		Current  = Other.GetCurrent();
		Total    = Other.GetTotal();
		TaskName = std::move(Other.GetTaskName());

		return *this;
	}

private:
	ULONGLONG     Current;
	ULONGLONG     Total;
	std::wstring  TaskName;
};

class NotifyServerLogOutputMsg : public EventMsg 
{
public:
	NotifyServerLogOutputMsg() = delete;
	NotifyServerLogOutputMsg(LogLevel Level, PCWSTR Message, BOOL IsCmd) noexcept {
		this->Level   = Level;
		this->Message = Message;
		this->IsCmd   = IsCmd;
	}

	MsgKind GetMessageKind() const {
		return EventMsg::MsgKind::M_NotifyServerLogOutput;
	}

	LogLevel GetLevel() const {
		return Level;
	}

	std::wstring GetMessage() const {
		return Message;
	}

	BOOL GetIsCmd() const {
		return IsCmd;
	}

	NotifyServerLogOutputMsg& operator = (const NotifyServerLogOutputMsg& Other) {
		Level   = Other.GetLevel();
		IsCmd   = Other.GetIsCmd();
		Message = std::move(Other.GetMessage());

		return *this;
	}

private:
	LogLevel       Level;
	std::wstring   Message;
	BOOL           IsCmd;
};


class NotifyServerUIReadyMsg : public EventMsg 
{
public:
	NotifyServerUIReadyMsg() noexcept {

	}

	MsgKind GetMessageKind() const {
		return EventMsg::MsgKind::M_NotifyServerUIReady;
	}
};

class NotifyServerMessageBoxMsg : public EventMsg 
{
public:
	NotifyServerMessageBoxMsg() = delete;
	NotifyServerMessageBoxMsg(PCWSTR Description, ULONG Flags, BOOL Locked) noexcept {
		this->Description = Description;
		this->Flags       = Flags;
		this->Locked      = Locked;
	}

	MsgKind GetMessageKind() const {
		return EventMsg::MsgKind::M_NotifyServerMessageBox;
	}

	std::wstring GetDescription() const {
		return Description;
	}

	BOOL GetFlags() const {
		return Flags;
	}

	BOOL GetLocked() const {
		return Locked;
	}

	NotifyServerMessageBoxMsg& operator = (const NotifyServerMessageBoxMsg& Other) {
		Description = std::move(Other.GetDescription());
		Flags       = Other.GetFlags();
		Locked      = Other.GetLocked();

		return *this;
	}
	
private:
	std::wstring Description;
	BOOL         Flags;
	BOOL         Locked;
};

class NotifyServerTaskStartAndDisableUIMsg : public EventMsg 
{
public:
	NotifyServerTaskStartAndDisableUIMsg() noexcept {
		
	}

	MsgKind GetMessageKind() const {
		return EventMsg::MsgKind::M_NotifyServerTaskStartAndDisableUI;
	}
};


class NotifyServerTaskEndAndEnableUIMsg : public EventMsg 
{
public:
	NotifyServerTaskEndAndEnableUIMsg() noexcept {

	}

	MsgKind GetMessageKind() const {
		return EventMsg::MsgKind::M_NotifyServerTaskEndAndEnableUI;
	}
};

class NotifyServerExitFromRemoteProcessMsg : public EventMsg 
{
public:
	NotifyServerExitFromRemoteProcessMsg() noexcept {

	}

	MsgKind GetMessageKind() const {
		return EventMsg::MsgKind::M_NotifyServerExitFromRemoteProcess;
	}
};


class NotifyServerRaiseErrorMsg : public EventMsg
{
public:
	NotifyServerRaiseErrorMsg() = delete;
	NotifyServerRaiseErrorMsg(RaiseErrorType ErrorType, PCWSTR Error) noexcept {
		this->ErrorType = ErrorType;
		this->Error     = Error;
	}

	MsgKind GetMessageKind() const {
		return EventMsg::MsgKind::M_NotifyServerRaiseError;
	}

	RaiseErrorType GetErrorType() const {
		return ErrorType;
	}

	std::wstring GetError() const {
		return Error;
	}

	NotifyServerRaiseErrorMsg& operator = (const NotifyServerRaiseErrorMsg& Other) {
		ErrorType = Other.GetErrorType();
		Error     = std::move(Other.GetError());

		return *this;
	}

private:
	RaiseErrorType ErrorType;
	std::wstring   Error;
};

class AlpcRev
{
public:
	virtual BOOL ParseMessage(PVOID MessageBuffer, SIZE_T MessageSize) = 0;
};


