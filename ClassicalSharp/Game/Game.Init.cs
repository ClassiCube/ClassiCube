// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
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
using ClassicalSharp.Mode;
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

namespace ClassicalSharp {

	public partial class Game : IDisposable {
		
		internal void OnLoad() {
			Mouse = window.Mouse;
			Keyboard = window.Keyboard;
			
			#if ANDROID
			Graphics = new OpenGLESApi();
			#elif !USE_DX
			Graphics = new OpenGLApi();
			#else
			Graphics = new Direct3D9Api(this);
			#endif
			Graphics.MakeApiInfo();
			ErrorHandler.AdditionalInfo = Graphics.ApiInfo;
			
			#if ANDROID
			Drawer2D = new CanvasDrawer2D(Graphics);
			#else
			Drawer2D = new GdiPlusDrawer2D(Graphics);
			#endif
			
			Entities = new EntityList(this);
			AcceptedUrls.Load();
			DeniedUrls.Load();
			ETags.Load();
			LastModified.Load();
			
			if (Options.GetBool(OptionsKey.SurvivalMode, false)) {
				Mode = new SurvivalGameMode();
			} else {
				Mode = new CreativeGameMode();
			}
			Components.Add(Mode);
			
			Input = new InputHandler(this);
			defaultIb = Graphics.MakeDefaultIb();
			ParticleManager = new ParticleManager(); Components.Add(ParticleManager);
			TabList = new TabList(); Components.Add(TabList);			
			LoadOptions();
			LoadGuiOptions();
			Chat = new Chat(); Components.Add(Chat);
			
			WorldEvents.OnNewMap += OnNewMapCore;
			WorldEvents.OnNewMapLoaded += OnNewMapLoadedCore;
			Events.TextureChanged += TextureChangedCore;
			
			BlockInfo.Init();
			ModelCache = new ModelCache(this);
			ModelCache.InitCache();
			AsyncDownloader = new AsyncDownloader(Drawer2D); Components.Add(AsyncDownloader);
			Lighting = new BasicLighting(); Components.Add(Lighting);
			
			Drawer2D.UseBitmappedChat = ClassicMode || !Options.GetBool(OptionsKey.UseChatFont, false);
			Drawer2D.BlackTextShadows = Options.GetBool(OptionsKey.BlackText, false);
			Graphics.Mipmaps = Options.GetBool(OptionsKey.Mipmaps, false);
			
			TerrainAtlas1D = new TerrainAtlas1D(this);
			TerrainAtlas = new TerrainAtlas2D(this);
			Animations = new Animations(); Components.Add(Animations);
			Inventory = new Inventory(); Components.Add(Inventory);
			
			BlockInfo.SetDefaultPerms();
			World = new World(this);
			LocalPlayer = new LocalPlayer(this); Components.Add(LocalPlayer);
			Entities.List[EntityList.SelfID] = LocalPlayer;
			Width = window.Width; Height = window.Height;
			
			MapRenderer = new MapRenderer(this);
			string renType = Options.Get(OptionsKey.RenderType) ?? "normal";
			if (!SetRenderType(renType))
				SetRenderType("normal");
			
			if (IPAddress == null) {
				Server = new Singleplayer.SinglePlayerServer(this);
			} else {
				Server = new Network.NetworkProcessor(this);
			}
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
			
			plugins = new PluginLoader(this);
			List<string> nonLoaded = plugins.LoadAll();
			
			for (int i = 0; i < Components.Count; i++)
				Components[i].Init(this);
			ExtractInitialTexturePack();
			for (int i = 0; i < Components.Count; i++)
				Components[i].Ready(this);
			InitScheduledTasks();
			
			if (nonLoaded != null) {
				for (int i = 0; i < nonLoaded.Count; i++) {
					plugins.MakeWarning(this, nonLoaded[i]);
				}
			}
			
			window.LoadIcon();
			string connectString = "Connecting to " + IPAddress + ":" + Port +  "..";
			if (Graphics.WarnIfNecessary(Chat)) {
				MapBordersRenderer.UseLegacyMode(true);
				EnvRenderer.UseLegacyMode(true);
			}
			Gui.SetNewScreen(new LoadingMapScreen(this, connectString, ""));
			Server.Connect(IPAddress, Port);
		}
		
		void ExtractInitialTexturePack() {
			defTexturePack = Options.Get(OptionsKey.DefaultTexturePack) ?? "default.zip";
			TexturePack extractor = new TexturePack();
			extractor.Extract("default.zip", this);
			// in case the user's default texture pack doesn't have all required textures
			if (DefaultTexturePack != "default.zip")
				extractor.Extract(DefaultTexturePack, this);
		}
		
		void LoadOptions() {
			ClassicMode = Options.GetBool("mode-classic", false);
			ClassicHacks = Options.GetBool(OptionsKey.AllowClassicHacks, false);
			UseCustomBlocks = Options.GetBool(OptionsKey.UseCustomBlocks, true);
			UseCPE = Options.GetBool(OptionsKey.UseCPE, true);
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
			ModifiableLiquids = !ClassicMode && Options.GetBool(OptionsKey.ModifiableLiquids, false);
			CameraClipping = Options.GetBool(OptionsKey.CameraClipping, true);
			
			UseServerTextures = Options.GetBool(OptionsKey.UseServerTextures, true);
			MouseSensitivity = Options.GetInt(OptionsKey.Sensitivity, 1, 100, 30);
			ShowBlockInHand = Options.GetBool(OptionsKey.ShowBlockInHand, true);
			InvertMouse = Options.GetBool(OptionsKey.InvertMouse, false);
			
			bool skipSsl = Options.GetBool("skip-ssl-check", false);
			if (skipSsl) {
				ServicePointManager.ServerCertificateValidationCallback = delegate { return true; };
				Options.Set("skip-ssl-check", false);
			}
		}
		
		void LoadGuiOptions() {
			ChatLines = Options.GetInt(OptionsKey.ChatLines, 1, 30, 12);
			ClickableChat = Options.GetBool(OptionsKey.ClickableChat, false);
			InventoryScale = Options.GetFloat(OptionsKey.InventoryScale, 0.25f, 5f, 1f);
			HotbarScale = Options.GetFloat(OptionsKey.HotbarScale, 0.25f, 5f, 1f);
			ChatScale = Options.GetFloat(OptionsKey.ChatScale, 0.35f, 5f, 1f);
			ShowFPS = Options.GetBool(OptionsKey.ShowFPS, true);

			UseClassicGui = Options.GetBool(OptionsKey.UseClassicGui, true)          || ClassicMode;
			UseClassicTabList = Options.GetBool(OptionsKey.UseClassicTabList, false) || ClassicMode;
			UseClassicOptions = Options.GetBool(OptionsKey.UseClassicOptions, false) || ClassicMode;
			
			TabAutocomplete = Options.GetBool(OptionsKey.TabAutocomplete, false);
			FontName = Options.Get(OptionsKey.FontName) ?? "Arial";
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
			
			AddScheduledTask(30, AsyncDownloader.PurgeOldEntriesTask);
			AddScheduledTask(netTicks, Server.Tick);
			entTask = AddScheduledTask(defTicks, Entities.Tick);
			
			AddScheduledTask(defTicks, ParticleManager.Tick);
			AddScheduledTask(defTicks, Animations.Tick);
		}
	}
}