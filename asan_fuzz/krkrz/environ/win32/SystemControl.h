//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// System Main Window (Controller)
//---------------------------------------------------------------------------
#ifndef SystemControlH
#define SystemControlH
//---------------------------------------------------------------------------
#include <string>
#include "TVPTimer.h"
//---------------------------------------------------------------------------
class tTVPSystemControl
{
private:	// ユーザー宣言
	bool ContinuousEventCalling;
	bool AutoShowConsoleOnError;

	bool EventEnable;

	DWORD LastCompactedTick;
	DWORD LastCloseClickedTick;
	DWORD LastShowModalWindowSentTick;
	DWORD LastRehashedTick;

	DWORD MixedIdleTick;

	TVPTimer SystemWatchTimer;
public:
	tTVPSystemControl();

	void InvokeEvents();
	void CallDeliverAllEventsOnIdle();

	void BeginContinuousEvent();
	void EndContinuousEvent();

	void NotifyCloseClicked();
	void NotifyEventDelivered();

	void SetEventEnabled( bool b ) {
		EventEnable = b;
	}
	bool GetEventEnabled() const { return EventEnable; }

	bool ApplicationIdle();

private:
	void DeliverEvents();
	void SystemWatchTimerTimer();
};
extern tTVPSystemControl *TVPSystemControl;
extern bool TVPSystemControlAlive;

#endif
