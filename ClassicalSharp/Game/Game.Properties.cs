// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Net;
using ClassicalSharp.Audio;
using ClassicalSharp.Commands;
using ClassicalSharp.Entities;
using ClassicalSharp.Events;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Gui;
using ClassicalSharp.Map;
using ClassicalSharp.Model;
using ClassicalSharp.Network;
using ClassicalSharp.Particles;
using ClassicalSharp.Renderers;
using ClassicalSharp.Selections;
using ClassicalSharp.Textures;
using OpenTK;
using OpenTK.Input;

namespace ClassicalSharp {
		
	/// <summary> Represents a game component. </summary>
	public interface IGameComponent : IDisposable {
		
		/// <summary> Called when the game is being loaded. </summary>
		void Init(Game game);
		
		/// <summary> Called when the texture pack has been loaded and all components have been initalised. </summary>
		void Ready(Game game);
		
		/// <summary> Called to reset the component's state when the user is reconnecting to a server. </summary>
		void Reset(Game game);
		
		/// <summary> Called to update the component's state when the user begins loading a new map. </summary>
		void OnNewMap(Game game);
		
		/// <summary> Called to update the component's state when the user has finished loading a new map. </summary>
		void OnNewMapLoaded(Game game);
	}
	
	/// <summary> Represents a task that runs on the main thread every certain interval. </summary>
	public class ScheduledTask {
		public double Accumulator, Interval;
		public Action<ScheduledTask> Callback;
	}
	
	public partial class Game {
		
		/// <summary> Abstracts the underlying 3D graphics rendering API. </summary>
		public IGraphicsApi Graphics;
		
		/// <summary> Contains the block data and metadata/properties for the player's current world. </summary>
		public World World;
		
		/// <summary> Represents a connection to a multiplayer or a singleplayer server. </summary>
		public IServerConnection Server;
		
		/// <summary> List of all entities in the current map, including the player. </summary>
		public EntityList Entities;
		
		/// <summary> Entity representing the player. </summary>
		public LocalPlayer LocalPlayer;
		
		/// <summary> Contains information for each player in the current world 
		/// (or for whole server if supported). </summary>
		public TabList TabList;
		
		/// <summary> Current camera the player is using to view the world. </summary>
		public Camera Camera;
		/// <summary> List of all cameras the user can use to view the world. </summary>
		public List<Camera> Cameras = new List<Camera>();
		
		/// <summary> Contains the metadata about each currently defined block. </summary>
		/// <remarks> e.g. blocks light, height, texture IDs, etc. </remarks>
		public BlockInfo BlockInfo;
		
		/// <summary> Total rendering time(in seconds) elapsed since the client was started. </summary>
		public double accumulator;
		public TerrainAtlas2D TerrainAtlas;
		public TerrainAtlas1D TerrainAtlas1D;
		public SkinType DefaultPlayerSkinType;
		
		/// <summary> Accumulator for the number of chunk updates performed. Reset every second. </summary>
		public int ChunkUpdates;
		
		/// <summary> Whether the third person camera should have their camera position clipped so as to not intersect blocks. </summary>
		public bool CameraClipping = true;
		
		public bool SkipClear = false;
		
		public IWorldLighting Lighting;
		
		public MapRenderer MapRenderer;
		public MapBordersRenderer MapBordersRenderer;
		public EnvRenderer EnvRenderer;
		public WeatherRenderer WeatherRenderer;
		public Inventory Inventory;
		public IDrawer2D Drawer2D;
		public GuiInterface Gui;
		
		public CommandList CommandList;
		public SelectionManager SelectionManager;
		public ParticleManager ParticleManager;
		public PickedPosRenderer Picking;
		public PickedPos SelectedPos = new PickedPos(), CameraClipPos = new PickedPos();
		public ModelCache ModelCache;
		internal string skinServer, chatInInputBuffer = null;
		internal int defaultIb;
		public OtherEvents Events = new OtherEvents();
		public EntityEvents EntityEvents = new EntityEvents();
		public WorldEvents WorldEvents = new WorldEvents();
		public UserEvents UserEvents = new UserEvents();
		public InputHandler Input;
		public Chat Chat;
		public HeldBlockRenderer HeldBlockRenderer;
		public AudioPlayer AudioPlayer;
		public AxisLinesRenderer AxisLinesRenderer;
		public SkyboxRenderer SkyboxRenderer;
		
		public List<IGameComponent> Components = new List<IGameComponent>();
		public List<ScheduledTask> Tasks = new List<ScheduledTask>();

		/// <summary> Whether x to stone brick tiles should be used. </summary>
		public bool UseCPEBlocks = false;
		
		/// <summary> Account username of the player. </summary>
		public string Username;
		
		/// <summary> Verification key used for multiplayer, unique for the username and individual server. </summary>
		public string Mppass;
		
		/// <summary> IP address of multiplayer server connected to, null if singleplayer. </summary>
		public IPAddress IPAddress;
		
		/// <summary> Port of multiplayer server connected to, 0 if singleplayer. </summary>
		public int Port;
		
		/// <summary> Radius of the sphere the player can see around the position of the current camera. </summary>
		public float ViewDistance = 512;
		internal float MaxViewDistance = 32768, UserViewDistance = 512;
		
		/// <summary> Field of view for the current camera in degrees. </summary>
		public int Fov = 70;
		internal int DefaultFov, ZoomFov = 0;
		
		/// <summary> Strategy used to limit how many frames should be displayed at most each second. </summary>
		public FpsLimitMethod FpsLimit;
		
		/// <summary> Whether lines should be rendered for each axis. </summary>
		public bool ShowAxisLines;
		
		/// <summary> Whether players should animate using simple swinging parallel to their bodies. </summary>
		public bool SimpleArmsAnim;
		
		/// <summary> Whether mouse rotation on the y axis should be inverted. </summary>
		public bool InvertMouse;
		
		public long Vertices;
		public FrustumCulling Culling;
		public AsyncDownloader AsyncDownloader;
		public Matrix4 View, Projection;
		
		/// <summary> How sensitive the client is to changes in the player's mouse position. </summary>
		public int MouseSensitivity = 30;
		
		public bool TabAutocomplete;
		
		public bool UseClassicGui, UseClassicTabList, UseClassicOptions, ClassicMode, ClassicHacks;
		
		public bool PureClassic { get { return ClassicMode && !ClassicHacks; } }
		
		public bool AllowCustomBlocks, UseCPE, AllowServerTextures;
		
		public bool SmoothLighting;
		
		public bool autoRotate = true;
		
		public string FontName = "Arial";
		
		public int ChatLines = 12;
		public bool ClickableChat = false, HideGui = false, ShowFPS = true;
		internal float HotbarScale = 1, ChatScale = 1, InventoryScale = 1;
		public bool ViewBobbing, ShowBlockInHand;
		public bool UseSound, UseMusic, ModifiableLiquids;
		
		public Vector3 CurrentCameraPos;
		
		public Animations Animations;
		internal int CloudsTex;
		internal bool screenshotRequested;
		
		internal EntryList AcceptedUrls = new EntryList("texturecache", "acceptedurls.txt"); 
		internal EntryList DeniedUrls = new EntryList("texturecache", "deniedurls.txt");
		internal EntryList ETags = new EntryList("texturecache", "etags.txt");
		internal EntryList LastModified = new EntryList("texturecache", "lastmodified.txt");
		
		PluginLoader plugins;		
		
		/// <summary> Calculates the amount that the hotbar widget should be scaled by when rendered. </summary>
		/// <remarks> Affected by both the current resolution of the window, as well as the
		/// scaling specified by the user (field HotbarScale). </remarks>
		public float GuiHotbarScale { get { return Scale(MinWindowScale  * HotbarScale); } }
		
		/// <summary> Calculates the amount that the block inventory menu should be scaled by when rendered. </summary>
		/// <remarks> Affected by both the current resolution of the window, as well as the
		/// scaling specified by the user (field InventoryScale). </remarks>
		public float GuiInventoryScale { get { return Scale(MinWindowScale  * InventoryScale); } }
		
		/// <summary> Calculates the amount that 2D chat widgets should be scaled by when rendered. </summary>
		/// <remarks> Affected by both the current resolution of the window, as well as the
		/// scaling specified by the user (field ChatScale). </remarks>
		public float GuiChatScale { get { return Scale((Height / 480f) * ChatScale); } }
		
		float MinWindowScale { get { return Math.Min(Width / 640f, Height / 480f); } }
		
		public float Scale(float value) { 
			return (float)Math.Round(value * 10, MidpointRounding.AwayFromZero) / 10; 
		}
		
		string defTexturePack = "default.zip";
		/// <summary> Gets or sets the path of the default texture pack that should be used by the client. </summary>
		/// <remarks> If the custom default texture pack specified by the user could not be found,
		/// this method returns "default.zip". </remarks>
		public string DefaultTexturePack {
			get {
				string path = Path.Combine(Program.AppDirectory, TexturePack.Dir);
				path = Path.Combine(path, defTexturePack);
				return File.Exists(path) && !ClassicMode ? defTexturePack : "default.zip"; 
			}
			set {
				defTexturePack = value;
				Options.Set(OptionsKey.DefaultTexturePack, value);
			}
		}
		
		internal IPlatformWindow window;
		public MouseDevice Mouse;
		public KeyboardDevice Keyboard;
		
		public int Width, Height;
		
		public bool Focused { get { return window.Focused; } }
		
		public bool Exists { get { return window.Exists; } }
		
		public Point PointToScreen(Point coords) {
			return window.PointToScreen(coords);
		}
		
		public bool VSync {
			get { return window.VSync; }
			set { window.VSync = value; }
		}
		
		bool visible = true;
		internal bool realVisible = true;
		public bool CursorVisible { 
			get { return visible; }
			set {
				// Defer mouse visibility changes.
				realVisible = value;
				if (Gui.overlays.Count > 0) return;
				   
				// Only set the value when it has changes.
				if (visible == value) return;
				window.CursorVisible = value;
				visible = value;
			}
		}
		
		public Point DesktopCursorPos {
			get { return window.DesktopCursorPos; }
			set { window.DesktopCursorPos = value; }
		}
	}
}