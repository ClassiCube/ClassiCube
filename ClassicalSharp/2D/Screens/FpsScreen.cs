// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using ClassicalSharp.Entities;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Gui.Widgets;
#if ANDROID
using Android.Graphics;
#endif

namespace ClassicalSharp.Gui.Screens {
	public class FpsScreen : Screen, IGameComponent {
		
		Font font, posFont;
		StringBuffer text;
		
		public FpsScreen(Game game) : base(game) {
			text = new StringBuffer(128);
		}

		public void Init(Game game) { }
		public void Ready(Game game) { Init(); }
		public void Reset(Game game) { }
		public void OnNewMap(Game game) { }
		public void OnNewMapLoaded(Game game) { }
		
		TextWidget fpsText, hackStates;
		TextAtlas posAtlas;
		public override void Render(double delta) {
			UpdateFPS(delta);
			if (game.HideGui || !game.ShowFPS) return;
			
			gfx.Texturing = true;
			fpsText.Render(delta);
			if (!game.ClassicMode && game.Gui.activeScreen == null) {
				UpdateHackState(false);
				DrawPosition();
				hackStates.Render(delta);
			}
			gfx.Texturing = false;
		}
		
		double accumulator;
		int frames, totalSeconds;
		
		void UpdateFPS(double delta) {
			frames++;
			accumulator += delta;
			if (accumulator < 1) return;
			
			int index = 0;
			totalSeconds++;
			int fps = (int)(frames / accumulator);
			
			text.Clear()
				.AppendNum(ref index, fps).Append(ref index, " fps, ");
			if (game.ClassicMode) {
				text.AppendNum(ref index, game.ChunkUpdates).Append(ref index, " chunk updates");
			} else {
				text.AppendNum(ref index, game.ChunkUpdates).Append(ref index, " chunks/s, ")
					.AppendNum(ref index, game.Vertices).Append(ref index, " vertices");
			}
			
			string textString = text.ToString();
			fpsText.SetText(textString);
			accumulator = 0;
			frames = 0;
			game.ChunkUpdates = 0;
		}
		
		string Q(int value) { return value == 1 ? "" : "s"; }
		
		public override void Init() {
			font = new Font(game.FontName, 16);
			posFont = new Font(game.FontName, 16, FontStyle.Italic);
			ContextRecreated();
			
			game.Events.ChatFontChanged += ChatFontChanged;
			gfx.ContextLost += ContextLost;
			gfx.ContextRecreated += ContextRecreated;
		}
		
		protected override void ContextLost() {
			fpsText.Dispose();
			posAtlas.Dispose();
			hackStates.Dispose();
		}
		
		protected override void ContextRecreated() {
			fpsText = new TextWidget(game, font)
				.SetLocation(Anchor.LeftOrTop, Anchor.LeftOrTop, 2, 2);
			fpsText.ReducePadding = true;
			fpsText.Init();
			string msg = text.Length > 0 ? text.ToString() : "FPS: no data yet";
			fpsText.SetText(msg);
			
			posAtlas = new TextAtlas(game);
			posAtlas.Pack("0123456789-, ()", posFont, "Feet pos: ");
			posAtlas.tex.Y = (short)(fpsText.Height + 2);
			
			int yOffset = fpsText.Height + posAtlas.tex.Height + 2;
			hackStates = new TextWidget(game, posFont)
				.SetLocation(Anchor.LeftOrTop, Anchor.LeftOrTop, 2, yOffset);
			hackStates.ReducePadding = true;
			hackStates.Init();
			UpdateHackState(true);
		}
		
		public override void Dispose() {
			font.Dispose();
			posFont.Dispose();
			ContextLost();
			
			game.Events.ChatFontChanged -= ChatFontChanged;
			gfx.ContextLost -= ContextLost;
			gfx.ContextRecreated -= ContextRecreated;
		}
		
		void ChatFontChanged(object sender, EventArgs e) { Recreate(); }
		
		public override void OnResize(int width, int height) { }
		
		void DrawPosition() {
			int index = 0;
			Texture tex = posAtlas.tex;
			tex.X1 = 2; tex.Width = (short)posAtlas.offset;
			IGraphicsApi.Make2DQuad(ref tex, FastColour.WhitePacked, 
			                        game.ModelCache.vertices, ref index);
			
			Vector3I pos = Vector3I.Floor(game.LocalPlayer.Position);
			posAtlas.curX = posAtlas.offset + 2;
			VertexP3fT2fC4b[] vertices = game.ModelCache.vertices;
			
			posAtlas.Add(13, vertices, ref index);
			posAtlas.AddInt(pos.X, vertices, ref index);
			posAtlas.Add(11, vertices, ref index);
			posAtlas.AddInt(pos.Y, vertices, ref index);
			posAtlas.Add(11, vertices, ref index);
			posAtlas.AddInt(pos.Z, vertices, ref index);
			posAtlas.Add(14, vertices, ref index);
			
			gfx.BindTexture(posAtlas.tex.ID);
			gfx.UpdateDynamicIndexedVb(DrawMode.Triangles,
			                           game.ModelCache.vb, game.ModelCache.vertices, index);
		}
		
		bool speeding, halfSpeeding, noclip, fly;
		int lastFov;
		void UpdateHackState(bool force) {
			HacksComponent hacks = game.LocalPlayer.Hacks;
			if (force || hacks.Speeding != speeding || hacks.HalfSpeeding != halfSpeeding || hacks.Noclip != noclip ||
			   hacks.Flying != fly || game.Fov != lastFov) {
				speeding = hacks.Speeding; halfSpeeding = hacks.HalfSpeeding; noclip = hacks.Noclip; fly = hacks.Flying;
				lastFov = game.Fov;
				int index = 0;
				text.Clear();
				
				if (game.Fov != game.DefaultFov) text.Append(ref index, "Zoom fov ")
					.AppendNum(ref index, lastFov).Append(ref index, "  ");
				if (fly) text.Append(ref index, "Fly ON   ");
				
				bool speed = (speeding || halfSpeeding) &&
					(hacks.CanSpeed || hacks.MaxSpeedMultiplier > 1);
				if (speed) text.Append(ref index, "Speed ON   ");
				if (noclip) text.Append(ref index, "Noclip ON   ");
				hackStates.SetText(text.ToString());
			}
		}
	}
}
