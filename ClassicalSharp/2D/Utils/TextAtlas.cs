// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {
	public sealed class TextAtlas : IDisposable {
		
		public Texture tex;
		Game game;
		int[] widths;
		internal int offset, curX, totalWidth;
		
		public TextAtlas( Game game ) {
			this.game = game;
		}
		
		public void Pack( string chars, Font font, string prefix ) {
			DrawTextArgs args = new DrawTextArgs( "", font, true );
			widths = new int[chars.Length];
			
			using( IDrawer2D drawer = game.Drawer2D ) {
				args.Text = prefix;
				Size size = game.Drawer2D.MeasureChatSize( ref args );
				offset = size.Width;
				size.Width += 16 * chars.Length;
				
				using( Bitmap bmp = IDrawer2D.CreatePow2Bitmap( size ) ) {
					drawer.SetBitmap( bmp );
					drawer.DrawChatText( ref args, 0, 0 );
					
					for( int i = 0; i < chars.Length; i++ ) {
						args.Text = new String( chars[i], 1 );
						widths[i] = game.Drawer2D.MeasureChatSize( ref args ).Width;
						drawer.DrawChatText( ref args, offset + 16 * i, 0 );
					}
					
					tex = drawer.Make2DTexture( bmp, size, 0, 0 );
					drawer.ReducePadding( ref tex, Utils.Floor( font.Size ) );
					
					tex.U2 = (float)offset / bmp.Width;
					tex.Width = (short)offset;
					totalWidth = bmp.Width;
				}
			}
		}
		
		public void Dispose() { game.Graphics.DeleteTexture( ref tex ); }
		
		
		public void Add( int charIndex, VertexP3fT2fC4b[] vertices, ref int index ) {
			int width = widths[charIndex];			
			Texture part = tex;
			part.X1 = curX; part.Width = (short)width;
			part.U1 = (offset + charIndex * 16) / (float)totalWidth;
			part.U2 = part.U1 + width / (float)totalWidth;
			
			curX += width;
			IGraphicsApi.Make2DQuad( ref part, FastColour.White, vertices, ref index );
		}
		
		public void AddInt( int value, VertexP3fT2fC4b[] vertices, ref int index ) {
			if( value < 0 ) Add( 10, vertices, ref index); // - sign
			
			int count = 0;
			value = Reverse( Math.Abs( value ), out count );
			for( int i = 0; i < count; i++ ) {
				Add( value % 10, vertices, ref index ); value /= 10;
			}
		}
		
		static int Reverse( int value, out int count ) {
			int orig = value, reversed = 0;
			count = 1; value /= 10;
			while( value > 0 ) {
				count++; value /= 10;
			}
			
			for( int i = 0; i < count; i++ ) {
				reversed *= 10;
				reversed += orig % 10;
				orig /= 10;
			}
			return reversed;
		}
	}
}
