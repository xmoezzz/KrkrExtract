using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Threading;

namespace KrkrExtract.UI.Legacy
{
    public class CustomCheckBox : CheckBox
    {
        public int m_NotificationID;
        public CustomCheckBox(int ID)
        {
            m_NotificationID = ID;
        }
    }

    public class CustomRadioButton : RadioButton
    {
        public int m_NotificationID;
        public CustomRadioButton(int ID)
        {
            m_NotificationID = ID;
        }
    }
    public class CustomPanel
    {
        private string m_TagName;
        private string m_Description;
        private string m_Name;
        private KrkrMain m_Owner;
        private bool m_AttachToOwner;
        private bool m_ModifyLock;
        private int  m_Handle;
        
        private KrkrUIPluginMode m_Mode;
        private Dictionary<int, Control> m_Controls;
        private GroupBox m_Group;

        private int m_CurrentTabIndex = 0;

        public CustomPanel(ref KrkrMain Owner, int Handle, KrkrUIPluginMode Mode, string TagName, string Description)
        {
            m_Mode = Mode;
            m_TagName = TagName;
            m_Description = Description;
            m_Owner = Owner;
            m_AttachToOwner = false;
            m_ModifyLock = false;
            m_Handle = Handle;

            m_Group = new GroupBox();
            m_Group.Location = new System.Drawing.Point(47, 49);
            m_Group.Size = new System.Drawing.Size(1311, 664);
            m_Group.Name = "groupBox_" + TagName;
            m_Group.TabIndex = 0;
            m_Group.TabStop = false;
            m_Group.Text = TagName + " Setting";
        }

        public string TagName
        {
            get { return m_TagName; }
        }

        public void Lockdown()
        {
            if (!m_AttachToOwner)
                return;

            foreach (Control c in m_Controls.Values)
                c.Enabled = false;
        }

        public void Enable()
        {
            if (!m_AttachToOwner)
                return;

            foreach (Control c in m_Controls.Values)
                c.Enabled = true;
        }

        public bool AddControl(int NotificationID, string Name, bool InitialChecked)
        {
            Monitor.Enter(m_Controls);

            if (m_Controls.ContainsKey(NotificationID))
                return false;

            Control c = null;
            if (m_Mode == KrkrUIPluginMode.UI_COMBINE_MODE)
                c = new CustomCheckBox(NotificationID);
            else
                c = new CustomRadioButton(NotificationID);

            if (InitialChecked)
            {
                if (m_Mode == KrkrUIPluginMode.UI_COMBINE_MODE)
                    ((CheckBox)c).Checked = true;
                else
                    ((RadioButton)c).Checked = true;
            }


            c.AutoSize = true;
            c.Name = Name + "_CustomControl";
            c.Size = new System.Drawing.Size(100, 36);
            c.TabIndex = m_CurrentTabIndex;
            c.TabStop = true;
            c.Text = Name;
            if (m_Mode == KrkrUIPluginMode.UI_COMBINE_MODE)
            {
                ((CheckBox)c).UseVisualStyleBackColor = true;
                ((CheckBox)c).CheckedChanged += new System.EventHandler(this.NotifyCheckStatusChanged);
            }
            else
            {
                ((RadioButton)c).UseVisualStyleBackColor = true;
                ((RadioButton)c).CheckedChanged += new System.EventHandler(this.NotifyCheckStatusChanged);
            }

            m_Controls.Add(NotificationID, c);
            Monitor.Exit(m_Controls);

            return true;
        }


        private void NotifyCheckStatusChanged(object sender, EventArgs e)
        {
            if (sender.GetType() == typeof(CustomCheckBox))
            {
                if (((CustomCheckBox)sender).Checked)
                    m_Owner.NotifyCustomStatusChanged(m_Handle, ((CustomCheckBox)sender).m_NotificationID, true);
                else
                    m_Owner.NotifyCustomStatusChanged(m_Handle, ((CustomCheckBox)sender).m_NotificationID, false);
            }
            else if (sender.GetType() == typeof(CustomRadioButton))
            {
                if (((CustomRadioButton)sender).Checked)
                    m_Owner.NotifyCustomStatusChanged(m_Handle, ((CustomRadioButton)sender).m_NotificationID, true);
                else
                    m_Owner.NotifyCustomStatusChanged(m_Handle, ((CustomRadioButton)sender).m_NotificationID, false);
            }
        }

        public bool AttachToOwner()
        {
            if (m_AttachToOwner)
                return false;

            return true;
        }
        
    }
}
