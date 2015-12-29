using System;
using System.Drawing;
using System.Windows.Forms;
using ClassicalSharp;

namespace Launcher2 {

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
		
		public LauncherInputWidget( LauncherWindow window ) : base( window ) {
		}

		public void DrawAt( IDrawer2D drawer, string text, Font font, Font hintFont, 
		                   Anchor horAnchor, Anchor verAnchor, int width, int height, int x, int y ) {
			ButtonWidth = width; ButtonHeight = height;
			Width = width; Height = height;
			CalculateOffset( x, y, horAnchor, verAnchor );
			Redraw( drawer, text, font, hintFont );
		}
		
		public void Redraw( IDrawer2D drawer, string text, Font font, Font hintFont ) {
			Text = text;
			if( Password )
				text = new String( '*', text.Length );
			DrawTextArgs args = new DrawTextArgs( text, font, true );
			Size size = drawer.MeasureSize( ref args );
			Width = Math.Max( ButtonWidth, size.Width + 7 );
			
			FastColour col = Active ? new FastColour( 240, 240, 240 ) : new FastColour( 180, 180, 180 );
			drawer.Clear( col, X + 1, Y, Width - 2, 2 );
			drawer.Clear( col, X + 1, Y + Height - 2, Width - 2, 2 );
			drawer.Clear( col, X, Y + 1, 2, Height - 2 );
			drawer.Clear( col, X + Width - 2, Y + 1, 2, Height - 2 );
			drawer.Clear( FastColour.Black, X + 2, Y + 2, Width - 4, Height - 4 );
			
			args.SkipPartsCheck = true;			
			if( Text.Length != 0 || HintText == null ) {
				int y = Y + 2 + (Height - size.Height) / 2;
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
			if( c >= ' ' && c <= '~' && Text.Length < MaxTextLength ) {
				Text += c;
				if( TextChanged != null ) TextChanged( this );
				return true;
			}
			return false;
		}
		
		/// <summary> Removes the last character in the currently entered text. </summary>
		/// <returns> true if a redraw is necessary, false otherwise. </returns>
		public bool RemoveChar() {
			if( Text.Length == 0 ) return false;
			
			Text = Text.Substring( 0, Text.Length - 1 );
			if( TextChanged != null ) TextChanged( this );
			return true;
		}
		
		/// <summary> Resets the currently entered text to an empty string </summary>
		/// <returns> true if a redraw is necessary, false otherwise. </returns>
		public bool ClearText() {
			if( Text.Length == 0 ) return false;
			
			Text = "";
			if( TextChanged != null ) TextChanged( this );
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
	}
}
