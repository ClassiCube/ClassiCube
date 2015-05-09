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
		Texture announcementTexture;
		TextInputWidget textInput;
		TextGroupWidget status, bottomRight, normalChat;
		bool suppressNextPress = true;
		DateTime announcementDisplayTime;

		int pageNumber, pagesCount;
		static readonly Color backColour = Color.FromArgb( 120, 60, 60, 60 );
		int chatIndex;
		Texture pageTexture;
		
		public override void Render( double delta ) {
			normalChat.Render( delta );
			status.Render( delta );
			bottomRight.Render( delta );
			if( announcementTexture.IsValid ) {
				announcementTexture.Render( GraphicsApi );
			}
			if( HandlesAllInput ) {
				textInput.Render( delta );
			}
			if( Window.Announcement != null && ( DateTime.UtcNow - announcementDisplayTime ).TotalSeconds > 5 ) {
				Window.Announcement = null;
				GraphicsApi.DeleteTexture( ref announcementTexture );
			}
			if( HistoryMode ) {
				pageTexture.Render( GraphicsApi );
			}
		}
		
		Font chatFont, chatBoldFont, historyFont, announcementFont;
		public override void Init() {
			chatFont = new Font( "Arial", Window.ChatFontSize );
			chatBoldFont = new Font( "Arial", Window.ChatFontSize, FontStyle.Bold );
			announcementFont = new Font( "Arial", 14 );
			historyFont = new Font( "Arial", 12, FontStyle.Italic );
			
			textInput = new TextInputWidget( Window, chatFont, chatBoldFont );
			textInput.ChatInputYOffset = ChatInputYOffset;
			status = new TextGroupWidget( Window, 3, chatFont );
			status.VerticalDocking = Docking.LeftOrTop;
			status.HorizontalDocking = Docking.BottomOrRight;
			status.Init();
			bottomRight = new TextGroupWidget( Window, 3, chatFont );
			bottomRight.VerticalDocking = Docking.BottomOrRight;
			bottomRight.HorizontalDocking = Docking.BottomOrRight;
			bottomRight.YOffset = ChatInputYOffset;
			bottomRight.Init();
			normalChat = new TextGroupWidget( Window, chatLines, chatFont );
			normalChat.XOffset = 10;
			normalChat.YOffset = ChatLogYOffset;
			normalChat.HorizontalDocking = Docking.LeftOrTop;
			normalChat.VerticalDocking = Docking.BottomOrRight;
			normalChat.Init();
			
			InitChat();
			status.SetText( 0, Window.Status1 );
			status.SetText( 1, Window.Status2 );
			status.SetText( 2, Window.Status3 );
			bottomRight.SetText( 0, Window.BottomRight1 );
			bottomRight.SetText( 1, Window.BottomRight2 );
			bottomRight.SetText( 2, Window.BottomRight3 );
			UpdateAnnouncement( Window.Announcement );
			if( !String.IsNullOrEmpty( Window.chatInInputBuffer ) ) {
				OpenTextInputBar( Window.chatInInputBuffer );
				Window.chatInInputBuffer = null;
			}
			Window.ChatReceived += ChatReceived;
		}

		void ChatReceived( object sender, ChatEventArgs e ) {
			CpeMessageType type = (CpeMessageType)e.Type;
			if( type == CpeMessageType.Normal ) UpdateChat( e.Text );
			else if( type == CpeMessageType.Status1 ) status.SetText( 0, e.Text );
			else if( type == CpeMessageType.Status2 ) status.SetText( 1, e.Text );
			else if( type == CpeMessageType.Status3 ) status.SetText( 2, e.Text );
			else if( type == CpeMessageType.BottomRight1 ) bottomRight.SetText( 2, e.Text );
			else if( type == CpeMessageType.BottomRight2 ) bottomRight.SetText( 1, e.Text );
			else if( type == CpeMessageType.BottomRight3 ) bottomRight.SetText( 0, e.Text );
			else if( type == CpeMessageType.Announcement ) UpdateAnnouncement( e.Text );
		}

		public override void Dispose() {
			if( !String.IsNullOrEmpty( textInput.chatInputText ) ) {
				Window.chatInInputBuffer = textInput.chatInputText;
			}
			chatFont.Dispose();
			chatBoldFont.Dispose();
			historyFont.Dispose();
			announcementFont.Dispose();
			
			normalChat.Dispose();
			textInput.Dispose();
			status.Dispose();
			bottomRight.Dispose();
			GraphicsApi.DeleteTexture( ref pageTexture );
			GraphicsApi.DeleteTexture( ref announcementTexture );
			Window.ChatReceived -= ChatReceived;
		}
		
		public override void OnResize( int oldWidth, int oldHeight, int width, int height ) {
			announcementTexture.X1 += ( width - oldWidth ) / 2;
			announcementTexture.Y1 += ( height - oldHeight ) / 2;
			pageTexture.Y1 += height - oldHeight;
			textInput.OnResize( oldWidth, oldHeight, width, height );
			status.OnResize( oldWidth, oldHeight, width, height );
			bottomRight.OnResize( oldWidth, oldHeight, width, height );
			normalChat.OnResize( oldWidth, oldHeight, width, height );
		}
		
		void InitChat() {
			if( !HistoryMode ) {
				List<string> chat = Window.ChatLog;
				int count = Math.Min( chat.Count, chatLines );
				for( int i = 0; i < count; i++ ) {
					UpdateChat( chat[chat.Count - count + i] );
				}
			} else {
				pagesCount = Window.ChatLog.Count / chatLines + 1;
				// When opening the screen history, use the most recent 'snapshot' of chat messages.
				pageNumber = pagesCount;
				MakePageNumberTexture();
				SetChatHistoryStart( ( pagesCount - 1 ) * chatLines );
			}
		}
		
		void UpdateAnnouncement( string text ) {
			announcementDisplayTime = DateTime.UtcNow;
			if( !String.IsNullOrEmpty( text ) ) {
				List<DrawTextArgs> parts = Utils.SplitText( GraphicsApi, text, true );
				Size size = Utils2D.MeasureSize( Utils.StripColours( text ), announcementFont, true );
				int x = Window.Width / 2 - size.Width / 2;
				int y = Window.Height / 4 - size.Height / 2;
				announcementTexture = Utils2D.MakeTextTexture( parts, announcementFont, size, x, y );
			} else {
				announcementTexture = new Texture( -1, 0, 0, 0, 0, 0, 0 );
			}
		}
		
		void UpdateChat( string text ) {
			if( !HistoryMode ) {
				normalChat.PushUpAndReplaceLast( text );
			} else {
				if( chatIndex < 0 ) return;
				normalChat.PushUpAndReplaceLast( text );
				chatIndex--;
				int oldPagesCount = pagesCount;
				pageTexture.Y1 = normalChat.CalcUsedY() - pageTexture.Height;
				pagesCount = Window.ChatLog.Count / chatLines + 1;
				if( oldPagesCount != pagesCount ) {
					MakePageNumberTexture();
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
			suppressNextPress = true;
			HandlesAllInput = true;
			textInput.chatInputText = initialText;
			textInput.Init();
		}
		
		public override bool HandlesKeyDown( Key key ) {
			suppressNextPress = false;

			if( HandlesAllInput ) { // text input bar
				if( key == Window.Keys[KeyMapping.SendChat] ) {
					HandlesAllInput = false;
					Window.Camera.RegrabMouse();
					textInput.SendTextInBufferAndReset();
				} else {
					textInput.HandlesKeyDown( key );
				}
				return true;
			}

			if( key == Window.Keys[KeyMapping.OpenChat] ) {
				OpenTextInputBar( "" );
			} else if( key == Key.Slash ) {
				OpenTextInputBar( "/" );
			}  else if( key == Window.Keys[KeyMapping.ChatHistoryMode] ) {
				HistoryMode = !HistoryMode;
				normalChat.Dispose();		
				InitChat();
			} else if( HistoryMode && key == Key.PageUp ) {
				pageNumber--;
				if( pageNumber <= 0 ) pageNumber = 1;
				MakePageNumberTexture();
				SetChatHistoryStart( ( pageNumber - 1 ) * chatLines );
			} else if( HistoryMode && key == Key.PageDown ) {
				pageNumber++;
				if( pageNumber > pagesCount ) pageNumber = pagesCount;
				MakePageNumberTexture();
				SetChatHistoryStart( ( pageNumber - 1 ) * chatLines );
			} else {
				return false;
			}
			return true;
		}
		
		void MakePageNumberTexture() {
			GraphicsApi.DeleteTexture( ref pageTexture );
			string text = "Page " + pageNumber + " of " + pagesCount;
			Size size = Utils2D.MeasureSize( text, historyFont, false );
			int y = normalChat.CalcUsedY() - size.Height;

			using( Bitmap bmp = new Bitmap( size.Width, size.Height ) ) {
				using( Graphics g = Graphics.FromImage( bmp ) ) {
					Utils2D.DrawRect( g, backColour, 0, 0, bmp.Width, bmp.Height );
					DrawTextArgs args = new DrawTextArgs( GraphicsApi, text, Color.Yellow, false );
					Utils2D.DrawText( g, historyFont, args, 0, 0 );
				}
				pageTexture = Utils2D.Make2DTexture( GraphicsApi, bmp, 10, y );
			}
		}
		
		void SetChatHistoryStart( int index ) {
			normalChat.Dispose();
			chatIndex = chatLines - 1;
			List<string> chat = Window.ChatLog;
			int max = Math.Min( chat.Count, index + chatLines );
			for( int i = index; i < max; i++ ) {
				UpdateChat( chat[i] );
			}
		}
	}
}