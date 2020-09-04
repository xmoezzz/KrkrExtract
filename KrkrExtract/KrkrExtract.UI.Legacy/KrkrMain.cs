using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Diagnostics;
using System.IO;
using System.IO.MemoryMappedFiles;
using System.Runtime.InteropServices;
using Microsoft.WindowsAPICodePack;
using Microsoft.WindowsAPICodePack.Shell;

namespace KrkrExtract.UI.Legacy
{
    public partial class KrkrMain : Form
    {
        private ConnectStatus m_ConnectionStatus;
        private ConnectType m_ConnectionType;
        private string m_CurrentTask;
        private IntPtr m_RemoteProcess;

        public KrkrMain(IntPtr RemoteProcess)
        {
            m_RemoteProcess = RemoteProcess;
            InitializeComponent();
            InitializationUIServerCallbacks();

            m_ConnectionStatus = ConnectStatus.Status_Not_Connect;

            var ConnectionType = Environment.GetEnvironmentVariable("KrkrConnectionType");
            if (String.IsNullOrEmpty(ConnectionType) || String.Compare(ConnectionType, "ncalrpc", true) == 0)
                m_ConnectionType = ConnectType.Connect_ncalrpc;
            else
                m_ConnectionType = ConnectType.Connect_ncacn_hvsocket;

            this.m_ConnectionStatus = ConnectStatus.Status_Not_Connect;
            this.Text = FormatTitle(null, 0, 0);

            RefreshConnectionStatus();
        }


        //Callbacks for UI server
        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        delegate void NotifyServerProgressChangedClient(string TaskName, UInt64 Current, UInt64 Total);

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        delegate void NotifyServerLogOutputClient(LogLevel Level, string LogInfo, bool IsCmd);

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        delegate void NotifyServerUIReadyClient();

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        delegate void NotifyServerMessageBoxClient(string Info, UInt32 Flags, bool Locked);

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        delegate void NotifyServerTaskStartAndDisableUIClient();

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        delegate void NotifyServerTaskEndAndEnableUIClient(bool TaskCompleteStatus, string Description);

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        delegate void NotifyServerExitFromRemoteProcessClient();


        /// <summary>
        /// not a rpc callback, just tell UI server...
        /// </summary>
        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        delegate void NotifyRemoteExitOrCrash(int ExitCode);

        private NotifyConnectionFromRemote m_NotifyConnectionFromRemoteCallback;
        private NotifyDisconnectionFromRemote m_NotifyDisconnectionFromRemoteCallback;
        private NotifyProgressChanged m_NotifyProgressChangedCallback;
        private NotifyEnterLockMode m_NotifyEnterLockModeCallback;
        private NotifyExitLockMode m_NotifyExitLockMode;
        private NotifySetCurrentItem m_NotifySetCurrentItem;
        private NotifyUIReady m_NotifyUIReady;
        private NotifyRemoteExitOrCrash m_NotifyRemoteExitOrCrash;
        private NotifyTaskError m_NotifyTaskError;
        private NotifyTaskErrorWithDescription m_NotifyTaskErrorWithDescription;

        private void InitializationUIServerCallbacks()
        {
            m_NotifyConnectionFromRemoteCallback =
                () =>
            {
                NotifyConnectionFromRemoteWorker();
            };

            m_NotifyDisconnectionFromRemoteCallback =
                () =>
            {
                NotifyDisconnectionFromRemoteWorker();
            };

            m_NotifyProgressChangedCallback =
                (UInt64 Current, UInt64 Total) =>
            {
                NotifyProgressChangedWorker(Current, Total);
            };

            m_NotifyEnterLockModeCallback =
                (string TaskName) =>
            {
                return NotifyEnterLockModeWorker(TaskName);
            };

            m_NotifyExitLockMode =
                () =>
            {
                return NotifyExitLockModeWorker();
            };

            m_NotifySetCurrentItem =
                (LogLevel Level, string Item) =>
            {
                NotifySetCurrentItemWorker(Level, Item);
            };

            m_NotifyUIReady =
                () =>
            {
                NotifyUIReadyWorker();
            };

            m_NotifyTaskError =
                () =>
            {
                MessageBox.Show(this, "KrkrExtract encountered with fatal error", "KrkrExtract", MessageBoxButtons.OK, MessageBoxIcon.Error);
            };

            m_NotifyTaskErrorWithDescription =
                (string Description) =>
            {
                MessageBox.Show(this, Description, "KrkrExtract", MessageBoxButtons.OK, MessageBoxIcon.Error);
            };

            m_NotifyRemoteExitOrCrash =
                (int ExitCode) =>
            {
                //0xC0000005
                MessageBox.Show(
                    this,
                    String.Format("Remote process crashed or exited unexpectedly\nCode : {0}", ExitCode),
                    "KrkrExtract",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error
                );


                /// duplicate this code
                Environment.Exit(ExitCode);

            };


            m_GetRemoteProcess =
                () =>
            {
                return m_RemoteProcess;
            };
        }



        /// <summary>
        /// Worker for NotifyConnectionFromRemote
        /// </summary>
        private void NotifyConnectionFromRemoteWorker()
        {
            m_ConnectionStatus = ConnectStatus.Status_Connected;
            RefreshConnectionStatus();
        }


        private void NotifyDisconnectionFromRemoteWorker()
        {
            m_ConnectionStatus = ConnectStatus.Status_Disconnected;
            RefreshConnectionStatus();
        }

        private void NotifyProgressChangedWorker(UInt64 Current, UInt64 Total)
        {
            if (Total == 0)
            {
                TaskProgressBar.Style = ProgressBarStyle.Continuous;
                TaskProgressBar.Maximum = 0;
                TaskProgressBar.Value = 0;

                if (Environment.OSVersion.Version >= new Version(6, 1))
                {
                    var Taskbar = Microsoft.WindowsAPICodePack.Taskbar.TaskbarManager.Instance;
                    if (Taskbar != null)
                    {
                        Taskbar.SetProgressState(Microsoft.WindowsAPICodePack.Taskbar.TaskbarProgressBarState.NoProgress);
                        Taskbar.SetProgressValue(0, 0);
                    }
                }
            }
            else if ((Int64)Total < 0)
            {
                TaskProgressBar.Style = ProgressBarStyle.Marquee;
                var Taskbar = Microsoft.WindowsAPICodePack.Taskbar.TaskbarManager.Instance;
                if (Taskbar != null)
                {
                    Taskbar.SetProgressState(Microsoft.WindowsAPICodePack.Taskbar.TaskbarProgressBarState.Indeterminate);
                    Taskbar.SetProgressValue(0, 0);
                }
            }
            else
            {
                TaskProgressBar.Style = ProgressBarStyle.Continuous;
                TaskProgressBar.Maximum = (int)Total;
                TaskProgressBar.Value = (int)Current;

                if (Environment.OSVersion.Version >= new Version(6, 1))
                {
                    var Taskbar = Microsoft.WindowsAPICodePack.Taskbar.TaskbarManager.Instance;
                    if (Taskbar != null)
                    {
                        Taskbar.SetProgressState(Microsoft.WindowsAPICodePack.Taskbar.TaskbarProgressBarState.Normal);
                        Taskbar.SetProgressValue((int)Current, (int)Total);
                    }
                }
            }
        }

        private bool NotifyEnterLockModeWorker(string TaskName)
        {

        }

        private bool NotifyExitLockModeWorker()
        {

        }

        private void NotifySetCurrentItemWorker(LogLevel Level, string Item)
        {

        }

        private void NotifyUIReadyWorker()
        {
        }


        private void NotifyRemoteExitOrCrashWorker()
        {
            m_ConnectionStatus = ConnectStatus.Status_Disconnected;
            RefreshConnectionStatus();
            MessageBox.Show(this, "Remote process exited unexpectedly", "KrkrExtract", MessageBoxButtons.OK, MessageBoxIcon.Warning);
        }

        /// <summary>
        /// Rpc connection status
        /// </summary>


        private void RefreshConnectionStatus()
        {
            if (m_ConnectionStatus == ConnectStatus.Status_Not_Connect)
            {
                this.ConnectToolStripStatusLabel.Text = 
                    String.Format("No Connection (desire type : {0})", 
                    this.m_ConnectionType == ConnectType.Connect_ncalrpc ? "ncalrpc" : "hvsocket");
                this.ConnectToolStripStatusLabel.ForeColor = Color.Yellow;
            }
            else if (m_ConnectionStatus == ConnectStatus.Status_Connected)
            {
                this.ConnectToolStripStatusLabel.Text = "Connected : " +
                    (this.m_ConnectionType == ConnectType.Connect_ncalrpc ? "ncalrpc" : "hvsocket");
                this.ConnectToolStripStatusLabel.ForeColor = Color.Green;
            }
            else
            {
                this.ConnectToolStripStatusLabel.Text = "Disconnected : " +
                    (this.m_ConnectionType == ConnectType.Connect_ncalrpc ? "ncalrpc" : "hvsocket") +
                    " remote exited unexpectedly";
                this.ConnectToolStripStatusLabel.ForeColor = Color.Red;
            }
        }


        [DllImport("KrkrExtract.Rpc.Client.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool NotifyRemotePsbModeChanged(KrkrPsbMode Mode);

        [DllImport("KrkrExtract.Rpc.Client.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool NotifyRemoteTextModeChanged(KrkrTextMode Mode);

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

        /// <summary>
        /// UI helper
        /// </summary>


        private string FormatTitle(string Task, UInt64 Current, UInt64 Total)
        {
            if (String.IsNullOrEmpty(Task))
            {
                return String.Format("KrkrExtract(version : {0}, built on : {1})",
                    GetKrkrExtractVersion(),
                    GetKrkrExtractBuildTime()
                    );
            }

            return String.Format("KrkrExtract(version : {0}, built on : {1}) [%s: {2}/{3}]",
                GetKrkrExtractVersion(),
                GetKrkrExtractBuildTime(),
                Current,
                Total
                );
        }

        private void LockdownWindow()
        {

            ///png
            this.PngRawRadioButton.Enabled = false;
            this.PngSystemDecodeRadioButton.Enabled = false;

            ///psb
            this.PsbRawCheckBox.Enabled = false;
            this.PsbUnpackAnmCheckBox.Enabled = false;
            this.PsbDecompileCheckBox.Enabled = false;
            this.PsbUnpackImageCheckBox.Enabled = false;
            this.PsbJsonCheckBox.Enabled = false;
            this.PsbDumpTextCheckBox.Enabled = false;
            this.PsbFullCheckBox.Enabled = false;

            ///text
            this.TextRawRadioButton.Enabled = false;
            this.TextDecompileRadioButton.Enabled = false;

            ///tjs2 
            this.Tjs2DecompileRadioButton.Enabled = false;
            this.Tjs2DisasmRadioButton.Enabled = false;
            this.Tjs2RawRadioButton.Enabled = false;

            ///pbd
            this.PbdJsonRadioButton.Enabled = false;
            this.PbdRawRadioButton.Enabled = false;

            ///tlg
            this.TlgBuiltinRadioButton.Enabled = false;
            this.TlgJpgRadioButton.Enabled = false;
            this.TlgPngRadioButton.Enabled = false;
            this.TlgRawRadioButton.Enabled = false;
            this.TlgSystemRadioButton.Enabled = false;

            ///amv
            this.AmvGifRadioButton.Enabled = false;
            this.AmvJpgRadioButton.Enabled = false;
            this.AmvPngRadioButton.Enabled = false;
            this.AmvRawRadioButton.Enabled = false;


            ///task
            this.VirtualConsoleInputTextBox.Enabled = false;
            this.VirtualConsoleListBox.Enabled = false;
            this.TaskOpenConsoleButton.Enabled = false;
            this.TaskCancelButton.Enabled = false;

            ///packer
            this.BaseFolderTextBox.Enabled = false;
            this.BaseFolderButton.Enabled = false;
            this.OriginalArchiveTextBox.Enabled = false;
            this.OriginalArchiveButton.Enabled = false;
            this.OutputArchiveTextBox.Enabled = false;
            this.OutputArchiveButton.Enabled = false;
            this.MakeArchiveButton.Enabled = false;

            ///universal patch
            this.UniversalPatchInheritICONCheckBox.Enabled = false;
            this.UniversalPatchProtectionCheckBox.Enabled = false;
            this.UniversalPatchInheritICONCheckBox.Enabled = false;

            ///universal dumper
            this.UniversalDumperButton.Enabled = false;
            
        }

        private void EnableWindow()
        {

        }

        private string GetKrkrExtractVersion()
        {
            try
            {
                var VersionInfo = FileVersionInfo.GetVersionInfo(Path.Combine(Environment.CurrentDirectory, "KrkrExtract.Core.dll"));
                return VersionInfo.FileVersion;
            }
            catch (Exception e)
            {
                return "Unknown version";
            }
        }

        private string GetKrkrExtractBuildTime()
        {
            try
            {
                FileStream file = File.OpenRead(Path.Combine(Environment.CurrentDirectory, "KrkrExtract.Core.dll"));
                using (var mmf = MemoryMappedFile.CreateFromFile(file, null,
                                                      file.Length,
                                                      MemoryMappedFileAccess.Read,
                                                      null, HandleInheritability.None, false))
                {
                    NativeMethods.IMAGE_DOS_HEADER dosHeader = new NativeMethods.IMAGE_DOS_HEADER();
                    using (var accessor = mmf.CreateViewAccessor(0,
                                                                  Marshal.SizeOf(dosHeader),
                                                                  MemoryMappedFileAccess.Read))
                    {
                        accessor.Read<NativeMethods.IMAGE_DOS_HEADER>(0, out dosHeader);
                        if (dosHeader.e_magic != NativeMethods.IMAGE_DOS_SIGNATURE)
                            throw new Exception();
                    }
                    int signature = 0;
                    NativeMethods.IMAGE_FILE_HEADER imageFileHeader = new NativeMethods.IMAGE_FILE_HEADER();
                    using (var accessor = mmf.CreateViewAccessor(dosHeader.e_lfanew,
                                                                  Marshal.SizeOf(signature) + Marshal.SizeOf(imageFileHeader),
                                                                  MemoryMappedFileAccess.Read))
                    {
                        signature = accessor.ReadInt32(0);
                        if (signature != NativeMethods.IMAGE_NT_SIGNATURE)
                            throw new Exception();

                        accessor.Read<NativeMethods.IMAGE_FILE_HEADER>(Marshal.SizeOf(signature), out imageFileHeader);
                    }

                    // convert a Unix timestamp to DateTime
                    DateTime origin = new DateTime(1970, 1, 1, 0, 0, 0, 0);
                    TimeSpan localOffset = TimeZone.CurrentTimeZone.GetUtcOffset(origin);
                    DateTime originUTC  = origin.AddHours(localOffset.Hours);
                    DateTime linkTime  = originUTC.AddSeconds((double)imageFileHeader.TimeDateStamp);
                    return linkTime.ToString();
                }
            }
            catch (Exception e)
            {
                return "Unknown time";
            }
        }

        private void PngRawRadioButton_CheckedChanged(object sender, EventArgs e)
        {

        }

        private void PngSystemDecodeRadioButton_CheckedChanged(object sender, EventArgs e)
        {

        }

        internal static class NativeMethods
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
            internal struct IMAGE_FILE_HEADER
            {
                internal short Machine;
                internal short NumberOfSections;
                internal int TimeDateStamp;
                internal int PointerToSymbolTable;
                internal int NumberOfSymbols;
                internal short SizeOfOptionalHeader;
                internal short Characteristics;
            }
        }

        public void NotifyCustomStatusChanged(int Handle, int Value, bool Checked)
        {

        }

        private void PsbRawCheckBox_CheckedChanged(object sender, EventArgs e)
        {

        }

        private void PsbDecompileCheckBox_CheckedChanged(object sender, EventArgs e)
        {

        }

        private void PsbUnpackImageCheckBox_CheckedChanged(object sender, EventArgs e)
        {

        }

        private void PsbUnpackAnmCheckBox_CheckedChanged(object sender, EventArgs e)
        {

        }

        private void PsbDumpTextCheckBox_CheckedChanged(object sender, EventArgs e)
        {

        }

        private void PsbJsonCheckBox_CheckedChanged(object sender, EventArgs e)
        {

        }

        private void PsbFullCheckBox_CheckedChanged(object sender, EventArgs e)
        {

        }

        private void TextRawRadioButton_CheckedChanged(object sender, EventArgs e)
        {

        }

        private void TextDecompileRadioButton_CheckedChanged(object sender, EventArgs e)
        {

        }

        private void Tjs2RawRadioButton_CheckedChanged(object sender, EventArgs e)
        {

        }

        private void Tjs2DisasmRadioButton_CheckedChanged(object sender, EventArgs e)
        {

        }

        private void Tjs2DecompileRadioButton_CheckedChanged(object sender, EventArgs e)
        {

        }

        private void PbdRawRadioButton_CheckedChanged(object sender, EventArgs e)
        {

        }

        private void PbdJsonRadioButton_CheckedChanged(object sender, EventArgs e)
        {

        }

        private void TlgRawRadioButton_CheckedChanged(object sender, EventArgs e)
        {

        }

        private void TlgBuiltinRadioButton_CheckedChanged(object sender, EventArgs e)
        {

        }

        private void TlgSystemRadioButton_CheckedChanged(object sender, EventArgs e)
        {

        }

        private void TlgPngRadioButton_CheckedChanged(object sender, EventArgs e)
        {

        }

        private void TlgJpgRadioButton_CheckedChanged(object sender, EventArgs e)
        {

        }

        private void AmvJpgRadioButton_CheckedChanged(object sender, EventArgs e)
        {

        }

        private void AmvGifRadioButton_CheckedChanged(object sender, EventArgs e)
        {

        }

        private void AmvPngRadioButton_CheckedChanged(object sender, EventArgs e)
        {

        }

        private void AmvRawRadioButton_CheckedChanged(object sender, EventArgs e)
        {

        }

        private void TaskCancelButton_Click(object sender, EventArgs e)
        {

        }

        private void TaskOpenConsoleButton_Click(object sender, EventArgs e)
        {

        }

        /// <summary>
        /// Packer
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void BaseFolderButton_Click(object sender, EventArgs e)
        {

        }

        private void OriginalArchiveButton_Click(object sender, EventArgs e)
        {

        }

        private void OutputArchiveButton_Click(object sender, EventArgs e)
        {

        }

        private void MakeArchiveButton_Click(object sender, EventArgs e)
        {

        }

        private void BaseFolderTextBox_TextChanged(object sender, EventArgs e)
        {

        }

        private void OriginalArchiveTextBox_TextChanged(object sender, EventArgs e)
        {

        }

        private void OutputArchiveTextBox_TextChanged(object sender, EventArgs e)
        {

        }

        /// <summary>
        /// Universal patch
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void UniversalPatchInheritICONCheckBox_CheckedChanged(object sender, EventArgs e)
        {

        }

        private void UniversalPatchProtectionCheckBox_CheckedChanged(object sender, EventArgs e)
        {

        }

        private void MakeUniversalPatchButton_Click(object sender, EventArgs e)
        {

        }


        /// <summary>
        /// Universal dumper
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void UniversalDumperButton_Click(object sender, EventArgs e)
        {

        }

        
    }
}
