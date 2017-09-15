// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
#if ANDROID
using System;
using System.Collections.Generic;
using ClassicalSharp.GraphicsAPI;
using System.Drawing;
using Android.Graphics;
using Android.Graphics.Drawables;

namespace ClassicalSharp {

	public sealed class CanvasDrawer2D : IDrawer2D {

		Dictionary<int, Paint> brushCache = new Dictionary<int, Paint>(16);
		Bitmap curBmp, measuringBmp;
		Canvas c, measuringC;
		
		public CanvasDrawer2D(IGraphicsApi graphics) {
			this.graphics = graphics;
			measuringBmp = Bitmap.CreateBitmap(1, 1, Bitmap.Config.Argb8888);
			measuringC = new Canvas(measuringBmp);
		}
		
		public override void SetBitmap(Bitmap bmp) {
			if (c != null) {
				Utils.LogDebug("Previous IDrawer2D.SetBitmap() call was not properly disposed");
				c.Dispose();
			}
			
			c = new Canvas(bmp);
			curBmp = bmp;
		}
		
		public override Bitmap ConvertTo32Bpp(Bitmap src) { return src; }

		public override void DrawRect(FastColour colour, int x, int y, int width, int height) {
			RectF rec = new RectF(x, y, x + width, y + height);
			Paint brush = GetOrCreateBrush(colour);
			c.DrawRect(rec, brush);
		}
		
		public override void DrawRectBounds(FastColour colour, float lineWidth, int x, int y, int width, int height) {
			RectF rec = new RectF(x, y, x + width, y + height);
			Paint brush = GetOrCreateBrush(colour);
			brush.SetStyle(Paint.Style.Stroke);
			c.DrawRect(rec, brush);
			brush.SetStyle(Paint.Style.FillAndStroke);
		}
		
		public override void Clear(FastColour colour) {
			c.DrawColor(colour);
		}
		
		public override void Clear(FastColour colour, int x, int y, int width, int height) {
			RectF rec = new RectF(x, y, x + width, y + height);
			Paint brush = GetOrCreateBrush(colour);
			brush.AntiAlias = false;
			c.DrawRect(rec, brush);
			brush.AntiAlias = true;
		}

		public override void Dispose() {
			c.Dispose();
			c = null;
		}
		
		public override void DisposeInstance() {
			foreach (var pair in brushCache)
				pair.Value.Dispose();
			
			DisposeText();
			DisposeBitmappedText();
		}
		
		Paint GetOrCreateBrush(Color color) {
			int key = color.ToArgb();
			Paint brush;
			if (brushCache.TryGetValue(key, out brush))
				return brush;
			
			brush = new Paint();
			brush.AntiAlias = true;
			brush.Color = color;
			brush.FilterBitmap = true;
			brush.SetStyle(Paint.Style.FillAndStroke);
			brushCache[key] = brush;
			return brush;
		}

		protected override void DrawSysText(ref DrawTextArgs args, int x, int y) {
			if (!args.SkipPartsCheck)
				GetTextParts(args.Text);
			
			float textX = x;
			Paint backBrush = GetOrCreateBrush(FastColour.Black);
			for (int i = 0; i < parts.Count; i++) {
				TextPart part = parts[i];
				Paint foreBrush = GetOrCreateBrush(part.Col);
				if (args.UseShadow)
					c.DrawText(part.Text, textX + Offset, y + Offset, backBrush);
				
				c.DrawText(part.Text, textX, y, foreBrush);
				textX += foreBrush.MeasureText(part.Text);
			}
		}
		
		public override void DrawClippedText(ref DrawTextArgs args, int x, int y, float maxWidth, float maxHeight) {
			throw new NotImplementedException();
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
			if (parts.Count == 0)
				return Size.Empty;
			
			SizeF total = SizeF.Empty;
			for (int i = 0; i < parts.Count; i++) {
				TextPart part = parts[i];
				Paint textBrush = GetOrCreateBrush(part.Col);
				total.Width += textBrush.MeasureText(part.Text);
			}
			total.Height = PtToPx(args.Font.Size);
			if (args.UseShadow) {
				total.Width += Offset; total.Height += Offset;
			}
			return Size.Ceiling(total);
		}
		
		int PtToPx(int point) {
			return (int)Math.Ceiling((float)point / 72 * 96); // TODO: not sure if even right, non 96 dpi?
		}
		
		void DisposeText() {
			measuringC.Dispose();
			measuringBmp.Dispose();
		}
	}
}
#endif