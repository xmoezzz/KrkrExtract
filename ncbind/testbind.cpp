#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ncbind.hpp"

////////////////////////////////////////
/// mes(...)で任意の型でログを出力できるようにするテンプレ

// 型変換用Functor
template <typename T> struct ttstrWrap { T operator()(T t) { return t; } };
#define TTSTRCAST(type, cast) template <> struct ttstrWrap<type> { cast operator()(type t) { return t; } }
#define TTSTRCAST_INT(type)  TTSTRCAST(type, tjs_int)

// 整数はみんな tjs_int でキャストする
TTSTRCAST_INT(  signed char);
TTSTRCAST_INT(  signed short);
TTSTRCAST_INT(  signed int);
TTSTRCAST_INT(  signed long);
TTSTRCAST_INT(unsigned char);
TTSTRCAST_INT(unsigned short);
TTSTRCAST_INT(unsigned int);
TTSTRCAST_INT(unsigned long);

// 実数は sprintf で文字列に
struct ttstrFormat {
	ttstrFormat(tjs_nchar const *fmt) : _format(fmt) {}
	template <typename T>
	tjs_nchar const* operator()(T t) {
#if _MSC_VER >= 1400
		_snprintf_s(_buff, sizeof(_buff), _TRUNCATE, _format, t);
#else
		_snprintf(_buff, sizeof(_buff)-1, _format, t);
#endif
		return _buff;
	}
private:
	tjs_nchar const *_format;
	tjs_nchar _buff[256];
};
template <> struct ttstrWrap<float>  : public ttstrFormat { ttstrWrap() : ttstrFormat("%f" ) {} };
template <> struct ttstrWrap<double> : public ttstrFormat { ttstrWrap() : ttstrFormat("%lf") {} };

// 検証用
void setlog(ttstr const &log) {
	iTJSDispatch2 * global = TVPGetScriptDispatch();
	if (global) {
		tTJSVariant var(log);
		global->PropSet(TJS_MEMBERENSURE, TJS_W("CHECKLOG"), NULL, &var, global);
		global->Release();
	}
}

// 任意個数の引数に対応するために全展開
#undef  FOREACH_START
#define FOREACH_START 1
#undef  FOREACH_END
#define FOREACH_END   FOREACH_MAX
#undef  FOREACH

#define DEF_EXT(n) typename T ## n
#define REF_EXT(n)          T ## n t ## n
#define STR_EXT(n)           ttstr(ttstrWrap<T##n>()(t ## n)) +

#define FOREACH \
	template <        FOREACH_COMMA_EXT(DEF_EXT)> \
		void mes(     FOREACH_COMMA_EXT(REF_EXT)) { \
			ttstr log = FOREACH_SPACE_EXT(STR_EXT) ttstr(""); \
			TVPAddLog(log); setlog(log); }
#include FOREACH_INCLUDE


////////////////////////////////////////
// レジスト後にスクリプトを実行してチェックする

struct checker {
	typedef void (*CallbackT)();
	checker(CallbackT cb) : _callback(cb), _next(0) {
		if (!_top) _top = this;
		if (_last) _last->_next = this;
		_last = this;
	}
	static void Check() {for (checker const* p = _top; p; p = p->_next) p->_callback(); }
	static bool Result;
private:
	CallbackT const _callback;
	/****/ checker const* _next;
	static checker const* _top;
	static checker      * _last;
};
checker const* checker::_top  = 0;
checker      * checker::_last = 0;
bool checker::Result = true;

static void checker_Check() {
	checker::Check();
	TVPAddImportantLog(ttstr("###### ") + ttstr(checker::Result ? "All OK" : "Some NG(s)") + ttstr(" ######"));
	TVPAddLog(TJS_W("")); \
}
NCB_POST_REGIST_CALLBACK(checker_Check);

#define CHECK(tag, script) \
	void AutoCheck_ ## tag() { \
		TVPAddLog(TJS_W("   --- CHECK(") TJS_W(#tag) TJS_W(")")); \
		tTJSVariant var; TVPExecuteScript(ttstr(script), &var); \
		bool result = var.AsInteger() ? true : false; \
		TVPAddLog(TJS_W("")); \
		TVPAddImportantLog(ttstr("   ### ") + ttstr(result ? "OK" : "NG") + ttstr(" : CHECK(" #tag ")")); \
		TVPAddLog(TJS_W("")); \
		checker::Result &= result; \
	} static checker checker_## tag (AutoCheck_ ## tag)

#define SCRIPT_BEGIN "var _t, _f = true; try {\n"
#define SCRIPT_END   "} catch { _f = false; } return _f;\n"
#define SCRIPT_OUT(mark, str) \
	"Debug.message(' " #mark " ' + (_t ? 'OK' : 'NG') + ' : ' + \"" str "\");\n"

#define SCRIPT_EVAL(str) \
	"_t = (" str "); _f &= _t;\n" \
	SCRIPT_OUT(?, str)

#define SCRIPT_LOG_CHECK(str, result) \
	str ";\n" \
	"_t = (CHECKLOG === \"" result "\"); _f &= _t;\n" \
	SCRIPT_OUT(*, str)

#define SCRIPT_EVAL_LOG(str, result) \
	"_t = (" str " && CHECKLOG === \"" result "\"); _f &= _t;\n" \
	SCRIPT_OUT(&, str)

#define SCRIPT_MUST_ERROR(str) \
	"_t = true; try {\n" str ";\n_t = false; } catch {}\n" \
	SCRIPT_OUT(!, str)


////////////////////////////////////////

struct TypeConvChecker {
	TypeConvChecker() : _name(TJS_W("TypeConvChecker")) {
		mes(_name, "::TypeConvChecker()");
	}
	bool           Bool(           bool  b) const { mes(_name, "::Bool(",  (b ? "True" : "False"), ")"); return b; }

	signed   char  SChar(   signed char  n) const { mes(_name, "::SChar(",  n, ")"); return n; }
	signed   short SShort(  signed short n) const { mes(_name, "::SShort(", n, ")"); return n; }
	signed   int   SInt(    signed int   n) const { mes(_name, "::SInt(",   n, ")"); return n; }
	signed   long  SLong(   signed long  n) const { mes(_name, "::SLong(",  n, ")"); return n; }

	unsigned char  UChar( unsigned char  n) const { mes(_name, "::UChar(",  n, ")"); return n; }
	unsigned short UShort(unsigned short n) const { mes(_name, "::UShort(", n, ")"); return n; }
	unsigned int   UInt(  unsigned int   n) const { mes(_name, "::UInt(",   n, ")"); return n; }
	unsigned long  ULong( unsigned long  n) const { mes(_name, "::ULong(",  n, ")"); return n; }

	float          Float(          float f) const { mes(_name, "::Float(",  f, ")"); return f; }
	double         Double(        double d) const { mes(_name, "::Double(", d, ")"); return d; }

	char const*    ConstCharP(char const *p)const { mes(_name, "::ConstCharP(", p, ")"); return p; }
	tjs_char const*GetName()                const { return _name; }
private:
	tjs_char const *_name;
};

static tjs_error TJS_INTF_METHOD
RawCallback1(tTJSVariant *result, tjs_int numparams,
			 tTJSVariant **param, iTJSDispatch2 *objthis) {
	mes("RawCallback1");
	return TJS_S_OK;
}
static tjs_error TJS_INTF_METHOD
RawCallback2(tTJSVariant *result, tjs_int numparams,
			 tTJSVariant **param, TypeConvChecker *objthis) {
	mes("RawCallback2:", objthis->GetName());
	return TJS_S_OK;
}


NCB_REGISTER_CLASS(TypeConvChecker) {
	Constructor(); //NCB_CONSTRUCTOR(());

	Method("Bool", &Class::Bool); //NCB_METHOD(Bool);

	NCB_METHOD(SChar);
	NCB_METHOD(SShort);
	NCB_METHOD(SInt);
	NCB_METHOD(SLong);

	NCB_METHOD(UChar);
	NCB_METHOD(UShort);
	NCB_METHOD(UInt);
	NCB_METHOD(ULong);

	NCB_METHOD(Float);
	NCB_METHOD(Double);
	NCB_METHOD(ConstCharP);

	NCB_PROPERTY_RO(Name, GetName);

	RawCallback("Raw1", &RawCallback1, 0); //NCB_METHOD_RAW_CALLBACK(Raw1, RawCallback1, 0);
	RawCallback("Raw2", &RawCallback2, 0); //NCB_METHOD_RAW_CALLBACK(Raw2, RawCallback2, 0);

	Variant("instint",   98765,    0);
	Variant("instfloat", 98765.99, 0);

	Variant("staticint",   12345);
	Variant("staticfloat", 12345.0123);
}

CHECK(TypeConvChecker,
	  SCRIPT_BEGIN
	  "var inst = new TypeConvChecker();"

	  SCRIPT_EVAL_LOG("inst.Bool(true)  == true",   "TypeConvChecker::Bool(True)")
	  SCRIPT_EVAL_LOG("inst.Bool(false) == false",  "TypeConvChecker::Bool(False)")

	  SCRIPT_EVAL_LOG("inst.ConstCharP('Hoge') == 'Hoge'", "TypeConvChecker::ConstCharP(Hoge)")
	  SCRIPT_EVAL(     "inst.Name == 'TypeConvChecker'")

	  SCRIPT_LOG_CHECK("inst.Raw1()", "RawCallback1")
	  SCRIPT_LOG_CHECK("inst.Raw2()", "RawCallback2:TypeConvChecker")

	  // variant test
	  SCRIPT_EVAL(     "inst.instint   === 98765")
	  SCRIPT_EVAL(     "inst.instfloat === 98765.99")
	  SCRIPT_MUST_ERROR("inst.staticint   === 12345")
	  SCRIPT_MUST_ERROR("inst.staticfloat === 12345.0123")

	  "inst.instint = 0;"
	  "var inst2 = new TypeConvChecker();"
	  SCRIPT_EVAL(     "inst.instint    === 0")
	  SCRIPT_EVAL(     "inst2.instint   === 98765")
	  "invalidate inst2;"

	  SCRIPT_EVAL(     "TypeConvChecker.staticint   === 12345")
	  SCRIPT_EVAL(     "TypeConvChecker.staticfloat === 12345.0123")
	  SCRIPT_EVAL(     "TypeConvChecker.instint     === 98765")
	  SCRIPT_EVAL(     "TypeConvChecker.instfloat   === 98765.99")

	  "invalidate inst;"
	  "TypeConvChecker.instint = 0;"
	  "var inst = new TypeConvChecker();"
	  SCRIPT_EVAL(     "inst.instint    === 0")

	  "invalidate inst;"
	  SCRIPT_END);

////////////////////////////////////////

struct BoxingChecker {
	static TypeConvChecker& CheckRef(TypeConvChecker &ref) { return ref; }
	static TypeConvChecker* CheckPtr(TypeConvChecker *ref) { return ref; }
	static bool CheckConstRef(TypeConvChecker const &ref, bool b) { return ref.Bool(b);  }
	static bool CheckConstPtr(TypeConvChecker const *ref, bool b) { return ref->Bool(b); }
};
NCB_REGISTER_CLASS(BoxingChecker) {
	NCB_METHOD(CheckRef);
	NCB_METHOD(CheckPtr);
	NCB_METHOD(CheckConstRef);
	NCB_METHOD(CheckConstPtr);
}
CHECK(BoxingChecker,
	  SCRIPT_BEGIN
	  "var inst = new TypeConvChecker();"

	  SCRIPT_EVAL(    "BoxingChecker.CheckRef(inst).Name == 'TypeConvChecker'")
	  SCRIPT_EVAL(    "BoxingChecker.CheckPtr(inst).Name == 'TypeConvChecker'")

	  SCRIPT_EVAL_LOG("BoxingChecker.CheckConstRef(inst, true ) == true",  "TypeConvChecker::Bool(True)")
	  SCRIPT_EVAL_LOG("BoxingChecker.CheckConstPtr(inst, false) == false", "TypeConvChecker::Bool(False)")

	  "invalidate inst;"
	  SCRIPT_END);

////////////////////////////////////////

struct OverloadTest {
	static void Method(int a, int  b) { mes("Method(int, int) : ", a, ",", b); }
	static void Method(char const *p) { mes("Method(char const*) : ", p); }
};

NCB_REGISTER_CLASS(OverloadTest) {
	Method("Method1", method_cast<void, Static, int, int>(   &Class::Method));
	Method("Method2", method_cast<void, Static, char const*>(&Class::Method));
}

CHECK(OverloadTest,
	  SCRIPT_BEGIN

	  SCRIPT_LOG_CHECK("OverloadTest.Method1(123,456)",   "Method(int, int) : 123,456")
	  SCRIPT_LOG_CHECK("OverloadTest.Method2('abcdefg')", "Method(char const*) : abcdefg")

	  SCRIPT_MUST_ERROR("var inst = new OverloadTest()")
	  SCRIPT_END);


////////////////////////////////////////

struct PropertyTest {
	PropertyTest() {}
	int  Get() const { return i; }
	void Set(int n)   { mes("Set(", n, ")"); i = n; }

	static int  StaticGet()      { return s; }
	static void StaticSet(int n) { mes("StaticSet(", n, ")"); s = n; }
private:
	int i;
	static int s;
};
int PropertyTest::s = 0;

NCB_REGISTER_CLASS(PropertyTest) {
	Constructor();									//NCB_CONSTRUCTOR(());

	Property("Prop",   &Class::Get, &Class::Set);	//NCB_PROPERTY(   Prop,   Get, Set);
	Property("PropRO", &Class::Get, 0);				//NCB_PROPERTY_RO(PropRO, Get);
	Property("PropWO", 0, &Class::Set);				//NCB_PROPERTY_WO(PropWO, Set);

	Property("StaticProp",   &Class::StaticGet, &Class::StaticSet);	//NCB_PROPERTY(   StaticProp,   StaticGet, StaticSet);
	Property("StaticPropRO", &Class::StaticGet, 0);					//NCB_PROPERTY_RO(StaticPropRO, StaticGet);
	Property("StaticPropWO", 0, &Class::StaticSet);					//NCB_PROPERTY_WO(StaticPropWO, StaticSet);
}

CHECK(PropertyTest,
	  SCRIPT_BEGIN
	  "var inst = new PropertyTest();"
	  SCRIPT_LOG_CHECK("inst.Prop =   123", "Set(123)")
	  SCRIPT_EVAL(     "inst.Prop === 123")
	  SCRIPT_LOG_CHECK("inst.PropWO =   456", "Set(456)")
	  SCRIPT_EVAL(     "inst.PropRO === 456")


	  SCRIPT_LOG_CHECK("PropertyTest.StaticProp =   999", "StaticSet(999)")
	  SCRIPT_EVAL(     "PropertyTest.StaticProp === 999")
	  SCRIPT_LOG_CHECK("PropertyTest.StaticPropWO =   555", "StaticSet(555)")
	  SCRIPT_EVAL(     "PropertyTest.StaticPropRO === 555")

	  SCRIPT_MUST_ERROR("inst.PropRO =  333")
	  SCRIPT_MUST_ERROR("inst.PropWO == 333")
	  SCRIPT_MUST_ERROR("PropertyTest.StaticPropRO =  111")
	  SCRIPT_MUST_ERROR("PropertyTest.StaticPropWO == 111")

	  "invalidate inst;"
	  SCRIPT_END);

////////////////////////////////////////

static void GlobalFunctionTest1(int a, char const *b) {
	mes("GlobalFunctionTest1(", a, ",", b, ")");
}

static tjs_error TJS_INTF_METHOD
GlobalFunctionTest2(tTJSVariant *result,tjs_int numparams, tTJSVariant **param, iTJSDispatch2 *objthis) {
	mes("GlobalFunctionTest2(", numparams, ")");
	return TJS_S_OK;
}


NCB_REGISTER_FUNCTION(Function1, GlobalFunctionTest1);
NCB_REGISTER_FUNCTION(Function2, GlobalFunctionTest2);

CHECK(FunctionTest,
	  SCRIPT_BEGIN
	  SCRIPT_LOG_CHECK("Function1(123,'abc')", "GlobalFunctionTest1(123,abc)")
	  SCRIPT_LOG_CHECK("Function2(123,'abc')", "GlobalFunctionTest2(2)")
	  SCRIPT_END);



////////////////////////////////////////
// 既存のクラスに追加するクラスのテスト

struct PadAttachTest1 {
	PadAttachTest1()  { TVPAddLog(TJS_W("PadAttachTest1::Constructor")); }
	~PadAttachTest1() { TVPAddLog(TJS_W("PadAttachTest1::Destructor")); }
	void Test1() const {            mes("PadAttachTest1::Test"); }
};
struct PadAttachTest2 {
	PadAttachTest2()  { TVPAddLog(TJS_W("PadAttachTest2::Constructor")); }
	~PadAttachTest2() { TVPAddLog(TJS_W("PadAttachTest2::Destructor")); }
	void Test2() const {            mes("PadAttachTest2::Test"); }
	void Hooked() const {           mes("PadAttachTest2::Hooked"); }

	void SetObjthis(iTJSDispatch2 *ot) { _objthis = ot; }
private:
	iTJSDispatch2 *_objthis;
};

//--------------------------------------
// ネイティブインスタンスのポインタを取得する部分を独自に書き記したい場合のサンプル

NCB_GET_INSTANCE_HOOK(PadAttachTest2)
{
	// スコープ内ではあらかじめ typedef PadAttachTest2 ClassT; と定義されている

	// コンストラクタ（あまり使う意味無し）
	NCB_GET_INSTANCE_HOOK_CLASS () {
		//NCB_LOG_W("GetInstanceHook::Constructor");
	}

	// インスタンスゲッタ
	NCB_INSTANCE_GETTER(objthis) { // objthis を iTJSDispatch2* 型の引数とする
		//NCB_LOG_W("GetInstanceHook::Getter");

		// ポインタ取得
		ClassT* obj = GetNativeInstance(objthis); //< ネイティブインスタンス取得組み込み関数
		if (!obj) {
			// ない場合は生成する
			obj = new ClassT();

			// objthis に obj をネイティブインスタンスとして登録する（忘れると毎回 new されますよー）
			SetNativeInstance(objthis, obj); //< ネイティブインスタンス設定組み込み関数
		}

		// インスタンス側にobjthisを持たせてほげほげしたい場合はこんな感じで
		if (obj) obj->SetObjthis(objthis);

		// デストラクタで使用したい場合はプライベート変数に保存
		_objthis = objthis;
		_obj = obj; 

		return obj;
	}

	// デストラクタ（実際のメソッドが呼ばれた後に呼ばれる）
	~NCB_GET_INSTANCE_HOOK_CLASS () {
		//NCB_LOG_W("GetInstanceHook::Destructor");

		// Hookedメソッドを呼ぶ
		if (_obj) _obj->Hooked();
	}

private:
	iTJSDispatch2 *_objthis;
	ClassT        *_obj;

}; // 実体は class 定義なので ; を忘れないでね


/// 通常アタッチ（インスタンスはメソッドが初めて呼ばれる時にnewされる）
NCB_ATTACH_CLASS(PadAttachTest1, Pad) {
	Method("Test1", &Class::Test1); //NCB_METHOD(Test1);
}

// フックつきアタッチ（あらかじめ NCB_GET_INSTANCE_HOOK が定義されていること：ない場合はコンパイルエラー）
NCB_ATTACH_CLASS_WITH_HOOK(PadAttachTest2, Pad) {
	Method("Test2", &Class::Test2); //NCB_METHOD(Test2);
}

//--------------------------------------
// attach function テスト
static void AttachFunctionTest1(int d) {
	mes("AttachFunctionTest1(", d, ")");
}
static tjs_error TJS_INTF_METHOD
AttachFunctionTest2(tTJSVariant *result,tjs_int numparams, tTJSVariant **param, iTJSDispatch2 *objthis) {
	mes("AttachFunctionTest2: ", numparams);

	return TJS_S_OK;
}

NCB_ATTACH_FUNCTION(Func1, Pad, AttachFunctionTest1);
NCB_ATTACH_FUNCTION(Func2, Pad, AttachFunctionTest2);


CHECK(PadAttachTest,
	  SCRIPT_BEGIN
	  "var inst = new Pad();"
	  SCRIPT_LOG_CHECK("inst.Test1()", "PadAttachTest1::Test")
	  SCRIPT_LOG_CHECK("inst.Test2()", "PadAttachTest2::Hooked")

	  SCRIPT_LOG_CHECK("inst.Func1(321)",   "AttachFunctionTest1(321)")
	  SCRIPT_LOG_CHECK("inst.Func2(1,2,3)", "AttachFunctionTest2: 3")

	  "invalidate inst;"
	  SCRIPT_END);



////////////////////////////////////////
// Proxy / Bridge テスト

#include <string>
struct ProxyTest {
	ProxyTest(char const* tag) : _tag(tag) { mes("construct: ", tag); }
	char const* GetTag() const { mes("gettag: ", _tag.c_str()); return _tag.c_str(); }

	PropertyTest prop;
	TypeConvChecker tc;
private:
	::std::string _tag;
};

static ttstr GetTagProxy(ProxyTest *px) {
	return ttstr("Proxy:") + px->GetTag();
}
struct BridgeToProp     { PropertyTest*    operator()(ProxyTest* p) const { mes("Bridge->PropertyTest");    return &(p->prop); } };
struct BridgeToTypeConv { TypeConvChecker* operator()(ProxyTest* p) const { mes("Bridge->TypeConvChecker"); return &(p->tc);   } };

static signed int SIntProxy(TypeConvChecker* tc, signed int n) {
	mes("ProxySInt:", n);
	return tc->SInt(n);
}

static int PropGetProxy(PropertyTest* pt) {
	mes("ProxyPropGet:");
	return pt->Get();
}
static void PropSetProxy(PropertyTest* pt, int n) {
	mes("ProxyPropSet:", n);
	pt->Set(n);
}


NCB_REGISTER_CLASS(ProxyTest) {
	Constructor<char const*>(0);				//NCB_CONSTRUCTOR((char const*));
	Method("GetTag", &Class::GetTag);			//NCB_METHOD(GetTag);
	Method("GetTagProxy", &GetTagProxy, Proxy);	//NCB_METHOD_PROXY(GetTagProxy, GetTagProxy);

	Method("BridgeSInt",      &TypeConvChecker::SInt, Bridge<BridgeToTypeConv>());
	Method("BridgeSIntProxy", &SIntProxy,        ProxyBridge<BridgeToTypeConv>());

	Property("BridgeProp",      &PropertyTest::Get, &PropertyTest::Set, Bridge<BridgeToProp>());
	Property("BridgePropProxy", &PropGetProxy,      &PropSetProxy, ProxyBridge<BridgeToProp>());
}


CHECK(ProxyTest,
	  SCRIPT_BEGIN
	  "var inst = new ProxyTest('ProxyCheck');"
	  SCRIPT_EVAL("inst.GetTag() === 'ProxyCheck'")
	  SCRIPT_EVAL("inst.GetTagProxy() === 'Proxy:ProxyCheck'")

	  SCRIPT_EVAL("inst.BridgeSInt(-1234) === -1234")
	  SCRIPT_EVAL("inst.BridgeSIntProxy(8765) === 8765")

	  SCRIPT_EVAL("inst.BridgeProp      = 999, inst.BridgeProp      === 999")
	  SCRIPT_EVAL("inst.BridgePropProxy = 111, inst.BridgePropProxy === 111")

	  "invalidate inst;"
	  SCRIPT_END);


////////////////////////////////////////
// getRef tTJSDispatch
class RefCountDispatch : public tTJSDispatch {
public:
	RefCountDispatch() : tTJSDispatch() {}
	tjs_uint getRefCount() { return tTJSDispatch::GetRefCount(); }
};

////////////////////////////////////////
// PropAccessor test

struct AccessorTest : public ncbPropAccessor {
	AccessorTest() : ncbPropAccessor(TJSCreateArrayObject(), false) {
		TVPAddLog(TJS_W("construct testAccessor"));
	}
	~AccessorTest() {
		TVPAddLog(TJS_W("destruct testAccessor"));
	}
	int  IntGetValue(IndexT idx)        { return GetValue(idx, DefsT::Tag<int>()); }
	bool IntSetValue(IndexT idx, int v) { return SetValue(idx, v); }

	AccessorTest* New() { return new AccessorTest(); }

	static tjs_uint CopyConstruct() {
		RefCountDispatch *ref = new RefCountDispatch();
		tjs_uint r = 0;
		{
			tTJSVariant v(ref, ref);
			ncbPropAccessor prop = ncbPropAccessor(v);
		}
		r = ref->getRefCount();
		ref->Release();
		return r;
	}
};

NCB_REGISTER_CLASS(AccessorTest) {
	Constructor();
	Method("IntGetValue", &Class::IntGetValue);
	Method("IntSetValue", &Class::IntSetValue);
	Method("New",         &Class::New);
	Method("CopyConstruct", &Class::CopyConstruct);
}


CHECK(AccessorTest,
	  SCRIPT_BEGIN
	  SCRIPT_EVAL("AccessorTest.CopyConstruct() == 1")

	  "var inst = new AccessorTest();"
	  SCRIPT_EVAL("inst.IntSetValue(0, 100) != 0")
	  SCRIPT_EVAL("inst.IntGetValue(0)      == 100")
	  "var inst2 = inst.New();"
	  SCRIPT_EVAL("inst2.IntGetValue(1)     == 0")

	  "invalidate inst;"
	  "invalidate inst2;"
	  SCRIPT_END);




////////////////////////////////////////

void PreRegistCallbackTest()    { mes("PreRegistCallbackTest"); }
void PostRegistCallbackTest()   { mes("PostRegistCallbackTest"); }
void PreUnregistCallbackTest()  { mes("PreUnregistCallbackTest"); }
void PostUnregistCallbackTest() { mes("PostUnregistCallbackTest"); }

NCB_PRE_REGIST_CALLBACK(   PreRegistCallbackTest);
NCB_POST_REGIST_CALLBACK(  PostRegistCallbackTest);
NCB_PRE_UNREGIST_CALLBACK( PreUnregistCallbackTest);
NCB_POST_UNREGIST_CALLBACK(PostUnregistCallbackTest);




