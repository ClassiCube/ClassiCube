using System;
using System.Drawing;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public class NormalScreen : Screen {
		
		public NormalScreen( Game window ) : base( window ) {
		}
		
		ChatScreen chat;
		BlockHotbarWidget hotbar;
		Widget playerList;
		Font playerFont;
		
		public override void Render( double delta ) {
			graphicsApi.Texturing = true;
			chat.Render( delta );
			hotbar.Render( delta );
			if( playerList != null ) {
				playerList.Render( delta );
				// NOTE: Should usually be caught by KeyUp, but just in case.
				if( !game.IsKeyDown( KeyMapping.PlayerList ) ) {
					playerList.Dispose();
					playerList = null;
				}
				graphicsApi.Texturing = false;
			} else {
				graphicsApi.Texturing = false;
				DrawCrosshairs();
			}
		}
		
		const int crosshairExtent = 15, crosshairWeight = 2;
		void DrawCrosshairs() {
			int curCol = 150 + (int)( 50 * Math.Abs( Math.Sin( game.accumulator ) ) );
			FastColour col = new FastColour( curCol, curCol, curCol );
			float centreX = game.Width / 2, centreY = game.Height / 2;
			graphicsApi.Draw2DQuad( centreX - crosshairExtent, centreY - crosshairWeight,
			                       crosshairExtent * 2, crosshairWeight * 2, col );
			graphicsApi.Draw2DQuad( centreX - crosshairWeight, centreY - crosshairExtent,
			                       crosshairWeight * 2, crosshairExtent * 2, col );
		}
		
		public override void Dispose() {
			playerFont.Dispose();
			chat.Dispose();
			hotbar.Dispose();
			if( playerList != null ) {
				playerList.Dispose();
			}
			if( !game.CursorVisible )
				game.CursorVisible = true;
		}
		
		public override void OnResize( int oldWidth, int oldHeight, int width, int height ) {
			chat.OnResize( oldWidth, oldHeight, width, height );
			hotbar.OnResize( oldWidth, oldHeight, width, height );
			if( playerList != null ) {
				playerList.OnResize( oldWidth, oldHeight, width, height );
			}
		}
		
		public override void Init() {
			playerFont = new Font( "Arial", 12 );
			chat = new ChatScreen( game );
			chat.game = game;
			const int blockSize = 32;
			chat.ChatLogYOffset = blockSize + blockSize;
			chat.ChatInputYOffset = blockSize + blockSize / 2;
			chat.Init();
			hotbar = new BlockHotbarWidget( game );
			hotbar.Init();
			if( game.CursorVisible )
				game.CursorVisible = false;
		}
		
		public override bool HandlesAllInput {
			get { return chat.HandlesAllInput; }
		}

		public override bool HandlesKeyPress( char key ) {
			return chat.HandlesKeyPress( key );
		}
		
		public override bool HandlesKeyDown( Key key ) {
			if( key == game.Keys[KeyMapping.PlayerList] ) {
				if( playerList == null ) {
					if( game.Network.UsingExtPlayerList ) {
						playerList = new ExtPlayerListWidget( game, playerFont );
					} else {
						playerList = new NormalPlayerListWidget( game, playerFont );
					}
					playerList.Init();
				}
			}
			if( chat.HandlesKeyDown( key ) ) {
				return true;
			}
			return hotbar.HandlesKeyDown( key );
		}
		
		public override bool HandlesKeyUp( Key key ) {
			if( key == game.Keys[KeyMapping.PlayerList] ) {
				if( playerList != null ) {
					playerList.Dispose();
					playerList = null;
					return true;
				}
			}
			return false;
		}
		
		public override bool HandlesMouseScroll( int delta ) {
			return chat.HandlesMouseScroll( delta );
		}
	}
}
