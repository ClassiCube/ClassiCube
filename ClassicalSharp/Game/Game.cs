﻿// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
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

#if USE16_BIT
using BlockID = System.UInt16;
#else
using BlockID = System.Byte;
#endif

namespace ClassicalSharp {

	public partial class Game : IDisposable {
		
		// For FileStreams we need to keep it open for lifetime of the image
		Stream lastAtlas;
		void LoadAtlas(Bitmap bmp, Stream data) {
			TerrainAtlas1D.Dispose();
			TerrainAtlas.Dispose();
			if (lastAtlas != null) lastAtlas.Dispose();
			
			TerrainAtlas.UpdateState(BlockInfo, bmp);
			TerrainAtlas1D.UpdateState(TerrainAtlas);
			lastAtlas = data;
		}
		
		public bool ChangeTerrainAtlas(Bitmap atlas, Stream data) {
			if (!ValidateBitmap("terrain.png", atlas)) return false;
			if (Graphics.LostContext) return false;
			
			LoadAtlas(atlas, data);
			Events.RaiseTerrainAtlasChanged();
			return true;
		}
		
		public void Run() { window.Run(); }
		
		public void Exit() { window.Exit(); }
		
		void OnNewMapCore(object sender, EventArgs e) {
			for (int i = 0; i < Components.Count; i++)
				Components[i].OnNewMap(this);
		}
		
		void OnNewMapLoadedCore(object sender, EventArgs e) {
			for (int i = 0; i < Components.Count; i++)
				Components[i].OnNewMapLoaded(this);
		}
		
		public T AddComponent<T>(T obj) where T : IGameComponent {
			Components.Add(obj);
			return obj;
		}
		
		public bool ReplaceComponent<T>(ref T old, T obj) where T : IGameComponent {
			for (int i = 0; i < Components.Count; i++) {
				if (!object.ReferenceEquals(Components[i], old)) continue;
				old.Dispose();
				
				Components[i] = obj;
				old = obj;
				obj.Init(this);
				return true;
			}
			
			Components.Add(obj);
			obj.Init(this);
			return false;
		}
		
		public void SetViewDistance(float distance, bool userDist) {
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
		
		Stopwatch frameTimer = new Stopwatch();
		internal void RenderFrame(double delta) {
			frameTimer.Reset();
			frameTimer.Start();
			
			Graphics.BeginFrame(this);
			Graphics.BindIb(defaultIb);
			accumulator += delta;
			Vertices = 0;
			
			Camera.UpdateMouse();
			if (!Focused && !Gui.ActiveScreen.HandlesAllInput)
				Gui.SetNewScreen(new PauseScreen(this));
			CheckZoomFov();
			
			DoScheduledTasks(delta);
			float t = (float)(entTask.Accumulator / entTask.Interval);
			LocalPlayer.SetInterpPosition(t);
			
			if (!SkipClear || SkyboxRenderer.ShouldRender)
				Graphics.Clear();
			CurrentCameraPos = Camera.GetCameraPos(t);
			UpdateViewMatrix();
			
			bool visible = Gui.activeScreen == null || !Gui.activeScreen.BlocksWorld;
			if (World.IsNotLoaded) visible = false;
			if (visible) {
				Render3D(delta, t);
			} else {
				SelectedPos.SetAsInvalid();
			}
			
			Gui.Render(delta);
			if (screenshotRequested)
				TakeScreenshot();
			Graphics.EndFrame(this);
			LimitFPS();
		}
		
		void CheckZoomFov() {
			bool allowZoom = Gui.activeScreen == null && !Gui.hudScreen.HandlesAllInput;
			if (allowZoom && IsKeyDown(KeyBind.ZoomScrolling))
				Input.SetFOV(ZoomFov, false);
		}
		
		void UpdateViewMatrix() {
			Graphics.SetMatrixMode(MatrixType.Modelview);
			Matrix4 modelView = Camera.GetView();
			View = modelView;
			Graphics.LoadMatrix(ref modelView);
			Culling.CalcFrustumEquations(ref Projection, ref modelView);
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
			MapRenderer.Update(delta);
			MapRenderer.RenderNormal(delta);
			MapBordersRenderer.RenderSides(delta);
			
			if (SelectedPos.Valid && !HideGui) {
				Picking.UpdateState(SelectedPos);
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
			if (SelectedPos.Valid && !HideGui
			    && BlockInfo.Draw[SelectedPos.Block] == DrawType.Translucent) {
				Picking.Render(delta);
			}
			
			Entities.DrawShadows();
			SelectionManager.Render(delta);
			Entities.RenderHoveredNames(Graphics, delta);
			
			bool left = IsMousePressed(MouseButton.Left);
			bool middle = IsMousePressed(MouseButton.Middle);
			bool right = IsMousePressed(MouseButton.Right);
			Input.PickBlocks(true, left, middle, right);
			if (!HideGui)
				HeldBlockRenderer.Render(delta);
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
		
		public ScheduledTask AddScheduledTask(double interval,
		                                      Action<ScheduledTask> callback) {
			ScheduledTask task = new ScheduledTask();
			task.Interval = interval; task.Callback = callback;
			Tasks.Add(task);
			return task;
		}
		
		void TakeScreenshot() {
			string path = PathIO.Combine(Program.AppDirectory, "screenshots");
			if (!Directory.Exists(path))
				Directory.CreateDirectory(path);
			
			string timestamp = DateTime.Now.ToString("dd-MM-yyyy-HH-mm-ss");
			string file = "screenshot_" + timestamp + ".png";
			path = PathIO.Combine(path, file);
			Graphics.TakeScreenshot(path, Width, Height);
			Chat.Add("&eTaken screenshot as: " + file);
			screenshotRequested = false;
		}
		
		public void UpdateProjection() {
			DefaultFov = Options.GetInt(OptionsKey.FieldOfView, 1, 150, 70);
			Matrix4 projection = Camera.GetProjection();
			Projection = projection;
			
			Graphics.SetMatrixMode(MatrixType.Projection);
			Graphics.LoadMatrix(ref projection);
			Graphics.SetMatrixMode(MatrixType.Modelview);
			Events.RaiseProjectionChanged();
		}
		
		internal void OnResize() {
			Width = window.Width; Height = window.Height;
			Graphics.OnWindowResize(this);
			UpdateProjection();
			Gui.OnResize();
		}
		
		public void Disconnect(string title, string reason) {
			Events.RaiseDisconnected(title, reason);
			
			Gui.Reset(this);
			World.Reset();
			World.blocks = null;
			Drawer2D.InitColours();
			BlockInfo.Reset(this);
			TexturePack.ExtractDefault(this);
			Gui.SetNewScreen(new ErrorScreen(this, title, reason));
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
			if (weather.heightmap != null && !World.IsNotLoaded)
				weather.OnBlockChanged(x, y, z, oldBlock, block);
			Lighting.OnBlockChanged(x, y, z, oldBlock, block);
			
			// Refresh the chunk the block was located in.
			int cx = x >> 4, cy = y >> 4, cz = z >> 4;
			MapRenderer.GetChunk(cx, cy, cz).AllAir &= BlockInfo.Draw[block] == DrawType.Gas;
			MapRenderer.RefreshChunk(cx, cy, cz);
		}
		
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
		
		public bool IsKeyDown(Key key) { return Input.IsKeyDown(key); }
		
		public bool IsKeyDown(KeyBind binding) { return Input.IsKeyDown(binding); }
		
		public bool IsMousePressed(MouseButton button) { return Input.IsMousePressed(button); }
		
		public Key Mapping(KeyBind mapping) { return Input.Keys[mapping]; }
		
		public void Dispose() {
			MapRenderer.Dispose();
			TerrainAtlas.Dispose();
			TerrainAtlas1D.Dispose();
			ModelCache.Dispose();
			Entities.Dispose();
			WorldEvents.OnNewMap -= OnNewMapCore;
			WorldEvents.OnNewMapLoaded -= OnNewMapLoadedCore;
			Events.TextureChanged -= TextureChangedCore;
			
			for (int i = 0; i < Components.Count; i++)
				Components[i].Dispose();
			
			Graphics.DeleteIb(ref defaultIb);
			Drawer2D.DisposeInstance();
			Graphics.DeleteTexture(ref CloudsTex);
			Graphics.Dispose();
			if (lastAtlas != null) lastAtlas.Dispose();
			
			if (Options.OptionsChanged.Count == 0) return;
			Options.Load();
			Options.Save();
		}
		
		public bool CanPick(BlockID block) {
			if (BlockInfo.Draw[block] == DrawType.Gas) return false;
			if (BlockInfo.Draw[block] == DrawType.Sprite) return true;
			if (BlockInfo.Collide[block] != CollideType.SwimThrough) return true;
			
			return !ModifiableLiquids ? false :
				Inventory.CanPlace[block] && Inventory.CanDelete[block];
		}
		
		
		/// <summary> Reads a bitmap from the stream (converting it to 32 bits per pixel if necessary),
		/// and updates the native texture for it. </summary>
		public bool UpdateTexture(ref int texId, string file, byte[] data, bool setSkinType) {			
			using (Bitmap bmp = Platform.ReadBmp32Bpp(Drawer2D, data)) {
				if (!ValidateBitmap(file, bmp)) return false;
				
				Graphics.DeleteTexture(ref texId);
				if (setSkinType) {
					DefaultPlayerSkinType = Utils.GetSkinType(bmp);
					if (DefaultPlayerSkinType == SkinType.Invalid)
						throw new NotSupportedException("char.png has invalid dimensions");
				}
				
				texId = Graphics.CreateTexture(bmp, true);
				return true;
			}
		}
		
		public bool ValidateBitmap(string file, Bitmap bmp) {
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
		
		public bool SetRenderType(string type) {
			if (Utils.CaselessEquals(type, "legacyfast")) {
				SetNewRenderType(true, true);
			} else if (Utils.CaselessEquals(type, "legacy")) {
				SetNewRenderType(true, false);
			} else if (Utils.CaselessEquals(type, "normal")) {
				SetNewRenderType(false, false);
			} else if (Utils.CaselessEquals(type, "normalfast")) {
				SetNewRenderType(false, true);
			} else {
				return false;
			}
			Options.Set(OptionsKey.RenderType, type);
			return true;
		}
		
		void SetNewRenderType(bool legacy, bool minimal) {
			if (MapBordersRenderer == null) {
				MapBordersRenderer = AddComponent(new MapBordersRenderer());
				MapBordersRenderer.legacy = legacy;
			} else {
				MapBordersRenderer.UseLegacyMode(legacy);
			}
			
			if (minimal) {
				if (EnvRenderer == null)
					EnvRenderer = AddComponent(new MinimalEnvRenderer());
				else
					ReplaceComponent(ref EnvRenderer, new MinimalEnvRenderer());
			} else if (EnvRenderer == null) {
				EnvRenderer = AddComponent(new StandardEnvRenderer());
				((StandardEnvRenderer)EnvRenderer).legacy = legacy;
			} else {
				if (!(EnvRenderer is StandardEnvRenderer))
					ReplaceComponent(ref EnvRenderer, new StandardEnvRenderer());
				((StandardEnvRenderer)EnvRenderer).UseLegacyMode(legacy);
			}
		}
		
		void TextureChangedCore(object sender, TextureEventArgs e) {
			byte[] data = e.Data;
			if (e.Name == "terrain.png") {
				Bitmap atlas = Platform.ReadBmp32Bpp(Drawer2D, data);
				if (ChangeTerrainAtlas(atlas, null)) return;
				atlas.Dispose();
			} else if (e.Name == "cloud.png" || e.Name == "clouds.png") {
				UpdateTexture(ref CloudsTex, e.Name, data, false);
			} else if (e.Name == "default.png") {
				Bitmap bmp = Platform.ReadBmp32Bpp(Drawer2D, data);
				Drawer2D.SetFontBitmap(bmp);
				Events.RaiseChatFontChanged();
			}
		}
		
		
		public Game(string username, string mppass, string skinServer,
		            bool nullContext, int width, int height) {
			window = new DesktopWindow(this, username, nullContext, width, height);
			Username = username;
			Mppass = mppass;
			this.skinServer = skinServer;
		}
	}
}