﻿// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Net;
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
#if ANDROID
using Android.Graphics;
#endif
using PathIO = System.IO.Path; // Android.Graphics.Path clash otherwise
using BlockID = System.UInt16;

namespace ClassicalSharp {

	public partial class Game : IDisposable {
		
		internal void OnLoad() {
			#if ANDROID
			Graphics = new OpenGLESApi();
			#elif !USE_DX
			Graphics = new OpenGLApi(window);
			#else
			Graphics = new Direct3D9Api(window);
			#endif
			Graphics.MakeApiInfo();
			ErrorHandler.AdditionalInfo = Graphics.ApiInfo;
			
			#if ANDROID
			Drawer2D = new CanvasDrawer2D(Graphics);
			#else
			Drawer2D = new GdiPlusDrawer2D(Graphics);
			#endif
			UpdateClientSize();
			
			Entities = new EntityList(this);
			TextureCache.Init();
			
			#if SURVIVAL_TEST
			if (Options.GetBool(OptionsKey.SurvivalMode, false)) {
				Mode = new SurvivalGameMode();
			} else {
				Mode = new CreativeGameMode();
			}
			#endif
			
			Input = new InputHandler(this);
			ParticleManager = new ParticleManager(); Components.Add(ParticleManager);
			TabList = new TabList(); Components.Add(TabList);
			LoadOptions();
			LoadGuiOptions();
			Chat = new Chat(); Components.Add(Chat);
			
			WorldEvents.OnNewMap += OnNewMapCore;
			WorldEvents.OnNewMapLoaded += OnNewMapLoadedCore;
			Events.TextureChanged += TextureChangedCore;
			
			BlockInfo.Allocate(256);
			BlockInfo.Init();
			
			ModelCache = new ModelCache(this);
			ModelCache.InitCache();
			Downloader = new AsyncDownloader(Drawer2D); Components.Add(Downloader);
			Lighting = new BasicLighting(); Components.Add(Lighting);
			
			Drawer2D.UseBitmappedChat = ClassicMode || !Options.GetBool(OptionsKey.UseChatFont, false);
			Drawer2D.BlackTextShadows = Options.GetBool(OptionsKey.BlackText, false);
			Graphics.Mipmaps = Options.GetBool(OptionsKey.Mipmaps, false);
			
			Atlas1D.game = this;
			Atlas2D.game = this;
			Animations = new Animations(); Components.Add(Animations);
			Inventory = new Inventory(); Components.Add(Inventory);
			Inventory.Map = new BlockID[BlockInfo.Count];
			
			BlockInfo.SetDefaultPerms();
			World = new World(this);
			LocalPlayer = new LocalPlayer(this); Components.Add(LocalPlayer);
			Entities.List[EntityList.SelfID] = LocalPlayer;
			
			MapRenderer = new MapRenderer(this);
			ChunkUpdater = new ChunkUpdater(this);
			EnvRenderer = new EnvRenderer(); Components.Add(EnvRenderer);
			MapBordersRenderer = new MapBordersRenderer(); Components.Add(MapBordersRenderer);
			
			string renType = Options.Get(OptionsKey.RenderType, "normal");
			int flags = CalcRenderType(renType);
			if (flags == -1) flags = 0;
			
			MapBordersRenderer.legacy = (flags & 1) != 0;
			EnvRenderer.legacy        = (flags & 1) != 0;
			EnvRenderer.minimal       = (flags & 2) != 0;
			
			if (IPAddress == null) {
				Server = new Singleplayer.SinglePlayerServer(this);
			} else {
				Server = new Network.NetworkProcessor(this);
			}
			Components.Add(Server);
			Graphics.LostContextFunction = Server.Tick;
			
			Cameras.Add(new FirstPersonCamera(this));
			Cameras.Add(new ThirdPersonCamera(this, false));
			Cameras.Add(new ThirdPersonCamera(this, true));
			Camera = Cameras[0];
			UpdateProjection();
			
			Gui = new GuiInterface(this); Components.Add(Gui);
			CommandList = new CommandList(); Components.Add(CommandList);
			SelectionManager = new SelectionManager(); Components.Add(SelectionManager);
			WeatherRenderer = new WeatherRenderer(); Components.Add(WeatherRenderer);
			HeldBlockRenderer = new HeldBlockRenderer(); Components.Add(HeldBlockRenderer);
			
			Graphics.DepthTest = true;
			Graphics.DepthTestFunc(CompareFunc.LessEqual);
			//Graphics.DepthWrite = true;
			Graphics.AlphaBlendFunc(BlendFunc.SourceAlpha, BlendFunc.InvSourceAlpha);
			Graphics.AlphaTestFunc(CompareFunc.Greater, 0.5f);
			Culling = new FrustumCulling();
			Picking = new PickedPosRenderer(); Components.Add(Picking);
			AudioPlayer = new AudioPlayer(); Components.Add(AudioPlayer);
			AxisLinesRenderer = new AxisLinesRenderer(); Components.Add(AxisLinesRenderer);
			SkyboxRenderer = new SkyboxRenderer(); Components.Add(SkyboxRenderer);
			
			PluginLoader.game = this;
			List<string> nonLoaded = PluginLoader.LoadAll();
			
			for (int i = 0; i < Components.Count; i++)
				Components[i].Init(this);
			ExtractInitialTexturePack();
			for (int i = 0; i < Components.Count; i++)
				Components[i].Ready(this);
			InitScheduledTasks();
			
			if (nonLoaded != null) {
				for (int i = 0; i < nonLoaded.Count; i++) {
					Overlay warning = new PluginOverlay(this, nonLoaded[i]);
					Gui.ShowOverlay(warning, false);
				}
			}
			
			LoadIcon();
			string connectString = "Connecting to " + IPAddress + ":" + Port +  "..";
			if (Graphics.WarnIfNecessary(Chat)) {
				MapBordersRenderer.UseLegacyMode(true);
				EnvRenderer.UseLegacyMode(true);
			}
			Gui.SetNewScreen(new LoadingScreen(this, connectString, ""));
			Server.BeginConnect();
		}
		
		void ExtractInitialTexturePack() {
			defTexturePack = Options.Get(OptionsKey.DefaultTexturePack, "default.zip");
			TexturePack.ExtractZip("default.zip", this);
			
			// in case the user's default texture pack doesn't have all required textures
			string texPack = DefaultTexturePack;
			if (!Utils.CaselessEq(texPack, "default.zip")) {
				TexturePack.ExtractZip(texPack, this);
			}
		}
		
		void LoadOptions() {
			ClassicMode = Options.GetBool(OptionsKey.ClassicMode, false);
			ClassicHacks = Options.GetBool(OptionsKey.ClassicHacks, false);
			AllowCustomBlocks = Options.GetBool(OptionsKey.CustomBlocks, true);
			UseCPE = Options.GetBool(OptionsKey.CPE, true);
			SimpleArmsAnim = Options.GetBool(OptionsKey.SimpleArmsAnim, false);
			ChatLogging = Options.GetBool(OptionsKey.ChatLogging, true);
			ClassicArmModel = Options.GetBool(OptionsKey.ClassicArmModel, ClassicMode);
			
			ViewBobbing = Options.GetBool(OptionsKey.ViewBobbing, true);
			FpsLimitMethod method = Options.GetEnum(OptionsKey.FpsLimit, FpsLimitMethod.LimitVSync);
			SetFpsLimitMethod(method);
			ViewDistance = Options.GetInt(OptionsKey.ViewDist, 16, 4096, 512);
			UserViewDistance = ViewDistance;
			SmoothLighting = Options.GetBool(OptionsKey.SmoothLighting, false);
			
			DefaultFov = Options.GetInt(OptionsKey.FieldOfView, 1, 150, 70);
			Fov = DefaultFov;
			ZoomFov = DefaultFov;
			BreakableLiquids = !ClassicMode && Options.GetBool(OptionsKey.ModifiableLiquids, false);
			CameraClipping = Options.GetBool(OptionsKey.CameraClipping, true);
			MaxChunkUpdates = Options.GetInt(OptionsKey.MaxChunkUpdates, 4, 1024, 30);
			
			AllowServerTextures = Options.GetBool(OptionsKey.ServerTextures, true);
			MouseSensitivity = Options.GetInt(OptionsKey.Sensitivity, 1, 100, 30);
			ShowBlockInHand = Options.GetBool(OptionsKey.ShowBlockInHand, true);
			InvertMouse = Options.GetBool(OptionsKey.InvertMouse, false);

			CameraFriction = Options.GetFloat(OptionsKey.CameraFriction, 0.1f, 50f, 20f);

			bool skipSsl = Options.GetBool("skip-ssl-check", false);
			if (skipSsl) {
				ServicePointManager.ServerCertificateValidationCallback = delegate { return true; };
				Options.Set("skip-ssl-check", false);
			}
		}
		
		void LoadGuiOptions() {
			ChatLines = Options.GetInt(OptionsKey.ChatLines, 0, 30, 12);
			ClickableChat = Options.GetBool(OptionsKey.ClickableChat, false);
			InventoryScale = Options.GetFloat(OptionsKey.InventoryScale, 0.25f, 5f, 1f);
			HotbarScale = Options.GetFloat(OptionsKey.HotbarScale, 0.25f, 5f, 1f);
			ChatScale = Options.GetFloat(OptionsKey.ChatScale, 0.35f, 5f, 1f);
			ShowFPS = Options.GetBool(OptionsKey.ShowFPS, true);

			UseClassicGui     = Options.GetBool(OptionsKey.ClassicGui, true)      || ClassicMode;
			UseClassicTabList = Options.GetBool(OptionsKey.ClassicTabList, false) || ClassicMode;
			UseClassicOptions = Options.GetBool(OptionsKey.ClassicOptions, false) || ClassicMode;
			
			TabAutocomplete = Options.GetBool(OptionsKey.TabAutocomplete, false);
			FontName = Options.Get(OptionsKey.FontName, "Arial");
			if (ClassicMode) FontName = "Arial";

			try {
				using (Font f = new Font(FontName, 16)) { }
			} catch (Exception) {
				FontName = "Arial";
				Options.Set(OptionsKey.FontName, "Arial");
			}
		}
		
		ScheduledTask entTask;
		void InitScheduledTasks() {
			const double defTicks = 1.0 / 20;
			const double netTicks = 1.0 / 60;
			
			AddScheduledTask(30, Downloader.PurgeOldEntriesTask);
			AddScheduledTask(netTicks, Server.Tick);
			entTask = AddScheduledTask(defTicks, Entities.Tick);
			
			AddScheduledTask(defTicks, ParticleManager.Tick);
			AddScheduledTask(defTicks, Animations.Tick);
		}
		
		void LoadIcon() {
			string launcherFile = "Launcher2.exe";
			if (!Platform.FileExists(launcherFile)) {
				launcherFile = "Launcher.exe";
			}
			if (!Platform.FileExists(launcherFile)) return;
			
			try {
				window.Icon = Icon.ExtractAssociatedIcon(launcherFile);
			} catch (Exception ex) {
				ErrorHandler.LogError("Game.LoadIcon()", ex);
			}
		}
	}
}
