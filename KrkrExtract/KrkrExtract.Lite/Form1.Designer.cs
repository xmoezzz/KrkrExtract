namespace KrkrExtract.Lite
{
    partial class Form1
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(Form1));
            this.label1 = new System.Windows.Forms.Label();
            this.StaticAnalysisCheckBox = new System.Windows.Forms.CheckBox();
            this.ProgressBar = new System.Windows.Forms.ProgressBar();
            this.StatusLabel = new System.Windows.Forms.Label();
            this.SuspendLayout();
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(99, 40);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(815, 96);
            this.label1.TabIndex = 0;
            this.label1.Text = "                            !!!Drop executable file here!!!\r\nIf you wish to use s" +
    "tatic analysis,please copy `Externals` to here\r\n                        (static " +
    "analysis is enabled by default)";
            // 
            // StaticAnalysisCheckBox
            // 
            this.StaticAnalysisCheckBox.AutoSize = true;
            this.StaticAnalysisCheckBox.Location = new System.Drawing.Point(678, 255);
            this.StaticAnalysisCheckBox.Name = "StaticAnalysisCheckBox";
            this.StaticAnalysisCheckBox.Size = new System.Drawing.Size(336, 36);
            this.StaticAnalysisCheckBox.TabIndex = 1;
            this.StaticAnalysisCheckBox.Text = "Enable Static Analysis";
            this.StaticAnalysisCheckBox.UseVisualStyleBackColor = true;
            this.StaticAnalysisCheckBox.CheckedChanged += new System.EventHandler(this.StaticAnalysisCheckBox_CheckedChanged);
            // 
            // ProgressBar
            // 
            this.ProgressBar.Location = new System.Drawing.Point(38, 167);
            this.ProgressBar.Name = "ProgressBar";
            this.ProgressBar.Size = new System.Drawing.Size(942, 66);
            this.ProgressBar.TabIndex = 2;
            // 
            // StatusLabel
            // 
            this.StatusLabel.AutoSize = true;
            this.StatusLabel.Location = new System.Drawing.Point(48, 255);
            this.StatusLabel.Name = "StatusLabel";
            this.StatusLabel.Size = new System.Drawing.Size(123, 32);
            this.StatusLabel.TabIndex = 3;
            this.StatusLabel.Text = "Status---";
            this.StatusLabel.TextChanged += new System.EventHandler(this.StatusLabel_TextChanged);
            // 
            // Form1
            // 
            this.AllowDrop = true;
            this.AutoScaleDimensions = new System.Drawing.SizeF(16F, 31F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(1017, 303);
            this.Controls.Add(this.StatusLabel);
            this.Controls.Add(this.ProgressBar);
            this.Controls.Add(this.StaticAnalysisCheckBox);
            this.Controls.Add(this.label1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.Name = "Form1";
            this.Text = "KrkrExtract.Lite";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.Form1_FormClosing);
            this.DragDrop += new System.Windows.Forms.DragEventHandler(this.Form1_DragDrop);
            this.DragEnter += new System.Windows.Forms.DragEventHandler(this.Form1_DragEnter);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.CheckBox StaticAnalysisCheckBox;
        private System.Windows.Forms.ProgressBar ProgressBar;
        private System.Windows.Forms.Label StatusLabel;
    }
}

