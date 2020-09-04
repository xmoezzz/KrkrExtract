#pragma once

#include <my.h>
#include <atomic>
#include <KrkrClientProxyer.h>

class AsyncCommandExecutor
{
public:
	AsyncCommandExecutor() = delete;
	AsyncCommandExecutor(const AsyncCommandExecutor&) = delete;
	~AsyncCommandExecutor();

	AsyncCommandExecutor(KrkrClientProxyer* Proxyer, ULONG TaskThreshold = 16);
	
	BOOL IsInitialized();
	BOOL Shutdown();
	BOOL Execute(PCWSTR Command);

private:
	BOOL PushCallback(PTP_WORK_CALLBACK Callback, PVOID Param = nullptr);

private:
	ULONG                m_TaskThreshold;
	RTL_CRITICAL_SECTION m_CriticalSection;
	TP_CALLBACK_ENVIRON  m_Environment;
	PTP_POOL             m_Pool           = nullptr;
	PTP_CLEANUP_GROUP    m_CleanUpGroup   = nullptr;
	std::atomic<BOOL>    m_Initialization = FALSE;
	KrkrClientProxyer*   m_Proxyer        = nullptr;
};

