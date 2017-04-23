// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Mode;
#if ANDROID
using Android.Graphics;
#endif

namespace ClassicalSharp.Gui.Widgets {
	public sealed class SurvivalHotbarWidget : HotbarWidget {
		
		TextAtlas posAtlas;
		Font font;
		public SurvivalHotbarWidget(Game game) : base(game) {
		}
		
		// TODO: scaling
		public override void Init() {
			base.Init();
			font = new Font(game.FontName, 16);
			posAtlas = new TextAtlas(game, 16);
			posAtlas.Pack("0123456789", font, "f");
			game.Events.ChatFontChanged += ChatFontChanged;
		}
		
		void ChatFontChanged(object sender, EventArgs e) { Recreate(); }
		
		public override void Render(double delta) {
			base.Render(delta);
			DrawCounts();
			DrawHearts();
		}
		
		public override void Dispose() {
			font.Dispose();
			posAtlas.Dispose();
			game.Events.ChatFontChanged -= ChatFontChanged;
		}
		
		
		void DrawCounts() {
			SurvivalGameMode surv = (SurvivalGameMode)game.Mode;
			VertexP3fT2fC4b[] vertices = game.ModelCache.vertices;
			int index = 0;
			posAtlas.tex.Y = (short)(game.Height - barHeight);
			
			int offset = game.Inventory.Offset;
			for (int i = 0; i < Inventory.BlocksPerRow; i++) {
				int x = (int)(X + (elemSize + borderSize) * i);
				posAtlas.curX = x;
				if (surv.invCount[offset + i] > 1)
					posAtlas.AddInt(surv.invCount[offset + i], vertices, ref index);
			}

			gfx.BindTexture(posAtlas.tex.ID);
			gfx.UpdateDynamicIndexedVb(DrawMode.Triangles,
			                           game.ModelCache.vb, game.ModelCache.vertices, index);
		}
		
		void DrawHearts() {
			Model.ModelCache cache = game.ModelCache;
			int index = 0, health = game.LocalPlayer.Health;
			int inner = (int)(7 * game.GuiHotbarScale);
			int middle = (int)(8 * game.GuiHotbarScale);
			int outer = (int)(9 * game.GuiHotbarScale);
			
			int selBlockSize = (int)(23 * game.GuiHotbarScale);
			int offset = middle - inner;
			int y = game.Height - selBlockSize - outer;
			
			for (int heart = 0; heart < 10; heart++) {
				Texture tex = new Texture(0, X + middle * heart, y, outer, outer, backRec);
				IGraphicsApi.Make2DQuad(ref tex, FastColour.WhitePacked, cache.vertices, ref index);
				if (health <= 0) continue;
				
				TextureRec rec = (health >= 2) ? fullRec : halfRec;
				tex = new Texture(0, X + middle * heart + offset, y + offset, inner, inner, rec);
				IGraphicsApi.Make2DQuad(ref tex, FastColour.WhitePacked, cache.vertices, ref index);
				health -= 2;
			}
			
			gfx.BindTexture(game.Gui.IconsTex);
			gfx.UpdateDynamicIndexedVb(DrawMode.Triangles, cache.vb, cache.vertices, index);
		}
		
		static TextureRec backRec = new TextureRec(16 / 256f, 0 / 256f, 9 / 256f, 9 / 256f);
		static TextureRec fullRec = new TextureRec(53 / 256f, 1 / 256f, 7 / 256f, 7 / 256f);
		static TextureRec halfRec = new TextureRec(62 / 256f, 1 / 256f, 7 / 256f, 7 / 256f);
	}
}