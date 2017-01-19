// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
#if ANDROID
using System;
using System.Collections.Generic;
using Android.Graphics;
using Android.Graphics.Drawables;

namespace ClassicalSharp {

	public sealed partial class CanvasDrawer2D : IDrawer2D {

		Dictionary<int, Paint> brushCache = new Dictionary<int, Paint>(16);		
		Bitmap curBmp;
		Canvas c;
		
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
	}
}
#endif