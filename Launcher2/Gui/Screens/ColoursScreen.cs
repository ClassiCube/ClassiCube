// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp;
using Launcher.Gui.Views;
using Launcher.Gui.Widgets;
using OpenTK.Input;

namespace Launcher.Gui.Screens {
	public sealed class ColoursScreen : InputScreen {
		
		ColoursView view;
		public ColoursScreen(LauncherWindow game) : base(game) {
			enterIndex = 6;
			view = new ColoursView(game);
			widgets = view.widgets;
		}

		public override void Init() {
			base.Init();
			view.Init();
			
			widgets[view.defIndex].OnClick = (x, y) => ResetColours();
			widgets[view.defIndex + 1].OnClick = (x, y) => game.SetScreen(new SettingsScreen(game));
			SetupInputHandlers();
			for (int i = 0; i < widgets.Length; i++) {
				InputWidget input = widgets[i] as InputWidget;
				if (input == null) continue;
				input.Chars.TextChanged = TextChanged;
			}
			Resize();
		}

		public override void Resize() {
			view.DrawAll();
			game.Dirty = true;
		}
		
		public override void Dispose() {
			view.Dispose();
			base.Dispose();
		}
		
		
		protected override void MouseMove(int x, int y, int xDelta, int yDelta) {
			base.MouseMove(x, y, xDelta, yDelta);
			
			// TODO: sliders
			return;			
			for (int i = 0; i < 3; i++) {
				SliderWidget slider = (SliderWidget)widgets[view.sliderIndex + i];				
				if (x < slider.X || y < slider.Y || x >= slider.X + slider.Width
				   || y >= slider.Y + slider.Height) continue;
				
				int value = x - slider.X;
				// Map from 0 to 255
				value = (255 * value) / (slider.Width - 1);
				slider.Value = value;
				RedrawWidget(slider);
				return;
			}
		}
		
		protected override void MouseWheelChanged(object sender, MouseWheelEventArgs e) {
			AdjustSelectedColour(e.Delta);
		}
		
		protected override void KeyDown(object sender, KeyboardKeyEventArgs e) {
			if (e.Key == Key.Left) {
				AdjustSelectedColour(-1);
			} else if (e.Key == Key.Right) {
				AdjustSelectedColour(+1);
			} else if (e.Key == Key.Up) {
				AdjustSelectedColour(+10);
			} else if (e.Key == Key.Down) {
				AdjustSelectedColour(-10);
			} else {
				base.KeyDown(sender, e);
			}
		}
		
		void AdjustSelectedColour(int delta) {
			if (curInput == null) return;
			int index = Array.IndexOf<Widget>(widgets, curInput);
			if (index >= 15) return;
			
			byte col;
			if (!Byte.TryParse(curInput.Text, out col))  return;
			int newCol = col + delta;
			
			Utils.Clamp(ref newCol, 0, 255);
			curInput.Text = newCol.ToString();
			if (curInput.Chars.CaretPos >= curInput.Text.Length)
				curInput.Chars.CaretPos = -1;
			TextChanged(curInput);
		}
		
		void ResetColours() {
			LauncherSkin.ResetToDefault();
			view.MakeAllRGBTriplets(true);
			game.RedrawBackground();
			Resize();
		}
		
		void TextChanged(InputWidget widget) {
			bool changed = false;
			int index = Array.IndexOf<Widget>(widgets, widget);
			if (index < 3) changed |= Parse(0, ref LauncherSkin.BackgroundCol);
			else if (index < 6) changed |= Parse(3, ref LauncherSkin.ButtonBorderCol);
			else if (index < 9) changed |= Parse(6, ref LauncherSkin.ButtonHighlightCol);
			else if (index < 12) changed |= Parse(9, ref LauncherSkin.ButtonForeCol);
			else if (index < 15) changed |= Parse(12, ref LauncherSkin.ButtonForeActiveCol);
			
			if (!changed) return;
			game.RedrawBackground();
			Resize();
		}
		
		bool Parse(int index, ref FastColour dst) {
			byte r, g, b;
			if (!Byte.TryParse(widgets[index + 0].Text, out r)
			   || !Byte.TryParse(widgets[index + 1].Text, out g)
			   || !Byte.TryParse(widgets[index + 2].Text, out b))
				return false;
			dst.R = r; dst.G = g; dst.B = b;
			return true;
		}
	}
}
