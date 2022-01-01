#include "tp_stub.h"
#include <my.h>
#include <vector>
#include <inttypes.h>
#include "json/json.h"
#include "tjsObject.h"


NTSTATUS SavePbd(LPCWSTR FileName, LPCWSTR Path)
{
	NTSTATUS    Status;
	NtFileDisk  File;
	std::string JsonString;

	tTJSVariant PbdObjectWrapper;

	Status = STATUS_UNSUCCESSFUL;
	TVPExecuteScript(ttstr(L"var dumpstr = Scripts.loadDataPack(\"") + FileName + ttstr("\");"));
	//TVPExecuteScript(ttstr(L"System.inform(typeof(dumpstr[0].layer_type));"));
	TVPExecuteScript(ttstr(L"return dumpstr;"), &PbdObjectWrapper);

	auto PbdObject = PbdObjectWrapper.AsObject();
	if (PbdObject == NULL)
		return Status;

	tTJSVariant PbdObjectCountWrapper;
	PbdObject->PropGet(TJS_MEMBERMUSTEXIST, L"count", 0, &PbdObjectCountWrapper, PbdObject);
	auto PbdObjectCount = PbdObjectCountWrapper.AsInteger();
	if (PbdObjectCount == 0)
		return Status;

	Json::Value Root;

	for (tTVInteger i = 0; i < PbdObjectCount; i++)
	{
		tTJSVariant ItemWrapper;
		PbdObject->PropGetByNum(TJS_MEMBERMUSTEXIST, (tjs_int)i, &ItemWrapper, PbdObject);
		auto Item = ItemWrapper.AsObject();
		if (Item == NULL)
			continue;

		tTJSVariant NameWrapper;
		tTJSVariant PathNameWrapper;
		tTJSVariant WidthWrapper;
		tTJSVariant HeightWrapper;
		tTJSVariant LayerIdWrapper;
		tTJSVariant LayerTypeWrapper;

		//
		// TODO
		//
		if (Item->IsInstanceOf(0, NULL, NULL, L"Dictionary", NULL) != TJS_S_TRUE)
			continue;

		auto caller = std::shared_ptr<DictIterateCaller>(new DictIterateCaller());
		if (!caller)
			continue;

		tTJSVariantClosure closure(caller.get());
		Item->EnumMembers(TJS_IGNOREPROP, &closure, NULL);
		Root.append(caller->Item);
	}

	JsonString = Root.toStyledString();
	Status = File.Create(Path);
	if (NT_FAILED(Status))
		return Status;

	File.Write((PBYTE)JsonString.c_str(), JsonString.length());
	File.Close();

	return Status;
}


