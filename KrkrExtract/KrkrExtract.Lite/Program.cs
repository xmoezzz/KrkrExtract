using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Diagnostics;
using System.IO;


namespace KrkrExtract.Lite
{

    static class Program
    {
        [STAThread]
        static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            NativeHelper.SetProcessDPIAware();
            Environment.SetEnvironmentVariable("KrkrRunMode", "Local");

            var args = Environment.GetCommandLineArgs();
            if (args.Length == 2)
            {
                try
                {
                    var pe = new PeHeaderReader(args[1]);
                }
                catch(Exception e)
                {
                    Environment.Exit(0);
                }

                var Module = new CreateProcess(true);
                Module.Run(args[1]);
                Environment.Exit(0);
            }
            else
            {
                Application.Run(new Form1());
            }
        }
    }
}
