using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace KrkrExtract.StaticAnalysis
{
    class Program
    {
        static void Main(string[] args)
        {
            if (args.Length <= 1)
            {
                Console.WriteLine("No input file...");
                Environment.Exit(-1);
            }

            var UnmanagedDllStatus = EmbeddedDllClass.LoadUnmanagedDlls();
            if (!UnmanagedDllStatus)
            {
                Console.WriteLine("Extract radare2 dll failed...");
                Environment.Exit(-1);
            }

            var Module = new StaticAnalysisRunner();
            Module.Run(args[1]);
            Environment.Exit(0);
        }
    }
}
