// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using System.IO;
using ClassicalSharp;
using Launcher.Gui.Widgets;

namespace Launcher.Gui.Views {
	public sealed class UpdatesView : IView {
		
		public DateTime LastStable, LastDev;
		internal int backIndex, relIndex, devIndex, statusIndex;
		internal bool gameOpen;
		
		public UpdatesView(LauncherWindow game) : base(game) {
			widgets = new Widget[13];
		}

		public override void Init() {
			titleFont = new Font(game.FontName, 16, FontStyle.Bold);
			textFont = new Font(game.FontName, 14, FontStyle.Regular);
			MakeWidgets();
		}
		
		public override void DrawAll() {
			MakeWidgets();
			RedrawAllButtonBackgrounds();
			
			using (drawer) {
				drawer.SetBitmap(game.Framebuffer);
				RedrawAll();
				FastColour col = LauncherSkin.ButtonBorderCol;
				int middle = game.Height / 2;
				game.Drawer.DrawRect(col, game.Width / 2 - 160, middle - 100, 320, 1);
				game.Drawer.DrawRect(col, game.Width / 2 - 160, middle - 5, 320, 1);
			}
		}
		
		const string dateFormat = "dd-MM-yyyy HH:mm";
		protected override void MakeWidgets() {
			widgetIndex = 0;
			string exePath = Path.Combine(Program.AppDirectory, "ClassicalSharp.exe");
			
			Makers.Label(this, "Your build:", textFont)
				.SetLocation(Anchor.Centre, Anchor.Centre, -55, -120);
			string yourBuild = File.GetLastWriteTime(exePath).ToString(dateFormat);
			Makers.Label(this, yourBuild, textFont)
				.SetLocation(Anchor.Centre, Anchor.Centre, 70, -120);
			
			Makers.Label(this, "Latest release:", textFont)
				.SetLocation(Anchor.Centre, Anchor.Centre, -70, -75);
			string latestStable = GetDateString(LastStable);
			Makers.Label(this, latestStable, textFont)
				.SetLocation(Anchor.Centre, Anchor.Centre, 70, -75);
			relIndex = widgetIndex;
			Makers.Button(this, "Direct3D 9", 130, 35, titleFont)
				.SetLocation(Anchor.Centre, Anchor.Centre, -80, -40);
			Makers.Button(this, "OpenGL", 130, 35, titleFont)
				.SetLocation(Anchor.Centre, Anchor.Centre, 80, -40);
			
			Makers.Label(this, "Latest dev build:", textFont)
				.SetLocation(Anchor.Centre, Anchor.Centre, -80, 20);
			string latestDev = GetDateString(LastDev);
			Makers.Label(this, latestDev, textFont)
				.SetLocation(Anchor.Centre, Anchor.Centre, 70, 20);
			devIndex = widgetIndex;
			Makers.Button(this, "Direct3D 9", 130, 35, titleFont)
				.SetLocation(Anchor.Centre, Anchor.Centre, -80, 55);
			Makers.Button(this, "OpenGL", 130, 35, titleFont)
				.SetLocation(Anchor.Centre, Anchor.Centre, 80, 55);
			
			Makers.Label(this, "&eDirect3D 9 is recommended for Windows", textFont)
				.SetLocation(Anchor.Centre, Anchor.Centre, 0, 105);
			statusIndex = widgetIndex;
			string text = gameOpen ? "&cThe game must be closed before updating" : "";
			Makers.Label(this, text, textFont)
				.SetLocation(Anchor.Centre, Anchor.Centre, 0, 130);
			
			backIndex = widgetIndex;
			Makers.Button(this, "Back", 80, 35, titleFont)
				.SetLocation(Anchor.Centre, Anchor.Centre, 0, 170);
		}
		
		internal void SetWarning() {
			string text = gameOpen ? "&cThe game must be closed before updating" : "";
			LabelWidget widget = (LabelWidget)widgets[statusIndex];
			widget.SetDrawData(drawer, text);
			widget.SetLocation(Anchor.Centre, Anchor.Centre, 0, 130);
		}
		
		string GetDateString(DateTime last) {
			if (last == DateTime.MaxValue) return "&cCheck failed";
			if (last == DateTime.MinValue) return "Checking..";	
			return last.ToString(dateFormat);
		}
	}
}
