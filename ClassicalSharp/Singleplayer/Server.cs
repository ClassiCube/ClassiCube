using System;
using System.Net;
using OpenTK;
using OpenTK.Input;

namespace ClassicalSharp.Singleplayer {

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
			MakeMap( 128, 128, 128 );
			game.CommandManager.RegisterCommand( new GenerateCommand() );
			game.CommandManager.RegisterCommand( new SaveMapCommand() );
			game.CommandManager.RegisterCommand( new LoadMapCommand() );
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
		
		internal void NewMap() {
			ServerName = "Single player";
			ServerMotd = "Generating a map..";
			game.LocalPlayer.UserType = 0x64;
			
			game.Map.Reset();
			game.SetNewScreen( new LoadingMapScreen( game, ServerName, ServerMotd ) );
		}
		
		internal unsafe void MakeMap( int width, int height, int length ) {
			byte[] map = new byte[width * height * length];
			var sw = System.Diagnostics.Stopwatch.StartNew();
			fixed( byte* ptr = map ) {
				MapSet( width, length, ptr, 0, height / 2 - 2, (byte)Block.Dirt );
				MapSet( width, length, ptr, height / 2 - 1, height / 2 - 1, (byte)Block.Grass );
			}
			game.Map.UseRawMap( map, width, height, length );
			game.RaiseOnNewMapLoaded();
			game.SetNewScreen( new NormalScreen( game ) );
			ResetPlayerPosition();
			game.AddChat( "&ePlaying single player", CpeMessage.Status1 );
			GC.Collect();
		}
		
		unsafe void MapSet( int width, int length, byte* ptr, int yStart, int yEnd, byte block ) {
			int startIndex = yStart * length * width;
			int endIndex = ( yEnd * length + (length - 1) ) * width + (width - 1);
			MemUtils.memset( (IntPtr)ptr, block, startIndex, endIndex - startIndex + 1 );
		}
		
		void ResetPlayerPosition() {
			float x = game.Map.Width / 2, y = game.Map.Height / 2, z = game.Map.Length / 2;
			LocationUpdate update = LocationUpdate.MakePosAndOri( x, y, z, 0, 0, false );
			game.LocalPlayer.SetLocation( update, false );
		}
	}
}