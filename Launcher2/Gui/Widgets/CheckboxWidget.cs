// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp;
using Launcher.Drawing;

namespace Launcher.Gui.Widgets {
	/// <summary> Represents a state that can be toggled by the user. </summary>
	public sealed class CheckboxWidget : Widget {
		
		public bool Value;
		
		public CheckboxWidget(LauncherWindow window, int width, int height) : base(window) {
			Width = width; Height = height;
		}

		public override void Redraw(IDrawer2D drawer) {
			if (Window.Minimised || !Visible) return;
			Rectangle rect = new Rectangle(X, Y, Width, Height / 2);
			using (FastBitmap bmp = Window.LockBits()) {				
				Gradient.Vertical(bmp, rect, boxTop, boxBottom);
				rect.Y += rect.Height;
				Gradient.Vertical(bmp, rect, boxBottom, boxTop);
				
				if (Value) {
					const int size = 12;
					int x = X + Width / 2 - size / 2;
					int y = Y + Height / 2 - size / 2;
					BitmapDrawer.DrawIndexed(indices, palette, size, x, y, bmp);
				}
			}
			drawer.DrawRectBounds(PackedCol.Black, 1, X, Y, Width - 1, Height - 1);
		}
		
		
		static PackedCol boxTop = new PackedCol(255, 255, 255);
		static PackedCol boxBottom = new PackedCol(240, 240, 240);
		
		// Based off checkbox from original ClassiCube Launcher
		static byte[] indices = new byte[] {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x06, 0x07, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x06, 0x09, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x06, 0x0B, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0D, 0x0E, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x0F, 0x06, 0x10, 0x00, 0x11, 0x06, 0x12, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x13, 0x14, 0x15, 0x00, 0x16, 0x17, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x18, 0x06, 0x19, 0x06, 0x1A, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x1B, 0x06, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x1D, 0x06, 0x1A, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		};

		static PackedCol[] palette = new PackedCol[] {
			new PackedCol(0, 0, 0, 0),    new PackedCol(144, 144, 144),
			new PackedCol(61, 61, 61),    new PackedCol(94, 94, 94),
			new PackedCol(197, 196, 197), new PackedCol(57, 57, 57),
			new PackedCol(33, 33, 33),    new PackedCol(177, 177, 177),
			new PackedCol(189, 189, 189), new PackedCol(67, 67, 67),
			new PackedCol(108, 108, 108), new PackedCol(171, 171, 171),
			new PackedCol(220, 220, 220), new PackedCol(43, 43, 43),
			new PackedCol(63, 63, 63),    new PackedCol(100, 100, 100),
			new PackedCol(192, 192, 192), new PackedCol(132, 132, 132),
			new PackedCol(175, 175, 175), new PackedCol(217, 217, 217),
			new PackedCol(42, 42, 42),    new PackedCol(86, 86, 86),
			new PackedCol(56, 56, 56),    new PackedCol(76, 76, 76),
			new PackedCol(139, 139, 139), new PackedCol(130, 130, 130),
			new PackedCol(181, 181, 181), new PackedCol(62, 62, 62),
			new PackedCol(75, 75, 75),    new PackedCol(184, 184, 184),
		};
	}
}
