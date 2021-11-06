using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Drawing;

namespace KrkrExtract.Lite
{
    public class ProcessView
    {
        public enum AnalysisStatus
        {
            Analyzing = 0,
            Ok = 1,
            Failed = 2
        }

        public int Pid { get; set; }
        public string ProcName { get; set; }
        public string FullProcPath { get; set; }
        public bool IsDenied { get; set; }
        public AnalysisStatus Status { get; set; }
        public Icon IconImage { get; set; }
    }
}
