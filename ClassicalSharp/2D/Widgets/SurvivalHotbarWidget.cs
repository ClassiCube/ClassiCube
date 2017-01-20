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
			posAtlas = new TextAtlas(game);
			posAtlas.Pack("0123456789", font, "f");
			game.Events.ChatFontChanged += ChatFontChanged;
			
			//float scale = 2 * game.GuiHotbarScale;
			//Width = (int)(9 * 10);// * scale);
			//Height = (int)9;// * scale);
			
			//X = game.Width / 2 - Width / 2;
			//Y = game.Height - Height - 100;
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
			
			for (int i = 0; i < hotbarCount; i++) {
				int x = (int)(X + (elemSize + borderSize) * i);
				posAtlas.curX = x;
				if (surv.invCount[i] > 1)
					posAtlas.AddInt(surv.invCount[i], vertices, ref index);
			}

			gfx.BindTexture(posAtlas.tex.ID);
			gfx.UpdateDynamicIndexedVb(DrawMode.Triangles,
			                           game.ModelCache.vb, game.ModelCache.vertices, index);
		}
		
		void DrawHearts() {
			Model.ModelCache cache = game.ModelCache;
			int index = 0, health = game.LocalPlayer.Health;
			for (int heart = 0; heart < 10; heart++) {
				Texture tex = new Texture(0, X + 16 * heart, Y - 18, 18, 18, backRec);
				IGraphicsApi.Make2DQuad(ref tex, FastColour.WhitePacked,
				                        cache.vertices, ref index);
				
				if (health >= 2) {
					tex = new Texture(0, X + 16 * heart + 2, Y - 18 + 2, 14, 14, fullRec);
				} else if (health == 1) {
					tex = new Texture(0, X + 16 * heart + 2, Y - 18 + 2, 14, 14, halfRec);
				} else {
					continue;
				}
				
				IGraphicsApi.Make2DQuad(ref tex, FastColour.WhitePacked,
				                        cache.vertices, ref index);
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