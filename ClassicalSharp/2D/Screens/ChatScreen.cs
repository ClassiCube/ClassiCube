using System;
using System.Collections.Generic;
using System.Drawing;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public class ChatScreen : Screen {
		
		public ChatScreen( Game window ) : base( window ) {
		}
		
		public int ChatInputYOffset, ChatLogYOffset;
		public bool HistoryMode;
		const int chatLines = 12;
		Texture announcementTex;
		TextInputWidget textInput;
		TextGroupWidget status, bottomRight, normalChat;
		bool suppressNextPress = true;
		DateTime announcementDisplayTime;
		int chatIndex;
		
		public override void Render( double delta ) {
			normalChat.Render( delta );
			status.Render( delta );
			bottomRight.Render( delta );
			if( announcementTex.IsValid ) {
				announcementTex.Render( graphicsApi );
			}
			if( HandlesAllInput ) {
				textInput.Render( delta );
			}
			if( game.Announcement != null && ( DateTime.UtcNow - announcementDisplayTime ).TotalSeconds > 5 ) {
				game.Announcement = null;
				graphicsApi.DeleteTexture( ref announcementTex );
			}
		}
		
		Font chatFont, chatBoldFont, historyFont, announcementFont;
		public override void Init() {
			chatFont = new Font( "Arial", game.ChatFontSize );
			chatBoldFont = new Font( "Arial", game.ChatFontSize, FontStyle.Bold );
			announcementFont = new Font( "Arial", 14 );
			historyFont = new Font( "Arial", 12, FontStyle.Italic );
			
			textInput = new TextInputWidget( game, chatFont, chatBoldFont );
			textInput.ChatInputYOffset = ChatInputYOffset;
			status = new TextGroupWidget( game, 3, chatFont );
			status.VerticalDocking = Docking.LeftOrTop;
			status.HorizontalDocking = Docking.BottomOrRight;
			status.Init();
			bottomRight = new TextGroupWidget( game, 3, chatFont );
			bottomRight.VerticalDocking = Docking.BottomOrRight;
			bottomRight.HorizontalDocking = Docking.BottomOrRight;
			bottomRight.YOffset = ChatInputYOffset;
			bottomRight.Init();
			normalChat = new TextGroupWidget( game, chatLines, chatFont );
			normalChat.XOffset = 10;
			normalChat.YOffset = ChatLogYOffset;
			normalChat.HorizontalDocking = Docking.LeftOrTop;
			normalChat.VerticalDocking = Docking.BottomOrRight;
			normalChat.Init();
			
			chatIndex = game.ChatLog.Count - chatLines;
			ResetChat();
			status.SetText( 0, game.Status1 );
			status.SetText( 1, game.Status2 );
			status.SetText( 2, game.Status3 );
			bottomRight.SetText( 2, game.BottomRight1 );
			bottomRight.SetText( 1, game.BottomRight2 );
			bottomRight.SetText( 0, game.BottomRight3 );
			UpdateAnnouncement( game.Announcement );
			if( !String.IsNullOrEmpty( game.chatInInputBuffer ) ) {
				OpenTextInputBar( game.chatInInputBuffer );
				game.chatInInputBuffer = null;
			}
			game.ChatReceived += ChatReceived;
		}

		void ChatReceived( object sender, ChatEventArgs e ) {
			CpeMessage type = e.Type;
			if( type == CpeMessage.Normal ) {
				chatIndex++;
				List<string> chat = game.ChatLog;
				normalChat.PushUpAndReplaceLast( chat[chatIndex + chatLines - 1] );
			} else if( type >= CpeMessage.Status1 && type <= CpeMessage.Status3 ) {
				status.SetText( (int)( type - CpeMessage.Status1 ), e.Text );
			} else if( type >= CpeMessage.BottomRight1 && type <= CpeMessage.BottomRight3 ) {
				bottomRight.SetText( 2 - (int)( type - CpeMessage.BottomRight1 ), e.Text );
			} else if( type == CpeMessage.Announcement ) {
				UpdateAnnouncement( e.Text );
			}
		}

		public override void Dispose() {
			if( !textInput.chatInputText.Empty ) {
				if( game.CursorVisible )
					game.CursorVisible = false;
				game.chatInInputBuffer = textInput.chatInputText.ToString();
			}
			chatFont.Dispose();
			chatBoldFont.Dispose();
			historyFont.Dispose();
			announcementFont.Dispose();
			
			normalChat.Dispose();
			textInput.Dispose();
			status.Dispose();
			bottomRight.Dispose();
			graphicsApi.DeleteTexture( ref announcementTex );
			game.ChatReceived -= ChatReceived;
		}
		
		public override void OnResize( int oldWidth, int oldHeight, int width, int height ) {
			announcementTex.X1 += ( width - oldWidth ) / 2;
			announcementTex.Y1 += ( height - oldHeight ) / 2;
			textInput.OnResize( oldWidth, oldHeight, width, height );
			status.OnResize( oldWidth, oldHeight, width, height );
			bottomRight.OnResize( oldWidth, oldHeight, width, height );
			normalChat.OnResize( oldWidth, oldHeight, width, height );
		}
		
		void UpdateAnnouncement( string text ) {
			announcementDisplayTime = DateTime.UtcNow;
			DrawTextArgs args = new DrawTextArgs( graphicsApi, text, true );
			announcementTex = Utils2D.MakeTextTexture( announcementFont, 0, 0, ref args );
			announcementTex.X1 = game.Width / 2 - announcementTex.Width / 2;
			announcementTex.Y1 = game.Height / 4 - announcementTex.Height / 2;
		}
		
		void ResetChat() {
			normalChat.Dispose();
			List<string> chat = game.ChatLog;
			for( int i = chatIndex; i < chatIndex + chatLines; i++ ) {
				if( i >= 0 && i < chat.Count )
					normalChat.PushUpAndReplaceLast( chat[i] );
			}
		}
		
		public override bool HandlesKeyPress( char key ) {
			if( !HandlesAllInput ) return false;
			if( suppressNextPress ) {
				suppressNextPress = false;
				return false;
			}
			return textInput.HandlesKeyPress( key );
		}
		
		void OpenTextInputBar( string initialText ) {
			if( !game.CursorVisible )
				game.CursorVisible = true;
			suppressNextPress = true;
			HandlesAllInput = true;
			textInput.chatInputText.Clear();
			textInput.chatInputText.Append( 0, initialText );
			textInput.Init();
		}
		
		public override bool HandlesKeyDown( Key key ) {
			suppressNextPress = false;

			if( HandlesAllInput ) { // text input bar
				if( key == game.Keys[KeyMapping.SendChat] ) {
					HandlesAllInput = false;
					if( game.CursorVisible )
						game.CursorVisible = false;
					game.Camera.RegrabMouse();
					textInput.SendTextInBufferAndReset();
				} else if( key == Key.PageUp ) {
					chatIndex -= chatLines;
					int minIndex = Math.Min( 0, game.ChatLog.Count - chatLines );
					if( chatIndex < minIndex )
						chatIndex = minIndex;
					ResetChat();
				} else if( key == Key.PageDown ) {
					chatIndex += chatLines;
					if( chatIndex > game.ChatLog.Count - chatLines )
						chatIndex = game.ChatLog.Count - chatLines;
					ResetChat();
				} else {
					textInput.HandlesKeyDown( key );
				}
				return true;
			}

			if( key == game.Keys[KeyMapping.OpenChat] ) {
				OpenTextInputBar( "" );
			} else if( key == Key.Slash ) {
				OpenTextInputBar( "/" );
			} else {
				return false;
			}
			return true;
		}
		
		public override bool HandlesMouseScroll( int delta ) {
			if( !HandlesAllInput ) return false;
			chatIndex += -delta;
			int maxIndex = game.ChatLog.Count - chatLines;
			int minIndex = Math.Min( 0, maxIndex );	
			Utils.Clamp( ref chatIndex, minIndex, maxIndex );
			ResetChat();
			return true;
		}
	}
}