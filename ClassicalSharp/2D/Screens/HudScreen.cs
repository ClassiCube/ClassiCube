// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Gui.Widgets;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	public class HudScreen : Screen, IGameComponent {
		
		public HudScreen( Game game ) : base( game ) { }
		
		ChatScreen chat;
		internal BlockHotbarWidget hotbar;
		PlayerListWidget playerList;
		Font playerFont;
		
		public void Init( Game game ) { }
		public void Ready( Game game) { Init(); }
		public void Reset( Game game ) { }
		public void OnNewMap( Game game ) { }
		public void OnNewMapLoaded( Game game ) { }
		
		internal int BottomOffset { get { return hotbar.Height; } }
		
		public override void Render( double delta ) {
			if( game.HideGui ) return;
			
			bool showMinimal = game.Gui.ActiveScreen.BlocksWorld;
			if( chat.HandlesAllInput && !game.PureClassic )
				chat.RenderBackground();
			
			gfx.Texturing = true;
			if( !showMinimal ) hotbar.Render( delta );
			chat.Render( delta );
			
			if( playerList != null && game.Gui.ActiveScreen == this ) {
				playerList.Render( delta );
				// NOTE: Should usually be caught by KeyUp, but just in case.
				if( !game.IsKeyDown( KeyBind.PlayerList ) ) {
					playerList.Dispose();
					playerList = null;
				}
			}
			
			if( playerList == null && !showMinimal )
				DrawCrosshairs();
			gfx.Texturing = false;
		}
		
		const int chExtent = 16, chWeight = 2;
		static TextureRec chRec = new TextureRec( 0, 0, 15/256f, 15/256f );
		void DrawCrosshairs() {
			int cenX = game.Width / 2, cenY = game.Height / 2;
			if( game.Gui.IconsTex > 0 ) {
				int extent = (int)(chExtent * game.Scale( game.Height / 480f ) );
				Texture chTex = new Texture( game.Gui.IconsTex, cenX - extent,
				                            cenY - extent, extent * 2, extent * 2, chRec );
				chTex.Render( gfx );
				return;
			}
			
			gfx.Texturing = false;
			int curCol = 150 + (int)(50 * Math.Abs( Math.Sin( game.accumulator ) ));
			FastColour col = new FastColour( curCol, curCol, curCol );
			gfx.Draw2DQuad( cenX - chExtent, cenY - chWeight, chExtent * 2, chWeight * 2, col );
			gfx.Draw2DQuad( cenX - chWeight, cenY - chExtent, chWeight * 2, chExtent * 2, col );
		}
		
		public override void Dispose() {
			playerFont.Dispose();
			chat.Dispose();
			hotbar.Dispose();
			if( playerList != null )
				playerList.Dispose();
		}
		
		public void GainFocus() {
			game.CursorVisible = false;
			if( game.Focused )
				game.Camera.RegrabMouse();
		}
		
		public void LoseFocus() {
			game.CursorVisible = true;
		}
		
		public override void OnResize( int width, int height ) {
			PlayerListWidget widget = playerList;
			chat.OnResize( width, height );
			hotbar.OnResize( width, height );
			if( widget != null )
				CreatePlayerListWidget();
		}
		
		public override void Init() {
			int size = game.Drawer2D.UseBitmappedChat ? 16 : 11;
			playerFont = new Font( game.FontName, size );
			hotbar = new BlockHotbarWidget( game );
			hotbar.Init();
			chat = new ChatScreen( game, this );
			chat.Init();
			game.WorldEvents.OnNewMap += OnNewMap;
		}

		void OnNewMap( object sender, EventArgs e ) {
			if( playerList != null )
				playerList.Dispose();
			playerList = null;
		}
		
		public override bool HandlesAllInput { get { return chat.HandlesAllInput; } }

		public override bool HandlesKeyPress( char key ) {
			return chat.HandlesKeyPress( key );
		}
		
		public override bool HandlesKeyDown( Key key ) {
			Key playerListKey = game.Mapping( KeyBind.PlayerList );
			bool handles = playerListKey != Key.Tab || !game.TabAutocomplete || !chat.HandlesAllInput;
			if( key == playerListKey && handles ) {
				if( playerList == null && !game.Server.IsSinglePlayer )
					CreatePlayerListWidget();
				return true;
			}
			
			if( chat.HandlesKeyDown( key ) )
				return true;
			return hotbar.HandlesKeyDown( key );
		}
		
		void CreatePlayerListWidget() {
			if( game.UseClassicTabList ) {
				playerList = new ClassicPlayerListWidget( game, playerFont );
			} else if( game.Server.UsingExtPlayerList ) {
				playerList = new ExtPlayerListWidget( game, playerFont );
			} else {
				playerList = new NormalPlayerListWidget( game, playerFont );
			}
			playerList.Init();
			playerList.MoveTo( playerList.X, game.Height / 4 );
		}
		
		public override bool HandlesKeyUp( Key key ) {
			if( key == game.Mapping( KeyBind.PlayerList ) ) {
				if( playerList != null ) {
					playerList.Dispose();
					playerList = null;
					return true;
				}
			}
			return false;
		}
		
		public void OpenTextInputBar( string text ) {
			chat.OpenTextInputBar( text );
		}
		
		public override bool HandlesMouseScroll( int delta ) {
			return chat.HandlesMouseScroll( delta );
		}
		
		public override bool HandlesMouseClick( int mouseX, int mouseY, MouseButton button ) {
			if( button != MouseButton.Left || !HandlesAllInput ) return false;
			
			string name;
			if( playerList == null || (name = playerList.GetNameUnder( mouseX, mouseY )) == null )
				return chat.HandlesMouseClick( mouseX, mouseY, button );
			chat.AppendTextToInput( name + " " );
			return true;
		}
	}
}
