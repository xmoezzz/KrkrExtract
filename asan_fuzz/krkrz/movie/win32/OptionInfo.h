//---------------------------------------------------------------------------
// option information for kirikiri configurator
//---------------------------------------------------------------------------

#ifndef __OptionInfoH__
#define __OptionInfoH__

#include <windows.h>


//---------------------------------------------------------------------------
// GetShouldRegisterToROT: returns whether to register the process into ROT
//---------------------------------------------------------------------------
static inline bool GetShouldRegisterToROT()
{
	static bool cached = false;
	static bool state = false;
	if(!cached)
	{
		tTJSVariant val;
		if(TVPGetCommandLine(TJS_W("-movie_reg_rot"), &val))
		{
			if(ttstr(val) == TJS_W("yes") || ttstr(val) == TJS_W("pause") )
				state = true;
		}
		cached = true;
	}
	return state;
}
//---------------------------------------------------------------------------
// MakeAPause: display a message-box if the option specified it
//---------------------------------------------------------------------------
static inline void MakeAPause(bool _error)
{
	// _error specifies reason of this pause.
	// The pause is caused by an error during building graph if _error is true.
	// Successful if false.

	static bool cached = false;
	static bool state = false;
	if(!cached)
	{
		tTJSVariant val;
		if(TVPGetCommandLine(TJS_W("-movie_reg_rot"), &val))
		{
			if(ttstr(val) == TJS_W("pause")) state = true;
		}
		cached = true;
	}
	if(state)
	{
		MessageBox(TVPGetApplicationWindowHandle(),
			_error?
				_T("The graph was not properly built. Pausing."):
				_T("The graph was successfully built. Pausing."), _T("Pause"), MB_OK);
	}
}

#endif
