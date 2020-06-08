namespace KrkrExtract
{
    partial class MainForm
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(MainForm));
            this.label1 = new System.Windows.Forms.Label();
            this.comboBox_mode = new System.Windows.Forms.ComboBox();
            this.label2 = new System.Windows.Forms.Label();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.label3 = new System.Windows.Forms.Label();
            this.textBox_dll = new System.Windows.Forms.TextBox();
            this.checkBox_close = new System.Windows.Forms.CheckBox();
            this.checkBox_absolute = new System.Windows.Forms.CheckBox();
            this.groupBox1.SuspendLayout();
            this.SuspendLayout();
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(47, 20);
            this.label1.Margin = new System.Windows.Forms.Padding(1, 0, 1, 0);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(263, 12);
            this.label1.TabIndex = 0;
            this.label1.Text = "Drag and drop game executable/shortcut here";
            // 
            // comboBox_mode
            // 
            this.comboBox_mode.FormattingEnabled = true;
            this.comboBox_mode.Items.AddRange(new object[] {
            "Normal",
            "Basic Lockdown",
            "Advance Lockdown",
            "Hypervisor"});
            this.comboBox_mode.Location = new System.Drawing.Point(46, 18);
            this.comboBox_mode.Margin = new System.Windows.Forms.Padding(1);
            this.comboBox_mode.Name = "comboBox_mode";
            this.comboBox_mode.Size = new System.Drawing.Size(282, 20);
            this.comboBox_mode.TabIndex = 0;
            this.comboBox_mode.SelectedIndexChanged += new System.EventHandler(this.comboBox_mode_SelectedIndexChanged);
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(9, 21);
            this.label2.Margin = new System.Windows.Forms.Padding(1, 0, 1, 0);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(35, 12);
            this.label2.TabIndex = 1;
            this.label2.Text = "Mode:";
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.checkBox_absolute);
            this.groupBox1.Controls.Add(this.checkBox_close);
            this.groupBox1.Controls.Add(this.textBox_dll);
            this.groupBox1.Controls.Add(this.label3);
            this.groupBox1.Controls.Add(this.comboBox_mode);
            this.groupBox1.Controls.Add(this.label2);
            this.groupBox1.Location = new System.Drawing.Point(12, 47);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(334, 91);
            this.groupBox1.TabIndex = 2;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Settings";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(9, 45);
            this.label3.Margin = new System.Windows.Forms.Padding(1, 0, 1, 0);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(29, 12);
            this.label3.TabIndex = 2;
            this.label3.Text = "DLL:";
            // 
            // textBox_dll
            // 
            this.textBox_dll.Location = new System.Drawing.Point(46, 42);
            this.textBox_dll.Name = "textBox_dll";
            this.textBox_dll.Size = new System.Drawing.Size(282, 21);
            this.textBox_dll.TabIndex = 3;
            this.textBox_dll.Text = "KrkrExtract.dll";
            // 
            // checkBox_close
            // 
            this.checkBox_close.AutoSize = true;
            this.checkBox_close.Checked = true;
            this.checkBox_close.CheckState = System.Windows.Forms.CheckState.Checked;
            this.checkBox_close.Location = new System.Drawing.Point(178, 69);
            this.checkBox_close.Name = "checkBox_close";
            this.checkBox_close.Size = new System.Drawing.Size(144, 16);
            this.checkBox_close.TabIndex = 4;
            this.checkBox_close.Text = "Exit after injection";
            this.checkBox_close.UseVisualStyleBackColor = true;
            // 
            // checkBox_absolute
            // 
            this.checkBox_absolute.AutoSize = true;
            this.checkBox_absolute.Checked = true;
            this.checkBox_absolute.CheckState = System.Windows.Forms.CheckState.Checked;
            this.checkBox_absolute.Location = new System.Drawing.Point(46, 69);
            this.checkBox_absolute.Name = "checkBox_absolute";
            this.checkBox_absolute.Size = new System.Drawing.Size(126, 16);
            this.checkBox_absolute.TabIndex = 5;
            this.checkBox_absolute.Text = "Absolute DLL Path";
            this.checkBox_absolute.UseVisualStyleBackColor = true;
            // 
            // MainForm
            // 
            this.AllowDrop = true;
            this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.ClientSize = new System.Drawing.Size(358, 150);
            this.Controls.Add(this.groupBox1);
            this.Controls.Add(this.label1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Margin = new System.Windows.Forms.Padding(1);
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "MainForm";
            this.Text = "KrkrExtract";
            this.DragDrop += new System.Windows.Forms.DragEventHandler(this.KrkrExtract_DragDrop);
            this.DragEnter += new System.Windows.Forms.DragEventHandler(this.KrkrExtract_DragEnter);
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.ComboBox comboBox_mode;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.TextBox textBox_dll;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.CheckBox checkBox_close;
        private System.Windows.Forms.CheckBox checkBox_absolute;
    }
}

