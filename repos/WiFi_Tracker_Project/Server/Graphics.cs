using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;

namespace Panopticon
{
    public class Graphics
    {
        public class Bitmap
        {
            internal int stride;
            internal byte[] pixels;
            internal int h;
            internal int w;
            public Bitmap(int HPix, int WPix, byte RbColor, byte GbColor, byte BbColor)
            {
                stride = 3 * WPix + (WPix % 4);
                pixels = new byte[HPix * stride];
                h = HPix;
                w = WPix;
                int x, y = 0;
                for (; y < h; y++)
                {
                    for (x = 0; x < w; x++)
                    {
                        SetPixel(x, y, RbColor, GbColor, BbColor, 1);
                    }
                }
            }
            public void SetPixel(int x, int y, byte R, byte G, byte B, float alpha)
            {

                int id = y * stride + x * 3;
                if (id >= pixels.Length || id < 0 || x < 0)
                    return;
                pixels[id] = (byte)((pixels[id] - R) * (1 - alpha) + R);
                pixels[id + 1] = (byte)((pixels[id + 1] - G) * (1 - alpha) + G);
                pixels[id + 2] = (byte)((pixels[id + 2] - B) * (1 - alpha) + B);

            }
            
            public BitmapSource GetBitmap()
            {
                BitmapSource bs = BitmapSource.Create(w, h, 300, 300, PixelFormats.Rgb24, null, pixels, stride);
                return bs;
            }
        }
        internal static BitmapSource createheatmap(int[,] heatmap)
        {
            int xlen = (int)(20 * (heatmap.GetLength(0)-1));
            int ylen = (int)(20 * (heatmap.GetLength(1)-1));
            Bitmap bitmap = new Bitmap(ylen, xlen, 255, 255, 255);
            double max = 0;
            double min = double.MaxValue;
            for (int x = 0; x < heatmap.GetLength(0); x++)
            {
                for (int y = 0; y < heatmap.GetLength(1); y++)
                {
                    if (heatmap[x, y] > max)
                        max = heatmap[x, y];
                    if (heatmap[x, y] < min)
                        min = heatmap[x, y];
                }
            }
            for (int x = 0; x < xlen && max>0; x++)
            {
                for (int y = 0; y < ylen; y++)
                {
                    int ix = (int)(x / 20);
                    int iy = (int)(y / 20);
                    double fx = (double)x / 20.0;
                    double fy = (double)y / 20.0;
                    double val = (1 - (fx - ix) + 1 - (fy - iy)) * heatmap[ix, iy] +
                        ((fx - ix) + 1 - (fy - iy)) * heatmap[ix + 1, iy] +
                        (1 - (fx - ix) + (fy - iy)) * heatmap[ix, iy + 1] +
                        ((fx - ix) + (fy - iy)) * heatmap[ix + 1, iy + 1];
                    val /= 4;
                    if (max > min)
                        val = Math.Pow((val - min) / (max - min), 0.6);
                    else
                        val = 1;
                    if (val > 1)
                        val = 1;
                    else if (val < 0)
                        val = 0; 
                    byte G = (byte)(val < 0.25 ? 255 : 255 * 4 * (1 - val) / 3);
                    byte B = (byte)(val < 0.25 ? 255 * 4 * (0.25 - val) : 0);
                    bitmap.SetPixel(x, y, (byte)255, G, B, 1);
                }
            }
            return bitmap.GetBitmap();
        }

        internal static void drawHistogram(Canvas cv, IEnumerable<Double> data, Func<double,int,object,String> tooltipper, object tooltipdata, MouseButtonEventHandler onmousedown, Brush[] strokecolors, Brush[] fillcolors, int initialbrush, Func<double,int,object,object> tagger, object tagdata,bool skipfirst=false)
        {
            cv.Children.Clear();
            double max = 0.001;
            int count = 0;
            foreach (double d in data)
            {
                count++;
                if (d > max)
                    max = d;
            }
            int i = 0;
            if (skipfirst)
                count--;
            double width = Math.Min(20,cv.Width / count);
            double margin = (cv.Width - count*width)/2;
            foreach (double d in data)
            {
                Rectangle re = new Rectangle();
                re.Height = d / max * cv.Height;
                re.Width = (i==0&&skipfirst)?0:width;
                re.Fill = fillcolors[(initialbrush + i) % fillcolors.Length];
                re.Stroke = strokecolors[(i+ initialbrush) % strokecolors.Length];
                re.VerticalAlignment = VerticalAlignment.Bottom;
                Canvas.SetLeft(re, margin);
                Canvas.SetBottom(re, 0);
                cv.Children.Add(re);
                re.ToolTip = tooltipper(d, i, tooltipdata);
                if (onmousedown != null)
                {
                    re.MouseDown += onmousedown;
                    re.Cursor = Cursors.Hand;
                }
                if (tagger != null)
                    re.Tag = tagger(d, i, tagdata);
                margin += re.Width;
                i++;
            }
        }
        
        public static class FancyColorCreator
        {
            /* This code is not owned by the authors of the application nor a work of theirs, is an edited version of a code found years ago on stack overflow or something like that, author unknown */
            const double goldenratioconiugate = 360 * 0.618033988749895;

            private static void HsvToRgb(double h, double S, double V, out int r, out int g, out int b)
            {
                double H = h;
                while (H < 0) { H += 360; };
                while (H >= 360) { H -= 360; };
                double R, G, B;
                if (V <= 0)
                { R = G = B = 0; }
                else if (S <= 0)
                {
                    R = G = B = V;
                }
                else
                {
                    double hf = H / 60.0;
                    int i = (int)Math.Floor(hf);
                    double f = hf - i;
                    double pv = V * (1 - S);
                    double qv = V * (1 - S * f);
                    double tv = V * (1 - S * (1 - f));
                    switch (i)
                    {

                        // Red is the dominant color

                        case 0:
                            R = V;
                            G = tv;
                            B = pv;
                            break;

                        // Green is the dominant color

                        case 1:
                            R = qv;
                            G = V;
                            B = pv;
                            break;
                        case 2:
                            R = pv;
                            G = V;
                            B = tv;
                            break;

                        // Blue is the dominant color

                        case 3:
                            R = pv;
                            G = qv;
                            B = V;
                            break;
                        case 4:
                            R = tv;
                            G = pv;
                            B = V;
                            break;

                        // Red is the dominant color

                        case 5:
                            R = V;
                            G = pv;
                            B = qv;
                            break;

                        // Just in case we overshoot on our math by a little, we put these here. Since its a switch it won't slow us down at all to put these here.

                        case 6:
                            R = V;
                            G = tv;
                            B = pv;
                            break;
                        case -1:
                            R = V;
                            G = pv;
                            B = qv;
                            break;

                        // The color is not defined, we should throw an error.

                        default:
                            //LFATAL("i Value error in Pixel conversion, Value is %d", i);
                            R = G = B = V; // Just pretend its black/white
                            break;
                    }
                }
                r = clip((int)(R * 255.0));
                g = clip((int)(G * 255.0));
                b = clip((int)(B * 255.0));
            }

            /// <summary>
            /// Clip a value to 0-255
            /// </summary>
            private static int clip(int i)
            {
                if (i < 0) return 0;
                if (i > 255) return 255;
                return i;
            }
            public static Brush randomBrush(int seed)
            {
                int r;
                int g;
                int b;
                double randomhue = (seed % 3600) / 10;
                randomhue += goldenratioconiugate;
                HsvToRgb(randomhue, 0.6, 0.85, out r, out g, out b);
                Color newCol = Color.FromRgb((byte)r, (byte)g, (byte)b);
                Brush brush = new SolidColorBrush(newCol);
                return brush;
            }
        }
    }
}
