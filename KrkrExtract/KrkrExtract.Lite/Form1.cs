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
using WindowsPE;
using Newtonsoft.Json;

namespace KrkrExtract.Lite
{
    public partial class Form1 : Form
    {

        private StaticAnalysisAndCreateProcess m_Module = null;
        public Form1()
        {
            InitializeComponent();
            StatusLabel.Text = "";
            m_Module = new StaticAnalysisAndCreateProcess(NotifyStatus, NotifyError, NotifyStart, NotifyEnd);
            if (!StaticAnalysisAndCreateProcess.CheckStaticAnalysisIsPresent())
            {
                StaticAnalysisCheckBox.Enabled = false;
                StaticAnalysisCheckBox.Checked = false;
                ProgressBar.Enabled = false;
                StatusLabel.Text = "No static analysis module.";
            }
            else
            {
                StaticAnalysisCheckBox.Enabled = true;
                StaticAnalysisCheckBox.Checked = true;
                ProgressBar.Enabled = true;
            }
        }

        private void StaticAnalysisCheckBox_CheckedChanged(object sender, EventArgs e)
        {

        }

        private void Form1_DragEnter(object sender, DragEventArgs e)
        {
            if (e.Data.GetDataPresent(DataFormats.FileDrop))
                e.Effect = DragDropEffects.Link;
            else
                e.Effect = DragDropEffects.None;
        }

        private bool NotifyStart()
        {
            try
            {
                ProgressBar.Style = ProgressBarStyle.Marquee;
            }
            catch(Exception e)
            {
                return false;
            }

            return true;
        }

        private bool NotifyEnd()
        {
            try
            {
                ProgressBar.Style = ProgressBarStyle.Blocks;
                ProgressBar.Value = ProgressBar.Minimum;
            }
            catch(Exception e)
            {
                return false;
            }
            return true;
        }

        private bool NotifyStatus(string Info)
        {
            try
            {
                StatusLabel.Text = Info;
            }
            catch(Exception e)
            {
                return false;
            }

            return true;
        }

        private bool NotifyError(string Info)
        {
            try
            {
                MessageBox.Show(Info, "KrkrExtract", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            catch(Exception e)
            {
                return false;
            }

            return true;
        }

        private void Form1_DragDrop(object sender, DragEventArgs e)
        {
            if (m_Module.InTask())
                return;

            string[] FileList = (string[])e.Data.GetData(DataFormats.FileDrop, false);
            if (FileList.Length > 1)
            {
                MessageBox.Show("DrapDrop only accepts one file", "KrkrExtract", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            string ProgramName = FileList[0];
            m_Module.Run(ProgramName, StaticAnalysisCheckBox.Checked);
        }

        private void Form1_FormClosing(object sender, FormClosingEventArgs e)
        {
            m_Module.KillSubProcess();
        }

        private bool ConsoleIsAttached = false;

        private void StatusLabel_TextChanged(object sender, EventArgs e)
        {
            if (ConsoleIsAttached == false)
            {
                ConsoleIsAttached = NativeHelper.AllocConsole();
            }
        }
    }
}
