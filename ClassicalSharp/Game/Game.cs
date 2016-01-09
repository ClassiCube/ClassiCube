using System;
using System.Drawing;
using System.IO;
using System.Net;
using System.Threading;
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
using OpenTK.Graphics;
using OpenTK.Input;
#if ANDROID
using Android.Graphics;
#endif

namespace ClassicalSharp {

	public partial class Game : GameWindow {
		
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
			Graphics.MakeGraphicsInfo();
			Players = new EntityList( this );
			
			Options.Load();
			AcceptedUrls.Load(); DeniedUrls.Load();
			ViewDistance = Options.GetInt( OptionsKey.ViewDist, 16, 4096, 512 );
			UserViewDistance = ViewDistance;
			CameraClipping = Options.GetBool( OptionsKey.CameraClipping, true );
			InputHandler = new InputHandler( this );
			Chat = new ChatLog( this );
			ParticleManager = new ParticleManager( this );
			HudScale = Options.GetFloat( OptionsKey.HudScale, 0.25f, 5f, 1f );
			ChatScale = Options.GetFloat( OptionsKey.ChatScale, 0.35f, 5f, 1f );
			defaultIb = Graphics.MakeDefaultIb();
			MouseSensitivity = Options.GetInt( OptionsKey.Sensitivity, 1, 100, 30 );
			UseClassicGui = Options.GetBool( OptionsKey.UseClassicGui, true );
			BlockInfo = new BlockInfo();
			BlockInfo.Init();
			ChatLines = Options.GetInt( OptionsKey.ChatLines, 1, 30, 12 );
			ClickableChat = Options.GetBool( OptionsKey.ClickableChat, false );
			ModelCache = new ModelCache( this );
			ModelCache.InitCache();
			AsyncDownloader = new AsyncDownloader( skinServer );
			Drawer2D = new GdiPlusDrawer2D( Graphics );
			Drawer2D.UseBitmappedChat = !Options.GetBool( OptionsKey.ArialChatFont, false );
			ViewBobbing = Options.GetBool( OptionsKey.ViewBobbing, false );
			ShowBlockInHand = Options.GetBool( OptionsKey.ShowBlockInHand, true );
			InvertMouse = Options.GetBool( OptionsKey.InvertMouse, false );
			SimpleArmsAnim = Options.GetBool( OptionsKey.SimpleArmsAnim, false );
			
			TerrainAtlas1D = new TerrainAtlas1D( Graphics );
			TerrainAtlas = new TerrainAtlas2D( Graphics, Drawer2D );
			Animations = new Animations( this );
			defTexturePack = Options.Get( OptionsKey.DefaultTexturePack ) ?? "default.zip";
			TexturePackExtractor extractor = new TexturePackExtractor();
			extractor.Extract( "default.zip", this );
			// in case the user's default texture pack doesn't have all required textures
			if( defTexturePack != "default.zip" )
				extractor.Extract( DefaultTexturePack, this );
			Inventory = new Inventory( this );
			
			BlockInfo.SetDefaultBlockPermissions( Inventory.CanPlace, Inventory.CanDelete );
			Map = new Map( this );
			LocalPlayer = new LocalPlayer( this );
			Players[255] = LocalPlayer;
			width = Width;
			height = Height;
			MapRenderer = new MapRenderer( this );
			MapBordersRenderer = new MapBordersRenderer( this );
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
			FieldOfView = Options.GetInt( OptionsKey.FieldOfView, 1, 150, 70 );
			ZoomFieldOfView = FieldOfView;
			UpdateProjection();
			CommandManager = new CommandManager();
			CommandManager.Init( this );
			SelectionManager = new SelectionManager( this );
			WeatherRenderer = new WeatherRenderer( this );
			WeatherRenderer.Init();
			BlockHandRenderer = new BlockHandRenderer( this );
			BlockHandRenderer.Init();
			
			FpsLimitMethod method = Options.GetEnum( OptionsKey.FpsLimit, FpsLimitMethod.LimitVSync );
			SetFpsLimitMethod( method );
			Graphics.DepthTest = true;
			Graphics.DepthTestFunc( CompareFunc.LessEqual );
			//Graphics.DepthWrite = true;
			Graphics.AlphaBlendFunc( BlendFunc.SourceAlpha, BlendFunc.InvSourceAlpha );
			Graphics.AlphaTestFunc( CompareFunc.Greater, 0.5f );
			fpsScreen = new FpsScreen( this );
			fpsScreen.Init();
			hudScreen = new HudScreen( this );
			hudScreen.Init();
			Culling = new FrustumCulling();
			EnvRenderer.Init();
			MapBordersRenderer.Init();
			Picking = new PickingRenderer( this );
			AudioPlayer = new AudioPlayer( this );
			LiquidsBreakable = Options.GetBool( OptionsKey.LiquidsBreakable, false );
			AxisLinesRenderer = new AxisLinesRenderer( this );
			
			LoadIcon();
			string connectString = "Connecting to " + IPAddress + ":" + Port +  "..";
			Graphics.WarnIfNecessary( Chat );
			SetNewScreen( new LoadingMapScreen( this, connectString, "Waiting for handshake" ) );
			Network.Connect( IPAddress, Port );
		}
		
		void LoadIcon() {
			string launcherPath = Path.Combine( Program.AppDirectory, "Launcher2.exe" );
			if( File.Exists( launcherPath ) ) {
				Icon = Icon.ExtractAssociatedIcon( launcherPath );
				return;
			}
			launcherPath = Path.Combine( Program.AppDirectory, "Launcher.exe" );
			if( File.Exists( launcherPath ) ) {
				Icon = Icon.ExtractAssociatedIcon( launcherPath );
			}
		}
		
		public void SetViewDistance( int distance, bool save ) {
			ViewDistance = distance;
			if( ViewDistance > MaxViewDistance )
				ViewDistance = MaxViewDistance;
			Utils.LogDebug( "setting view distance to: {0} ({1})", distance, ViewDistance );
			
			if( save ) {
				UserViewDistance = distance;
				Options.Set( OptionsKey.ViewDist, distance );
			}
			Events.RaiseViewDistanceChanged();
			UpdateProjection();
		}
		
		/// <summary> Gets whether the active screen handles all input. </summary>
		public bool ScreenLockedInput {
			get { return activeScreen == null ? hudScreen.HandlesAllInput :
					activeScreen.HandlesAllInput; } // inlined here.
		}
		
		public Screen GetActiveScreen {
			get { return activeScreen == null ? hudScreen : activeScreen; }
		}
		
		protected override void OnRenderFrame( FrameEventArgs e ) {
			PerformFpsElapsed( e.Time * 1000 );
			Graphics.BeginFrame( this );
			Graphics.BindIb( defaultIb );
			accumulator += e.Time;
			Vertices = 0;
			if( !Focused && !ScreenLockedInput )
				SetNewScreen( new PauseScreen( this ) );
			
			base.OnRenderFrame( e );
			CheckScheduledTasks( e.Time );
			float t = (float)( ticksAccumulator / ticksPeriod );
			LocalPlayer.SetInterpPosition( t );
			
			Graphics.Clear();
			Graphics.SetMatrixMode( MatrixType.Modelview );
			Matrix4 modelView = Camera.GetView( e.Time );
			View = modelView;
			Graphics.LoadMatrix( ref modelView );
			Culling.CalcFrustumEquations( ref Projection, ref modelView );
			
			bool visible = activeScreen == null || !activeScreen.BlocksWorld;
			if( Map.IsNotLoaded ) visible = false;
			if( visible ) {
				AxisLinesRenderer.Render( e.Time );
				Players.RenderModels( Graphics, e.Time, t );
				Players.RenderNames( Graphics, e.Time, t );
				CurrentCameraPos = Camera.GetCameraPos( LocalPlayer.EyePosition );
				
				ParticleManager.Render( e.Time, t );
				Camera.GetPickedBlock( SelectedPos ); // TODO: only pick when necessary
				EnvRenderer.Render( e.Time );
				if( SelectedPos.Valid && !HideGui )
					Picking.Render( e.Time, SelectedPos );
				MapRenderer.Render( e.Time );
				SelectionManager.Render( e.Time );
				WeatherRenderer.Render( e.Time );
				Players.RenderHoveredNames( Graphics, e.Time, t );
				
				bool left = IsMousePressed( MouseButton.Left );
				bool middle = IsMousePressed( MouseButton.Middle );
				bool right = IsMousePressed( MouseButton.Right );
				InputHandler.PickBlocks( true, left, middle, right );
				if( !HideGui )
					BlockHandRenderer.Render( e.Time, t );
			} else {
				SelectedPos.SetAsInvalid();
			}
			
			Graphics.Mode2D( Width, Height, EnvRenderer is StandardEnvRenderer );
			fpsScreen.Render( e.Time );
			if( activeScreen == null || !activeScreen.HidesHud )
				hudScreen.Render( e.Time );
			if( activeScreen != null )
				activeScreen.Render( e.Time );
			Graphics.Mode3D( EnvRenderer is StandardEnvRenderer );
			
			if( screenshotRequested )
				TakeScreenshot();
			Graphics.EndFrame( this );
		}
		
		const int ticksFrequency = 20;
		const double ticksPeriod = 1.0 / ticksFrequency;
		const double imageCheckPeriod = 30.0;
		const double cameraPeriod = 1.0 / 60;
		double ticksAccumulator, imageCheckAccumulator,
		cameraAccumulator;
		
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
				AudioPlayer.Tick( ticksPeriod );
				BlockHandRenderer.Tick( ticksPeriod );
				ticksThisFrame++;
				ticksAccumulator -= ticksPeriod;
			}
			
			while( cameraAccumulator >= cameraPeriod ) {
				Camera.Tick( cameraPeriod );
				cameraAccumulator -= cameraPeriod;
			}
			
			if( ticksThisFrame > ticksFrequency / 3 )
				Utils.LogDebug( "Falling behind (did {0} ticks this frame)", ticksThisFrame );
		}
		
		void TakeScreenshot() {
			string path = Path.Combine( Program.AppDirectory, "screenshots" );
			if( !Directory.Exists( path ) )
				Directory.CreateDirectory( path );
			
			string timestamp = DateTime.Now.ToString( "dd-MM-yyyy-HH-mm-ss" );
			string file = "screenshot_" + timestamp + ".png";
			path = Path.Combine( "screenshots", file );
			Graphics.TakeScreenshot( path, ClientSize.Width, ClientSize.Height );
			Chat.Add( "&eTaken screenshot as: " + file );
			screenshotRequested = false;
		}
		
		public void UpdateProjection() {
			Matrix4 projection = Camera.GetProjection( out HeldBlockProjection );
			Projection = projection;
			Graphics.SetMatrixMode( MatrixType.Projection );
			Graphics.LoadMatrix( ref projection );
			Graphics.SetMatrixMode( MatrixType.Modelview );
		}
		
		protected override void OnResize( object sender, EventArgs e ) {
			base.OnResize( sender, e );
			Graphics.OnWindowResize( this );
			UpdateProjection();
			if( activeScreen != null )
				activeScreen.OnResize( width, height, Width, Height );
			hudScreen.OnResize( width, height, Width, Height );
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
		
		public void SetNewScreen( Screen screen ) { SetNewScreen( screen, true ); }
		
		public void SetNewScreen( Screen screen, bool disposeOld ) {
			// don't switch to the new screen immediately if the user
			// is currently looking at a warning dialog.
			if( activeScreen is WarningScreen ) {
				WarningScreen warning = (WarningScreen)activeScreen;
				if( warning.lastScreen != null )
					warning.lastScreen.Dispose();
				
				warning.lastScreen = screen;
				if( warning.lastScreen != null )
					screen.Init();
				return;
			}
			InputHandler.ScreenChanged( activeScreen, screen );
			if( activeScreen != null && disposeOld )
				activeScreen.Dispose();
			
			if( screen == null ) {
				hudScreen.GainFocus();
			} else if( activeScreen == null ) {
				hudScreen.LoseFocus();
			}
			
			if( screen != null )
				screen.Init();
			activeScreen = screen;
		}
		
		public void RefreshHud() { hudScreen.Recreate(); }
		
		public void ShowWarning( WarningScreen screen ) {
			if( !(activeScreen is WarningScreen) ) {
				screen.lastScreen = activeScreen;
				activeScreen = screen;
				
				screen.wasCursorVisible = CursorVisible;
				if( !CursorVisible ) CursorVisible = true;
			} else {
				screen.wasCursorVisible = WarningScreens[0].wasCursorVisible;
			}
			WarningScreens.Add( screen );
			screen.Init();
		}
		
		public void CycleCamera() {
			PerspectiveCamera oldCam = (PerspectiveCamera)Camera;
			if( Camera == firstPersonCam ) Camera = thirdPersonCam;
			else if( Camera == thirdPersonCam ) Camera = forwardThirdPersonCam;
			else Camera = firstPersonCam;

			if( !LocalPlayer.CanUseThirdPersonCamera || !LocalPlayer.HacksEnabled )
				Camera = firstPersonCam;
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
		
		float limitMilliseconds;
		double limitAcc;
		public void SetFpsLimitMethod( FpsLimitMethod method ) {
			FpsLimit = method;
			limitAcc = 0;
			limitMilliseconds = 0;
			Graphics.SetVSync( this,
			                  method == FpsLimitMethod.LimitVSync );
			
			if( method == FpsLimitMethod.Limit120FPS )
				limitMilliseconds = 1000f / 60;
			if( method == FpsLimitMethod.Limit60FPS )
				limitMilliseconds = 1000f / 30;
			if( method == FpsLimitMethod.Limit30FPS )
				limitMilliseconds = 1000f / 15;
		}
		
		void PerformFpsElapsed( double elapsedMs ) {
			limitAcc += elapsedMs;
			if( limitAcc >= limitMilliseconds ) { // going slower than limit?
				limitAcc -= limitMilliseconds;
			} else { // going faster than limit
				double sleepTime = limitMilliseconds - limitAcc;
				sleepTime = Math.Ceiling( sleepTime );
				Thread.Sleep( (int)sleepTime );
				limitAcc = 0;
			}
		}
		
		public bool IsKeyDown( Key key ) { return InputHandler.IsKeyDown( key ); }
		
		public bool IsKeyDown( KeyBinding binding ) { return InputHandler.IsKeyDown( binding ); }
		
		public bool IsMousePressed( MouseButton button ) { return InputHandler.IsMousePressed( button ); }
		
		public Key Mapping( KeyBinding mapping ) { return InputHandler.Keys[mapping]; }
		
		public override void Dispose() {
			MapRenderer.Dispose();
			MapBordersRenderer.Dispose();
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
			AudioPlayer.Dispose();
			AxisLinesRenderer.Dispose();
			
			Chat.Dispose();
			if( activeScreen != null )
				activeScreen.Dispose();
			Graphics.DeleteIb( defaultIb );
			Graphics.Dispose();
			Drawer2D.DisposeInstance();
			Animations.Dispose();
			Graphics.DeleteTexture( ref CloudsTexId );
			Graphics.DeleteTexture( ref RainTexId );
			Graphics.DeleteTexture( ref SnowTexId );
			Graphics.DeleteTexture( ref GuiTexId );
			Graphics.DeleteTexture( ref GuiClassicTexId );
			
			if( Options.HasChanged )
				Options.Save();
			base.Dispose();
		}
		
		internal bool CanPick( byte block ) {
			if( BlockInfo.IsAir[block] )
				return false;
			if( !BlockInfo.IsLiquid[block] ) return true;
			
			return !LiquidsBreakable ? false :
				Inventory.CanPlace[block] && Inventory.CanDelete[block];
		}
		
		public Game( string username, string mppass, string skinServer,
		            bool nullContext, int width, int height )
			: base( width, height, GraphicsMode.Default, Program.AppName + " (" + username + ")", nullContext, 0, DisplayDevice.Default ) {
			Username = username;
			Mppass = mppass;
			this.skinServer = skinServer;
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