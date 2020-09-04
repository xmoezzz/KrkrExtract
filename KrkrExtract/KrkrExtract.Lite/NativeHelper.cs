using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;

namespace KrkrExtract.Lite
{
    public enum KrkrMode
    {
        NORMAL = 0,
        BASIC_LOCK = 1,
        ADV_LOCK = 2,
        HYPERVISOR = 3
    }

    public static class NativeHelper
    {
        [DllImport("LoaderHelper.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern SafeFileHandle CreateProcessInternalWithDll([MarshalAs(UnmanagedType.LPWStr), In]string ProcessName, KrkrMode Mode, ref UInt32 ProcessId);

        [DllImport("Kernel32.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool CloseHandle(IntPtr Handle);

        [DllImport("kernel32.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool AllocConsole();

        [DllImport("User32.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool SetProcessDPIAware();
    }
}
