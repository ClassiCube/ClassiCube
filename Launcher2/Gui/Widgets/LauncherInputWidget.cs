// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using System.Windows.Forms;
using ClassicalSharp;

namespace Launcher.Gui.Widgets {

	/// <summary> Widget that represents text can have modified by the user. </summary>
	public sealed class LauncherInputWidget : LauncherWidget {
		
		public int ButtonWidth, ButtonHeight;
		
		/// <summary> Text displayed when the user has not entered anything in the text field. </summary>
		public string HintText;
		
		/// <summary> Whether the input widget currently is focused through a mouse click or tab. </summary>
		public bool Active;
		
		/// <summary> Whether all characters should be rendered as *. </summary>
		public bool Password;
		
		/// <summary> Maximum number of characters that the 'Text' field can contain. </summary>
		public int MaxTextLength = 32;
		
		/// <summary> Filter applied to text received from the clipboard. Can be null. </summary>
		public Func<string, string> ClipboardFilter;
		
		/// <summary> Delegate invoked when the text changes. </summary>
		public Action<LauncherInputWidget> TextChanged;
		
		/// <summary> Delegate that only lets certain characters be entered. </summary>
		public Func<char, bool> TextFilter;
		
		public int CaretPos = -1;
		
		Font font, hintFont;
		int textHeight;
		public LauncherInputWidget( LauncherWindow window ) : base( window ) {
		}

		public void SetDrawData( IDrawer2D drawer, string text, Font font, Font hintFont,
		                   Anchor horAnchor, Anchor verAnchor, int width, int height, int x, int y ) {
			ButtonWidth = width; ButtonHeight = height;
			Width = width; Height = height;
			CalculateOffset( x, y, horAnchor, verAnchor );
			
			Text = text;
			if( Password ) text = new String( '*', text.Length );
			this.font = font;
			this.hintFont = hintFont;
			
			DrawTextArgs args = new DrawTextArgs( text, font, true );
			Size size = drawer.MeasureSize( ref args );
			Width = Math.Max( ButtonWidth, size.Width + 15 );
			textHeight = size.Height;
		}
		
		public void SetDrawData( IDrawer2D drawer, string text ) {
			Text = text;
			if( Password ) text = new String( '*', text.Length );
			
			DrawTextArgs args = new DrawTextArgs( text, font, true );
			Size size = drawer.MeasureSize( ref args );
			Width = Math.Max( ButtonWidth, size.Width + 15 );
			textHeight = size.Height;		
		}
		
		public override void Redraw( IDrawer2D drawer ) {
			string text = Text;
			if( Password ) text = new String( '*', text.Length );
			DrawTextArgs args = new DrawTextArgs( text, font, true );
			
			Size size = drawer.MeasureSize( ref args );
			Width = Math.Max( ButtonWidth, size.Width + 15 );
			textHeight = size.Height;	
			args.SkipPartsCheck = true;			
			if( Window.Minimised ) return;
			
			FastColour col = Active ? new FastColour( 240, 240, 240 ) : new FastColour( 180, 180, 180 );
			drawer.Clear( col, X + 1, Y, Width - 2, 2 );
			drawer.Clear( col, X + 1, Y + Height - 2, Width - 2, 2 );
			drawer.Clear( col, X, Y + 1, 2, Height - 2 );
			drawer.Clear( col, X + Width - 2, Y + 1, 2, Height - 2 );
			drawer.Clear( FastColour.Black, X + 2, Y + 2, Width - 4, Height - 4 );
			
			if( Text.Length != 0 || HintText == null ) {
				int y = Y + 2 + (Height - textHeight) / 2;
				drawer.DrawText( ref args, X + 5, y );
			} else {
				args.SkipPartsCheck = false;
				args.Text = HintText;
				args.Font = hintFont;
				
				Size hintSize = drawer.MeasureSize( ref args );
				int y = Y + (Height - hintSize.Height) / 2;
				args.SkipPartsCheck = true;
				drawer.DrawText( ref args, X + 5, y );
			}
		}
		
		/// <summary> Appends a character to the end of the currently entered text. </summary>
		/// <returns> true if a redraw is necessary, false otherwise. </returns>
		public bool AppendChar( char c ) {
			if( TextFilter != null && !TextFilter( c ) )
				return false;
			if( c >= ' ' && c <= '~' && c != '&' && Text.Length < MaxTextLength ) {
				if( CaretPos == -1 ) {
					Text += c;
				} else {
					Text = Text.Insert( CaretPos, new String( c, 1 ) );
					CaretPos++;
				}
				if( TextChanged != null ) TextChanged( this );
				return true;
			}
			return false;
		}
		
		/// <summary> Removes the character preceding the caret in the currently entered text. </summary>
		/// <returns> true if a redraw is necessary, false otherwise. </returns>
		public bool BackspaceChar() {
			if( Text.Length == 0 ) return false;
			
			if( CaretPos == -1 ) {
				Text = Text.Substring( 0, Text.Length - 1 );
			} else {
				if( CaretPos == 0 ) return false;
				Text = Text.Remove( CaretPos - 1, 1 );
				CaretPos--;
				if( CaretPos == -1 ) CaretPos = 0;
			}
			
			if( TextChanged != null ) TextChanged( this );
			if( CaretPos >= Text.Length )
				CaretPos = -1;
			return true;
		}
		
		/// <summary> Removes the haracter at the caret in the currently entered text. </summary>
		/// <returns> true if a redraw is necessary, false otherwise. </returns>
		public bool DeleteChar() {
			if( Text.Length == 0 || CaretPos == -1 ) return false;
			
			Text = Text.Remove( CaretPos, 1 );
			if( CaretPos == -1 ) CaretPos = 0;
			
			if( TextChanged != null ) TextChanged( this );
			if( CaretPos >= Text.Length )
				CaretPos = -1;
			return true;
		}
		
		/// <summary> Resets the currently entered text to an empty string </summary>
		/// <returns> true if a redraw is necessary, false otherwise. </returns>
		public bool ClearText() {
			if( Text.Length == 0 ) return false;
			
			Text = "";
			if( TextChanged != null ) TextChanged( this );
			CaretPos = -1;
			return true;
		}
		
		/// <summary> Copies the contents of the currently entered text to the system clipboard. </summary>
		public void CopyToClipboard() {
			if( !String.IsNullOrEmpty( Text ) )
				Clipboard.SetText( Text );
		}
		
		/// <summary> Sets the currently entered text to the contents of the system clipboard. </summary>
		/// <returns> true if a redraw is necessary, false otherwise. </returns>
		public bool CopyFromClipboard() {
			string text = Clipboard.GetText();
			if( String.IsNullOrEmpty( text ) || Text.Length >= MaxTextLength ) return false;
			
			if( ClipboardFilter != null )
				text = ClipboardFilter( text );
			if( Text.Length + text.Length > MaxTextLength ) {
				text = text.Substring( 0, MaxTextLength - Text.Length );
			}
			Text += text;
			if( TextChanged != null ) TextChanged( this );
			return true;
		}
		
		public Rectangle MeasureCaret( IDrawer2D drawer, Font font ) {
			string text = Text;
			if( Password )
				text = new String( '*', text.Length );			
			Rectangle r = new Rectangle( X + 5, Y + Height - 5, 0, 2 );
			DrawTextArgs args = new DrawTextArgs( text, font, true );
			
			if( CaretPos == -1 ) {
				Size size = drawer.MeasureSize( ref args );
				r.X += size.Width; r.Width = 10;
			} else {
				args.Text = text.Substring( 0, CaretPos );
				int trimmedWidth = drawer.MeasureChatSize( ref args ).Width;
				args.Text = new String( text[CaretPos], 1 );
				int charWidth = drawer.MeasureChatSize( ref args ).Width;
				r.X += trimmedWidth; r.Width = charWidth;
			}
			return r;
		}
		
		public void AdvanceCursorPos( int dir ) {
			if( (CaretPos == 0 && dir == -1) || (CaretPos == -1 && dir == 1) )
				return;
			if( CaretPos == -1 && dir == -1 )
				CaretPos = Text.Length;
			
			CaretPos += dir;
			if( CaretPos < 0 || CaretPos >= Text.Length )
				CaretPos = -1;
		}
		
		public void SetCaretToCursor( int mouseX, int mouseY, IDrawer2D drawer, Font font ) {
			string text = Text;
			if( Password )
				text = new String( '*', text.Length );
			mouseX -= X; mouseY -= Y;
			
			DrawTextArgs args = new DrawTextArgs( text, font, true );
			Size size = drawer.MeasureSize( ref args );
			if( mouseX >= size.Width ) {
				CaretPos = -1; return;
			}
			
			for( int i = 0; i < Text.Length; i++ ) {
				args.Text = text.Substring( 0, i );
				int trimmedWidth = drawer.MeasureChatSize( ref args ).Width;
				args.Text = new String( text[i], 1 );
				int charWidth = drawer.MeasureChatSize( ref args ).Width;
				if( mouseX >= trimmedWidth && mouseX < trimmedWidth + charWidth ) {
					CaretPos = i; return;
				}
			}
		}
	}
}
