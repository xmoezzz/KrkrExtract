#pragma once

#include "NativeThread.h"
#include "AlpcRev.h"
#include <atomic>
#include <string>
#include "PrivateStub.h"

class RevThread final : public NativeThread
{
public:
	RevThread() = delete;
	RevThread(const RevThread&) = delete;
	RevThread(
		AlpcRev*                       Instance, 
		PCWSTR                         IoPrivatePath, 
		ULONG                          ShakehandTimeout, 
		ULONG                          Secret,
		NotifyServerRaiseErrorCallback NotifyServerRaiseError
	);

	~RevThread();

	BOOL   IsAlpcServerReady();
	BOOL   ParseMessage(PVOID MessageBuffer, SIZE_T MessageSize);
	BOOL   SendMessageToRemote(ULONG MessageType, std::string&& Request);
	BOOL   SendMessageToRemote(ULONG MessageType, std::string&  Request);
	HANDLE GetRemoteProcess();
	BOOL   CloseRemoteProcess();
	BOOL   Stop();

private:
	void ThreadFunction();
	BOOL IsRemoteProcessAlive();

	//
	// Call this function only if
	// the remote server is able to launch everything (not in loader)
	// Sub-process will inherit environment variables
	//
	NTSTATUS ValidateHandleShakeMessage(PVOID MessageBuffer, SIZE_T MessageSize);

	AlpcRev*                       m_Instance       = nullptr;
	HANDLE                         m_AlpcServerPort = nullptr;
	HANDLE                         m_AlpcClientPort = nullptr;
	HANDLE                         m_RemoteProcess  = nullptr;
	ULONG                          m_ShakehandTimeout;
	ULONG                          m_Secret = 0;
	std::wstring                   m_RpcPath;
	std::atomic<BOOL>              m_PortIsReady = FALSE;
	NotifyServerRaiseErrorCallback m_NotifyServerRaiseErrorStub = nullptr;
};


