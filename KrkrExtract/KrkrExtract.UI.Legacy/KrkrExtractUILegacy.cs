using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace KrkrExtract.UI.Legacy
{
    public class KrkrExtractUILegacy
    {
        public void Run(IntPtr RemoteProcess)
        {
            var form = new KrkrMain(RemoteProcess);
            form.ShowDialog();
            form.Dispose();
        }
    }
}
