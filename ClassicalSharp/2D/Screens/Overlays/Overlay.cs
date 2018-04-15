// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using ClassicalSharp.Gui.Widgets;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	
	public abstract class Overlay : MenuScreen {
		
		public Action<Overlay> OnRenderFrame;
		public string[] lines = new string[4];
		public string Metadata;
		
		public Overlay(Game game) : base(game) { }
		
		public override void Init() {
			base.Init();		
			if (game.Graphics.LostContext) return;
			ContextRecreated();
		}

		public override void Render(double delta) {
			base.Render(delta);
			if (OnRenderFrame != null) OnRenderFrame(this);
		}
		
		public override bool HandlesKeyDown(Key key) { return true; }
		
		protected void CloseOverlay() {
			Dispose();
			
			game.Gui.overlays.Remove(this);
			if (game.Gui.overlays.Count == 0)
				game.CursorVisible = game.realVisible;
			game.Camera.RegrabMouse();
		}
		
		public void MakeLabels() {
			widgets[0] = TextWidget.Create(game, lines[0], titleFont)
				.SetLocation(Anchor.Centre, Anchor.Centre, 0, -120);			
			for (int i = 1; i < 4; i++) {
				if (lines[i] == null) continue;
				
				TextWidget label = TextWidget.Create(game, lines[i], textFont)
					.SetLocation(Anchor.Centre, Anchor.Centre, 0, -70 + 20 * i);
				label.Colour = new FastColour(224, 224, 224);
				widgets[i] = label;
			}
		}
	}
}