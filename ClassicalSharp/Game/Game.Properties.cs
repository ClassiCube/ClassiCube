// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Net;
using ClassicalSharp.Audio;
using ClassicalSharp.Commands;
using ClassicalSharp.Entities;
using ClassicalSharp.GraphicsAPI;
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
	
	public delegate void ScheduledTaskCallback(ScheduledTask task);	
	/// <summary> Represents a task that runs on the main thread every certain interval. </summary>
	public class ScheduledTask {
		public double Accumulator, Interval;
		public ScheduledTaskCallback Callback;
	}
	
	public partial class Game {
		
		public IGraphicsApi Graphics;
		public World World;		
		public IServerConnection Server;
		
		public EntityList Entities;
		public LocalPlayer LocalPlayer;
		public TabList TabList;

		public Camera Camera;
		public List<Camera> Cameras = new List<Camera>();
		
		/// <summary> Total rendering time(in seconds) elapsed since the client was started. </summary>
		public double accumulator;
		
		public int ChunkUpdates;
		
		public bool CameraClipping = true;
		
		public IWorldLighting Lighting;
		
		public MapRenderer MapRenderer;
		public ChunkUpdater ChunkUpdater;
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
		internal string skinServer;
		public InputHandler Input;
		public Chat Chat;
		public HeldBlockRenderer HeldBlockRenderer;
		public AudioPlayer AudioPlayer;
		public AxisLinesRenderer AxisLinesRenderer;
		public SkyboxRenderer SkyboxRenderer;
		
		public List<IGameComponent> Components = new List<IGameComponent>();
		public List<ScheduledTask> Tasks = new List<ScheduledTask>();

		/// <summary> Whether x to stone brick tiles should be used. </summary>
		public bool SupportsCPEBlocks = false;
		
		public string Username;
		public string Mppass;

		public IPAddress IPAddress;
		public int Port;
		
		/// <summary> Radius of the sphere the player can see around the position of the current camera. </summary>
		public int ViewDistance = 512;
		internal int MaxViewDistance = 32768, UserViewDistance = 512;
		
		public int Fov = 70;
		internal int DefaultFov, ZoomFov;
		
		public FpsLimitMethod FpsLimit;
		
		public bool ShowAxisLines;
	
		/// <summary> Whether players should animate using simple swinging parallel to their bodies. </summary>
		public bool SimpleArmsAnim;

		/// <summary> Whether the arm model should use the classic position. </summary>
		public bool ClassicArmModel;

		public bool InvertMouse;
		
		public int Vertices;
		public FrustumCulling Culling;
		public AsyncDownloader Downloader;
		
		/// <summary> How sensitive the client is to changes in the player's mouse position. </summary>
		public int MouseSensitivity = 30;
		
		public bool TabAutocomplete;
		
		public bool UseClassicGui, UseClassicTabList, UseClassicOptions, ClassicMode, ClassicHacks;
		
		public bool PureClassic { get { return ClassicMode && !ClassicHacks; } }
		
		public bool AllowCustomBlocks, UseCPE, AllowServerTextures;
		
		public bool SmoothLighting;
		
		public bool ChatLogging;
		
		public bool AutoRotate = true;
		
		public bool SmoothCamera;
		
		public string FontName = "Arial";
		
		public int MaxChunkUpdates = 30;
		
		public int ChatLines = 12;
		public bool ClickableChat, HideGui, ShowFPS;
		internal float HotbarScale = 1, ChatScale = 1, InventoryScale = 1;
		public bool ViewBobbing, ShowBlockInHand;
		public bool BreakableLiquids;
		public int SoundsVolume, MusicVolume;
		
		public Vector3 CurrentCameraPos;
		
		public Animations Animations;
		internal bool screenshotRequested;
		
		/// <summary> Calculates the amount that the hotbar widget should be scaled by when rendered. </summary>
		/// <remarks> Affected by both the current resolution of the window, as well as the
		/// scaling specified by the user (field HotbarScale). </remarks>
		public float GuiHotbarScale { get { return Scale(MinWindowScale  * HotbarScale); } }
		
		/// <summary> Calculates the amount that the block inventory menu should be scaled by when rendered. </summary>
		/// <remarks> Affected by both the current resolution of the window, as well as the
		/// scaling specified by the user (field InventoryScale). </remarks>
		public float GuiInventoryScale { get { return Scale(MinWindowScale * (InventoryScale * 0.5f)); } }
		
		/// <summary> Calculates the amount that 2D chat widgets should be scaled by when rendered. </summary>
		/// <remarks> Affected by both the current resolution of the window, as well as the
		/// scaling specified by the user (field ChatScale). </remarks>
		public float GuiChatScale { get { return Scale(MinWindowScale * ChatScale); } }
		
		int MinWindowScale { get { return 1 + (int)(Math.Floor(Math.Min(Width / 640f, Height / 480f))); } }
		
		public float Scale(float value) { 
			return (float)Math.Round(value * 10, MidpointRounding.AwayFromZero) / 10; 
		}
		
		string defTexturePack = "default.zip";
		/// <summary> Gets or sets the path of the default texture pack that should be used by the client. </summary>
		/// <remarks> If the custom default texture pack specified by the user could not be found,
		/// this method returns "default.zip". </remarks>
		public string DefaultTexturePack {
			get {
				string texPath = Path.Combine("texpacks", defTexturePack);
				return Platform.FileExists(texPath) && !ClassicMode ? defTexturePack : "default.zip"; 
			}
			set {
				defTexturePack = value;
				Options.Set(OptionsKey.DefaultTexturePack, value);
			}
		}
		
		public INativeWindow window;		
		public int Width, Height;
	}
}