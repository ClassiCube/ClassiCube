//#define TEST_VANILLA
using System;
using System.Net;
using ClassicalSharp.Generator;
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
			ServerSupportsFullCP437 = true;
			ServerSupportsPartialMessages = true;
		}
		
		public override bool IsSinglePlayer { get { return true; } }
		
		public override void Connect( IPAddress address, int port ) {
			int max = game.UseCPE ? BlockInfo.MaxDefinedCpeBlock : BlockInfo.MaxDefinedOriginalBlock;
			for( int i = 1; i <= max; i++ ) {
				game.Inventory.CanPlace[i] = true;
				game.Inventory.CanDelete[i] = true;
			}
			
			game.Events.RaiseBlockPermissionsChanged();
			int seed = new Random().Next();
			GenMap( 128, 64, 128, seed, new NotchyGenerator() );
		}
		
		public override void SendChat( string text, bool partial ) {
			if( String.IsNullOrEmpty( text ) ) return;
			text = text.TrimEnd().Replace( '%', '&' );
			game.Chat.Add( text, MessageType.Normal );
		}
		
		public override void SendPosition( Vector3 pos, float yaw, float pitch ) {
		}
		
		public override void SendSetBlock( int x, int y, int z, bool place, byte block ) {
			if( place )
				physics.OnBlockPlaced( x, y, z, block );
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
				LoadingMapScreen screen = ((LoadingMapScreen)game.GetActiveScreen);
				
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
				game.Map.SetData( generatedMap, generator.Width,
				                 generator.Height, generator.Length );
				generatedMap = null;
				
				game.MapEvents.RaiseOnNewMapLoaded();
				ResetPlayerPosition();
			}
			
			generator = null;
			game.Chat.Add( "&ePlaying single player", MessageType.Status1 );
			GC.Collect();
		}
		
		internal void GenMap( int width, int height, int length, int seed, IMapGenerator generator ) {
			game.Map.Reset();
			GC.Collect();
			this.generator = generator;
			game.SetNewScreen( new LoadingMapScreen( game, "Generating level", "Generating.." ) );
			generator.GenerateAsync( game, width, height, length, seed );
		}
		
		unsafe void MapSet( int width, int length, byte* ptr, int yStart, int yEnd, byte block ) {
			int startIndex = yStart * length * width;
			int endIndex = ( yEnd * length + (length - 1) ) * width + (width - 1);
			MemUtils.memset( (IntPtr)ptr, block, startIndex, endIndex - startIndex + 1 );
		}
		
		void ResetPlayerPosition() {
			int x = game.Map.Width / 2, z = game.Map.Length / 2;
			int y = game.Map.GetLightHeight( x, z ) + 2;
			
			LocationUpdate update = LocationUpdate.MakePosAndOri( x, y, z, 0, 0, false );
			game.LocalPlayer.SetLocation( update, false );
			game.LocalPlayer.SpawnPoint = new Vector3( x, y, z );
		}
	}
}