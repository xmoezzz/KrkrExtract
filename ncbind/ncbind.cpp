#include <windows.h>
#include "ncbind.hpp"


#define EXPORT(hr) extern "C" __declspec(dllexport) hr __stdcall

#ifdef _MSC_VER
#pragma comment(linker, "/EXPORT:V2Link=_V2Link@4")
#pragma comment(linker, "/EXPORT:V2Unlink=_V2Unlink@0")
#endif

//--------------------------------------
int WINAPI
DllEntryPoint(HINSTANCE /*hinst*/, unsigned long /*reason*/, void* /*lpReserved*/)
{
	return 1;
}

//---------------------------------------------------------------------------
static tjs_int GlobalRefCountAtInit = 0;

EXPORT(HRESULT) V2Link(iTVPFunctionExporter *exporter)
{
	// スタブの初期化(必ず記述する)
	TVPInitImportStub(exporter);

	NCB_LOG_W("V2Link");

	// AutoRegisterで登録されたクラス等を登録する
	ncbAutoRegister::AllRegist();

	// この時点での TVPPluginGlobalRefCount の値を
	GlobalRefCountAtInit = TVPPluginGlobalRefCount;
	// として控えておく。TVPPluginGlobalRefCount はこのプラグイン内で
	// 管理されている tTJSDispatch 派生オブジェクトの参照カウンタの総計で、
	// 解放時にはこれと同じか、これよりも少なくなってないとならない。
	// そうなってなければ、どこか別のところで関数などが参照されていて、
	// プラグインは解放できないと言うことになる。

	return S_OK;
}
//---------------------------------------------------------------------------
EXPORT(HRESULT) V2Unlink()
{
	// 吉里吉里側から、プラグインを解放しようとするときに呼ばれる関数

	// もし何らかの条件でプラグインを解放できない場合は
	// この時点で E_FAIL を返すようにする。
	// ここでは、TVPPluginGlobalRefCount が GlobalRefCountAtInit よりも
	// 大きくなっていれば失敗ということにする。
	if (TVPPluginGlobalRefCount > GlobalRefCountAtInit) {
		NCB_LOG_W("V2Unlink ...failed");
		return E_FAIL;
		// E_FAIL が帰ると、Plugins.unlink メソッドは偽を返す
	} else {
		NCB_LOG_W("V2Unlink");
	}
	/*
		ただし、クラスの場合、厳密に「オブジェクトが使用中である」ということを
		知るすべがありません。基本的には、Plugins.unlink によるプラグインの解放は
		危険であると考えてください (いったん Plugins.link でリンクしたら、最後ま
		でプラグインを解放せず、プログラム終了と同時に自動的に解放させるのが吉)。
	*/

	// AutoRegisterで登録されたクラス等を削除する
	ncbAutoRegister::AllUnregist();

	// スタブの使用終了(必ず記述する)
	TVPUninitImportStub();

	return S_OK;
}


//---------------------------------------------------------------------------
// static変数の実体

// auto register 先頭ポインタ
ncbAutoRegister::ThisClassT const*
ncbAutoRegister::_top[ncbAutoRegister::LINE_COUNT] = NCB_INNER_AUTOREGISTER_LINES_INSTANCE;

