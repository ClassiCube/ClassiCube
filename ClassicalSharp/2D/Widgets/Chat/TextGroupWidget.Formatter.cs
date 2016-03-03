using System;
using System.Collections.Generic;
using System.Drawing;

namespace ClassicalSharp {
	
	public sealed partial class TextGroupWidget : Widget {
		
		public void SetText( int index, string text ) {
			graphicsApi.DeleteTexture( ref Textures[index] );
			DrawTextArgs args = new DrawTextArgs( text, font, true );
			linkData[index] = default(LinkData);
			LinkFlags prevFlags = index > 0 ? linkData[index - 1].flags : 0;
			
			if( !String.IsNullOrEmpty( text ) ) {
				Texture tex = NextToken( text, 0, ref prevFlags ) == -1 ? DrawSimple( ref args ) :
					DrawAdvanced( ref args, index, text );
				tex.X1 = CalcOffset( game.Width, tex.Width, XOffset, HorizontalAnchor );
				tex.Y1 = CalcY( index, tex.Height );
				Textures[index] = tex;
				lines[index] = text;
			} else {
				int height = PlaceholderHeight[index] ? defaultHeight : 0;
				int y = CalcY( index, height );
				Textures[index] = new Texture( -1, 0, y, 0, height, 0, 0 );
				lines[index] = null;
			}
			UpdateDimensions();
		}
		
		Texture DrawSimple( ref DrawTextArgs args ) {
			return game.Drawer2D.MakeChatTextTexture( ref args, 0, 0 );
		}
		
		unsafe Texture DrawAdvanced( ref DrawTextArgs args, int index, string text ) {
			LinkData data = Split( index, text );
			Size total = Size.Empty;
			Size* partSizes = stackalloc Size[data.parts.Length];
			linkData[index] = data;
			
			for( int i = 0; i < data.parts.Length; i++ ) {
				args.Text = data.parts[i];
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
				
				for( int i = 0; i < data.parts.Length; i++ ) {
					args.Text = data.parts[i];
					args.Font = (i & 1) == 0 ? font : underlineFont;
					Size size = partSizes[i];
					
					drawer.DrawChatText( ref args, x, 0 );
					data.bounds[i].X = x;
					data.bounds[i].Width = size.Width;
					x += size.Width;
				}
				return drawer.Make2DTexture( bmp, total, 0, 0 );
			}
		}
		
		LinkData Split( int index, string line ) {
			int start = 0, lastEnd = 0, count = 0;
			LinkData data = default(LinkData);
			data.parts = new string[GetTokensCount( index, line )];
			data.urls = new string[data.parts.Length];
			data.bounds = new Rectangle[data.parts.Length];
			LinkFlags prevFlags = index > 0 ? linkData[index - 1].flags : 0;
			
			while( (start = NextToken( line, start, ref prevFlags )) >= 0 ) {
				int nextEnd = line.IndexOf( ' ', start );
				if( nextEnd == -1 ) {
					nextEnd = line.Length;
					data.flags |= LinkFlags.Continue;
				}
				
				data.AddPart( count, GetPart( line, lastEnd, start ) );     // word bit
				data.AddPart( count + 1, GetPart( line, start, nextEnd ) ); // url bit
				count += 2;
				
				if( (prevFlags & LinkFlags.Append) != 0 ) {
					string url = linkData[index - 1].LastUrl + data.urls[count - 1];
					data.urls[count - 1] =  url;
					data.parts[count - 2] = "";
					UpdatePreviousUrls( index - 1, url );
				}
				
				if( (prevFlags & LinkFlags.NewLink) != 0 )
					data.flags |= LinkFlags.NewLink;
				start = nextEnd;
				lastEnd = nextEnd;
			}
			
			if( lastEnd < line.Length )
				data.AddPart( count, GetPart( line, lastEnd, line.Length ) ); // word bit
			return data;
		}
		
		void UpdatePreviousUrls( int i, string url ) {			
			while( i >= 0 && linkData[i].urls != null && (linkData[i].flags & LinkFlags.Continue) != 0 ) {
				linkData[i].LastUrl = url;
				if( linkData[i].urls.Length > 2 || (linkData[i].flags & LinkFlags.NewLink) != 0 ) 
					break;
				i--;
			}
		}
		
		string GetPart( string line, int start, int end ) {
			string part = line.Substring( start, end - start );
			int colIndex = line.LastIndexOf( '&', start, start );
			// We may split up a line into say %e<word><url>
			// url and word both need to have %e at the start.
			
			if( colIndex >= 0 && colIndex < line.Length - 1 ) {
				if( game.Drawer2D.ValidColour( line[colIndex + 1] ) )
					part = "&" + line[colIndex + 1] + part;
			}
			return part;
		}
		
		int NextToken( string line, int start, ref LinkFlags prevFlags ) {
			bool isWrapped = start == 0 && line.StartsWith( "> " );
			if( (prevFlags & LinkFlags.Continue) != 0 && isWrapped ) {
				prevFlags = 0;
				if( !Utils.IsUrlPrefix( Utils.StripColours( line ), 2 ) )
					prevFlags |= LinkFlags.Append;
				else
					prevFlags |= LinkFlags.NewLink;
				return 2;
			}
			
			prevFlags = 0;		
			int nextHttp = line.IndexOf( "http://", start );
			int nextHttps = line.IndexOf( "https://", start );
			return nextHttp == -1 ? nextHttps : nextHttp;
		}
		
		int GetTokensCount( int index, string line ) {
			int start = 0, lastEnd = 0, count = 0;
			LinkFlags prevFlags = index > 0 ? linkData[index - 1].flags : 0;
			
			while( (start = NextToken( line, start, ref prevFlags )) >= 0 ) {
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
		
		struct LinkData {
			public Rectangle[] bounds;
			public string[] parts, urls;
			public LinkFlags flags;
			
			public void AddPart( int index, string part ) {
				parts[index] = part;
				urls[index] = part;
			}
			
			public string LastUrl { 
				get { return urls[parts.Length - 1]; }
				set { urls[parts.Length - 1] = value; }
			}
		}
		
		[Flags]
		enum LinkFlags : byte {
			Continue = 2, // "part1" "> part2" type urls
			Append = 4, // used for internally combining "part2" and "part2"
			NewLink = 8, // used to signify that part2 is a separate url from part1
		}
	}
}