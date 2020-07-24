using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace KrkrExtract.Lite
{
    public partial class StatusForm : Form
    {
        public StatusForm()
        {
            InitializeComponent();
        }

        public bool Begin()
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

        public bool End()
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
    }
}
