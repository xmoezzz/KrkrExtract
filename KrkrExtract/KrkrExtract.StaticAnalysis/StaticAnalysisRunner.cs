using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;
using WindowsPE;
using R2;

namespace KrkrExtract.StaticAnalysis
{
    class StaticAnalysisRunner
    {
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
                Console.WriteLine("Unsupported krkr engine or packed file");
                RunStaticAnalysis = false;
            }
            if (RunStaticAnalysis && !KrkrHelper.NeedStaticAnalysis(FilePath))
            {
                Console.WriteLine("Skip static analysis");
                RunStaticAnalysis = false;
            }

            if (RunStaticAnalysis)
            {
                return R2.StaticAnalysis.Run(FilePath);
            }

            return 0;
        }


        public bool Run(string FilePath)
        {
            if (!FilePath.ToLower().EndsWith(".exe"))
            {
                Console.WriteLine("Only *.exe (PE files) are supported");
                return false;
            }

            if (CheckStaticAnalysisResultIsPersent(FilePath))
            {
                Console.WriteLine("Found the last static analysis result");
                return true;
            }

            var address = RunStaticAnalysis(FilePath);
            var configFile = FilePath + ".krconfig";
            if (address != 0 && address != UInt64.MaxValue)
            {
                Console.WriteLine("Ok : {0}", address);
                return KrkrHelper.WriteKrkrConfigFile(configFile, address);
            }

            Console.WriteLine("failed...");
            return false;
        }
    }
}
