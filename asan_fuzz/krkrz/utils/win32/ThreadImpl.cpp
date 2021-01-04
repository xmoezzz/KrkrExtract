//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Thread base class
//---------------------------------------------------------------------------
#define NOMINMAX
#include "tjsCommHead.h"

#include <process.h>
#include <algorithm>

#include "ThreadIntf.h"
#include "ThreadImpl.h"
#include "MsgIntf.h"



//---------------------------------------------------------------------------
// tTVPThread : a wrapper class for thread
//---------------------------------------------------------------------------
tTVPThread::tTVPThread(bool suspended)
{
	Terminated = false;
	Handle = NULL;
	ThreadId = 0;
	Suspended = suspended;

	Handle = (HANDLE) _beginthreadex(
		NULL, 0, StartProc, this, suspended ? CREATE_SUSPENDED : 0,
			(unsigned *)&ThreadId);

	if(Handle == INVALID_HANDLE_VALUE) TVPThrowInternalError;
}
//---------------------------------------------------------------------------
tTVPThread::~tTVPThread()
{
	CloseHandle(Handle);
}
//---------------------------------------------------------------------------
unsigned __stdcall tTVPThread::StartProc(void * arg)
{
	((tTVPThread*)arg)->Execute();
	return 0;
}
//---------------------------------------------------------------------------
void tTVPThread::WaitFor()
{
	WaitForSingleObject(Handle, INFINITE);
}
//---------------------------------------------------------------------------
tTVPThreadPriority tTVPThread::GetPriority()
{
	int n = GetThreadPriority(Handle);
	switch(n)
	{
	case THREAD_PRIORITY_IDLE:			return ttpIdle;
	case THREAD_PRIORITY_LOWEST:		return ttpLowest;
	case THREAD_PRIORITY_BELOW_NORMAL:	return ttpLower;
	case THREAD_PRIORITY_NORMAL:		return ttpNormal;
	case THREAD_PRIORITY_ABOVE_NORMAL:	return ttpHigher;
	case THREAD_PRIORITY_HIGHEST:		return ttpHighest;
	case THREAD_PRIORITY_TIME_CRITICAL:	return ttpTimeCritical;
	}

	return ttpNormal;
}
//---------------------------------------------------------------------------
void tTVPThread::SetPriority(tTVPThreadPriority pri)
{
	int npri = THREAD_PRIORITY_NORMAL;
	switch(pri)
	{
	case ttpIdle:			npri = THREAD_PRIORITY_IDLE;			break;
	case ttpLowest:			npri = THREAD_PRIORITY_LOWEST;			break;
	case ttpLower:			npri = THREAD_PRIORITY_BELOW_NORMAL;	break;
	case ttpNormal:			npri = THREAD_PRIORITY_NORMAL;			break;
	case ttpHigher:			npri = THREAD_PRIORITY_ABOVE_NORMAL;	break;
	case ttpHighest:		npri = THREAD_PRIORITY_HIGHEST;			break;
	case ttpTimeCritical:	npri = THREAD_PRIORITY_TIME_CRITICAL;	break;
	}

	SetThreadPriority(Handle, npri);
}
//---------------------------------------------------------------------------
void tTVPThread::Suspend()
{
	SuspendThread(Handle);
}
//---------------------------------------------------------------------------
void tTVPThread::Resume()
{
	while((tjs_int32)ResumeThread(Handle) > 1) ;
}
//---------------------------------------------------------------------------







//---------------------------------------------------------------------------
// tTVPThreadEvent
//---------------------------------------------------------------------------
tTVPThreadEvent::tTVPThreadEvent(bool manualreset)
{
	Handle = CreateEvent(NULL, manualreset?TRUE:FALSE, FALSE, NULL);
	if(!Handle) TVPThrowInternalError;
}
//---------------------------------------------------------------------------
tTVPThreadEvent::~tTVPThreadEvent()
{
	CloseHandle(Handle);
}
//---------------------------------------------------------------------------
void tTVPThreadEvent::Set()
{
	SetEvent(Handle);
}
//---------------------------------------------------------------------------
void tTVPThreadEvent::Reset()
{
	ResetEvent(Handle);
}
//---------------------------------------------------------------------------
bool tTVPThreadEvent::WaitFor(tjs_uint timeout)
{
	// wait for event;
	// returns true if the event is set, otherwise (when timed out) returns false.

	DWORD state = WaitForSingleObject(Handle, timeout == 0 ? INFINITE : timeout);

	if(state == WAIT_OBJECT_0) return true;
	return false;
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
tjs_int TVPDrawThreadNum = 1;

struct ThreadInfo {
  bool readyToExit;
  HANDLE thread;
  TVP_THREAD_TASK_FUNC  lpStartAddress;
  TVP_THREAD_PARAM lpParameter;
};
static std::vector<ThreadInfo*> TVPThreadList;
static std::vector<tjs_int> TVPProcesserIdList;
static LONG TVPRunningThreadCount = 0;
static tjs_int TVPThreadTaskNum, TVPThreadTaskCount;

//---------------------------------------------------------------------------
static tjs_int GetProcesserNum(void)
{
  static tjs_int processor_num = 0;
  if (! processor_num) {
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    processor_num = info.dwNumberOfProcessors;
  }
  return processor_num;
}

tjs_int TVPGetProcessorNum(void)
{
  return GetProcesserNum();
}

//---------------------------------------------------------------------------
tjs_int TVPGetThreadNum(void)
{
  tjs_int threadNum = TVPDrawThreadNum ? TVPDrawThreadNum : GetProcesserNum();
  threadNum = std::min(threadNum, TVPMaxThreadNum);
  return threadNum;
}

//---------------------------------------------------------------------------
static DWORD WINAPI ThreadLoop(LPVOID p)
{
  ThreadInfo *threadInfo = (ThreadInfo*)p;
  for(;;) {
    if (threadInfo->readyToExit)
      break;
    (threadInfo->lpStartAddress)(threadInfo->lpParameter);
    InterlockedDecrement(&TVPRunningThreadCount);
    SuspendThread(GetCurrentThread());
  }
  delete threadInfo;
  ExitThread(0);

  return TRUE;
}
//---------------------------------------------------------------------------
void TVPBeginThreadTask(tjs_int taskNum)
{
  TVPThreadTaskNum = taskNum;
  TVPThreadTaskCount = 0;
  tjs_int extraThreadNum = TVPGetThreadNum() - 1;
  if (TVPProcesserIdList.empty()) {
#ifndef TJS_64BIT_OS
    DWORD processAffinityMask, systemAffinityMask;
    GetProcessAffinityMask(GetCurrentProcess(),
                           &processAffinityMask,
                           &systemAffinityMask);
    for (tjs_int i = 0; i < MAXIMUM_PROCESSORS; i++) {
      if (processAffinityMask & (1 << i))
        TVPProcesserIdList.push_back(i);
    }
#else
    ULONGLONG processAffinityMask, systemAffinityMask;
    GetProcessAffinityMask(GetCurrentProcess(),
                           (PDWORD_PTR)&processAffinityMask,
                           (PDWORD_PTR)&systemAffinityMask);
    for (tjs_int i = 0; i < MAXIMUM_PROCESSORS; i++) {
      if (processAffinityMask & (1ULL << i))
        TVPProcesserIdList.push_back(i);
    }
#endif
    if (TVPProcesserIdList.empty())
      TVPProcesserIdList.push_back(MAXIMUM_PROCESSORS);
  }
  while ( static_cast<tjs_int>(TVPThreadList.size()) < extraThreadNum) {
    ThreadInfo *threadInfo = new ThreadInfo();
    threadInfo->readyToExit = false;
    threadInfo->thread = CreateThread(NULL, 0, ThreadLoop, threadInfo, CREATE_SUSPENDED, NULL);
    SetThreadIdealProcessor(threadInfo->thread, TVPProcesserIdList[TVPThreadList.size() % TVPProcesserIdList.size()]);
    TVPThreadList.push_back(threadInfo);
  }
  while ( static_cast<tjs_int>(TVPThreadList.size()) > extraThreadNum) {
    ThreadInfo *threadInfo = TVPThreadList.back();
    threadInfo->readyToExit = true;
    while (ResumeThread(threadInfo->thread) == 0)
      Sleep(0);
    TVPThreadList.pop_back();
  }
}

//---------------------------------------------------------------------------
void TVPExecThreadTask(TVP_THREAD_TASK_FUNC func, TVP_THREAD_PARAM param)
{
  if (TVPThreadTaskCount >= TVPThreadTaskNum - 1) {
    func(param);
    return;
  }    
  ThreadInfo *threadInfo;
  threadInfo = TVPThreadList[TVPThreadTaskCount++];
  threadInfo->lpStartAddress = func;
  threadInfo->lpParameter = param;
  InterlockedIncrement(&TVPRunningThreadCount);
  while (ResumeThread(threadInfo->thread) == 0)
    Sleep(0);
}
//---------------------------------------------------------------------------
void TVPEndThreadTask(void) 
{
  while ((LONG)InterlockedCompareExchange(&TVPRunningThreadCount, 0, 0) != 0)
    Sleep(0);
}

//---------------------------------------------------------------------------
