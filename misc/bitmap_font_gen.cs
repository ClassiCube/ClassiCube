void Main()
{
	Console.WriteLine("static cc_uint8 font_bitmap[][8] = {");
	using (Bitmap bmp = new Bitmap(@"C:\classicube-dev\default.png"))
	{
		for (int CY = 0; CY < 16; CY++)
			for (int CX = 0; CX < 16; CX++)
				DecodeTile(bmp, CX, CY);
	}
	Console.WriteLine("}");
}

static void DecodeTile(Bitmap bmp, int cx, int cy) {
	int c = (cy << 4) | cx;
	if (c <= 32 || c >= 127) return;
	
	int X = cx * 8, Y = cy * 8;
	
	Console.Write("\t{ ");
	
	for (int y = Y; y < Y + 8; y++) {
		uint mask = 0;
		int shift = 0;
		
		for (int x = X; x < X + 8; x++, shift++) {
			Color P = bmp.GetPixel(x, y);
			
			if (P.A == 0) {
				mask |= 0u << shift;
			} else if (P.R == 255 && P.G == 255 && P.B == 255 && P.A == 255) {
				mask |= 1u << shift;
			} else { 
				throw new InvalidOperationException("unsupported colour" + P); 
			}
		}
		
		string suffix = y == Y + 7 ? " " : ",";
		Console.Write("0x" + mask.ToString("X2") + suffix);
	}
	Console.WriteLine("}, /* " + (char)c + " */");
}
