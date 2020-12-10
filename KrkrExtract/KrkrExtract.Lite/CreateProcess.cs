using KrkrExtract.Lite;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Windows.Forms;
using Microsoft.Win32.SafeHandles;
using System.Runtime.InteropServices;

public class CreateProcess
{
    private bool IsConsoleMode = false;
    private bool AttachedConsole = false;
    public CreateProcess(bool IsConsole)
    {
        IsConsoleMode = IsConsole;
    }

    void TryAllocConsole()
    {
        if (AttachedConsole == false)
        {
            NativeHelper.AllocConsole();
            AttachedConsole = true;
        }
    }

    private void NotifyError(string error)
    {
        if (IsConsoleMode)
        {
            TryAllocConsole();
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
        TryAllocConsole();
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

    public bool Run(string FilePath)
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

        CreateProcessWithDll(FilePath);
        Environment.Exit(0);
        return true;
    }
}
