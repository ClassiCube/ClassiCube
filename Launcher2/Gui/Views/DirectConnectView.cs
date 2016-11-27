// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp;
using Launcher.Gui.Widgets;

namespace Launcher.Gui.Views {
	public sealed class DirectConnectView : IView {
		
		internal int connectIndex, backIndex, ccSkinsIndex, statusIndex;

		public DirectConnectView(LauncherWindow game) : base(game) {
			widgets = new Widget[8];
		}

		public override void Init() {
			titleFont = new Font(game.FontName, 15, FontStyle.Bold);
			textFont = new Font(game.FontName, 14, FontStyle.Regular);
			inputHintFont = new Font(game.FontName, 12, FontStyle.Italic);
			MakeWidgets();
		}
		
		string Get(int index) {
			Widget widget = widgets[index];
			return widget == null ? "" : widget.Text;
		}

		
		protected override void MakeWidgets() {
			widgetIndex = 0;
			
			MakeInput(Get(0), 330, false, 32, "&gUsername..")
				.SetLocation(Anchor.Centre, Anchor.Centre, 0, -120);
			MakeInput(Get(1), 330, false, 64, "&gIP address:Port number..")
				.SetLocation(Anchor.Centre, Anchor.Centre, 0, -75);
			MakeInput(Get(2), 330, false, 32, "&gMppass..")
				.SetLocation(Anchor.Centre, Anchor.Centre, 0, -30);
			
			connectIndex = widgetIndex;
			Makers.Button(this, "Connect", 110, 35, titleFont)
				.SetLocation(Anchor.Centre, Anchor.Centre, -110, 20);
			backIndex = widgetIndex;
			Makers.Button(this, "Back", 80, 35, titleFont)
				.SetLocation(Anchor.Centre, Anchor.Centre, 125, 20);
			
			statusIndex = widgetIndex;
			Makers.Label(this, "", textFont)
				.SetLocation(Anchor.Centre, Anchor.Centre, 0, 70);
			Makers.Label(this, "Use classicube.net for skins", textFont)
				.SetLocation(Anchor.Centre, Anchor.Centre, 30, 100);
			
			ccSkinsIndex = widgetIndex;
			Makers.Checkbox(this, true, 24)
				.SetLocation(Anchor.Centre, Anchor.Centre, -110, 100);
		}
	}
}
