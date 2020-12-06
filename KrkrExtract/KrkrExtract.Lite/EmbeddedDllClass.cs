using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Reflection;
using System.Diagnostics;
using System.IO;
using System.ComponentModel;
using System.Security.Cryptography;

namespace KrkrExtract.Lite
{
    public static class EmbeddedDllClass
    {
        static private bool CallOnlyOnce = false;

        static private List<string> Dlls = new List<string>()
        {
            "r_anal.dll",
            "r_asm.dll",
            "r_bin.dll",
            "r_bp.dll",
            "r_config.dll",
            "r_cons.dll",
            "r_core.dll",
            "r_crypto.dll",
            "r_debug.dll",
            "r_egg.dll",
            "r_flag.dll",
            "r_fs.dll",
            "r_hash.dll",
            "r_io.dll",
            "r_lang.dll",
            "r_magic.dll",
            "r_main.dll",
            "r_parse.dll",
            "r_reg.dll",
            "r_search.dll",
            "r_socket.dll",
            "r_syscall.dll",
            "r_util.dll"
        };


        private static byte[] LoadDllBytes(string dllName)
        {
            switch (dllName)
            {
                case "r_anal.dll":
                    return Properties.Resources.r_anal;
                case "r_asm.dll":
                    return Properties.Resources.r_asm;
                case "r_bin.dll":
                    return Properties.Resources.r_bin;
                case "r_bp.dll":
                    return Properties.Resources.r_bp;
                case "r_config.dll":
                    return Properties.Resources.r_config;
                case "r_cons.dll":
                    return Properties.Resources.r_cons;
                case "r_core.dll":
                    return Properties.Resources.r_core;
                case "r_crypto.dll":
                    return Properties.Resources.r_crypto;
                case "r_debug.dll":
                    return Properties.Resources.r_debug;
                case "r_egg.dll":
                    return Properties.Resources.r_egg;
                case "r_flag.dll":
                    return Properties.Resources.r_flag;
                case "r_fs.dll":
                    return Properties.Resources.r_fs;
                case "r_hash.dll":
                    return Properties.Resources.r_hash;
                case "r_io.dll":
                    return Properties.Resources.r_io;
                case "r_lang.dll":
                    return Properties.Resources.r_lang;
                case "r_magic.dll":
                    return Properties.Resources.r_magic;
                case "r_main.dll":
                    return Properties.Resources.r_main;
                case "r_parse.dll":
                    return Properties.Resources.r_parse;
                case "r_reg.dll":
                    return Properties.Resources.r_reg;
                case "r_search.dll":
                    return Properties.Resources.r_search;
                case "r_socket.dll":
                    return Properties.Resources.r_socket;
                case "r_syscall.dll":
                    return Properties.Resources.r_syscall;
                case "r_util.dll":
                    return Properties.Resources.r_util;
                default:
                    throw new Exception("Unknown res : " + dllName);
            }
            
        }


        private static void ApplyEnv()
        {
            if (CallOnlyOnce)
                return;
            
            Assembly assembly = Assembly.GetExecutingAssembly();
            string[] names = assembly.GetManifestResourceNames();
            AssemblyName assemblyName = assembly.GetName();

            string tempFolder = String.Format(
                "{0}.{1}.{2}", 
                assemblyName.Name, 
                assemblyName.ProcessorArchitecture, 
                assemblyName.Version
                );

            string dirName = Path.Combine(Path.GetTempPath(), tempFolder);
            if (!Directory.Exists(dirName))
            {
                Directory.CreateDirectory(dirName);
            }

            // Add the temporary dirName to the PATH environment variable (at the head!)
            string pathEnv = Environment.GetEnvironmentVariable("PATH");
            string[] paths = pathEnv.Split(';');
            bool found = false;

            foreach (string path in paths)
            {
                if (path == dirName)
                {
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                Environment.SetEnvironmentVariable("PATH", dirName + ";" + pathEnv);
            }
        }


        static bool ValidateAllDlls()
        {
            Assembly assembly = Assembly.GetExecutingAssembly();
            string[] names = assembly.GetManifestResourceNames();
            AssemblyName assemblyName = assembly.GetName();

            string tempFolder = String.Format(
                "{0}.{1}.{2}",
                assemblyName.Name,
                assemblyName.ProcessorArchitecture,
                assemblyName.Version
                );

            string dirName = Path.Combine(Path.GetTempPath(), tempFolder);
            if (!Directory.Exists(dirName))
            {
                return false;
            }

            foreach (var dllName in Dlls)
            {
                var dllPath = Path.Combine(dirName, dllName);
                if (!File.Exists(dllPath))
                    return false;

                var dllInfo = new FileInfo(dllPath);
                byte[] curBuffer = new byte[dllInfo.Length];

                using (FileStream file = File.Open(dllPath, FileMode.Open, FileAccess.Read, FileShare.None))
                {
                    int bytesTranferred = file.Read(curBuffer, 0, (int)dllInfo.Length);
                    if (bytesTranferred != (int)dllInfo.Length)
                        return false;
                }

                byte[] buffer = LoadDllBytes(dllName);
                int size = buffer.Length;

                if (!buffer.Equals(curBuffer))
                    return false;
            }

            return true;
        }

        static bool ExtractDllToTemp()
        {
            Assembly assembly = Assembly.GetExecutingAssembly();
            string[] names = assembly.GetManifestResourceNames();
            AssemblyName assemblyName = assembly.GetName();

            string tempFolder = String.Format(
                "{0}.{1}.{2}",
                assemblyName.Name,
                assemblyName.ProcessorArchitecture,
                assemblyName.Version
                );

            string dirName = Path.Combine(Path.GetTempPath(), tempFolder);
            if (!Directory.Exists(dirName))
            {
                Directory.CreateDirectory(dirName);
            }

            foreach (var dllName in Dlls)
            {
                var dllPath = Path.Combine(dirName, dllName);
                byte[] buffer = LoadDllBytes(dllName);
                int size = buffer.Length;
                
                using (FileStream file = File.Open(dllPath, FileMode.Create, FileAccess.Write, FileShare.None))
                {
                    file.Write(buffer, 0, buffer.Length);
                }
            }

            return true;
        }

        public static bool LoadUnmanagedDlls()
        {
            try
            {
                // bool status = ValidateAllDlls();

                bool status = ExtractDllToTemp();
                ApplyEnv();

                return status;
            }
            catch(Exception e)
            {
                Console.WriteLine(e.Message);
                Console.WriteLine(e.ToString());
            }

            return false;
        }

    }
}
