using System;
using System.IO;
using System.Windows.Forms;

using Shell32;

namespace KrkrExtract
{
    public partial class MainForm : Form
    {
        public MainForm()
        {
            InitializeComponent();
            comboBox_mode.SelectedIndex = 0;
        }

        private void KrkrExtract_DragEnter(object sender, DragEventArgs e)
        {
            e.Effect = e.Data.GetDataPresent(DataFormats.FileDrop) ? DragDropEffects.Move : DragDropEffects.None;
        }

        private void KrkrExtract_DragDrop(object sender, DragEventArgs e)
        {
            var files = (string[])e.Data.GetData(DataFormats.FileDrop, false);
            if (files.Length != 1)
            {
                MessageBox.Show("DrapDrop only accept one file", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            var dll = textBox_dll.Text;
            if (checkBox_absolute.Checked)
            {
                dll = Path.GetFullPath(dll);
            }
            
            var file = files[0];
            if (file.ToLower().EndsWith(".lnk"))
            {
                var shell = new Shell();
                var folder = shell.NameSpace(Path.GetDirectoryName(file)).ParseName(Path.GetFileName(file));
                if (folder == null)
                {
                    MessageBox.Show("Failed to parse shortcut", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    return;
                }
                if (folder.GetLink is ShellLinkObject link)
                {
                    Program.StartInjectProcess(link.Path, link.Arguments, link.WorkingDirectory, dll);
                }
                else
                {
                    MessageBox.Show("Failed to parse shortcut", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    return;
                }
            }
            else
            {
                Program.StartInjectProcess(file, "", null, dll);
            }

            if (checkBox_close.Checked)
            {
                Close();
            }
        }

        private void comboBox_mode_SelectedIndexChanged(object sender, EventArgs e) => Environment.SetEnvironmentVariable("KrkrMode", comboBox_mode.Text);
    }
}
