using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;

namespace KrkrExtract.Lite
{
    public class KrkrHelper
    {
        public class KrkrInfo
        {
            public UInt64 ExporterOffset = 0;
        }

        private static Task<bool> IsKrkrEngine(string FilePath)
        {
            var task = Task.Run(() =>
            {
                var PatternZ = Encoding.Unicode.GetBytes("TVP(KIRIKIRI) Z core / Scripting Platform for Win32");
                var Pattern2 = Encoding.Unicode.GetBytes("TVP(KIRIKIRI) 2 core / Scripting Platform for Win32");

                var searcher1 = new BoyerMooreBinarySearch(PatternZ);
                var searcher2 = new BoyerMooreBinarySearch(Pattern2);

                if (searcher1.GetMatchIndexes(new FileInfo(FilePath)).Count == 0 &&
                    searcher2.GetMatchIndexes(new FileInfo(FilePath)).Count == 0)
                {
                    return false;
                }
                return true;
            });

            return task;
        }

        private static Task<KrkrInfo> RunKrkrAnalysis(string FilePath)
        {
            var task = Task.Run(() =>
            {
                var Info = new KrkrInfo();
                Info.ExporterOffset = R2.StaticAnalysis.Run(FilePath);
                return Info;
            });

            return task;
        }
    }
}

