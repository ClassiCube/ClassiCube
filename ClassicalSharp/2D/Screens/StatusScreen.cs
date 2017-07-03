// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using ClassicalSharp.Entities;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Gui.Widgets;
using ClassicalSharp.Network;
#if ANDROID
using Android.Graphics;
#endif

namespace ClassicalSharp.Gui.Screens {
	public class StatusScreen : Screen, IGameComponent {
		
		Font font;
		StringBuffer statusBuffer;
		
		public StatusScreen(Game game) : base(game) {
			statusBuffer = new StringBuffer(128);
		}

		public void Init(Game game) { }
		public void Ready(Game game) { Init(); }
		public void Reset(Game game) { }
		public void OnNewMap(Game game) { }
		public void OnNewMapLoaded(Game game) { }
		
		TextWidget status, hackStates;
		TextAtlas posAtlas;
		public override void Render(double delta) {
			UpdateStatus(delta);
			if (game.HideGui || !game.ShowFPS) return;
			
			gfx.Texturing = true;
			status.Render(delta);
			if (!game.ClassicMode && game.Gui.activeScreen == null) {
				UpdateHackState(false);
				DrawPosition();
				hackStates.Render(delta);
			}
			gfx.Texturing = false;
		}
		
		double accumulator;
		int frames, totalSeconds;
		
		void UpdateStatus(double delta) {
			frames++;
			accumulator += delta;
			if (accumulator < 1) return;
			
			int index = 0;
			totalSeconds++;
			int fps = (int)(frames / accumulator);
			
			statusBuffer.Clear()
				.AppendNum(ref index, fps).Append(ref index, " fps, ");
			if (game.ClassicMode) {
				statusBuffer.AppendNum(ref index, game.ChunkUpdates).Append(ref index, " chunk updates");
			} else {
				statusBuffer.AppendNum(ref index, game.ChunkUpdates).Append(ref index, " chunks/s, ")
					.AppendNum(ref index, game.Vertices).Append(ref index, " vertices");
				
				int ping = PingList.AveragePingMilliseconds();
				if (ping != 0) {
					statusBuffer.Append(ref index, ", ping ").AppendNum(ref index, ping).Append(ref index, " ms");
				}
			}
			
			status.SetText(statusBuffer.ToString());
			accumulator = 0;
			frames = 0;
			game.ChunkUpdates = 0;
		}
		
		public override void Init() {
			font = new Font(game.FontName, 16);
			ContextRecreated();
			
			game.Events.ChatFontChanged += ChatFontChanged;
			gfx.ContextLost += ContextLost;
			gfx.ContextRecreated += ContextRecreated;
		}
		
		protected override void ContextLost() {
			status.Dispose();
			posAtlas.Dispose();
			hackStates.Dispose();
		}
		
		protected override void ContextRecreated() {
			status = new TextWidget(game, font)
				.SetLocation(Anchor.LeftOrTop, Anchor.LeftOrTop, 2, 2);
			status.ReducePadding = true;
			status.Init();
			string msg = statusBuffer.Length > 0 ? statusBuffer.ToString() : "FPS: no data yet";
			status.SetText(msg);
			
			posAtlas = new TextAtlas(game, 16);
			posAtlas.Pack("0123456789-, ()", font, "Position: ");
			posAtlas.tex.Y = (short)(status.Height + 2);
			
			int yOffset = status.Height + posAtlas.tex.Height + 2;
			hackStates = new TextWidget(game, font)
				.SetLocation(Anchor.LeftOrTop, Anchor.LeftOrTop, 2, yOffset);
			hackStates.ReducePadding = true;
			hackStates.Init();
			UpdateHackState(true);
		}
		
		public override void Dispose() {
			font.Dispose();
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
			gfx.UpdateDynamicVb_IndexedTris(game.ModelCache.vb, game.ModelCache.vertices, index);
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
				statusBuffer.Clear();
				
				if (game.Fov != game.DefaultFov) statusBuffer.Append(ref index, "Zoom fov ")
					.AppendNum(ref index, lastFov).Append(ref index, "  ");
				if (fly) statusBuffer.Append(ref index, "Fly ON   ");
				
				bool speed = (speeding || halfSpeeding) &&
					(hacks.CanSpeed || hacks.MaxSpeedMultiplier > 1);
				if (speed) statusBuffer.Append(ref index, "Speed ON   ");
				if (noclip) statusBuffer.Append(ref index, "Noclip ON   ");
				hackStates.SetText(statusBuffer.ToString());
			}
		}
	}
}
