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
using System.IO;
using System.Diagnostics;

namespace KrkrExtract.Lite
{
    public partial class Form1 : Form
    {

        private CreateProcess m_Module = null;
        private ImageList m_ImageListSmall = null;

        public Form1()
        {
            InitializeComponent();
            m_Module = new CreateProcess(false);
            m_ImageListSmall = new ImageList();
        }

        private void Form1_DragEnter(object sender, DragEventArgs e)
        {
            if (e.Data.GetDataPresent(DataFormats.FileDrop))
                e.Effect = DragDropEffects.Link;
            else
                e.Effect = DragDropEffects.None;
        }

        private void ReloadProcess()
        {
            listView1.Items.Clear();
            m_ImageListSmall.Images.Clear();
            m_ImageListSmall.ImageSize = new Size(16, 16);
            listView1.SmallImageList = m_ImageListSmall;
            Processes proc = new Processes();

            try
            {
                var response = proc.GetAllProcesses();
                foreach (var item in response)
                {
                    string[] row =
                    {
                        item.ProcName,
                        item.Pid.ToString(),
                        item.FullProcPath
                    };

                    var lv1 = new ListViewItem(row);
                    
                    listView1.Items.Add(lv1);
                    listView1.Items[listView1.Items.Count - 1].ImageIndex = m_ImageListSmall.Images.Count;
                    m_ImageListSmall.Images.Add(item.IconImage);
                }

                if (listView1.Items.Count == 0)
                {
                    MessageBox.Show("No krkr process found!", "KrkrExtract");
                }
            }
            catch (Exception ex)
            {

            }
        }

        private void Form1_DragDrop(object sender, DragEventArgs e)
        {
            this.Enabled = false;
            string[] FileList = (string[])e.Data.GetData(DataFormats.FileDrop, false);

            if (FileList.Length > 1)
            {
                MessageBox.Show(
                    "DrapDrop only accepts one file",
                    "KrkrExtract", 
                    MessageBoxButtons.OK, 
                    MessageBoxIcon.Error
                );

                this.Enabled = true;
                return;
            }

            string ProgramName = FileList[0];
            m_Module.Run(ProgramName);
            this.Enabled = true;
        }

        private void listToolStripMenuItem_Click(object sender, EventArgs e)
        {
            ReloadProcess();
        }

        private void refreshToolStripMenuItem_Click(object sender, EventArgs e)
        {
            ReloadProcess();
        }
    }
}
