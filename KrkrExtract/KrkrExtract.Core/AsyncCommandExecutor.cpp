#include "AsyncCommandExecutor.h"
#include "DebuggerHandler.h"

AsyncCommandExecutor::AsyncCommandExecutor(KrkrClientProxyer* Proxyer, ULONG TaskThreshold) :
	m_TaskThreshold(TaskThreshold),
	m_Proxyer(Proxyer)
{
	NTSTATUS Status;

	RtlInitializeCriticalSection(&m_CriticalSection);
	InitializeThreadpoolEnvironment(&m_Environment);
	m_TaskThreshold = m_TaskThreshold > 32 ? 32 : m_TaskThreshold;
	m_TaskThreshold = m_TaskThreshold < 4  ? 4 : m_TaskThreshold;

	Status = TpAllocPool(&m_Pool, nullptr);
	if (NT_FAILED(Status))
		return;

	TpSetPoolMaxThreads(m_Pool, m_TaskThreshold);

	Status = TpAllocCleanupGroup(&m_CleanUpGroup);
	if (NT_FAILED(Status))
		return;

	TpSetCallbackThreadpool(&m_Environment, m_Pool);
	TpSetCallbackCleanupGroup(&m_Environment, m_CleanUpGroup, nullptr);

	m_Initialization = TRUE;
}

AsyncCommandExecutor::~AsyncCommandExecutor()
{
	Shutdown();
	RtlDeleteCriticalSection(&m_CriticalSection);
}

BOOL AsyncCommandExecutor::IsInitialized()
{
	return m_Initialization;
}


struct CommandExecutorContext
{
	PWSTR              Command;
	KrkrClientProxyer* Proxyer;
};

VOID NTAPI CommandExecutorCallback(
	_Inout_     PTP_CALLBACK_INSTANCE Instance,
	_Inout_opt_ PVOID                 Context,
	_Inout_     PTP_WORK              Work
)
{
	if (!Context)
		return;

	((CommandExecutorContext*)Context)->Proxyer->ParseUtilCommand(((CommandExecutorContext*)Context)->Command);

	if (((CommandExecutorContext*)Context)->Command) {
		FreeMemoryP(((CommandExecutorContext*)Context)->Command);
	}

	if (Context) {
		FreeMemoryP(Context);
	}

}

BOOL AsyncCommandExecutor::Execute(PCWSTR Command)
{
	ULONG Length;
	PWSTR DuplicatedCommand;

	Length = StrLengthW(Command);
	if (Length == 0)
		return FALSE;
	
	Length += 1;
	DuplicatedCommand = (PWSTR)AllocateMemoryP(Length, HEAP_ZERO_MEMORY);
	if (!DuplicatedCommand)
		return FALSE;

	StrCopyW(DuplicatedCommand, Command);
	return PushCallback(CommandExecutorCallback, DuplicatedCommand);
}


BOOL AsyncCommandExecutor::PushCallback(PTP_WORK_CALLBACK Callback, PVOID Param)
{
	NTSTATUS                Status;
	PTP_WORK                Work;
	CommandExecutorContext* Context;


	Context = (CommandExecutorContext*)AllocateMemoryP(sizeof(CommandExecutorContext));
	if (!Context)
		return FALSE;

	Context->Command = (PWSTR)Param;
	Context->Proxyer = m_Proxyer;

	Work   = nullptr;
	Status = TpAllocWork(&Work, Callback, Context, &m_Environment);
	if (NT_FAILED(Status))
		return FALSE;
	
	TpPostWork(Work);
	return TRUE;
}

BOOL AsyncCommandExecutor::Shutdown()
{
	TpReleaseCleanupGroupMembers(m_CleanUpGroup, FALSE, nullptr);
	TpReleaseCleanupGroup(m_CleanUpGroup);
	m_CleanUpGroup = nullptr;

	TpReleasePool(m_Pool);
	m_Pool = nullptr;
	m_Initialization = FALSE;

	return TRUE;
}


