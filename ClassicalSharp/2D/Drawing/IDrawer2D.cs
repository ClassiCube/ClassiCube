// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Collections.Generic;
using System.Drawing;
#if !LAUNCHER
using ClassicalSharp.GraphicsAPI;
#endif
#if ANDROID
using Android.Graphics;
#endif

namespace ClassicalSharp {
	
	/// <summary> Contains arguments for measuring or drawing text. </summary>
	public struct DrawTextArgs {		
		public string Text;
		public Font Font;
		public bool UseShadow, SkipPartsCheck;
		
		public DrawTextArgs(string text, Font font, bool useShadow) {
			Text = text;
			Font = font;
			UseShadow = useShadow;
			SkipPartsCheck = false;
		}		
	}

	/// <summary> Class responsible for performing drawing operations on bitmaps
	/// and for converting bitmaps into graphics api textures. </summary>
	/// <remarks> Uses GDI+ on Windows, uses Cairo on Mono. </remarks>
	public abstract partial class IDrawer2D : IDisposable {
#if !LAUNCHER		
		protected IGraphicsApi graphics;
#endif
		public const float Offset = 1.3f;
		
		/// <summary> Whether chat text should be drawn and measuring using the currently bitmapped font, 
		/// false uses the font supplied as the DrawTextArgs argument supplied to the function. </summary>
		public bool UseBitmappedChat;
		
		/// <summary> Whether the shadows behind text (that uses shadows) is fully black. </summary>
		public bool BlackTextShadows;
		
		/// <summary> Sets the underlying bitmap that drawing operations will be performed on. </summary>
		public abstract void SetBitmap(Bitmap bmp);
		
		/// <summary> Draws a 2D flat rectangle of the specified dimensions at the
		/// specified coordinates in the currently bound bitmap. </summary>
		public abstract void DrawRect(FastColour colour, int x, int y, int width, int height);
		
		/// <summary> Draws the outline of a 2D flat rectangle of the specified dimensions
		/// at the specified coordinates in the currently bound bitmap. </summary>
		public abstract void DrawRectBounds(FastColour colour, int lineWidth, int x, int y, int width, int height);
		
		/// <summary> Clears the entire given area to the specified colour. </summary>
		public abstract void Clear(FastColour colour, int x, int y, int width, int height);
		
		/// <summary> Disposes of any resources used by this class that are associated with the underlying bitmap. </summary>
		public abstract void Dispose();
		
		public abstract Bitmap ConvertTo32Bpp(Bitmap src);
		
		public void ConvertTo32Bpp(ref Bitmap src) {
			Bitmap newBmp = ConvertTo32Bpp(src);
			src.Dispose();
			src = newBmp;
		}

		protected abstract void DrawSysText(ref DrawTextArgs args, int x, int y);
		
		/// <summary> Draws a string using the specified arguments and fonts at the
		/// specified coordinates in the currently bound bitmap, clipping if necessary. </summary>
		public abstract void DrawClippedText(ref DrawTextArgs args, int x, int y, float maxWidth, float maxHeight);

		protected abstract void DrawBitmappedText(ref DrawTextArgs args, int x, int y);
		
		public void DrawText(ref DrawTextArgs args, int x, int y) {
			if (!UseBitmappedChat)
				DrawSysText(ref args, x, y);
			else
				DrawBitmappedText(ref args, x, y);
		}
		
		protected abstract Size MeasureSysSize(ref DrawTextArgs args);
		
		public Size MeasureSize(ref DrawTextArgs args) {
			return !UseBitmappedChat ? MeasureSysSize(ref args) : MeasureBitmappedSize(ref args);
		}
		
		public int FontHeight(Font font, bool useShadow) {
			DrawTextArgs args = new DrawTextArgs("I", font, useShadow);
			return MeasureSize(ref args).Height;
		}
		
#if !LAUNCHER
		public Texture MakeTextTexture(ref DrawTextArgs args, int windowX, int windowY) {
			Size size = MeasureSize(ref args);
			if (size == Size.Empty)
				return new Texture(-1, windowX, windowY, 0, 0, 1, 1);
			
			using (Bitmap bmp = CreatePow2Bitmap(size)) {
				SetBitmap(bmp);
				args.SkipPartsCheck = true;
				DrawText(ref args, 0, 0);
				
				Dispose();
				return Make2DTexture(bmp, size, windowX, windowY);
			}
		}
#endif
		
		
		/// <summary> Disposes of all native resources used by this class. </summary>
		/// <remarks> You will no longer be able to perform measuring or drawing calls after this. </remarks>
		public abstract void DisposeInstance();
		
#if !LAUNCHER
		public Texture Make2DTexture(Bitmap bmp, Size used, int windowX, int windowY) {			
			int texId = graphics.CreateTexture(bmp, false, false);
			return new Texture(texId, windowX, windowY, used.Width, used.Height,
			                   (float)used.Width / bmp.Width, (float)used.Height / bmp.Height);
		}
#endif
		
		/// <summary> Creates a power-of-2 sized bitmap larger or equal to to the given size. </summary>
		public static Bitmap CreatePow2Bitmap(Size size) {
			return Platform.CreateBmp(Utils.NextPowerOf2(size.Width), Utils.NextPowerOf2(size.Height));
		}
		
		public static FastColour[] Cols = new FastColour[256];
		
		public IDrawer2D() { InitColours(); }
		
		public void InitColours() {
			for (int i = 0; i < Cols.Length; i++)
				Cols[i] = default(FastColour);
			
			for (int i = 0; i <= 9; i++)
				Cols['0' + i] = FastColour.GetHexEncodedCol(i, 191, 64);
			for (int i = 10; i <= 15; i++) {
				Cols['a' + i - 10] = FastColour.GetHexEncodedCol(i, 191, 64);
				Cols['A' + i - 10] = Cols['a' + i - 10];
			}
		}
		
		protected List<TextPart> parts = new List<TextPart>(64);
		protected struct TextPart {
			public string Text;
			public FastColour Col;
			
			public TextPart(string text, FastColour col) {
				Text = text;
				Col = col;
			}
		}
		
		protected void GetTextParts(string value) {
			parts.Clear();
			if (EmptyText(value)) {
			} else if (value.IndexOf('&') == -1) {
				parts.Add(new TextPart(value, Cols['f']));
			} else {
				SplitText(value);
			}
		}
		
		/// <summary> Splits the input string by recognised colour codes. (e.g &amp;f) </summary>
		protected void SplitText(string value) {
			char code = 'f';
			for (int i = 0; i < value.Length; ) {
				int length = 0, start = i;
				for (; i < value.Length; i++) {
					if (value[i] == '&' && ValidColCode(value, i + 1)) break;
					length++;
				}
				
				if (length > 0) {
					string part = value.Substring(start, length);
					parts.Add(new TextPart(part, Cols[code]));
				}
				
				i += 2; // skip over colour code
				if (i <= value.Length) code = value[i - 1];
			}
		}
	
		/// <summary> Returns whenever the given character is a valid colour code. </summary>
		public static bool ValidColCode(string text, int i) {
			if (i >= text.Length) return false;
			char c = text[i];
			return c <= '\xFF' && Cols[c].A > 0;
		}
		
		/// <summary> Returns whenever the given character is a valid colour code. </summary>
		public static bool ValidColCode(char c) {
			return c <= '\xFF' && Cols[c].A > 0;
		}
		
		public static bool EmptyText(string text) {
			if (text == null || text.Length == 0) return true;
			
			for (int i = 0; i < text.Length; i++) {
				if (text[i] != '&') return false;
				if (!ValidColCode(text, i + 1)) return false;
				i++; // skip colour code
			}
			return true;
		}

#if !LAUNCHER		
		/// <summary> Returns the last valid colour code in the given input, 
		/// or \0 if no valid colour code was found. </summary>
		public static char LastCol(string text, int start) {
			if (start >= text.Length)
				start = text.Length - 1;
			
			for (int i = start; i >= 0; i--) {
				if (text[i] != '&') continue;
				if (ValidColCode(text, i + 1)) return text[i + 1];
			}
			return '\0';
		}
		
		public static bool IsWhiteCol(char c) {
			return c == '\0' || c == 'f' || c == 'F';
		}
		
		public void ReducePadding(ref Texture tex, int point, int scale) {
			if (!UseBitmappedChat) return;
			point = AdjTextSize(point);
			
			int padding = (tex.Height - point) / scale;
			float vAdj = (float)padding / Utils.NextPowerOf2(tex.Height);
			tex.V1 += vAdj; tex.V2 -= vAdj;
			tex.Height -= (ushort)(padding * 2);
		}

		public void ReducePadding(ref int height, int point, int scale) {
			if (!UseBitmappedChat) return;
			point = AdjTextSize(point);
			
			int padding = (height - point) / scale;
			height -= padding * 2;
		}
		#endif
	}
}
