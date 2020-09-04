#include "tp_stub.h"
#include <my.h>
#include <vector>
#include <inttypes.h>
#include "json/json.h"

class CustomVariant
{
public:
	CustomVariant() : type(tvtVoid) {
		integer = 0;
		real = 0.0;
	}

	CustomVariant& operator = (const CustomVariant& o) {
		type = o.type;
		string = o.string;
		integer = o.integer;
		real = o.real;

		return *this;
	}

	std::string GetVal() {
		CHAR number[400];
		RtlZeroMemory(number, sizeof(number));
		switch (type)
		{
		case tvtVoid:
			return std::string("(void)");
		case tvtObject:
			return string;
		case tvtString:
			return string;
		case tvtOctet:
			return string;
		case tvtInteger:
			sprintf_s(number, "%" PRIu64, integer);
			return std::string(number);
		case tvtReal:
			sprintf_s(number, "%g", real);
			return std::string(number);
		}
		return {};
	}

	bool IsNumber() {
		if (type == tvtInteger || type == tvtReal) {
			return true;
		}
		return false;
	}

public:
	tTJSVariantType type;
	std::string     string;
	tTVInteger      integer;
	tTVReal         real;
};

class DictIterateCaller : public tTJSDispatch
{
public:
	DictIterateCaller() {
	}

	virtual tjs_error TJS_INTF_METHOD FuncCall( // function invocation
		tjs_uint32 flag,			// calling flag
		const tjs_char * membername,// member name ( NULL for a default member )
		tjs_uint32 *hint,			// hint for the member name (in/out)
		tTJSVariant *result,		// result
		tjs_int numparams,			// number of parameters
		tTJSVariant **param,		// parameters
		iTJSDispatch2 *objthis		// object as "this"
	) {
		if (numparams > 1) {
			if ((int)*param[1] != TJS_HIDDENMEMBER) {
				auto _key = param[0];
				auto _val = param[2];

				CustomVariant key = DumpValue(_key);
				CustomVariant val = DumpValue(_val);

				if (val.IsNumber()) {
					if (val.type == tvtInteger) {
						Item[key.GetVal()] = val.integer;
					}
					else {
						Item[key.GetVal()] = val.real;
					}
				}
				else {
					Item[key.GetVal()] = val.GetVal();
				}
			}
		}
		if (result) *result = true;
		return TJS_S_OK;
	}


	tjs_int TVPWideCharToUtf8(tjs_char in, char * out)
	{
		// convert a wide character 'in' to utf-8 character 'out'
		if (in < (1 << 7))
		{
			if (out)
			{
				out[0] = (char)in;
			}
			return 1;
		}
		else if (in < (1 << 11))
		{
			if (out)
			{
				out[0] = (char)(0xc0 | (in >> 6));
				out[1] = (char)(0x80 | (in & 0x3f));
			}
			return 2;
		}
		else if (in < (1 << 16))
		{
			if (out)
			{
				out[0] = (char)(0xe0 | (in >> 12));
				out[1] = (char)(0x80 | ((in >> 6) & 0x3f));
				out[2] = (char)(0x80 | (in & 0x3f));
			}
			return 3;
		}
		else if (in < (1 << 21))
		{
			if (out)
			{
				out[0] = (char)(0xf0 | (in >> 18));
				out[1] = (char)(0x80 | ((in >> 12) & 0x3f));
				out[2] = (char)(0x80 | ((in >> 6) & 0x3f));
				out[3] = (char)(0x80 | (in & 0x3f));
			}
			return 4;
		}
		else if (in < (1 << 26))
		{
			if (out)
			{
				out[0] = (char)(0xf8 | (in >> 24));
				out[1] = (char)(0x80 | ((in >> 16) & 0x3f));
				out[2] = (char)(0x80 | ((in >> 12) & 0x3f));
				out[3] = (char)(0x80 | ((in >> 6) & 0x3f));
				out[4] = (char)(0x80 | (in & 0x3f));
			}
			return 5;
		}
		else if (in < (1 << 31))
		{
			if (out)
			{
				out[0] = (char)(0xfc | (in >> 30));
				out[1] = (char)(0x80 | ((in >> 24) & 0x3f));
				out[2] = (char)(0x80 | ((in >> 18) & 0x3f));
				out[3] = (char)(0x80 | ((in >> 12) & 0x3f));
				out[4] = (char)(0x80 | ((in >> 6) & 0x3f));
				out[5] = (char)(0x80 | (in & 0x3f));
			}
			return 6;
		}
		return -1;
	}

	tjs_int TVPWideCharToUtf8StringInternal(const tjs_char *in, char * out)
	{
		// convert input wide string to output utf-8 string
		int count = 0;
		while (*in)
		{
			tjs_int n;
			if (out)
			{
				n = TVPWideCharToUtf8(*in, out);
				out += n;
			}
			else
			{
				n = TVPWideCharToUtf8(*in, NULL);
				/*
					in this situation, the compiler's inliner
					will collapse all null check parts in
					TVPWideCharToUtf8.
				*/
			}
			if (n == -1) return -1; // invalid character found
			count += n;
			in++;
		}
		return count;
	}


	bool TVPUtf16ToUtf8Internal(std::string& out, const std::wstring& in) {
		tjs_int len = TVPWideCharToUtf8String(in.c_str(), NULL);
		if (len < 0) return false;
		char* buf = new char[len];
		if (buf) {
			try {
				len = TVPWideCharToUtf8StringInternal(in.c_str(), buf);
				if (len > 0) out.assign(buf, len);
				delete[] buf;
			}
			catch (...) {
				delete[] buf;
				return false;
			}
		}
		return len > 0;
	}


	std::string VariantGeString(tTJSVariant* val) {
		auto  s = val->AsString();
		ttstr str = s;

		int         Size;
		std::string Result;
		std::wstring in(str.c_str());

		if (TVPUtf16ToUtf8Internal(Result, in) == false) {
			return {};
		}
		return Result;
	}



	CustomVariant DumpValue(tTJSVariant* val) {
		CustomVariant     var;
		tTJSVariantOctet* octet;
		iTJSDispatch2*    object;

		if (val) {
			tTJSVariantType type = val->Type();
			switch (type)
			{
			case tvtVoid:
				break;
			case tvtObject:
				object = val->AsObject();
				var.type = tvtObject;
				if (object) {
					CHAR ObjectName[100];
					RtlZeroMemory(ObjectName, sizeof(ObjectName));
					wsprintfA(ObjectName, "(object)(%p)", object);
					var.string = ObjectName;
				}
				else {
					var.string = std::string("(null object)()");
				}
				break;
			case tvtString:
				var.type = tvtString;
				var.string = VariantGeString(val);
				break;
			case tvtOctet:
				octet = val->AsOctet();
				var.type = tvtOctet;
				if (octet) {
					const tjs_uint8* data = octet->GetData();
					tjs_uint len = octet->GetLength();
					CHAR        Buffer[10];
					std::string Result = "<";
					for (tjs_uint i = 0; i < len; i++) {
						RtlZeroMemory(Buffer, sizeof(Buffer));
						wsprintfA(Buffer, "%02x", data[i]);
						Result += Buffer;
						if (i < len - 1) {
							Result += " ";
						}
					}
					Result += ">";
					var.string = Result;
				}
				else {
					var.string = std::string("(null octet)<>");
				}
				break;
			case tvtInteger:
				var.type = tvtInteger;
				var.integer = val->AsInteger();
				break;
			case tvtReal:
				var.type = tvtReal;
				var.real = val->AsReal();
				break;
			}
		}
		return var;
	}

	Json::Value Item;
};



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


