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
        public static bool XeCreateInstance(
            bool IsOfficialServer,
            uint Secret,
            uint HeartbeatTimeoutThreshold,
            uint HandshakeTimeoutThreshold
        )
        {
            switch (RuntimeInformation.ProcessArchitecture)
            {
                case Architecture.X86:
                    return Client32.XeCreateInstance(
                        IsOfficialServer, 
                        Secret, 
                        HeartbeatTimeoutThreshold, 
                        HandshakeTimeoutThreshold
                        );

                case Architecture.X64:
                    return Client64.XeCreateInstance(
                        IsOfficialServer,
                        Secret,
                        HeartbeatTimeoutThreshold,
                        HandshakeTimeoutThreshold
                        );

                case Architecture.Arm64:
                    return ClientAarch64.XeCreateInstance(
                        IsOfficialServer,
                        Secret,
                        HeartbeatTimeoutThreshold,
                        HandshakeTimeoutThreshold
                    );

                case Architecture.Arm:
                    throw new NotImplementedException("Unsupported arch : Arm");
            }

            throw new NotImplementedException("Unsupported arch : Unknown");
        }

        public static bool XeRunServer(
            UnmanagedPointers.NotifyServerProgressChangedServer NotifyServerProgressChangedStub,
            UnmanagedPointers.NotifyServerLogOutputServer       NotifyServerLogOutputStub,
            UnmanagedPointers.NotifyServerUIReadyServer         NotifyServerUIReadyStub,
            UnmanagedPointers.NotifyServerMessageBoxServer      NotifyServerMessageBoxStub,
            UnmanagedPointers.NotifyServerTaskStartAndDisableUIServer NotifyServerTaskStartAndDisableUIStub,
            UnmanagedPointers.NotifyServerTaskEndAndEnableUIServer    NotifyServerTaskEndAndEnableUIStub,
            UnmanagedPointers.NotifyServerExitFromRemoteProcessServer NotifyServerExitFromRemoteProcessStub,
            UnmanagedPointers.NotifyServerRaiseErrorServer            NotifyServerRaiseErrorStub
        )
        {
            switch (RuntimeInformation.ProcessArchitecture)
            {
                case Architecture.X86:
                    return Client32.XeRunServer(
                        NotifyServerProgressChangedStub,
                        NotifyServerLogOutputStub,
                        NotifyServerUIReadyStub,
                        NotifyServerMessageBoxStub,
                        NotifyServerTaskStartAndDisableUIStub,
                        NotifyServerTaskEndAndEnableUIStub,
                        NotifyServerExitFromRemoteProcessStub,
                        NotifyServerRaiseErrorStub
                    );

                case Architecture.X64:
                    return Client64.XeRunServer(
                        NotifyServerProgressChangedStub,
                        NotifyServerLogOutputStub,
                        NotifyServerUIReadyStub,
                        NotifyServerMessageBoxStub,
                        NotifyServerTaskStartAndDisableUIStub,
                        NotifyServerTaskEndAndEnableUIStub,
                        NotifyServerExitFromRemoteProcessStub,
                        NotifyServerRaiseErrorStub
                    );

                case Architecture.Arm64:
                    return ClientAarch64.XeRunServer(
                        NotifyServerProgressChangedStub,
                        NotifyServerLogOutputStub,
                        NotifyServerUIReadyStub,
                        NotifyServerMessageBoxStub,
                        NotifyServerTaskStartAndDisableUIStub,
                        NotifyServerTaskEndAndEnableUIStub,
                        NotifyServerExitFromRemoteProcessStub,
                        NotifyServerRaiseErrorStub
                    );

                case Architecture.Arm:
                    throw new NotImplementedException("Unsupported arch : Arm");
            }

            throw new NotImplementedException("Unsupported arch : Unknown");
        }


        public static bool XeClientUniversalDumperModeChecked(
            KrkrPsbMode  PsbMode,
            KrkrTextMode TextMode,
            KrkrPngMode  PngMode,
            KrkrTjs2Mode Tjs2Mode,
            KrkrTlgMode  TlgMode,
            KrkrAmvMode  AmvMode,
            KrkrPbdMode  PdbMode
        )
        {
            switch (RuntimeInformation.ProcessArchitecture)
            {
                case Architecture.X86:
                    return Client32.XeClientUniversalDumperModeChecked(
                        PsbMode,
                        TextMode,
                        PngMode,
                        Tjs2Mode,
                        TlgMode,
                        AmvMode,
                        PdbMode
                    );

                case Architecture.X64:
                    return Client64.XeClientUniversalDumperModeChecked(
                        PsbMode,
                        TextMode,
                        PngMode,
                        Tjs2Mode,
                        TlgMode,
                        AmvMode,
                        PdbMode
                    );

                case Architecture.Arm64:
                    return ClientAarch64.XeClientUniversalDumperModeChecked(
                        PsbMode,
                        TextMode,
                        PngMode,
                        Tjs2Mode,
                        TlgMode,
                        AmvMode,
                        PdbMode
                    );

                case Architecture.Arm:
                    throw new NotImplementedException("Unsupported arch : Arm");
            }

            throw new NotImplementedException("Unsupported arch : Unknown");
        }

        public static bool XeClientUniversalPatchMakeChecked(bool Protect, bool Icon)
        {
            switch(RuntimeInformation.ProcessArchitecture)
            {
                case Architecture.X86:
                    return Client32.XeClientUniversalPatchMakeChecked(Protect, Icon);

                case Architecture.X64:
                    return Client64.XeClientUniversalPatchMakeChecked(Protect, Icon);

                case Architecture.Arm64:
                    return ClientAarch64.XeClientUniversalPatchMakeChecked(Protect, Icon);

                case Architecture.Arm:
                    throw new NotImplementedException("Unsupported arch : Arm");
            }

            throw new NotImplementedException("Unsupported arch : Unknown");
        }

        public static bool XeClientPackerChecked(
            string BaseDir,
            string OriginalArchiveName,
            string OutputArchiveName
        )
        {
            switch (RuntimeInformation.ProcessArchitecture)
            {
                case Architecture.X86:
                    return Client32.XeClientPackerChecked(
                        BaseDir,
                        OriginalArchiveName,
                        OutputArchiveName
                    );

                case Architecture.X64:
                    return Client64.XeClientPackerChecked(
                        BaseDir,
                        OriginalArchiveName,
                        OutputArchiveName
                    );

                case Architecture.Arm64:
                    return ClientAarch64.XeClientPackerChecked(
                        BaseDir,
                        OriginalArchiveName,
                        OutputArchiveName
                    );

                case Architecture.Arm:
                    throw new NotImplementedException("Unsupported arch : Arm");

            }

            throw new NotImplementedException("Unsupported arch : Unknown");
        }

        public static bool XeClientCommandEmitted(string Command)
        {
            switch (RuntimeInformation.ProcessArchitecture)
            {
                case Architecture.X86:
                    return Client32.XeClientCommandEmitted(Command);

                case Architecture.X64:
                    return Client64.XeClientCommandEmitted(Command);

                case Architecture.Arm64:
                    return ClientAarch64.XeClientCommandEmitted(Command);

                case Architecture.Arm:
                    throw new NotImplementedException("Unsupported arch : Arm");
            }

            throw new NotImplementedException("Unsupported arch : Unknown");
        }

        public static bool XeClientTaskDumpStart(
            KrkrPsbMode  PsbMode,
            KrkrTextMode TextMode,
            KrkrPngMode  PngMode,
            KrkrTjs2Mode Tjs2Mode,
            KrkrTlgMode  TlgMode,
            KrkrAmvMode  AmvMode,
            KrkrPbdMode  PdbMode,
            string       File
        )
        {
            switch (RuntimeInformation.ProcessArchitecture)
            {
                case Architecture.X86:
                    return Client32.XeClientTaskDumpStart(
                        PsbMode,
                        TextMode,
                        PngMode,
                        Tjs2Mode,
                        TlgMode,
                        AmvMode,
                        PdbMode,
                        File
                    );

                case Architecture.X64:
                    return Client64.XeClientTaskDumpStart(
                        PsbMode,
                        TextMode,
                        PngMode,
                        Tjs2Mode,
                        TlgMode,
                        AmvMode,
                        PdbMode,
                        File
                    );

                case Architecture.Arm64:
                    return ClientAarch64.XeClientTaskDumpStart(
                        PsbMode,
                        TextMode,
                        PngMode,
                        Tjs2Mode,
                        TlgMode,
                        AmvMode,
                        PdbMode,
                        File
                    );

                case Architecture.Arm:
                    throw new NotImplementedException("Unsupported arch : Arm");
            }

            throw new NotImplementedException("Unsupported arch : Unknown");
        }

        public static bool XeClientCancelTask()
        {
            switch (RuntimeInformation.ProcessArchitecture)
            {
                case Architecture.X86:
                    return Client32.XeClientCancelTask();

                case Architecture.X64:
                    return Client64.XeClientCancelTask();

                case Architecture.Arm64:
                    return ClientAarch64.XeClientCancelTask();

                case Architecture.Arm:
                    throw new NotImplementedException("Unsupported arch : Arm");
            }

            throw new NotImplementedException("Unsupported arch : Unknown");
        }

        public static bool XeClientTaskCloseWindow()
        {
            switch (RuntimeInformation.ProcessArchitecture)
            {
                case Architecture.X86:
                    return Client32.XeClientTaskCloseWindow();

                case Architecture.X64:
                    return Client64.XeClientTaskCloseWindow();

                case Architecture.Arm64:
                    return ClientAarch64.XeClientTaskCloseWindow();

                case Architecture.Arm:
                    throw new NotImplementedException("Unsupported arch : Arm");
            }

            throw new NotImplementedException("Unsupported arch : Unknown");
        }

        public static IntPtr XeGetRemoteProcessHandle()
        {
            switch(RuntimeInformation.ProcessArchitecture)
            {
                case Architecture.X86:
                    return Client32.XeGetRemoteProcessHandle();

                case Architecture.X64:
                    return Client64.XeGetRemoteProcessHandle();

                case Architecture.Arm64:
                    return ClientAarch64.XeGetRemoteProcessHandle();

                case Architecture.Arm:
                    throw new NotImplementedException("Unsupported arch : Arm");
            }

            throw new NotImplementedException("Unsupported arch : Unknown");
        }
    }

    public static class Client32
    {
        [DllImport("KrkrExtract.Rpc.Server.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool XeCreateInstance(
            bool IsOfficialServer, 
            uint Secret, 
            uint HeartbeatTimeoutThreshold,
            uint HandshakeTimeoutThreshold
        );

        [DllImport("KrkrExtract.Rpc.Server.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool XeRunServer(
            UnmanagedPointers.NotifyServerProgressChangedServer NotifyServerProgressChangedStub,
            UnmanagedPointers.NotifyServerLogOutputServer       NotifyServerLogOutputStub,
            UnmanagedPointers.NotifyServerUIReadyServer         NotifyServerUIReadyStub,
            UnmanagedPointers.NotifyServerMessageBoxServer      NotifyServerMessageBoxStub,
            UnmanagedPointers.NotifyServerTaskStartAndDisableUIServer NotifyServerTaskStartAndDisableUIStub,
            UnmanagedPointers.NotifyServerTaskEndAndEnableUIServer    NotifyServerTaskEndAndEnableUIStub,
            UnmanagedPointers.NotifyServerExitFromRemoteProcessServer NotifyServerExitFromRemoteProcessStub,
            UnmanagedPointers.NotifyServerRaiseErrorServer            NotifyServerRaiseErrorStub
        );

        [DllImport("KrkrExtract.Rpc.Server.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]

        public static extern bool XeClientUniversalDumperModeChecked(
            KrkrPsbMode  PsbMode,
            KrkrTextMode TextMode,
            KrkrPngMode  PngMode,
            KrkrTjs2Mode Tjs2Mode,
            KrkrTlgMode  TlgMode,
            KrkrAmvMode  AmvMode,
            KrkrPbdMode  PdbMode
        );

        [DllImport("KrkrExtract.Rpc.Server.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool XeClientUniversalPatchMakeChecked(bool Protect, bool Icon);

        [DllImport("KrkrExtract.Rpc.Server.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool XeClientPackerChecked(
            string BaseDir,
            string OriginalArchiveName,
            string OutputArchiveName
        );

        [DllImport("KrkrExtract.Rpc.Server.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool XeClientCommandEmitted(string Command);

        [DllImport("KrkrExtract.Rpc.Server.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool XeClientTaskDumpStart(
            KrkrPsbMode  PsbMode,
            KrkrTextMode TextMode,
            KrkrPngMode  PngMode,
            KrkrTjs2Mode Tjs2Mode,
            KrkrTlgMode  TlgMode,
            KrkrAmvMode  AmvMode,
            KrkrPbdMode  PdbMode,
            string       File
        );

        [DllImport("KrkrExtract.Rpc.Server.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool XeClientCancelTask();

        [DllImport("KrkrExtract.Rpc.Server.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool XeClientTaskCloseWindow();

        [DllImport("KrkrExtract.Rpc.Server.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern IntPtr XeGetRemoteProcessHandle();
    }

    public static class Client64
    {
        [DllImport("KrkrExtract.Rpc.Server.64.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool XeCreateInstance(
        bool IsOfficialServer,
        uint Secret,
        uint HeartbeatTimeoutThreshold,
        uint HandshakeTimeoutThreshold
    );

        [DllImport("KrkrExtract.Rpc.Server.64.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool XeRunServer(
            UnmanagedPointers.NotifyServerProgressChangedServer NotifyServerProgressChangedStub,
            UnmanagedPointers.NotifyServerLogOutputServer NotifyServerLogOutputStub,
            UnmanagedPointers.NotifyServerUIReadyServer NotifyServerUIReadyStub,
            UnmanagedPointers.NotifyServerMessageBoxServer NotifyServerMessageBoxStub,
            UnmanagedPointers.NotifyServerTaskStartAndDisableUIServer NotifyServerTaskStartAndDisableUIStub,
            UnmanagedPointers.NotifyServerTaskEndAndEnableUIServer NotifyServerTaskEndAndEnableUIStub,
            UnmanagedPointers.NotifyServerExitFromRemoteProcessServer NotifyServerExitFromRemoteProcessStub,
            UnmanagedPointers.NotifyServerRaiseErrorServer NotifyServerRaiseErrorStub
        );

        [DllImport("KrkrExtract.Rpc.Server.64.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]

        public static extern bool XeClientUniversalDumperModeChecked(
            KrkrPsbMode PsbMode,
            KrkrTextMode TextMode,
            KrkrPngMode PngMode,
            KrkrTjs2Mode Tjs2Mode,
            KrkrTlgMode TlgMode,
            KrkrAmvMode AmvMode,
            KrkrPbdMode PdbMode
        );

        [DllImport("KrkrExtract.Rpc.Server.64.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool XeClientUniversalPatchMakeChecked(bool Protect, bool Icon);

        [DllImport("KrkrExtract.Rpc.Server.64.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool XeClientPackerChecked(
            string BaseDir,
            string OriginalArchiveName,
            string OutputArchiveName
        );

        [DllImport("KrkrExtract.Rpc.Server.64.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool XeClientCommandEmitted(string Command);

        [DllImport("KrkrExtract.Rpc.Server.64.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool XeClientTaskDumpStart(
            KrkrPsbMode PsbMode,
            KrkrTextMode TextMode,
            KrkrPngMode PngMode,
            KrkrTjs2Mode Tjs2Mode,
            KrkrTlgMode TlgMode,
            KrkrAmvMode AmvMode,
            KrkrPbdMode PdbMode,
            string File
        );

        [DllImport("KrkrExtract.Rpc.Server.64.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool XeClientCancelTask();

        [DllImport("KrkrExtract.Rpc.Server.64.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool XeClientTaskCloseWindow();

        [DllImport("KrkrExtract.Rpc.Server.64.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern IntPtr XeGetRemoteProcessHandle();
    }

    public static class ClientAarch64
    {
        [DllImport("KrkrExtract.Rpc.Server.Arm64.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool XeCreateInstance(
        bool IsOfficialServer,
        uint Secret,
        uint HeartbeatTimeoutThreshold,
        uint HandshakeTimeoutThreshold
    );

        [DllImport("KrkrExtract.Rpc.Server.Arm64.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool XeRunServer(
            UnmanagedPointers.NotifyServerProgressChangedServer NotifyServerProgressChangedStub,
            UnmanagedPointers.NotifyServerLogOutputServer NotifyServerLogOutputStub,
            UnmanagedPointers.NotifyServerUIReadyServer NotifyServerUIReadyStub,
            UnmanagedPointers.NotifyServerMessageBoxServer NotifyServerMessageBoxStub,
            UnmanagedPointers.NotifyServerTaskStartAndDisableUIServer NotifyServerTaskStartAndDisableUIStub,
            UnmanagedPointers.NotifyServerTaskEndAndEnableUIServer NotifyServerTaskEndAndEnableUIStub,
            UnmanagedPointers.NotifyServerExitFromRemoteProcessServer NotifyServerExitFromRemoteProcessStub,
            UnmanagedPointers.NotifyServerRaiseErrorServer NotifyServerRaiseErrorStub
        );

        [DllImport("KrkrExtract.Rpc.Server.Arm64.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]

        public static extern bool XeClientUniversalDumperModeChecked(
            KrkrPsbMode PsbMode,
            KrkrTextMode TextMode,
            KrkrPngMode PngMode,
            KrkrTjs2Mode Tjs2Mode,
            KrkrTlgMode TlgMode,
            KrkrAmvMode AmvMode,
            KrkrPbdMode PdbMode
        );

        [DllImport("KrkrExtract.Rpc.Server.Arm64.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool XeClientUniversalPatchMakeChecked(bool Protect, bool Icon);

        [DllImport("KrkrExtract.Rpc.Server.Arm64.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool XeClientPackerChecked(
            string BaseDir,
            string OriginalArchiveName,
            string OutputArchiveName
        );

        [DllImport("KrkrExtract.Rpc.Server.Arm64.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool XeClientCommandEmitted(string Command);

        [DllImport("KrkrExtract.Rpc.Server.Arm64.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool XeClientTaskDumpStart(
            KrkrPsbMode PsbMode,
            KrkrTextMode TextMode,
            KrkrPngMode PngMode,
            KrkrTjs2Mode Tjs2Mode,
            KrkrTlgMode TlgMode,
            KrkrAmvMode AmvMode,
            KrkrPbdMode PdbMode,
            string File
        );

        [DllImport("KrkrExtract.Rpc.Server.Arm64.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool XeClientCancelTask();

        [DllImport("KrkrExtract.Rpc.Server.Arm64.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool XeClientTaskCloseWindow();

        [DllImport("KrkrExtract.Rpc.Server.Arm64.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern IntPtr XeGetRemoteProcessHandle();
    }
}


