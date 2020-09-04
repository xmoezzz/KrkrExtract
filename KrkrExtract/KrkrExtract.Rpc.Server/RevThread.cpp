#include "RevThread.h"
#include "Console.h"
#include <RpcSequence.h>

RevThread::RevThread(
	AlpcRev*                       Instance, 
	PCWSTR                         IoPrivatePath, 
	ULONG                          ShakehandTimeout,
	ULONG                          Secret,
	NotifyServerRaiseErrorCallback NotifyServerRaiseError
) :
	m_Instance(Instance),
	m_ShakehandTimeout(ShakehandTimeout),
	m_Secret(Secret),
	m_NotifyServerRaiseErrorStub(NotifyServerRaiseError)
{
	WCHAR  Path[0x200];

	wsprintfW(Path, L"\\RPC Control\\%s", IoPrivatePath);
	m_RpcPath        = Path;
	m_AlpcClientPort = nullptr;
}

RevThread::~RevThread()
{
	Stop();
}

BOOL RevThread::CloseRemoteProcess()
{
	NTSTATUS                  Status;
	PROCESS_BASIC_INFORMATION BasicInfo;

	if (!m_RemoteProcess)
		return TRUE;

	Status = NtQueryInformationProcess(m_RemoteProcess, ProcessBasicInformation, &BasicInfo, sizeof(BasicInfo), nullptr);
	if (NT_FAILED(Status) || BasicInfo.ExitStatus != STATUS_PENDING)
		return FALSE;

	NtTerminateProcess(m_RemoteProcess, 0);
	NtClose(m_RemoteProcess);
	m_RemoteProcess = nullptr;
	return TRUE;
}

BOOL RevThread::Stop()
{
	CloseRemoteProcess();

	if (m_AlpcClientPort) {
		NtAlpcDisconnectPort(m_AlpcClientPort, 0);
	}
	m_AlpcClientPort = nullptr;

	if (m_AlpcServerPort) {
		NtClose(m_AlpcServerPort);
	}
	m_AlpcServerPort = nullptr;
	m_Instance       = nullptr;
	
	return TRUE;
}


BOOL RevThread::IsAlpcServerReady()
{
	return m_PortIsReady.load();
}

BOOL RevThread::ParseMessage(PVOID MessageBuffer, SIZE_T MessageSize)
{
	return m_Instance->ParseMessage(MessageBuffer, MessageSize);
}

#pragma pack(4)
struct HandShakeRequest
{
	UINT32 Secret;
	UINT32 RemoteProcessPid;
};
#pragma pack()


BOOL NTAPI NotifyServerRaiseErrorDefault(RaiseErrorType ErrorType, PCWSTR Error)
{
	NtPrintConsole(L"%s\n", Error);
	return FALSE;
}

HANDLE RevThread::GetRemoteProcess()
{
	return m_RemoteProcess;
}

NTSTATUS RevThread::ValidateHandleShakeMessage(PVOID MessageBuffer, SIZE_T MessageSize)
{
	NTSTATUS                  Status;
	HANDLE                    RemoteProcess;
	OBJECT_ATTRIBUTES         ObjectAttribute;
	CLIENT_ID                 ClientID;
	PROCESS_BASIC_INFORMATION BasicInfo;
	HandShakeRequest*         Request;

	if (MessageSize < sizeof(HandShakeRequest))
		return FALSE;

	Request = (HandShakeRequest*)MessageBuffer;
	if (Request->Secret != m_Secret)
		return FALSE;

	if (!m_NotifyServerRaiseErrorStub)
		m_NotifyServerRaiseErrorStub = NotifyServerRaiseErrorDefault;
	
	if (Request->RemoteProcessPid == GetCurrentProcessId())
	{
		if (!m_NotifyServerRaiseErrorStub(RaiseErrorType::RAISE_ERROR_INVALID_PID, L"Remote Process : Invalid pid")) {
			Ps::ExitProcess(0);
		}
	}

	ClientID.UniqueThread  = nullptr;
	ClientID.UniqueProcess = (HANDLE)Request->RemoteProcessPid;

	RtlZeroMemory(&ObjectAttribute, sizeof(ObjectAttribute));
	ObjectAttribute.Length = sizeof(ObjectAttribute);
	ObjectAttribute.Attributes = 0;

	Status = NtOpenProcess(&RemoteProcess, PROCESS_ALL_ACCESS, &ObjectAttribute, &ClientID);
	if (NT_FAILED(Status))
	{
		switch (Status)
		{
		case STATUS_ACCESS_DENIED:
			if (!m_NotifyServerRaiseErrorStub(RaiseErrorType::RAISE_ERROR_REMOTE_PRIVILEGED, L"Remote Process : access denied")) {
				Ps::ExitProcess(0);
			}
			break;

		default:
			if (!m_NotifyServerRaiseErrorStub(RaiseErrorType::RAISE_ERROR_REMOTE_GENEROUS, L"Remote Process : failed to lookup process")) {
				Ps::ExitProcess(0);
			}
			break;
		}
	}

	Status = NtQueryInformationProcess(RemoteProcess, ProcessBasicInformation, &BasicInfo, sizeof(BasicInfo), nullptr);
	if (NT_FAILED(Status) || BasicInfo.ExitStatus != STATUS_PENDING)
	{
		if (!m_NotifyServerRaiseErrorStub(RaiseErrorType::RAISE_ERROR_REMOTE_DEAD, L"Remote Process : failed to lookup process")) {
			Ps::ExitProcess(0);
		}
	}

	m_RemoteProcess = RemoteProcess;
	return TRUE;
}

template <class T> inline std::shared_ptr<T> AllocateMemorySafe(SIZE_T Size)
{
	return std::shared_ptr<T>(
		(T*)AllocateMemory(Size),
		[](T* Ptr)
	{
		if (Ptr) {
			FreeMemory(Ptr);
		}
	});
}

std::shared_ptr<PORT_MESSAGE> AllocateAlpcMessage(PPORT_MESSAGE PortMessage, ULONG MessageType, SIZE_T MessageSize, LPVOID Message)
{
	auto AlpcMessage = AllocateMemorySafe<PORT_MESSAGE>(MessageSize + sizeof(PORT_MESSAGE) + sizeof(MessageType));
	if (!AlpcMessage)
		return AlpcMessage;

	RtlCopyMemory(AlpcMessage.get(), PortMessage, sizeof(PORT_MESSAGE));
	RtlCopyMemory((PBYTE)AlpcMessage.get() + sizeof(PORT_MESSAGE), &MessageType, sizeof(UINT32));
	RtlCopyMemory((PBYTE)AlpcMessage.get() + sizeof(PORT_MESSAGE) + sizeof(UINT32), Message, MessageSize);

	return AlpcMessage;
}


BOOL RevThread::SendMessageToRemote(ULONG MessageType, std::string&& Request)
{
	NTSTATUS     Status;
	PORT_MESSAGE PortMessage;

	if (!IsAlpcServerReady())
		return FALSE;
	
	if (!m_AlpcClientPort)
		return FALSE;

	RtlZeroMemory(&PortMessage, 0, sizeof(PortMessage));

	PortMessage.u1.s1.DataLength = Request.length();
	PortMessage.u1.s1.TotalLength = Request.length() + sizeof(PortMessage);

	auto Message = AllocateAlpcMessage(&PortMessage, MessageType, Request.length(), (PVOID)Request.data());
	if (!Message)
		return STATUS_NO_MEMORY;

	Status = NtAlpcSendWaitReceivePort(m_AlpcClientPort, 0, Message.get(), NULL, NULL, NULL, NULL, NULL);
	return NT_SUCCESS(Status);
}

BOOL RevThread::SendMessageToRemote(ULONG MessageType, std::string& Request)
{
	NTSTATUS     Status;
	PORT_MESSAGE PortMessage;

	if (!IsAlpcServerReady())
		return FALSE;

	if (!m_AlpcClientPort)
		return FALSE;

	RtlZeroMemory(&PortMessage, 0, sizeof(PortMessage));

	PortMessage.u1.s1.DataLength = Request.length();
	PortMessage.u1.s1.TotalLength = Request.length() + sizeof(PortMessage);

	auto Message = AllocateAlpcMessage(&PortMessage, MessageType, Request.length(), (PVOID)Request.data());
	if (!Message)
		return STATUS_NO_MEMORY;

	Status = NtAlpcSendWaitReceivePort(m_AlpcClientPort, 0, Message.get(), NULL, NULL, NULL, NULL, NULL);
	return NT_SUCCESS(Status);
}

BOOL RevThread::IsRemoteProcessAlive()
{
	NTSTATUS                  Status;
	PROCESS_BASIC_INFORMATION BasicInfo;

	Status = NtQueryInformationProcess(m_RemoteProcess, ProcessBasicInformation, &BasicInfo, sizeof(BasicInfo), nullptr);
	if (NT_FAILED(Status) || BasicInfo.ExitStatus != STATUS_PENDING)
		return FALSE;

	return TRUE;
}


void RevThread::ThreadFunction()
{
	NTSTATUS             Status;
	PPORT_MESSAGE        ReceivedMessage;
	SIZE_T               MessageSize;
	UNICODE_STRING       PortName;
	OBJECT_ATTRIBUTES    ObjectAttribute;
	ALPC_PORT_ATTRIBUTES PortAttribute;
	LARGE_INTEGER        TimeOut;
	static BYTE          SharedBuffer[0x1000];

	while (m_ShouldRun)
	{
		RtlInitUnicodeString(&PortName, m_RpcPath.c_str());
		InitializeObjectAttributes(&ObjectAttribute, &PortName, 0, 0, 0);
		RtlZeroMemory(&PortAttribute, sizeof(PortAttribute));

		Status = NtAlpcCreatePort(&m_AlpcServerPort, &ObjectAttribute, &PortAttribute);
		if (NT_FAILED(Status))
			break;

		ReceivedMessage = (PPORT_MESSAGE)SharedBuffer;
		FormatTimeOut(&TimeOut, m_ShakehandTimeout);
		for (ULONG RetryCount = 0; RetryCount <= 5; RetryCount++)
		{
			RtlZeroMemory(SharedBuffer, sizeof(SharedBuffer));
			Status = NtAlpcSendWaitReceivePort(
				m_AlpcServerPort,
				0,
				NULL,
				NULL,
				ReceivedMessage,
				&MessageSize,
				NULL,
				&TimeOut
			);

			if (Status == STATUS_TIMEOUT)
				break;

			if (NT_SUCCESS(Status) && ValidateHandleShakeMessage((PBYTE)SharedBuffer + sizeof(PORT_MESSAGE), ReceivedMessage->u1.s1.DataLength))
				break;
		}

		if (NT_FAILED(Status))
		{
			NtPrintConsole(L"Handshake failed : %08x\n", Status);
			break;
		}

		m_PortIsReady = TRUE;

		LOOP_FOREVER
		{
			if (!InterlockedCompareExchange(&m_ShouldRun, FALSE, TRUE))
				 break;

			FormatTimeOut(&TimeOut, 20);
			
			Status = NtAlpcSendWaitReceivePort(
				m_AlpcServerPort,
				0,
				NULL,
				NULL,
				ReceivedMessage,
				&MessageSize,
				NULL,
				&TimeOut
			);

			if (NT_SUCCESS(Status))
			{
				m_Instance->ParseMessage(SharedBuffer + sizeof(PORT_MESSAGE), ReceivedMessage->u1.s1.DataLength);
				RtlZeroMemory(SharedBuffer, sizeof(SharedBuffer));
			}

			if (!IsRemoteProcessAlive())
			{
				if (m_NotifyServerRaiseErrorStub)
				{
					if (!m_NotifyServerRaiseErrorStub(
						RaiseErrorType::RAISE_ERROR_REMOTE_DEAD,
						L"Remote process is dead")
						)
					{
						Ps::ExitProcess(0);
					}
				}
			}

			Ps::Sleep(10);
		}
	}
	
	m_PortIsReady = FALSE;
	InterlockedExchange(&m_ShouldRun, FALSE);
}

