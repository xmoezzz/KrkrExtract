using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;

namespace KrkrExtract.Lite
{
    public class LoaderHelper
    {
        [Flags]
        enum STARTF : uint
        {
            STARTF_USESHOWWINDOW = 0x00000001,
            STARTF_USESIZE = 0x00000002,
            STARTF_USEPOSITION = 0x00000004,
            STARTF_USECOUNTCHARS = 0x00000008,
            STARTF_USEFILLATTRIBUTE = 0x00000010,
            STARTF_RUNFULLSCREEN = 0x00000020,
            STARTF_FORCEONFEEDBACK = 0x00000040,
            STARTF_FORCEOFFFEEDBACK = 0x00000080,
            STARTF_USESTDHANDLES = 0x00000100,
        }


        /// <summary>
        /// Flags for create process.
        /// </summary>
        [Flags]
        public enum CreateProcessFlags : uint
        {
            /// <summary>
            /// No flags.
            /// </summary>
            None = 0,
            /// <summary>
            /// Debug process.
            /// </summary>
            DebugProcess = 0x00000001,
            /// <summary>
            /// Debug only this process.
            /// </summary>
            DebugOnlyThisProcess = 0x00000002,
            /// <summary>
            /// Create suspended.
            /// </summary>
            Suspended = 0x00000004,
            /// <summary>
            /// Detach process.
            /// </summary>
            DetachedProcess = 0x00000008,
            /// <summary>
            /// Create a new console.
            /// </summary>
            NewConsole = 0x00000010,
            /// <summary>
            /// Normal priority class.
            /// </summary>
            NormalPriorityClass = 0x00000020,
            /// <summary>
            /// Idle priority class.
            /// </summary>
            IdlePriorityClass = 0x00000040,
            /// <summary>
            /// High priority class.
            /// </summary>
            HighPriorityClass = 0x00000080,
            /// <summary>
            /// Realtime priority class.
            /// </summary>
            RealtimePriorityClass = 0x00000100,
            /// <summary>
            /// Create a new process group.
            /// </summary>
            NewProcessGroup = 0x00000200,
            /// <summary>
            /// Create from a unicode environment.
            /// </summary>
            UnicodeEnvironment = 0x00000400,
            /// <summary>
            /// Create a separate WOW VDM.
            /// </summary>
            SeparateWowVdm = 0x00000800,
            /// <summary>
            /// Share the WOW VDM.
            /// </summary>
            SharedWowVdm = 0x00001000,
            /// <summary>
            /// Force DOS process.
            /// </summary>
            ForceDOS = 0x00002000,
            /// <summary>
            /// Below normal priority class.
            /// </summary>
            BelowNormalPriorityClass = 0x00004000,
            /// <summary>
            /// Above normal priority class.
            /// </summary>
            AboveNormalPriorityClass = 0x00008000,
            /// <summary>
            /// Inherit parent affinity.
            /// </summary>
            InheritParentAffinity = 0x00010000,
            /// <summary>
            /// Inherit caller priority (deprecated)
            /// </summary>
            InheritCallerPriority = 0x00020000,
            /// <summary>
            /// Create a protected process.
            /// </summary>
            ProtectedProcess = 0x00040000,
            /// <summary>
            /// Specify extended startup information is present.
            /// </summary>
            ExtendedStartupInfoPresent = 0x00080000,
            /// <summary>
            /// Process mode background begin.
            /// </summary>
            ModeBackgroundBegin = 0x00100000,
            /// <summary>
            /// Process mode background end.
            /// </summary>
            ModeBackgroundEnd = 0x00200000,
            /// <summary>
            /// Create a secure process.
            /// </summary>
            SecureProcess = 0x00400000,
            /// <summary>
            /// Breakaway from a job object.
            /// </summary>
            BreakawayFromJob = 0x01000000,
            /// <summary>
            /// Preserve code authz level.
            /// </summary>
            PreserveCodeAuthZLevel = 0x02000000,
            /// <summary>
            /// Default error mode.
            /// </summary>
            DefaultErrorMode = 0x04000000,
            /// <summary>
            /// No window.
            /// </summary>
            NoWindow = 0x08000000,
            /// <summary>
            /// Profile user.
            /// </summary>
            ProfileUser = 0x10000000,
            /// <summary>
            /// Profile kernel.
            /// </summary>
            ProfileKernel = 0x20000000,
            /// <summary>
            /// Profile server.
            /// </summary>
            ProfileServer = 0x40000000,
            /// <summary>
            /// Ignore system default.
            /// </summary>
            IgnoreSystemDefault = 0x80000000
        }


        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        struct STARTUPINFO
        {
            public int cb;
            public string lpReserved;
            public string lpDesktop;
            public string lpTitle;
            public int dwX;
            public int dwY;
            public int dwXSize;
            public int dwYSize;
            public int dwXCountChars;
            public int dwYCountChars;
            public int dwFillAttribute;
            public STARTF dwFlags;
            public short wShowWindow;
            public short cbReserved2;
            public IntPtr lpReserved2;
            public IntPtr hStdInput;
            public IntPtr hStdOutput;
            public IntPtr hStdError;
        }


        [StructLayout(LayoutKind.Sequential)]
        internal struct PROCESS_INFORMATION
        {
            public IntPtr hProcess;
            public IntPtr hThread;
            public int dwProcessId;
            public int dwThreadId;
        }

        [DllImport("kernel32.dll", SetLastError = true, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        static extern bool CreateProcess(
            string lpApplicationName, 
            string lpCommandLine,
            IntPtr lpProcessAttributes, 
            IntPtr lpThreadAttributes, 
            bool bInheritHandles, 
            uint dwCreationFlags,
            IntPtr lpEnvironment, 
            string lpCurrentDirectory, 
            [In] ref STARTUPINFO lpStartupInfo, 
            out PROCESS_INFORMATION lpProcessInformation
            );

        [StructLayout(LayoutKind.Sequential)]
        public struct FLOATING_SAVE_AREA
        {
            public uint ControlWord;
            public uint StatusWord;
            public uint TagWord;
            public uint ErrorOffset;
            public uint ErrorSelector;
            public uint DataOffset;
            public uint DataSelector;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 80)]
            public byte[] RegisterArea;
            public uint Cr0NpxState;
        }


        public enum CONTEXT_FLAGS : uint
        {
            CONTEXT_i386 = 0x10000,
            CONTEXT_i486 = 0x10000,   //  same as i386
            CONTEXT_CONTROL = CONTEXT_i386 | 0x01, // SS:SP, CS:IP, FLAGS, BP
            CONTEXT_INTEGER = CONTEXT_i386 | 0x02, // AX, BX, CX, DX, SI, DI
            CONTEXT_SEGMENTS = CONTEXT_i386 | 0x04, // DS, ES, FS, GS
            CONTEXT_FLOATING_POINT = CONTEXT_i386 | 0x08, // 387 state
            CONTEXT_DEBUG_REGISTERS = CONTEXT_i386 | 0x10, // DB 0-3,6,7
            CONTEXT_EXTENDED_REGISTERS = CONTEXT_i386 | 0x20, // cpu specific extensions
            CONTEXT_FULL = CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_SEGMENTS,
            CONTEXT_ALL = CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_SEGMENTS | CONTEXT_FLOATING_POINT | CONTEXT_DEBUG_REGISTERS | CONTEXT_EXTENDED_REGISTERS
        }

        [Flags]
        public enum AllocationType
        {
            Commit = 0x1000,
            Reserve = 0x2000,
            Decommit = 0x4000,
            Release = 0x8000,
            Reset = 0x80000,
            Physical = 0x400000,
            TopDown = 0x100000,
            WriteWatch = 0x200000,
            LargePages = 0x20000000
        }

        [Flags]
        public enum MemoryProtection
        {
            Execute = 0x10,
            ExecuteRead = 0x20,
            ExecuteReadWrite = 0x40,
            ExecuteWriteCopy = 0x80,
            NoAccess = 0x01,
            ReadOnly = 0x02,
            ReadWrite = 0x04,
            WriteCopy = 0x08,
            GuardModifierflag = 0x100,
            NoCacheModifierflag = 0x200,
            WriteCombineModifierflag = 0x400
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct CONTEXT
        {
            public CONTEXT_FLAGS ContextFlags; //set this to an appropriate value
                                               // Retrieved by CONTEXT_DEBUG_REGISTERS
            public uint Dr0;
            public uint Dr1;
            public uint Dr2;
            public uint Dr3;
            public uint Dr6;
            public uint Dr7;
            // Retrieved by CONTEXT_FLOATING_POINT
            public FLOATING_SAVE_AREA FloatSave;
            // Retrieved by CONTEXT_SEGMENTS
            public uint SegGs;
            public uint SegFs;
            public uint SegEs;
            public uint SegDs;
            // Retrieved by CONTEXT_INTEGER
            public uint Edi;
            public uint Esi;
            public uint Ebx;
            public uint Edx;
            public uint Ecx;
            public uint Eax;
            // Retrieved by CONTEXT_CONTROL
            public uint Ebp;
            public uint Eip;
            public uint SegCs;
            public uint EFlags;
            public uint Esp;
            public uint SegSs;
            // Retrieved by CONTEXT_EXTENDED_REGISTERS
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 512)]
            public byte[] ExtendedRegisters;
        }


        [DllImport("kernel32.dll", CallingConvention=CallingConvention.StdCall, SetLastError = true)]
        public static extern bool GetThreadContext(IntPtr hThread, ref CONTEXT lpContext);

        [DllImport("kernel32.dll", CallingConvention = CallingConvention.StdCall, SetLastError = true)]
        public static extern bool SetThreadContext(IntPtr hThread, ref CONTEXT lpContext);

        [DllImport("kernel32.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Auto)]
        public static extern IntPtr GetModuleHandle(string lpModuleName);

        [DllImport("kernel32.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Ansi, ExactSpelling = true, SetLastError = true)]
        public static extern IntPtr GetProcAddress(IntPtr hModule, string procName);

        [DllImport("kernel32.dll", CallingConvention = CallingConvention.StdCall, SetLastError = true)]
        public static extern bool WriteProcessMemory(IntPtr hProcess, IntPtr lpBaseAddress, byte[] lpBuffer, int nSize, out int lpNumberOfBytesWritten);

        [DllImport("kernel32.dll", CallingConvention = CallingConvention.StdCall, SetLastError = true)]
        public static extern bool ResumeThread(IntPtr hThread);

        [DllImport("kernel32.dll", CallingConvention = CallingConvention.StdCall, SetLastError = true)]
        public static extern IntPtr VirtualAllocEx(IntPtr hProcess, IntPtr lpAddress, uint dwSize, AllocationType flAllocationType, MemoryProtection flProtect);

        [DllImport("kernel32.dll", CallingConvention = CallingConvention.StdCall, SetLastError = true)]
        static extern bool CloseHandle(IntPtr hObject);

        [DllImport("kernel32.dll", CallingConvention = CallingConvention.StdCall, SetLastError = true)]
        static extern bool TerminateProcess(IntPtr hProcess, uint uExitCode);

        static byte[] PerpareDllPathOrName(string DllPath)
        {
            byte[] Buffers = Encoding.GetEncoding("UTF-16").GetBytes(DllPath);
            Array.Resize(ref Buffers, Buffers.Length + 2);
            Buffers[Buffers.Length - 2] = 0x00;
            Buffers[Buffers.Length - 1] = 0x00;

            return Buffers;
        }


        static bool WritePayload(ref PROCESS_INFORMATION Process, string DllPath)
        {
            CONTEXT CurrentCtx = new CONTEXT();
            CurrentCtx.ContextFlags = CONTEXT_FLAGS.CONTEXT_CONTROL | CONTEXT_FLAGS.CONTEXT_INTEGER;

            if (!GetThreadContext(Process.hThread, ref CurrentCtx)) {
                return false;
            }

            IntPtr LoadLibraryPtr = GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryW");
            if (LoadLibraryPtr == IntPtr.Zero) {
                return false;
            }

            var Buffer = VirtualAllocEx(Process.hProcess, IntPtr.Zero, 4 * 1024, AllocationType.Commit | AllocationType.Reserve, MemoryProtection.ExecuteReadWrite);
            if (Buffer == IntPtr.Zero) {
                return false;
            }

            var DllNameBytes = PerpareDllPathOrName(DllPath);

            if (!WriteProcessMemory(
                Process.hProcess, 
                Buffer, 
                DllNameBytes, 
                DllNameBytes.Length, 
                out int Written) || Written != DllNameBytes.Length)
            {
                return false;
            }

            var ReturnAddress = CurrentCtx.Eip;
            CurrentCtx.Eip = (uint)(((uint)Buffer + DllNameBytes.Length) / 16 + 3) * 16;

            // Construct shell code
            var Shellcode = new byte[]
            {
                0x50, // push eax
                0xb8, 0x00, 0x00, 0x00, 0x00, // mov eax, <Buffer>
                0x50, // push eax
                0xb8, 0x00, 0x00, 0x00, 0x00, // mov eax, <LoadLibraryW>
                0xff, 0xd0, // call eax
                0x58, // pop eax
                0xe9, 0x00, 0x00, 0x00, 0x00 // jmp <returnAddress>
            };

            Array.Copy(BitConverter.GetBytes((int)Buffer), 0, Shellcode, 2, 4);
            Array.Copy(BitConverter.GetBytes((int)LoadLibraryPtr), 0, Shellcode, 8, 4);
            Array.Copy(BitConverter.GetBytes(ReturnAddress - CurrentCtx.Eip - Shellcode.Length), 0, Shellcode, Shellcode.Length - 4, 4);

            if (!WriteProcessMemory(
                Process.hProcess, 
                (IntPtr)CurrentCtx.Eip, 
                Shellcode, 
                Shellcode.Length, 
                out Written) || Written != Shellcode.Length)
            {
                return false;
            }

            if (!SetThreadContext(Process.hThread, ref CurrentCtx)) {
                return false;
            }

            if (!ResumeThread(Process.hThread)) {
                return false;
            }

            return true;
        }


        public const int CREATE_SUSPENDED = 0x00000004;

        static public bool CreateProcessWithDll(string ProgramName, string DllPath)
        {
            var StartInfo = new STARTUPINFO();
            StartInfo.cb = Marshal.SizeOf(StartInfo);

            if (!CreateProcess(
                ProgramName,
                "", 
                IntPtr.Zero, 
                IntPtr.Zero, 
                false, 
                CREATE_SUSPENDED, 
                IntPtr.Zero, 
                null, 
                ref StartInfo, 
                out PROCESS_INFORMATION ProcessInfo
                ))
            {
                return false;
            }

            if (!WritePayload(ref ProcessInfo, DllPath)) {
                TerminateProcess(ProcessInfo.hProcess, 0);
                if (ProcessInfo.hProcess != IntPtr.Zero) CloseHandle(ProcessInfo.hProcess);
                if (ProcessInfo.hThread  != IntPtr.Zero) CloseHandle(ProcessInfo.hThread);
                return false;
            }

            if (ProcessInfo.hProcess != IntPtr.Zero) CloseHandle(ProcessInfo.hProcess);
            if (ProcessInfo.hThread  != IntPtr.Zero) CloseHandle(ProcessInfo.hThread);

            return true;
        }
    }
}
