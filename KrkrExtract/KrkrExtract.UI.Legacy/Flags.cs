using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace KrkrExtract.UI.Legacy
{
    public enum KrkrPsbMode : uint
    {
        PSB_RAW = 1,
        PSB_TEXT = 1 << 1,
        PSB_DECOM = 1 << 2,
        PSB_IMAGE = 1 << 3,
        PSB_ANM = 1 << 4,
        PSB_JSON = 1 << 5,
        PSB_ALL = 0xFFFFFFFF
    };

    public enum KrkrTextMode : uint
    {
        TEXT_RAW,
        TEXT_DECODE,
        TEXT_ALL
    };

    public enum KrkrPngMode : uint
    {
        PNG_RAW,
        PNG_SYS,
        PNG_ALL
    };

    public enum KrkrTjs2Mode : uint
    {
        TJS2_RAW,
        TJS2_DEASM,
        TJS2_DECOM,
    };

    public enum KrkrTlgMode : uint
    {
        TLG_RAW,
        TLG_BUILDIN,
        TLG_SYS,
        TLG_PNG,
        TLG_JPG
    };

    public enum KrkrAmvMode : uint
    {
        AMV_JPG,
        AMV_PNG,
        AMV_GIF,
        AMV_RAW
    };

    public enum KrkrPbdMode : uint
    {
        PBD_RAW,
        PBD_JSON
    };

    public enum LogLevel : uint
    {
        LOG_INFO,
        LOG_WARN,
        LOG_ERROR
    };
}



