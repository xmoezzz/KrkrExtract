using KrkrExtract.Lite;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Windows.Forms;
using WindowsPE;
using Microsoft.Win32.SafeHandles;
using System.Runtime.InteropServices;

public class StaticAnalysisAndCreateProcess
{
    private Process m_Process;

    private string m_FilePath;

    private string m_TempFile;

    private bool m_InTask;

    private Func<string, bool> m_NotifyStatus;

    private Func<string, bool> m_NotifyError;

    private Func<bool> m_NotifyStart;

    private Func<bool> m_NotifyEnd;

    public StaticAnalysisAndCreateProcess(Func<string, bool> NotifyStatus, Func<string, bool> NotifyError, Func<bool> NotifyStart, Func<bool> NotifyEnd)
    {
        m_NotifyStatus = NotifyStatus;
        m_NotifyError = NotifyError;
        m_NotifyStart = NotifyStart;
        m_NotifyEnd   = NotifyEnd;
    }

    public static bool CheckStaticAnalysisIsPresent()
    {
        string currentDirectory = Directory.GetCurrentDirectory();
        string ScriptPath = Path.Combine(Path.Combine(Path.Combine(Path.Combine(currentDirectory, "Externals"), "scripts"), "r2"), "find_private_proc.py");
        string R2Home = Path.Combine(Path.Combine(Path.Combine(currentDirectory, "Externals"), Environment.Is64BitOperatingSystem ? "radare2_64" : "radare2"), "bin");
        string PythonPath = Path.Combine(Path.Combine(Path.Combine(currentDirectory, "Externals"), "python"), "python.exe");
        if (!Directory.Exists(R2Home))
        {
            Console.WriteLine("R2Home (not found) : {0}", R2Home);
            return false;
        }
        if (!File.Exists(ScriptPath))
        {
            Console.WriteLine("ScriptPath (not found) : {0}", ScriptPath);
            return false;
        }
        if (!File.Exists(PythonPath))
        {
            Console.WriteLine("PythonPath (not found) : {0}", PythonPath);
            return false;
        }
        return true;
    }

    public static bool CheckStaticAnalysisResultIsPersent(string FilePath)
    {
        string ConfigFile = FilePath + ".krconfig";
        if (!File.Exists(ConfigFile))
        {
            return false;
        }
        try
        {
            Dictionary<string, ulong> config = KrkrHelper.ReadKrkrConfigFile(ConfigFile);
            if (config.Count == 0)
            {
                return false;
            }
            if (!config.ContainsKey("exporter"))
            {
                return false;
            }
            if (config["exporter"] == ulong.MaxValue || config["exporter"] == 0L)
            {
                return false;
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine(Environment.StackTrace);
            Console.WriteLine("Exception:");
            Console.WriteLine(ex.ToString());
            return false;
        }
        return true;
    }

    private string JoinToArgs(string[] args)
    {
        string retv = "";
        foreach (string v in args)
        {
            retv = retv + "\"" + v + "\" ";
        }
        return retv;
    }

    private void CleanupVars()
    {
        m_FilePath = null;
        m_TempFile = null;
        if (m_Process != null)
        {
            m_Process = null;
        }
        m_InTask = false;
    }

    private void ProcessExitHandler(object sender, EventArgs eventArgs)
    {
        m_NotifyEnd();
        if (!File.Exists(m_TempFile) || m_Process.ExitCode != 0)
        {
            Console.WriteLine("static analysis failed : {0}", m_Process.ExitCode);
            m_NotifyStatus("static analysis failed");
            CleanupVars();
            return;
        }
        ulong ExporterOffset = 0uL;
        try
        {
            using (StreamReader stream = new StreamReader(File.Open(m_TempFile, FileMode.Open, FileAccess.Read)))
            {
                ExporterOffset = ulong.Parse(stream.ReadLine().Replace(Environment.NewLine, string.Empty));
                PEImage pe = PEImage.ReadFromFile(m_FilePath);
                if (ExporterOffset <= (ulong)(long)pe.BaseAddress)
                {
                    throw new Exception("Exporter address < pe.BaseAddress");
                }
                ExporterOffset = (ulong)((long)ExporterOffset - (long)pe.BaseAddress);
            }
            File.Delete(m_TempFile);
        }
        catch (Exception ex)
        {
            Console.WriteLine(Environment.StackTrace);
            Console.WriteLine("Exception:");
            Console.WriteLine(ex.ToString());
            CleanupVars();
            return;
        }
        if (ExporterOffset == 0L)
        {
            m_NotifyStatus("failed to parse exporter address");
            CleanupVars();
            return;
        }
        try
        {
            if (!KrkrHelper.WriteKrkrConfigFile(m_FilePath + ".krconfig", ExporterOffset))
            {
                m_NotifyStatus("failed to write info to config file");
                CleanupVars();
                return;
            }
        }
        catch (Exception)
        {
            m_NotifyStatus("failed to write info to config file");
            CleanupVars();
            return;
        }
        m_NotifyStatus("Static analysis : ok");
        CreateProcessAndExit(m_FilePath);
        CleanupVars();
    }

    private void CreateProcessAndExit(string FilePath)
    {
        uint ProcessId = 0u;
        SafeFileHandle Handle = new SafeFileHandle(IntPtr.Zero, false);
        try
        {
            Handle = NativeHelper.CreateProcessInternalWithDll(FilePath, KrkrMode.NORMAL, ref ProcessId);
        }
        catch (DllNotFoundException e)
        {
            MessageBox.Show("LoaderHelper.dll not found", "KrkrExtract", MessageBoxButtons.OK, MessageBoxIcon.Hand);
            Environment.Exit(0);
        }

        if (Handle.IsInvalid)
        {
            MessageBox.Show("Failed to create process...\nCode :" + Marshal.GetLastWin32Error(), "KrkrExtract", MessageBoxButtons.OK, MessageBoxIcon.Error);
            Environment.Exit(-1);
        }
        Handle.Close();
        Environment.Exit(0);
    }

    private bool RunStaticAnalysisSubProcess(string FilePath)
    {
        bool RunStaticAnalysis = true;
        if (RunStaticAnalysis && !KrkrHelper.IsKrkrEngine(FilePath))
        {
            m_NotifyStatus("Unsupported krkr engine or packed file");
            RunStaticAnalysis = false;
        }
        if (RunStaticAnalysis && !KrkrHelper.NeedStaticAnalysis(FilePath))
        {
            m_NotifyStatus("Skip static analysis");
            RunStaticAnalysis = false;
        }
        m_InTask = true;
        string CurrentDir = Directory.GetCurrentDirectory();
        string ScriptPath = Path.Combine(Path.Combine(Path.Combine(Path.Combine(CurrentDir, "Externals"), "scripts"), "r2"), "find_private_proc.py");
        string R2Home = Path.Combine(Path.Combine(Path.Combine(CurrentDir, "Externals"), Environment.Is64BitOperatingSystem ? "radare2_64" : "radare2"), "bin");
        string PythonPath = Path.Combine(Path.Combine(Path.Combine(CurrentDir, "Externals"), "python"), "python.exe");

        string TempFile = Path.Combine(CurrentDir, "3389.bin");
        ProcessStartInfo info = new ProcessStartInfo(PythonPath, JoinToArgs(new string[4]
        {
            ScriptPath,
            FilePath,
            R2Home,
            TempFile
        }));


        info.UseShellExecute = false;
        m_Process = new Process();
        m_Process.Exited += ProcessExitHandler;
        m_Process.StartInfo = info;
        m_Process.EnableRaisingEvents = true;
        m_TempFile = TempFile;
        m_FilePath = FilePath;
        try
        {
            m_Process.Start();
            m_NotifyStatus("Analyzing");
        }
        catch (Exception ex)
        {
            Console.WriteLine(Environment.StackTrace);
            Console.WriteLine("Exception:");
            Console.WriteLine(ex.ToString());
            CleanupVars();
            return false;
        }
        m_NotifyStart();
        return true;
    }

    public bool InTask()
    {
        return m_InTask;
    }

    public bool KillSubProcess()
    {
        try
        {
            if (m_Process != null)
            {
                m_Process.Kill();
            }
        }
        catch (Exception)
        {
            return false;
        }
        return true;
    }

    public bool Run(string FilePath, bool RunStaticAnalysis)
    {
        if (m_InTask)
        {
            return false;
        }
        string ProgramName = FilePath;
        if (ProgramName.ToLower().EndsWith(".lnk"))
        {
            ProgramName = LnkHelper.GetShortcutTargetFile(ProgramName);
            if (string.IsNullOrEmpty(ProgramName))
            {
                m_NotifyError("Couldn't resolve symbolic link");
                CleanupVars();
                return false;
            }
        }

        if (!ProgramName.ToLower().EndsWith(".exe"))
        {
            m_NotifyError("Only *.exe (PE files) are supported");
            CleanupVars();
            return false;
        }


        m_FilePath = FilePath;

        if (CheckStaticAnalysisResultIsPersent(FilePath))
        {
            m_NotifyStatus("Found the last static analysis result");
            CleanupVars();
            RunStaticAnalysis = false;
        }

        if (!RunStaticAnalysis)
        {
            CreateProcessAndExit(ProgramName);
        }


        m_InTask = true;
        return RunStaticAnalysisSubProcess(ProgramName);
    }
}
