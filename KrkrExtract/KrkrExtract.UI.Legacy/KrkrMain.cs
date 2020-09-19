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

namespace KrkrExtract.UI.Legacy
{
    public partial class KrkrMain : Form
    {
        private ConnectStatus m_ConnectionStatus = ConnectStatus.Status_Not_Connect;
        private ConnectType   m_ConnectionType   = ConnectType.Connect_native;
        private bool          m_ConsoleIsAttached= false;

        private ConnectStatus GetConnectionStatus() { return m_ConnectionStatus; }
        private ConnectType   GetConnectionType()   { return m_ConnectionType; }
        private IntPtr        GetRemoteProcessHande() { return Client.XeGetRemoteProcessHandle();  }

        private string GetRemoteProcessDir()
        {
            var pid = NativeMethods.GetProcessId(GetRemoteProcessHande());
            if (pid == 0)
                return string.Empty;

            var process = Process.GetProcessById((int)pid);
            if (process == null)
                return string.Empty;

            return Path.GetDirectoryName(process.MainModule.FileName);
        }


        public KrkrMain()
        {
            InitializeComponent();
            InitializationUIServerCallbacks();

            m_ConnectionStatus = ConnectStatus.Status_Not_Connect;

            var ConnectionType = Environment.GetEnvironmentVariable("KrkrConnectionType");

            if (String.IsNullOrEmpty(ConnectionType) || String.Compare(ConnectionType, "ncalrpc", true) == 0)
                m_ConnectionType = ConnectType.Connect_native;
            else
                m_ConnectionType = ConnectType.Connect_hypervisor_based;

            this.m_ConnectionStatus = ConnectStatus.Status_Not_Connect;
            this.Text = FormatTitle(null, 0, 0);

            RefreshConnectionStatus();
        }


        private UnmanagedPointers.NotifyServerProgressChangedServer m_NotifyServerProgressChangedCallback;
        private UnmanagedPointers.NotifyServerLogOutputServer m_NotifyServerLogOutputCallback;
        private UnmanagedPointers.NotifyServerUIReadyServer m_NotifyServerUIReadyCallback;
        private UnmanagedPointers.NotifyServerMessageBoxServer m_NotifyServerMessageBoxCallback;
        private UnmanagedPointers.NotifyServerTaskStartAndDisableUIServer m_NotifyServerTaskStartAndDisableUICallback;
        private UnmanagedPointers.NotifyServerTaskEndAndEnableUIServer m_NotifyServerTaskEndAndEnableUICallback;
        private UnmanagedPointers.NotifyServerExitFromRemoteProcessServer m_NotifyServerExitFromRemoteProcessCallback;
        private UnmanagedPointers.NotifyServerRaiseErrorServer m_NotifyServerRaiseErrorCallback;


        private void InitializationUIServerCallbacks()
        {
            m_NotifyServerProgressChangedCallback = 
                (string TaskName, UInt64 Current, UInt64 Total) =>
            {
                NotifyServerProgressChangedWorker(TaskName, Current, Total);
            };

            m_NotifyServerLogOutputCallback =
                (LogLevel Level, string LogInfo, bool IsCmd) =>
            {
                NotifyServerLogOutputWorker(Level, LogInfo, IsCmd);
            };

            m_NotifyServerUIReadyCallback =
                () =>
            {
                NotifyServerUIReadyWorker();
            };


            m_NotifyServerMessageBoxCallback =
                (string Info, UInt32 Flags, bool Locked) =>
            {
                NotifyServerMessageBoxWorker(Info, Flags, Locked);
            };


            m_NotifyServerTaskStartAndDisableUICallback =
                () =>
            {
                NotifyServerTaskStartAndDisableUIWorker();
            };


            m_NotifyServerTaskEndAndEnableUICallback =
                (bool TaskCompleteStatus, string Description) =>
            {
                NotifyServerTaskEndAndEnableUIWorker(TaskCompleteStatus, Description);
            };

            m_NotifyServerExitFromRemoteProcessCallback =
                () =>
            {
                NotifyServerExitFromRemoteProcessWorker();
            };

            m_NotifyServerRaiseErrorCallback =
                (RaiseErrorType ErrorType, string Error) =>
            {
                return NotifyServerRaiseErrorWorker(ErrorType, Error);
            };

        }

        /// <summary>
        /// Workers
        /// </summary>

        #region Workers

        private void NotifyServerProgressChangedWorker(string TaskName, UInt64 Current, UInt64 Total)
        {
            var IsVistaOrHigher = Environment.OSVersion.Version >= new Version(6, 1);

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

        private void NotifyServerLogOutputWorker(LogLevel Level, string LogInfo, bool IsCmd)
        {
            var Time = DateTime.Now.ToString("MM/dd/yyyy HH:mm:ss");
            var IsWindows10 = Environment.OSVersion.Version >= new Version(6, 1);
            var DebugSymbol = IsWindows10 ? "💠" : "DEBUG";
            var InfoSymbol  = IsWindows10 ? "ℹ️" : "INFO";
            var WarnSymbol  = IsWindows10 ? "⚠" : "WARN";
            var ErrorSymbol = IsWindows10 ? "❌" : "ERROR";
            var OkSymbol    = IsWindows10 ? "✔️"  : "OK";
            var Output = "";

            switch (Level)
            {
                case LogLevel.LOG_DEBUG:
                    Output = "[" + DebugSymbol + " " + Time + (IsCmd ? " (cmd)" : "") + "]" + LogInfo;
                    break;

                case LogLevel.LOG_INFO:
                    Output = "[" + InfoSymbol + " " + Time + (IsCmd ? " (cmd)" : "") + "]" + LogInfo;
                    break;

                case LogLevel.LOG_WARN:
                    Output = "[" + WarnSymbol + " " + Time + (IsCmd ? " (cmd)" : "") + "]" + LogInfo;
                    break;

                case LogLevel.LOG_ERROR:
                    Output = "[" + ErrorSymbol + " " + Time + (IsCmd ? " (cmd)" : "") + "]" + LogInfo;
                    break;

                case LogLevel.LOG_OK:
                    Output = "[" + OkSymbol    + " " + Time + (IsCmd ? " (cmd)" : "") + "]" + LogInfo;
                    break;
            }

            if (!string.IsNullOrEmpty(Output))
            {
                VirtualConsoleListBox.Items.Add(Output);
                Console.WriteLine(Output);
            }
        }

        private void NotifyServerUIReadyWorker()
        {
            EnableWindow();
            SetStatusbarConnected();
        }

        private void NotifyServerMessageBoxWorker(string Info, UInt32 Flags, bool Locked)
        {
            if (Locked)
                NativeMethods.MessageBoxW(this.Handle, Info, "KrkrExtract", Flags);
            else
                NativeMethods.MessageBoxW(IntPtr.Zero, Info, "KrkrExtract", Flags);
        }

        private void NotifyServerTaskStartAndDisableUIWorker()
        {
            LockdownWindow();
        }

        private void NotifyServerTaskEndAndEnableUIWorker(bool TaskCompleteStatus, string Description)
        {
            if (!TaskCompleteStatus)
            {
                if (string.IsNullOrEmpty(Description))
                {
                    MessageBox.Show(
                        this, 
                        "KrkrExtract : Unknown error", 
                        "KrkrExtract", 
                        MessageBoxButtons.OK, 
                        MessageBoxIcon.Error
                    );
                }
                else
                {
                    MessageBox.Show(
                        this,
                        Description,
                        "KrkrExtract",
                        MessageBoxButtons.OK,
                        MessageBoxIcon.Error
                    );
                }
            }

            FormatTitle(null, 0, 0);
            EnableWindow();
        }

        private void NotifyServerExitFromRemoteProcessWorker()
        {
            Environment.Exit(0);
        }

        private bool NotifyServerRaiseErrorWorker(RaiseErrorType ErrorType, string Error)
        {
            switch (ErrorType)
            {
                case RaiseErrorType.RAISE_ERROR_HEARTBEAT_TIMEOUT:
                    Console.Write("Heartbeat timeout");
                    MessageBox.Show(this, Error, "KrkrExtract : fatal error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    break;

                case RaiseErrorType.RAISE_ERROR_INVALID_PID:
                    Console.Write("Invalid pid");
                    MessageBox.Show(this, Error, "KrkrExtract : fatal error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    break;

                case RaiseErrorType.RAISE_ERROR_REMOTE_CRASH:
                    Console.Write("Remote crash");
                    MessageBox.Show(this, Error, "KrkrExtract : fatal error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    break;

                case RaiseErrorType.RAISE_ERROR_REMOTE_DEAD:
                    Console.Write("Remote dead to early");
                    MessageBox.Show(this, Error, "KrkrExtract : fatal error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    break;

                case RaiseErrorType.RAISE_ERROR_REMOTE_GENEROUS:
                    Console.Write("Generou error raised");
                    MessageBox.Show(this, Error, "KrkrExtract : fatal error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    break;

                case RaiseErrorType.RAISE_ERROR_REMOTE_PRIVILEGED:
                    Console.Write("Privilege error");
                    MessageBox.Show(this, Error, "KrkrExtract : fatal error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    break;

                case RaiseErrorType.RAISE_ERROR_SECRET_DISMATCH:
                    Console.Write("Secret dismatch");
                    MessageBox.Show(this, Error, "KrkrExtract : fatal error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    break;

                default:
                    Console.Write("Unknown errors (debug me)");
                    MessageBox.Show(this, Error, "KrkrExtract : fatal error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    break;
            }

            Environment.Exit(-1);
            return false;
        }

        #endregion

        /// <summary>
        /// Current flags
        /// </summary>


        #region flags

        private uint GetCurrentPsbFlags()
        {
            uint flags = 0;

            flags |= PsbDecompileCheckBox.Checked ? (uint)KrkrPsbMode.PSB_DECOM : 0;
            flags |= PsbDumpTextCheckBox.Checked ? (uint)KrkrPsbMode.PSB_TEXT : 0;
            flags |= PsbFullCheckBox.Checked ? (uint)KrkrPsbMode.PSB_ALL : 0;
            flags |= PsbJsonCheckBox.Checked ? (uint)KrkrPsbMode.PSB_JSON : 0;
            flags |= PsbRawCheckBox.Checked ? (uint)KrkrPsbMode.PSB_RAW : 0;
            flags |= PsbUnpackAnmCheckBox.Checked ? (uint)KrkrPsbMode.PSB_ANM : 0;
            flags |= PsbUnpackImageCheckBox.Checked ? (uint)KrkrPsbMode.PSB_IMAGE : 0;

            if (flags == 0)
                flags = (uint)KrkrPsbMode.PSB_RAW;

            return flags;
        }

        private uint GetCurrentTextFlags()
        {
            uint flags = 0;

            flags |= TextRawRadioButton.Checked ? (uint)KrkrTextMode.TEXT_RAW : 0;
            flags |= TextDecompileRadioButton.Checked ? (uint)KrkrTextMode.TEXT_DECODE : 0;

            if (flags == 0)
                flags = (uint)KrkrTextMode.TEXT_RAW;

            return flags;
        }

        private uint GetCurrentPngFlags()
        {
            uint flags = 0;

            flags |= PngRawRadioButton.Checked ? (uint)KrkrPngMode.PNG_RAW : 0;
            flags |= PngSystemDecodeRadioButton.Checked ? (uint)KrkrPngMode.PNG_SYS : 0;

            if (flags == 0)
                flags = (uint)KrkrPngMode.PNG_RAW;

            return flags;
        }

        private uint GetCurrentTjs2Flags()
        {
            uint flags = 0;

            flags |= Tjs2RawRadioButton.Checked ? (uint)KrkrTjs2Mode.TJS2_RAW : 0;
            flags |= Tjs2DisasmRadioButton.Checked ? (uint)KrkrTjs2Mode.TJS2_DEASM : 0;
            flags |= Tjs2DecompileRadioButton.Checked ? (uint)KrkrTjs2Mode.TJS2_DECOM : 0;

            if (flags == 0)
                flags = (uint)KrkrTjs2Mode.TJS2_RAW;

            return flags;
        }

        private uint GetCurrentTlgFlags()
        {
            uint flags = 0;

            flags |= TlgRawRadioButton.Checked ? (uint)KrkrTlgMode.TLG_RAW : 0;
            flags |= TlgPngRadioButton.Checked ? (uint)KrkrTlgMode.TLG_PNG : 0;
            flags |= TlgJpgRadioButton.Checked ? (uint)KrkrTlgMode.TLG_JPG : 0;
            flags |= TlgSystemRadioButton.Checked ? (uint)KrkrTlgMode.TLG_SYS : 0;
            flags |= TlgBuiltinRadioButton.Checked ? (uint)KrkrTlgMode.TLG_BUILDIN : 0;

            if (flags == 0)
                flags = (uint)KrkrTlgMode.TLG_RAW;

            return flags;
        }


        private uint GetCurrentAmvFlags()
        {
            uint flags = 0;

            flags |= AmvRawRadioButton.Checked ? (uint)KrkrAmvMode.AMV_RAW : 0;
            flags |= AmvPngRadioButton.Checked ? (uint)KrkrAmvMode.AMV_PNG : 0;
            flags |= AmvJpgRadioButton.Checked ? (uint)KrkrAmvMode.AMV_JPG : 0;
            flags |= AmvGifRadioButton.Checked ? (uint)KrkrAmvMode.AMV_GIF : 0;

            if (flags == 0)
                flags = (uint)KrkrAmvMode.AMV_RAW;

            return flags;
        }

        private uint GetCurrentPbdFlags()
        {
            uint flags = 0;

            flags |= PbdRawRadioButton.Checked ? (uint)KrkrPbdMode.PBD_RAW : 0;
            flags |= PbdJsonRadioButton.Checked ? (uint)KrkrPbdMode.PBD_JSON : 0;

            if (flags == 0)
                flags = (uint)KrkrPbdMode.PBD_RAW;

            return flags;
        }

        #endregion

        /// <summary>
        /// Connections & Status
        /// </summary>

        #region Connection

        private void SetStatusbarConnected()
        {
            m_ConnectionStatus = ConnectStatus.Status_Connected;
            RefreshConnectionStatus();
        }

        private void SetStatusbarDisconnected()
        {
            m_ConnectionStatus = ConnectStatus.Status_Disconnected;
            RefreshConnectionStatus();
        }

        private void RefreshConnectionStatus()
        {
            if (m_ConnectionStatus == ConnectStatus.Status_Not_Connect)
            {
                this.ConnectToolStripStatusLabel.Text = 
                    String.Format("No Connection (desire type : {0})", 
                    this.m_ConnectionType == ConnectType.Connect_native ? "native" : "hvsocket");
                this.ConnectToolStripStatusLabel.ForeColor = Color.Yellow;
            }
            else if (m_ConnectionStatus == ConnectStatus.Status_Connected)
            {
                this.ConnectToolStripStatusLabel.Text = "Connected : " +
                    (this.m_ConnectionType == ConnectType.Connect_native ? "native" : "hvsocket");
                this.ConnectToolStripStatusLabel.ForeColor = Color.Green;
            }
            else
            {
                this.ConnectToolStripStatusLabel.Text = "Disconnected : " +
                    (this.m_ConnectionType == ConnectType.Connect_native ? "native" : "hvsocket") +
                    " remote exited unexpectedly";
                this.ConnectToolStripStatusLabel.ForeColor = Color.Red;
            }
        }

        #endregion


        /// <summary>
        /// UI helper
        /// </summary>

        #region UI helper

        private string FormatTitle(string Task, UInt64 Current, UInt64 Total)
        {
            if (String.IsNullOrEmpty(Task))
            {
                return String.Format("KrkrExtract(version : {0}, built on : {1})",
                    Helper.GetKrkrExtractVersion(),
                    Helper.GetKrkrExtractBuildTime()
                    );
            }

            return String.Format("KrkrExtract(version : {0}, built on : {1}) [%s: {2}/{3}]",
                Helper.GetKrkrExtractVersion(),
                Helper.GetKrkrExtractBuildTime(),
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

            ///png
            this.PngRawRadioButton.Enabled = true;
            this.PngSystemDecodeRadioButton.Enabled = true;

            ///psb
            this.PsbRawCheckBox.Enabled = true;
            this.PsbUnpackAnmCheckBox.Enabled = true;
            this.PsbDecompileCheckBox.Enabled = true;
            this.PsbUnpackImageCheckBox.Enabled = true;
            this.PsbJsonCheckBox.Enabled = true;
            this.PsbDumpTextCheckBox.Enabled = true;
            this.PsbFullCheckBox.Enabled = true;

            ///text
            this.TextRawRadioButton.Enabled = true;
            this.TextDecompileRadioButton.Enabled = true;

            ///tjs2 
            this.Tjs2DecompileRadioButton.Enabled = true;
            this.Tjs2DisasmRadioButton.Enabled = true;
            this.Tjs2RawRadioButton.Enabled = true;

            ///pbd
            this.PbdJsonRadioButton.Enabled = true;
            this.PbdRawRadioButton.Enabled = true;

            ///tlg
            this.TlgBuiltinRadioButton.Enabled = true;
            this.TlgJpgRadioButton.Enabled = true;
            this.TlgPngRadioButton.Enabled = true;
            this.TlgRawRadioButton.Enabled = true;
            this.TlgSystemRadioButton.Enabled = true;

            ///amv
            this.AmvGifRadioButton.Enabled = true;
            this.AmvJpgRadioButton.Enabled = true;
            this.AmvPngRadioButton.Enabled = true;
            this.AmvRawRadioButton.Enabled = true;


            ///task
            this.VirtualConsoleInputTextBox.Enabled = true;
            this.VirtualConsoleListBox.Enabled = true;
            this.TaskOpenConsoleButton.Enabled = true;
            this.TaskCancelButton.Enabled = true;

            ///packer
            this.BaseFolderTextBox.Enabled = true;
            this.BaseFolderButton.Enabled = true;
            this.OriginalArchiveTextBox.Enabled = true;
            this.OriginalArchiveButton.Enabled  = true;
            this.OutputArchiveTextBox.Enabled   = true;
            this.OutputArchiveButton.Enabled    = true;
            this.MakeArchiveButton.Enabled      = true;

            ///universal patch
            this.UniversalPatchInheritICONCheckBox.Enabled = true;
            this.UniversalPatchProtectionCheckBox.Enabled  = true;
            this.UniversalPatchInheritICONCheckBox.Enabled = true;

            ///universal dumper
            this.UniversalDumperButton.Enabled = true;
        }

        #endregion


        /// <summary>
        /// Task
        /// </summary>

        #region Tasks

        private void TaskCancelButton_Click(object sender, EventArgs e)
        {
            try
            {
                var Status = Client.XeClientCancelTask();

                if (!Status)
                    throw new NativeRpcException("Client.XeClientPackerChecked failed");
            }
            catch (NativeRpcException WhichException)
            {
                Console.WriteLine(WhichException.ToString());

                MessageBox.Show(
                    this,
                    WhichException.ToString(),
                    "KrkrExtract",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error
                );
            }
            catch (Exception WhichException)
            {
                Console.WriteLine(WhichException.ToString());

                MessageBox.Show(
                    this,
                    WhichException.ToString(),
                    "KrkrExtract",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error
                );
            }
        }

        private void TaskOpenConsoleButton_Click(object sender, EventArgs e)
        {
            //Open Console
            if (m_ConsoleIsAttached)
            {
                var Status = NativeMethods.FreeConsole();
                if (Status)
                {
                    TaskOpenConsoleButton.Text = "Open Console";
                    m_ConsoleIsAttached = false;
                }
                else
                {
                    TaskOpenConsoleButton.Text = "Open Console";
                    m_ConsoleIsAttached = false;
                }
            }
            else
            {
                var Status = NativeMethods.AllocConsole();
                if (Status)
                {
                    TaskOpenConsoleButton.Text = "Close Console";
                    m_ConsoleIsAttached = true;
                }
                else
                {
                    TaskOpenConsoleButton.Text = "Close Console";
                    m_ConsoleIsAttached = true;
                }
            }
        }

        private void BaseFolderButton_Click(object sender, EventArgs e)
        {
            using (FolderBrowserDialog FolderBrowser = new FolderBrowserDialog())
            {
                var RemoteDir = GetRemoteProcessDir();

                FolderBrowser.Description  = "Select the folder to pack";
                FolderBrowser.RootFolder   = Environment.SpecialFolder.Desktop;
                FolderBrowser.SelectedPath = string.IsNullOrEmpty(RemoteDir) ? "C:\\" : RemoteDir;

                if (FolderBrowser.ShowDialog() == DialogResult.OK)
                {
                    BaseFolderTextBox.Text = FolderBrowser.SelectedPath;
                }
            }
        }

        private void OriginalArchiveButton_Click(object sender, EventArgs e)
        {
            using (OpenFileDialog FileDialog = new OpenFileDialog())
            {
                var RemoteDir = GetRemoteProcessDir();

                FileDialog.InitialDirectory = string.IsNullOrEmpty(RemoteDir) ? "C:\\" : RemoteDir;
                FileDialog.Title  = "Select an original archive";
                FileDialog.Filter = "Xp3 file (*.txt)|*.txt|All file (*.*)|*.*";
                FileDialog.FilterIndex = 2;
                FileDialog.Multiselect      = false;
                FileDialog.ReadOnlyChecked  = true;
                FileDialog.RestoreDirectory = true;

                if (FileDialog.ShowDialog() == DialogResult.OK)
                {
                    OriginalArchiveTextBox.Text = FileDialog.FileName;
                }
            }
        }

        private void OutputArchiveButton_Click(object sender, EventArgs e)
        {
            using (OpenFileDialog FileDialog = new OpenFileDialog())
            {
                var RemoteDir = GetRemoteProcessDir();

                FileDialog.InitialDirectory = string.IsNullOrEmpty(RemoteDir) ? "C:\\" : RemoteDir;
                FileDialog.Title  = "Select the output file path";
                FileDialog.Filter = "Xp3 file (*.txt)|*.txt|All file (*.*)|*.*";
                FileDialog.FilterIndex = 2;
                FileDialog.Multiselect      = false;
                FileDialog.ReadOnlyChecked  = false;
                FileDialog.RestoreDirectory = true;

                if (FileDialog.ShowDialog() == DialogResult.OK)
                {
                    OutputArchiveTextBox.Text = FileDialog.FileName;
                }
            }
        }

        private void MakeArchiveButton_Click(object sender, EventArgs e)
        {
            try
            {
                var Status = Client.XeClientPackerChecked(
                    BaseFolderTextBox.Text,
                    OriginalArchiveTextBox.Text,
                    OutputArchiveTextBox.Text
                    );

                if (!Status)
                    throw new NativeRpcException("Client.XeClientPackerChecked failed");
            }
            catch (NativeRpcException WhichException)
            {
                Console.WriteLine(WhichException.ToString());

                MessageBox.Show(
                    this,
                    WhichException.ToString(),
                    "KrkrExtract",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error
                );
            }
            catch (Exception WhichException)
            {
                Console.WriteLine(WhichException.ToString());

                MessageBox.Show(
                    this,
                    WhichException.ToString(),
                    "KrkrExtract",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error
                );
            }
        }

        private void MakeUniversalPatchButton_Click(object sender, EventArgs e)
        {
            try
            {
                var Status = Client.XeClientUniversalPatchMakeChecked(
                    UniversalPatchProtectionCheckBox.Checked,
                    UniversalPatchInheritICONCheckBox.Checked
                    );

                if (!Status)
                    throw new NativeRpcException("Client.XeClientUniversalPatchMakeChecked");

            }
            catch (NativeRpcException WhichException)
            {
                Console.WriteLine(WhichException.ToString());

                MessageBox.Show(
                    this,
                    WhichException.ToString(),
                    "KrkrExtract",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error
                );
            }
            catch (Exception WhichException)
            {
                Console.WriteLine(WhichException.ToString());

                MessageBox.Show(
                    this,
                    WhichException.ToString(),
                    "KrkrExtract",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error
                );
            }
        }

        private void UniversalDumperButton_Click(object sender, EventArgs e)
        {
            try
            {
                var Status = Client.XeClientUniversalDumperModeChecked(
                        (KrkrPsbMode) GetCurrentPsbFlags(),
                        (KrkrTextMode)GetCurrentTextFlags(),
                        (KrkrPngMode) GetCurrentPngFlags(),
                        (KrkrTjs2Mode)GetCurrentTjs2Flags(),
                        (KrkrTlgMode) GetCurrentTlgFlags(),
                        (KrkrAmvMode) GetCurrentAmvFlags(),
                        (KrkrPbdMode) GetCurrentPbdFlags()
                    );

                if (!Status)
                    throw new NativeRpcException("Client.XeClientUniversalDumperModeChecked");

            }
            catch (NativeRpcException WhichException)
            {
                Console.WriteLine(WhichException.ToString());

                MessageBox.Show(
                    this,
                    WhichException.ToString(),
                    "KrkrExtract",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error
                );
            }
            catch (Exception WhichException)
            {
                Console.WriteLine(WhichException.ToString());

                MessageBox.Show(
                    this,
                    WhichException.ToString(),
                    "KrkrExtract",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error
                );
            }
        }

        private void KrkrMain_FormClosed(object sender, FormClosedEventArgs e)
        {
            Client.XeClientTaskCloseWindow();
        }

        private void KrkrMain_Load(object sender, EventArgs e)
        {

            var Secret = Environment.GetEnvironmentVariable("KrkrSecret");
            uint SecretValue = 0;
            var Status = uint.TryParse(Secret, out SecretValue);

            if (!Status)
                SecretValue = 0;


            try
            {
                Status = Client.XeCreateInstance(
                    true,
                    SecretValue,
                    10000,
                    5000
                    );

                if (!Status)
                    throw new Exception("Client.XeCreateInstance failed");

            }
            catch(Exception WhichException)
            {
                MessageBox.Show(
                    this,
                    "Client.XeCreateInstance failed\n" + WhichException.ToString(),
                    "KrkrExtract",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error
                );

                Environment.Exit(-1);
            }

            try
            {
                Status = Client.XeRunServer(
                    m_NotifyServerProgressChangedCallback,
                    m_NotifyServerLogOutputCallback,
                    m_NotifyServerUIReadyCallback,
                    m_NotifyServerMessageBoxCallback,
                    m_NotifyServerTaskStartAndDisableUICallback,
                    m_NotifyServerTaskEndAndEnableUICallback,
                    m_NotifyServerExitFromRemoteProcessCallback,
                    m_NotifyServerRaiseErrorCallback
                );


                if (!Status)
                    throw new Exception("Client.XeRunServer failed");
            }
            catch (Exception WhichException)
            {
                MessageBox.Show(
                    this,
                    "Client.XeCreateInstance failed\n" + WhichException.ToString(),
                    "KrkrExtract",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error
                );

                Environment.Exit(-1);
            }

        }

        private void KrkrMain_DragEnter(object sender, DragEventArgs e)
        {
            if (e.Data.GetDataPresent(DataFormats.FileDrop))
                e.Effect = DragDropEffects.Link;
            else
                e.Effect = DragDropEffects.None;
        }

        private void KrkrMain_DragDrop(object sender, DragEventArgs e)
        {
            string[] FileList = (string[])e.Data.GetData(DataFormats.FileDrop, false);

            if (FileList.Length == 0)
                return;

            var ArchivePath = FileList[0];
            
            try
            {
                var Status = Client.XeClientTaskDumpStart(
                        (KrkrPsbMode) GetCurrentPsbFlags(),
                        (KrkrTextMode)GetCurrentTextFlags(),
                        (KrkrPngMode) GetCurrentPngFlags(),
                        (KrkrTjs2Mode)GetCurrentTjs2Flags(),
                        (KrkrTlgMode) GetCurrentTlgFlags(),
                        (KrkrAmvMode) GetCurrentAmvFlags(),
                        (KrkrPbdMode) GetCurrentPbdFlags(),
                        ArchivePath
                    );

                if (!Status)
                    throw new Exception("Client.XeClientTaskDumpStart failed");
            }
            catch (Exception WhichException)
            {
                NotifyServerLogOutputWorker(LogLevel.LOG_ERROR, WhichException.ToString(), false);
            }
        }

        #endregion
    }
}
