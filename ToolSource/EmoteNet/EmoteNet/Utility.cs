using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Threading.Tasks;
using System.Windows;
using System.Linq;
using System.Windows.Media;
using System.Windows.Media.Imaging;

namespace EmoteNet
{

    public class Entry
    {
        public virtual string Name { get; set; }
        public virtual string Type { get; set; }
        public long Offset { get; set; }
        public uint Size { get; set; }

        public Entry()
        {
            Type = "";
            Offset = -1;
        }

        /// <summary>
        /// Check whether entry lies within specified file bound.
        /// </summary>
        public bool CheckPlacement(long max_offset)
        {
            return Offset < max_offset && Size <= max_offset && Offset <= max_offset - Size;
        }
    }


    public class ImageMetaData
    {
        public uint Width { get; set; }
        public uint Height { get; set; }
        public int OffsetX { get; set; }
        public int OffsetY { get; set; }
        public int BPP { get; set; }
        public string FileName { get; set; }
    }

    public static class Encodings
    {
        public static readonly Encoding cp932 = Encoding.GetEncoding(932);

        public static Encoding WithFatalFallback(this Encoding enc)
        {
            var encoding = enc.Clone() as Encoding;
            encoding.EncoderFallback = EncoderFallback.ExceptionFallback;
            return encoding;
        }
    }

    public static class Binary
    {
        public static uint BigEndian(uint u)
        {
            return (u & 0xff) << 24 | (u & 0xff00) << 8 | (u & 0xff0000) >> 8 | (u & 0xff000000) >> 24;
        }
        public static int BigEndian(int i)
        {
            return (int)BigEndian((uint)i);
        }
        public static ushort BigEndian(ushort u)
        {
            return (ushort)((u & 0xff) << 8 | (u & 0xff00) >> 8);
        }
        public static short BigEndian(short i)
        {
            return (short)BigEndian((ushort)i);
        }
        public static ulong BigEndian(ulong u)
        {
            return (ulong)BigEndian((uint)(u & 0xffffffff)) << 32
                 | (ulong)BigEndian((uint)(u >> 32));
        }
        public static long BigEndian(long i)
        {
            return (long)BigEndian((ulong)i);
        }

        public static bool AsciiEqual(byte[] name1, string name2)
        {
            return AsciiEqual(name1, 0, name2);
        }

        public static bool AsciiEqual(byte[] name1, int offset, string name2)
        {
            if (name1.Length - offset < name2.Length)
                return false;
            for (int i = 0; i < name2.Length; ++i)
                if ((char)name1[offset + i] != name2[i])
                    return false;
            return true;
        }

        public static void CopyOverlapped(byte[] data, int src, int dst, int count)
        {
            if (dst > src)
            {
                while (count > 0)
                {
                    int preceding = System.Math.Min(dst - src, count);
                    System.Buffer.BlockCopy(data, src, data, dst, preceding);
                    dst += preceding;
                    count -= preceding;
                }
            }
            else
            {
                System.Buffer.BlockCopy(data, src, data, dst, count);
            }
        }

        public static string GetCString(byte[] data, int index, int length_limit, Encoding enc)
        {
            int name_length = 0;
            while (name_length < length_limit && 0 != data[index + name_length])
                name_length++;
            return enc.GetString(data, index, name_length);
        }

        public static string GetCString(byte[] data, int index, int length_limit)
        {
            return GetCString(data, index, length_limit, Encodings.cp932);
        }

        public static uint RotR(uint v, int count)
        {
            count &= 0x1F;
            return v >> count | v << (32 - count);
        }

        public static uint RotL(uint v, int count)
        {
            count &= 0x1F;
            return v << count | v >> (32 - count);
        }

        public static ulong RotR(ulong v, int count)
        {
            count &= 0x3F;
            return v >> count | v << (64 - count);
        }

        public static ulong RotL(ulong v, int count)
        {
            count &= 0x3F;
            return v << count | v >> (64 - count);
        }

        public static byte RotByteR(byte v, int count)
        {
            count &= 7;
            return (byte)(v >> count | v << (8 - count));
        }

        public static byte RotByteL(byte v, int count)
        {
            count &= 7;
            return (byte)(v << count | v >> (8 - count));
        }
    }

    public static class BigEndian
    {
        public static ushort ToUInt16(byte[] value, int index)
        {
            return (ushort)(value[index] << 8 | value[index + 1]);
        }

        public static short ToInt16(byte[] value, int index)
        {
            return (short)(value[index] << 8 | value[index + 1]);
        }

        public static uint ToUInt32(byte[] value, int index)
        {
            return (uint)(value[index] << 24 | value[index + 1] << 16 | value[index + 2] << 8 | value[index + 3]);
        }

        public static int ToInt32(byte[] value, int index)
        {
            return (int)ToUInt32(value, index);
        }
    }

    public static class LittleEndian
    {
        public static ushort ToUInt16(byte[] value, int index)
        {
            return (ushort)(value[index] | value[index + 1] << 8);
        }

        public static short ToInt16(byte[] value, int index)
        {
            return (short)(value[index] | value[index + 1] << 8);
        }

        public static uint ToUInt32(byte[] value, int index)
        {
            return (uint)(value[index] | value[index + 1] << 8 | value[index + 2] << 16 | value[index + 3] << 24);
        }

        public static int ToInt32(byte[] value, int index)
        {
            return (int)ToUInt32(value, index);
        }

        public static ulong ToUInt64(byte[] value, int index)
        {
            return (ulong)ToUInt32(value, index) | ((ulong)ToUInt32(value, index + 4) << 32);
        }

        public static long ToInt64(byte[] value, int index)
        {
            return (long)ToUInt64(value, index);
        }

        public static void Pack(ushort value, byte[] buf, int index)
        {
            buf[index] = (byte)(value);
            buf[index + 1] = (byte)(value >> 8);
        }

        public static void Pack(uint value, byte[] buf, int index)
        {
            buf[index] = (byte)(value);
            buf[index + 1] = (byte)(value >> 8);
            buf[index + 2] = (byte)(value >> 16);
            buf[index + 3] = (byte)(value >> 24);
        }

        public static void Pack(ulong value, byte[] buf, int index)
        {
            Pack((uint)value, buf, index);
            Pack((uint)(value >> 32), buf, index + 4);
        }

        public static void Pack(short value, byte[] buf, int index)
        {
            Pack((ushort)value, buf, index);
        }

        public static void Pack(int value, byte[] buf, int index)
        {
            Pack((uint)value, buf, index);
        }

        public static void Pack(long value, byte[] buf, int index)
        {
            Pack((ulong)value, buf, index);
        }
    }


    public class ImageData
    {
        private BitmapSource m_bitmap;

        public BitmapSource Bitmap { get { return m_bitmap; } }
        public uint Width { get { return (uint)m_bitmap.PixelWidth; } }
        public uint Height { get { return (uint)m_bitmap.PixelHeight; } }
        public int OffsetX { get; set; }
        public int OffsetY { get; set; }
        public int BPP { get { return m_bitmap.Format.BitsPerPixel; } }

        public static double DefaultDpiX { get; set; }
        public static double DefaultDpiY { get; set; }

        static ImageData()
        {
            SetDefaultDpi(96, 96);
        }

        public static void SetDefaultDpi(double x, double y)
        {
            DefaultDpiX = x;
            DefaultDpiY = y;
        }

        public ImageData(BitmapSource data, ImageMetaData meta)
        {
            m_bitmap = data;
            OffsetX = meta.OffsetX;
            OffsetY = meta.OffsetY;
        }

        public ImageData(BitmapSource data, int x = 0, int y = 0)
        {
            m_bitmap = data;
            OffsetX = x;
            OffsetY = y;
        }

        public static ImageData Create(ImageMetaData info, PixelFormat format, BitmapPalette palette,
                                        byte[] pixel_data, int stride)
        {
            var bitmap = BitmapSource.Create((int)info.Width, (int)info.Height, DefaultDpiX, DefaultDpiY,
                                              format, palette, pixel_data, stride);
            bitmap.Freeze();
            return new ImageData(bitmap, info);
        }

        public static ImageData Create(ImageMetaData info, PixelFormat format, BitmapPalette palette,
                                        byte[] pixel_data)
        {
            return Create(info, format, palette, pixel_data, (int)info.Width * ((format.BitsPerPixel + 7) / 8));
        }

        public static ImageData CreateFlipped(ImageMetaData info, PixelFormat format, BitmapPalette palette,
                                        byte[] pixel_data, int stride)
        {
            var bitmap = BitmapSource.Create((int)info.Width, (int)info.Height, DefaultDpiX, DefaultDpiY,
                                              format, palette, pixel_data, stride);
            var flipped = new TransformedBitmap(bitmap, new ScaleTransform { ScaleY = -1 });
            flipped.Freeze();
            return new ImageData(flipped, info);
        }
    }



    public class PrefixStream : Stream
    {
        byte[] m_header;
        Stream m_stream;
        long m_position = 0;
        bool m_should_dispose;

        public PrefixStream(byte[] header, Stream main, bool leave_open = false)
        {
            m_header = header;
            m_stream = main;
            m_should_dispose = !leave_open;
        }

        public override bool CanRead { get { return m_stream.CanRead; } }
        public override bool CanSeek { get { return m_stream.CanSeek; } }
        public override bool CanWrite { get { return false; } }
        public override long Length { get { return m_stream.Length + m_header.Length; } }
        public override long Position
        {
            get { return m_position; }
            set
            {
                if (!m_stream.CanSeek)
                    throw new NotSupportedException("Underlying stream does not support Stream.Position property");
                m_position = Math.Max(value, 0);
                if (m_position > m_header.Length)
                {
                    long stream_pos = m_stream.Seek(m_position - m_header.Length, SeekOrigin.Begin);
                    m_position = m_header.Length + stream_pos;
                }
            }
        }

        public override void Flush()
        {
            m_stream.Flush();
        }

        public override long Seek(long offset, SeekOrigin origin)
        {
            if (SeekOrigin.Begin == origin)
                Position = offset;
            else if (SeekOrigin.Current == origin)
                Position = m_position + offset;
            else
                Position = Length + offset;

            return m_position;
        }

        public override int Read(byte[] buffer, int offset, int count)
        {
            int read = 0;
            if (m_position < m_header.Length)
            {
                int header_count = Math.Min(count, m_header.Length - (int)m_position);
                Buffer.BlockCopy(m_header, (int)m_position, buffer, offset, header_count);
                m_position += header_count;
                read += header_count;
                offset += header_count;
                count -= header_count;
            }
            if (count > 0)
            {
                if (m_header.Length == m_position && m_stream.CanSeek)
                    m_stream.Position = 0;
                int stream_read = m_stream.Read(buffer, offset, count);
                m_position += stream_read;
                read += stream_read;
            }
            return read;
        }

        public override int ReadByte()
        {
            if (m_position < m_header.Length)
                return m_header[m_position++];
            if (m_position == m_header.Length && m_stream.CanSeek)
                m_stream.Position = 0;
            int b = m_stream.ReadByte();
            if (-1 != b)
                m_position++;
            return b;
        }

        public override void SetLength(long length)
        {
            throw new NotSupportedException("PrefixStream.SetLength method is not supported");
        }

        public override void Write(byte[] buffer, int offset, int count)
        {
            throw new NotSupportedException("PrefixStream.Write method is not supported");
        }

        public override void WriteByte(byte value)
        {
            throw new NotSupportedException("PrefixStream.WriteByte method is not supported");
        }

        bool disposed = false;
        protected override void Dispose(bool disposing)
        {
            if (!disposed)
            {
                if (m_should_dispose && disposing)
                    m_stream.Dispose();
                disposed = true;
                base.Dispose(disposing);
            }
        }
    }

    public class ResReader : System.IO.BinaryReader
    {
        public ResReader(Stream stream)
            : base(stream, Encoding.ASCII, true)
        {
        }
    }

}
