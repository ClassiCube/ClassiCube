// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Gui.Widgets;
using ClassicalSharp.Textures;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	
	public class TexIdsOverlay : Overlay {
		
		TextAtlas idAtlas;
		public TexIdsOverlay(Game game) : base(game) { widgets = new Widget[1]; }
		const int verticesCount = Atlas2D.TilesPerRow * Atlas2D.MaxRowsCount * 4;
		static VertexP3fT2fC4b[] vertices;
		int dynamicVb;
		int xOffset, yOffset, tileSize, baseTexLoc;
		
		public override void Init() {
			textFont = new Font(game.FontName, 8);
			base.Init();
			if (vertices == null) {
				vertices = new VertexP3fT2fC4b[verticesCount];
			}
		}
		
		public override void Render(double delta) {
			RenderMenuBounds();
			game.Graphics.Texturing = true;
			game.Graphics.SetBatchFormat(VertexFormat.P3fT2fC4b);
			RenderWidgets(widgets, delta);
			
			int rows = Atlas2D.RowsCount, origXOffset = xOffset;
			while (rows > 0) {
				RenderTerrain();
				RenderText();
				rows -= Atlas2D.TilesPerRow;
				
				xOffset    += tileSize            * Atlas2D.TilesPerRow;
				baseTexLoc += Atlas2D.TilesPerRow * Atlas2D.TilesPerRow;
			}
			
			baseTexLoc = 0;
			xOffset = origXOffset;
			game.Graphics.Texturing = false;
		}
		
		protected override void ContextLost() {
			base.ContextLost();
			game.Graphics.DeleteVb(ref dynamicVb);
			idAtlas.Dispose();
		}
		
		protected override void ContextRecreated() {
			dynamicVb = game.Graphics.CreateDynamicVb(VertexFormat.P3fT2fC4b, verticesCount);
			idAtlas = new TextAtlas(game, 16);
			idAtlas.Pack("0123456789", textFont, "f");
			
			tileSize = game.Height / Atlas2D.TilesPerRow;
			tileSize = (tileSize / 8) * 8;
			Utils.Clamp(ref tileSize, 8, 40);
			
			xOffset = CalcPos(Anchor.Centre, 0, tileSize * Atlas2D.RowsCount,   game.Width);
			yOffset = CalcPos(Anchor.Centre, 0, tileSize * Atlas2D.TilesPerRow, game.Height);

			widgets[0] = TextWidget.Create(game, "Texture ID reference sheet", titleFont)
				.SetLocation(Anchor.Centre, Anchor.Min, 0, yOffset - 30);
		}
		
		void RenderTerrain() {
			for (int i = 0; i < Atlas2D.TilesPerRow * Atlas2D.TilesPerRow;) {
				int index = 0, texIdx = 0;
				
				for (int j = 0; j < Atlas1D.TilesPerAtlas; j++) {
					TextureRec rec = Atlas1D.GetTexRec((i + j) + baseTexLoc, 1, out texIdx);
					int x = (i + j) % Atlas2D.TilesPerRow;
					int y = (i + j) / Atlas2D.TilesPerRow;
					
					Texture tex = new Texture(0, xOffset + x * tileSize, yOffset + y * tileSize,
					                          tileSize, tileSize, rec);
					IGraphicsApi.Make2DQuad(ref tex, PackedCol.White, vertices, ref index);
				}
				i += Atlas1D.TilesPerAtlas;
				
				game.Graphics.BindTexture(Atlas1D.TexIds[texIdx]);
				game.Graphics.UpdateDynamicVb_IndexedTris(dynamicVb, vertices, index);
			}
		}
		
		const int textOffset = 3;
		void RenderText() {
			int index = 0;
			idAtlas.tex.Y = (short)(yOffset + (tileSize - idAtlas.tex.Height));
			
			for (int y = 0; y < Atlas2D.TilesPerRow; y++) {
				for (int x = 0; x < Atlas2D.TilesPerRow; x++) {
					idAtlas.curX = xOffset + tileSize * x + textOffset;
					int id = x + y * Atlas2D.TilesPerRow;
					idAtlas.AddInt(id + baseTexLoc, vertices, ref index);
				}
				idAtlas.tex.Y += (short)tileSize;
				
				if ((y % 4) != 3) continue;
				game.Graphics.BindTexture(idAtlas.tex.ID);
				game.Graphics.UpdateDynamicVb_IndexedTris(dynamicVb, vertices, index);
				index = 0;
			}
		}
		
		public override bool HandlesKeyDown(Key key) {
			if (key == game.Mapping(KeyBind.IDOverlay) || key == game.Mapping(KeyBind.PauseOrExit)) {
				game.Gui.DisposeOverlay(this);
				return true;
			}
			return game.Gui.UnderlyingScreen.HandlesKeyDown(key);
		}
		
		public override bool HandlesKeyPress(char key) {
			return game.Gui.UnderlyingScreen.HandlesKeyPress(key);
		}
		
		public override bool HandlesKeyUp(Key key) {
			return game.Gui.UnderlyingScreen.HandlesKeyUp(key);
		}
	}
}