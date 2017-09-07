// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using ClassicalSharp.Gui.Widgets;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	
	public abstract class Overlay : MenuScreen {
		
		public Action<Overlay> OnRenderFrame;
		protected TextWidget[] labels;
		public string[] lines = new string[4];
		public string Metadata;
		
		public Overlay(Game game) : base(game) { }
		
		public override void Init() {
			base.Init();
			titleFont = new Font(game.FontName, 16, FontStyle.Bold);
			regularFont = new Font(game.FontName, 16);
			backCol.A = 210;
			
			if (game.Graphics.LostContext) return;
			MakeButtons();
			RedrawText();
		}

		public override void Render(double delta) {
			RenderMenuBounds();
			gfx.Texturing = true;
			RenderWidgets(widgets, delta);
			RenderWidgets(labels, delta);
			gfx.Texturing = false;
			
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
			if (game.Gui.overlays.Count > 0)
				game.Gui.overlays.RemoveAt(0);
			if (game.Gui.overlays.Count == 0)
				game.CursorVisible = game.realVisible;
			game.Camera.RegrabMouse();
		}
		
		public abstract void RedrawText();
		
		public abstract void MakeButtons();
				
		protected void SetTextWidgets(string[] lines) {
			if (labels != null) {
				for (int i = 0; i < labels.Length; i++)
					labels[i].Dispose();
			}
			
			int count = 0;
			for (int i = 0; i < lines.Length; i++) {
				if (lines[i] != null) count++;
			}
			
			labels = new TextWidget[count];
			labels[0] = TextWidget.Create(game, lines[0], titleFont)
				.SetLocation(Anchor.Centre, Anchor.Centre, 0, -120);
					
			for (int i = 1; i < count; i++) {
				labels[i] = TextWidget.Create(game, lines[i], regularFont)
					.SetLocation(Anchor.Centre, Anchor.Centre, 0, -70 + 20 * i);
				labels[i].Colour = new FastColour(224, 224, 224);
			}
		}
	}
}