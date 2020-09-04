using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using NtApiDotNet;
using WindowsPE;

namespace KrkrExtract.Lite
{
    static class Injector
    {
        public static bool CreateProcessWithDll(string FileName, string DllName)
        {

        }

        private static bool CreateProcessWithDll32(string FileName, string DllName)
        {
            NtApiDotNet.ContextX86 Context;
        }

        private static bool CreateProcessWithDll64(string FileName, string DllName)
        {
            NtApiDotNet.ContextAmd64 Context;
        }
    }
}
