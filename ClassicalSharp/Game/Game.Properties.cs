using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Net;
using ClassicalSharp.Audio;
using ClassicalSharp.Commands;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Model;
using ClassicalSharp.Network;
using ClassicalSharp.Particles;
using ClassicalSharp.Renderers;
using ClassicalSharp.Selections;
using ClassicalSharp.TexturePack;
using OpenTK;
using OpenTK.Input;

namespace ClassicalSharp {

	public partial class Game {
		
		/// <summary> Abstracts the underlying 3D graphics rendering API. </summary>
		public IGraphicsApi Graphics;
		
		/// <summary> Contains the block data and metadata of the player's current world. </summary>
		public Map Map;
		
		/// <summary> Represents a connection to a multiplayer or a singleplayer server. </summary>
		public INetworkProcessor Network;
		
		/// <summary> List of all entities in the current map, including the player. </summary>
		public EntityList Players;
		
		/// <summary> Entity representing the player. </summary>
		public LocalPlayer LocalPlayer;
		
		/// <summary> Contains extended player listing information for each player in the current world. </summary>
		/// <remarks> Only used if CPE extension ExtPlayerList is enabled. </remarks>
		public CpeListInfo[] CpePlayersList = new CpeListInfo[256];
		
		/// <summary> Current camera the player is using to view the world with. </summary>
		/// <remarks> e.g. first person, thid person, forward third person, etc. </remarks>
		public Camera Camera;
		Camera firstPersonCam, thirdPersonCam, forwardThirdPersonCam;
		
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
		
		public bool ShowClock = false;
		
		public MapRenderer MapRenderer;
		public MapBordersRenderer MapBordersRenderer;
		public EnvRenderer EnvRenderer;
		public WeatherRenderer WeatherRenderer;
		public Inventory Inventory;
		public IDrawer2D Drawer2D;
		
		public CommandManager CommandManager;
		public SelectionManager SelectionManager;
		public ParticleManager ParticleManager;
		public PickedPosRenderer Picking;
		public PickedPos SelectedPos = new PickedPos(), CameraClipPos = new PickedPos();
		public ModelCache ModelCache;
		internal string skinServer, chatInInputBuffer = null;
		internal int defaultIb;
		FpsScreen fpsScreen;
		internal HudScreen hudScreen;
		public Events Events = new Events();
		public EntityEvents EntityEvents = new EntityEvents();
		public MapEvents MapEvents = new MapEvents();
		public InputHandler InputHandler;
		public Chat Chat;
		public BlockHandRenderer BlockHandRenderer;
		public AudioPlayer AudioPlayer;
		public AxisLinesRenderer AxisLinesRenderer;
		
		/// <summary> Account username of the player. </summary>
		public string Username;
		
		/// <summary> Verification key used for multiplayer, unique for the username and individual server. </summary>
		public string Mppass;
		
		/// <summary> IP address of multiplayer server connected to, null if singleplayer. </summary>
		public IPAddress IPAddress;
		
		/// <summary> Port of multiplayer server connected to, 0 if singleplayer. </summary>
		public int Port;
		
		/// <summary> Radius of the sphere the player can see around the position of the current camera. </summary>
		public int ViewDistance = 512;
		internal int MaxViewDistance = 32768, UserViewDistance = 512;
		
		/// <summary> Field of view for the current camera in degrees. </summary>
		public int FieldOfView = 70;
		internal int ZoomFieldOfView = 0;
		
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
		int width, height;
		public AsyncDownloader AsyncDownloader;
		public Matrix4 View, Projection, HeldBlockProjection;
		
		/// <summary> How sensitive the client is to changes in the player's mouse position. </summary>
		public int MouseSensitivity = 30;
		
		public bool TabAutocomplete;
		
		public bool UseClassicGui, UseClassicTabList, UseClassicOptions, PureClassicMode;
		
		public bool AllowCustomBlocks, UseCPE, AllowServerTextures;
		
		public string FontName = "Arial";
		
		public int ChatLines = 12;
		public bool ClickableChat = false, HideGui = false, ShowFPS = true;
		internal float HotbarScale = 1, ChatScale = 1, InventoryScale = 1;
		public bool ViewBobbing, ShowBlockInHand;
		public bool UseSound, UseMusic, ModifiableLiquids;
		
		public Vector3 CurrentCameraPos;
		
		public Animations Animations;
		internal int CloudsTexId, RainTexId, SnowTexId, GuiTexId, GuiClassicTexId;
		internal bool screenshotRequested;
		internal List<WarningScreen> WarningScreens = new List<WarningScreen>();
		internal UrlsList AcceptedUrls = new UrlsList( "acceptedurls.txt" ), DeniedUrls = new UrlsList( "deniedurls.txt" );
		
		/// <summary> Calculates the amount that the hotbar widget should be scaled by when rendered. </summary>
		/// <remarks> Affected by both the current resolution of the window, as well as the
		/// scaling specified by the user (field HotbarScale). </remarks>
		public float GuiHotbarScale { get { return MinWindowScale  * HotbarScale; } }
		
		/// <summary> Calculates the amount that the block inventory menu should be scaled by when rendered. </summary>
		/// <remarks> Affected by both the current resolution of the window, as well as the
		/// scaling specified by the user (field InventoryScale). </remarks>
		public float GuiInventoryScale { get { return MinWindowScale  * InventoryScale; } }
		
		/// <summary> Calculates the amount that 2D chat widgets should be scaled by when rendered. </summary>
		/// <remarks> Affected by both the current resolution of the window, as well as the
		/// scaling specified by the user (field ChatScale). </remarks>
		public float GuiChatScale { get { return (Height / 480f) * ChatScale; } }
		
		float MinWindowScale { get { return Math.Min( Width / 640f, Height / 480f ); } }
		
		string defTexturePack = "default.zip";
		/// <summary> Gets or sets the path of the default texture pack that should be used by the client. </summary>
		/// <remarks> If the custom default texture pack specified by the user could not be found,
		/// this method returns "default.zip". </remarks>
		public string DefaultTexturePack {
			get {
				string texPackDir = TexturePackExtractor.Dir;
				string path = Path.Combine( Program.AppDirectory, Path.Combine( texPackDir, defTexturePack ) );
				return File.Exists( path ) ? defTexturePack : "default.zip"; 
			}
			set {
				defTexturePack = value;
				Options.Set( OptionsKey.DefaultTexturePack, value );
			}
		}
		
		internal IPlatformWindow window;
		public MouseDevice Mouse;
		public KeyboardDevice Keyboard;
		
		public int Width { get { return window.Width; } }
		
		public int Height { get { return window.Height; } }
		
		public Size ClientSize { get { return window.ClientSize; } }
		
		public bool Focused { get { return window.Focused; } }
		
		public bool Exists { get { return window.Exists; } }
		
		public Point PointToScreen( Point coords ) {
			return window.PointToScreen( coords );
		}
		
		public bool VSync {
			get { return window.VSync; }
			set { window.VSync = value; }
		}
		
		public bool CursorVisible { 
			get { return window.CursorVisible; }
			set { window.CursorVisible = value; }
		}
		
		public Point DesktopCursorPos {
			get { return window.DesktopCursorPos; }
			set { window.DesktopCursorPos = value; }
		}
	}
}