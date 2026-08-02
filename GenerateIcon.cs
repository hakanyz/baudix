using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.IO;

class Program {
    static void Main() {
        int size = 256;
        Bitmap bmp = new Bitmap(size, size, PixelFormat.Format32bppArgb);
        using (Graphics g = Graphics.FromImage(bmp)) {
            g.SmoothingMode = SmoothingMode.AntiAlias;
            
            // Draw background (rounded rect approx)
            using (Brush b = new SolidBrush(Color.FromArgb(20, 24, 29))) {
                float rx = 24 * (256f/120f);
                g.FillRectangle(b, 0, 0, size, size); // simple square is fine for now
            }
            // Draw the lines
            using (Pen p = new Pen(Color.FromArgb(255, 179, 0), 12)) {
                p.StartCap = LineCap.Square;
                p.EndCap = LineCap.Square;
                p.LineJoin = LineJoin.Miter;
                
                float s = 2.1333f;
                PointF[] pts = {
                    new PointF(22*s, 45*s), new PointF(28*s, 45*s), new PointF(28*s, 75*s),
                    new PointF(41*s, 75*s), new PointF(41*s, 45*s), new PointF(47*s, 45*s),
                    new PointF(47*s, 75*s), new PointF(66*s, 75*s), new PointF(66*s, 45*s),
                    new PointF(79*s, 45*s), new PointF(79*s, 75*s), new PointF(85*s, 75*s),
                    new PointF(85*s, 45*s), new PointF(98*s, 45*s)
                };
                g.DrawLines(p, pts);
            }
        }
        
        using (MemoryStream pngStream = new MemoryStream()) {
            bmp.Save(pngStream, ImageFormat.Png);
            byte[] pngBytes = pngStream.ToArray();
            
            using (FileStream fs = new FileStream("resources/baudix_icon.ico", FileMode.Create)) {
                using (BinaryWriter bw = new BinaryWriter(fs)) {
                    bw.Write((short)0);
                    bw.Write((short)1);
                    bw.Write((short)1);
                    bw.Write((byte)0);
                    bw.Write((byte)0);
                    bw.Write((byte)0);
                    bw.Write((byte)0);
                    bw.Write((short)1);
                    bw.Write((short)32);
                    bw.Write((int)pngBytes.Length);
                    bw.Write((int)22);
                    bw.Write(pngBytes);
                }
            }
        }
    }
}
