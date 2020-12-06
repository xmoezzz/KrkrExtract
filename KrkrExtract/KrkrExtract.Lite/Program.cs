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
    public class Options
    {
        [Option('s', "static", Required = false, HelpText = "Preform static analysis on this binary.")]
        public bool PreformStaticAnalysis { get; set; }
    }

    static class Program
    {
        [STAThread]
        static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            NativeHelper.SetProcessDPIAware();

            var UnmanagedDllStatus = EmbeddedDllClass.LoadUnmanagedDlls();

            Environment.SetEnvironmentVariable("KrkrRunMode", "Local");

            var args = Environment.GetCommandLineArgs();
            if (args.Length > 2)
            {
                Parser.Default.ParseArguments<Options>(args)
                   .WithParsed<Options>(o =>
                   {
                       var filename = args.Last();
                       var Module = new StaticAnalysisAndCreateProcess(true);
                       var finalStatus = UnmanagedDllStatus && o.PreformStaticAnalysis;
                       Module.Run(filename, finalStatus);
                       Environment.Exit(0);
                   });
            }
            else
            {
                Application.Run(new Form1(UnmanagedDllStatus));
            }
        }
    }
}
