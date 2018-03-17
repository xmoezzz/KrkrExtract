using System;
using System.IO;
using System.Collections.Generic;
using System.Windows;
using System.Windows.Forms;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Drawing;
using System.Windows.Media;
using System.Windows.Media.Imaging;


/*
 * Private Key
 * Emote Export "EmoteFilterTexture"
 * 
 * ULONG_PTR Size = GetOpSize32(EatLookupByName(hModule, "EmoteFilterTexture"));
 * 
 * 
63352E60 EmoteFilterTexture     55               push ebp
63352E61                        8BEC             mov ebp,esp
63352E63                        83E4 F8          and esp,FFFFFFF8
 * 
 * Find 2nd push imm
63352EA9                        68 50233C63      push offset 633C2350    Emote Key: refers to a string                          ; ASCII "192918854"
63352EAE                        894424 18        mov dword ptr [esp+18],eax
63352EB2                        894C24 50        mov dword ptr [esp+50],ecx
63352EB6                        E8 4C930500      call 633AC207    call atoi
63352EBB                        33FF             xor edi,edi
63352EBD                        83C4 04          add esp,4
63352EC0                        C78424 80000000  mov dword ptr [esp+80],offset 633C28B4
63352ECB                        C78424 84000000  mov dword ptr [esp+84],75BCD15
63352ED6                        C78424 88000000  mov dword ptr [esp+88],159A55E5
63352EE1                        C78424 8C000000  mov dword ptr [esp+8C],1F123BB5
63352EEC                        898424 90000000  mov dword ptr [esp+90],eax   Emote Key
63352EF3                        89BC24 94000000  mov dword ptr [esp+94],edi   0 
63352EFA                        89BC24 98000000  mov dword ptr [esp+98],edi   0
 
 */

namespace EmoteNet
{
    class Program
    {
        static void Main(string[] args)
        {
            if (args.Length != 1)
                System.Environment.Exit(0);

            string FileName = args[0];
            var FileStreamReader = new FileStream(FileName, FileMode.Open);
            var PsbReaderHandle = new PsbReader(FileStreamReader);
            if(PsbReaderHandle.Parse(PsbOpener.KnownKeys[0]))
            {
                List<Entry> TextureDir = PsbReaderHandle.GetTextures();
                foreach (TexEntry Entry in TextureDir)
                {
                    byte[] buffer = new byte[Entry.Size];
                    long FileCur = FileStreamReader.Position;

                    Console.WriteLine("Texture Name:{0}", Entry.Name);
                    FileStreamReader.Position = Entry.Offset;

                    PsbTextureFormat ImageBuilder = new PsbTextureFormat();
                    ImageMetaData metaData = ImageBuilder.ReadMetaData(Entry);
                    ImageData Graph = ImageBuilder.Read(FileStreamReader, metaData);

                    Bitmap bmp = BitmapSourceToBitmap(Graph.Bitmap);
                    bmp.LockBits(new Rectangle(System.Drawing.Point.Empty, bmp.Size), 
                        System.Drawing.Imaging.ImageLockMode.ReadOnly, 
                        System.Drawing.Imaging.PixelFormat.Format32bppPArgb);

                    string FileOutName = Entry.Name + ".bmp";
                    bmp.Save(FileOutName);
                    FileStreamReader.Position = FileCur;
                }
                Console.WriteLine("Emote Dumper: Extract OK!");
                Thread.Sleep(2000);

            }
            else
            {
                MessageBox.Show("Not Supported Version!!\nMaybe you need a new private key", "Emote Dumper .Net Version");
                System.Environment.Exit(0);
            }
        }

        static public System.Drawing.Bitmap BitmapSourceToBitmap(BitmapSource s)
        {
            System.Drawing.Bitmap bmp = new System.Drawing.Bitmap(s.PixelWidth, s.PixelHeight, System.Drawing.Imaging.PixelFormat.Format32bppPArgb);
            System.Drawing.Imaging.BitmapData data = bmp.LockBits(new System.Drawing.Rectangle(System.Drawing.Point.Empty, bmp.Size), System.Drawing.Imaging.ImageLockMode.WriteOnly, System.Drawing.Imaging.PixelFormat.Format32bppPArgb);
            s.CopyPixels(Int32Rect.Empty, data.Scan0, data.Height * data.Stride, data.Stride);
            bmp.UnlockBits(data);
            return bmp;
        }
    }
}
