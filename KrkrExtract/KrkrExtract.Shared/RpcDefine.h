#pragma once

#include <stdint.h>
#include <string>

enum class KrkrPsbMode : uint32_t
{
	PSB_NONE = 0UL, //for checking only
	PSB_RAW = 1UL,
	PSB_TEXT = (1UL << 1),
	PSB_DECOM = (1UL << 2),
	PSB_IMAGE = (1UL << 3),
	PSB_ANM = (1UL << 4),
	PSB_JSON = (1UL << 5),
	PSB_ALL = 0xffffffffUL
};

enum class KrkrTextMode : uint32_t
{
	TEXT_RAW = 0,
	TEXT_DECODE = (TEXT_RAW + 1)
};

enum class KrkrPngMode : uint32_t
{
	PNG_RAW = 0,
	PNG_SYS = (PNG_RAW + 1)
};


enum class KrkrTjs2Mode : uint32_t
{
	TJS2_RAW = 0,
	TJS2_DEASM = (TJS2_RAW + 1),
	TJS2_DECOM = (TJS2_DEASM + 1)
};


enum class KrkrTlgMode : uint32_t
{
	TLG_RAW = 0,
	TLG_BUILDIN = (TLG_RAW + 1),
	TLG_SYS = (TLG_BUILDIN + 1),
	TLG_PNG = (TLG_SYS + 1),
	TLG_JPG = (TLG_PNG + 1)
};


enum class KrkrAmvMode : uint32_t
{
	AMV_JPG = 0,
	AMV_PNG = (AMV_JPG + 1),
	AMV_GIF = (AMV_PNG + 1),
	AMV_RAW = (AMV_GIF + 1)
};

enum class KrkrPbdMode : uint32_t
{
	PBD_RAW = 0,
	PBD_JSON = (PBD_RAW + 1)
};


typedef struct UNPACK_FLAGS
{
	KrkrPsbMode  PsbMode;
	KrkrTextMode TextMode;
	KrkrPngMode  PngMode;
	KrkrTjs2Mode Tjs2Mode;
	KrkrTlgMode  TlgMode;
	KrkrAmvMode  AmvMode;
	KrkrPbdMode  PdbMode;

} UNPACK_FLAGS, *PUNPACK_FLAGS;



enum class CommandStatus : uint32_t
{
	COMMAND_OK = 0,
	COMMAND_ERROR = (COMMAND_OK + 1),
	COMMAND_WARN = (COMMAND_ERROR + 1)
};




