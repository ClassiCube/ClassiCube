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
		
		public bool ChangeTerrainAtlas( Bitmap atlas ) {
			bool pow2 = Utils.IsPowerOf2( atlas.Width ) && Utils.IsPowerOf2( atlas.Height );
			if( !pow2 || atlas.Width != atlas.Height ) {
				Chat.Add( "&cCurrent texture pack has an invalid terrain.png" );
				Chat.Add( "&cWidth and length must be the same, and also powers of two." );
				return false;
			}
			LoadAtlas( atlas );
			Events.RaiseTerrainAtlasChanged();
			return true;
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
			Players = new EntityList( this );
			AcceptedUrls.Load(); 
			DeniedUrls.Load();
			ETags.Load();
			InputHandler = new InputHandler( this );
			defaultIb = Graphics.MakeDefaultIb();
			ParticleManager = new ParticleManager( this );
			LoadOptions();
			LoadGuiOptions();
			Chat = AddComponent( new Chat() );
			WorldEvents.OnNewMap += OnNewMapCore;
			WorldEvents.OnNewMapLoaded += OnNewMapLoadedCore;
			
			BlockInfo = new BlockInfo();
			BlockInfo.Init();
			ModelCache = new ModelCache( this );
			ModelCache.InitCache();
			AsyncDownloader = AddComponent( new AsyncDownloader() );
			Drawer2D = new GdiPlusDrawer2D( Graphics );
			Drawer2D.UseBitmappedChat = ClassicMode || !Options.GetBool( OptionsKey.ArialChatFont, false );
			
			TerrainAtlas1D = new TerrainAtlas1D( Graphics );
			TerrainAtlas = new TerrainAtlas2D( Graphics, Drawer2D );
			Animations = AddComponent( new Animations() );
			Inventory = AddComponent( new Inventory() );
			
			BlockInfo.SetDefaultBlockPermissions( Inventory.CanPlace, Inventory.CanDelete );
			World = new World( this );
			LocalPlayer = new LocalPlayer( this );
			Players[255] = LocalPlayer;
			width = Width;
			height = Height;
			MapRenderer = new MapRenderer( this );
			MapBordersRenderer = AddComponent( new MapBordersRenderer() );
			EnvRenderer = AddComponent( new StandardEnvRenderer() );
			if( IPAddress == null ) {
				Network = new Singleplayer.SinglePlayerServer( this );
			} else {
				Network = new Network.NetworkProcessor( this );
			}
			Graphics.LostContextFunction = Network.Tick;
			
			firstPersonCam = new FirstPersonCamera( this );
			thirdPersonCam = new ThirdPersonCamera( this );
			forwardThirdPersonCam = new ForwardThirdPersonCamera( this );
			Camera = firstPersonCam;
			UpdateProjection();
			CommandManager = AddComponent( new CommandManager() );
			SelectionManager = AddComponent( new SelectionManager() );
			WeatherRenderer = AddComponent( new WeatherRenderer() );
			BlockHandRenderer = AddComponent( new BlockHandRenderer() );
			
			Graphics.DepthTest = true;
			Graphics.DepthTestFunc( CompareFunc.LessEqual );
			//Graphics.DepthWrite = true;
			Graphics.AlphaBlendFunc( BlendFunc.SourceAlpha, BlendFunc.InvSourceAlpha );
			Graphics.AlphaTestFunc( CompareFunc.Greater, 0.5f );
			fpsScreen = AddComponent( new FpsScreen( this ) );
			hudScreen = AddComponent( new HudScreen( this ) );
			Culling = new FrustumCulling();
			Picking = AddComponent( new PickedPosRenderer() );
			AudioPlayer = AddComponent( new AudioPlayer() );
			AxisLinesRenderer = AddComponent( new AxisLinesRenderer() );
			
			foreach( IGameComponent comp in Components )
				comp.Init( this );
			ExtractInitialTexturePack();
			foreach( IGameComponent comp in Components )
				comp.Ready( this );
			
			LoadIcon();
			string connectString = "Connecting to " + IPAddress + ":" + Port +  "..";
			if( Graphics.WarnIfNecessary( Chat ) ) {
				MapBordersRenderer.UseLegacyMode( true );
				((StandardEnvRenderer)EnvRenderer).UseLegacyMode( true );
			}
			SetNewScreen( new LoadingMapScreen( this, connectString, "Waiting for handshake" ) );
			Network.Connect( IPAddress, Port );
		}
		
		void ExtractInitialTexturePack() {
			defTexturePack = Options.Get( OptionsKey.DefaultTexturePack ) ?? "default.zip";
			TexturePackExtractor extractor = new TexturePackExtractor();
			extractor.Extract( "default.zip", this );
			// in case the user's default texture pack doesn't have all required textures
			if( defTexturePack != "default.zip" )
				extractor.Extract( DefaultTexturePack, this );
		}
		
		void LoadOptions() {
			ClassicMode = Options.GetBool( "mode-classic", false );
			ClassicHacks = Options.GetBool( OptionsKey.AllowClassicHacks, false );
			UseClassicGui = Options.GetBool( OptionsKey.UseClassicGui, true );
			UseClassicTabList = Options.GetBool( OptionsKey.UseClassicTabList, false );
			UseClassicOptions = Options.GetBool( OptionsKey.UseClassicOptions, false );
			AllowCustomBlocks = Options.GetBool( OptionsKey.AllowCustomBlocks, true );
			UseCPE = Options.GetBool( OptionsKey.UseCPE, true );
			SimpleArmsAnim = Options.GetBool( OptionsKey.SimpleArmsAnim, false );
			
			ViewBobbing = Options.GetBool( OptionsKey.ViewBobbing, false );		
			FpsLimitMethod method = Options.GetEnum( OptionsKey.FpsLimit, FpsLimitMethod.LimitVSync );
			SetFpsLimitMethod( method );
			ViewDistance = Options.GetInt( OptionsKey.ViewDist, 16, 4096, 512 );
			UserViewDistance = ViewDistance;
			
			DefaultFov = Options.GetInt( OptionsKey.FieldOfView, 1, 150, 70 );
			Fov = DefaultFov;
			ZoomFov = DefaultFov;
			ModifiableLiquids = !ClassicMode && Options.GetBool( OptionsKey.ModifiableLiquids, false );
			CameraClipping = Options.GetBool( OptionsKey.CameraClipping, true );
			
			AllowServerTextures = Options.GetBool( OptionsKey.AllowServerTextures, true );
			MouseSensitivity = Options.GetInt( OptionsKey.Sensitivity, 1, 100, 30 );
			ShowBlockInHand = Options.GetBool( OptionsKey.ShowBlockInHand, true );
			InvertMouse = Options.GetBool( OptionsKey.InvertMouse, false );
		}
		
		void LoadGuiOptions() {
			ChatLines = Options.GetInt( OptionsKey.ChatLines, 1, 30, 12 );
			ClickableChat = Options.GetBool( OptionsKey.ClickableChat, false );
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
		
		void OnNewMapCore( object sender, EventArgs e ) {
			foreach( IGameComponent comp in Components )
				comp.OnNewMap( this );
		}
		
		void OnNewMapLoadedCore( object sender, EventArgs e ) {
			foreach( IGameComponent comp in Components )
				comp.OnNewMapLoaded( this );
		}
		
		public T AddComponent<T>( T obj ) where T : IGameComponent {
			Components.Add( obj );
			return obj;
		}
		
		public bool ReplaceComponent<T>( ref T old, T obj ) where T : IGameComponent {
			for( int i = 0; i < Components.Count; i++ ) {
				if( !object.ReferenceEquals( Components[i], old ) ) continue;
				old.Dispose(); 
				
				Components[i] = obj;
				old = obj;
				obj.Init( this );				
				return true;
			}
			
			Components.Add( obj );
			obj.Init( this );
			return false;
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
		
		/// <summary> Gets the screen that the user is currently interacting with. </summary>
		public Screen ActiveScreen {
			get { return WarningOverlays.Count > 0 ? WarningOverlays[0]
					: activeScreen == null ? hudScreen : activeScreen; }
		}
		
		Stopwatch frameTimer = new Stopwatch();
		internal void RenderFrame( double delta ) {
			frameTimer.Reset();
			frameTimer.Start();
			
			Graphics.BeginFrame( this );
			Graphics.BindIb( defaultIb );
			accumulator += delta;
			Vertices = 0;
			if( !Focused && !ActiveScreen.HandlesAllInput )
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
			
			if( WarningOverlays.Count > 0)
				WarningOverlays[0].Render( delta );
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
			World.blocks = null;
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
			bool cursorVis = CursorVisible;
			if( WarningOverlays.Count == 0 ) CursorVisible = true;
			WarningOverlays.Add( screen );
			if( WarningOverlays.Count == 1 ) CursorVisible = cursorVis;
			// Save cursor visibility state
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
			SetNewScreen( null );
			fpsScreen.Dispose();
			TerrainAtlas.Dispose();
			TerrainAtlas1D.Dispose();
			ModelCache.Dispose();
			ParticleManager.Dispose();
			Players.Dispose();
			WorldEvents.OnNewMap -= OnNewMapCore;
			WorldEvents.OnNewMapLoaded -= OnNewMapLoadedCore;
			
			foreach( IGameComponent comp in Components )
				comp.Dispose();
			
			if( activeScreen != null )
				activeScreen.Dispose();
			Graphics.DeleteIb( defaultIb );
			Graphics.Dispose();
			Drawer2D.DisposeInstance();
			Graphics.DeleteTexture( ref CloudsTex );
			Graphics.DeleteTexture( ref GuiTex );
			Graphics.DeleteTexture( ref GuiClassicTex );
			Graphics.DeleteTexture( ref IconsTex );
			foreach( WarningScreen screen in WarningOverlays )
				screen.Dispose();
			
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
		
		
		/// <summary> Reads a bitmap from the stream (converting it to 32 bits per pixel if necessary),
		/// and updates the native texture for it. </summary>
		public void UpdateTexture( ref int texId, byte[] data, bool setSkinType ) {
			MemoryStream stream = new MemoryStream( data );
			Graphics.DeleteTexture( ref texId );
			
			using( Bitmap bmp = Platform.ReadBmp( stream ) ) {
				if( setSkinType )
					DefaultPlayerSkinType = Utils.GetSkinType( bmp );
				
				if( !FastBitmap.CheckFormat( bmp.PixelFormat ) ) {
					using( Bitmap bmp32 = Drawer2D.ConvertTo32Bpp( bmp ) )
						texId = Graphics.CreateTexture( bmp32 );
				} else {
					texId = Graphics.CreateTexture( bmp );
				}
			}
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