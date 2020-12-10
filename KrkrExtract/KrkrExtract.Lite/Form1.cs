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

        public Form1()
        {
            InitializeComponent();
            m_Module = new CreateProcess(false);
        }

        private void Form1_DragEnter(object sender, DragEventArgs e)
        {
            if (e.Data.GetDataPresent(DataFormats.FileDrop))
                e.Effect = DragDropEffects.Link;
            else
                e.Effect = DragDropEffects.None;
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
    }
}
