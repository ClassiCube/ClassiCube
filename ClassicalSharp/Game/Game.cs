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
using ClassicalSharp.Events;
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
using OpenTK.Input;
#if ANDROID
using Android.Graphics;
#endif
using PathIO = System.IO.Path; // Android.Graphics.Path clash otherwise
using BlockID = System.UInt16;

namespace ClassicalSharp {

	public partial class Game : IDisposable {
		
		public Game(string username, string mppass, string skinServer,
		            bool nullContext, int width, int height) {
			window = new DesktopWindow(this, username, nullContext, width, height);
			Username = username;
			Mppass = mppass;
			this.skinServer = skinServer;
		}
		
		public bool ChangeTerrainAtlas(Bitmap atlas) {
			if (!ValidateBitmap("terrain.png", atlas)) return false;
			if (atlas.Width != atlas.Height) {
				Chat.Add("&cUnable to use terrain.png from the texture pack.");
				Chat.Add("&c Its width is not the same as its height.");
				return false;
			}
			if (Graphics.LostContext) return false;
			
			TerrainAtlas1D.Dispose();
			TerrainAtlas2D.Dispose();
			TerrainAtlas2D.UpdateState(atlas);
			TerrainAtlas1D.UpdateState();
			
			Events.RaiseTerrainAtlasChanged();
			return true;
		}
		
		public void Run() { window.Run(); }
		
		public void Exit() { window.Exit(); }
		
		public void SetViewDistance(int distance, bool userDist) {
			if (userDist) {
				UserViewDistance = distance;
				Options.Set(OptionsKey.ViewDist, distance);
			}
			
			distance = Math.Min(distance, MaxViewDistance);
			if (distance == ViewDistance) return;
			ViewDistance = distance;
			
			Events.RaiseViewDistanceChanged();
			UpdateProjection();
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
			WorldEvents.RaiseOnNewMap();
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
		
		public bool IsKeyDown(Key key) { return Input.IsKeyDown(key); }
		
		public bool IsKeyDown(KeyBind binding) { return Input.IsKeyDown(binding); }
		
		public bool IsMousePressed(MouseButton button) { return Input.IsMousePressed(button); }
		
		public Key Mapping(KeyBind mapping) { return Input.Keys[mapping]; }
		
		public bool CanPick(BlockID block) {
			if (BlockInfo.Draw[block] == DrawType.Gas) return false;
			if (BlockInfo.Draw[block] == DrawType.Sprite) return true;
			
			if (BlockInfo.Collide[block] != CollideType.Liquid) return true;
			return ModifiableLiquids && BlockInfo.CanPlace[block] && BlockInfo.CanDelete[block];
		}
		
		
		/// <summary> Reads a bitmap from the stream (converting it to 32 bits per pixel if necessary),
		/// and updates the native texture for it. </summary>
		public bool UpdateTexture(ref int texId, string file, byte[] data, bool setSkinType) {
			using (Bitmap bmp = Platform.ReadBmp32Bpp(Drawer2D, data)) {
				if (!ValidateBitmap(file, bmp)) return false;
				
				Graphics.DeleteTexture(ref texId);
				if (setSkinType) SetDefaultSkinType(bmp);
				
				texId = Graphics.CreateTexture(bmp, true, false);
				return true;
			}
		}
		
		void SetDefaultSkinType(Bitmap bmp) {
			DefaultPlayerSkinType = Utils.GetSkinType(bmp);
			if (DefaultPlayerSkinType == SkinType.Invalid)
				throw new NotSupportedException("char.png has invalid dimensions");
			
			Entity[] entities = Entities.List;
			for (int i = 0; i < EntityList.MaxCount; i++) {
				if (entities[i] == null || entities[i].TextureId != -1) continue;
				entities[i].SkinType = DefaultPlayerSkinType;
			}
		}
		
		public bool ValidateBitmap(string file, Bitmap bmp) {
			if (bmp == null) {
				Chat.Add("&cError loading " + file + " from the texture pack.");
				return false;
			}
			
			int maxSize = Graphics.MaxTextureDimensions;
			if (bmp.Width > maxSize || bmp.Height > maxSize) {
				Chat.Add("&cUnable to use " + file + " from the texture pack.");
				Chat.Add("&c Its size is (" + bmp.Width + "," + bmp.Height
				         + "), your GPU supports (" + maxSize + "," + maxSize + ") at most.");
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
			if (Utils.CaselessEquals(type, "legacyfast")) {
				return 0x03;
			} else if (Utils.CaselessEquals(type, "legacy")) {
				return 0x01;
			} else if (Utils.CaselessEquals(type, "normal")) {
				return 0x00;
			} else if (Utils.CaselessEquals(type, "normalfast")) {
				return 0x02;
			}
			return -1;
		}
		
		internal void OnResize() {
			Width = window.Width; Height = window.Height;
			Graphics.OnWindowResize(this);
			UpdateProjection();
			Gui.OnResize();
		}
		
		void OnNewMapCore(object sender, EventArgs e) {
			for (int i = 0; i < Components.Count; i++)
				Components[i].OnNewMap(this);
		}
		
		void OnNewMapLoadedCore(object sender, EventArgs e) {
			for (int i = 0; i < Components.Count; i++)
				Components[i].OnNewMapLoaded(this);
		}
		
		void TextureChangedCore(object sender, TextureEventArgs e) {
			if (e.Name == "terrain.png") {
				Bitmap atlas = Platform.ReadBmp32Bpp(Drawer2D, e.Data);
				if (ChangeTerrainAtlas(atlas)) return;
				atlas.Dispose();
			} else if (e.Name == "default.png") {
				Bitmap bmp = Platform.ReadBmp32Bpp(Drawer2D, e.Data);
				Drawer2D.SetFontBitmap(bmp);
				Events.RaiseChatFontChanged();
			}
		}

		Stopwatch frameTimer = new Stopwatch();
		float limitMilliseconds;
		public void SetFpsLimitMethod(FpsLimitMethod method) {
			FpsLimit = method;
			limitMilliseconds = 0;
			Graphics.SetVSync(this, method == FpsLimitMethod.LimitVSync);
			
			if (method == FpsLimitMethod.Limit120FPS)
				limitMilliseconds = 1000f / 120;
			if (method == FpsLimitMethod.Limit60FPS)
				limitMilliseconds = 1000f / 60;
			if (method == FpsLimitMethod.Limit30FPS)
				limitMilliseconds = 1000f / 30;
		}
		
		void LimitFPS() {
			if (FpsLimit == FpsLimitMethod.LimitVSync) return;
			
			double elapsed = frameTimer.Elapsed.TotalMilliseconds;
			double leftOver = limitMilliseconds - elapsed;
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
			Mode.BeginFrame(delta);
			
			Camera.UpdateMouse();
			if (!Focused && !Gui.ActiveScreen.HandlesAllInput) {
				Gui.SetNewScreen(new PauseScreen(this));
			}
			
			bool allowZoom = Gui.activeScreen == null && !Gui.hudScreen.HandlesAllInput;
			if (allowZoom && IsKeyDown(KeyBind.ZoomScrolling)) {
				Input.SetFOV(ZoomFov, false);
			}
			
			DoScheduledTasks(delta);
			float t = (float)(entTask.Accumulator / entTask.Interval);
			LocalPlayer.SetInterpPosition(t);
			
			if (!SkipClear) Graphics.Clear();
			CurrentCameraPos = Camera.GetCameraPos(t);
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
			
			Mode.EndFrame(delta);
			Graphics.EndFrame(this);
			LimitFPS();
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
			if (!Platform.DirectoryExists("screenshots")) {
				Platform.DirectoryCreate("screenshots");
			}
			
			string timestamp = DateTime.Now.ToString("dd-MM-yyyy-HH-mm-ss");
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
			TerrainAtlas2D.Dispose();
			TerrainAtlas1D.Dispose();
			ModelCache.Dispose();
			Entities.Dispose();
			WorldEvents.OnNewMap -= OnNewMapCore;
			WorldEvents.OnNewMapLoaded -= OnNewMapLoadedCore;
			Events.TextureChanged -= TextureChangedCore;
			
			for (int i = 0; i < Components.Count; i++)
				Components[i].Dispose();
			
			Drawer2D.DisposeInstance();
			Graphics.Dispose();
			
			if (!Options.HasChanged()) return;
			Options.Load();
			Options.Save();
		}
	}
}