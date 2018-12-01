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
		TextWidget line1, line2;
		TextAtlas posAtlas;
		
		public StatusScreen(Game game) : base(game) {
			statusBuffer = new StringBuffer(128);
			HandlesAllInput = false;
		}

		void IGameComponent.Init(Game game) { }
		void IGameComponent.Ready(Game game) { Init(); }
		void IGameComponent.Reset(Game game) { }
		void IGameComponent.OnNewMap(Game game) { }
		void IGameComponent.OnNewMapLoaded(Game game) { }
		
		public override void Init() {
			font = new Font(game.FontName, 16);
			ContextRecreated();
			
			Events.ChatFontChanged  += ChatFontChanged;
			Events.ContextLost      += ContextLost;
			Events.ContextRecreated += ContextRecreated;
		}
		
		public override void Render(double delta) {
			UpdateStatus(delta);
			if (game.HideGui) return;
			
			game.Graphics.Texturing = true;
			if (game.ShowFPS) line1.Render(delta);
			
			if (game.ClassicMode) {
				line2.Render(delta);
			} else if (game.Gui.activeScreen == null && game.ShowFPS) {
				if (HacksChanged()) { UpdateHackState(); }
				DrawPosition();
				line2.Render(delta);
			}
			game.Graphics.Texturing = false;
		}
		
		public override void Dispose() {
			font.Dispose();
			ContextLost();
			
			Events.ChatFontChanged  -= ChatFontChanged;
			Events.ContextLost      -= ContextLost;
			Events.ContextRecreated -= ContextRecreated;
		}
		public override void OnResize() { }
		
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
			
			line1.Set(statusBuffer.ToString(), font);
			accumulator = 0;
			frames = 0;
			game.ChunkUpdates = 0;
		}
		
		protected override void ContextLost() {
			line1.Dispose();
			posAtlas.Dispose();
			line2.Dispose();
		}
		
		protected override void ContextRecreated() {
			line1 = new TextWidget(game)
				.SetLocation(Anchor.Min, Anchor.Min, 2, 2);
			line1.ReducePadding = true;
			string msg = statusBuffer.Length > 0 ? statusBuffer.ToString() : "FPS: no data yet";
			line1.Set(msg, font);
			
			posAtlas = new TextAtlas(game, 16);
			posAtlas.Pack("0123456789-, ()", font, "Position: ");
			posAtlas.tex.Y = (short)(line1.Height + 2);
			
			int yOffset = line1.Height + posAtlas.tex.Height + 2;
			line2 = new TextWidget(game)
				.SetLocation(Anchor.Min, Anchor.Min, 2, yOffset);
			line2.ReducePadding = true;
			
			if (game.ClassicMode) {
				// Swap around so 0.30 version is at top
				line2.YOffset = 2;
				line1.YOffset = posAtlas.tex.Y;
				line2.Set("0.30", font);
				
				line1.Reposition();
				line2.Reposition();
			} else {
				UpdateHackState();
			}
		}
		
		void ChatFontChanged() { Recreate(); }
		
		void DrawPosition() {
			int index = 0;
			Texture tex = posAtlas.tex;
			tex.X1 = 2; tex.Width = (ushort)posAtlas.offset;
			IGraphicsApi.Make2DQuad(ref tex, PackedCol.White,
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
			
			bool speeding = (speed || halfSpeed) && hacks.CanSpeed;
			if (speeding) statusBuffer.Append("Speed ON   ");
			if (noclip)   statusBuffer.Append("Noclip ON   ");
			line2.Set(statusBuffer.ToString(), font);
		}
	}
}
