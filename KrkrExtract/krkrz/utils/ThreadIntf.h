//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Thread base class
//---------------------------------------------------------------------------
#ifndef ThreadIntfH
#define ThreadIntfH
#include "tjsNative.h"



//---------------------------------------------------------------------------
// tTVPThreadPriority
//---------------------------------------------------------------------------
enum tTVPThreadPriority
{
	ttpIdle, ttpLowest, ttpLower, ttpNormal, ttpHigher, ttpHighest, ttpTimeCritical
};
//---------------------------------------------------------------------------

#include "ThreadImpl.h"

/*[*/
const tjs_int TVPMaxThreadNum = 8;
typedef void (TJS_USERENTRY *TVP_THREAD_TASK_FUNC)(void *);
typedef void * TVP_THREAD_PARAM;
/*]*/

TJS_EXP_FUNC_DEF(tjs_int, TVPGetProcessorNum, ());
TJS_EXP_FUNC_DEF(tjs_int, TVPGetThreadNum, ());
TJS_EXP_FUNC_DEF(void, TVPBeginThreadTask, (tjs_int num));
TJS_EXP_FUNC_DEF(void, TVPExecThreadTask, (TVP_THREAD_TASK_FUNC func, TVP_THREAD_PARAM param));
TJS_EXP_FUNC_DEF(void, TVPEndThreadTask, ());

#endif
