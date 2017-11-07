// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp;
using Launcher.Gui.Widgets;
using OpenTK.Input;

namespace Launcher.Gui.Views {	
	public sealed class ColoursView : IView {
		
		public ColoursView(LauncherWindow game) : base(game) {
			widgets = new Widget[25]; // TODO: add +3 to total for sliders
		}
		internal int defIndex, sliderIndex;
		
		public override void Init() {
			titleFont = new Font(game.FontName, 16, FontStyle.Bold);
			textFont = new Font(game.FontName, 14, FontStyle.Regular);
			inputHintFont = new Font(game.FontName, 12, FontStyle.Italic);
			MakeWidgets();
		}
		
		
		protected override void MakeWidgets() {
			widgetIndex = 0;
			MakeAllRGBTriplets(false);
			int start = widgetIndex;
			Makers.Label(this, "Background", textFont)
				.SetLocation(Anchor.Centre, Anchor.Centre, -60, -100);
			Makers.Label(this, "Button border", textFont)
				.SetLocation(Anchor.Centre, Anchor.Centre, -70, -60);
			Makers.Label(this, "Button highlight", textFont)
				.SetLocation(Anchor.Centre, Anchor.Centre, -80, -20);
			Makers.Label(this, "Button", textFont)
				.SetLocation(Anchor.Centre, Anchor.Centre, -40, 20);
			Makers.Label(this, "Active button", textFont)
				.SetLocation(Anchor.Centre, Anchor.Centre, -70, 60);
			
			for (int i = start; i < widgetIndex; i++) {
				// TODO: darken when inactive for sliders
				//((LabelWidget)widgets[i]).DarkenWhenInactive = true;
				widgets[i].TabSelectable = true;
			}
			
			Makers.Label(this, "Red", titleFont)
				.SetLocation(Anchor.Centre, Anchor.Centre, 30, -130);
			Makers.Label(this, "Green", titleFont)
				.SetLocation(Anchor.Centre, Anchor.Centre, 95, -130);
			Makers.Label(this, "Blue", titleFont)
				.SetLocation(Anchor.Centre, Anchor.Centre, 160, -130);
			
			defIndex = widgetIndex;
			Makers.Button(this, "Default colours", 160, 35, titleFont)
				.SetLocation(Anchor.Centre, Anchor.Centre, 0, 120);
			Makers.Button(this, "Back", 80, 35, titleFont)
				.SetLocation(Anchor.Centre, Anchor.Centre, 0, 170);
			
			// TODO: sliders
			return;
			
			sliderIndex = widgetIndex;
			Makers.Slider(this, 100, 10, 30, 255, FastColour.Red)
				.SetLocation(Anchor.LeftOrTop, Anchor.LeftOrTop, 5, 5);
			Makers.Slider(this, 100, 10, 30, 255, FastColour.Green)
				.SetLocation(Anchor.LeftOrTop, Anchor.LeftOrTop, 5, 25);
			Makers.Slider(this, 100, 10, 30, 255, FastColour.Blue)
				.SetLocation(Anchor.LeftOrTop, Anchor.LeftOrTop, 5, 45);
		}
		
		public void MakeAllRGBTriplets(bool force) {
			widgetIndex = 0;
			MakeRGBTriplet(LauncherSkin.BackgroundCol, force, -100);
			MakeRGBTriplet(LauncherSkin.ButtonBorderCol, force, -60);
			MakeRGBTriplet(LauncherSkin.ButtonHighlightCol, force, -20);
			MakeRGBTriplet(LauncherSkin.ButtonForeCol, force, 20);
			MakeRGBTriplet(LauncherSkin.ButtonForeActiveCol, force, 60);
		}
		
		void MakeRGBTriplet(FastColour defCol, bool force, int y) {
			MakeInput(GetCol(defCol.R, force), 55, false, 3, null)
				.SetLocation(Anchor.Centre, Anchor.Centre, 30, y);
			MakeInput(GetCol(defCol.G, force), 55, false, 3, null)
				.SetLocation(Anchor.Centre, Anchor.Centre, 95, y);
			MakeInput(GetCol(defCol.B, force), 55, false, 3, null)
				.SetLocation(Anchor.Centre, Anchor.Centre, 160, y);
		}
		
		string GetCol(byte col, bool force) {
			if (force) return col.ToString();
			Widget widget = widgets[widgetIndex];
			return widget == null ? col.ToString() : widget.Text;
		}
	}
}
