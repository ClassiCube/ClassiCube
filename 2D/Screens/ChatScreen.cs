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
		TextInputWidget textInput;
		TextGroupWidget normalChat;
		bool suppressNextPress = true;

		int pageNumber, pagesCount;
		static readonly Color backColour = Color.FromArgb( 120, 60, 60, 60 );
		int chatIndex;
		Texture pageTexture;
		
		public override void Render( double delta ) {
			normalChat.Render( delta );
			if( HandlesAllInput ) {
				textInput.Render( delta );
			}
			if( HistoryMode ) {
				pageTexture.Render( GraphicsApi );
			}
		}
		
		public override void Init() {
			int fontSize = Window.ChatFontSize;
			textInput = new TextInputWidget( Window, fontSize );
			textInput.ChatInputYOffset = ChatInputYOffset;
			normalChat = new TextGroupWidget( Window, chatLines, fontSize );
			normalChat.XOffset = 10;
			normalChat.YOffset = ChatLogYOffset;
			normalChat.HorizontalDocking = Docking.LeftOrTop;
			normalChat.VerticalDocking = Docking.BottomOrRight;
			normalChat.Init();
			
			InitChat();
			if( !String.IsNullOrEmpty( Window.chatInInputBuffer ) ) {
				OpenTextInputBar( Window.chatInInputBuffer );
				Window.chatInInputBuffer = null;
			}
			Window.ChatReceived += ChatReceived;
		}

		void ChatReceived( object sender, TextEventArgs e ) {
			UpdateChat( e.Text );
		}

		public override void Dispose() {
			if( !String.IsNullOrEmpty( textInput.chatInputText ) ) {
				Window.chatInInputBuffer = textInput.chatInputText;
			}
			normalChat.Dispose();
			textInput.Dispose();
			GraphicsApi.DeleteTexture( ref pageTexture );
			Window.ChatReceived -= ChatReceived;
		}
		
		public override void OnResize( int oldWidth, int oldHeight, int width, int height ) {
			pageTexture.Y1 += height - oldHeight;
			textInput.OnResize( oldWidth, oldHeight, width, height );
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
			Size size = Utils2D.MeasureSize( text, "Arial", 12, FontStyle.Italic, false );
			int y = normalChat.CalcUsedY() - size.Height;

			using( Bitmap bmp = new Bitmap( size.Width, size.Height ) ) {
				using( Graphics g = Graphics.FromImage( bmp ) ) {
					Utils2D.DrawRect( g, backColour, bmp.Width, bmp.Height );
					DrawTextArgs args = new DrawTextArgs( GraphicsApi, text, Color.Yellow, false );
					Utils2D.DrawText( g, "Arial", 12, FontStyle.Italic, args );
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