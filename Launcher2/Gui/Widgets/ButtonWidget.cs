// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp;
using Launcher.Drawing;

namespace Launcher.Gui.Widgets {
	public sealed class ButtonWidget : Widget {
		
		public bool Shadow = true;
		const int border = 1;
		Size textSize;
		Font font;
		
		public ButtonWidget(LauncherWindow window) : base(window) {
			TabSelectable = true;
		}
		
		public void SetDrawData(IDrawer2D drawer, string text, Font font, int width, int height) {
			Width = width; Height = height;
			CalculatePosition();
			this.font = font;

			Text = text;
			DrawTextArgs args = new DrawTextArgs(text, font, true);
			textSize = drawer.MeasureSize(ref args);
		}
		
		public override void Redraw(IDrawer2D drawer) {
			if (Window.Minimised || !Visible) return;
			string text = Text;
			if (!Active) text = "&7" + text;
			int xOffset = Width - textSize.Width, yOffset = Height - textSize.Height;
			DrawTextArgs args = new DrawTextArgs(text, font, true);
			
			DrawBorder(drawer);
			if (Window.ClassicBackground) DrawClassic(drawer);
			else DrawNormal(drawer);
			
			drawer.DrawText(ref args, X + xOffset / 2, Y + yOffset / 2);
		}
		
		void DrawBorder(IDrawer2D drawer) {
			FastColour backCol = Window.ClassicBackground ? FastColour.Black : LauncherSkin.ButtonBorderCol;
			drawer.Clear(backCol, X + border, Y, Width - border * 2, border);
			drawer.Clear(backCol, X + border, Y + Height - border, Width - border * 2, border);
			drawer.Clear(backCol, X, Y + border, border, Height - border * 2);
			drawer.Clear(backCol, X + Width - border, Y + border, border, Height - border * 2);
		}
		
		void DrawNormal(IDrawer2D drawer) {
			if (Active) return;
			FastColour lineCol = LauncherSkin.ButtonHighlightCol;
			drawer.Clear(lineCol, X + border * 2, Y + border, Width - border * 4, border);
		}
		
		void DrawClassic(IDrawer2D drawer) {
			FastColour highlightCol = Active ? new FastColour(189, 198, 255) : new FastColour(168, 168, 168);
			drawer.Clear(highlightCol, X + border * 2, Y + border, Width - border * 4, border);
			drawer.Clear(highlightCol, X + border, Y + border * 2, border, Height - border * 4);
		}
		
		public void RedrawBackground() {
			if (Window.Minimised || !Visible) return;
			using (FastBitmap dst = Window.LockBits())
				RedrawBackground(dst);
		}
		
		public void RedrawBackground(FastBitmap dst) {
			if (Window.Minimised || !Visible) return;
			Rectangle rect = new Rectangle(X + border, Y + border, Width - border * 2, Height - border * 2);
			if (Window.ClassicBackground) {
				FastColour foreCol = Active ? new FastColour(126, 136, 191) : new FastColour(111, 111, 111);
				Gradient.Noise(dst, rect, foreCol, 8);
			} else {
				FastColour foreCol = Active ? LauncherSkin.ButtonForeActiveCol : LauncherSkin.ButtonForeCol;
				FastColour top = Expand(foreCol, 8), bottom = Expand(foreCol, -8);
				Gradient.Vertical(dst, rect, top, bottom);

			}
		}
		
		static FastColour Expand(FastColour a, int amount) {
			int r = a.R + amount; Utils.Clamp(ref r, 0, 255);
			int g = a.G + amount; Utils.Clamp(ref g, 0, 255);
			int b = a.B + amount; Utils.Clamp(ref b, 0, 255);
			return new FastColour(r, g, b);
		}
	}
}
