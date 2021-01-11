#pragma once

#include "NativeThread.h"
#include "AlpcRev.h"
#include <atomic>
#include <string>
#include "PrivateStub.h"
#include "Alpc.h"

class RevThread final : public NativeThread
{
public:
	RevThread() = delete;
	RevThread(const RevThread&) = delete;
	RevThread(
		AlpcRev*                       Instance, 
		ULONG                          ShakehandTimeout, 
		ULONG                          Secret
	);

	~RevThread();

	BOOL   IsAlpcServerReady();
	BOOL   ParseMessage(PVOID MessageBuffer, SIZE_T MessageSize);
	BOOL   SendMessageToRemote(ULONG MessageType, std::string&& Request);
	BOOL   SendMessageToRemote(ULONG MessageType, std::string&  Request);
	BOOL   Stop();
	BOOL   CreateAlpcServer();

private:
	void ThreadFunction();

	//
	// Call this function only if
	// the remote server is able to launch everything (not in loader)
	// Sub-process will inherit environment variables
	//
	NTSTATUS ValidateHandleShakeMessage(PVOID MessageBuffer, SIZE_T MessageSize);

	AlpcRev*                       m_Instance       = nullptr;
	ULONG                          m_ShakehandTimeout;
	ULONG                          m_Secret = 0;
	ALPCServer*                    m_Server          = nullptr;
	std::atomic<ULONG>             m_ReadyClientPid  = 0;
};


