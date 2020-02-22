using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Runtime.InteropServices;

namespace KrkrExtract
{
    public enum KrkrMode
    {
        NORMAL = 0,
        BASIC_LOCK = 1,
        ADV_LOCK = 2,
        HYPERVISOR = 3
    }


    public partial class KrkrExtract : Form
    {
        private KrkrMode m_Mode;
        public KrkrExtract()
        {
            InitializeComponent();
            m_Mode = KrkrMode.NORMAL;
            ModeComboBox.SelectedIndex = 0;
        }

        [DllImport("LoaderHelper.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool CreateProcessInternalWithDll([MarshalAs(UnmanagedType.LPWStr), In]string ProcessName, KrkrMode Mode);

        private void ModeComboBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            ComboBox box = (ComboBox)sender;
            switch (box.SelectedIndex)
            {
                case 0:
                    m_Mode = KrkrMode.NORMAL;
                    break;

                case 1:
                    m_Mode = KrkrMode.BASIC_LOCK;
                    break;

                case 2:
                    m_Mode = KrkrMode.ADV_LOCK;
                    break;

                case 3:
                    m_Mode = KrkrMode.HYPERVISOR;
                    break;
            }
        }

        public string GetShortcutTargetFile(string shortcutFilename)
        {
            string pathOnly = System.IO.Path.GetDirectoryName(shortcutFilename);
            string filenameOnly = System.IO.Path.GetFileName(shortcutFilename);

            Shell32.Shell shell = new Shell32.Shell();
            Shell32.Folder folder = shell.NameSpace(pathOnly);
            Shell32.FolderItem folderItem = folder.ParseName(filenameOnly);
            if (folderItem != null)
            {
                Shell32.ShellLinkObject link = (Shell32.ShellLinkObject)folderItem.GetLink;
                return link.Path;
            }
            return null;
        }

        private void KrkrExtract_DragDrop(object sender, DragEventArgs e)
        {
            string[] FileList = (string[])e.Data.GetData(DataFormats.FileDrop, false);
            if (FileList.Length > 1)
            {
                MessageBox.Show("DrapDrop only accept one file", "KrkrExtract", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            string ProgramName = FileList[0];
            if (ProgramName.ToLower().EndsWith(".lnk"))
            {
                ProgramName = GetShortcutTargetFile(ProgramName);
                if(string.IsNullOrEmpty(ProgramName))
                {
                    MessageBox.Show("Couldn't resolve symbolic link", "KrkrExtract", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    return;
                }
            }

            if (!ProgramName.ToLower().EndsWith(".exe"))
            {
                MessageBox.Show("Only *.exe (PE files) are supported", "KrkrExtract", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            if (!CreateProcessInternalWithDll(ProgramName, m_Mode))
            {
                MessageBox.Show("Failed to launch executable...", "KrkrExtract", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            else
            {
                Environment.Exit(0);
            }

        }

        private void KrkrExtract_DragEnter(object sender, DragEventArgs e)
        {
            if (e.Data.GetDataPresent(DataFormats.FileDrop))
                e.Effect = DragDropEffects.Link;
            else
                e.Effect = DragDropEffects.None;
        }
        
    }
}
