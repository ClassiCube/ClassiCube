// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
#if !ANDROID
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Text;
#if !LAUNCHER		
using ClassicalSharp.GraphicsAPI;
#endif

namespace ClassicalSharp {

	public sealed class GdiPlusDrawer2D : IDrawer2D {

		struct CachedBrush { public int ARGB; public SolidBrush Brush; }
		List<CachedBrush> brushes = new List<CachedBrush>(16);
		Graphics g, measuringGraphics;
		Bitmap curBmp, measuringBmp;
		StringFormat format;

#if !LAUNCHER		
		public GdiPlusDrawer2D(IGraphicsApi graphics) {
			this.graphics = graphics;
#else
		public GdiPlusDrawer2D() {
#endif
			format = StringFormat.GenericTypographic;
			format.FormatFlags |= StringFormatFlags.MeasureTrailingSpaces;
			format.Trimming = StringTrimming.None;
			//format.FormatFlags |= StringFormatFlags.NoWrap;
			//format.FormatFlags |= StringFormatFlags.NoClip;
			
			measuringBmp = new Bitmap(1, 1);
			measuringGraphics = Graphics.FromImage(measuringBmp);
			measuringGraphics.TextRenderingHint = TextRenderingHint.AntiAlias;
		}
		
		public override void SetBitmap(Bitmap bmp) {
			if (g != null) {
				throw new InvalidOperationException("Previous IDrawer2D.SetBitmap() call was not properly disposed");
			}
			
			g = Graphics.FromImage(bmp);
			g.TextRenderingHint = TextRenderingHint.AntiAlias;
			g.SmoothingMode = SmoothingMode.HighQuality;
			curBmp = bmp;
		}
		
		public override void DrawRect(FastColour colour, int x, int y, int width, int height) {
			Brush brush = GetOrCreateBrush(colour);
			g.FillRectangle(brush, x, y, width, height);
		}
		
		public override void DrawRectBounds(FastColour colour, int lineWidth, int x, int y, int width, int height) {
			using (Pen pen = new Pen(colour, lineWidth)) {
				pen.Alignment = PenAlignment.Inset;
				g.DrawRectangle(pen, x, y, width, height);
			}
		}
		
		public override void Clear(FastColour colour, int x, int y, int width, int height) {
			g.SmoothingMode = SmoothingMode.None;
			Brush brush = GetOrCreateBrush(colour);
			g.FillRectangle(brush, x, y, width, height);
			g.SmoothingMode = SmoothingMode.HighQuality;
		}
		
		public override void Dispose() {
			g.Dispose();
			g = null;
		}

		public override Bitmap ConvertTo32Bpp(Bitmap src) {
			Bitmap bmp = new Bitmap(src.Width, src.Height);
			using (Graphics g = Graphics.FromImage(bmp)) {
				g.InterpolationMode = InterpolationMode.NearestNeighbor;
				g.DrawImage(src, 0, 0, src.Width, src.Height);
			}
			return bmp;
		}
		
		public override void DisposeInstance() {
			for (int i = 0; i < brushes.Count; i++) {
				brushes[i].Brush.Dispose();
			}
			
			DisposeText();
			DisposeBitmappedText();
		}
		
		SolidBrush GetOrCreateBrush(FastColour col) {
			int argb = col.ToArgb();
			for (int i = 0; i < brushes.Count; i++) {
				if (brushes[i].ARGB == argb) return brushes[i].Brush;
			}
			
			CachedBrush b; b.ARGB = argb; b.Brush = new SolidBrush(col);
			brushes.Add(b);
			return b.Brush;
		}
		
		protected override void DrawSysText(ref DrawTextArgs args, int x, int y) {
			if (!args.SkipPartsCheck)
				GetTextParts(args.Text);
			
			float textX = x;
			Brush backBrush = GetOrCreateBrush(FastColour.Black);
			for (int i = 0; i < parts.Count; i++) {
				TextPart part = parts[i];
				Brush foreBrush = GetOrCreateBrush(part.Col);
				if (args.UseShadow)
					g.DrawString(part.Text, args.Font, backBrush, textX + Offset, y + Offset, format);
				
				g.DrawString(part.Text, args.Font, foreBrush, textX, y, format);
				textX += g.MeasureString(part.Text, args.Font, Int32.MaxValue, format).Width;
			}
		}
		
		public override void DrawClippedText(ref DrawTextArgs args, int x, int y, float maxWidth, float maxHeight) {
			if (!args.SkipPartsCheck)
				GetTextParts(args.Text);
			
			Brush shadowBrush = GetOrCreateBrush(FastColour.Black);
			StringFormatFlags flags = format.FormatFlags;
			format.FormatFlags |= StringFormatFlags.NoWrap;
			format.Trimming = StringTrimming.EllipsisCharacter;
			float textX = x;
			
			for (int i = 0; i < parts.Count; i++) {
				TextPart part = parts[i];
				Brush textBrush = GetOrCreateBrush(part.Col);
				RectangleF rect = new RectangleF(textX + Offset, y + Offset, maxWidth, maxHeight);
				if (args.UseShadow)
					g.DrawString(part.Text, args.Font, shadowBrush, rect, format);
				
				rect = new RectangleF(textX, y, maxWidth, maxHeight);
				g.DrawString(part.Text, args.Font, textBrush, rect, format);
				textX += g.MeasureString(part.Text, args.Font, Int32.MaxValue, format).Width;
			}
			format.Trimming = StringTrimming.None;
			format.FormatFlags = flags;
		}
		
		FastBitmap bitmapWrapper = new FastBitmap();
		protected override void DrawBitmappedText(ref DrawTextArgs args, int x, int y) {
			using (bitmapWrapper) {
				bitmapWrapper.SetData(curBmp, true, false);
				DrawBitmapTextImpl(bitmapWrapper, ref args, x, y);
			}
		}
		
		protected override Size MeasureSysSize(ref DrawTextArgs args) {
			GetTextParts(args.Text);
			if (EmptyText(args.Text)) return Size.Empty;
			
			float width = 0, height = 0;
			for (int i = 0; i < parts.Count; i++) {
				SizeF size = measuringGraphics.MeasureString(parts[i].Text, args.Font, Int32.MaxValue, format);
				height = Math.Max(height, size.Height);
				width += size.Width;
			}
			
			if (args.UseShadow) { width += Offset; height += Offset; }
			return new Size((int)Math.Ceiling(width), (int)Math.Ceiling(height));
		}
		
		void DisposeText() {
			measuringGraphics.Dispose();
			measuringBmp.Dispose();
		}
	}
}
#endif