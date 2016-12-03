// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp.Gui.Widgets;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	public sealed class WarningScreen : MenuScreen {
		
		public WarningScreen(Game game, bool showAlways, bool confirmNo) : base(game) {
			this.confirmNo = confirmNo;
			this.showAlways = showAlways;
		}
		
		public void SetHandlers(Action<WarningScreen, bool> yesClick,
		                        Action<WarningScreen, bool> noClick,
		                        Action<WarningScreen> renderFrame) {
			this.yesClick = yesClick;
			this.noClick = noClick;
			this.renderFrame = renderFrame;
		}
		
		public void SetTextData(string title, params string[] body) {
			this.title = title;
			this.body = body;
		}
		
		string title, lastTitle;
		string[] body, lastBody;
		bool confirmNo, confirmMode, showAlways;
		int alwaysIndex = 100;
		
		public override void Init() {
			titleFont = new Font(game.FontName, 16, FontStyle.Bold);
			regularFont = new Font(game.FontName, 16, FontStyle.Regular);
			backCol.A = 210;
			InitStandardButtons();
			SetText(title, body);
		}

		public void SetText(string title, params string[] body) {
			lastTitle = title;
			lastBody = body;
			
			if (confirmMode) {
				SetTextImpl("&eYou might be missing out.",
				            "Texture packs can play a vital role in the look and feel of maps.",
				            "", "Sure you don't want to download the texture pack?");
			} else {
				SetTextImpl(title, body);
			}
		}
		
		void SetTextImpl(string title, params string[] body) {
			if (labels != null) {
				for (int i = 0; i < labels.Length; i++)
					labels[i].Dispose();
			}
			this.title = title;
			this.body = body;
			
			labels = new TextWidget[body.Length + 1];
			labels[0] = TextWidget.Create(game, title, titleFont)
				.SetLocation(Anchor.Centre, Anchor.Centre, 0, -120);
			
			for (int i = 0; i < body.Length; i++) {
				labels[i + 1] = TextWidget.Create(game, body[i], regularFont)
					.SetLocation(Anchor.Centre, Anchor.Centre, 0, -70 + 20 * i);
				labels[i + 1].Colour = new FastColour(224, 224, 224);
			}
		}
		TextWidget[] labels;
		
		void CloseScreen() {
			if (game.Gui.overlays.Count > 0)
				game.Gui.overlays.RemoveAt(0);
			if (game.Gui.overlays.Count == 0)
				game.CursorVisible = game.realVisible;
		}
		
		public override void Render(double delta) {
			RenderMenuBounds();
			gfx.Texturing = true;
			RenderMenuWidgets(delta);
			for (int i = 0; i < labels.Length; i++)
				labels[i].Render(delta);
			gfx.Texturing = false;
			
			if (renderFrame != null)
				renderFrame(this);
		}
		
		public override void OnResize(int width, int height) {
			base.OnResize(width, height);
			for (int i = 0; i < labels.Length; i++)
				labels[i].CalculatePosition();
		}
		
		public override void Dispose() {
			base.Dispose();
			for (int i = 0; i < labels.Length; i++)
				labels[i].Dispose();
		}
		
		void InitStandardButtons() {
			widgets = new ButtonWidget[showAlways ? 4 : 2];
			alwaysIndex = 100;
			
			widgets[0] = ButtonWidget.Create(game, 160, 35, "Yes", titleFont, OnYesClick)
				.SetLocation(Anchor.Centre, Anchor.Centre, -110, 30);
			widgets[1] = ButtonWidget.Create(game, 160, 35, "No", titleFont, OnNoClick)
				.SetLocation(Anchor.Centre, Anchor.Centre, 110, 30);
			if (!showAlways) return;
			
			alwaysIndex = 2;
			widgets[2] = ButtonWidget.Create(game, 160, 35, "Always yes", titleFont, OnYesClick)
				.SetLocation(Anchor.Centre, Anchor.Centre, -110, 80);
			widgets[3] = ButtonWidget.Create(game, 160, 35, "Always no", titleFont, OnNoClick)
				.SetLocation(Anchor.Centre, Anchor.Centre, 110, 80);
		}
		
		Action<WarningScreen, bool> yesClick, noClick;
		Action<WarningScreen> renderFrame;
		void OnYesClick(Game g, Widget w, MouseButton btn, int x, int y) {
			if (btn != MouseButton.Left) return;
			bool always = Array.IndexOf<Widget>(widgets, w) >= alwaysIndex;
			
			if (yesClick != null) yesClick(this, always);
			Dispose();
			CloseScreen();
		}
		
		void OnNoClick(Game g, Widget w, MouseButton btn, int x, int y) {
			if (btn != MouseButton.Left) return;
			bool always = Array.IndexOf<Widget>(widgets, w) >= alwaysIndex;
			
			if (confirmNo && !confirmMode) {
				InitConfirmButtons(always); return;
			}
			
			if (noClick != null) noClick(this, always);
			Dispose();
			CloseScreen();
		}
		
		void InitConfirmButtons(bool always) {
			alwaysIndex = always ? 0 : 100;
			widgets = new ButtonWidget[] {
				ButtonWidget.Create(game, 160, 35, "I'm sure", titleFont, OnNoClick)
					.SetLocation(Anchor.Centre, Anchor.Centre, -110, 30),
				ButtonWidget.Create(game, 160, 35, "Go back", titleFont, GoBackClick)
					.SetLocation(Anchor.Centre, Anchor.Centre, 110, 30),
			};
			confirmMode = true;
			SetText(lastTitle, lastBody);
		}
		
		void GoBackClick(Game g, Widget w, MouseButton btn, int x, int y) {
			if (btn != MouseButton.Left) return;
			InitStandardButtons();
			confirmMode = false;
			SetText(lastTitle, lastBody);
		}
	}
}