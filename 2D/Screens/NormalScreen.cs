using System;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public class NormalScreen : Screen {
		
		public NormalScreen( Game window ) : base( window ) {
		}
		
		ChatScreen chat;
		HotbarWidget hotbar;
		Widget playerList;
		
		public override void Render( double delta ) {
			chat.Render( delta );
			hotbar.Render( delta );
			if( playerList != null ) {
				playerList.Render( delta );
				// NOTE: Should usually be caught by KeyUp, but just in case.
				if( !Window.IsKeyDown( KeyMapping.PlayerList ) ) {
					playerList.Dispose();
					playerList = null;
				}
			}
		}
		
		public override void Dispose() {
			chat.Dispose();
			hotbar.Dispose();
			if( playerList != null ) {
				playerList.Dispose();
			}
		}
		
		public override void OnResize( int oldWidth, int oldHeight, int width, int height ) {
			chat.OnResize( oldWidth, oldHeight, width, height );
			hotbar.OnResize( oldWidth, oldHeight, width, height );
			if( playerList != null ) {
				playerList.OnResize( oldWidth, oldHeight, width, height );
			}
		}
		
		public override void Init() {
			chat = new ChatScreen( Window );
			chat.Window = Window;
			const int blockSize = 32;
			chat.ChatLogYOffset = blockSize + blockSize + blockSize / 2;
			chat.ChatInputYOffset = blockSize + blockSize;
			chat.Init();
			hotbar = new HotbarWidget( Window );
			hotbar.Init();
		}
		
		public override bool HandlesAllInput {
			get { return chat.HandlesAllInput; }
		}

		public override bool HandlesKeyPress( char key ) {
			return chat.HandlesKeyPress( key );
		}
		
		public override bool HandlesKeyDown( Key key ) {
			if( key == Window.Keys[KeyMapping.PlayerList] ) {
				if( playerList == null ) {
					playerList = new ExtPlayerListWidget( Window );
					playerList.Init();
				}
			}
			if( chat.HandlesKeyDown( key ) ) {
				return true;
			}
			return hotbar.HandlesKeyDown( key );
		}
		
		public override bool HandlesKeyUp( Key key ) {
			if( key == Window.Keys[KeyMapping.PlayerList] ) {
				if( playerList != null ) {
					playerList.Dispose();
					playerList = null;
					return true;
				}
			}
			return false;
		}
	}
}
