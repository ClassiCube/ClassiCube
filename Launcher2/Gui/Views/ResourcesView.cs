// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp;
using Launcher.Drawing;
using Launcher.Gui.Widgets;

namespace Launcher.Gui.Views {	
	public sealed class ResourcesView : IView {
		
		Font statusFont;
		public ResourcesView(LauncherWindow game) : base(game) {
			widgets = new Widget[6];
		}

		public override void Init() {
			statusFont = new Font(game.FontName, 13, FontStyle.Italic);
			titleFont = new Font(game.FontName, 16, FontStyle.Bold);
			textFont = new Font(game.FontName, 14, FontStyle.Regular);
			
			MakeWidgets();
			widgets[cancelIndex].Visible = false;
			widgets[sliderIndex].Visible = false;
		}
		
		const int boxWidth = 190 * 2, boxHeight = 70 * 2;
		public override void DrawAll() {
			RedrawBackground();
			base.DrawAll();
		}
		
		void RedrawBackground() {
			using (FastBitmap bmp = game.LockBits()) {
				Rectangle r = new Rectangle(0, 0, bmp.Width, bmp.Height);
				Drawer2DExt.Clear(bmp, r, clearCol);
				
				r = new Rectangle(game.Width / 2 - boxWidth / 2,
				                  game.Height / 2 - boxHeight / 2,
				                  boxWidth, boxHeight);
				Gradient.Noise(bmp, r, backCol, 4);
			}
		}

		internal int yesIndex, noIndex, cancelIndex, sliderIndex, textIndex;
		const string format = "&eDownload size: {0} megabytes";
		internal bool downloadingItems;
		
		internal void RedrawStatus(string text) {
			LabelWidget widget = (LabelWidget)widgets[0];
			using (drawer) {
				drawer.SetBitmap(game.Framebuffer);
				drawer.Clear(backCol, widget.X, widget.Y, widget.Width, widget.Height);
				widget.SetDrawData(drawer, text);
				widget.SetLocation(Anchor.Centre, Anchor.Centre, 0, -10);
				widget.Redraw(drawer);
			}
		}
		
		internal void DrawProgressBox(int progress) {
			SliderWidget slider = (SliderWidget)widgets[sliderIndex];
			slider.Visible = true;
			slider.Value = progress;
			slider.Redraw(drawer);
		}
		
		public override void Dispose() {
			base.Dispose();
			statusFont.Dispose();
		}

		
		protected override void MakeWidgets() {
			widgetIndex = 0;
			MakeStatus();
			
			textIndex = widgetIndex;
			Makers.Label(this, mainText, textFont)
				.SetLocation(Anchor.Centre, Anchor.Centre, 0, -40);
			yesIndex = widgetIndex;
			Makers.Button(this, "Yes", 70, 35, titleFont)
				.SetLocation(Anchor.Centre, Anchor.Centre, -70, 45);
			noIndex = widgetIndex;
			Makers.Button(this, "No", 70, 35, titleFont)
				.SetLocation(Anchor.Centre, Anchor.Centre, 70, 45);
			
			cancelIndex = widgetIndex;
			Makers.Button(this, "Cancel", 120, 35, titleFont)
				.SetLocation(Anchor.Centre, Anchor.Centre, 0, 45);
			sliderIndex = widgetIndex;
			Makers.Slider(this, 200, 10, 0, 100, progFront)
				.SetLocation(Anchor.Centre, Anchor.Centre, 0, 15);
		}
		
		void MakeStatus() {
			widgetIndex = 0;
			if (downloadingItems) {
				Makers.Label(this, widgets[0].Text, statusFont)
					.SetLocation(Anchor.Centre, Anchor.Centre, 0, -10);
			} else {
				float dataSize = game.fetcher.DownloadSize;
				string text = String.Format(format, dataSize.ToString("F2"));
				Makers.Label(this, text, statusFont)
					.SetLocation(Anchor.Centre, Anchor.Centre, 0, 10);
			}
		}
		
		
		static FastColour backCol = new FastColour(120, 85, 151);
		static FastColour clearCol = new FastColour(12, 12, 12);
		static FastColour progFront = new FastColour(0, 220, 0);

		static readonly string mainText = "Some required resources weren't found" +
			Environment.NewLine + "Okay to download them?";
	}
}
