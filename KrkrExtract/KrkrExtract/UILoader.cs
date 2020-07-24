using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Reflection;
using System.Windows.Forms;
using System.IO;

namespace KrkrExtract
{
    static class UILoader
    {

        static dynamic LoadLegacyUI(IntPtr RemoteProcess)
        {
            try
            {
                var Dll = Assembly.LoadFile(Path.Combine(Directory.GetCurrentDirectory(), "KrkrExtract.UI.Legacy.dll"));

                foreach(Type ExportedType in Dll.GetExportedTypes())
                {
                    if (ExportedType.GetTypeInfo().Name == "KrkrExtractUILegacy")
                        return Activator.CreateInstance(ExportedType);
                }
                throw new EntryPointNotFoundException("KrkrExtractUILegacy not found");
            }
            catch(Exception e)
            {
                MessageBox.Show(e.ToString(), "KrkrExtract (Loadind UI)", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }

            return null;
            
        }

        static dynamic LoadFluentDesignUI()
        {
            try
            {
                var Dll = Assembly.LoadFile(Path.Combine(Directory.GetCurrentDirectory(), "KrkrExtract.UI.FluentDesign.dll"));

                foreach (Type ExportedType in Dll.GetExportedTypes())
                {
                    if (ExportedType.GetTypeInfo().Name == "KrkrExtractUIFluentDesign")
                        return Activator.CreateInstance(ExportedType);
                }
                throw new EntryPointNotFoundException("KrkrExtractUIFluentDesign not found");
            }
            catch (Exception e)
            {
                MessageBox.Show(e.ToString(), "KrkrExtract (Loadind UI)", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }

            return null;

        }


        public static void LoadUIAndRun(IntPtr RemoteProcess)
        {
            if (Environment.OSVersion.Version.Major == 10 &&
                Environment.OSVersion.Version.Minor == 1  &&
                Environment.OSVersion.Version.Revision >= 16299)
            {
                dynamic f = LoadLegacyUI(RemoteProcess);
                f.Run();
            }
            else
            {
                dynamic f = LoadLegacyUI(RemoteProcess);
                f.Run();
            }
        }
    }
}
