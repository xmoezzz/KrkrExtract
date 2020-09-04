#pragma once

#include <my.h>

class NativeThread
{
public:

	NativeThread();
	virtual ~NativeThread();
	virtual void ThreadFunction() = 0;
	void Stop();
	void SetPriority(int priority);
	void SendKill();
	bool WaitFor(unsigned time);
	bool IsTerminated();
	bool IsStarted()         const { return m_Handle != nullptr; }
	bool IsBeingTerminated() const { return m_ShouldRun == FALSE; }

	static DWORD GetCurrentThreadID();

	bool Run();

protected:
	HANDLE        m_Handle;
	volatile LONG m_ShouldRun;
	CLIENT_ID     m_Client;
};

