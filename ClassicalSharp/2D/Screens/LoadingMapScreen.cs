// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using ClassicalSharp.Events;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Gui.Widgets;
using ClassicalSharp.Model;
using ClassicalSharp.Textures;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	public class LoadingMapScreen : Screen {
		
		readonly Font font;
		public LoadingMapScreen(Game game, string title, string message) : base(game) {
			this.title = title;
			this.message = message;
			font = new Font(game.FontName, 16);			
			BlocksWorld = true;
			RenderHudOver = true;
			HandlesAllInput = true;
		}
		
		string title, message;
		float progress;
		TextWidget titleWidget, messageWidget;
		const int progWidth = 220, progHeight = 10;
		readonly FastColour backCol = new FastColour(128, 128, 128);
		readonly FastColour progressCol = new FastColour(128, 255, 128);

		
		public override void Init() {
			gfx.Fog = false;
			ContextRecreated();
			
			game.WorldEvents.MapLoading += MapLoading;
			gfx.ContextLost += ContextLost;
			gfx.ContextRecreated += ContextRecreated;
		}
		
		public void SetTitle(string title) {
			this.title = title;
			if (titleWidget != null) titleWidget.Dispose();
			
			titleWidget = TextWidget.Create(game, title, font)
				.SetLocation(Anchor.Centre, Anchor.Centre, 0, -80);
		}
		
		public void SetMessage(string message) {
			this.message = message;
			if (messageWidget != null) messageWidget.Dispose();
			
			messageWidget = TextWidget.Create(game, message, font)
				.SetLocation(Anchor.Centre, Anchor.Centre, 0, -30);
		}
		
		public void SetProgress(float progress) {
			this.progress = progress;
		}

		void MapLoading(object sender, MapLoadingEventArgs e) {
			progress = e.Progress;
		}
		
		public override void Dispose() {
			font.Dispose();
			ContextLost();
			
			game.WorldEvents.MapLoading -= MapLoading;
			gfx.ContextLost -= ContextLost;
			gfx.ContextRecreated -= ContextRecreated;
		}
		
		public override void OnResize(int width, int height) {
			messageWidget.CalculatePosition();
			titleWidget.CalculatePosition();
		}
		
		protected override void ContextLost() {
			if (messageWidget == null) return;
			messageWidget.Dispose();
			titleWidget.Dispose();
		}
		
		protected override void ContextRecreated() {
			if (gfx.LostContext) return;
			SetTitle(title);
			SetMessage(message);
		}
		
		
		public override bool HandlesKeyDown(Key key) {
			if (key == Key.Tab) return true;
			return game.Gui.hudScreen.HandlesKeyDown(key);
		}
		
		public override bool HandlesKeyPress(char key)  {
			return game.Gui.hudScreen.HandlesKeyPress(key);
		}
		
		public override bool HandlesKeyUp(Key key) {
			if (key == Key.Tab) return true;
			return game.Gui.hudScreen.HandlesKeyUp(key);
		}
		
		public override bool HandlesMouseClick(int mouseX, int mouseY, MouseButton button) { return true; }
		
		public override bool HandlesMouseMove(int mouseX, int mouseY) { return true; }
		
		public override bool HandlesMouseScroll(int delta)  { return true; }
		
		public override bool HandlesMouseUp(int mouseX, int mouseY, MouseButton button) { return true; }
		
		
		public override void Render(double delta) {
			gfx.Texturing = true;
			DrawBackground();
			titleWidget.Render(delta);
			messageWidget.Render(delta);
			gfx.Texturing = false;
			
			int progX = game.Width / 2 - progWidth / 2;
			int progY = game.Height / 2 - progHeight / 2;
			gfx.Draw2DQuad(progX, progY, progWidth, progHeight, backCol);
			gfx.Draw2DQuad(progX, progY, progWidth * progress, progHeight, progressCol);
		}
		
		void DrawBackground() {
			VertexP3fT2fC4b[] vertices = game.ModelCache.vertices;
			int index = 0, atlasIndex = 0;
			int drawnY = 0, height = game.Height;
			int col = new FastColour(64, 64, 64).Pack();
			
			int texLoc = game.BlockInfo.GetTextureLoc(Block.Dirt, Side.Top);
			TerrainAtlas1D atlas = game.TerrainAtlas1D;
			Texture tex = new Texture(0, 0, 0, game.Width, 64, 
			                          atlas.GetTexRec(texLoc, 1, out atlasIndex));
			tex.U2 = (float)game.Width / 64;
			bool bound = false;
			
			while (drawnY < height) {
				tex.Y1 = drawnY;
				IGraphicsApi.Make2DQuad(ref tex, col, vertices, ref index);
				if (index >= vertices.Length)
					DrawBackgroundVertices(ref index, atlasIndex, ref bound);
				drawnY += 64;
			}
			DrawBackgroundVertices(ref index, atlasIndex, ref bound);
		}
		
		void DrawBackgroundVertices(ref int index, int atlasIndex, ref bool bound) {
			if (index == 0) return;
			if (!bound) {
				bound = true;
				gfx.BindTexture(game.TerrainAtlas1D.TexIds[atlasIndex]);
			}
					
			ModelCache cache = game.ModelCache;
			gfx.SetBatchFormat(VertexFormat.P3fT2fC4b);
			gfx.UpdateDynamicVb_IndexedTris(cache.vb, cache.vertices, index);
			index = 0;
		}
	}
}
