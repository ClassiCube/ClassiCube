// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
//#define TEST_VANILLA
using System;
using System.Net;
using ClassicalSharp.Entities;
using ClassicalSharp.Generator;
using ClassicalSharp.Gui;
using OpenTK;
using OpenTK.Input;

namespace ClassicalSharp.Singleplayer {

	public sealed class SinglePlayerServer : INetworkProcessor {
		
		internal Physics physics;
		internal byte[] generatedMap;
		IMapGenerator generator;
		string lastState;
		
		public SinglePlayerServer( Game window ) {
			game = window;
			physics = new Physics( game );
			ServerSupportsFullCP437 = !game.ClassicMode;
			ServerSupportsPartialMessages = true;
		}
		
		public override bool IsSinglePlayer { get { return true; } }
		
		public override void Connect( IPAddress address, int port ) {
			game.Chat.SetLogName( "Singleplayer" );
			game.UseCPEBlocks = game.UseCPE;
			int max = game.UseCPEBlocks ? BlockInfo.MaxCpeBlock : BlockInfo.MaxOriginalBlock;
			for( int i = 1; i <= max; i++ ) {
				game.Inventory.CanPlace[i] = true;
				game.Inventory.CanDelete[i] = true;
			}
			game.AsyncDownloader.DownloadSkin( game.LocalPlayer.SkinIdentifier,
			                                  game.LocalPlayer.SkinName );
			
			game.Events.RaiseBlockPermissionsChanged();
			int seed = new Random().Next();
			GenMap( 128, 64, 128, seed, new NotchyGenerator() );
		}
		
		char lastCol = '\0';
		public override void SendChat( string text, bool partial ) {
			if( !String.IsNullOrEmpty( text ) )
				AddChat( text );			
			if( !partial ) lastCol = '\0'; 
		}
		
		void AddChat( string text ) {
			text = text.TrimEnd().Replace( '%', '&' );
			if( !IDrawer2D.IsWhiteColour( lastCol ) )
				text = "&" + lastCol + text;
			
			char col = game.Drawer2D.LastColour( text, text.Length );
			if( col != '\0' ) lastCol = col;
			game.Chat.Add( text, MessageType.Normal );
		}
		
		public override void SendPosition( Vector3 pos, float yaw, float pitch ) {
		}
		
		public override void SendPlayerClick( MouseButton button, bool buttonDown, byte targetId, PickedPos pos ) {
		}
		
		public override void Dispose() {
			physics.Dispose();
		}
		
		public override void Tick( double delta ) {
			if( Disconnected ) return;
			physics.Tick();
			CheckAsyncResources();
			
			if( generator == null )
				return;
			if( generator.Done ) {
				EndGeneration();
			} else {
				string state = generator.CurrentState;
				float progress = generator.CurrentProgress;
				LoadingMapScreen screen = ((LoadingMapScreen)game.ActiveScreen);
				
				screen.SetProgress( progress );
				if( state != lastState ) {
					lastState = state;
					screen.SetMessage( state );
				}
			}
		}
		
		void EndGeneration() {
			game.SetNewScreen( null );
			if( generatedMap == null ) {
				game.Chat.Add( "&cFailed to generate the map." );
			} else {
				IMapGenerator gen = generator;
				game.World.SetNewMap( generatedMap, gen.Width, gen.Height, gen.Length );
				generatedMap = null;
				ResetPlayerPosition();
				game.WorldEvents.RaiseOnNewMapLoaded();
			}
			
			generator = null;
			game.Chat.Add( "&ePlaying single player", MessageType.Status1 );
			GC.Collect();
		}
		
		internal void GenMap( int width, int height, int length, int seed, IMapGenerator generator ) {
			game.World.Reset();
			GC.Collect();
			this.generator = generator;
			game.SetNewScreen( new LoadingMapScreen( game, "Generating level", "Generating.." ) );
			generator.GenerateAsync( game, width, height, length, seed );
		}
		

		void ResetPlayerPosition() {
			int x = game.World.Width / 2, z = game.World.Length / 2;
			int y = game.World.GetLightHeight( x, z ) + 2;
			
			LocationUpdate update = LocationUpdate.MakePosAndOri( x, y, z, 0, 0, false );
			game.LocalPlayer.SetLocation( update, false );
			game.LocalPlayer.Spawn = new Vector3( x, y, z );
			game.CurrentCameraPos = game.Camera.GetCameraPos( game.LocalPlayer.EyePosition );
		}
	}
}