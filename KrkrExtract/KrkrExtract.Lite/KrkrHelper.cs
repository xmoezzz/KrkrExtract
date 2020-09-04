using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;
using WindowsPE;
using Newtonsoft.Json;

namespace KrkrExtract.Lite
{
    public static class KrkrHelper
    {
        public static bool NeedStaticAnalysis(string FilePath)
        {
            try
            {
                var pe = PEImage.ReadFromFile(FilePath);
                if (pe.GetExportFunctions() == null)
                    return true;

                foreach (var func in pe.GetExportFunctions())
                {
                    if (func.Name == "TVPGetFunctionExporter")
                    {
                        return false;
                    }
                }
            }
            catch (Exception e)
            {
                Console.WriteLine(Environment.StackTrace);
                Console.WriteLine("Exception:");
                Console.WriteLine(e.ToString());
                return false;
            }

            return true;
        }


        public static bool IsKrkrEngine(string FilePath)
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
        }

        public static Dictionary<String, UInt64> ReadKrkrConfigFile(string ConfigFile)
        {
            try
            {
                using (var stream = new BinaryReader(File.Open(ConfigFile, FileMode.Open, FileAccess.Read)))
                {
                    if (stream.BaseStream.Length <= 4)
                        throw new Exception("bad size");

                    var magic = stream.ReadInt32();
                    if (magic != 0x11223344)
                        throw new Exception("bad magic");

                    var buffer = stream.ReadBytes((int)stream.BaseStream.Length - 4);
                    for (int i = 0; i < (int)stream.BaseStream.Length - 4; i++)
                        buffer[i] ^= 0x6F;

                    var config = JsonConvert.DeserializeObject<Dictionary<String, UInt64>>(Encoding.UTF8.GetString(buffer));
                    return config;
                }
            }
            catch (Exception e)
            {
                Console.WriteLine(Environment.StackTrace);
                Console.WriteLine("Exception:");
                Console.WriteLine(e.ToString());
            }

            return new Dictionary<String, UInt64>();
        }

        public static bool WriteKrkrConfigFile(string ConfigFile, UInt64 ExporterOffset)
        {
            try
            {
                var config = new Dictionary<String, UInt64>();
                if (File.Exists(ConfigFile))
                    config = ReadKrkrConfigFile(ConfigFile);

                config["exporter"] = ExporterOffset;
                
                var buffer = Encoding.UTF8.GetBytes(JsonConvert.SerializeObject(config));
                for (int i = 0; i < buffer.Length; i++)
                    buffer[i] ^= 0x6F;

                using (var stream = new BinaryWriter(File.Create(ConfigFile)))
                {
                    var magic = new byte[4] { 0x44, 0x33, 0x22, 0x11 };
                    stream.Write(magic);
                    stream.Write(buffer);
                }
            }
            catch (Exception e)
            {
                Console.WriteLine(Environment.StackTrace);
                Console.WriteLine("Exception:");
                Console.WriteLine(e.ToString());
                return false;
            }

            return true;
        }

    }
}
