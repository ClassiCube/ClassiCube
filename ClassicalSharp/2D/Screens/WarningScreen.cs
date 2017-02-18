// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
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
		
		string title;
		string[] body;
		bool confirmNo, confirmingMode, showAlways;
		int alwaysIndex = 100;
		
		public override void Init() {
			base.Init();
			titleFont = new Font(game.FontName, 16, FontStyle.Bold);
			regularFont = new Font(game.FontName, 16, FontStyle.Regular);
			backCol.A = 210;
			
			if (game.Graphics.LostContext) return;
			InitStandardButtons();
			RedrawText();
		}

		public void RedrawText() {
			if (confirmingMode) {
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

		protected override void ContextLost() {
			base.ContextLost();
			if (labels == null) return;	
			
			for (int i = 0; i < labels.Length; i++)
				labels[i].Dispose();
		}
		
		protected override void ContextRecreated() {
			InitStandardButtons();
			RedrawText();
		}
		
		
		void InitStandardButtons() {
			base.ContextLost();			
			alwaysIndex = 100;
			
			if (confirmingMode) {
				widgets = new ButtonWidget[2];
				widgets[0] = ButtonWidget.Create(game, 160, "I'm sure", titleFont, OnNoClick)
					.SetLocation(Anchor.Centre, Anchor.Centre, -110, 30);
				widgets[1] = ButtonWidget.Create(game, 160, "Go back", titleFont, GoBackClick)
					.SetLocation(Anchor.Centre, Anchor.Centre, 110, 30);
				return;
			}
						
			widgets = new ButtonWidget[showAlways ? 4 : 2];
			widgets[0] = ButtonWidget.Create(game, 160, "Yes", titleFont, OnYesClick)
				.SetLocation(Anchor.Centre, Anchor.Centre, -110, 30);
			widgets[1] = ButtonWidget.Create(game, 160, "No", titleFont, OnNoClick)
				.SetLocation(Anchor.Centre, Anchor.Centre, 110, 30);
			if (!showAlways) return;
			
			alwaysIndex = 2;
			widgets[2] = ButtonWidget.Create(game, 160, "Always yes", titleFont, OnYesClick)
				.SetLocation(Anchor.Centre, Anchor.Centre, -110, 85);
			widgets[3] = ButtonWidget.Create(game, 160, "Always no", titleFont, OnNoClick)
				.SetLocation(Anchor.Centre, Anchor.Centre, 110, 85);
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
			
			if (confirmNo && !confirmingMode) {
				confirmingMode = true;
				ContextRecreated();
				return;
			}
			
			if (noClick != null) noClick(this, always);
			Dispose();
			CloseScreen();
		}
		
		void GoBackClick(Game g, Widget w, MouseButton btn, int x, int y) {
			if (btn != MouseButton.Left) return;
			
			confirmingMode = false;
			ContextRecreated();
		}
	}
}