using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;
using System.IO.MemoryMappedFiles;
using System.Diagnostics;
using System.Runtime.InteropServices;

namespace KrkrExtract.UI.Legacy
{
    public static class Helper
    {
        private static string _KrVersion   = null;
        private static string _KrBuildTime = null;


        static public string GetKrkrExtractVersion()
        {
            if (!string.IsNullOrEmpty(_KrVersion))
                return _KrVersion;

            try
            {
                var VersionInfo = FileVersionInfo.GetVersionInfo(
                    Path.Combine(
                        Environment.CurrentDirectory, 
                        "KrkrExtract.Core.dll"
                        )
                    );

                _KrVersion = VersionInfo.FileVersion;

                return VersionInfo.FileVersion;
            }
            catch (Exception e)
            {
                return "Unknown version";
            }
        }


        static public string GetKrkrExtractBuildTime()
        {
            if (!string.IsNullOrEmpty(_KrBuildTime))
                return _KrBuildTime;

            try
            {
                FileStream file = File.OpenRead(Path.Combine(Environment.CurrentDirectory, "KrkrExtract.Core.dll"));
                using (var mmf = MemoryMappedFile.CreateFromFile(file, null,
                                                      file.Length,
                                                      MemoryMappedFileAccess.Read,
                                                      null, HandleInheritability.None, false))
                {
                    NativeMethods.IMAGE_DOS_HEADER dosHeader = new NativeMethods.IMAGE_DOS_HEADER();
                    using (var accessor = mmf.CreateViewAccessor(0,
                                                                  Marshal.SizeOf(dosHeader),
                                                                  MemoryMappedFileAccess.Read))
                    {
                        accessor.Read<NativeMethods.IMAGE_DOS_HEADER>(0, out dosHeader);
                        if (dosHeader.e_magic != NativeMethods.IMAGE_DOS_SIGNATURE)
                            throw new Exception();
                    }


                    int signature = 0;
                    NativeMethods.IMAGE_FILE_HEADER imageFileHeader = new NativeMethods.IMAGE_FILE_HEADER();
                    using (var accessor = mmf.CreateViewAccessor(dosHeader.e_lfanew,
                                                                  Marshal.SizeOf(signature) + Marshal.SizeOf(imageFileHeader),
                                                                  MemoryMappedFileAccess.Read))
                    {
                        signature = accessor.ReadInt32(0);
                        if (signature != NativeMethods.IMAGE_NT_SIGNATURE)
                            throw new Exception();

                        accessor.Read<NativeMethods.IMAGE_FILE_HEADER>(Marshal.SizeOf(signature), out imageFileHeader);
                    }

                    DateTime origin = new DateTime(1970, 1, 1, 0, 0, 0, 0);
                    TimeSpan localOffset = TimeZone.CurrentTimeZone.GetUtcOffset(origin);
                    DateTime originUTC = origin.AddHours(localOffset.Hours);
                    DateTime linkTime = originUTC.AddSeconds((double)imageFileHeader.TimeDateStamp);
                    return linkTime.ToString();
                }
            }
            catch (Exception e)
            {
                return "Unknown time";
            }
        }
    }

}
