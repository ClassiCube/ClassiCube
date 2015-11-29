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

namespace ClassicalSharp {

	public partial class Game : GameWindow {
		
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
		
		public MapRenderer MapRenderer;
		public MapBordersRenderer MapBordersRenderer;
		public EnvRenderer EnvRenderer;
		public WeatherRenderer WeatherRenderer;
		public Inventory Inventory;
		public IDrawer2D Drawer2D;
		
		public CommandManager CommandManager;
		public SelectionManager SelectionManager;
		public ParticleManager ParticleManager;
		public PickingRenderer Picking;
		public PickedPos SelectedPos = new PickedPos();
		public ModelCache ModelCache;
		internal string skinServer, chatInInputBuffer = null;
		internal int defaultIb;
		FpsScreen fpsScreen;
		internal HudScreen hudScreen;
		public Events Events = new Events();
		public InputHandler InputHandler;
		public ChatLog Chat;
		public BlockHandRenderer BlockHandRenderer;
		public AudioManager AudioManager;
		
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
		
		/// <summary> Field of view for the current camera in degrees. </summary>
		public int FieldOfView = 70;
		
		/// <summary> Strategy used to limit how many frames should be displayed at most each second. </summary>
		public FpsLimitMethod FpsLimit;
		
		public long Vertices;
		public FrustumCulling Culling;
		int width, height;
		public AsyncDownloader AsyncDownloader;
		public Matrix4 View, Projection, HeldBlockProjection;
		
		/// <summary> How sensitive the client is to changes in the player's mouse position. </summary>
		public int MouseSensitivity = 30;
		
		public int ChatLines = 12;
		public bool ClickableChat, HideGui, ShowFPS = true;
		internal float HudScale = 1.0f, ChatScale = 1.0f;
		public bool ViewBobbing, UseGuiPng, ShowBlockInHand;
		public bool UseSound, UseMusic;
		
		public Animations Animations;
		internal int CloudsTexId, RainTexId, SnowTexId, GuiTexId;
		internal bool screenshotRequested;
		internal List<WarningScreen> WarningScreens = new List<WarningScreen>();
		internal AcceptedUrls AcceptedUrls = new AcceptedUrls();
		
		/// <summary> Calculates the amount that 2D widgets should be scaled by when rendered. </summary>
		/// <remarks> Affected by both the current resolution of the window, as well as the
		/// scaling specified by the user (field HudScale). </remarks>
		public float GuiScale {
			get {
				float scaleX = Width / 640f, scaleY = Height / 480f;
				return Math.Min( scaleX, scaleY ) * HudScale;
			}
		}
		
		string defTexturePack = "default.zip";
		/// <summary> Gets or sets the path of the default texture pack that should be used by the client. </summary>
		/// <remarks> If the custom default texture pack specified by the user could not be found,
		/// this method returns "default.zip". </remarks>
		public string DefaultTexturePack {
			get { return File.Exists( defTexturePack ) ? defTexturePack : "default.zip"; }
			set {
				defTexturePack = value;
				Options.Set( OptionsKey.DefaultTexturePack, value );
			}
		}
	}
}