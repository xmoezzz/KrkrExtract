using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;

namespace KrkrExtract.UI.Legacy
{
    public enum RaiseErrorType
    {
        RAISE_ERROR_HEARTBEAT_TIMEOUT = 0,
	    RAISE_ERROR_REMOTE_CRASH      = 1,
	    RAISE_ERROR_SECRET_DISMATCH   = 2,
	    RAISE_ERROR_INVALID_PID       = 3,
	    RAISE_ERROR_REMOTE_DEAD       = 4,
	    RAISE_ERROR_REMOTE_PRIVILEGED = 5,
	    RAISE_ERROR_REMOTE_GENEROUS   = 6
    };

    public static class UnmanagedPointers
    {
        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        public delegate void NotifyServerProgressChangedServer(string TaskName, UInt64 Current, UInt64 Total);

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        public delegate void NotifyServerLogOutputServer(LogLevel Level, string LogInfo, bool IsCmd);

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        public delegate void NotifyServerUIReadyServer();

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        public delegate void NotifyServerMessageBoxServer(string Info, UInt32 Flags, bool Locked);

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        public delegate void NotifyServerTaskStartAndDisableUIServer();

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        public delegate void NotifyServerTaskEndAndEnableUIServer(bool TaskCompleteStatus, string Description);

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        public delegate void NotifyServerExitFromRemoteProcessServer();

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        public delegate bool NotifyServerRaiseErrorServer(RaiseErrorType ErrorType, string Error);
    }
}
