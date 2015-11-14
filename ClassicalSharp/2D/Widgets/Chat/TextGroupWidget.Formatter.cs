using System;
using System.Collections.Generic;
using System.Drawing;

namespace ClassicalSharp {
	
	public sealed partial class TextGroupWidget : Widget {
		
		public void SetText( int index, string text ) {
			graphicsApi.DeleteTexture( ref Textures[index] );
			DrawTextArgs args = new DrawTextArgs( text, font, true );
			Font underlineFont = new Font( font, FontStyle.Underline );
			urlBounds[index] = null;
			
			if( !String.IsNullOrEmpty( text ) ) {
				Texture tex = NextToken( text, 0 ) == -1 ? DrawSimple( ref args ) :
					DrawAdvanced( ref args, index, text );
				tex.X1 = CalcOffset( game.Width, tex.Width, XOffset, HorizontalAnchor );
				tex.Y1 = CalcY( index, tex.Height );
				Textures[index] = tex;
				lines[index] = text;
			} else {
				Textures[index] = new Texture( -1, 0, 0, 0, defaultHeight, 0, 0 );
				lines[index] = null;
			}
			UpdateDimensions();
		}
		
		Texture DrawSimple( ref DrawTextArgs args ) {
			return game.Drawer2D.MakeChatTextTexture( ref args, 0, 0 );
		}
		
		Texture DrawAdvanced( ref DrawTextArgs args, int index, string text ) {
			string[] items = Split( index, text );
			Size total = Size.Empty;
			Size[] partSizes = new Size[items.Length];
			
			for( int i = 0; i < items.Length; i++ ) {
				args.Text = items[i];
				args.Font = (i & 1) == 0 ? font : underlineFont;
				partSizes[i] = game.Drawer2D.MeasureChatSize( ref args );
				total.Height = Math.Max( partSizes[i].Height, total.Height );
				total.Width += partSizes[i].Width;
			}
			
			using( IDrawer2D drawer = game.Drawer2D )
				using( Bitmap bmp = IDrawer2D.CreatePow2Bitmap( total ) )
			{
				drawer.SetBitmap( bmp );
				int x = 0;
				
				for( int i = 0; i < items.Length; i++ ) {
					args.Text = items[i];
					args.Font = (i & 1) == 0 ? font : underlineFont;
					Size size = partSizes[i];
					
					drawer.DrawChatText( ref args, x, 0 );			
					urlBounds[index][i].X = x;
					urlBounds[index][i].Width = size.Width;
					x += size.Width;
				}
				return drawer.Make2DTexture( bmp, total, 0, 0 );
			}
		}
		
		string[] Split( int index, string line ) {
			int start = 0, lastEnd = 0, count = 0;
			string[] items = new string[GetTokensCount( line )];
			Rectangle[] parts = new Rectangle[items.Length];
			
			while( (start = NextToken( line, start )) >= 0 ) {
				int nextEnd = line.IndexOf( ' ', start );
				if( nextEnd == -1 )
					nextEnd = line.Length;
				
				parts[count].Y = lastEnd << 12 | (start - lastEnd);
				items[count++] = line.Substring( lastEnd, start - lastEnd ); // word bit
				parts[count].Y = start << 12 | (nextEnd - start);
				items[count++] = line.Substring( start, nextEnd - start ); // url bit
				start = nextEnd;
				lastEnd = nextEnd;
			}
			
			if( lastEnd < line.Length ) {
				parts[count].Y = lastEnd << 12 | (line.Length - lastEnd);
				items[count++] = line.Substring( lastEnd, line.Length - lastEnd );
			}
			urlBounds[index] = parts;
			return items;
		}
		
		int NextToken( string line, int start ) {
			int nextHttp = line.IndexOf( "http://", start );
			int nextHttps = line.IndexOf( "https://", start );
			return nextHttp == -1 ? nextHttps : nextHttp;
		}
		
		int GetTokensCount( string line ) {
			int start = 0, lastEnd = 0, count = 0;
			while( (start = NextToken( line, start )) >= 0 ) {
				int nextEnd = line.IndexOf( ' ', start );
				if( nextEnd == -1 )
					nextEnd = line.Length;
				
				start = nextEnd;
				lastEnd = nextEnd;
				count += 2;
			}
			
			if( lastEnd < line.Length )
				count++;
			return count;
		}
	}
}