using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;


namespace KrkrExtract
{
    static class Program
    {
        static bool CheckMutex()
        {
            Mutex KrkrExtractLock;

            return Mutex.TryOpenExisting("{KrkrExtract_run_once_per_session}", out KrkrExtractLock);
        }

        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main()
        {
            SetProcessDPIAware();
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);

            if (CheckMutex())
            {
                MessageBox.Show("Waiting for another task...", "KrkrExtract", MessageBoxButtons.OK, MessageBoxIcon.Error);
                Environment.Exit(-1);
            }


            var args = Environment.GetCommandLineArgs();
            if (args.Length >= 2)
            {
                string ProgramName = args[1];
                if (KrkrExtract.CreateProcessInternalWithDll(ProgramName, KrkrMode.NORMAL) == NativeHelper.INVALID_HANDLE_VALUE)
                {
                    MessageBox.Show("Failed to launch executable...", "KrkrExtract", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    Environment.Exit(-1);
                }

                UILoader.LoadUIAndRun();

            }
            else
            {
                Application.Run(new KrkrExtract());
                UILoader.LoadUIAndRun();
            }
        }

        [System.Runtime.InteropServices.DllImport("User32.dll")]
        private static extern bool SetProcessDPIAware();
    }
}
