// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Diagnostics;
using System.Drawing;
using ClassicalSharp.Gui.Widgets;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	
	public delegate void WarningClickHandler(WarningOverlay screen, bool isAlways);
	
	public sealed class UrlWarningOverlay : Overlay {
		
		public UrlWarningOverlay(Game game, string url) : base(game) {
			widgets = new ButtonWidget[2];
			Metadata = url;
			lines[0] = "&eAre you sure you want to open this link?";
			lines[1] = url;
			lines[2] = "Be careful - links from strangers may be websites that";
			lines[3] = " have viruses, or things you may not want to open/see.";
		}

		public override void RedrawText() {
			SetTextWidgets(lines);
		}
		
		public override void MakeButtons() {
			DisposeWidgets(widgets);
			widgets[0] = ButtonWidget.Create(game, 160, "Yes", titleFont, OpenUrl)
				.SetLocation(Anchor.Centre, Anchor.Centre, -110, 30);
			widgets[1] = ButtonWidget.Create(game, 160, "No", titleFont, AppendUrl)
				.SetLocation(Anchor.Centre, Anchor.Centre, 110, 30);
		}
		
		void OpenUrl(Game g, Widget w, MouseButton btn, int x, int y) {
			if (btn != MouseButton.Left) return;
			
			try {
				Process.Start(Metadata);
			} catch (Exception ex) {
				ErrorHandler.LogError("UrlWarningOverlay.OpenUrl", ex);
			}
			CloseOverlay();
		}
		
		void AppendUrl(Game g, Widget w, MouseButton btn, int x, int y) {
			if (btn != MouseButton.Left) return;
			
			if (game.ClickableChat) {
				game.Gui.hudScreen.AppendInput(Metadata);
			}
			CloseOverlay();
		}
	}
	
	public sealed class ConfirmDenyOverlay : Overlay {
		WarningClickHandler noClick;
		bool alwaysDeny;
		
		public ConfirmDenyOverlay(Game game, string url, bool always, WarningClickHandler noClick) : base(game) {
			Metadata = url;
			alwaysDeny = always;
			this.noClick = noClick;
			widgets = new ButtonWidget[2];
			lines[0] = "&eYou might be missing out.",
			lines[1] = "Texture packs can play a vital role in the look and feel of maps.",
			lines[2] = "";
			lines[3] = "Sure you don't want to download the texture pack?";
		}

		public override void RedrawText() {
			SetTextWidgets(lines);
		}
		
		public override void MakeButtons() {
			DisposeWidgets(widgets);
			widgets[0] = ButtonWidget.Create(game, 160, "I'm sure", titleFont, ConfirmNoClick)
				.SetLocation(Anchor.Centre, Anchor.Centre, -110, 30);
			widgets[1] = ButtonWidget.Create(game, 160, "Go back", titleFont, GoBackClick)
				.SetLocation(Anchor.Centre, Anchor.Centre, 110, 30);
		}
		
		void ConfirmNoClick(Game g, Widget w, MouseButton btn, int x, int y) {
			if (btn != MouseButton.Left) return;
			noClick(this, alwaysDeny);
			CloseOverlay();
		}
		
		void GoBackClick(Game g, Widget w, MouseButton btn, int x, int y) {
			if (btn != MouseButton.Left) return;
			
			// TODO: Do the back thingy here
			CloseOverlay();
		}
	}
	
	public sealed class WarningOverlay : Overlay {
		
		public WarningOverlay(Game game, bool showAlways, bool confirmNo) : base(game) {
			this.confirmNo = confirmNo;
			this.showAlways = showAlways;
		}
		
		public void SetHandlers(WarningClickHandler yesClick,
		                        WarningClickHandler noClick) {
			this.yesClick = yesClick;
			this.noClick = noClick;
		}
		
		bool confirmNo, confirmingMode, showAlways;
		int alwaysIndex = 100;

		public override void RedrawText() {
			if (confirmingMode) {
				SetTextWidgets(new string[] {
				               	"&eYou might be missing out.",
				               	"Texture packs can play a vital role in the look and feel of maps.",
				               	"", "Sure you don't want to download the texture pack?"});
			} else {
				SetTextWidgets(lines);
			}
		}
		
		public override void MakeButtons() {
			DisposeWidgets(widgets);
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
		
		WarningClickHandler yesClick, noClick;
		
		void OnYesClick(Game g, Widget w, MouseButton btn, int x, int y) {
			if (btn != MouseButton.Left) return;
			bool always = IndexOfWidget(w) >= alwaysIndex;
			
			if (yesClick != null) yesClick(this, always);
			CloseOverlay();
		}
		
		void OnNoClick(Game g, Widget w, MouseButton btn, int x, int y) {
			if (btn != MouseButton.Left) return;
			bool always = IndexOfWidget(w) >= alwaysIndex;
			
			if (confirmNo && !confirmingMode) {
				confirmingMode = true;
				ContextRecreated();
				return;
			}
			
			if (noClick != null) noClick(this, always);
			CloseOverlay();
		}
		
		void GoBackClick(Game g, Widget w, MouseButton btn, int x, int y) {
			if (btn != MouseButton.Left) return;
			
			confirmingMode = false;
			ContextRecreated();
		}
	}
}