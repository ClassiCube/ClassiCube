﻿using System;
using System.Drawing;
using System.IO;
using System.Net;
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

	// TODO: Rewrite this so it isn't tied to GameWindow. (so we can use DirectX as backend)
	public partial class Game : GameWindow {
		
		public IGraphicsApi Graphics;
		public Map Map;
		public INetworkProcessor Network;
		
		public EntityList Players = new EntityList();
		public CpeListInfo[] CpePlayersList = new CpeListInfo[256];
		public LocalPlayer LocalPlayer;
		public Camera Camera;
		Camera firstPersonCam, thirdPersonCam;
		public BlockInfo BlockInfo;
		public double accumulator;
		public TerrainAtlas2D TerrainAtlas;
		public TerrainAtlas1D TerrainAtlas1D;
		public SkinType DefaultPlayerSkinType;
		public int ChunkUpdates;
		
		public MapRenderer MapRenderer;
		public MapEnvRenderer MapEnvRenderer;
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
		internal string skinServer, chatInInputBuffer, defaultTexPack;
		internal int defaultIb;
		public bool CanUseThirdPersonCamera = true;
		FpsScreen fpsScreen;
		
		public IPAddress IPAddress;
		public string Username;
		public string Mppass;
		public int Port;
		public int ViewDistance = 512;
		
		public long Vertices;
		public FrustumCulling Culling;
		int width, height;
		public AsyncDownloader AsyncDownloader;
		public Matrix4 View, Projection;
		public int MouseSensitivity = 30;
		public bool HideGui = false;
		public Animations Animations;
		internal int CloudsTextureId, RainTextureId, SnowTextureId;
		
		void LoadAtlas( Bitmap bmp ) {
			TerrainAtlas1D.Dispose();
			TerrainAtlas.Dispose();
			TerrainAtlas.UpdateState( bmp );
			TerrainAtlas1D.UpdateState( TerrainAtlas );
		}
		
		public void ChangeTerrainAtlas( Bitmap newAtlas ) {
			LoadAtlas( newAtlas );
			Raise( TerrainAtlasChanged );
		}
		
		public Game( string username, string mppass, string skinServer, string defaultTexPack ) : base() {
			Username = username;
			Mppass = mppass;
			this.skinServer = skinServer;
			this.defaultTexPack = defaultTexPack;
		}
		
		protected override void OnLoad( EventArgs e ) {
			#if !USE_DX
			Graphics = new OpenGLApi();
			#else
			Graphics = new Direct3D9Api( this );
			#endif
			try {
				Options.Load();
			} catch( IOException ) {
				Utils.LogWarning( "Unable to load options.txt" );
			}
			Keys = new KeyMap();
			Drawer2D = new GdiPlusDrawer2D( Graphics );
			ViewDistance = Options.GetInt( "viewdist", 16, 8192, 512 );
			defaultIb = Graphics.MakeDefaultIb();
			ModelCache = new ModelCache( this );
			ModelCache.InitCache();
			AsyncDownloader = new AsyncDownloader( skinServer );
			Graphics.PrintGraphicsInfo();
			TerrainAtlas1D = new TerrainAtlas1D( Graphics );
			TerrainAtlas = new TerrainAtlas2D( Graphics );
			Animations = new Animations( this );
			TexturePackExtractor extractor = new TexturePackExtractor();
			extractor.Extract( defaultTexPack, this );
			Inventory = new Inventory( this );
			
			BlockInfo = new BlockInfo();
			BlockInfo.Init();
			BlockInfo.SetDefaultBlockPermissions( Inventory.CanPlace, Inventory.CanDelete );
			Map = new Map( this );
			LocalPlayer = new LocalPlayer( this );
			Players[255] = LocalPlayer;
			width = Width;
			height = Height;
			MapRenderer = new MapRenderer( this );
			MapEnvRenderer = new MapEnvRenderer( this );
			EnvRenderer = new StandardEnvRenderer( this );
			if( IPAddress == null ) {
				Network = new Singleplayer.SinglePlayerServer( this );
			} else {
				Network = new NetworkProcessor( this );
			}
			Graphics.LostContextFunction = Network.Tick;
			
			firstPersonCam = new FirstPersonCamera( this );
			thirdPersonCam = new ThirdPersonCamera( this );
			Camera = firstPersonCam;
			CommandManager = new CommandManager();
			CommandManager.Init( this );
			SelectionManager = new SelectionManager( this );
			ParticleManager = new ParticleManager( this );
			WeatherRenderer = new WeatherRenderer( this );
			WeatherRenderer.Init();
			
			Graphics.SetVSync( this, true );
			Graphics.DepthTest = true;
			Graphics.DepthTestFunc( CompareFunc.LessEqual );
			//Graphics.DepthWrite = true;
			Graphics.AlphaBlendFunc( BlendFunc.SourceAlpha, BlendFunc.InvSourceAlpha );
			Graphics.AlphaTestFunc( CompareFunc.Greater, 0.5f );
			RegisterInputHandlers();
			Title = Utils.AppName;
			fpsScreen = new FpsScreen( this );
			fpsScreen.Init();
			Culling = new FrustumCulling();
			EnvRenderer.Init();
			MapEnvRenderer.Init();
			Picking = new PickingRenderer( this );
			string connectString = "Connecting to " + IPAddress + ":" + Port +  "..";
			SetNewScreen( new LoadingMapScreen( this, connectString, "Reticulating splines" ) );
			Network.Connect( IPAddress, Port );
		}
		
		public void SetViewDistance( int distance ) {
			ViewDistance = distance;
			Utils.LogDebug( "setting view distance to: " + distance );
			Options.Set( "viewdist", distance.ToString() );
			Raise( ViewDistanceChanged );
			UpdateProjection();
		}
		
		/// <summary> Gets whether the active screen handles all input. </summary>
		public bool ScreenLockedInput {
			get { return activeScreen != null && activeScreen.HandlesAllInput; }
		}
		
		const int ticksFrequency = 20;
		const double ticksPeriod = 1.0 / ticksFrequency;
		const double imageCheckPeriod = 30.0;
		double ticksAccumulator = 0, imageCheckAccumulator = 0;
		
		protected override void OnRenderFrame( FrameEventArgs e ) {
			Graphics.BeginFrame( this );
			Graphics.BindIb( defaultIb );
			accumulator += e.Time;
			imageCheckAccumulator += e.Time;
			ticksAccumulator += e.Time;
			Vertices = 0;
			if( !Focused && ( activeScreen == null || !activeScreen.HandlesAllInput ) ) {
				SetNewScreen( new PauseScreen( this ) );
			}
			
			base.OnRenderFrame( e );
			CheckScheduledTasks();
			float t = (float)( ticksAccumulator / ticksPeriod );
			LocalPlayer.SetInterpPosition( t );
			
			Graphics.Clear();
			Graphics.SetMatrixMode( MatrixType.Modelview );
			Matrix4 modelView = Camera.GetView();
			View = modelView;
			Graphics.LoadMatrix( ref modelView );
			Culling.CalcFrustumEquations( ref Projection, ref modelView );
			
			bool visible = activeScreen == null || !activeScreen.BlocksWorld;
			if( visible ) {
				Players.Render( e.Time, t );
				ParticleManager.Render( e.Time, t );
				Camera.GetPickedBlock( SelectedPos ); // TODO: only pick when necessary
				EnvRenderer.Render( e.Time );
				MapRenderer.Render( e.Time );
				if( SelectedPos.Valid )
					Picking.Render( e.Time, SelectedPos );
				WeatherRenderer.Render( e.Time );
				SelectionManager.Render( e.Time );
				bool left = IsMousePressed( MouseButton.Left );
				bool right = IsMousePressed( MouseButton.Right );
				bool middle = IsMousePressed( MouseButton.Middle );
				PickBlocks( true, left, right, middle );
			} else {
				SelectedPos.SetAsInvalid();
			}
			
			Graphics.Mode2D( Width, Height );
			fpsScreen.Render( e.Time );
			if( activeScreen != null ) {
				activeScreen.Render( e.Time );
			}
			Graphics.Mode3D();
			
			if( screenshotRequested )
				TakeScreenshot();
			Graphics.EndFrame( this );
		}
		
		void CheckScheduledTasks() {
			if( imageCheckAccumulator > imageCheckPeriod ) {
				imageCheckAccumulator -= imageCheckPeriod;
				AsyncDownloader.PurgeOldEntries( 10 );
			}
			
			int ticksThisFrame = 0;
			while( ticksAccumulator >= ticksPeriod ) {
				Network.Tick( ticksPeriod );
				Players.Tick( ticksPeriod );
				Camera.Tick( ticksPeriod );
				ParticleManager.Tick( ticksPeriod );
				Animations.Tick( ticksPeriod );
				ticksThisFrame++;
				ticksAccumulator -= ticksPeriod;
			}
			
			if( ticksThisFrame > ticksFrequency / 3 ) {
				Utils.LogWarning( "Falling behind (did {0} ticks this frame)", ticksThisFrame );
			}
		}
		
		void TakeScreenshot() {
			if( !Directory.Exists( "screenshots" ) ) {
				Directory.CreateDirectory( "screenshots" );
			}
			string timestamp = DateTime.Now.ToString( "dd-MM-yyyy-HH-mm-ss" );
			string path = Path.Combine( "screenshots", "screenshot_" + timestamp + ".png" );
			Graphics.TakeScreenshot( path, ClientSize );
			screenshotRequested = false;
		}
		
		public void UpdateProjection() {
			Matrix4 projection = Camera.GetProjection();
			Projection = projection;
			Graphics.SetMatrixMode( MatrixType.Projection );
			Graphics.LoadMatrix( ref projection );
			Graphics.SetMatrixMode( MatrixType.Modelview );
		}
		
		protected override void OnResize( EventArgs e ) {
			base.OnResize( e );
			Graphics.OnWindowResize( this );
			UpdateProjection();
			if( activeScreen != null ) {
				activeScreen.OnResize( width, height, Width, Height );
			}
			width = Width;
			height = Height;
		}
		
		public void Disconnect( string title, string reason ) {
			SetNewScreen( new ErrorScreen( this, title, reason ) );
			Map.Reset();
			Map.mapData = null;
			GC.Collect();
		}
		
		Screen activeScreen;
		public void SetNewScreen( Screen screen ) {
			if( activeScreen != null )
				activeScreen.Dispose();
			if( activeScreen != null && activeScreen.HandlesAllInput )
				lastClick = DateTime.UtcNow;
			
			activeScreen = screen;
			if( screen != null )
				screen.Init();
			if( Network.UsingPlayerClick ) {
				byte targetId = Players.GetClosetPlayer( this );
				ButtonStateChanged( MouseButton.Left, false, targetId );
				ButtonStateChanged( MouseButton.Right, false, targetId );
				ButtonStateChanged( MouseButton.Middle, false, targetId );
			}
		}
		
		public void SetCamera( bool thirdPerson ) {
			PerspectiveCamera oldCam = (PerspectiveCamera)Camera;
			Camera = ( thirdPerson && CanUseThirdPersonCamera ) ? thirdPersonCam : firstPersonCam;
			PerspectiveCamera newCam = (PerspectiveCamera)Camera;
			newCam.delta = oldCam.delta;
			newCam.previous = oldCam.previous;
			UpdateProjection();
		}
		
		public void UpdateBlock( int x, int y, int z, byte block ) {
			int oldHeight = Map.GetLightHeight( x, z ) + 1;
			Map.SetBlock( x, y, z, block );
			int newHeight = Map.GetLightHeight( x, z ) + 1;
			MapRenderer.RedrawBlock( x, y, z, block, oldHeight, newHeight );
		}
		
		public override void Dispose() {
			MapRenderer.Dispose();
			MapEnvRenderer.Dispose();
			EnvRenderer.Dispose();
			WeatherRenderer.Dispose();
			SetNewScreen( null );
			fpsScreen.Dispose();
			SelectionManager.Dispose();
			TerrainAtlas.Dispose();
			TerrainAtlas1D.Dispose();
			ModelCache.Dispose();
			Picking.Dispose();
			ParticleManager.Dispose();
			Players.Dispose();
			AsyncDownloader.Dispose();
			if( writer != null ) {
				writer.Dispose();
			}
			if( activeScreen != null ) {
				activeScreen.Dispose();
			}
			Graphics.DeleteIb( defaultIb );
			Graphics.Dispose();
			Drawer2D.DisposeInstance();
			Animations.Dispose();
			Graphics.DeleteTexture( ref CloudsTextureId );
			Graphics.DeleteTexture( ref RainTextureId );
			Graphics.DeleteTexture( ref SnowTextureId );
			
			if( Options.HasChanged ) {
				try {
					Options.Save();
				} catch( IOException ) {
					Utils.LogWarning( "Unable to save options.txt" );
				}
			}
			base.Dispose();
		}
	}
	
	public sealed class CpeListInfo {
		
		public byte NameId;
		
		/// <summary> Unformatted name of the player for autocompletion, etc. </summary>
		/// <remarks> Colour codes are always removed from this. </remarks>
		public string PlayerName;
		
		/// <summary> Formatted name for display in the player list. </summary>
		/// <remarks> Can include colour codes. </remarks>
		public string ListName;
		
		/// <summary> Name of the group this player is in. </summary>
		/// <remarks> Can include colour codes. </remarks>
		public string GroupName;
		
		/// <summary> Player's rank within the group. (0 is highest) </summary>
		/// <remarks> Multiple group members can share the same rank,
		/// so a player's group rank is not a unique identifier. </remarks>
		public byte GroupRank;
		
		public CpeListInfo( byte id, string playerName, string listName,
		                   string groupName, byte groupRank ) {
			NameId = id;
			PlayerName = playerName;
			ListName = listName;
			GroupName = groupName;
			GroupRank = groupRank;
		}
	}
}