using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace KrkrExtract.UI.Legacy
{
    public class NativeRpcException : Exception
    {
        public NativeRpcException()
        {
        }

        public NativeRpcException(string message)
            : base(message)
        {
        }

        public NativeRpcException(string message, Exception inner)
            : base(message, inner)
        {
        }
    }
}
