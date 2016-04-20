// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Net;
using System.Threading;
using ClassicalSharp.Audio;
using ClassicalSharp.Commands;
using ClassicalSharp.Entities;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Gui;
using ClassicalSharp.Map;
using ClassicalSharp.Model;
using ClassicalSharp.Network;
using ClassicalSharp.Particles;
using ClassicalSharp.Renderers;
using ClassicalSharp.Selections;
using ClassicalSharp.TexturePack;
using OpenTK;
using OpenTK.Input;
#if ANDROID
using Android.Graphics;
#endif

namespace ClassicalSharp {

	public partial class Game : IDisposable {
		
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
		
		public void Run() { window.Run(); }
		
		public void Exit() { window.Exit(); }
		
		
		internal void OnLoad() {
			Mouse = window.Mouse;
			Keyboard = window.Keyboard;
			#if !USE_DX
			Graphics = new OpenGLApi();
			#else
			Graphics = new Direct3D9Api( this );
			#endif
			Graphics.MakeGraphicsInfo();
			
			Options.Load();
			ClassicMode = Options.GetBool( "mode-classic", false );
			ClassicHacks = Options.GetBool( OptionsKey.AllowClassicHacks, false );
			Players = new EntityList( this );
			AcceptedUrls.Load(); DeniedUrls.Load();
			ViewDistance = Options.GetInt( OptionsKey.ViewDist, 16, 4096, 512 );
			UserViewDistance = ViewDistance;
			CameraClipping = Options.GetBool( OptionsKey.CameraClipping, true );
			InputHandler = new InputHandler( this );
			defaultIb = Graphics.MakeDefaultIb();
			ParticleManager = new ParticleManager( this );
			MouseSensitivity = Options.GetInt( OptionsKey.Sensitivity, 1, 100, 30 );
			LoadGui();
			
			UseClassicGui = Options.GetBool( OptionsKey.UseClassicGui, true );
			UseClassicTabList = Options.GetBool( OptionsKey.UseClassicTabList, false );
			UseClassicOptions = Options.GetBool( OptionsKey.UseClassicOptions, false );
			AllowCustomBlocks = Options.GetBool( OptionsKey.AllowCustomBlocks, true );
			UseCPE = Options.GetBool( OptionsKey.UseCPE, true );
			AllowServerTextures = Options.GetBool( OptionsKey.AllowServerTextures, true );
			
			BlockInfo = new BlockInfo();
			BlockInfo.Init();
			ChatLines = Options.GetInt( OptionsKey.ChatLines, 1, 30, 12 );
			ClickableChat = Options.GetBool( OptionsKey.ClickableChat, false );
			ModelCache = new ModelCache( this );
			ModelCache.InitCache();
			AsyncDownloader = new AsyncDownloader( skinServer );
			Drawer2D = new GdiPlusDrawer2D( Graphics );
			Drawer2D.UseBitmappedChat = ClassicMode || !Options.GetBool( OptionsKey.ArialChatFont, false );
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
			World = new World( this );
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
				Network = new Net.NetworkProcessor( this );
			}
			Graphics.LostContextFunction = Network.Tick;
			
			firstPersonCam = new FirstPersonCamera( this );
			thirdPersonCam = new ThirdPersonCamera( this );
			forwardThirdPersonCam = new ForwardThirdPersonCamera( this );
			Camera = firstPersonCam;
			DefaultFov = Options.GetInt( OptionsKey.FieldOfView, 1, 150, 70 );
			Fov = DefaultFov;
			ZoomFov = DefaultFov;
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
			Picking = new PickedPosRenderer( this );
			AudioPlayer = new AudioPlayer( this );
			ModifiableLiquids = !ClassicMode && Options.GetBool( OptionsKey.ModifiableLiquids, false );
			AxisLinesRenderer = new AxisLinesRenderer( this );
			
			LoadIcon();
			string connectString = "Connecting to " + IPAddress + ":" + Port +  "..";
			if( Graphics.WarnIfNecessary( Chat ) ) {
				MapBordersRenderer.SetUseLegacyMode( true );
				((StandardEnvRenderer)EnvRenderer).SetUseLegacyMode( true );
			}
			SetNewScreen( new LoadingMapScreen( this, connectString, "Waiting for handshake" ) );
			Network.Connect( IPAddress, Port );
		}
		
		void LoadGui() {
			Chat = new Chat( this );
			InventoryScale = Options.GetFloat( OptionsKey.InventoryScale, 0.25f, 5f, 1f );
			HotbarScale = Options.GetFloat( OptionsKey.HotbarScale, 0.25f, 5f, 1f );
			ChatScale = Options.GetFloat( OptionsKey.ChatScale, 0.35f, 5f, 1f );
			ShowFPS = Options.GetBool( OptionsKey.ShowFPS, true );
			
			TabAutocomplete = Options.GetBool( OptionsKey.TabAutocomplete, false );
			FontName = Options.Get( OptionsKey.FontName ) ?? "Arial";
			if( ClassicMode ) FontName = "Arial";
			
			try {
				using( Font f = new Font( FontName, 16 ) ) { }
			} catch( Exception ) {
				FontName = "Arial";
				Options.Set( OptionsKey.FontName, "Arial" );
			}
		}
		
		void LoadIcon() {
			string launcherPath = Path.Combine( Program.AppDirectory, "Launcher2.exe" );
			if( File.Exists( launcherPath ) ) {
				window.Icon = Icon.ExtractAssociatedIcon( launcherPath ); return;
			}
			launcherPath = Path.Combine( Program.AppDirectory, "Launcher.exe" );
			if( File.Exists( launcherPath ) ) {
				window.Icon = Icon.ExtractAssociatedIcon( launcherPath );
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
		
		/// <summary> Gets whether the screen the user is currently interacting with 
		/// handles all input. </summary>
		public bool ScreenLockedInput { get { return ActiveScreen.HandlesAllInput; } }
		
		/// <summary> Gets the screen that the user is currently interacting with. </summary>
		public Screen ActiveScreen {
			get { return activeScreen == null ? hudScreen : activeScreen; }
		}
		
		Stopwatch frameTimer = new Stopwatch();
		internal void RenderFrame( double delta ) {
			frameTimer.Reset();
			frameTimer.Start();
			
			Graphics.BeginFrame( this );
			Graphics.BindIb( defaultIb );
			accumulator += delta;
			Vertices = 0;
			if( !Focused && !ScreenLockedInput )
				SetNewScreen( new PauseScreen( this ) );
			CheckZoomFov();
			
			CheckScheduledTasks( delta );
			float t = (float)( ticksAccumulator / ticksPeriod );
			LocalPlayer.SetInterpPosition( t );		
			Graphics.Clear();
			UpdateViewMatrix( delta );
			
			bool visible = activeScreen == null || !activeScreen.BlocksWorld;
			if( World.IsNotLoaded ) visible = false;
			if( visible )
				Render3D( delta, t );
			else
				SelectedPos.SetAsInvalid();
			
			RenderGui( delta );
			if( screenshotRequested )
				TakeScreenshot();
			Graphics.EndFrame( this );
			LimitFPS();
		}
		
		void CheckZoomFov() {
			bool allowZoom = activeScreen == null && !hudScreen.HandlesAllInput;
			if( allowZoom && IsKeyDown( KeyBinding.ZoomScrolling ) )
				InputHandler.SetFOV( ZoomFov, false );
		}
		
		void UpdateViewMatrix( double delta ) {
			Graphics.SetMatrixMode( MatrixType.Modelview );
			Matrix4 modelView = Camera.GetView( delta );
			View = modelView;
			Graphics.LoadMatrix( ref modelView );
			Culling.CalcFrustumEquations( ref Projection, ref modelView );
		}
		
		void Render3D( double delta, float t ) {
			AxisLinesRenderer.Render( delta );
			Players.RenderModels( Graphics, delta, t );
			Players.RenderNames( Graphics, delta, t );
			CurrentCameraPos = Camera.GetCameraPos( LocalPlayer.EyePosition );
			
			ParticleManager.Render( delta, t );
			Camera.GetPickedBlock( SelectedPos ); // TODO: only pick when necessary
			EnvRenderer.Render( delta );
			if( SelectedPos.Valid && !HideGui )
				Picking.Render( delta, SelectedPos );
			MapRenderer.Render( delta );
			SelectionManager.Render( delta );
			Players.RenderHoveredNames( Graphics, delta, t );
			
			bool left = IsMousePressed( MouseButton.Left );
			bool middle = IsMousePressed( MouseButton.Middle );
			bool right = IsMousePressed( MouseButton.Right );
			InputHandler.PickBlocks( true, left, middle, right );
			if( !HideGui )
				BlockHandRenderer.Render( delta, t );
		}
		
		void RenderGui( double delta ) {
			Graphics.Mode2D( Width, Height, EnvRenderer is StandardEnvRenderer );
			fpsScreen.Render( delta );
			if( activeScreen == null || !activeScreen.HidesHud && !activeScreen.RenderHudAfter )
				hudScreen.Render( delta );
			if( activeScreen != null )
				activeScreen.Render( delta );
			if( activeScreen != null && !activeScreen.HidesHud && activeScreen.RenderHudAfter )
				hudScreen.Render( delta );
			Graphics.Mode3D( EnvRenderer is StandardEnvRenderer );
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
			DefaultFov = Options.GetInt( OptionsKey.FieldOfView, 1, 150, 70 );
			Matrix4 projection = Camera.GetProjection( out HeldBlockProjection );
			Projection = projection;
			
			Graphics.SetMatrixMode( MatrixType.Projection );
			Graphics.LoadMatrix( ref projection );
			Graphics.SetMatrixMode( MatrixType.Modelview );
			Events.RaiseProjectionChanged();
		}
		
		internal void OnResize() {
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
			World.Reset();
			World.mapData = null;
			Drawer2D.InitColours();
			
			for( int tile = BlockInfo.CpeBlocksCount; tile < BlockInfo.BlocksCount; tile++ )
				BlockInfo.ResetBlockInfo( (byte)tile, false );
			BlockInfo.SetupCullingCache();
			BlockInfo.InitLightOffsets();
			GC.Collect();
		}
		
		internal Screen activeScreen;
		
		public void SetNewScreen( Screen screen ) { SetNewScreen( screen, true ); }
		
		public void SetNewScreen( Screen screen, bool disposeOld ) {
			// Don't switch to the new screen immediately if the user
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
				CursorVisible = true;
			} else {
				screen.wasCursorVisible = WarningScreens[0].wasCursorVisible;
			}
			WarningScreens.Add( screen );
			screen.Init();
		}
		
		public void CycleCamera() {
			if( ClassicMode ) return;
			PerspectiveCamera oldCam = (PerspectiveCamera)Camera;
			if( Camera == firstPersonCam ) Camera = thirdPersonCam;
			else if( Camera == thirdPersonCam ) Camera = forwardThirdPersonCam;
			else Camera = firstPersonCam;

			if( !LocalPlayer.Hacks.CanUseThirdPersonCamera || !LocalPlayer.Hacks.Enabled )
				Camera = firstPersonCam;
			PerspectiveCamera newCam = (PerspectiveCamera)Camera;
			newCam.delta = oldCam.delta;
			newCam.previous = oldCam.previous;
			UpdateProjection();
		}
		
		public void UpdateBlock( int x, int y, int z, byte block ) {
			int oldHeight = World.GetLightHeight( x, z ) + 1;
			World.SetBlock( x, y, z, block );
			int newHeight = World.GetLightHeight( x, z ) + 1;
			MapRenderer.RedrawBlock( x, y, z, block, oldHeight, newHeight );
		}
		
		float limitMilliseconds;
		public void SetFpsLimitMethod( FpsLimitMethod method ) {
			FpsLimit = method;
			limitMilliseconds = 0;
			Graphics.SetVSync( this, method == FpsLimitMethod.LimitVSync );
			
			if( method == FpsLimitMethod.Limit120FPS )
				limitMilliseconds = 1000f / 120;
			if( method == FpsLimitMethod.Limit60FPS )
				limitMilliseconds = 1000f / 60;
			if( method == FpsLimitMethod.Limit30FPS )
				limitMilliseconds = 1000f / 30;
		}
		
		void LimitFPS() {
			if( FpsLimit == FpsLimitMethod.LimitVSync ) return;
			
			double elapsed = frameTimer.Elapsed.TotalMilliseconds;
			double leftOver = limitMilliseconds - elapsed;
			if( leftOver > 0.001 ) // going faster than limit
				Thread.Sleep( (int)Math.Round( leftOver, MidpointRounding.AwayFromZero ) );
		}
		
		public bool IsKeyDown( Key key ) { return InputHandler.IsKeyDown( key ); }
		
		public bool IsKeyDown( KeyBinding binding ) { return InputHandler.IsKeyDown( binding ); }
		
		public bool IsMousePressed( MouseButton button ) { return InputHandler.IsMousePressed( button ); }
		
		public Key Mapping( KeyBinding mapping ) { return InputHandler.Keys[mapping]; }
		
		public void Dispose() {
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
			
			if( Options.HasChanged ) {
				Options.Load();
				Options.Save();
			}
		}
		
		internal bool CanPick( byte block ) {
			if( BlockInfo.IsAir[block] ) return false;
			if( BlockInfo.IsSprite[block] ) return true;
			if( BlockInfo.Collide[block] != CollideType.SwimThrough ) return true;
			
			return !ModifiableLiquids ? false :
				Inventory.CanPlace[block] && Inventory.CanDelete[block];
		}
		
		public Game( string username, string mppass, string skinServer,
		            bool nullContext, int width, int height ) {
			window = new DesktopWindow( this, username, nullContext, width, height );
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