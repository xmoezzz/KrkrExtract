//---------------------------------------------------------------------------
/*
	TJS2 Script Engine
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Intermediate Code Execution
//---------------------------------------------------------------------------

#ifndef tjsInterCodeExecH
#define tjsInterCodeExecH


namespace TJS
{
//---------------------------------------------------------------------------
extern void TJSVariantArrayStackAddRef();
extern void TJSVariantArrayStackRelease();
extern void TJSVariantArrayStackCompact();
extern void TJSVariantArrayStackCompactNow();
//---------------------------------------------------------------------------
}


#endif
