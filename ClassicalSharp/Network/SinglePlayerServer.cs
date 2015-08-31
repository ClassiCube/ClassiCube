using System;
using System.Net;
using OpenTK;
using OpenTK.Input;

namespace ClassicalSharp {

	public sealed class SinglePlayerServer : INetworkProcessor {
		
		Game game;
		public SinglePlayerServer( Game window ) {
			game = window;
		}
		
		public override bool IsSinglePlayer {
			get { return true; }
		}
		
		public override void Connect( IPAddress address, int port ) {
			for( int i = (int)Block.Stone; i <= (int)Block.StoneBrick; i++ ) {
				game.CanPlace[i] = true;
				game.CanDelete[i] = true;
			}
			game.RaiseBlockPermissionsChanged();
			NewMap();
			MapLoaded();
		}
		
		public override void SendChat( string text ) {
			if( !String.IsNullOrEmpty( text ) ) {
				game.AddChat( text, CpeMessage.Normal );
			}
		}
		
		public override void SendPosition( Vector3 pos, float yaw, float pitch ) {
		}
		
		public override void SendSetBlock( int x, int y, int z, bool place, byte block ) {
		}
		
		public override void SendPlayerClick( MouseButton button, bool buttonDown, byte targetId, PickedPos pos ) {
		}
		
		public override void Dispose() {
		}
		
		public override void Tick( double delta ) {
			if( Disconnected ) return;
		}
		
		void NewMap() {
			ServerName = "Single player";
			ServerMotd = "Generating a map..";
			game.LocalPlayer.UserType = 0x64;
			
			game.Map.Reset();
			game.RaiseOnNewMap();
			game.SetNewScreen( new LoadingMapScreen( game, ServerName, ServerMotd ) );
		}
		
		void MapLoaded() {
			game.SetNewScreen( new NormalScreen( game ) );
			int width = 16, height = 16, length = 16;
			byte[] map = new byte[width * height * length];
			MapSet( width, length, map, 0, height / 2 - 2, (byte)Block.Dirt );
			MapSet( width, length, map, height / 2 - 1, height / 2 - 1, (byte)Block.Grass );
			game.Map.UseRawMap( map, width, height, length );
			game.RaiseOnNewMapLoaded();
			ResetPlayerPosition();
			game.AddChat( "&ePlaying single player", CpeMessage.Status1 );
			GC.Collect();
		}
		
		void MapSet( int width, int length, byte[] map, int yStart, int yEnd, byte block ) {
			int startIndex = yStart * length * width;
			int endIndex = ( yEnd * length + (length - 1) ) * width + (width - 1);
			for( int i = startIndex; i <= endIndex; i++ ) {
				map[i] = block;
			}
		}
		
		void ResetPlayerPosition() {
			float x = game.Map.Width / 2, y = game.Map.Height / 2, z = game.Map.Length / 2;
			LocationUpdate update = LocationUpdate.MakePosAndOri( x, y, z, 0, 0, false );
			game.LocalPlayer.SetLocation( update, false );
		}
	}
}