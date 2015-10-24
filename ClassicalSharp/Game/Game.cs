using System;
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
using OpenTK.Graphics;

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
		Camera firstPersonCam, thirdPersonCam, forwardThirdPersonCam;
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
		public Events Events = new Events();
		public InputHandler InputHandler;
		public ChatLog Chat;
		
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
		public int MouseSensitivity = 40;
		public int ChatLines = 12;
		public bool HideGui = false, ShowFPS = true;
		public bool PushbackBlockPlacing;
		public Animations Animations;
		internal int CloudsTextureId, RainTextureId, SnowTextureId;
		internal bool screenshotRequested;
		
		void LoadAtlas( Bitmap bmp ) {
			TerrainAtlas1D.Dispose();
			TerrainAtlas.Dispose();
			TerrainAtlas.UpdateState( BlockInfo, bmp );
			TerrainAtlas1D.UpdateState( TerrainAtlas );
		}
		
		public void ChangeTerrainAtlas( Bitmap newAtlas ) {
			LoadAtlas( newAtlas );
			Events.RaiseTerrainAtlasChanged();
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
			ViewDistance = Options.GetInt( OptionsKey.ViewDist, 16, 4096, 512 );
			InputHandler = new InputHandler( this );
			Chat = new ChatLog( this );
			Chat.FontSize = Options.GetInt( OptionsKey.FontSize, 6, 30, 12 );
			Drawer2D = new GdiPlusDrawer2D( Graphics );
			defaultIb = Graphics.MakeDefaultIb();
			MouseSensitivity = Options.GetInt( OptionsKey.Sensitivity, 1, 100, 40 );
			BlockInfo = new BlockInfo();
			BlockInfo.Init();
			ChatLines = Options.GetInt( OptionsKey.ChatLines, 1, 30, 12 );
			
			ModelCache = new ModelCache( this );
			ModelCache.InitCache();
			AsyncDownloader = new AsyncDownloader( skinServer );
			Graphics.PrintGraphicsInfo();
			TerrainAtlas1D = new TerrainAtlas1D( Graphics );
			TerrainAtlas = new TerrainAtlas2D( Graphics, Drawer2D );
			Animations = new Animations( this );
			TexturePackExtractor extractor = new TexturePackExtractor();
			extractor.Extract( defaultTexPack, this );
			Inventory = new Inventory( this );
			
			BlockInfo.SetDefaultBlockPermissions( Inventory.CanPlace, Inventory.CanDelete );
			Map = new Map( this );
			LocalPlayer = new LocalPlayer( this );
			LocalPlayer.SpeedMultiplier = Options.GetInt( OptionsKey.Speed, 1, 50, 10 );
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
			forwardThirdPersonCam = new ForwardThirdPersonCamera( this );
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
			Options.Set( OptionsKey.ViewDist, distance );
			Events.RaiseViewDistanceChanged();
			UpdateProjection();
		}
		
		/// <summary> Gets whether the active screen handles all input. </summary>
		public bool ScreenLockedInput {
			get { return activeScreen != null && activeScreen.HandlesAllInput; }
		}
		
		const int ticksFrequency = 20;
		const double ticksPeriod = 1.0 / ticksFrequency;
		const double imageCheckPeriod = 30.0;
		const double cameraPeriod = 1.0 / 60.0;
		double ticksAccumulator = 0, imageCheckAccumulator = 0, cameraAccumulator = 0;
		
		protected override void OnRenderFrame( FrameEventArgs e ) {
			Graphics.BeginFrame( this );
			Graphics.BindIb( defaultIb );
			accumulator += e.Time;
			Vertices = 0;
			if( !Focused && (activeScreen == null || !activeScreen.HandlesAllInput) ) {
				SetNewScreen( new PauseScreen( this ) );
			}
			
			base.OnRenderFrame( e );
			CheckScheduledTasks( e.Time );
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
				if( SelectedPos.Valid && !HideGui )
					Picking.Render( e.Time, SelectedPos );
				MapRenderer.Render( e.Time );
				SelectionManager.Render( e.Time );
				WeatherRenderer.Render( e.Time );
				InputHandler.PickBlocks( true );
			} else {
				SelectedPos.SetAsInvalid();
			}
			
			Graphics.Mode2D( Width, Height, EnvRenderer is StandardEnvRenderer );
			//OpenTK.Graphics.OpenGL.GL.PolygonMode( 0x0408, 0x1B02 );
			fpsScreen.Render( e.Time );
			if( activeScreen != null ) {
				activeScreen.Render( e.Time );
			}
			Graphics.Mode3D( EnvRenderer is StandardEnvRenderer );
			//if( Keyboard[Key.F2] )
			//	OpenTK.Graphics.OpenGL.GL.PolygonMode( 0x0408, 0x1B01 );
			
			if( screenshotRequested )
				TakeScreenshot();
			Graphics.EndFrame( this );
		}
		
		void CheckScheduledTasks( double time ) {
			imageCheckAccumulator += time;
			ticksAccumulator += time;
			cameraAccumulator += time;
			
			if( imageCheckAccumulator > imageCheckPeriod ) {
				imageCheckAccumulator -= imageCheckPeriod;
				AsyncDownloader.PurgeOldEntries( 10 );
			}
			
			int ticksThisFrame = 0;
			while( ticksAccumulator >= ticksPeriod ) {
				Network.Tick( ticksPeriod );
				Players.Tick( ticksPeriod );
				ParticleManager.Tick( ticksPeriod );
				Animations.Tick( ticksPeriod );
				ticksThisFrame++;
				ticksAccumulator -= ticksPeriod;
			}
			
			while( cameraAccumulator >= cameraPeriod ) {
				Camera.Tick( ticksPeriod );
				cameraAccumulator -= cameraPeriod;
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
		
		protected override void OnResize( object sender, EventArgs e ) {
			base.OnResize( sender, e );
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
		
		internal Screen activeScreen;
		public void SetNewScreen( Screen screen ) {
			InputHandler.ScreenChanged( activeScreen, screen );
			
			if( activeScreen != null )
				activeScreen.Dispose();
			activeScreen = screen;
			if( screen != null )
				screen.Init();
		}
		
		public void SetCamera( bool thirdPerson ) {
			PerspectiveCamera oldCam = (PerspectiveCamera)Camera;
			Camera = (thirdPerson && CanUseThirdPersonCamera) ?
				(Camera is FirstPersonCamera ? thirdPersonCam : forwardThirdPersonCam ) :
				firstPersonCam;
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
		
		public bool IsKeyDown( Key key ) { return InputHandler.IsKeyDown( key ); }
		
		public bool IsKeyDown( KeyMapping mapping ) { return InputHandler.IsKeyDown( mapping ); }
		
		public bool IsMousePressed( MouseButton button ) { return InputHandler.IsMousePressed( button ); }
		
		public Key Mapping( KeyMapping mapping ) { return InputHandler.Keys[mapping]; }
		
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
			Chat.Dispose();
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
		
		internal bool CanPick( byte block ) {
			return !(block == 0 || (BlockInfo.IsLiquid[block] &&
			                        !(Inventory.CanPlace[block] && Inventory.CanDelete[block])));
		}
		
		public Game( string username, string mppass, string skinServer, string defaultTexPack )
			#if USE_DX
			: base( 640, 480, GraphicsMode.Default, Utils.AppName, true, 0, DisplayDevice.Default )
			#else
			: base( 640, 480, GraphicsMode.Default, Utils.AppName, false, 0, DisplayDevice.Default )
			#endif
		{
			Username = username;
			Mppass = mppass;
			this.skinServer = skinServer;
			this.defaultTexPack = defaultTexPack;
			
			if( !File.Exists( defaultTexPack ) ) {
				Utils.LogWarning( defaultTexPack + " not found" );
				this.defaultTexPack = "default.zip";
			}
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