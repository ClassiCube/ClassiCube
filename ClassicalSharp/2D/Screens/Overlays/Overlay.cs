// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using ClassicalSharp.Gui.Widgets;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	
	public abstract class Overlay : MenuScreen {
		
		public Action<Overlay> OnRenderFrame;
		protected TextWidget[] labels = new TextWidget[4];
		public string[] lines = new string[4];
		public string Metadata;
		
		public Overlay(Game game) : base(game) { }
		
		public override void Init() {
			base.Init();
			titleFont = new Font(game.FontName, 16, FontStyle.Bold);
			regularFont = new Font(game.FontName, 16);
			
			if (game.Graphics.LostContext) return;
			MakeButtons();
			RedrawText();
		}

		public override void Render(double delta) {
			RenderMenuBounds();
			game.Graphics.Texturing = true;
			RenderWidgets(widgets, delta);
			RenderWidgets(labels, delta);
			game.Graphics.Texturing = false;
			
			if (OnRenderFrame != null) OnRenderFrame(this);
		}
		
		public override void OnResize(int width, int height) {
			base.OnResize(width, height);
			RepositionWidgets(labels);
		}

		protected override void ContextLost() {
			base.ContextLost();
			DisposeWidgets(labels);
		}
		
		protected override void ContextRecreated() {
			MakeButtons();
			RedrawText();
		}
		
		protected void CloseOverlay() {
			Dispose();
			
			game.Gui.overlays.Remove(this);
			if (game.Gui.overlays.Count == 0)
				game.CursorVisible = game.realVisible;
			game.Camera.RegrabMouse();
		}
		
		public virtual void RedrawText() {
			for (int i = 0; i < labels.Length; i++) {
				if (labels[i] == null) continue;				
				labels[i].Dispose();
				labels[i] = null;
			}

			labels[0] = TextWidget.Create(game, lines[0], titleFont)
				.SetLocation(Anchor.Centre, Anchor.Centre, 0, -120);
			
			for (int i = 1; i < 4; i++) {
				if (lines[i] == null) continue;
				labels[i] = TextWidget.Create(game, lines[i], regularFont)
					.SetLocation(Anchor.Centre, Anchor.Centre, 0, -70 + 20 * i);
				labels[i].Colour = new FastColour(224, 224, 224);
			}
		}
		
		public abstract void MakeButtons();
	}
}