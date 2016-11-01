// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Collections.Generic;
using System.Drawing;
using ClassicalSharp.Entities;
using OpenTK.Input;
#if ANDROID
using Android.Graphics;
#endif

namespace ClassicalSharp.Gui.Widgets {
	public sealed class ChatInputWidget : InputWidget {

		public ChatInputWidget( Game game, Font font ) : base( game, font ) {
			typingLogPos = game.Chat.InputLog.Count; // Index of newest entry + 1.
			ShowCaret = true;
		}

		static FastColour backColour = new FastColour( 0, 0, 0, 127 );
		int typingLogPos;
		string originalText;
		bool shownWarning;		
		
		public override int MaxLines { get { return game.ClassicMode ? 1 : 3; } }
		public override string Prefix { get { return "> "; } }
		public override int Padding { get { return 5; } }
		public override int MaxCharsPerLine {
			get {
				bool allChars = game.ClassicMode || game.Server.SupportsPartialMessages;
				return allChars ? 64 : 62;
			}
		}
		
		public override void Init() {
			base.Init();
			bool supports = game.Server.SupportsPartialMessages;
			
			if( Text.Length > MaxCharsPerLine && !shownWarning && !supports ) {
				game.Chat.Add( "&eNote: Each line will be sent as a separate packet.", MessageType.ClientStatus6 );
				shownWarning = true;
			} else if( Text.Length <= MaxCharsPerLine && shownWarning ) {
				game.Chat.Add( null, MessageType.ClientStatus6 );
				shownWarning = false;
			}
		}
		
		public override void Render( double delta ) {
			gfx.Texturing = false;
			int y = Y, x = X;
			
			for( int i = 0; i < lineSizes.Length; i++ ) {
				if( i > 0 && lineSizes[i].Height == 0 ) break;
				bool caretAtEnd = (caretRow == i) && (caretCol == MaxCharsPerLine || caret == -1);
				int drawWidth = lineSizes[i].Width + (caretAtEnd ? caretTex.Width : 0);
				// Cover whole window width to match original classic behaviour
				if( game.PureClassic )
					drawWidth = Math.Max( drawWidth, game.Width - X * 4 );
				
				gfx.Draw2DQuad( x, y, drawWidth + Padding * 2, prefixHeight, backColour );
				y += lineSizes[i].Height;
			}
			
			gfx.Texturing = true;		
			inputTex.Render( gfx );
			RenderCaret( delta );
		}

		
		public override void EnterInput() {
			SendChat();
			originalText = null;
			typingLogPos = game.Chat.InputLog.Count; // Index of newest entry + 1.
			
			game.Chat.Add( null, MessageType.ClientStatus4 );
			game.Chat.Add( null, MessageType.ClientStatus5 );
			game.Chat.Add( null, MessageType.ClientStatus6 );
			base.EnterInput();
		}
		
		
		void SendChat() {
			if( Text.Empty ) return;
			// Don't want trailing spaces in output message
			string allText = new String( Text.value, 0, Text.TextLength );
			game.Chat.InputLog.Add( allText );
			
			if( game.Server.SupportsPartialMessages ) {
				SendWithPartial( allText );
			} else {
				SendNormal();
			}
		}
		
		void SendWithPartial( string allText ) {
			// don't automatically word wrap the message.
			while( allText.Length > Utils.StringLength ) {
				game.Chat.Send( allText.Substring( 0, Utils.StringLength ), true );
				allText = allText.Substring( Utils.StringLength );
			}
			game.Chat.Send( allText, false );
		}
		
		void SendNormal() {
			int packetsCount = 0;
			for( int i = 0; i < lines.Length; i++ ) {
				if( lines[i] == null ) break;
				packetsCount++;
			}
			
			// split up into both partial and final packet.
			for( int i = 0; i < packetsCount - 1; i++ )
				SendNormalText( i, true );
			SendNormalText( packetsCount - 1, false );
		}
		
		void SendNormalText( int i, bool partial ) {
			string text = lines[i];
			char lastCol = GetLastColour( 0, i );
			if( !IDrawer2D.IsWhiteColour( lastCol ) )
				text = "&" + lastCol + text;
			game.Chat.Send( text, partial );
		}
		
		
		#region Input handling
		
		public override bool HandlesKeyDown( Key key ) {
			if( game.HideGui ) return key < Key.F1 || key > Key.F35;
			bool controlDown = ControlDown();
			
			if( key == Key.Tab ) { TabKey(); return true; }
			if( key == Key.Up ) { UpKey( controlDown ); return true; }
			if( key == Key.Down ) { DownKey( controlDown ); return true; }
			
			return base.HandlesKeyDown( key );
		}
		
		void UpKey( bool controlDown ) {
			if( controlDown ) {
				int pos = caret == -1 ? Text.Length : caret;
				if( pos < MaxCharsPerLine ) return;
				
				caret = pos - MaxCharsPerLine;
				UpdateCaret();
				return;
			}
			
			if( typingLogPos == game.Chat.InputLog.Count )
				originalText = Text.ToString();
			if( game.Chat.InputLog.Count > 0 ) {
				typingLogPos--;
				if( typingLogPos < 0 ) typingLogPos = 0;
				
				Text.Clear();
				Text.Append( 0, game.Chat.InputLog[typingLogPos] );
				caret = -1;
				Recreate();
			}
		}
		
		void DownKey( bool controlDown ) {
			if( controlDown ) {
				if( caret == -1 || caret >= (lines.Length - 1) * MaxCharsPerLine ) return;
				caret += MaxCharsPerLine;
				UpdateCaret();
				return;
			}
			
			if( game.Chat.InputLog.Count > 0 ) {
				typingLogPos++;
				Text.Clear();
				if( typingLogPos >= game.Chat.InputLog.Count ) {
					typingLogPos = game.Chat.InputLog.Count;
					if( originalText != null )
						Text.Append( 0, originalText );
				} else {
					Text.Append( 0, game.Chat.InputLog[typingLogPos] );
				}
				caret = -1;
				Recreate();
			}
		}
		
		void TabKey() {
			int pos = caret == -1 ? Text.Length - 1 : caret;
			int start = pos;
			char[] value = Text.value;
			
			while( start >= 0 && IsNameChar( value[start] ) )
				start--;
			start++;
			if( pos < 0 || start > pos ) return;
			
			string part = new String( value, start, pos + 1 - start );
			List<string> matches = new List<string>();
			game.Chat.Add( null, MessageType.ClientStatus5 );
			
			TabListEntry[] entries = game.TabList.Entries;
			for( int i = 0; i < EntityList.MaxCount; i++ ) {
				if( entries[i] == null ) continue;
				
				string rawName = entries[i].PlayerName;
				string name = Utils.StripColours( rawName );
				if( name.StartsWith( part, StringComparison.OrdinalIgnoreCase ) )
					matches.Add( name );
			}
			
			if( matches.Count == 1 ) {
				if( caret == -1 ) pos++;
				int len = pos - start;
				for( int i = 0; i < len; i++ )
					Text.DeleteAt( start );
				if( caret != -1 ) caret -= len;
				Append( matches[0] );
			} else if( matches.Count > 1 ) {
				StringBuffer sb = new StringBuffer( Utils.StringLength );
				int index = 0;
				sb.Append( ref index, "&e" );
				sb.AppendNum( ref index, matches.Count );
				sb.Append( ref index, " matching names: " );
				
				for( int i = 0; i < matches.Count; i++) {
					string match = matches[i];
					if( (sb.Length + match.Length + 1) > sb.Capacity ) break;
					sb.Append( ref index, match );
					sb.Append( ref index, ' ' );
				}
				game.Chat.Add( sb.ToString(), MessageType.ClientStatus5 );
			}
		}
		
		bool IsNameChar( char c ) {
			return c == '_' || c == '.' || (c >= '0' && c <= '9')
				|| (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
		}
		
		#endregion
	}
}