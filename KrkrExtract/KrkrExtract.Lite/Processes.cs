using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text;
using System.Threading.Tasks;
using System.Drawing;
using System.Windows.Forms;

namespace KrkrExtract.Lite
{
    public class Processes
    {
        public enum ProcessAccessFlags : uint
        {
            All = 0x001F0FFF,
            Terminate = 0x00000001,
            CreateThread = 0x00000002,
            VirtualMemoryOperation = 0x00000008,
            VirtualMemoryRead = 0x00000010,
            VirtualMemoryWrite = 0x00000020,
            DuplicateHandle = 0x00000040,
            CreateProcess = 0x000000080,
            SetQuota = 0x00000100,
            SetInformation = 0x00000200,
            QueryInformation = 0x00000400,
            QueryLimitedInformation = 0x00001000,
            Synchronize = 0x00100000
        }

        public List<ProcessView> GetAllProcesses()
        {
            Process[] allProcesses = null;
            List<ProcessView> procView = new List<ProcessView>();
            List<ProcessView> response = null;

            try
            {
                allProcesses = Process.GetProcesses();
                foreach (var item in allProcesses)
                {
                    var proc = new ProcessView();
                    proc.ProcName = item.ProcessName;
                    proc.Pid = item.Id;
                    proc.FullProcPath = GetMainModuleFilepath(item.Id);
                    proc.IsDenied = String.IsNullOrEmpty(proc.FullProcPath);
                    if (proc.IsDenied)
                    {
                        continue;
                    }

                    proc.IconImage = Icon.ExtractAssociatedIcon(proc.FullProcPath);
                    proc.Status = ProcessView.AnalysisStatus.Analyzing;
                    procView.Add(proc);
                }
                response = new List<ProcessView>(procView.OrderBy(x => x.Pid).ToList());
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.ToString());
                return new List<ProcessView>();
            }

            return response;
        }

        private bool IsDenied(Process proc)
        {
            try
            {
                return !proc.HasExited;
            }
            catch
            {
                return false;
            }
        }

        public int KillProcess(int id)
        {
            try
            {
                Process proc = Process.GetProcessById(id);
                proc.Kill();
                return id;
            }
            catch (Exception ex)
            {
                return 0;
            }
        }
        //Get information about process path
        public string GetMainModuleFilepath(int procId)
        {
            IntPtr op = IntPtr.Zero;
            try
            {
                op = OpenProcess(ProcessAccessFlags.QueryLimitedInformation, false, procId);
                StringBuilder buffer = new StringBuilder(1024);
                int capacity = buffer.Capacity;
                if (QueryFullProcessImageName(op, 0, buffer, ref capacity))
                {
                    return buffer.ToString();
                }
            }
            catch (Exception ex)
            {
                return "";
            }
            finally
            {
                CloseHandle(op);
            }

            return "";
        }

        //Get dll hash information
        private void GetModuleHash(string path, out string md5, out string sha1)
        {
            byte[] hashMd5;
            byte[] hashSha1;
            md5 = "";
            sha1 = "";
            try
            {

                using (Stream fs = File.OpenRead(path))
                {
                    hashMd5 = MD5.Create().ComputeHash(fs);
                    md5 = BitConverter.ToString(hashMd5).Replace("-", "").ToLowerInvariant();
                }
                using (Stream fs = File.OpenRead(path))
                {
                    hashSha1 = SHA1.Create().ComputeHash(fs);
                    sha1 = BitConverter.ToString(hashSha1).Replace("-", "").ToLowerInvariant();
                }

            }
            catch (Exception ex)
            {
                md5 = ex.Message;
                sha1 = ex.Message;
            }
        }

        [DllImport("kernel32.dll", SetLastError = true)]
        public static extern IntPtr OpenProcess(ProcessAccessFlags processAccess, bool bInheritHandle, int processId);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool QueryFullProcessImageName(IntPtr hProcess, int dwFlags, StringBuilder lpExeName, ref int lpdwSize);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool CloseHandle(IntPtr hObject);
    }
}
