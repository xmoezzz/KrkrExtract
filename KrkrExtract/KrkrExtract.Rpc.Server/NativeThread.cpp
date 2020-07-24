#include "NativeThread.h"


static DWORD WINAPI ThreadFunctionStatic(void* data)
{
	NativeThread* thread = static_cast<NativeThread*>(data);
	thread->ThreadFunction();
	return 0;
}


NativeThread::NativeThread() :
	m_Handle(nullptr),
	m_ShouldRun(false)
{
}

NativeThread::~NativeThread()
{
	Stop();
}

bool NativeThread::Run()
{
	NTSTATUS Status;

	if (m_Handle)
		return false;

	m_ShouldRun = true;
	Status = Nt_CreateThread(ThreadFunctionStatic, this, 0, NtCurrentProcess(), &m_Handle, &m_Client);
	if (NT_FAILED(Status))
		return false;

	return true;
}

void NativeThread::Stop()
{
	NTSTATUS      Status;
	LARGE_INTEGER TimeOut;

	if (!m_Handle)
		return;

	//
	// Notify : 
	//
	m_ShouldRun = false;
	FormatTimeOut(&TimeOut, 10);
	for (ULONG RetryCount = 0; RetryCount < 5; RetryCount++)
	{
		Status = NtWaitForSingleObject(m_Handle, FALSE, &TimeOut);
		if (NT_FAILED(Status))
			break;
	}

	if (!IsTerminated()) {
		NtTerminateThread(m_Handle, STATUS_REQUEST_CANCELED);
	}

	NtClose(m_Handle);
	m_Handle = NULL;
}


void NativeThread::SendKill()
{
	InterlockedExchange((volatile LONG*)&m_ShouldRun, false);
}

bool NativeThread::WaitFor(unsigned time)
{
	NTSTATUS      Status;
	LARGE_INTEGER TimeOut;

	FormatTimeOut(&TimeOut, time);
	Status = NtWaitForSingleObject(m_Handle, FALSE, NULL);
	return NT_SUCCESS(Status);
}


bool NativeThread::IsTerminated()
{
	NTSTATUS                 Status;
	THREAD_BASIC_INFORMATION Info;

	if (!m_Handle)
		return true;

	Status = NtQueryInformationThread(m_Handle, ThreadBasicInformation, &Info, sizeof(Info), NULL);
	if (NT_FAILED(Status))
		return true;

	return Info.ExitStatus != STATUS_PENDING;
}

void NativeThread::SetPriority(int priority)
{
	if (m_Handle) {
		Nt_SetThreadPriority(m_Handle, priority);
	}
}

DWORD NativeThread::GetCurrentThreadID()
{
	return Nt_GetCurrentThreadId();
}


