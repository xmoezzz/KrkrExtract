using KrkrExtract.Lite;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Windows.Forms;
using WindowsPE;
using Microsoft.Win32.SafeHandles;
using System.Runtime.InteropServices;
using R2;

public class StaticAnalysisAndCreateProcess
{
    private bool IsConsoleMode = false;
    public StaticAnalysisAndCreateProcess(bool IsConsole)
    {
        IsConsoleMode = IsConsole;
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


    private void NotifyError(string error)
    {
        if (IsConsoleMode)
        {
            Console.WriteLine(error);
        }
        else
        {
            MessageBox.Show(
                error,
                "KrkrExtract",
                MessageBoxButtons.OK,
                MessageBoxIcon.Error
            );
        }
    }

    private void NotifyStatus(string status)
    {
        Console.WriteLine(status);
    }

    /// <summary>
    /// create process with desire filepath 
    /// and inject dll to the remote process
    /// </summary>
    /// <param name="FilePath">desire filepath</param>
    private bool CreateProcessWithDll(string FilePath)
    {
        if (!LoaderHelper.CreateProcessWithDll(FilePath, "KrkrExtract.Core.dll"))
        {
            NotifyError("Failed to create process...\nCode :" + Marshal.GetLastWin32Error());
            return false;
        }

        return true;
    }

    /// <summary>
    /// vaildate file and perform static analysis on this binary
    /// TODO : ...
    /// </summary>
    /// <param name="FilePath">the binary path</param>
    /// <returns>the VA of target function</returns>
    private UInt64 RunStaticAnalysis(string FilePath)
    {
        bool RunStaticAnalysis = true;
        if (RunStaticAnalysis && !KrkrHelper.IsKrkrEngine(FilePath))
        {
            NotifyStatus("Unsupported krkr engine or packed file");
            RunStaticAnalysis = false;
        }
        if (RunStaticAnalysis && !KrkrHelper.NeedStaticAnalysis(FilePath))
        {
            NotifyStatus("Skip static analysis");
            RunStaticAnalysis = false;
        }

        return StaticAnalysis.Run(FilePath);
    }



    public bool Run(string FilePath, bool PreformStaticAnalysis)
    {
        string ProgramName = FilePath;
        if (ProgramName.ToLower().EndsWith(".lnk"))
        {
            ProgramName = LnkHelper.GetShortcutTargetFile(ProgramName);
            if (string.IsNullOrEmpty(ProgramName))
            {
                NotifyError("Couldn't resolve symbolic link");
                return false;
            }
        }

        if (!ProgramName.ToLower().EndsWith(".exe"))
        {
            NotifyError("Only *.exe (PE files) are supported");
            return false;
        }

        if (CheckStaticAnalysisResultIsPersent(FilePath))
        {
            NotifyStatus("Found the last static analysis result");
            PreformStaticAnalysis = false;
        }

        if (PreformStaticAnalysis)
        {
            var address = RunStaticAnalysis(FilePath);
            var configFile = FilePath + ".krconfig";
            if (address != 0 && address != UInt64.MaxValue)
            {
                KrkrHelper.WriteKrkrConfigFile(configFile, address);
            }
        }

        return CreateProcessWithDll(FilePath);
    }
}
