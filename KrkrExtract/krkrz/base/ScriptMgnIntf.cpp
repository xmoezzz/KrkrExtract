//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// TJS2 Script Managing
//---------------------------------------------------------------------------

#include "tjsCommHead.h"

#include "tjs.h"
#include "tjsDebug.h"
#include "tjsArray.h"
#include "ScriptMgnIntf.h"
#include "StorageIntf.h"
#include "DebugIntf.h"
#include "WindowIntf.h"
#include "LayerIntf.h"
//#include "CDDAIntf.h"
//#include "MIDIIntf.h"
#include "WaveIntf.h"
#include "TimerIntf.h"
#include "EventIntf.h"
#include "SystemIntf.h"
#include "PluginIntf.h"
//#include "MenuItemIntf.h"
#include "ClipboardIntf.h"
#include "MsgIntf.h"
//#include "KAGParser.h"
#include "VideoOvlIntf.h"
//#include "PadIntf.h"
#include "TextStream.h"
#include "Random.h"
#include "tjsRandomGenerator.h"
#include "SysInitIntf.h"
#include "PhaseVocoderFilter.h"
#include "BasicDrawDevice.h"
#include "BinaryStream.h"
#include "SysInitImpl.h"
#include "SystemControl.h"
#include "Application.h"

#include "RectItf.h"
#include "ImageFunction.h"
#include "BitmapIntf.h"
#include "tjsScriptBlock.h"
#include "ApplicationSpecialPath.h"
#include "SystemImpl.h"
#include "BitmapLayerTreeOwner.h"
#include "Extension.h"

//---------------------------------------------------------------------------
// Script system initialization script
//---------------------------------------------------------------------------
static const tjs_nchar * TVPInitTJSScript =
	// note that this script is stored as narrow string
TJS_N("const\
\
/* constants */\
 /* tTVPBorderStyle */ bsNone=0,  bsSingle=1,  bsSizeable=2,  bsDialog=3,  bsToolWindow=4,  bsSizeToolWin=5,\
 /* tTVPUpdateType */ utNormal=0,  utEntire =1,\
 /* tTVPMouseButton */  mbLeft=0,  mbRight=1,  mbMiddle=2, mbX1=3, mbX2=4,\
 /* tTVPMouseCursorState */ mcsVisible=0, mcsTempHidden=1, mcsHidden=2,\
 /* tTVPImeMode */ imDisable=0, imClose=1, imOpen=2, imDontCare=3, imSAlpha=4, imAlpha=5, imHira=6, imSKata=7, imKata=8, imChinese=9, imSHanguel=10, imHanguel=11,\
 /* Set of shift state */  ssShift=(1<<0),  ssAlt=(1<<1),  ssCtrl=(1<<2),  ssLeft=(1<<3),  ssRight=(1<<4),  ssMiddle=(1<<5),  ssDouble =(1<<6),  ssRepeat = (1<<7),\
 /* TVP_FSF_???? */ fsfFixedPitch=1, fsfSameCharSet=2, fsfNoVertical=4, \
	fsfTrueTypeOnly=8, fsfUseFontFace=0x100, fsfIgnoreSymbol=0x10,\
 /* tTVPLayerType */ ltBinder=0, ltCoverRect=1, ltOpaque=1, ltTransparent=2, ltAlpha=2, ltAdditive=3, ltSubtractive=4, ltMultiplicative=5, ltEffect=6, ltFilter=7, ltDodge=8, ltDarken=9, ltLighten=10, ltScreen=11, ltAddAlpha = 12,\
	ltPsNormal = 13, ltPsAdditive = 14, ltPsSubtractive = 15, ltPsMultiplicative = 16, ltPsScreen = 17, ltPsOverlay = 18, ltPsHardLight = 19, ltPsSoftLight = 20, ltPsColorDodge = 21, ltPsColorDodge5 = 22, ltPsColorBurn = 23, ltPsLighten = 24, ltPsDarken = 25, ltPsDifference = 26, ltPsDifference5 = 27, ltPsExclusion = 28, \
 /* tTVPBlendOperationMode */ omPsNormal = ltPsNormal,omPsAdditive = ltPsAdditive,omPsSubtractive = ltPsSubtractive,omPsMultiplicative = ltPsMultiplicative,omPsScreen = ltPsScreen,omPsOverlay = ltPsOverlay,omPsHardLight = ltPsHardLight,omPsSoftLight = ltPsSoftLight,omPsColorDodge = ltPsColorDodge,omPsColorDodge5 = ltPsColorDodge5,omPsColorBurn = ltPsColorBurn,omPsLighten = ltPsLighten,omPsDarken = ltPsDarken,omPsDifference = ltPsDifference,omPsDifference5 = ltPsDifference5,omPsExclusion = ltPsExclusion, \
	omAdditive=ltAdditive, omSubtractive=ltSubtractive, omMultiplicative=ltMultiplicative, omDodge=ltDodge, omDarken=ltDarken, omLighten=ltLighten, omScreen=ltScreen, omAddAlpha=ltAddAlpha, omOpaque=ltOpaque, omAlpha=ltAlpha, omAuto = 128,\
 /* tTVPDrawFace */ dfBoth=0, dfAlpha = dfBoth, dfAddAlpha = 4, dfMain=1, dfOpaque = dfMain, dfMask=2, dfProvince=3, dfAuto=128,\
 /* tTVPHitType */ htMask=0, htProvince=1,\
 /* tTVPScrollTransFrom */ sttLeft=0, sttTop=1, sttRight=2, sttBottom=3,\
 /* tTVPScrollTransStay */ ststNoStay=0, ststStayDest=1, ststStaySrc=2, \
 /* tTVPKAGDebugLevel */ tkdlNone=0, tkdlSimple=1, tkdlVerbose=2, \
 /* tTVPAsyncTriggerMode */	atmNormal=0, atmExclusive=1, atmAtIdle=2, \
 /* tTVPBBStretchType */ stNearest=0, stFastLinear=1, stLinear=2, stCubic=3, stSemiFastLinear = 4, stFastCubic = 5, stLanczos2 = 6, stFastLanczos2 = 7, stLanczos3 = 8, stFastLanczos3 = 9, stSpline16 = 10, stFastSpline16 = 11, stSpline36 = 12, stFastSpline36 = 13, stAreaAvg = 14, stFastAreaAvg = 15, stGaussian = 16, stFastGaussian = 17, stBlackmanSinc = 18, stFastBlackmanSinc = 19, stRefNoClip = 0x10000,\
 /* tTVPClipboardFormat */ cbfText = 1,\
 /* TVP_COMPACT_LEVEL_???? */ clIdle = 5, clDeactivate = 10, clMinimize = 15, clAll = 100,\
 /* tTVPVideoOverlayMode Add: T.Imoto */ vomOverlay=0, vomLayer=1, vomMixer=2, vomMFEVR=3,\
 /* tTVPPeriodEventReason */ perLoop = 0, perPeriod = 1, perPrepare = 2, perSegLoop = 3,\
 /* tTVPSoundGlobalFocusMode */ sgfmNeverMute = 0, sgfmMuteOnMinimize = 1, sgfmMuteOnDeactivate = 2,\
 /* tTVPTouchDevice */ tdNone=0, tdIntegratedTouch=0x01, tdExternalTouch=0x02, tdIntegratedPen=0x04, tdExternalPen=0x08, tdMultiInput=0x40, tdDigitizerReady=0x80,\
    tdMouse=0x0100, tdMouseWheel=0x0200,\
 /* Display Orientation */ oriUnknown=0, oriPortrait=1, oriLandscape=2,\
\
/* file attributes */\
 faReadOnly=0x01, faHidden=0x02, faSysFile=0x04, faVolumeID=0x08, faDirectory=0x10, faArchive=0x20, faAnyFile=0x3f,\
/* mouse cursor constants */\
 crDefault = 0x0,\
 crNone = -1,\
 crArrow = -2,\
 crCross = -3,\
 crIBeam = -4,\
 crSize = -5,\
 crSizeNESW = -6,\
 crSizeNS = -7,\
 crSizeNWSE = -8,\
 crSizeWE = -9,\
 crUpArrow = -10,\
 crHourGlass = -11,\
 crDrag = -12,\
 crNoDrop = -13,\
 crHSplit = -14,\
 crVSplit = -15,\
 crMultiDrag = -16,\
 crSQLWait = -17,\
 crNo = -18,\
 crAppStart = -19,\
 crHelp = -20,\
 crHandPoint = -21,\
 crSizeAll = -22,\
 crHBeam = 1,\
/* color constants */\
 clScrollBar = 0x80000000,\
 clBackground = 0x80000001,\
 clActiveCaption = 0x80000002,\
 clInactiveCaption = 0x80000003,\
 clMenu = 0x80000004,\
 clWindow = 0x80000005,\
 clWindowFrame = 0x80000006,\
 clMenuText = 0x80000007,\
 clWindowText = 0x80000008,\
 clCaptionText = 0x80000009,\
 clActiveBorder = 0x8000000a,\
 clInactiveBorder = 0x8000000b,\
 clAppWorkSpace = 0x8000000c,\
 clHighlight = 0x8000000d,\
 clHighlightText = 0x8000000e,\
 clBtnFace = 0x8000000f,\
 clBtnShadow = 0x80000010,\
 clGrayText = 0x80000011,\
 clBtnText = 0x80000012,\
 clInactiveCaptionText = 0x80000013,\
 clBtnHighlight = 0x80000014,\
 cl3DDkShadow = 0x80000015,\
 cl3DLight = 0x80000016,\
 clInfoText = 0x80000017,\
 clInfoBk = 0x80000018,\
 clNone = 0x1fffffff,\
 clAdapt= 0x01ffffff,\
 clPalIdx = 0x3000000,\
 clAlphaMat = 0x4000000,\
/* for Menu.trackPopup (see winuser.h) */\
 tpmLeftButton      = 0x0000,\
 tpmRightButton     = 0x0002,\
 tpmLeftAlign       = 0x0000,\
 tpmCenterAlign     = 0x0004,\
 tpmRightAlign      = 0x0008,\
 tpmTopAlign        = 0x0000,\
 tpmVCenterAlign    = 0x0010,\
 tpmBottomAlign     = 0x0020,\
 tpmHorizontal      = 0x0000,\
 tpmVertical        = 0x0040,\
 tpmNoNotify        = 0x0080,\
 tpmReturnCmd       = 0x0100,\
 tpmRecurse         = 0x0001,\
 tpmHorPosAnimation = 0x0400,\
 tpmHorNegAnimation = 0x0800,\
 tpmVerPosAnimation = 0x1000,\
 tpmVerNegAnimation = 0x2000,\
 tpmNoAnimation     = 0x4000,\
/* for Pad.showScrollBars (see Vcl/stdctrls.hpp :: enum TScrollStyle) */\
 ssNone       = 0,\
 ssHorizontal = 1,\
 ssVertical   = 2,\
 ssBoth       = 3,\
/* virtual keycodes */\
 VK_LBUTTON =0x01,\
 VK_RBUTTON =0x02,\
 VK_CANCEL =0x03,\
 VK_MBUTTON =0x04,\
 VK_BACK =0x08,\
 VK_TAB =0x09,\
 VK_CLEAR =0x0C,\
 VK_RETURN =0x0D,\
 VK_SHIFT =0x10,\
 VK_CONTROL =0x11,\
 VK_MENU =0x12,\
 VK_PAUSE =0x13,\
 VK_CAPITAL =0x14,\
 VK_KANA =0x15,\
 VK_HANGEUL =0x15,\
 VK_HANGUL =0x15,\
 VK_JUNJA =0x17,\
 VK_FINAL =0x18,\
 VK_HANJA =0x19,\
 VK_KANJI =0x19,\
 VK_ESCAPE =0x1B,\
 VK_CONVERT =0x1C,\
 VK_NONCONVERT =0x1D,\
 VK_ACCEPT =0x1E,\
 VK_MODECHANGE =0x1F,\
 VK_SPACE =0x20,\
 VK_PRIOR =0x21,\
 VK_NEXT =0x22,\
 VK_END =0x23,\
 VK_HOME =0x24,\
 VK_LEFT =0x25,\
 VK_UP =0x26,\
 VK_RIGHT =0x27,\
 VK_DOWN =0x28,\
 VK_SELECT =0x29,\
 VK_PRINT =0x2A,\
 VK_EXECUTE =0x2B,\
 VK_SNAPSHOT =0x2C,\
 VK_INSERT =0x2D,\
 VK_DELETE =0x2E,\
 VK_HELP =0x2F,\
 VK_0 =0x30,\
 VK_1 =0x31,\
 VK_2 =0x32,\
 VK_3 =0x33,\
 VK_4 =0x34,\
 VK_5 =0x35,\
 VK_6 =0x36,\
 VK_7 =0x37,\
 VK_8 =0x38,\
 VK_9 =0x39,\
 VK_A =0x41,\
 VK_B =0x42,\
 VK_C =0x43,\
 VK_D =0x44,\
 VK_E =0x45,\
 VK_F =0x46,\
 VK_G =0x47,\
 VK_H =0x48,\
 VK_I =0x49,\
 VK_J =0x4A,\
 VK_K =0x4B,\
 VK_L =0x4C,\
 VK_M =0x4D,\
 VK_N =0x4E,\
 VK_O =0x4F,\
 VK_P =0x50,\
 VK_Q =0x51,\
 VK_R =0x52,\
 VK_S =0x53,\
 VK_T =0x54,\
 VK_U =0x55,\
 VK_V =0x56,\
 VK_W =0x57,\
 VK_X =0x58,\
 VK_Y =0x59,\
 VK_Z =0x5A,\
 VK_LWIN =0x5B,\
 VK_RWIN =0x5C,\
 VK_APPS =0x5D,\
 VK_NUMPAD0 =0x60,\
 VK_NUMPAD1 =0x61,\
 VK_NUMPAD2 =0x62,\
 VK_NUMPAD3 =0x63,\
 VK_NUMPAD4 =0x64,\
 VK_NUMPAD5 =0x65,\
 VK_NUMPAD6 =0x66,\
 VK_NUMPAD7 =0x67,\
 VK_NUMPAD8 =0x68,\
 VK_NUMPAD9 =0x69,\
 VK_MULTIPLY =0x6A,\
 VK_ADD =0x6B,\
 VK_SEPARATOR =0x6C,\
 VK_SUBTRACT =0x6D,\
 VK_DECIMAL =0x6E,\
 VK_DIVIDE =0x6F,\
 VK_F1 =0x70,\
 VK_F2 =0x71,\
 VK_F3 =0x72,\
 VK_F4 =0x73,\
 VK_F5 =0x74,\
 VK_F6 =0x75,\
 VK_F7 =0x76,\
 VK_F8 =0x77,\
 VK_F9 =0x78,\
 VK_F10 =0x79,\
 VK_F11 =0x7A,\
 VK_F12 =0x7B,\
 VK_F13 =0x7C,\
 VK_F14 =0x7D,\
 VK_F15 =0x7E,\
 VK_F16 =0x7F,\
 VK_F17 =0x80,\
 VK_F18 =0x81,\
 VK_F19 =0x82,\
 VK_F20 =0x83,\
 VK_F21 =0x84,\
 VK_F22 =0x85,\
 VK_F23 =0x86,\
 VK_F24 =0x87,\
 VK_NUMLOCK =0x90,\
 VK_SCROLL =0x91,\
 VK_LSHIFT =0xA0,\
 VK_RSHIFT =0xA1,\
 VK_LCONTROL =0xA2,\
 VK_RCONTROL =0xA3,\
 VK_LMENU =0xA4,\
 VK_RMENU =0xA5,\
/* VK_PADXXXX are KIRIKIRI specific */\
 VK_PADLEFT =0x1B5,\
 VK_PADUP =0x1B6,\
 VK_PADRIGHT =0x1B7,\
 VK_PADDOWN =0x1B8,\
 VK_PAD1 =0x1C0,\
 VK_PAD2 =0x1C1,\
 VK_PAD3 =0x1C2,\
 VK_PAD4 =0x1C3,\
 VK_PAD5 =0x1C4,\
 VK_PAD6 =0x1C5,\
 VK_PAD7 =0x1C6,\
 VK_PAD8 =0x1C7,\
 VK_PAD9 =0x1C8,\
 VK_PAD10 =0x1C9,\
 VK_PADANY = 0x1DF,\
 VK_PROCESSKEY =0xE5,\
 VK_ATTN =0xF6,\
 VK_CRSEL =0xF7,\
 VK_EXSEL =0xF8,\
 VK_EREOF =0xF9,\
 VK_PLAY =0xFA,\
 VK_ZOOM =0xFB,\
 VK_NONAME =0xFC,\
 VK_PA1 =0xFD,\
 VK_OEM_CLEAR =0xFE,\
 frFreeType=0,\
 frGDI=1,\
/* graphic cache system */\
 gcsAuto=-1,\
/* image 'mode' tag (mainly is generated by image format converter) constants */\
 imageTagLayerType = %[\
opaque		:%[type:ltOpaque			],\
rect		:%[type:ltOpaque			],\
alpha		:%[type:ltAlpha				],\
transparent	:%[type:ltAlpha				],\
addalpha	:%[type:ltAddAlpha			],\
add			:%[type:ltAdditive			],\
sub			:%[type:ltSubtractive		],\
mul			:%[type:ltMultiplicative	],\
dodge		:%[type:ltDodge				],\
darken		:%[type:ltDarken			],\
lighten		:%[type:ltLighten			],\
screen		:%[type:ltScreen			],\
psnormal	:%[type:ltPsNormal			],\
psadd		:%[type:ltPsAdditive		],\
pssub		:%[type:ltPsSubtractive		],\
psmul		:%[type:ltPsMultiplicative	],\
psscreen	:%[type:ltPsScreen			],\
psoverlay	:%[type:ltPsOverlay			],\
pshlight	:%[type:ltPsHardLight		],\
psslight	:%[type:ltPsSoftLight		],\
psdodge		:%[type:ltPsColorDodge		],\
psdodge5	:%[type:ltPsColorDodge5		],\
psburn		:%[type:ltPsColorBurn		],\
pslighten	:%[type:ltPsLighten			],\
psdarken	:%[type:ltPsDarken			],\
psdiff		:%[type:ltPsDifference		],\
psdiff5		:%[type:ltPsDifference5		],\
psexcl		:%[type:ltPsExclusion		],\
],\
/* draw thread num */\
 dtnAuto=0\
;");
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// global variables
//---------------------------------------------------------------------------
tTJS *TVPScriptEngine = NULL;
ttstr TVPStartupScriptName(TJS_W("startup.tjs"));
static ttstr TVPScriptTextEncoding(TJS_W("UTF-8"));
//---------------------------------------------------------------------------

bool TVPCreateWithDll32(ttstr& process, ttstr& dllname);

//---------------------------------------------------------------------------
// Garbage Collection stuff
//---------------------------------------------------------------------------
class tTVPTJSGCCallback : public tTVPCompactEventCallbackIntf
{
	void TJS_INTF_METHOD OnCompact(tjs_int level)
	{
		// OnCompact method from tTVPCompactEventCallbackIntf
		// called when the application is idle, deactivated, minimized, or etc...
		if(TVPScriptEngine)
		{
			if(level >= TVP_COMPACT_LEVEL_IDLE)
			{
				TVPScriptEngine->DoGarbageCollection();
			}
		}
	}
} static TVPTJSGCCallback;
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// TVPInitScriptEngine
//---------------------------------------------------------------------------
static bool TVPScriptEngineInit = false;
void TVPInitScriptEngine()
{
	if(TVPScriptEngineInit) return;
	TVPScriptEngineInit = true;

	tTJSVariant val;

	// Set eval expression mode
	if(TVPGetCommandLine(TJS_W("-evalcontext"), &val) )
	{
		ttstr str(val);
		if(str == TJS_W("global"))
		{
			TJSEvalOperatorIsOnGlobal = true;
			TJSWarnOnNonGlobalEvalOperator = true;
		}
	}

	// Set igonre-prop compat mode
	if(TVPGetCommandLine(TJS_W("-unaryaster"), &val) )
	{
		ttstr str(val);
		if(str == TJS_W("compat"))
		{
			TJSUnaryAsteriskIgnoresPropAccess = true;
		}
	}

	// Set debug mode
	if(TVPGetCommandLine(TJS_W("-debug"), &val) )
	{
		ttstr str(val);
		if(str == TJS_W("yes"))
		{
			TJSEnableDebugMode = true;
			TVPAddImportantLog((const tjs_char *)TVPWarnDebugOptionEnabled);
//			if(TVPGetCommandLine(TJS_W("-warnrundelobj"), &val) )
//			{
//				str = val;
//				if(str == TJS_W("yes"))
//				{
					TJSWarnOnExecutionOnDeletingObject = true;
//				}
//			}
		}
	}
	// Set Read text encoding
	if(TVPGetCommandLine(TJS_W("-readencoding"), &val) )
	{
		ttstr str(val);
		TVPSetDefaultReadEncoding( str );
	}
	TVPScriptTextEncoding = ttstr(TVPGetDefaultReadEncoding());

#ifdef TVP_START_UP_SCRIPT_NAME
	TVPStartupScriptName = TVP_START_UP_SCRIPT_NAME;
#else
	// Set startup script name
	if(TVPGetCommandLine(TJS_W("-startup"), &val) )
	{
		ttstr str(val);
		TVPStartupScriptName = str;
	}
#endif

	// create script engine object
	TVPScriptEngine = new tTJS();

	// add kirikiriz
	TVPScriptEngine->SetPPValue( TJS_W("kirikiriz"), 1 );

	// set TJSGetRandomBits128
	TJSGetRandomBits128 = TVPGetRandomBits128;

	// script system initialization
	TVPScriptEngine->ExecScript(ttstr(TVPInitTJSScript));

	// set console output gateway handler
	TVPScriptEngine->SetConsoleOutput(TVPGetTJS2ConsoleOutputGateway());


	// set text stream functions
	TJSCreateTextStreamForRead = TVPCreateTextStreamForRead;
	TJSCreateTextStreamForWrite = TVPCreateTextStreamForWrite;
	
	// set binary stream functions
	TJSCreateBinaryStreamForRead = TVPCreateBinaryStreamForRead;
	TJSCreateBinaryStreamForWrite = TVPCreateBinaryStreamForWrite;

	// register some TVP classes/objects/functions/propeties
	iTJSDispatch2 *dsp;
	iTJSDispatch2 *global = TVPScriptEngine->GetGlobalNoAddRef();


#define REGISTER_OBJECT(classname, instance) \
	dsp = (instance); \
	val = tTJSVariant(dsp/*, dsp*/); \
	dsp->Release(); \
	global->PropSet(TJS_MEMBERENSURE|TJS_IGNOREPROP, TJS_W(#classname), NULL, \
		&val, global);

	/* classes */
	REGISTER_OBJECT(Debug, TVPCreateNativeClass_Debug());
	REGISTER_OBJECT(Font, TVPCreateNativeClass_Font());
	REGISTER_OBJECT(Layer, TVPCreateNativeClass_Layer());
	REGISTER_OBJECT(Timer, TVPCreateNativeClass_Timer());
	REGISTER_OBJECT(AsyncTrigger, TVPCreateNativeClass_AsyncTrigger());
	REGISTER_OBJECT(System, TVPCreateNativeClass_System());
	REGISTER_OBJECT(Storages, TVPCreateNativeClass_Storages());
	REGISTER_OBJECT(Plugins, TVPCreateNativeClass_Plugins());
	REGISTER_OBJECT(VideoOverlay, TVPCreateNativeClass_VideoOverlay());
	REGISTER_OBJECT(Clipboard, TVPCreateNativeClass_Clipboard());
	REGISTER_OBJECT(Scripts, TVPCreateNativeClass_Scripts()); // declared in this file
	REGISTER_OBJECT(Rect, TVPCreateNativeClass_Rect());
	REGISTER_OBJECT(Bitmap, TVPCreateNativeClass_Bitmap());
	REGISTER_OBJECT(ImageFunction, TVPCreateNativeClass_ImageFunction());
	REGISTER_OBJECT(BitmapLayerTreeOwner, TVPCreateNativeClass_BitmapLayerTreeOwner());

	/* WaveSoundBuffer and its filters */
	iTJSDispatch2 * waveclass = NULL;
	REGISTER_OBJECT(WaveSoundBuffer, (waveclass = TVPCreateNativeClass_WaveSoundBuffer()));
	dsp = new tTJSNC_PhaseVocoder();
	val = tTJSVariant(dsp);
	dsp->Release();
	waveclass->PropSet(TJS_MEMBERENSURE|TJS_IGNOREPROP|TJS_STATICMEMBER,
		TJS_W("PhaseVocoder"), NULL, &val, waveclass);

	/* Window and its drawdevices */
	iTJSDispatch2 * windowclass = NULL;
	REGISTER_OBJECT(Window, (windowclass = TVPCreateNativeClass_Window()));
	dsp = new tTJSNC_BasicDrawDevice();
	val = tTJSVariant(dsp);
	dsp->Release();
	windowclass->PropSet(TJS_MEMBERENSURE|TJS_IGNOREPROP|TJS_STATICMEMBER,
		TJS_W("BasicDrawDevice"), NULL, &val, windowclass);

	// Add Extension Classes
	TVPCauseAtInstallExtensionClass( global );

	// Garbage Collection Hook
	TVPAddCompactEventHook(&TVPTJSGCCallback);
}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// TVPUninitScriptEngine
//---------------------------------------------------------------------------
static bool TVPScriptEngineUninit = false;
void TVPUninitScriptEngine()
{
	if(TVPScriptEngineUninit) return;
	TVPScriptEngineUninit = true;

	TVPScriptEngine->Shutdown();
	TVPScriptEngine->Release();
	/*
		Objects, theirs lives are contolled by reference counter, may not be all
		freed here in some occations.
	*/
	TVPScriptEngine = NULL;
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// TVPRestartScriptEngine
//---------------------------------------------------------------------------
void TVPRestartScriptEngine()
{
	TVPUninitScriptEngine();
	TVPScriptEngineInit = false;
	TVPInitScriptEngine();
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// TVPGetScriptEngine
//---------------------------------------------------------------------------
tTJS * TVPGetScriptEngine()
{
	return TVPScriptEngine;
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// TVPGetScriptDispatch
//---------------------------------------------------------------------------
iTJSDispatch2 * TVPGetScriptDispatch()
{
	if(TVPScriptEngine) return TVPScriptEngine->GetGlobal(); else return NULL;
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// TVPExecuteScript
//---------------------------------------------------------------------------
void TVPExecuteScript(const ttstr& content, tTJSVariant *result)
{
	if(TVPScriptEngine)
		TVPScriptEngine->ExecScript(content, result);
	else
		TVPThrowInternalError;
}
//---------------------------------------------------------------------------
void TVPExecuteScript(const ttstr& content, const ttstr &name, tjs_int lineofs, tTJSVariant *result)
{
	if(TVPScriptEngine)
		TVPScriptEngine->ExecScript(content, result, NULL, &name, lineofs);
	else
		TVPThrowInternalError;
}
//---------------------------------------------------------------------------
void TVPExecuteScript(const ttstr& content, iTJSDispatch2 *context, tTJSVariant *result)
{
	if(TVPScriptEngine)
		TVPScriptEngine->ExecScript(content, result, context);
	else
		TVPThrowInternalError;
}
//---------------------------------------------------------------------------
void TVPExecuteScript(const ttstr& content, const ttstr &name, tjs_int lineofs, iTJSDispatch2 *context, tTJSVariant *result)
{
	if(TVPScriptEngine)
		TVPScriptEngine->ExecScript(content, result, context, &name, lineofs);
	else
		TVPThrowInternalError;
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// TVPExecuteExpression
//---------------------------------------------------------------------------
void TVPExecuteExpression(const ttstr& content, tTJSVariant *result)
{
	TVPExecuteExpression(content, NULL, result);
}
//---------------------------------------------------------------------------
void TVPExecuteExpression(const ttstr& content, const ttstr &name, tjs_int lineofs, tTJSVariant *result)
{
	TVPExecuteExpression(content, name, lineofs, NULL, result);
}
//---------------------------------------------------------------------------
void TVPExecuteExpression(const ttstr& content, iTJSDispatch2 *context, tTJSVariant *result)
{
	if(TVPScriptEngine)
	{
		iTJSConsoleOutput *output = TVPScriptEngine->GetConsoleOutput();
		TVPScriptEngine->SetConsoleOutput(NULL); // once set TJS console to null
		try
		{
			TVPScriptEngine->EvalExpression(content, result, context);
		}
		catch(...)
		{
			TVPScriptEngine->SetConsoleOutput(output);
			throw;
		}
		TVPScriptEngine->SetConsoleOutput(output);
	}
	else
	{
		TVPThrowInternalError;
	}
}
//---------------------------------------------------------------------------
void TVPExecuteExpression(const ttstr& content, const ttstr &name, tjs_int lineofs, iTJSDispatch2 *context, tTJSVariant *result)
{
	if(TVPScriptEngine)
	{
		iTJSConsoleOutput *output = TVPScriptEngine->GetConsoleOutput();
		TVPScriptEngine->SetConsoleOutput(NULL); // once set TJS console to null
		try
		{
			TVPScriptEngine->EvalExpression(content, result, context, &name, lineofs);
		}
		catch(...)
		{
			TVPScriptEngine->SetConsoleOutput(output);
			throw;
		}
		TVPScriptEngine->SetConsoleOutput(output);
	}
	else
	{
		TVPThrowInternalError;
	}
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// TVPExecuteBytecode
//---------------------------------------------------------------------------
void TVPExecuteBytecode( const tjs_uint8* content, size_t len, iTJSDispatch2 *context, tTJSVariant *result, const tjs_char *name )
{
	if(!TVPScriptEngine) TVPThrowInternalError;

	TVPScriptEngine->LoadByteCode( content, len, result, context, name);
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
void TVPExecuteStorage(const ttstr &name, tTJSVariant *result, bool isexpression,
	const tjs_char * modestr)
{
	TVPExecuteStorage(name, NULL, result, isexpression, modestr);
}
//---------------------------------------------------------------------------
void TVPExecuteStorage(const ttstr &name, iTJSDispatch2 *context, tTJSVariant *result, bool isexpression,
	const tjs_char * modestr)
{
	// execute storage which contains script
	if(!TVPScriptEngine) TVPThrowInternalError;
	
	{ // for bytecode
		ttstr place(TVPSearchPlacedPath(name));
		ttstr shortname(TVPExtractStorageName(place));
		tTJSBinaryStream* stream = TVPCreateBinaryStreamForRead(place, modestr);
		if( stream ) {
			bool isbytecode = false;
			try {
				isbytecode = TVPScriptEngine->LoadByteCode( stream, result, context, shortname.c_str() );
			} catch(...) {
				delete stream;
				throw;
			}
			delete stream;
			if( isbytecode ) return;
		}
	}

	ttstr place(TVPSearchPlacedPath(name));
	ttstr shortname(TVPExtractStorageName(place));

	iTJSTextReadStream * stream = TVPCreateTextStreamForReadByEncoding(place, modestr,TVPScriptTextEncoding);
	ttstr buffer;
	try
	{
		stream->Read(buffer, 0);
	}
	catch(...)
	{
		stream->Destruct();
		throw;
	}
	stream->Destruct();

	if(TVPScriptEngine)
	{
		if(!isexpression)
			TVPScriptEngine->ExecScript(buffer, result, context,
				&shortname);
		else
			TVPScriptEngine->EvalExpression(buffer, result, context,
				&shortname);
	}
}
//---------------------------------------------------------------------------
void TVPCompileStorage( const ttstr& name, bool isrequestresult, bool outputdebug, bool isexpression, const ttstr& outputpath ) {
	// execute storage which contains script
	if(!TVPScriptEngine) TVPThrowInternalError;

	ttstr place(TVPSearchPlacedPath(name));
	ttstr shortname(TVPExtractStorageName(place));
	iTJSTextReadStream * stream = TVPCreateTextStreamForReadByEncoding(place, TJS_W(""),TVPScriptTextEncoding);

	ttstr buffer;
	try {
		stream->Read(buffer, 0);
	} catch(...) {
		stream->Destruct();
		throw;
	}
	stream->Destruct();

	tTJSBinaryStream* outputstream = TVPCreateStream(outputpath, TJS_BS_WRITE);
	if(TVPScriptEngine) {
		try {
			TVPScriptEngine->CompileScript( buffer.c_str(), outputstream, isrequestresult, outputdebug, isexpression, name.c_str(), 0 );
		} catch(...) {
			delete outputstream;
			throw;
		}
	}
	delete outputstream;
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// TVPCreateMessageMapFile
//---------------------------------------------------------------------------
void TVPCreateMessageMapFile(const ttstr &filename)
{
#ifdef TJS_TEXT_OUT_CRLF
	ttstr script(TJS_W("{\r\n\tvar r = System.assignMessage;\r\n"));
#else
	ttstr script(TJS_W("{\n\tvar r = System.assignMessage;\n"));
#endif

	script += TJSCreateMessageMapString();

	script += TJS_W("}");

	iTJSTextWriteStream * stream = TVPCreateTextStreamForWrite(
		filename, TJS_W(""));
	try
	{
		stream->Write(script);
	}
	catch(...)
	{
		stream->Destruct();
		throw;
	}

	stream->Destruct();
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// TVPDumpScriptEngine
//---------------------------------------------------------------------------
void TVPDumpScriptEngine()
{
	TVPTJS2StartDump();
	TVPScriptEngine->SetConsoleOutput(TVPGetTJS2DumpOutputGateway());
	try
	{
		TVPScriptEngine->Dump();
	}
	catch(...)
	{
		TVPTJS2EndDump();
		TVPScriptEngine->SetConsoleOutput(TVPGetTJS2ConsoleOutputGateway());
		throw;
	}
	TVPScriptEngine->SetConsoleOutput(TVPGetTJS2ConsoleOutputGateway());
	TVPTJS2EndDump();
}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// TVPExecuteStartupScript
//---------------------------------------------------------------------------
void TVPExecuteStartupScript()
{
	// execute "startup.tjs"
	try
	{
		try
		{
			TVPAddLog( TVPInfoLoadingStartupScript + TVPStartupScriptName );
			TVPExecuteStorage(TVPStartupScriptName);
			TVPAddLog( (const tjs_char*)TVPInfoStartupScriptEnded );
		}
		TJS_CONVERT_TO_TJS_EXCEPTION
	}
	TVP_CATCH_AND_SHOW_SCRIPT_EXCEPTION(TJS_W("startup"))
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// unhandled exception handler related
//---------------------------------------------------------------------------
static bool  TJSGetSystem_exceptionHandler_Object(tTJSVariantClosure & dest)
{
	// get System.exceptionHandler
	iTJSDispatch2 * global = TVPGetScriptEngine()->GetGlobalNoAddRef();
	if(!global) return false;

	tTJSVariant val;
	tTJSVariant val2;
	tTJSVariantClosure clo;

	tjs_error er;
	er = global->PropGet(TJS_MEMBERMUSTEXIST, TJS_W("System"), NULL, &val, global);
	if(TJS_FAILED(er)) return false;

	if(val.Type() != tvtObject) return false;

	clo = val.AsObjectClosureNoAddRef();

	if(clo.Object == NULL) return false;

	clo.PropGet(TJS_MEMBERMUSTEXIST, TJS_W("exceptionHandler"), NULL, &val2, NULL);

	if(val2.Type() != tvtObject) return false;

	dest = val2.AsObjectClosure();

	if(!dest.Object)
	{
		dest.Release();
		return false;
	}

	return true;
}
//---------------------------------------------------------------------------
bool TVPProcessUnhandledException(eTJSScriptException &e)
{
	bool result;
	tTJSVariantClosure clo;
	clo.Object = clo.ObjThis = NULL;

	try
	{
		// get the script engine
		tTJS *engine = TVPGetScriptEngine();
		if(!engine)
			return false; // the script engine had been shutdown

		// get System.exceptionHandler
		if(!TJSGetSystem_exceptionHandler_Object(clo))
			return false; // System.exceptionHandler cannot be retrieved

		// execute clo
		tTJSVariant obj(e.GetValue());

		tTJSVariant *pval[] =  { &obj };

		tTJSVariant res;

		clo.FuncCall(0, NULL, NULL, &res, 1, pval, NULL);

		result = res.operator bool();
	}
	catch(eTJSScriptError &e)
	{
		clo.Release();
		TVPShowScriptException(e);
	}
	catch(eTJS &e)
	{
		clo.Release();
		TVPShowScriptException(e);
	}
	catch(...)
	{
		clo.Release();
		throw;
	}
	clo.Release();

	return result;
}
//---------------------------------------------------------------------------
bool TVPProcessUnhandledException(eTJSScriptError &e)
{
	bool result;
	tTJSVariantClosure clo;
	clo.Object = clo.ObjThis = NULL;

	try
	{
		// get the script engine
		tTJS *engine = TVPGetScriptEngine();
		if(!engine)
			return false; // the script engine had been shutdown

		// get System.exceptionHandler
		if(!TJSGetSystem_exceptionHandler_Object(clo))
			return false; // System.exceptionHandler cannot be retrieved

		// execute clo
		tTJSVariant obj;
		tTJSVariant msg(e.GetMessage());
		tTJSVariant trace(e.GetTrace());
		TJSGetExceptionObject(engine, &obj, msg, &trace);

		tTJSVariant *pval[] =  { &obj };

		tTJSVariant res;

		clo.FuncCall(0, NULL, NULL, &res, 1, pval, NULL);

		result = res.operator bool();
	}
	catch(eTJSScriptError &e)
	{
		clo.Release();
		TVPShowScriptException(e);
	}
	catch(eTJS &e)
	{
		clo.Release();
		TVPShowScriptException(e);
	}
	catch(...)
	{
		clo.Release();
		throw;
	}
	clo.Release();

	return result;
}
//---------------------------------------------------------------------------
bool TVPProcessUnhandledException(eTJS &e)
{
	bool result;
	tTJSVariantClosure clo;
	clo.Object = clo.ObjThis = NULL;

	try
	{
		// get the script engine
		tTJS *engine = TVPGetScriptEngine();
		if(!engine)
			return false; // the script engine had been shutdown

		// get System.exceptionHandler
		if(!TJSGetSystem_exceptionHandler_Object(clo))
			return false; // System.exceptionHandler cannot be retrieved

		// execute clo
		tTJSVariant obj;
		tTJSVariant msg(e.GetMessage());
		TJSGetExceptionObject(engine, &obj, msg);

		tTJSVariant *pval[] =  { &obj };

		tTJSVariant res;

		clo.FuncCall(0, NULL, NULL, &res, 1, pval, NULL);

		result = res.operator bool();
	}
	catch(eTJSScriptError &e)
	{
		clo.Release();
		TVPShowScriptException(e);
	}
	catch(eTJS &e)
	{
		clo.Release();
		TVPShowScriptException(e);
	}
	catch(...)
	{
		clo.Release();
		throw;
	}
	clo.Release();

	return result;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void TVPStartObjectHashMap()
{
	// addref ObjectHashMap if the program is being debugged.
	if(TJSEnableDebugMode)
		TJSAddRefObjectHashMap();
}

//---------------------------------------------------------------------------
// TVPBeforeProcessUnhandledException
//---------------------------------------------------------------------------
void TVPBeforeProcessUnhandledException()
{
	TVPDumpHWException();
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// TVPShowScriptException
//---------------------------------------------------------------------------
/*
	These functions display the error location, reason, etc.
	And disable the script event dispatching to avoid massive occurrence of
	errors.
*/
//---------------------------------------------------------------------------
void TVPShowScriptException(eTJS &e)
{
	TVPSetSystemEventDisabledState(true);
	TVPOnError();

	if(!TVPSystemUninitCalled)
	{
		ttstr errstr = (ttstr(TVPScriptExceptionRaised) + TJS_W("\n") + e.GetMessage());
		TVPAddLog(ttstr(TVPScriptExceptionRaised) + TJS_W("\n") + e.GetMessage());
		Application->MessageDlg( errstr.AsStdString(), std::wstring(), mtError, mbOK );
		TVPTerminateSync(1);
	}
}
//---------------------------------------------------------------------------
void TVPShowScriptException(eTJSScriptError &e)
{
	TVPSetSystemEventDisabledState(true);
	TVPOnError();

	if(!TVPSystemUninitCalled)
	{
		ttstr errstr = (ttstr(TVPScriptExceptionRaised) + TJS_W("\n") + e.GetMessage());
		TVPAddLog(ttstr(TVPScriptExceptionRaised) + TJS_W("\n") + e.GetMessage());
		if(e.GetTrace().GetLen() != 0)
			TVPAddLog(ttstr(TJS_W("trace : ")) + e.GetTrace());
		Application->MessageDlg( errstr.AsStdString(), Application->GetTitle(), mtStop, mbOK );

#ifdef TVP_ENABLE_EXECUTE_AT_EXCEPTION
		const tjs_char* scriptName = e.GetBlockNoAddRef()->GetName();
		if( scriptName != NULL && scriptName[0] != 0 ) {
			ttstr path(scriptName);
			try {
				ttstr newpath = TVPGetPlacedPath(path);
				if( newpath.IsEmpty() ) {
					path = TVPNormalizeStorageName(path);
				} else {
					path = newpath;
				}
				TVPGetLocalName( path );
				std::wstring scriptPath( path.AsStdString() );
				tjs_int lineno = 1+e.GetBlockNoAddRef()->SrcPosToLine(e.GetPosition() )- e.GetBlockNoAddRef()->GetLineOffset();

#if defined(WIN32) && defined(_DEBUG) && !defined(ENABLE_DEBUGGER)
// デバッガ実行されている時、Visual Studio で行ジャンプする時の指定をデバッグ出力に出して、break で停止する
				if( ::IsDebuggerPresent() ) {
					std::wstring debuglile( std::wstring(L"2>")+path.AsStdString()+L"("+std::to_wstring(lineno)+L"): error :" + errstr.AsStdString() );
					::OutputDebugString( debuglile.c_str() );
					// ここで breakで停止した時、直前の出力行をダブルクリックすれば、例外箇所のスクリプトをVisual Studioで開ける
					::DebugBreak();
				}
#endif
				scriptPath = std::wstring(L"\"") + scriptPath + std::wstring(L"\"");
				tTJSVariant val;
				if( TVPGetCommandLine(TJS_W("-exceptionexe"), &val) )
				{
					ttstr exepath(val);
					//exepath = ttstr(TJS_W("\"")) + exepath + ttstr(TJS_W("\""));
					if( TVPGetCommandLine(TJS_W("-exceptionarg"), &val) )
					{
						ttstr arg(val);
						if( !exepath.IsEmpty() && !arg.IsEmpty() ) {
							std::wstring str( arg.AsStdString() );
							str = ApplicationSpecialPath::ReplaceStringAll( str, std::wstring(L"%filepath%"), scriptPath );
							str = ApplicationSpecialPath::ReplaceStringAll( str, std::wstring(L"%line%"), std::to_wstring(lineno) );
							//exepath = exepath + ttstr(str);
							//_wsystem( exepath.c_str() );
							arg = ttstr(str);
							TVPAddLog( ttstr(TJS_W("(execute) "))+exepath+ttstr(TJS_W(" "))+arg);
							TVPShellExecute( exepath, arg );
						}
					}
				}
			} catch(...) {
			}
		}
#endif
		TVPTerminateSync(1);
	}
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// TVPInitializeStartupScript
//---------------------------------------------------------------------------
void TVPInitializeStartupScript()
{
	TVPStartObjectHashMap();

	TVPExecuteStartupScript();
	if(TVPTerminateOnNoWindowStartup && TVPGetWindowCount() == 0 ) {
		// no window is created and main window is invisible
		Application->Terminate();
	}
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// tTJSNC_Scripts
//---------------------------------------------------------------------------
tjs_uint32 tTJSNC_Scripts::ClassID = -1;
tTJSNC_Scripts::tTJSNC_Scripts() : inherited(TJS_W("Scripts"))
{
	// registration of native members

	TJS_BEGIN_NATIVE_MEMBERS(Scripts)
	TJS_DECL_EMPTY_FINALIZE_METHOD
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_CONSTRUCTOR_DECL_NO_INSTANCE(/*TJS class name*/Scripts)
{
	return TJS_S_OK;
}
TJS_END_NATIVE_CONSTRUCTOR_DECL(/*TJS class name*/Scripts)
//----------------------------------------------------------------------

//-- methods

//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/execStorage)
{
	// execute script which stored in storage
	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	ttstr name = *param[0];

	ttstr modestr;
	if(numparams >=2 && param[1]->Type() != tvtVoid)
		modestr = *param[1];

	iTJSDispatch2 *context = numparams >= 3 && param[2]->Type() != tvtVoid ? param[2]->AsObjectNoAddRef() : NULL;
	
	TVPExecuteStorage(name, context, result, false, modestr.c_str());

	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/execStorage)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/evalStorage)
{
	// execute expression which stored in storage
	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	ttstr name = *param[0];

	ttstr modestr;
	if(numparams >=2 && param[1]->Type() != tvtVoid)
		modestr = *param[1];

	iTJSDispatch2 *context = numparams >= 3 && param[2]->Type() != tvtVoid ? param[2]->AsObjectNoAddRef() : NULL;

	TVPExecuteStorage(name, context, result, true, modestr.c_str());

	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/evalStorage)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/compileStorage) // bytecode
{
	if(numparams < 2) return TJS_E_BADPARAMCOUNT;

	ttstr name = *param[0];
	ttstr output = *param[1];

	bool isresult = false;
	if( numparams >= 3 && (tjs_int)*param[2] ) {
		isresult = true;
	}

	bool outputdebug = false;
	if( numparams >= 4 && (tjs_int)*param[3] ) {
		outputdebug = true;
	}

	bool isexpression = false;
	if( numparams >= 5 && (tjs_int)*param[4] ) {
		isexpression = true;
	}
	TVPCompileStorage( name, isresult, outputdebug, isexpression, output );

	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/compileStorage)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/exec)
{
	// execute given string as a script
	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	ttstr content = *param[0];

	ttstr name;
	tjs_int lineofs = 0;
	if(numparams >= 2 && param[1]->Type() != tvtVoid) name = *param[1];
	if(numparams >= 3 && param[2]->Type() != tvtVoid) lineofs = *param[2];

	iTJSDispatch2 *context = numparams >= 4 && param[3]->Type() != tvtVoid ? param[3]->AsObjectNoAddRef() : NULL;
	
	if(TVPScriptEngine)
		TVPScriptEngine->ExecScript(content, result, context,
			&name, lineofs);
	else
		TVPThrowInternalError;

	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/exec)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/eval)
{
	// execute given string as a script
	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	ttstr content = *param[0];

	ttstr name;
	tjs_int lineofs = 0;
	if(numparams >= 2 && param[1]->Type() != tvtVoid) name = *param[1];
	if(numparams >= 3 && param[2]->Type() != tvtVoid) lineofs = *param[2];

	iTJSDispatch2 *context = numparams >= 4 && param[3]->Type() != tvtVoid ? param[3]->AsObjectNoAddRef() : NULL;
	
	if(TVPScriptEngine)
		TVPScriptEngine->EvalExpression(content, result, context,
			&name, lineofs);
	else
		TVPThrowInternalError;

	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/eval)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/dump)
{
	// execute given string as a script
	TVPDumpScriptEngine();

	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/dump)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/getTraceString)
{
	// get current stack trace as string
	tjs_int limit = 0;

	if(numparams >= 1 && param[0]->Type() != tvtVoid)
		limit = *param[0];

	if(result)
	{
		*result = TJSGetStackTraceString(limit);
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/getTraceString)
//----------------------------------------------------------------------
#ifdef TJS_DEBUG_DUMP_STRING
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/dumpStringHeap)
{
	// dump all strings held by TJS2 framework
	TJSDumpStringHeap();

	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/dumpStringHeap)
#endif
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/setCallMissing) /* UNDOCUMENTED: subject to change */
{
	// set to call "missing" method
	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	iTJSDispatch2 *dsp = param[0]->AsObjectNoAddRef();

	if(dsp)
	{
		tTJSVariant missing(TJS_W("missing"));
		dsp->ClassInstanceInfo(TJS_CII_SET_MISSING, 0, &missing);
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/setCallMissing) /* UNDOCUMENTED: subject to change */
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/getClassNames) /* UNDOCUMENTED: subject to change */
{
	// get class name as an array, last (most end) class first.
	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	iTJSDispatch2 *dsp = param[0]->AsObjectNoAddRef();

	if(dsp)
	{
		iTJSDispatch2 * array =  TJSCreateArrayObject();
		try
		{
			tjs_uint num = 0;
			while(true)
			{
				tTJSVariant val;
				tjs_error err = dsp->ClassInstanceInfo(TJS_CII_GET, num, &val);
				if(TJS_FAILED(err)) break;
				array->PropSetByNum(TJS_MEMBERENSURE, num, &val, array);
				num ++;
			}
			if(result) *result = tTJSVariant(array, array);
		}
		catch(...)
		{
			array->Release();
			throw;
		}
		array->Release();
	}
	else
	{
		return TJS_E_FAIL;
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/getClassNames) /* UNDOCUMENTED: subject to change */
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(textEncoding)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		*result = TVPScriptTextEncoding;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER
	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TVPScriptTextEncoding = *param;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_STATIC_PROP_DECL(textEncoding)
//----------------------------------------------------------------------

TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/createWithDll32)
{
	if (numparams < 2) return TJS_E_BADPARAMCOUNT;
	if (param[0]->Type() != tvtString || param[1]->Type() != tvtString)
		return TJS_E_INVALIDPARAM;

	bool retv = TVPCreateWithDll32(ttstr(*param[0]), ttstr(*param[1]));

	if (result)
	{
		*result = retv;
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/createWithDll32)

//----------------------------------------------------------------------
	TJS_END_NATIVE_MEMBERS
}
//---------------------------------------------------------------------------
tTJSNativeInstance * tTJSNC_Scripts::CreateNativeInstance()
{
	// this class cannot create an instance
	TVPThrowExceptionMessage(TVPCannotCreateInstance);

	return NULL;
}
//---------------------------------------------------------------------------



ULONG_PTR PrintConsole(PCWSTR Format, ...)
{
	BOOL        Success, IsConsole;
	ULONG       Length;
	WCHAR       Buffer[0xF00 / 2];
	va_list     Args;
	HANDLE      StdOutput;

	va_start(Args, Format);
	Length = _vsnwprintf(Buffer, _countof(Buffer) - 1, Format, Args);
	if (Length == -1)
		return Length;

	StdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	Success = WriteConsoleW(
		StdOutput,
		Buffer,
		Length,
		&Length,
		NULL
	);

	return Success ? Length : 0;
}

#include <Psapi.h>

DWORD_PTR GetProcessBaseAddress(HANDLE processHandle)
{
	DWORD_PTR   baseAddress = 0;
	HMODULE* moduleArray;
	LPBYTE      moduleArrayBytes;
	DWORD       bytesRequired;

	if (processHandle)
	{
		if (EnumProcessModules(processHandle, NULL, 0, &bytesRequired))
		{
			if (bytesRequired)
			{
				moduleArrayBytes = (LPBYTE)LocalAlloc(LPTR, bytesRequired);

				if (moduleArrayBytes)
				{
					unsigned int moduleCount;

					moduleCount = bytesRequired / sizeof(HMODULE);
					moduleArray = (HMODULE*)moduleArrayBytes;

					if (EnumProcessModules(processHandle, moduleArray, bytesRequired, &bytesRequired))
					{
						baseAddress = (DWORD_PTR)moduleArray[0];
					}

					LocalFree(moduleArrayBytes);
				}
			}
		}

		CloseHandle(processHandle);
	}

	return baseAddress;
}



BOOL CalcPeSize(PIMAGE_NT_HEADERS NtHeader, ULONG_PTR Version)
{
	ULONG_PTR Size;
	PIMAGE_SECTION_HEADER SectionHeaders;

	switch (Version)
	{
	case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
		Size = ((PIMAGE_NT_HEADERS32)NtHeader)->OptionalHeader.SizeOfHeaders;
		SectionHeaders = (PIMAGE_SECTION_HEADER)((PBYTE)NtHeader + sizeof(IMAGE_NT_HEADERS32));
		for (ULONG_PTR i = 0; i < ((PIMAGE_NT_HEADERS32)NtHeader)->FileHeader.NumberOfSections; i++) {
			Size += SectionHeaders[i].SizeOfRawData;
		}
		return Size;

	case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
		Size = ((PIMAGE_NT_HEADERS64)NtHeader)->OptionalHeader.SizeOfHeaders;
		SectionHeaders = (PIMAGE_SECTION_HEADER)((PBYTE)NtHeader + sizeof(IMAGE_NT_HEADERS64));
		for (ULONG_PTR i = 0; i < ((PIMAGE_NT_HEADERS64)NtHeader)->FileHeader.NumberOfSections; i++) {
			Size += SectionHeaders[i].SizeOfRawData;
		}
		return Size;
	}

	return 0;
}


HANDLE MapFile(HANDLE FileHandle, PVOID* Base)
{
	HANDLE Handle;

	if (Base == NULL) {
		return NULL;
	}

	Handle = CreateFileMappingW(FileHandle, NULL, PAGE_READONLY, 0, 0, NULL);
	if (Handle == INVALID_HANDLE_VALUE || Handle == 0) {
		return NULL;
	}

	*Base = MapViewOfFile(Handle, FILE_MAP_READ, 0, 0, 0);

	return Handle;
}


BOOL UnMap(PVOID Base, HANDLE Handle)
{
	BOOL b1 = UnmapViewOfFile(Base);
	BOOL b2 = CloseHandle(Handle);

	if (b1 && b2) return TRUE;
	return FALSE;
}


inline PIMAGE_NT_HEADERS ImageNtHeaders(PVOID ImageBase, PULONG_PTR NtHeadersVersion = nullptr)
{
	PIMAGE_NT_HEADERS32 NtHeaders32;

	NtHeaders32 = (PIMAGE_NT_HEADERS32)((ULONG64)ImageBase + (ULONG)((PIMAGE_DOS_HEADER)ImageBase)->e_lfanew);

	if (NtHeadersVersion != nullptr) switch (NtHeaders32->OptionalHeader.Magic)
	{
	case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
	case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
		*NtHeadersVersion = NtHeaders32->OptionalHeader.Magic;
		break;

	default:
		return nullptr;
	}

	return (PIMAGE_NT_HEADERS)NtHeaders32;
}

BOOL PrepareShellCode(HANDLE Process, HANDLE Thread, PCWSTR DllPath, PCWSTR ExePath)
{
	BOOL              Status;
	PVOID             VirtualMemory;
	PVOID             DllNameBase;
	WOW64_CONTEXT     Context;
	SIZE_T            BytesTransferred;
	std::vector<BYTE> ShellCode;
	HANDLE            Handle, FileHandle;
	PVOID             Base;

	if (DllPath == nullptr) {
		return FALSE;
	}

	RtlZeroMemory(&Context, sizeof(Context));
	Context.ContextFlags = CONTEXT_FULL;

	//Wow64SuspendThread(Thread);
	FileHandle = CreateFileW(ExePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (FileHandle == 0 || FileHandle == INVALID_HANDLE_VALUE) {
		return FALSE;
	}
	
	Base = 0;
	Handle = MapFile(FileHandle, &Base);
	if (!Handle) {
		CloseHandle(FileHandle);
		return FALSE;
	}

	PIMAGE_DOS_HEADER DosHeader = (PIMAGE_DOS_HEADER)Base;
	if (DosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
		UnMap(Base, Handle);
		CloseHandle(FileHandle);
		return FALSE;
	}

	PIMAGE_NT_HEADERS32 NtHeader = (PIMAGE_NT_HEADERS32)ImageNtHeaders(Base);
	if (!NtHeader) {
		UnMap(Base, Handle);
		CloseHandle(FileHandle);
		return FALSE;
	}

	ULONG Oep = NtHeader->OptionalHeader.AddressOfEntryPoint;

	UnMap(Base, Handle);
	CloseHandle(FileHandle);

	Status = Wow64GetThreadContext(Thread, &Context);
	if (!Status) {
		return FALSE;
	}

	VirtualMemory = VirtualAllocEx(Process, NULL, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (!VirtualMemory) {
		return FALSE;
	}

	if (lstrlenW(DllPath) > 0x240) {
		return FALSE;
	}

	DllNameBase = (PBYTE)VirtualMemory + 0x500;

	BytesTransferred = 0;
	Status = WriteProcessMemory(
		Process,
		DllNameBase,
		DllPath,
		(lstrlenW(DllPath) + 1) * 2,
		&BytesTransferred
	);

	if (!Status) {
		VirtualFreeEx(Process, VirtualMemory, 0x1000, MEM_RELEASE);
		return FALSE;
	}

#define SIN(...) ShellCode.insert(ShellCode.end(), {__VA_ARGS__})
#define SIN2(x) ShellCode.insert(ShellCode.end(), x.begin(), x.end())

	auto MovEax_XX = [](DWORD XX)->std::vector<BYTE>
	{
		std::vector<BYTE> Inst;
		union
		{
			BYTE  Bytes[4];
			DWORD Dword;
		};

		Inst.push_back(0xB8);
		Dword = XX;

		for (ULONG i = 0; i < _countof(Bytes); i++) {
			Inst.push_back(Bytes[i]);
		}

		return Inst;
	};

	auto Const_XX = [](DWORD XX)->std::vector<BYTE>
	{
		std::vector<BYTE> Inst;
		union
		{
			BYTE  Bytes[4];
			DWORD Dword;
		};

		Dword = XX;
		for (ULONG i = 0; i < _countof(Bytes); i++) {
			Inst.push_back(Bytes[i]);
		}

		return Inst;
	};

	SIN(0x64, 0xA1, 0x30, 0x00, 0x00, 0x00); // mov     eax, large fs:30h
	SIN(0x22, 0x33, 0x44);
	SIN(0x8B, 0x40, 0x08); // mov     eax, [eax+8]
	SIN(0x05); // add eax, imm
	auto&& OffsetImm = Const_XX(Oep);
	SIN2(OffsetImm);
	SIN(0x50); // push eax ; save oep
	SIN(0x64, 0xA1, 0x30, 0x00, 0x00, 0x00); // mov     eax, large fs:30h
	SIN(0x8B, 0x40, 0x08); // mov     eax, [eax+8]
	SIN(0xb8); // mov eax, imm
	auto&& DllNameImm = Const_XX((DWORD)DllNameBase);
	SIN2(DllNameImm);
	SIN(0x9C); // pushfd
	SIN(0x60); // pushad
	SIN(0x50); // push eax; DllName.vaddr
	SIN(0xE8, 0x08, 0x00, 0x00, 0x00); // call eip + 0x13
	SIN(0x83, 0xC4, 0x04); // add esp, 0x4
	SIN(0x61); // popad
	SIN(0x9D); // popfd
	SIN(0x58); // pop eax
	SIN(0xFF, 0xE0); // jmp eax

	// LoadDll

	SIN(0x55);       // push    ebp
	SIN(0x8B, 0xEC); // mov     ebp, esp
	SIN(0x64, 0xA1, 0x30, 0x00, 0x00, 0x00); // mov     eax, large fs:30h
	SIN(0x83, 0xEC, 0x08); // sub     esp, 8
	SIN(0x8B, 0x40, 0x0C); // mov     eax, [eax+0Ch]
	SIN(0x53); // push    ebx
	SIN(0x56); // push    esi
	SIN(0x57); // push    edi
	SIN(0x8B, 0x48, 0x1C); // mov     ecx, [eax+1Ch]
	SIN(0x83, 0xE9, 0x10); // sub     ecx, 10h
	SIN(0x8B, 0xF9); // mov     edi, ecx
	SIN(0x66, 0x0F, 0x1F, 0x44, 0x00, 0x00); // nop     word ptr [eax+eax+00h]

	// loc_401030:
	SIN(0x8B, 0x49, 0x10); // mov     ecx, [ecx+10h]
	SIN(0x83, 0xE9, 0x10); // sub     ecx, 10h
	SIN(0x8B, 0x51, 0x30); // mov     edx, [ecx+30h]
	SIN(0x85, 0xD2); // test    edx, edx
	SIN(0x74, 0x46); // jz      short loc_401083
	SIN(0x8B, 0x02); // mov     eax, [edx]
	SIN(0x8B, 0x72, 0x04); // mov     esi, [edx+4]
	SIN(0x25, 0xDF, 0xFF, 0xDF, 0xFF); // and     eax, 0FFDFFFDFh
	SIN(0x81, 0xE6, 0xDF, 0xFF, 0xDF, 0xFF); // and     esi, 0FFDFFFDFh
	SIN(0x3D, 0x4B, 0x00, 0x45, 0x00); // cmp     eax, 45004Bh
	SIN(0x75, 0x2F); // jnz     short loc_401083
	SIN(0x81, 0xFE, 0x52, 0x00, 0x4E, 0x00); // cmp     esi, 4E0052h
	SIN(0x75, 0x27); // jnz     short loc_401083
	SIN(0x8B, 0x42, 0x08); // mov     eax, [edx+8]
	SIN(0x8B, 0x72, 0x0C); // mov     esi, [edx+0Ch]
	SIN(0x25, 0xDF, 0xFF, 0xDF, 0xFF); // and     eax, 0FFDFFFDFh
	SIN(0x81, 0xE6, 0xDF, 0xFF, 0xDF, 0xFF); // and     esi, 0FFDFFFDFh
	SIN(0x3D, 0x45, 0x00, 0x4C, 0x00); // cmp     eax, 4C0045h
	SIN(0x75, 0x0F); // jnz     short loc_401083
	SIN(0x81, 0xFE, 0x13, 0x00, 0x12, 0x00); // cmp     esi, 120013h
	SIN(0x75, 0x07); // jnz     short loc_401083
	SIN(0x66, 0x83, 0x7A, 0x10, 0x2E); // cmp     word ptr [edx+10h], 2Eh
	SIN(0x74, 0x0D); // jz      short loc_401090

	// loc_401083
	SIN(0x3B, 0xF9); // cmp     edi, ecx
	SIN(0x75, 0xA9); // jnz     short loc_401030

	// loc_401087
	SIN(0x5F); // pop     edi
	SIN(0x5E); // pop     esi
	SIN(0x33, 0xC0); //  xor     eax, eax
	SIN(0x5B); // pop     ebx
	SIN(0x8B, 0xE5); // mov     esp, ebp
	SIN(0x5D); // pop     ebp
	SIN(0xC3); // retn

	// loc_401090
	SIN(0x8B, 0x59, 0x18); // mov     ebx, [ecx+18h]
	SIN(0xB8, 0x4D, 0x5A, 0x00, 0x00); // mov     eax, 5A4Dh
	SIN(0x66, 0x39, 0x03);
	SIN(0x75, 0xEA); // jnz     short loc_401087
	SIN(0x8B, 0x53, 0x3C);  // mov     edx, [ebx+3Ch]
	SIN(0x03, 0xD3); // add     edx, ebx
	SIN(0x81, 0x3A, 0x50, 0x45, 0x00, 0x00); // cmp     dword ptr [edx], 4550h
	SIN(0x75, 0xDD); // jnz     short loc_401087
	SIN(0x0F, 0xB7, 0x42, 0x18); // movzx   eax, word ptr [edx+18h]
	SIN(0xBE, 0x0B, 0x01, 0x00, 0x00); // mov     esi, 10Bh
	SIN(0x8B, 0xC8); // mov     ecx, eax
	SIN(0xBF, 0x0B, 0x02, 0x00, 0x00); // mov     edi, 20Bh
	SIN(0x89, 0x4D, 0xF8); // mov     [ebp+var_8], ecx
	SIN(0x66, 0x3B, 0xCE); // cmp     cx, si
	SIN(0x74, 0x0C); // jz      short loc_4010CE
	SIN(0x66, 0x3B, 0xC7); // cmp     ax, di
	SIN(0x75, 0xC0); // jnz     short loc_401087
	SIN(0xB8, 0x88, 0x00, 0x00, 0x00); // mov     eax, 88h
	SIN(0xEB, 0x05); // jmp     short loc_4010D3

	// loc_4010CE
	SIN(0xB8, 0x78, 0x00, 0x00, 0x00); // mov     eax, 78h

	// loc_4010D3
	SIN(0x8B, 0x7C, 0x10, 0x04); // mov     edi, [eax+edx+4]
	SIN(0x8B, 0x4A, 0x50); // mov     ecx, [edx+50h]
	SIN(0x85, 0xFF); // test    edi, edi
	SIN(0x74, 0xA9); // jz      short loc_401087
	SIN(0x8B, 0x34, 0x10); // mov     esi, [eax+edx]
	SIN(0x33, 0xC0); // xor     eax, eax
	SIN(0x89, 0x75, 0xFC); // mov     [ebp+var_4], esi
	SIN(0x01, 0x7D, 0xFC); // add     [ebp+var_4], edi
	SIN(0x13, 0xC0); // adc     eax, eax
	SIN(0x85, 0xC0); // test    eax, eax
	SIN(0x77, 0x98); // ja      short loc_401087
	SIN(0x72, 0x06); // jb      short loc_4010F7
	SIN(0x83, 0x7D, 0xFC, 0xFF); // cmp     [ebp+var_4], 0FFFFFFFFh
	SIN(0x77, 0x90); // ja      short loc_401087

	// loc_4010F7
	SIN(0x3B, 0xF1); // cmp     esi, ecx
	SIN(0x77, 0x8C); // ja      short loc_401087
	SIN(0x8D, 0x04, 0x3E); // lea     eax, [esi+edi]
	SIN(0x3B, 0xC1); // cmp     eax, ecx
	SIN(0x77, 0x85); // ja      short loc_401087
	SIN(0x39, 0x4C, 0x1E, 0x1C); // cmp     [esi+ebx+1Ch], ecx
	SIN(0x0F, 0x83, 0x7B, 0xFF, 0xFF, 0xFF); // jnb     loc_401087
	SIN(0xB8, 0x0B, 0x02, 0x00, 0x00); // mov     eax, 20Bh
	SIN(0x66, 0x39, 0x45, 0xF8); // cmp     word ptr [ebp+var_8], ax
	SIN(0x74, 0x5B); // jz      short loc_401172
	SIN(0x8B, 0x42, 0x78); // mov     eax, [edx+78h]
	SIN(0x03, 0xC3); // add     eax, ebx
	SIN(0x8B, 0x48, 0x18); // mov     ecx, [eax+18h]
	SIN(0x8B, 0x70, 0x20); // mov     esi, [eax+20h]
	SIN(0x8B, 0x78, 0x24); // mov     edi, [eax+24h]
	SIN(0x03, 0xF3); // add     esi, ebx
	SIN(0x89, 0x4D, 0xFC);  // mov     [ebp+var_4], ecx
	SIN(0x8B, 0x48, 0x1C); // mov     ecx, [eax+1Ch]
	SIN(0x03, 0xCB); // add     ecx, ebx
	SIN(0x03, 0xFB); // add     edi, ebx
	SIN(0x89, 0x4D, 0xF8); // mov     [ebp+var_8], ecx

	// loc_401134
	SIN(0x8B, 0x16); // mov     edx, [esi]
	SIN(0x33, 0xC9); // xor     ecx, ecx
	SIN(0x03, 0xD3); // add     edx, ebx
	SIN(0x8A, 0x02); // mov     al, [edx]
	SIN(0x84, 0xC0); // test    al, al
	SIN(0x74, 0x1D); // jz      short loc_40115D

	// loc_401140
	SIN(0xC1, 0xC1, 0x0D); // rol     ecx, 0Dh
	SIN(0x8D, 0x52, 0x01); // lea     edx, [edx+1]
	SIN(0x0F, 0xB6, 0xC0); // movzx   eax, al
	SIN(0x33, 0xC8); // xor     ecx, eax
	SIN(0x8A, 0x02); // mov     al, [edx]
	SIN(0x84, 0xC0); // test    al, al
	SIN(0x75, 0xEF); // jnz     short loc_401140
	SIN(0x81, 0xF9, 0xAC, 0x35, 0x33, 0xA3); // cmp     ecx, 0A33335ACh
	SIN(0x0F, 0x84, 0xBC, 0x00, 0x00, 0x00); // jz      loc_401219

	// loc_40115D
	SIN(0x83, 0xC7, 0x02); // add     edi, 2
	SIN(0x83, 0xC6, 0x04); // add     esi, 4
	SIN(0x83, 0x6D, 0xFC, 0x01); // sub     [ebp+var_4], 1
	SIN(0x75, 0xCB); // jnz     short loc_401134
	SIN(0x5F); // pop     edi
	SIN(0x5E); // pop     esi
	SIN(0x33, 0xC0); // xor     eax, eax
	SIN(0x5B); // pop     ebx
	SIN(0x8B, 0xE5); // mov     esp, ebp
	SIN(0x5D); // pop     ebp
	SIN(0xC3); // retn

	// loc_401172
	SIN(0x8B, 0xB2, 0x8C, 0x00, 0x00, 0x00); // mov     esi, [edx+8Ch]
	SIN(0x85, 0xF6); // test    esi, esi
	SIN(0x0F, 0x84, 0x77, 0x00, 0x00, 0x00); // jz      loc_4011F7
	SIN(0x8B, 0xBA, 0x88, 0x00, 0x00, 0x00); // mov     edi, [edx+88h]
	SIN(0x33, 0xC0); // xor     eax, eax
	SIN(0x8B, 0xD7); // mov     edx, edi
	SIN(0x03, 0xD6); // add     edx, esi
	SIN(0x13, 0xC0); // adc     eax, eax
	SIN(0x85, 0xC0); // test    eax, eax
	SIN(0x77, 0x65); // ja      short loc_4011F7
	SIN(0x72, 0x05); // jb      short loc_401199
	SIN(0x83, 0xFA, 0xFF); // cmp     edx, 0FFFFFFFFh
	SIN(0x77, 0x5E); // ja      short loc_4011F7

	// loc_401199
	SIN(0x3B, 0xF9); // cmp     edi, ecx
	SIN(0x77, 0x5A); // ja      short loc_4011F7
	SIN(0x8D, 0x04, 0x37); // lea     eax, [edi+esi]
	SIN(0x3B, 0xC1); // cmp     eax, ecx
	SIN(0x77, 0x53); // ja      short loc_4011F7
	SIN(0x39, 0x4C, 0x1F, 0x1C); // cmp     [edi+ebx+1Ch], ecx
	SIN(0x73, 0x4D); // jnb     short loc_4011F7
	SIN(0x8B, 0x44, 0x3B, 0x18); // mov     eax, [ebx+edi+18h]
	SIN(0x8B, 0x74, 0x3B, 0x20); // mov     esi, [ebx+edi+20h]
	SIN(0x89, 0x45, 0xFC); // mov     [ebp+var_4], eax
	SIN(0x03, 0xF3); // add     esi, ebx
	SIN(0x8B, 0x44, 0x3B, 0x1C); // mov     eax, [ebx+edi+1Ch]
	SIN(0x8B, 0x7C, 0x3B, 0x24); // mov     edi, [ebx+edi+24h]
	SIN(0x03, 0xC3); // add     eax, ebx
	SIN(0x89, 0x45, 0xF8); // mov     [ebp+var_8], eax
	SIN(0x03, 0xFB); // add     edi, ebx

	// loc_4011C6
	SIN(0x8B, 0x16); // mov     edx, [esi]
	SIN(0x33, 0xC9); // xor     ecx, ecx
	SIN(0x03, 0xD3); // add     edx, ebx
	SIN(0x8A, 0x02); // mov     al, [edx]
	SIN(0x84, 0xC0); // test    al, al
	SIN(0x74, 0x19); // jz      short loc_4011EB

	// loc_4011D2
	SIN(0xC1, 0xC1, 0x0D); // rol     ecx, 0Dh
	SIN(0x8D, 0x52, 0x01); // lea     edx, [edx+1]
	SIN(0x0F, 0xB6, 0xC0); // movzx   eax, al
	SIN(0x33, 0xC8); // xor     ecx, eax
	SIN(0x8A, 0x02); // mov     al, [edx]
	SIN(0x84, 0xC0); // test    al, al
	SIN(0x75, 0xEF); // jnz     short loc_4011D2
	SIN(0x81, 0xF9, 0xAC, 0x35, 0x33, 0xA3); // cmp     ecx, 0A33335ACh
	SIN(0x74, 0x2E); // jz      short loc_401219

	// loc_4011EB
	SIN(0x83, 0xC7, 0x02); // add     edi, 2
	SIN(0x83, 0xC6, 0x04); // add     esi, 4
	SIN(0x83, 0x6D, 0xFC, 0x01); // sub     [ebp+var_4], 1
	SIN(0x75, 0xCF); // jnz     short loc_4011C6

	// loc_4011F7
	SIN(0x33, 0xC0); // xor     eax, eax

	// loc_4011F9
	SIN(0x85, 0xC0); // test    eax, eax
	SIN(0x0F, 0x84, 0x86, 0xFE, 0xFF, 0xFF); // jz      loc_401087
	SIN(0x8B, 0x00); // mov     eax, [eax]
	SIN(0x03, 0xC3); // add     eax, ebx
	SIN(0x0F, 0x84, 0x7C, 0xFE, 0xFF, 0xFF); // jz      loc_401087
	SIN(0xff, 0x75, 0x08); // push DWORD PTR [ebp+0x8] ; the arg
	SIN(0x90); // nop ; padding, sizeof(push imm) = 5
	SIN(0x90); // nop
	SIN(0xFF, 0xD0); // call    eax
	SIN(0x5F); // pop     edi
	SIN(0x5E); // pop     esi
	SIN(0x5B); // pop     ebx
	SIN(0x8B, 0xE5); // mov     esp, ebp
	SIN(0x5D); // pop     ebp
	SIN(0xC3); // retn

	// loc_401219
	SIN(0x0F, 0xB7, 0x07); // movzx   eax, word ptr [edi]
	SIN(0x8B, 0x4D, 0xF8); // mov     ecx, [ebp+var_8]
	SIN(0x8D, 0x04, 0x81); // lea     eax, [ecx+eax*4]
	SIN(0xEB, 0xD5); // jmp     short loc_4011F9

#undef SIN
#undef SIN2

	Status = WriteProcessMemory(
		Process,
		VirtualMemory,
		ShellCode.data(),
		ShellCode.size(),
		&BytesTransferred
	);

	if (!Status) {
		return FALSE;
	}

	Context.Eip = (DWORD)VirtualMemory;
	Wow64SetThreadContext(Thread, &Context);
	return TRUE;
}



BOOL
CreateProcessWithDllW_ANY_TO_I386(
	LPCWSTR lpApplicationName,
	LPWSTR lpCommandLine,
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL bInheritHandles,
	DWORD dwCreationFlags,
	LPVOID lpEnvironment,
	LPCWSTR lpCurrentDirectory,
	LPSTARTUPINFOW lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation,
	LPCWSTR lpDllFullPath
)
{
	BOOL  Status;
	DWORD CreaFlags;

	CreaFlags = dwCreationFlags | CREATE_SUSPENDED;

	Status = CreateProcessW(
		lpApplicationName,
		lpCommandLine,
		lpProcessAttributes,
		lpThreadAttributes,
		bInheritHandles,
		CreaFlags,
		lpEnvironment,
		lpCurrentDirectory,
		lpStartupInfo,
		lpProcessInformation
	);

	if (!Status) {
		return FALSE;
	}

	Status = PrepareShellCode(
		lpProcessInformation->hProcess,
		lpProcessInformation->hThread,
		lpDllFullPath,
		lpCommandLine
	);

	if (!Status) {
		TerminateProcess(lpProcessInformation->hProcess, -1);
		CloseHandle(lpProcessInformation->hProcess);
		CloseHandle(lpProcessInformation->hThread);
		return FALSE;
	}

	ResumeThread(lpProcessInformation->hThread);
	CloseHandle(lpProcessInformation->hProcess);
	CloseHandle(lpProcessInformation->hThread);
	return TRUE;
}


bool TVPCreateWithDll32(ttstr& process, ttstr& dllname)
{
	STARTUPINFOW        si;
	PROCESS_INFORMATION pi;

	RtlZeroMemory(&si, sizeof(si));
	RtlZeroMemory(&pi, sizeof(pi));
	si.cb = sizeof(si);

	wchar_t ProcessName[MAX_PATH];
	TJS_strcpy(ProcessName, IncludeTrailingBackslash(ExtractFileDir(ExePath())).c_str());
	TJS_strcat(ProcessName, process.c_str());

	return CreateProcessWithDllW_ANY_TO_I386(
		NULL,
		ProcessName,
		NULL,
		NULL,
		FALSE,
		NULL,
		NULL,
		NULL,
		&si,
		&pi,
		dllname.c_str()
	);
}


//---------------------------------------------------------------------------
// TVPCreateNativeClass_Scripts
//---------------------------------------------------------------------------
tTJSNativeClass * TVPCreateNativeClass_Scripts()
{
	tTJSNC_Scripts *cls = new tTJSNC_Scripts();

	// setup some platform-specific members

//----------------------------------------------------------------------

// currently none

//----------------------------------------------------------------------
	return cls;
}
//---------------------------------------------------------------------------

