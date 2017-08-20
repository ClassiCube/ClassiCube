// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Gui.Widgets;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	
	public sealed class TexIdsOverlay : Overlay {
		
		TextAtlas idAtlas;
		public TexIdsOverlay(Game game) : base(game) { }
		
		public override void Init() {
			base.Init();
			idAtlas = new TextAtlas(game, 16);
			idAtlas.Pack("0123456789", regularFont, "f");
		}
		
		public override void Render(double delta) {
			int index = 0;
			VertexP3fT2fC4b[] vertices = game.ModelCache.vertices;			
			idAtlas.tex.Y = 100;
			
			for (int y = 0; y < 2; y++) {
				idAtlas.curX = 0;
				for (int x = 0; x < 16; x++) {
					idAtlas.AddInt(x + (y * 16), vertices, ref index);
					idAtlas.curX += 20;
				}
				idAtlas.tex.Y += 50;
			}
			
			RenderMenuBounds();
			gfx.Texturing = true;
			gfx.SetBatchFormat(VertexFormat.P3fT2fC4b);
			gfx.BindTexture(idAtlas.tex.ID);
			gfx.UpdateDynamicVb_IndexedTris(game.ModelCache.vb, game.ModelCache.vertices, index);
			gfx.Texturing = false;
		}
		
		public override bool HandlesKeyDown(Key key) {
			if (key == Key.F10 || key == game.Input.Keys[KeyBind.PauseOrExit]) {
				Dispose();
				CloseOverlay();
			}
			return true;
		}

		public override void RedrawText() { }
		
		public override void MakeButtons() {
			widgets = new Widget[0];
		}
		
		public override void Dispose() {
			base.Dispose();
			idAtlas.Dispose();
		}
	}
}