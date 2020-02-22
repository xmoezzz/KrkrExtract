using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace KrkrExtract
{
    static class Program
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main()
        {
            SetProcessDPIAware();
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);

            var args = Environment.GetCommandLineArgs();
            if (args.Length >= 2)
            {
                string ProgramName = args[1];
                if (!KrkrExtract.CreateProcessInternalWithDll(ProgramName, KrkrMode.NORMAL))
                    MessageBox.Show("Failed to launch executable...", "KrkrExtract", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            else
            {
                Application.Run(new KrkrExtract());
            }
        }

        [System.Runtime.InteropServices.DllImport("User32.dll")]
        private static extern bool SetProcessDPIAware();
    }
}
