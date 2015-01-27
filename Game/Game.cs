using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Net;
using ClassicalSharp.Commands;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Model;
using ClassicalSharp.Network;
using ClassicalSharp.Particles;
using ClassicalSharp.Renderers;
using OpenTK;
using OpenTK.Input;

namespace ClassicalSharp {

	// TODO: Rewrite this so it isn't tied to GameWindow. (so we can use DirectX as backend)
	public partial class Game : GameWindow {
		
		public IGraphicsApi Graphics;
		public Map Map;
		public NetworkProcessor Network;
		
		public Player[] NetPlayers = new Player[256];
		public Dictionary<string, PlayerListInfo> PlayersList = new Dictionary<string, PlayerListInfo>();
		public LocalPlayer LocalPlayer;
		public Camera Camera;
		Camera firstPersonCam, thirdPersonCam;
		public BlockInfo BlockInfo;
		public double accumulator;
		public TextureAtlas2D TerrainAtlas;
		public int TerrainAtlasTexId = -1;
		public TextureAtlas1D TerrainAtlas1D;
		public int[] TerrainAtlas1DTexIds;
		public SkinType DefaultPlayerSkinType;
		public int ChunkUpdates;
		
		public MapRenderer MapRenderer;
		public EnvRenderer EnvRenderer;
		
		public CommandManager CommandManager;
		public ParticleManager ParticleManager;
		public PickingRenderer Picking;
		public PickedPos SelectedPos;
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
				if( !CanChangeHeldBlock )
					throw new InvalidOperationException( "Server has forbidden changing held block." );
				hotbarIndex = value;
				RaiseHeldBlockChanged();
			}
		}
		
		public Block HeldBlock {
			get { return BlocksHotbar[hotbarIndex]; }
			set {
				if( !CanChangeHeldBlock )
					throw new InvalidOperationException( "Server has forbidden changing held block." );
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
		
		void LoadAtlas( Bitmap bmp ) {
			// Cleanup old atlas if applicable.
			if( TerrainAtlasTexId != -1 ) {
				Graphics.DeleteTexture( TerrainAtlasTexId );
			}
			if( TerrainAtlas1DTexIds != null ) {
				for( int i = 0; i < TerrainAtlas1DTexIds.Length; i++ ) {
					Graphics.DeleteTexture( TerrainAtlas1DTexIds[i] );
				}
			}
			if( TerrainAtlas != null ) {
				TerrainAtlas.AtlasBitmap.Dispose();
			}
			
			TerrainAtlas = new TextureAtlas2D( Graphics, bmp, 16, 16, 5 );
			using( FastBitmap fastBmp = new FastBitmap( bmp, true ) ) {
				BlockInfo.MakeOptimisedTexture( fastBmp );
			}
			TerrainAtlasTexId = Graphics.LoadTexture( TerrainAtlas.AtlasBitmap );
			int elementsPerBitmap;
			int size = Math.Min( 2048, Graphics.MaxTextureDimensions );
			Bitmap[] bmps = TerrainAtlas.Into1DAtlases( size, out elementsPerBitmap );
			Utils.LogDebug( "Loaded new atlas: {0} bmps, {1} per bmp", bmps.Length, elementsPerBitmap );
			
			TerrainAtlas1D = new TextureAtlas1D( Graphics, bmps, elementsPerBitmap );
			TerrainAtlas1DTexIds = new int[bmps.Length];
			for( int i = 0; i < bmps.Length; i++ ) {
				Bitmap bmp1D = bmps[i];
				if( Graphics.SupportsNonPowerOf2Textures ) {
					TerrainAtlas1DTexIds[i] = Graphics.LoadTexture( bmp1D );
				} else {
					using( Bitmap adjBmp = Utils2D.ResizeToPower2( bmp1D ) ) {
						TerrainAtlas1DTexIds[i] = Graphics.LoadTexture( adjBmp );
					}
				}
				bmp1D.Dispose();
			}
		}
		
		public void ChangeTerrainAtlas( Bitmap newAtlas ) {
			LoadAtlas( newAtlas );
			RaiseEvent( TerrainAtlasChanged );
		}
		
		void PrintGraphicsInfo() {
			Console.ForegroundColor = ConsoleColor.Green;
			Graphics.PrintApiSpecificInfo();
			Console.WriteLine( "Max 2D texture dimensions: " + Graphics.MaxTextureDimensions );
			Console.WriteLine( "Supports non power of 2 textures: " + Graphics.SupportsNonPowerOf2Textures );
			Console.WriteLine( "== End of graphics info ==" );
			Console.ResetColor();
		}
		
		protected override void OnLoad( EventArgs e ) {
			Graphics = new OpenGLApi();
			ModelCache = new ModelCache( this );
			AsyncDownloader = new AsyncDownloader( skinServer );
			PrintGraphicsInfo();
			Bitmap terrainBmp = new Bitmap( "terrain.png" );
			LoadAtlas( terrainBmp );
			BlockInfo = new BlockInfo();
			BlockInfo.Init();
			BlockInfo.SetDefaultBlockPermissions( CanPlace, CanDelete );
			Map = new Map( this );
			LocalPlayer = new LocalPlayer( 255, this );
			width = Width;
			height = Height;
			MapRenderer = new MapRenderer( this );
			EnvRenderer = new NormalEnvRenderer( this );
			Network = new NetworkProcessor( this );
			firstPersonCam = new FirstPersonCamera( this );
			thirdPersonCam = new ThirdPersonCamera( this );
			Camera = firstPersonCam;
			CommandManager = new CommandManager();
			CommandManager.Init( this );
			ParticleManager = new ParticleManager( this );
			
			VSync = VSyncMode.On;
			Graphics.DepthTest = true;
			//Graphics.DepthWrite = true;
			Graphics.AlphaBlendFunc( BlendFunc.SourceAlpha, BlendFunc.InvSourceAlpha );
			Graphics.AlphaTestFunc( AlphaFunc.Greater, 0.5f );
			RegisterInputHandlers();
			Title = Utils.AppName;
			fpsScreen = new FpsScreen( this );
			fpsScreen.Init();
			Culling = new FrustumCulling();
			EnvRenderer.Init();
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
			accumulator += e.Time;
			imageCheckAccumulator += e.Time;
			ticksAccumulator += e.Time;
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
			LocalPlayer.SetInterpPosition( (float)( ticksAccumulator / ticksPeriod ) );
			
			Graphics.Clear();
			Graphics.SetMatrixMode( MatrixType.Modelview );
			Matrix4 modelView = Camera.GetView();
			View = modelView;
			Graphics.LoadMatrix( ref modelView );
			Culling.CalcFrustumEquations( ref Projection, ref modelView );
			
			bool visible = activeScreen == null || !activeScreen.BlocksWorld;
			if( visible ) {
				//EnvRenderer.EnableAmbientLighting();
				float t = (float)( ticksAccumulator / ticksPeriod );
				RenderPlayers( e.Time, t );
				ParticleManager.Render( e.Time, t );
				SelectedPos = Camera.GetPickedPos(); // TODO: only pick when necessary
				Picking.Render( e.Time );
				EnvRenderer.Render( e.Time );
				MapRenderer.Render( e.Time );
				bool left = IsMousePressed( MouseButton.Left );
				bool right = IsMousePressed( MouseButton.Right );
				PickBlocks( true, left, right );
				//EnvRenderer.DisableAmbientLighting();
			} else {
				SelectedPos = null;
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
			SwapBuffers();
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
			EnvRenderer.Dispose();
			SetNewScreen( null );
			fpsScreen.Dispose();
			Graphics.DeleteTexture( TerrainAtlasTexId );
			for( int i = 0; i < TerrainAtlas1DTexIds.Length; i++ ) {
				Graphics.DeleteTexture( TerrainAtlas1DTexIds[i] );
			}
			ModelCache.Dispose();
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
	
	public sealed class PlayerListInfo {
		
		public string Name;
		
		public short Ping;
		
		public PlayerListInfo( string name, short ping ) {
			Name = name;
			Ping = ping;
		}
	}
}