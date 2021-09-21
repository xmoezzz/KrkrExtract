#pragma once

#include <grpcpp/grpcpp.h>
#include <server.grpc.pb.h>
#include <Stubs.h>
#include <Utf.Convert.h>
#include <memory>
#include <my.h>


using grpc::Channel;
using grpc::ClientContext;
using krserver::KrConnectionApi;
using krserver::ServerProgressBarRequest;
using krserver::ServerProgressBarResponse;
using krserver::ServerLogOutputRequest;
using krserver::ServerLogOutputResponse;
using krserver::ServerCommandResultOutputRequest;
using krserver::ServerCommandResultOutputResponse;
using krserver::ServerUIReadyRequest;
using krserver::ServerUIReadyResponse;
using krserver::ServerMessageBoxRequest;
using krserver::ServerMessageBoxResponse;
using krserver::ServerTaskStartAndDisableUIRequest;
using krserver::ServerTaskStartAndDisableUIResponse;
using krserver::ServerTaskEndAndEnableUIRequest;
using krserver::ServerTaskEndAndEnableUIResponse;
using krserver::ServerUIHeartbeatPackageRequest;
using krserver::ServerUIHeartbeatPackageResponse;
using krserver::ServerExitFromRemoteProcessRequest;
using krserver::ServerExitFromRemoteProcessResponse;


class ConnectionApi : public ServerStub 
{
public:

	ConnectionApi(std::shared_ptr<Channel> channel)
		: Stub(KrConnectionApi::NewStub(channel)) {}

	BOOL NotifyServerProgressBar(
		_In_ PCWSTR    TaskName,
		_In_ ULONGLONG Current,
		_In_ ULONGLONG Total
	) {
		ServerProgressBarRequest Request;
		ServerProgressBarResponse Response;
		ClientContext Context;

		Request.set_taskname(Utf16ToUtf8(TaskName));
		Request.set_current(Current);
		Request.set_total(Total);

		SetContextTimeout(Context);
		auto Status = Stub->ServerProgressBar(&Context, Request, &Response);
		if (Status.ok()) {

			if (Response.message().length()) {
				auto&& Message = Utf8ToUtf16(Response.message());
				PrintConsoleW(L"ConnectionApi::NotifyServerProgressBar : %s\n", Message.c_str());
			}
			return Response.success();
		}

		PrintConsoleW(L"ConnectionApi::NotifyServerProgressBar : failed. [%d]\n", Status.error_code());
		PrintConsoleW(L"%s\n", Utf8ToUtf16(Status.error_message()).c_str());
		return FALSE;
 	}

	BOOL NotifyServerLogOutput(
		_In_ LogLevel Level,
		_In_ PCWSTR Command
	) {
		ServerLogOutputRequest Request;
		ServerLogOutputResponse Response;
		ClientContext Context;

		Request.set_level((UInt64)Level);
		Request.set_command(Utf16ToUtf8(Command));

		SetContextTimeout(Context);
		auto Status = Stub->ServerLogOutput(&Context, Request, &Response);
		if (Status.ok()) {

			if (Response.message().length()) {
				auto&& Message = Utf8ToUtf16(Response.message());
				PrintConsoleW(L"ConnectionApi::NotifyServerLogOutput : %s\n", Message.c_str());
			}
			return Response.success();
		}

		PrintConsoleW(L"ConnectionApi::NotifyServerLogOutput : failed. [%d]\n", Status.error_code());
		PrintConsoleW(L"%s\n", Utf8ToUtf16(Status.error_message()).c_str());
		return FALSE;
	}

	BOOL NotifyServerCommandResultOutput(
		_In_  CommandStatus CmdStatus,
		_In_  PCWSTR Reply
	) {
		ServerCommandResultOutputRequest Request;
		ServerCommandResultOutputResponse Response;
		ClientContext Context;

		Request.set_status((UInt64)CmdStatus);
		Request.set_reply(Utf16ToUtf8(Reply));

		SetContextTimeout(Context);
		auto Status = Stub->ServerCommandResultOutput(&Context, Request, &Response);
		if (Status.ok()) {

			if (Response.message().length()) {
				auto&& Message = Utf8ToUtf16(Response.message());
				PrintConsoleW(L"ConnectionApi::ServerCommandResultOutput : %s\n", Message.c_str());
			}
			return Response.success();
		}

		PrintConsoleW(L"ConnectionApi::ServerCommandResultOutput : failed. [%d]\n", Status.error_code());
		PrintConsoleW(L"%s\n", Utf8ToUtf16(Status.error_message()).c_str());
		return FALSE;
	}

	BOOL NotifyServerUIReady(
		ULONG ClientPort,
		PCSTR SessionKey,
		ULONG Extra
	) {
		ServerUIReadyRequest Request;
		ServerUIReadyResponse Response;
		ClientContext Context;

		Request.set_clientport(ClientPort);
		Request.set_sessionkey(SessionKey);
		Request.set_extra(Extra);

		SetContextTimeout(Context);
		auto Status = Stub->ServerUIReady(&Context, Request, &Response);
		if (Status.ok()) {

			if (Response.message().length()) {
				auto&& Message = Utf8ToUtf16(Response.message());
				PrintConsoleW(L"ConnectionApi::ServerUIReady : %s\n", Message.c_str());
			}
			Response.heartbeatinterval();

			return Response.success();
		}

		PrintConsoleW(L"ConnectionApi::ServerUIReady : failed. [%d]\n", Status.error_code());
		PrintConsoleW(L"%s\n", Utf8ToUtf16(Status.error_message()).c_str());
		return FALSE;
	}

	BOOL NotifyServerMessageBox(
		_In_ PCWSTR Description,
		_In_ ULONG Flags,
		_In_ BOOL Locked
	) {
		ServerMessageBoxRequest Request;
		ServerMessageBoxResponse Response;
		ClientContext Context;

		Request.set_description(Utf16ToUtf8(Description));
		Request.set_flags(Flags);
		Request.set_locked(Locked);

		SetContextTimeout(Context);
		auto Status = Stub->ServerMessageBox(&Context, Request, &Response);
		if (Status.ok()) {

			if (Response.message().length()) {
				auto&& Message = Utf8ToUtf16(Response.message());
				PrintConsoleW(L"ConnectionApi::ServerMessageBox : %s\n", Message.c_str());
			}
			return Response.success();
		}

		PrintConsoleW(L"ConnectionApi::ServerMessageBox : failed. [%d]\n", Status.error_code());
		PrintConsoleW(L"%s\n", Utf8ToUtf16(Status.error_message()).c_str());
		return FALSE;
	}

	BOOL NotifyServerTaskStartAndDisableUI() {

		ServerTaskStartAndDisableUIRequest Request;
		ServerTaskStartAndDisableUIResponse Response;
		ClientContext Context;

		SetContextTimeout(Context);
		auto Status = Stub->ServerTaskStartAndDisableUI(&Context, Request, &Response);
		if (Status.ok()) {

			if (Response.message().length()) {
				auto&& Message = Utf8ToUtf16(Response.message());
				PrintConsoleW(L"ConnectionApi::ServerTaskStartAndDisableUI : %s\n", Message.c_str());
			}
			return Response.success();
		}

		PrintConsoleW(L"ConnectionApi::ServerTaskStartAndDisableUI : failed. [%d]\n", Status.error_code());
		PrintConsoleW(L"%s\n", Utf8ToUtf16(Status.error_message()).c_str());
		return FALSE;
	}

	BOOL NotifyServerTaskEndAndEnableUI(
		_In_ BOOL TaskCompleteStatus,
		_In_ PCWSTR Description
	) {
		ServerTaskEndAndEnableUIRequest Request;
		ServerTaskEndAndEnableUIResponse Response;
		ClientContext Context;

		Request.set_taskcompletestatus(TaskCompleteStatus);
		Request.set_description(Utf16ToUtf8(Description));

		SetContextTimeout(Context);
		auto Status = Stub->ServerTaskEndAndEnableUI(&Context, Request, &Response);
		if (Status.ok()) {

			if (Response.message().length()) {
				auto&& Message = Utf8ToUtf16(Response.message());
				PrintConsoleW(L"ConnectionApi::ServerTaskEndAndEnableUI : %s\n", Message.c_str());
			}
			return Response.success();
		}

		PrintConsoleW(L"ConnectionApi::ServerTaskEndAndEnableUI : failed. [%d]\n", Status.error_code());
		PrintConsoleW(L"%s\n", Utf8ToUtf16(Status.error_message()).c_str());
		return FALSE;
	}

	BOOL NotifyServerUIHeartbeatPackage() {

		ServerUIHeartbeatPackageRequest Request;
		ServerUIHeartbeatPackageResponse Response;
		ClientContext Context;

		SetContextTimeout(Context);
		auto Status = Stub->ServerUIHeartbeatPackage(&Context, Request, &Response);
		if (Status.ok()) {
			return TRUE;
		}

		PrintConsoleW(L"ConnectionApi::NotifyServerUIHeartbeatPackage : failed. [%d]\n", Status.error_code());
		PrintConsoleW(L"%s\n", Utf8ToUtf16(Status.error_message()).c_str());
		return FALSE;
	}

	BOOL NotifyServerExitFromRemoteProcess() {

		ServerExitFromRemoteProcessRequest Request;
		ServerExitFromRemoteProcessResponse Response;
		ClientContext Context;

		SetContextTimeout(Context);
		auto Status = Stub->ServerExitFromRemoteProcess(&Context, Request, &Response);
		if (Status.ok()) {

			if (Response.message().length()) {
				auto&& Message = Utf8ToUtf16(Response.message());
				PrintConsoleW(L"ConnectionApi::ServerExitFromRemoteProcess : %s\n", Message.c_str());
			}
			return Response.success();
		}

		PrintConsoleW(L"ConnectionApi::NotifyServerExitFromRemoteProcess : failed. [%d]\n", Status.error_code());
		PrintConsoleW(L"%s\n", Utf8ToUtf16(Status.error_message()).c_str());
		return FALSE;
	}

	std::uint32_t GetHeartbeatInterval() {
		return HeartbeatInterval;
	}

private:
	void SetContextTimeout(ClientContext& Context) {

		// Connection timeout in seconds
		ULONG ClientConnectionTimeout = 2;

		std::chrono::system_clock::time_point deadline =
			std::chrono::system_clock::now() + std::chrono::seconds(ClientConnectionTimeout);

		Context.set_deadline(deadline);
	}

	std::unique_ptr<KrConnectionApi::Stub> Stub;
	std::uint32_t                          HeartbeatInterval = 5000;
};

