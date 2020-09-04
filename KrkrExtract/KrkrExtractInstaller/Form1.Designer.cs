namespace KrkrExtractInstaller
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
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.InstallUser = new System.Windows.Forms.Button();
            this.UninstallUser = new System.Windows.Forms.Button();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.UninstallAll = new System.Windows.Forms.Button();
            this.InstallAll = new System.Windows.Forms.Button();
            this.groupBox1.SuspendLayout();
            this.groupBox2.SuspendLayout();
            this.SuspendLayout();
            // 
            // label1
            // 
            this.label1.Font = new System.Drawing.Font("Microsoft Sans Serif", 7.8F);
            this.label1.Location = new System.Drawing.Point(23, 26);
            this.label1.Margin = new System.Windows.Forms.Padding(5, 0, 5, 0);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(968, 393);
            this.label1.TabIndex = 5;
            this.label1.Text = resources.GetString("label1.Text");
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.UninstallUser);
            this.groupBox1.Controls.Add(this.InstallUser);
            this.groupBox1.Location = new System.Drawing.Point(13, 444);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(978, 147);
            this.groupBox1.TabIndex = 6;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Install for current user";
            // 
            // InstallUser
            // 
            this.InstallUser.Location = new System.Drawing.Point(46, 52);
            this.InstallUser.Name = "InstallUser";
            this.InstallUser.Size = new System.Drawing.Size(407, 70);
            this.InstallUser.TabIndex = 0;
            this.InstallUser.Text = "Install for current user";
            this.InstallUser.UseVisualStyleBackColor = true;
            this.InstallUser.Click += new System.EventHandler(this.InstallUser_Click);
            // 
            // UninstallUser
            // 
            this.UninstallUser.Location = new System.Drawing.Point(529, 52);
            this.UninstallUser.Name = "UninstallUser";
            this.UninstallUser.Size = new System.Drawing.Size(407, 70);
            this.UninstallUser.TabIndex = 1;
            this.UninstallUser.Text = "Uninstall for current user";
            this.UninstallUser.UseVisualStyleBackColor = true;
            this.UninstallUser.Click += new System.EventHandler(this.UninstallUser_Click);
            // 
            // groupBox2
            // 
            this.groupBox2.Controls.Add(this.UninstallAll);
            this.groupBox2.Controls.Add(this.InstallAll);
            this.groupBox2.Location = new System.Drawing.Point(12, 608);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(978, 147);
            this.groupBox2.TabIndex = 7;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "Install for all users(requires administrator)";
            // 
            // UninstallAll
            // 
            this.UninstallAll.Location = new System.Drawing.Point(529, 52);
            this.UninstallAll.Name = "UninstallAll";
            this.UninstallAll.Size = new System.Drawing.Size(407, 70);
            this.UninstallAll.TabIndex = 1;
            this.UninstallAll.Text = "Uninstall for all users";
            this.UninstallAll.UseVisualStyleBackColor = true;
            this.UninstallAll.Click += new System.EventHandler(this.UninstallAll_Click);
            // 
            // InstallAll
            // 
            this.InstallAll.Location = new System.Drawing.Point(46, 52);
            this.InstallAll.Name = "InstallAll";
            this.InstallAll.Size = new System.Drawing.Size(407, 70);
            this.InstallAll.TabIndex = 0;
            this.InstallAll.Text = "Install for all users";
            this.InstallAll.UseVisualStyleBackColor = true;
            this.InstallAll.Click += new System.EventHandler(this.InstallAll_Click);
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(16F, 31F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(1008, 767);
            this.Controls.Add(this.groupBox2);
            this.Controls.Add(this.groupBox1);
            this.Controls.Add(this.label1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.Name = "Form1";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "KrkrExtract Installer";
            this.TopMost = true;
            this.Load += new System.EventHandler(this.Form1_Load);
            this.groupBox1.ResumeLayout(false);
            this.groupBox2.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.Button UninstallUser;
        private System.Windows.Forms.Button InstallUser;
        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.Button UninstallAll;
        private System.Windows.Forms.Button InstallAll;
    }
}

