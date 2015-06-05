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
using OpenTK;
using OpenTK.Input;

namespace ClassicalSharp {

	// TODO: Rewrite this so it isn't tied to GameWindow. (so we can use DirectX as backend)
	public partial class Game : GameWindow {
		
		public IGraphicsApi Graphics;
		public Map Map;
		public NetworkProcessor Network;
		
		public Player[] NetPlayers = new Player[256];
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
		
		public CommandManager CommandManager;
		public SelectionManager SelectionManager;
		public ParticleManager ParticleManager;
		public PickingRenderer Picking;
		public PickedPos SelectedPos = new PickedPos();
		public ModelCache ModelCache;
		internal string skinServer, chatInInputBuffer;
		public bool CanUseThirdPersonCamera = true;
		FpsScreen fpsScreen;
		
		int hotbarIndex = 0;
		public bool CanChangeHeldBlock = true;
		public Block[] BlocksHotbar = new Block[] { Block.Stone, Block.Cobblestone,
			Block.Brick, Block.Dirt, Block.WoodenPlanks, Block.Wood, Block.Leaves, Block.Glass, Block.Slab };
		
		public int HeldBlockIndex {
			get { return hotbarIndex; }
			set {
				if( !CanChangeHeldBlock ) {
					AddChat( "&e/client: &cThe server has forbidden you from changing your held block." );
					return;
				}
				hotbarIndex = value;
				RaiseHeldBlockChanged();
			}
		}
		
		public Block HeldBlock {
			get { return BlocksHotbar[hotbarIndex]; }
			set {
				if( !CanChangeHeldBlock ) {
					AddChat( "&e/client: &cThe server has forbidden you from changing your held block." );
					return;
				}
				BlocksHotbar[hotbarIndex] = value;
				RaiseHeldBlockChanged();
			}
		}
		
		public bool[] CanPlace = new bool[256];
		public bool[] CanDelete = new bool[256];
		
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
		
		void LoadAtlas( Bitmap bmp ) {
			TerrainAtlas1D.Dispose();
			TerrainAtlas.Dispose();			
			TerrainAtlas.UpdateState( bmp );						
			TerrainAtlas1D.UpdateState( TerrainAtlas );
		}
		
		public void ChangeTerrainAtlas( Bitmap newAtlas ) {
			LoadAtlas( newAtlas );
			RaiseEvent( TerrainAtlasChanged );
		}
		
		void PrintGraphicsInfo() {
			Console.ForegroundColor = ConsoleColor.Green;
			Graphics.PrintApiSpecificInfo();
			Console.WriteLine( "Max 2D texture dimensions: " + Graphics.MaxTextureDimensions );
			Console.WriteLine( "== End of graphics info ==" );
			Console.ResetColor();
		}
		
		protected override void OnLoad( EventArgs e ) {
			#if !USE_DX
			Graphics = new OpenGLApi();
			#else
			Graphics = new DirectXApi( this );
			#endif
			ModelCache = new ModelCache( this );
			AsyncDownloader = new AsyncDownloader( skinServer );
			PrintGraphicsInfo();
			Bitmap terrainBmp = new Bitmap( "terrain.png" );
			TerrainAtlas1D = new TerrainAtlas1D( Graphics );
			TerrainAtlas = new TerrainAtlas2D( Graphics );
			LoadAtlas( terrainBmp );
			BlockInfo = new BlockInfo();
			BlockInfo.Init();
			BlockInfo.SetDefaultBlockPermissions( CanPlace, CanDelete );
			Map = new Map( this );
			LocalPlayer = new LocalPlayer( 255, this );
			width = Width;
			height = Height;
			MapRenderer = new MapRenderer( this );
			MapEnvRenderer = new MapEnvRenderer( this );
			EnvRenderer = new StandardEnvRenderer( this );
			Network = new NetworkProcessor( this );
			firstPersonCam = new FirstPersonCamera( this );
			thirdPersonCam = new ThirdPersonCamera( this );
			Camera = firstPersonCam;
			CommandManager = new CommandManager();
			CommandManager.Init( this );
			SelectionManager = new SelectionManager( this );
			ParticleManager = new ParticleManager( this );
			WeatherRenderer = new WeatherRenderer( this );
			WeatherRenderer.Init();
			
			VSync = VSyncMode.On;
			Graphics.DepthTest = true;
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
			Network.Connect();
		}
		
		public void SetViewDistance( int distance ) {
			ViewDistance = distance;
			Utils.LogDebug( "setting view distance to: " + distance );
			RaiseEvent( ViewDistanceChanged );
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
			accumulator += e.Time;
			imageCheckAccumulator += e.Time;
			ticksAccumulator += e.Time;
			Vertices = 0;
			if( !Focused && !( activeScreen is PauseScreen ) && !Map.IsNotLoaded ) {
				SetNewScreen( new PauseScreen( this ) );
			}
			
			if( imageCheckAccumulator > imageCheckPeriod ) {
				imageCheckAccumulator -= imageCheckPeriod;
				AsyncDownloader.PurgeOldEntries( 10 );
			}
			base.OnRenderFrame( e );
			
			int ticksThisFrame = 0;
			while( ticksAccumulator >= ticksPeriod ) {
				Network.Tick( ticksPeriod );
				LocalPlayer.Tick( ticksPeriod );
				Camera.Tick( ticksPeriod );
				ParticleManager.Tick( ticksPeriod );
				for( int i = 0; i < NetPlayers.Length; i++ ) {
					if( NetPlayers[i] != null ) {
						NetPlayers[i].Tick( ticksPeriod );
					}
				}
				ticksThisFrame++;
				ticksAccumulator -= ticksPeriod;
			}
			if( ticksThisFrame > ticksFrequency / 3 ) {
				Utils.LogWarning( "Falling behind (did {0} ticks this frame)", ticksThisFrame );
			}
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
				RenderPlayers( e.Time, t );
				ParticleManager.Render( e.Time, t );
				Camera.GetPickedBlock( SelectedPos ); // TODO: only pick when necessary
				Picking.Render( e.Time );				
				EnvRenderer.Render( e.Time );			
				MapRenderer.Render( e.Time );
				WeatherRenderer.Render( e.Time );
				SelectionManager.Render( e.Time );
				bool left = IsMousePressed( MouseButton.Left );
				bool right = IsMousePressed( MouseButton.Right );
				bool middle = IsMousePressed( MouseButton.Middle );
				PickBlocks( true, left, right, middle );
			} else {
				SelectedPos.Valid = false;
			}
			
			Graphics.Mode2D( Width, Height );
			fpsScreen.Render( e.Time );
			if( activeScreen != null ) {
				activeScreen.Render( e.Time );
			}
			Graphics.Mode3D();
			
			if( screenshotRequested ) {
				if( !Directory.Exists( "screenshots" ) ) {
					Directory.CreateDirectory( "screenshots" );
				}
				string path = Path.Combine( "screenshots", "screenshot_" + DateTime.Now.ToString( dateFormat ) + ".png" );
				Graphics.TakeScreenshot( path, ClientSize );
				screenshotRequested = false;
			}
			Graphics.EndFrame( this );
		}
		
		void RenderPlayers( double deltaTime, float t ) {
			//Graphics.AlphaTest = true;			
			for( int i = 0; i < NetPlayers.Length; i++ ) {
				if( NetPlayers[i] != null ) {
					NetPlayers[i].Render( deltaTime, t );
				}
			}
			LocalPlayer.Render( deltaTime, t );
			//Graphics.AlphaTest = false;
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
			for( int i = 0; i < NetPlayers.Length; i++ ) {
				if( NetPlayers[i] != null ) {
					NetPlayers[i].Despawn();
				}
			}
			LocalPlayer.Despawn();
			Graphics.CheckResources();
			AsyncDownloader.Dispose();
			if( writer != null ) {
				writer.Close();
			}
			Graphics.Dispose();
			Utils2D.Dispose();
			base.Dispose();
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
			Graphics.OnWindowResize( Width, Height );
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
			RaiseOnNewMap();
			Map.mapData = null;
			GC.Collect();
		}
		
		Screen activeScreen;
		public void SetNewScreen( Screen screen ) {
			if( activeScreen != null ) {
				activeScreen.Dispose();
			}
			if( activeScreen != null && activeScreen.HandlesAllInput ) {
				Camera.RegrabMouse();
				lastClick = DateTime.UtcNow;
			}
			activeScreen = screen;
			if( screen != null ) {
				screen.Window = this;
				screen.Init();
			}
		}
		
		public void SetCamera( bool thirdPerson ) {
			Camera = ( thirdPerson && CanUseThirdPersonCamera ) ? thirdPersonCam : firstPersonCam;
			UpdateProjection();
		}
		
		public void UpdateBlock( int x, int y, int z, byte block ) {
			int oldHeight = Map.GetLightHeight( x, z );
			Map.SetBlock( x, y, z, block );
			int newHeight = Map.GetLightHeight( x, z );
			MapRenderer.RedrawBlock( x, y, z, block, oldHeight, newHeight );
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