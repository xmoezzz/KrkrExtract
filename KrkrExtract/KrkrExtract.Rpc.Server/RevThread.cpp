#include "RevThread.h"
#include "Console.h"
#include <memory>
#include <RpcSequence.h>
#include <my.h>
#include <memory>
#include <stdint.h>

RevThread::RevThread(
	AlpcRev*    Instance,
	ULONG       ShakehandTimeout,
	ULONG       Secret
) :
	m_Instance(Instance),
	m_ShakehandTimeout(ShakehandTimeout),
	m_Secret(Secret)
{

}

RevThread::~RevThread()
{
	Stop();
}


BOOL RevThread::CreateAlpcServer()
{
	WCHAR    PrivId[0x100];
	WCHAR    Path[0x100];

	RtlZeroMemory(PrivId, sizeof(PrivId));
	RtlZeroMemory(Path, sizeof(Path));
	GetEnvironmentVariableW(L"KrkrRpcIoPrivatePath", PrivId, countof(PrivId));

	wsprintfW(Path, L"\\RPC Control\\%s", PrivId);
	m_Server = ALPCServer::create(
		Path,
		0x1000 // one page
	);

	return m_Server ? TRUE : FALSE;
}


BOOL RevThread::Stop()
{
	m_Instance = nullptr;

	if (m_Server) {
		delete m_Server;
		m_Server = FALSE;
	}
	
	return TRUE;
}


BOOL RevThread::IsAlpcServerReady()
{
	return m_ReadyClientPid != 0;
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


//
// sent by client
//
struct ConnectionRequest
{
	enum class ConnectionStatus : ULONG {
		OK            = TAG4('Krkr'),
		CHILD_PROCESS = 1
	};

	UINT32           RemoteProcessPid;
	ConnectionStatus Status;
};
#pragma pack()


NTSTATUS RevThread::ValidateHandleShakeMessage(PVOID MessageBuffer, SIZE_T MessageSize)
{
	NTSTATUS                  Status;
	HandShakeRequest*         Request;

	if (MessageSize < sizeof(HandShakeRequest))
		return FALSE;

	//
	// 好孩子不要在生产环境写这样的代码==
	//
	Request = (HandShakeRequest*)MessageBuffer;
	if (Request->Secret != m_Secret)
		return FALSE;

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

BOOL RevThread::SendMessageToRemote(ULONG MessageType, std::string&& Request)
{
	NTSTATUS     Status;

	if (!IsAlpcServerReady())
		return FALSE;

	auto msg = std::make_unique<ALPCMessage>(Request.length() + sizeof(MessageType));
	msg->with_attribute(new ALPCMessageAttribute(ALPC_MESSAGE_ALL_ATTRIBUTE));

	RtlCopyMemory(msg->data(), &MessageType, sizeof(MessageType));
	RtlCopyMemory(static_cast<uint8_t*>(msg->data()) + sizeof(MessageType), Request.data(), Request.length());

	return m_Server->send(*msg);
}


BOOL RevThread::SendMessageToRemote(ULONG MessageType, std::string& Request)
{
	return SendMessageToRemote(MessageType, std::move(Request));
}



void RevThread::ThreadFunction()
{
	NTSTATUS             Status;
	PPORT_MESSAGE        ReceivedMessage;
	SIZE_T               MessageSize;
	LARGE_INTEGER        TimeOut;
	static BYTE          SharedBuffer[0x1000];

	auto msg = std::make_unique<ALPCMessage>(0x1000);
	msg->with_attribute(new ALPCMessageAttribute(ALPC_MESSAGE_ALL_ATTRIBUTE));
	

	while (m_ShouldRun)
	{
		if (!m_ShouldRun) {
			break;
		}

		if (!m_Server->recv(*msg)) {
			Ps::Sleep(20);
			continue;
		}

		switch (msg->type())
		{
		case LPC_CONNECTION_REQUEST:
			if (m_ReadyClientPid) {
				break;
			}

			if (msg->size() != sizeof(ConnectionRequest)) {
				break;
			}

			if (static_cast<ConnectionRequest*>(msg->data())->Status != ConnectionRequest::ConnectionStatus::OK) {
				break;
			}

			if (!m_Server->accept(*msg)) {
				PrintConsoleW(L"server : accept failed\n");
				break;
			}
			
			m_ReadyClientPid = (ULONG)msg->buffer()->ClientId.UniqueProcess;
			break;

		case LPC_REQUEST:
			m_Instance->ParseMessage(
				msg->data(),
				msg->size()
			);
			break;

		case LPC_CLIENT_DIED:
		case LPC_PORT_CLOSED:
			if (m_ReadyClientPid == (ULONG)msg->buffer()->ClientId.UniqueProcess) {
				PrintConsoleW(L"Client died\n");
				break;
			}
			break;

		default:
			break;
		}

		msg->clear();
		Ps::Sleep(10);
	}
	
	m_ReadyClientPid = 0;
	m_ShouldRun = FALSE;
}

