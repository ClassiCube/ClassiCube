// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using ClassicalSharp.Gui.Widgets;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	
	public delegate void WarningClickHandler(WarningOverlay screen, bool isAlways);
	
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
			Dispose();
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
			Dispose();
			CloseOverlay();
		}
		
		void GoBackClick(Game g, Widget w, MouseButton btn, int x, int y) {
			if (btn != MouseButton.Left) return;
			
			confirmingMode = false;
			ContextRecreated();
		}
	}
}