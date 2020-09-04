using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;

namespace KrkrExtract.UI.Legacy
{
    public static class Client
    {
        [DllImport("KrkrExtract.Rpc.Client.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool NotifyRemotePsbModeChanged(KrkrPsbMode Mode);

        [DllImport("KrkrExtract.Rpc.Client.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool NotifyRemoteTextModeChanged(KrkrPsbMode Mode);

        [DllImport("KrkrExtract.Rpc.Client.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool NotifyRemotePngModeChanged(KrkrPngMode Mode);

        [DllImport("KrkrExtract.Rpc.Client.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool NotifyRemoteTjs2ModeChanged(KrkrTjs2Mode Mode);

        [DllImport("KrkrExtract.Rpc.Client.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool NotifyRemoteTlgModeChanged(KrkrTlgMode Mode);

        [DllImport("KrkrExtract.Rpc.Client.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool NotifyRemoteAlphaMovieModeChanged(KrkrAmvMode Mode);

        [DllImport("KrkrExtract.Rpc.Client.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool NotifyRemotePbdModeChanged(KrkrPbdMode Mode);

        [DllImport("KrkrExtract.Rpc.Client.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool NotifyRemoteUniversalDumperModeChecked();

        [DllImport("KrkrExtract.Rpc.Client.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool NotifyRemoteUniversalPatchIconChecked(bool Apply);

        [DllImport("KrkrExtract.Rpc.Client.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool NotifyRemoteUniversalPatchProtectionChecked(bool Apply);

        [DllImport("KrkrExtract.Rpc.Client.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool NotifyRemoteUniversalPatchMakeChecked(bool Apply);

        [DllImport("KrkrExtract.Rpc.Client.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool NotifyRemotePackerChecked();

        [DllImport("KrkrExtract.Rpc.Client.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool NotifyRemotePackerBaseDirChecked([MarshalAs(UnmanagedType.LPWStr), In]string Dir);

        [DllImport("KrkrExtract.Rpc.Client.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool NotifyRemotePackerOriginalArchiveNameChecked([MarshalAs(UnmanagedType.LPWStr), In]string Name);

        [DllImport("KrkrExtract.Rpc.Client.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool NotifyRemotePackerOutputArchiveNameChecked([MarshalAs(UnmanagedType.LPWStr), In]string Name);

        [DllImport("KrkrExtract.Rpc.Client.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool NotifyRemoteCommandEmitted([MarshalAs(UnmanagedType.LPWStr), In]string Command);

        [DllImport("KrkrExtract.Rpc.Client.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool NotifyRemoteTaskDumpStart([MarshalAs(UnmanagedType.LPWStr), In]string File);

        [DllImport("KrkrExtract.Rpc.Client.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool NotifyRemoteTaskCancelled();

        [DllImport("KrkrExtract.Rpc.Client.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool NotifyRemoteTaskCloseWindow();

        [DllImport("KrkrExtract.Rpc.Client.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool NotifyRemoteOpenConsole();

        [DllImport("KrkrExtract.Rpc.Client.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool NotifyRemoteCloseConsole();

        [DllImport("KrkrExtract.Rpc.Client.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool NotifyRemoteCustomCommand(int Handle, int Value, bool Checked);

        [DllImport("KrkrExtract.Rpc.Client.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool NotifyRemoteHandleShakeFromClient();

        [DllImport("KrkrExtract.Rpc.Client.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool InitializeRpcClient(bool IsHvsovcket);
    }
}
