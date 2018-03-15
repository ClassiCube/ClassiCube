// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Gui.Widgets;
using ClassicalSharp.Textures;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	
	public sealed class TexIdsOverlay : Overlay {
		
		TextAtlas idAtlas;
		public TexIdsOverlay(Game game) : base(game) { }
		const int verticesCount = TerrainAtlas2D.TilesPerRow * TerrainAtlas2D.RowsCount * 4;
		static VertexP3fT2fC4b[] vertices;
		int dynamicVb;
		
		public override void Init() {
			base.Init();
			if (vertices == null) {
				vertices = new VertexP3fT2fC4b[verticesCount];
			}
			regularFont.Dispose();
			regularFont = new Font(game.FontName, 8);
			ContextRecreated();
		}
		
		const int textOffset = 3;
		int xOffset, yOffset, tileSize;
		
		void UpdateTileSize() {
			tileSize = game.window.Height / TerrainAtlas2D.RowsCount;
			tileSize = (tileSize / 8) * 8;
			Utils.Clamp(ref tileSize, 8, 40);
		}
		
		public override void Render(double delta) {
			RenderMenuBounds();
			game.Graphics.Texturing = true;
			game.Graphics.SetBatchFormat(VertexFormat.P3fT2fC4b);
			RenderWidgets(widgets, delta);
			RenderTerrain();
			RenderTextOverlay();
			game.Graphics.Texturing = false;
		}
		
		protected override void ContextLost() {
			base.ContextLost();
			game.Graphics.DeleteVb(ref dynamicVb);
			idAtlas.Dispose();
		}
		
		protected override void ContextRecreated() {
			base.ContextRecreated();
			dynamicVb = game.Graphics.CreateDynamicVb(VertexFormat.P3fT2fC4b, verticesCount);
			idAtlas = new TextAtlas(game, 16);
			idAtlas.Pack("0123456789", regularFont, "f");
			UpdateTileSize();
		}
		
		void RenderTerrain() {
			int elementsPerAtlas = TerrainAtlas1D.elementsPerAtlas1D;
			for (int i = 0; i < TerrainAtlas2D.TilesPerRow * TerrainAtlas2D.RowsCount;) {
				int index = 0, texIdx = i / elementsPerAtlas, ignored;
				
				for (int j = 0; j < elementsPerAtlas; j++) {
					TextureRec rec = TerrainAtlas1D.GetTexRec(i + j, 1, out ignored);
					int x = (i + j) % TerrainAtlas2D.TilesPerRow;
					int y = (i + j) / TerrainAtlas2D.TilesPerRow;
					
					Texture tex = new Texture(0, xOffset + x * tileSize, yOffset + y * tileSize,
					                          tileSize, tileSize, rec);
					IGraphicsApi.Make2DQuad(ref tex, FastColour.WhitePacked, vertices, ref index);
				}
				i += elementsPerAtlas;
				
				game.Graphics.BindTexture(TerrainAtlas1D.TexIds[texIdx]);
				game.Graphics.UpdateDynamicVb_IndexedTris(dynamicVb, vertices, index);
			}
		}
		
		void RenderTextOverlay() {
			int index = 0;
			idAtlas.tex.Y = (short)(yOffset + (tileSize - idAtlas.tex.Height));
			
			for (int y = 1; y <= TerrainAtlas2D.RowsCount; y++) {
				for (int x = 0; x < TerrainAtlas2D.TilesPerRow; x++) {
					idAtlas.curX = xOffset + tileSize * x + textOffset;
					int id = x + ((y - 1) * TerrainAtlas2D.TilesPerRow);
					idAtlas.AddInt(id, vertices, ref index);
				}
				idAtlas.tex.Y += (short)tileSize;
				
				if ((y % 4) != 0) continue;				
				game.Graphics.BindTexture(idAtlas.tex.ID);
				game.Graphics.UpdateDynamicVb_IndexedTris(dynamicVb, vertices, index);
				index = 0;
			}
		}
		
		public override bool HandlesKeyDown(Key key) {
			if (key == Key.F10 || key == game.Input.Keys[KeyBind.PauseOrExit]) {
				Dispose();
				CloseOverlay();
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

		public override void RedrawText() { }
		
		public override void MakeButtons() {
			UpdateTileSize();
			xOffset = (game.Width / 2)  - (tileSize * TerrainAtlas2D.TilesPerRow) / 2;
			yOffset = (game.Height / 2) - (tileSize * TerrainAtlas2D.RowsCount)   / 2;
			
			DisposeWidgets(widgets);
			widgets = new Widget[1];
			widgets[0] = TextWidget.Create(game, "Texture ID reference sheet", titleFont)
				.SetLocation(Anchor.Centre, Anchor.LeftOrTop, 0, yOffset - 30);
		}
	}
}