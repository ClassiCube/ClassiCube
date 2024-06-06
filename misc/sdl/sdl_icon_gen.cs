using System.Drawing.Imaging;
using System.Drawing;
using System.IO;

namespace test 
{
	public static class Program 
	{
		const string src = "CCicon.ico";
		const string dst = "CCIcon_SDL.h";
		
		static void DumpIcon(StreamWriter sw, int width, int height) {
			using (Icon icon = new Icon(src, width, height)) {
				using (Bitmap bmp = icon.ToBitmap()) {
					for (int y = 0; y < bmp.Height; y++) {
						for (int x = 0; x < bmp.Width; x++) {
							Color c = bmp.GetPixel(x, y);
							int p = (c.A << 24) | (c.R << 16) | (c.G << 8) | c.B;
							sw.Write("0x" + ((uint)p).ToString("X8") + ",");
						}
						sw.WriteLine();
					}
				}
			}
			sw.WriteLine("static const int CCIcon_Width  = {0};", width);
			sw.WriteLine("static const int CCIcon_Height = {0};", height);
			sw.WriteLine("static const int CCIcon_Pitch  = {0};", width * 4);
		}
		
		public static void Main(string[] args) {
			using (StreamWriter sw = new StreamWriter(dst)) {
				sw.WriteLine("/* Generated using misc/sdl/sdl_icon_gen.cs */");
				sw.WriteLine("");
				sw.WriteLine("static const unsigned CCIcon_Data[] = {");
				DumpIcon(sw, 64, 64);
				sw.WriteLine("};");
			}
		}
	}
}
