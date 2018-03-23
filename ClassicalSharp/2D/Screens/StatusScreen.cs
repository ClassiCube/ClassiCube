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
			game.Graphics.Texturing = true;
			status.Render(delta);
			
			if (!game.ClassicMode && game.Gui.activeScreen == null) {
				if (HacksChanged()) { UpdateHackState(); }
				DrawPosition();
				hackStates.Render(delta);
			}
			game.Graphics.Texturing = false;
		}
		
		double accumulator;
		int frames;
		
		void UpdateStatus(double delta) {
			frames++;
			accumulator += delta;
			if (accumulator < 1) return;
			
			int fps = (int)(frames / accumulator);
			statusBuffer.Clear().AppendNum(fps).Append(" fps, ");
			
			if (game.ClassicMode) {
				statusBuffer.AppendNum(game.ChunkUpdates).Append(" chunk updates");
			} else {
				if (game.ChunkUpdates > 0) {
					statusBuffer.AppendNum(game.ChunkUpdates).Append(" chunks/s, ");
				}
				int indices = (game.Vertices >> 2) * 6;
				statusBuffer.AppendNum(indices).Append(" vertices");
				
				int ping = PingList.AveragePingMilliseconds();
				if (ping != 0) {
					statusBuffer.Append(", ping ").AppendNum(ping).Append(" ms");
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
			game.Graphics.ContextLost += ContextLost;
			game.Graphics.ContextRecreated += ContextRecreated;
		}
		
		protected override void ContextLost() {
			status.Dispose();
			posAtlas.Dispose();
			hackStates.Dispose();
		}
		
		protected override void ContextRecreated() {
			status = new TextWidget(game, font)
				.SetLocation(Anchor.Min, Anchor.Min, 2, 2);
			status.ReducePadding = true;
			status.Init();
			string msg = statusBuffer.Length > 0 ? statusBuffer.ToString() : "FPS: no data yet";
			status.SetText(msg);
			
			posAtlas = new TextAtlas(game, 16);
			posAtlas.Pack("0123456789-, ()", font, "Position: ");
			posAtlas.tex.Y = (short)(status.Height + 2);
			
			int yOffset = status.Height + posAtlas.tex.Height + 2;
			hackStates = new TextWidget(game, font)
				.SetLocation(Anchor.Min, Anchor.Min, 2, yOffset);
			hackStates.ReducePadding = true;
			hackStates.Init();
			UpdateHackState();
		}
		
		public override void Dispose() {
			font.Dispose();
			ContextLost();
			
			game.Events.ChatFontChanged -= ChatFontChanged;
			game.Graphics.ContextLost -= ContextLost;
			game.Graphics.ContextRecreated -= ContextRecreated;
		}
		
		void ChatFontChanged(object sender, EventArgs e) { Recreate(); }
		
		public override void OnResize(int width, int height) { }
		
		void DrawPosition() {
			int index = 0;
			Texture tex = posAtlas.tex;
			tex.X1 = 2; tex.Width = (ushort)posAtlas.offset;
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
			
			game.Graphics.BindTexture(posAtlas.tex.ID);
			game.Graphics.UpdateDynamicVb_IndexedTris(game.ModelCache.vb, game.ModelCache.vertices, index);
		}
		
		bool speed, halfSpeed, noclip, fly, canSpeed;
		int lastFov;
		bool HacksChanged()  {
			HacksComponent hacks = game.LocalPlayer.Hacks;
			return hacks.Speeding != speed || hacks.HalfSpeeding != halfSpeed || hacks.Flying != fly
				|| hacks.Noclip != noclip || game.Fov != lastFov || hacks.CanSpeed != canSpeed;
		}
		
		void UpdateHackState() {
			HacksComponent hacks = game.LocalPlayer.Hacks;
			speed = hacks.Speeding; halfSpeed = hacks.HalfSpeeding; fly = hacks.Flying; 
			noclip = hacks.Noclip; lastFov = game.Fov; canSpeed = hacks.CanSpeed;
			
			statusBuffer.Clear();
			if (game.Fov != game.DefaultFov) statusBuffer.Append("Zoom fov ").AppendNum(lastFov).Append("  ");
			if (fly) statusBuffer.Append("Fly ON   ");
			
			bool speeding = (speed || halfSpeed) && (hacks.CanSpeed || hacks.BaseHorSpeed > 1);
			if (speeding) statusBuffer.Append("Speed ON   ");
			if (noclip) statusBuffer.Append("Noclip ON   ");
			hackStates.SetText(statusBuffer.ToString());
		}
	}
}
