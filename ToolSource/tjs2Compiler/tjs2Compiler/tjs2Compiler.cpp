#include <Windows.h>
#include "tjs.h"
#include "TextStream.h"
#include "TVPStream.h"

tTJS *TVPScriptEngine = NULL;
static ttstr TVPScriptTextEncoding = L"UTF-8";


//Init ScriptEngine before compilation!
static bool TVPScriptEngineInit = false;
void TVPInitScriptEngine()
{
	if (TVPScriptEngineInit) return;
	TVPScriptEngineInit = true;

	TVPScriptTextEncoding = ttstr(TVPGetDefaultReadEncoding());

	// create script engine object
	TVPScriptEngine = new tTJS();

	// add kirikiriz
	TVPScriptEngine->SetPPValue(TJS_W("kirikiriz"), 1);
}


void TVPCompileStorage(const ttstr& name, bool isrequestresult, bool outputdebug, bool isexpression, const ttstr& outputpath) 
{
	// execute storage which contains script
	if (!TVPScriptEngine)
	{
		MessageBoxW(NULL, L"You Must Init ScriptEngine Firstly", L"Error", MB_OK);
		return;
	}

	iTJSTextReadStream * stream = TVPCreateTextStreamForReadByEncoding(name, TJS_W(""), TVPScriptTextEncoding);

	ttstr buffer;
	try 
	{
		stream->Read(buffer, 0);
	}
	catch (...) 
	{
		stream->Destruct();
		throw;
	}
	stream->Destruct();

	tTJSBinaryStream* outputstream = TVPCreateStream(outputpath, TJS_BS_WRITE);
	if (TVPScriptEngine) 
	{
		try 
		{
			TVPScriptEngine->CompileScript(buffer.c_str(), outputstream, isrequestresult, outputdebug, isexpression, name.c_str(), 0);
		}
		catch (...) 
		{
			delete outputstream;
			throw;
		}
	}
	delete outputstream;
}

//-------TJS2 Compiler-----------
int wmain(int argc, WCHAR* argv[])
{
	if (argc != 2 && argc != 3)
	{
		return 0;
	}

	TVPInitScriptEngine();
	TVPCompileStorage(ttstr(argv[1]), false, false, false, argc==3 ? ttstr(argv[2]) : ttstr(argv[1]) + L".comp");
	return 0;
}

