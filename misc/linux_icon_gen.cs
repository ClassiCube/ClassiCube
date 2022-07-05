using System.Drawing.Imaging;
using System.Drawing;
using System.IO;

namespace test {
	public static class Program {
		
		const string src = "CCicon.ico";
		const string dst = "CCicon.c";
		
		static void DumpIcon(StreamWriter sw, int width, int height) {
			sw.WriteLine(width + "," + height + ",");
			
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
		}
		
		public static void Main(string[] args) {
			using (StreamWriter sw = new StreamWriter(dst)) {
				sw.WriteLine("const unsigned long CCIcon_Data[] = {");
				DumpIcon(sw, 64, 64);
				DumpIcon(sw, 32, 32);
				DumpIcon(sw, 16, 16);
				sw.WriteLine("};");
				sw.WriteLine("const int CCIcon_Size = sizeof(CCIcon_Data) / sizeof(unsigned long);");
			}
		}
	}
}
