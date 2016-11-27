// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
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
			
			Options.Load();
			Entities = new EntityList(this);
			AcceptedUrls.Load();
			DeniedUrls.Load();
			ETags.Load();
			LastModified.Load();
			
			InputHandler = new InputHandler(this);
			defaultIb = Graphics.MakeDefaultIb();
			ParticleManager = AddComponent(new ParticleManager());
			TabList = AddComponent(new TabList());
			LoadOptions();
			LoadGuiOptions();
			Chat = AddComponent(new Chat());
			WorldEvents.OnNewMap += OnNewMapCore;
			WorldEvents.OnNewMapLoaded += OnNewMapLoadedCore;
			Events.TextureChanged += TextureChangedCore;
			
			BlockInfo = new BlockInfo();
			BlockInfo.Init();
			ModelCache = new ModelCache(this);
			ModelCache.InitCache();
			AsyncDownloader = AddComponent(new AsyncDownloader());
			
			#if ANDROID
			Drawer2D = new CanvasDrawer2D(Graphics);
			#else
			Drawer2D = new GdiPlusDrawer2D(Graphics);
			#endif
			
			Drawer2D.UseBitmappedChat = ClassicMode || !Options.GetBool(OptionsKey.ArialChatFont, false);
			Drawer2D.BlackTextShadows = Options.GetBool(OptionsKey.BlackTextShadows, false);
			
			TerrainAtlas1D = new TerrainAtlas1D(Graphics);
			TerrainAtlas = new TerrainAtlas2D(Graphics, Drawer2D);
			Animations = AddComponent(new Animations());
			Inventory = AddComponent(new Inventory());
			
			BlockInfo.SetDefaultBlockPerms(Inventory.CanPlace, Inventory.CanDelete);
			World = new World(this);
			LocalPlayer = AddComponent(new LocalPlayer(this));
			Entities[255] = LocalPlayer;
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
			
			Gui = AddComponent(new GuiInterface(this));
			CommandList = AddComponent(new CommandList());
			SelectionManager = AddComponent(new SelectionManager());
			WeatherRenderer = AddComponent(new WeatherRenderer());
			HeldBlockRenderer = AddComponent(new HeldBlockRenderer());
			
			Graphics.DepthTest = true;
			Graphics.DepthTestFunc(CompareFunc.LessEqual);
			//Graphics.DepthWrite = true;
			Graphics.AlphaBlendFunc(BlendFunc.SourceAlpha, BlendFunc.InvSourceAlpha);
			Graphics.AlphaTestFunc(CompareFunc.Greater, 0.5f);
			Culling = new FrustumCulling();
			Picking = AddComponent(new PickedPosRenderer());
			AudioPlayer = AddComponent(new AudioPlayer());
			AxisLinesRenderer = AddComponent(new AxisLinesRenderer());
			SkyboxRenderer = AddComponent(new SkyboxRenderer());
			
			for (int i = 0; i < Components.Count; i++)
				Components[i].Init(this);
			ExtractInitialTexturePack();
			for (int i = 0; i < Components.Count; i++)
				Components[i].Ready(this);
			InitScheduledTasks();
			
			window.LoadIcon();
			string connectString = "Connecting to " + IPAddress + ":" + Port +  "..";
			if (Graphics.WarnIfNecessary(Chat)) {
				MapBordersRenderer.UseLegacyMode(true);
				EnvRenderer.UseLegacyMode(true);
			}
			Gui.SetNewScreen(new LoadingMapScreen(this, connectString, "Waiting for handshake"));
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
			AllowCustomBlocks = Options.GetBool(OptionsKey.AllowCustomBlocks, true);
			UseCPE = Options.GetBool(OptionsKey.UseCPE, true);
			SimpleArmsAnim = Options.GetBool(OptionsKey.SimpleArmsAnim, false);
			
			ViewBobbing = Options.GetBool(OptionsKey.ViewBobbing, false);
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
			
			AllowServerTextures = Options.GetBool(OptionsKey.AllowServerTextures, true);
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

			UseClassicGui = Options.GetBool(OptionsKey.UseClassicGui, true) || ClassicMode;
			UseClassicTabList = Options.GetBool(OptionsKey.UseClassicTabList, false);
			UseClassicOptions = Options.GetBool(OptionsKey.UseClassicOptions, false);
			
			TabAutocomplete = Options.GetBool(OptionsKey.TabAutocomplete, false);
			FontName = Options.Get(OptionsKey.FontName) ?? "Arial";
			if (ClassicMode) FontName = "Arial";
			
			try {
				using (Font f = new Font(FontName, 16)) { }
			} catch(Exception) {
				FontName = "Arial";
				Options.Set(OptionsKey.FontName, "Arial");
			}
		}
		
		ScheduledTask entTask;
		void InitScheduledTasks() {
			const double defTicks = 1.0 / 20, camTicks = 1.0 / 60;
			AddScheduledTask(30, AsyncDownloader.PurgeOldEntriesTask);
			AddScheduledTask(defTicks, Server.Tick);
			entTask = AddScheduledTask(defTicks, Entities.Tick);
			
			AddScheduledTask(defTicks, ParticleManager.Tick);
			AddScheduledTask(defTicks, Animations.Tick);
			AddScheduledTask(camTicks, CameraTick);
		}
		
		void CameraTick(ScheduledTask task) {
			Camera.Tick(task.Interval);
		}
	}
}