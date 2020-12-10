/* --pancake */
using System;
using System.Runtime.InteropServices;
using System.Collections.Generic;
using System.Threading.Tasks;

namespace r2pipe
{
    public class DllR2Pipe : IR2Pipe
    {
        [DllImport("libr_core")]
        private static extern UIntPtr r_core_new();
        [DllImport("libr_core")]
        private static extern string r_core_cmd_str(UIntPtr core, string cmd);
        [DllImport("libr_core")]
        private static extern void r_core_free(UIntPtr core);



        /// <summary>
        /// The URI
        /// </summary>
        internal UIntPtr core;

        /// <summary>
        /// Initializes a new instance of the <see cref="DllR2Pipe"/> class.
        /// </summary>
        public DllR2Pipe()
        {
            this.core = r_core_new();
        }

        /// <summary>
        /// Executes given RunCommand in radare2
        /// </summary>
        /// <param name="command">The command to execute.</param>
        /// <returns>
        /// Returns a string
        /// </returns>
        public string RunCommand(string command)
        {
            return r_core_cmd_str(this.core, command);
        }

#if !OLDNETFX
        /// <summary>
        /// Executes given RunCommand in radare2 asynchronously
        /// </summary>
        /// <param name="command">The command to execute.</param>
        /// <returns>
        /// Returns a string
        /// </returns>
        public async Task<string> RunCommandAsync(string command)
        {
            // this is not async at all. use threads? :D
            return r_core_cmd_str(this.core, command);
        }
#endif

        public void Dispose()
        {
            r_core_free(this.core);
            this.core = new UIntPtr(0);
        }
    }
}

