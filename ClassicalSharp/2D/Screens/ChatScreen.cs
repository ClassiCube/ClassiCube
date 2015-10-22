using System;
using System.Collections.Generic;
using System.Drawing;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public class ChatScreen : Screen {
		
		public ChatScreen( Game game ) : base( game ) {
		}
		
		const int chatLines = 12;
		Texture announcementTex;
		TextInputWidget textInput;
		TextGroupWidget status, bottomRight, normalChat;
		bool suppressNextPress = true;
		int chatIndex;
		
		public override void Render( double delta ) {
			status.Render( delta );
			bottomRight.Render( delta );
			
			DateTime now = DateTime.UtcNow;
			if( HandlesAllInput )
				normalChat.Render( delta );
			else
				RenderRecentChat( now, delta );			
			
			if( announcementTex.IsValid )
				announcementTex.Render( graphicsApi );
			if( HandlesAllInput )
				textInput.Render( delta );
			
			if( game.Chat.Announcement.Text != null && announcementTex.IsValid &&
			   (now - game.Chat.Announcement.Received).TotalSeconds > 5 ) {
				graphicsApi.DeleteTexture( ref announcementTex );
			}
		}
		
		void RenderRecentChat( DateTime now, double delta ) {
			int[] metadata = (int[])Metadata;
			for( int i = 0; i < normalChat.Textures.Length; i++ ) {
				Texture texture = normalChat.Textures[i];
				if( !texture.IsValid ) continue;
				
				DateTime received = game.Chat.Log[metadata[i]].Received;
				if( (now - received).TotalSeconds <= 10 )
					texture.Render( graphicsApi );
			}
		}
		
		static FastColour backColour = new FastColour( 60, 60, 60, 180 );
		public void RenderBackground() {
			int height = normalChat.GetUsedHeight();
			int y = normalChat.Y + normalChat.Height - height - 5;
			int x = normalChat.X - 5;
			int width = normalChat.Width + 10;
			
			if( height > 0 )
				graphicsApi.Draw2DQuad( x, y, width, height + 10, backColour );
		}
		
		Font chatFont, chatInputFont, announcementFont;
		public override void Init() {
			chatFont = new Font( "Arial", game.Chat.FontSize );
			chatInputFont = new Font( "Arial", game.Chat.FontSize, FontStyle.Bold );
			announcementFont = new Font( "Arial", 14 );
			const int blockSize = 40;
			
			textInput = new TextInputWidget( game, chatFont, chatInputFont );
			textInput.YOffset = blockSize * 2 + blockSize / 2;
			status = new TextGroupWidget( game, 3, chatFont );
			status.VerticalAnchor = Anchor.LeftOrTop;
			status.HorizontalAnchor = Anchor.BottomOrRight;
			status.Init();
			bottomRight = new TextGroupWidget( game, 3, chatFont );
			bottomRight.VerticalAnchor = Anchor.BottomOrRight;
			bottomRight.HorizontalAnchor = Anchor.BottomOrRight;
			bottomRight.YOffset = blockSize + blockSize / 2;
			bottomRight.Init();
			normalChat = new TextGroupWidget( game, chatLines, chatFont );
			normalChat.XOffset = 10;
			normalChat.YOffset = blockSize * 3;
			normalChat.HorizontalAnchor = Anchor.LeftOrTop;
			normalChat.VerticalAnchor = Anchor.BottomOrRight;
			normalChat.Init();
			int[] indices = new int[chatLines];
			for( int i = 0; i < indices.Length; i++ )
				indices[i] = -1;
			Metadata = indices;
			
			ChatLog chat = game.Chat;
			chatIndex = chat.Log.Count - chatLines;
			ResetChat();
			status.SetText( 0, chat.Status1.Text );
			status.SetText( 1, chat.Status2.Text );
			status.SetText( 2, chat.Status3.Text );
			bottomRight.SetText( 2, chat.BottomRight1.Text );
			bottomRight.SetText( 1, chat.BottomRight2.Text );
			bottomRight.SetText( 0,chat.BottomRight3.Text );
			UpdateAnnouncement( chat.Announcement.Text );
			
			if( !String.IsNullOrEmpty( game.chatInInputBuffer ) ) {
				OpenTextInputBar( game.chatInInputBuffer );
				game.chatInInputBuffer = null;
			}
			game.Events.ChatReceived += ChatReceived;
		}

		void ChatReceived( object sender, ChatEventArgs e ) {
			CpeMessage type = e.Type;
			if( type == CpeMessage.Normal ) {
				chatIndex++;
				List<ChatLine> chat = game.Chat.Log;
				normalChat.PushUpAndReplaceLast( chat[chatIndex + chatLines - 1].Text );
				
				int[] metadata = (int[])Metadata;
				for( int i = 0; i < chatLines - 1; i++ )
					metadata[i] = metadata[i + 1];
				metadata[chatLines - 1] = chatIndex + chatLines - 1;
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
			chatInputFont.Dispose();
			announcementFont.Dispose();
			
			normalChat.Dispose();
			textInput.Dispose();
			status.Dispose();
			bottomRight.Dispose();
			graphicsApi.DeleteTexture( ref announcementTex );
			game.Events.ChatReceived -= ChatReceived;
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
			DrawTextArgs args = new DrawTextArgs( text, announcementFont, true );
			announcementTex = game.Drawer2D.MakeTextTexture( ref args, 0, 0 );
			announcementTex.X1 = game.Width / 2 - announcementTex.Width / 2;
			announcementTex.Y1 = game.Height / 4 - announcementTex.Height / 2;
		}
		
		void ResetChat() {
			normalChat.Dispose();
			List<ChatLine> chat = game.Chat.Log;
			int[] metadata = (int[])Metadata;
			for( int i = 0; i < chatLines; i++ )
				metadata[i] = -1;
			
			for( int i = chatIndex; i < chatIndex + chatLines; i++ ) {
				if( i >= 0 && i < chat.Count ) {
					normalChat.PushUpAndReplaceLast( chat[i].Text );
					metadata[i - chatIndex] = i;
				}
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
			game.Keyboard.KeyRepeat = true;
			textInput.chatInputText.Clear();
			textInput.chatInputText.Append( 0, initialText );
			textInput.Init();
		}
		
		public override bool HandlesKeyDown( Key key ) {
			suppressNextPress = false;

			if( HandlesAllInput ) { // text input bar
				if( key == game.Mapping( KeyMapping.SendChat ) 
				   || key == game.Mapping( KeyMapping.PauseOrExit ) ) {
					HandlesAllInput = false;
					if( game.CursorVisible )
						game.CursorVisible = false;
					game.Camera.RegrabMouse();
					game.Keyboard.KeyRepeat = false;
					
					if( key == game.Mapping( KeyMapping.PauseOrExit ) )
						textInput.chatInputText.Clear();
					textInput.SendTextInBufferAndReset();
				} else if( key == Key.PageUp ) {
					chatIndex -= chatLines;
					int minIndex = Math.Min( 0, game.Chat.Log.Count - chatLines );
					if( chatIndex < minIndex )
						chatIndex = minIndex;
					ResetChat();
				} else if( key == Key.PageDown ) {
					chatIndex += chatLines;
					if( chatIndex > game.Chat.Log.Count - chatLines )
						chatIndex = game.Chat.Log.Count - chatLines;
					ResetChat();
				} else if( key == game.Mapping( KeyMapping.HideGui ) ) {
					game.HideGui = !game.HideGui;
				} else {
					textInput.HandlesKeyDown( key );
				}
				return true;
			}

			if( key == game.Mapping( KeyMapping.OpenChat ) ) {
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
			int maxIndex = game.Chat.Log.Count - chatLines;
			int minIndex = Math.Min( 0, maxIndex );
			Utils.Clamp( ref chatIndex, minIndex, maxIndex );
			ResetChat();
			return true;
		}
	}
}