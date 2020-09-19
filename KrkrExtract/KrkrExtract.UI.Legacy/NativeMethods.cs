using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;

namespace KrkrExtract.UI.Legacy
{

    public static class NativeMethods
    {
        internal const int IMAGE_DOS_SIGNATURE = 0x5A4D;    // MZ
        internal const int IMAGE_NT_SIGNATURE = 0x00004550; // PE00

        [StructLayout(LayoutKind.Sequential)]
        internal struct IMAGE_DOS_HEADER
        {  // DOS .EXE header
            internal short e_magic;         // Magic number
            internal short e_cblp;          // Bytes on last page of file
            internal short e_cp;            // Pages in file
            internal short e_crlc;          // Relocations
            internal short e_cparhdr;       // Size of header in paragraphs
            internal short e_minalloc;      // Minimum extra paragraphs needed
            internal short e_maxalloc;      // Maximum extra paragraphs needed
            internal short e_ss;            // Initial (relative) SS value
            internal short e_sp;            // Initial SP value
            internal short e_csum;          // Checksum
            internal short e_ip;            // Initial IP value
            internal short e_cs;            // Initial (relative) CS value
            internal short e_lfarlc;        // File address of relocation table
            internal short e_ovno;          // Overlay number
            internal short e_res1;          // Reserved words
            internal short e_res2;          // Reserved words
            internal short e_res3;          // Reserved words
            internal short e_res4;          // Reserved words
            internal short e_oemid;         // OEM identifier (for e_oeminfo)
            internal short e_oeminfo;       // OEM information; e_oemid specific
            internal short e_res20;         // Reserved words
            internal short e_res21;         // Reserved words
            internal short e_res22;         // Reserved words
            internal short e_res23;         // Reserved words
            internal short e_res24;         // Reserved words
            internal short e_res25;         // Reserved words
            internal short e_res26;         // Reserved words
            internal short e_res27;         // Reserved words
            internal short e_res28;         // Reserved words
            internal short e_res29;         // Reserved words
            internal int e_lfanew;          // File address of new exe header
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct IMAGE_FILE_HEADER
        {
            internal short Machine;
            internal short NumberOfSections;
            internal int TimeDateStamp;
            internal int PointerToSymbolTable;
            internal int NumberOfSymbols;
            internal short SizeOfOptionalHeader;
            internal short Characteristics;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct SYSTEM_INFO
        {
            public short wProcessorArchitecture;
            public short wReserved;
            public int dwPageSize;
            public IntPtr lpMinimumApplicationAddress;
            public IntPtr lpMaximumApplicationAddress;
            public IntPtr dwActiveProcessorMask;
            public int dwNumberOfProcessors;
            public int dwProcessorType;
            public int dwAllocationGranularity;
            public short wProcessorLevel;
            public short wProcessorRevision;
        }

        [DllImport("kernel32.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern void GetSystemInfo(ref SYSTEM_INFO lpSystemInfo);

        [DllImport("kernel32,dll", CallingConvention = CallingConvention.StdCall)]
        public static extern void GetNativeSystemInfo(ref SYSTEM_INFO lpSystemInfo);

        [DllImport("Kernel32.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern uint GetProcessId(IntPtr Process);

        [DllImport("Kernel32.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern bool AllocConsole();

        [DllImport("Kernel32.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern bool FreeConsole();

        [DllImport("User32.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int MessageBoxW(System.IntPtr hWnd, string lpText, string lpCaption, uint uType);
    }
}
