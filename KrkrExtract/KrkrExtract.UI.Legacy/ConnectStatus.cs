using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace KrkrExtract.UI.Legacy
{
    public enum ConnectStatus
    {
        /// <summary>
        /// not connected, initial status
        /// </summary>
        Status_Not_Connect,

        /// <summary>
        /// both UI server and worker client are connected (not ready)
        /// </summary>
        Status_Connected,

        /// <summary>
        /// Ready to dump, pack, ...
        /// </summary>
        Status_Ready,

        /// <summary>
        /// disconnected from remote (maybe crash)
        /// if remote process wants to exit, it should communicate with UI server first
        /// </summary>
        Status_Disconnected
    }
}

