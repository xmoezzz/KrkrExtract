using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Microsoft.Win32;

namespace KrkrExtractInstaller
{
    class ShellExtensionManager
    {
        private static string clsid = "{748F806D-022B-480D-B15F-50D80DAF232A}";
        private static string fileType = "*";
        private static string friendlyName = "KrkrExtract.KeContextMenuHandler Class";
        private static string keyName = $@"Software\Classes\{fileType}\shellex\ContextMenuHandlers\{clsid}";

        public static void RegisterShellExtContextMenuHandler(bool allUsers)
        {
            var rootName = allUsers ? Registry.LocalMachine : Registry.CurrentUser;

            using (var key = rootName.CreateSubKey(keyName))
            {
                key?.SetValue(null, friendlyName);
            }
        }

        public static void UnregisterShellExtContextMenuHandler(bool allUsers)
        {
            var rootName = allUsers ? Registry.LocalMachine : Registry.CurrentUser;

            rootName.DeleteSubKeyTree(keyName);
        }

        public static bool IsInstalled(bool allUsers)
        {
            var rootName = allUsers ? Registry.LocalMachine : Registry.CurrentUser;

            return rootName.OpenSubKey(keyName, false) != null;
        }
    }
}
