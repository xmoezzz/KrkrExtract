using System;

namespace KeContextMenuHandler
{
    internal struct MenuItem
    {
        internal IntPtr Bitmap;
        internal string Commands;
        internal bool? ShowInMainMenu;
        internal string Text;
        internal bool Enabled;

        internal MenuItem(string text, bool enabled, bool? showInMainMenu, IntPtr bitmap, string commands)
        {
            ShowInMainMenu = showInMainMenu;
            Text = text;
            Enabled = enabled;
            Bitmap = bitmap;
            Commands = commands;
        }
    }
}