// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using OpenTK.Input;
#if ANDROID
using Android.Graphics;
#endif

namespace ClassicalSharp.Gui.Widgets {
	public sealed class ChatInputWidget : InputWidget {
		
		int typingLogPos;		
		public ChatInputWidget( Game game, Font font ) : base( game, font ) {
			typingLogPos = game.Chat.InputLog.Count; // Index of newest entry + 1.
		}
		
		public override bool HandlesKeyDown( Key key ) {
			if( game.HideGui ) return key < Key.F1 || key > Key.F35;
			bool controlDown = inputHandler.ControlDown();
			if( key == Key.Up ) { UpKey( controlDown ); return true; }
			if( key == Key.Down ) { DownKey( controlDown ); return true; }
			
			return inputHandler.HandlesKeyDown( key );
		}
		
		public override void SendAndReset() {			
			base.SendAndReset();
			typingLogPos = game.Chat.InputLog.Count; // Index of newest entry + 1.
		}
		
		
		void UpKey( bool controlDown ) {
			if( controlDown ) {
				int pos = caretPos == -1 ? buffer.Length : caretPos;
				if( pos < LineLength ) return;
				
				caretPos = pos - LineLength;
				CalculateCaretData();
				return;
			}
			
			if( typingLogPos == game.Chat.InputLog.Count )
				originalText = buffer.ToString();
			if( game.Chat.InputLog.Count > 0 ) {
				typingLogPos--;
				if( typingLogPos < 0 ) typingLogPos = 0;
				
				buffer.Clear();
				buffer.Append( 0, game.Chat.InputLog[typingLogPos] );
				caretPos = -1;
				Recreate();
			}
		}
		
		void DownKey( bool controlDown ) {
			if( controlDown ) {
				if( caretPos == -1 || caretPos >= (parts.Length - 1) * LineLength ) return;
				caretPos += LineLength;
				CalculateCaretData();
				return;
			}
			
			if( game.Chat.InputLog.Count > 0 ) {
				typingLogPos++;
				buffer.Clear();
				if( typingLogPos >= game.Chat.InputLog.Count ) {
					typingLogPos = game.Chat.InputLog.Count;
					if( originalText != null )
						buffer.Append( 0, originalText );
				} else {
					buffer.Append( 0, game.Chat.InputLog[typingLogPos] );
				}
				caretPos = -1;
				Recreate();
			}
		}
	}
}