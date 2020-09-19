using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Diagnostics;
using System.IO;
using WindowsPE;
using CommandLine;


namespace KrkrExtract.Lite
{
    static class Program
    {
        private static bool ConsoleIsAttached = false;
        private static StatusForm m_StatusForm = new StatusForm();

        private static bool NotifyError(string Info)
        {
            if (ConsoleIsAttached == false)
            {
                ConsoleIsAttached = NativeHelper.AllocConsole();
            }

            try
            {
                MessageBox.Show(Info, "KrkrExtract", MessageBoxButtons.OK, MessageBoxIcon.Error);
                Console.WriteLine(Info);
            }
            catch(Exception e)
            {
                return false;
            }

            return true;
        }

        private static bool NotifyStatus(string Info)
        {
            if (ConsoleIsAttached == false)
            {
                ConsoleIsAttached = NativeHelper.AllocConsole();
            }

            try
            {
                m_StatusForm.Text = Info;
                Console.WriteLine(Info);
                return true;
            }
            catch (Exception e)
            {
                return false;
            }
        }

        private static bool NotifyStart()
        {
            try
            {
                m_StatusForm.Begin();
                return true;
            }
            catch(Exception e)
            {
                return false;
            }
        }


        private static bool NotifyEnd()
        {
            try
            {
                m_StatusForm.End();
                return true;
            }
            catch (Exception e)
            {
                return false;
            }
        }


        [STAThread]
        static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            NativeHelper.SetProcessDPIAware();

            var DllLoader = new BuiltinDlls();
            DllLoader.ResolveInternalDlls();

            Environment.SetEnvironmentVariable("KrkrRunMode", "Local");
            
            var args = Environment.GetCommandLineArgs();

            if (args.Length >= 2)
            {
                string ProgramName = args[1];
                bool RunStaticAnalysis = true;
                if (args.Length >= 3 && args[2].ToLower() == "-noana")
                {
                    RunStaticAnalysis = false;
                }

                var Module = new StaticAnalysisAndCreateProcess(NotifyStatus, NotifyError, NotifyStart, NotifyEnd);
                Module.Run(ProgramName, RunStaticAnalysis);
            }
            else
            {
                Application.Run(new Form1());
            }
        }
    }
}
