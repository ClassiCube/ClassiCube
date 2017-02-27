// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
#if ANDROID
using System;
using ClassicalSharp.GraphicsAPI;
using Android.Graphics;
using Android.Graphics.Drawables;
using System.Drawing;

namespace ClassicalSharp {

	public sealed partial class CanvasDrawer2D {
		
		Bitmap measuringBmp;
		Canvas measuringC;
		
		public CanvasDrawer2D(IGraphicsApi graphics) {
			this.graphics = graphics;		
			measuringBmp = Bitmap.CreateBitmap(1, 1, Bitmap.Config.Argb8888);
			measuringC = new Canvas(measuringBmp);
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