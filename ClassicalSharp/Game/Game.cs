// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Net;
using System.Threading;
using ClassicalSharp.Audio;
using ClassicalSharp.Commands;
using ClassicalSharp.Entities;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Gui.Screens;
using ClassicalSharp.Map;
using ClassicalSharp.Model;
using ClassicalSharp.Network;
using ClassicalSharp.Particles;
using ClassicalSharp.Renderers;
using ClassicalSharp.Selections;
using ClassicalSharp.Textures;
using OpenTK;
using OpenTK.Graphics;
using OpenTK.Input;
using OpenTK.Platform;
#if ANDROID
using Android.Graphics;
#endif
using PathIO = System.IO.Path; // Android.Graphics.Path clash otherwise
using BlockID = System.UInt16;

namespace ClassicalSharp {

	public partial class Game : IDisposable {
		
		public Game(string username, string mppass, string skinServer, int width, int height) {
			string title = Program.AppName + " (" + username + ")";
			window = Factory.CreateWindow(width, height, title, GraphicsMode.Default, DisplayDevice.Default);
			window.Visible = true;
			Username = username;
			Mppass = mppass;
			this.skinServer = skinServer;
		}
		
		public bool ChangeTerrainAtlas(Bitmap atlas) {
			if (!ValidateBitmap("terrain.png", atlas)) return false;
			if (atlas.Height < atlas.Width) {
				Chat.Add("&cUnable to use terrain.png from the texture pack.");
				Chat.Add("&c Its height is less than its width.");
				return false;
			}
			if (Graphics.LostContext) return false;
			
			Atlas1D.Dispose();
			Atlas2D.Dispose();
			Atlas2D.UpdateState(atlas);
			Atlas1D.UpdateState();
			
			Events.RaiseTerrainAtlasChanged();
			return true;
		}
		
		Stopwatch render_watch = new Stopwatch();
		public void Run() {
			window.Closed += OnClosed;
			window.Resize += OnResize;
			
			OnLoad();
			OnResize();
			Utils.LogDebug("Entering main loop.");
			render_watch.Start();
			
			while (true) {
				window.ProcessEvents();
				if (window.Exists && !isExiting) {
					// Cap the maximum time drift to 1 second (e.g. when the process is suspended).
					double time = render_watch.Elapsed.TotalSeconds;
					if (time > 1.0) time = 1.0;
					if (time <= 0) continue;
					
					render_watch.Reset();
					render_watch.Start();
					RenderFrame(time);
				} else { return; }
			}
		}
		
		bool isExiting;
		public void Exit() {
			isExiting = true;
			// TODO: is isExiting right
			window.Close();
		}
		
		public void SetViewDistance(int distance) {
			distance = Math.Min(distance, MaxViewDistance);
			if (distance == ViewDistance) return;
			ViewDistance = distance;
			
			Events.RaiseViewDistanceChanged();
			UpdateProjection();
		}
		
		public void UserSetViewDistance(int distance) {
			UserViewDistance = distance;
			Options.Set(OptionsKey.ViewDist, distance);
			SetViewDistance(distance);
		}
		
		public ScheduledTask AddScheduledTask(double interval, ScheduledTaskCallback callback) {
			ScheduledTask task = new ScheduledTask();
			task.Interval = interval; task.Callback = callback;
			Tasks.Add(task);
			return task;
		}
		
		public void UpdateProjection() {
			DefaultFov = Options.GetInt(OptionsKey.FieldOfView, 1, 150, 70);
			Camera.GetProjection(out Graphics.Projection);
			
			Graphics.SetMatrixMode(MatrixType.Projection);
			Graphics.LoadMatrix(ref Graphics.Projection);
			Graphics.SetMatrixMode(MatrixType.Modelview);
			Events.RaiseProjectionChanged();
		}
		
		public void Disconnect(string title, string reason) {
			World.Reset();
			Events.RaiseOnNewMap();
			Gui.SetNewScreen(new DisconnectScreen(this, title, reason));
			
			IDrawer2D.InitCols();
			BlockInfo.Reset();
			TexturePack.ExtractDefault(this);

			for (int i = 0; i < Components.Count; i++) {
				Components[i].Reset(this);
			}
			GC.Collect();
		}
		
		public void CycleCamera() {
			if (ClassicMode) return;
			
			int i = Cameras.IndexOf(Camera);
			i = (i + 1) % Cameras.Count;
			Camera = Cameras[i];
			Camera.ResetRotOffset();
			
			if (!LocalPlayer.Hacks.CanUseThirdPersonCamera || !LocalPlayer.Hacks.Enabled)
				Camera = Cameras[0];
			UpdateProjection();
		}
		
		public void UpdateBlock(int x, int y, int z, BlockID block) {
			BlockID oldBlock = World.GetBlock(x, y, z);
			World.SetBlock(x, y, z, block);
			
			WeatherRenderer weather = WeatherRenderer;
			if (weather.heightmap != null)
				weather.OnBlockChanged(x, y, z, oldBlock, block);
			Lighting.OnBlockChanged(x, y, z, oldBlock, block);
			
			// Refresh the chunk the block was located in.
			int cx = x >> 4, cy = y >> 4, cz = z >> 4;
			MapRenderer.GetChunk(cx, cy, cz).AllAir &= BlockInfo.Draw[block] == DrawType.Gas;
			MapRenderer.RefreshChunk(cx, cy, cz);
		}

		public bool IsKeyDown(KeyBind binding) { return Input.IsKeyDown(binding); }
		
		public bool IsMousePressed(MouseButton button) { return Input.IsMousePressed(button); }
		
		public Key Mapping(KeyBind mapping) { return Input.Keys[mapping]; }
		
		public bool CanPick(BlockID block) {
			if (BlockInfo.Draw[block] == DrawType.Gas) return false;
			if (BlockInfo.Draw[block] == DrawType.Sprite) return true;
			
			if (BlockInfo.Collide[block] != CollideType.Liquid) return true;
			return BreakableLiquids && BlockInfo.CanPlace[block] && BlockInfo.CanDelete[block];
		}
		
		
		public bool LoadTexture(ref int texId, string file, byte[] data) {
			SkinType type = SkinType.Invalid;
			return UpdateTexture(ref texId, file, data, ref type);
		}

		public bool UpdateTexture(ref int texId, string file, byte[] data, ref SkinType type) {
			bool calc = type != SkinType.Invalid;
			using (Bitmap bmp = Platform.ReadBmp(Drawer2D, data)) {
				if (!ValidateBitmap(file, bmp)) return false;
				if (calc) type = Utils.GetSkinType(bmp);
				
				Graphics.DeleteTexture(ref texId);
				texId = Graphics.CreateTexture(bmp, true, false);
				return true;
			}
		}
		
		public bool ValidateBitmap(string file, Bitmap bmp) {
			if (bmp == null) {
				Chat.Add("&cError loading " + file + " from the texture pack.");
				return false;
			}
			
			int maxWidth = Graphics.MaxTexWidth, maxHeight = Graphics.MaxTexHeight;
			if (bmp.Width > maxWidth || bmp.Height > maxHeight) {
				Chat.Add("&cUnable to use " + file + " from the texture pack.");
				Chat.Add("&c Its size is (" + bmp.Width + "," + bmp.Height
				         + "), your GPU supports (" + maxWidth + "," + maxHeight + ") at most.");
				return false;
			}

			if (!Utils.IsPowerOf2(bmp.Width) || !Utils.IsPowerOf2(bmp.Height)) {
				Chat.Add("&cUnable to use " + file + " from the texture pack.");
				Chat.Add("&c Its size is (" + bmp.Width + "," + bmp.Height
				         + "), which is not power of two dimensions.");
				return false;
			}
			return true;
		}
		
		public int CalcRenderType(string type) {
			if (Utils.CaselessEq(type, "legacyfast")) {
				return 0x03;
			} else if (Utils.CaselessEq(type, "legacy")) {
				return 0x01;
			} else if (Utils.CaselessEq(type, "normal")) {
				return 0x00;
			} else if (Utils.CaselessEq(type, "normalfast")) {
				return 0x02;
			}
			return -1;
		}
		
		void UpdateClientSize() {
			Size size = window.ClientSize;
			Width  = Math.Max(size.Width,  1);
			Height = Math.Max(size.Height, 1);
		}
		
		void OnResize() {
			UpdateClientSize();
			Graphics.OnWindowResize(this);
			UpdateProjection();
			Gui.OnResize();
		}
		
		void OnClosed() { isExiting = true; }
		
		void OnNewMapCore() {
			for (int i = 0; i < Components.Count; i++)
				Components[i].OnNewMap(this);
		}
		
		void OnNewMapLoadedCore() {
			for (int i = 0; i < Components.Count; i++)
				Components[i].OnNewMapLoaded(this);
		}
		
		void TextureChangedCore(string name, byte[] data) {
			if (Utils.CaselessEq(name, "terrain.png")) {
				Bitmap atlas = Platform.ReadBmp(Drawer2D, data);
				if (ChangeTerrainAtlas(atlas)) return;
				atlas.Dispose();
			} else if (Utils.CaselessEq(name, "default.png")) {
				Bitmap bmp = Platform.ReadBmp(Drawer2D, data);
				Drawer2D.SetFontBitmap(bmp);
				Events.RaiseChatFontChanged();
			}
		}
		
		void OnLowVRAMDetected() {
			if (UserViewDistance <= 16) throw new OutOfMemoryException("Out of video memory!");
			UserViewDistance /= 2;
			UserViewDistance = Math.Max(16, UserViewDistance);

			ChunkUpdater.Refresh();
			SetViewDistance(UserViewDistance);
			Chat.Add("&cOut of VRAM! Halving view distance..");
		}

		Stopwatch frameTimer = new Stopwatch();
		internal float limitMillis;
		public void SetFpsLimit(FpsLimitMethod method) {
			FpsLimit    = method;
			limitMillis = CalcLimitMillis(method);
			Graphics.SetVSync(this, method == FpsLimitMethod.LimitVSync);
		}
		
		internal float CalcLimitMillis(FpsLimitMethod method) {
			if (method == FpsLimitMethod.Limit120FPS) return 1000f / 120;
			if (method == FpsLimitMethod.Limit60FPS)  return 1000f / 60;
			if (method == FpsLimitMethod.Limit30FPS)  return 1000f / 30;
			return 0;
		}
		
		void LimitFPS() {
			double elapsed = frameTimer.Elapsed.TotalMilliseconds;
			double leftOver = limitMillis - elapsed;
			if (leftOver > 0.001) // going faster than limit
				Thread.Sleep((int)Math.Round(leftOver, MidpointRounding.AwayFromZero));
		}
		
		internal void RenderFrame(double delta) {
			frameTimer.Reset();
			frameTimer.Start();
			
			Graphics.BeginFrame(this);
			Graphics.BindIb(Graphics.defaultIb);
			accumulator += delta;
			Vertices = 0;
			
			Camera.UpdateMouse();
			if (!window.Focused && !Gui.ActiveScreen.HandlesAllInput) {
				Gui.SetNewScreen(new PauseScreen(this));
			}
			
			bool allowZoom = Gui.activeScreen == null && !Gui.hudScreen.HandlesAllInput;
			if (allowZoom && IsKeyDown(KeyBind.ZoomScrolling)) {
				Input.SetFOV(ZoomFov, false);
			}
			
			DoScheduledTasks(delta);
			float t = (float)(entTask.Accumulator / entTask.Interval);
			LocalPlayer.SetInterpPosition(t);
			
			Graphics.Clear();
			CurrentCameraPos = Camera.GetPosition(t);
			UpdateViewMatrix();
			
			bool visible = Gui.activeScreen == null || !Gui.activeScreen.BlocksWorld;
			if (!World.HasBlocks) visible = false;
			if (visible) {
				Render3D(delta, t);
			} else {
				SelectedPos.SetAsInvalid();
			}
			
			Gui.Render(delta);
			if (screenshotRequested) TakeScreenshot();
			
			Graphics.EndFrame(this);
			if (limitMillis != 0) LimitFPS();
		}
		
		void UpdateViewMatrix() {
			Graphics.SetMatrixMode(MatrixType.Modelview);
			Camera.GetView(out Graphics.View);
			Graphics.LoadMatrix(ref Graphics.View);
			Culling.CalcFrustumEquations(ref Graphics.Projection, ref Graphics.View);
		}
		
		void Render3D(double delta, float t) {
			if (SkyboxRenderer.ShouldRender)
				SkyboxRenderer.Render(delta);
			AxisLinesRenderer.Render(delta);
			Entities.RenderModels(Graphics, delta, t);
			Entities.RenderNames(Graphics, delta);
			
			ParticleManager.Render(delta, t);
			Camera.GetPickedBlock(SelectedPos); // TODO: only pick when necessary
			EnvRenderer.Render(delta);
			ChunkUpdater.Update(delta);
			MapRenderer.RenderNormal(delta);
			MapBordersRenderer.RenderSides(delta);
			
			Entities.DrawShadows();
			if (SelectedPos.Valid && !HideGui) {
				Picking.Update(SelectedPos);
				Picking.Render(delta);
			}
			
			// Render water over translucent blocks when underwater for proper alpha blending
			Vector3 pos = LocalPlayer.Position;
			if (CurrentCameraPos.Y < World.Env.EdgeHeight
			    && (pos.X < 0 || pos.Z < 0 || pos.X > World.Width || pos.Z > World.Length)) {
				MapRenderer.RenderTranslucent(delta);
				MapBordersRenderer.RenderEdges(delta);
			} else {
				MapBordersRenderer.RenderEdges(delta);
				MapRenderer.RenderTranslucent(delta);
			}
			
			// Need to render again over top of translucent block, as the selection outline
			// is drawn without writing to the depth buffer
			if (SelectedPos.Valid && !HideGui && BlockInfo.Draw[SelectedPos.Block] == DrawType.Translucent) {
				Picking.Render(delta);
			}
			
			SelectionManager.Render(delta);
			Entities.RenderHoveredNames(Graphics, delta);
			
			bool left   = IsMousePressed(MouseButton.Left);
			bool middle = IsMousePressed(MouseButton.Middle);
			bool right  = IsMousePressed(MouseButton.Right);
			Input.PickBlocks(true, left, middle, right);
			if (!HideGui) HeldBlockRenderer.Render(delta);
		}
		
		void DoScheduledTasks(double time) {
			for (int i = 0; i < Tasks.Count; i++) {
				ScheduledTask task = Tasks[i];
				task.Accumulator += time;
				
				while (task.Accumulator >= task.Interval) {
					task.Callback(task);
					task.Accumulator -= task.Interval;
				}
			}
		}
		
		void TakeScreenshot() {
			Utils.EnsureDirectory("screenshots");
			string timestamp = Utils.LocalNow().ToString("dd-MM-yyyy-HH-mm-ss");
			string file = "screenshot_" + timestamp + ".png";
			string path = PathIO.Combine("screenshots", file);
			
			using (Stream fs = Platform.FileCreate(path)) {
				Graphics.TakeScreenshot(fs, Width, Height);
			}
			
			screenshotRequested = false;
			Chat.Add("&eTaken screenshot as: " + file);
		}
		
		public void Dispose() {
			ChunkUpdater.Dispose();
			Atlas2D.Dispose();
			Atlas1D.Dispose();
			ModelCache.Dispose();
			Entities.Dispose();
			
			Events.OnNewMap        -= OnNewMapCore;
			Events.OnNewMapLoaded  -= OnNewMapLoadedCore;
			Events.TextureChanged  -= TextureChangedCore;
			Events.LowVRAMDetected -= OnLowVRAMDetected;
			
			for (int i = 0; i < Components.Count; i++)
				Components[i].Dispose();
			
			Drawer2D.DisposeInstance();
			Graphics.Dispose();
			// TODO: is this needed
			//window.Dispose();
			
			if (!Options.HasChanged()) return;
			Options.Load();
			Options.Save();
		}
	}
}