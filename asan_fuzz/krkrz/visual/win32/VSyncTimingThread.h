
#ifndef __VSYNC_TIMING_THREAD_H__
#define __VSYNC_TIMING_THREAD_H__

#include "ThreadIntf.h"
#include "NativeEventQueue.h"

//---------------------------------------------------------------------------
// VSync用のタイミングを発生させるためのスレッド
//---------------------------------------------------------------------------
class tTVPVSyncTimingThread : public tTVPThread
{
	DWORD SleepTime;
	tTVPThreadEvent Event;
	tTJSCriticalSection CS;
	DWORD VSyncInterval; //!< VSync の間隔(参考値)
	DWORD LastVBlankTick; //!< 最後の vblank の時間

	bool Enabled;

	NativeEventQueue<tTVPVSyncTimingThread> EventQueue;

	class tTJSNI_Window* OwnerWindow;
public:
	tTVPVSyncTimingThread(class tTJSNI_Window* owner);
	~tTVPVSyncTimingThread();

protected:
	void Execute();
	void Proc( NativeEvent& ev );

public:
	void MeasureVSyncInterval(); // VSyncInterval を計測する
};
//---------------------------------------------------------------------------

#endif // __VSYNC_TIMING_THREAD_H__
