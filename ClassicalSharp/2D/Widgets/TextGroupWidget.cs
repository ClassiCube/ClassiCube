using System;
using System.Collections.Generic;
using System.Drawing;

namespace ClassicalSharp {
	
	public sealed class TextGroupWidget : Widget {
		
		public TextGroupWidget( Game window, int elementsCount, Font font ) : base( window ) {
			ElementsCount = elementsCount;
			this.font = font;
		}
		
		Texture[] textures;
		int ElementsCount, defaultHeight;
		public int XOffset = 0, YOffset = 0;
		readonly Font font;
		
		public override void Init() {
			textures = new Texture[ElementsCount];
			defaultHeight = Utils2D.MeasureSize( "I", font, true ).Height;
			for( int i = 0; i < textures.Length; i++ ) {
				textures[i].Height = defaultHeight;
			}
			UpdateDimensions();
		}
		
		public void SetText( int index, string text ) {
			graphicsApi.DeleteTexture( ref textures[index] );
			
			if( !String.IsNullOrEmpty( text ) ) {
				DrawTextArgs args = new DrawTextArgs( graphicsApi, text, true );
				Texture tex = Utils2D.MakeTextTexture( font, 0, 0, ref args );
				tex.X1 = CalcOffset( game.Width, tex.Width, XOffset, HorizontalDocking );
				tex.Y1 = CalcY( index, tex.Height );
				textures[index] = tex;
			} else {
				textures[index] = new Texture( -1, 0, 0, 0, defaultHeight, 0, 0 );
			}
			UpdateDimensions();
		}
		
		public void PushUpAndReplaceLast( string text ) {
			int y = Y;
			graphicsApi.DeleteTexture( ref textures[0] );
			for( int i = 0; i < textures.Length - 1; i++ ) {
				textures[i] = textures[i + 1];
				textures[i].Y1 = y;
				y += textures[i].Height;
			}
			textures[textures.Length - 1].ID = 0; // Delete() is called by SetText otherwise.
			SetText( textures.Length - 1, text );
		}
		
		int CalcY( int index, int newHeight ) {
			int y = 0;
			int deltaY = newHeight - textures[index].Height;
			
			if( VerticalDocking == Docking.LeftOrTop ) {
				y = Y;
				for( int i = 0; i < index; i++ ) {
					y += textures[i].Height;
				}
				for( int i = index + 1; i < textures.Length; i++ ) {
					textures[i].Y1 += deltaY;
				}
			} else {
				y = game.Height - YOffset;
				for( int i = index + 1; i < textures.Length; i++ ) {
					y -= textures[i].Height;
				}
				y -= newHeight;
				for( int i = 0; i < index; i++ ) {
					textures[i].Y1 -= deltaY;
				}				
			}
			return y;
		}
		
		void UpdateDimensions() {
			Height = 0;
			for( int i = 0; i < textures.Length; i++ ) {
				Height += textures[i].Height;
			}
			Y = CalcOffset( game.Height, Height, YOffset, VerticalDocking );
			
			Width = 0;
			for( int i = 0; i < textures.Length; i++ ) {
				int width = textures[i].Width;
				if( width > Width ) {
					Width = width;
				}
			}
			X = CalcOffset( game.Width, Width, XOffset, HorizontalDocking );
		}
		
		public override void Render( double delta ) {
			for( int i = 0; i < textures.Length; i++ ) {
				Texture texture = textures[i];
				if( texture.IsValid ) {
					texture.Render( graphicsApi );
				}
			}
		}
		
		public override void Dispose() {
			for( int i = 0; i < textures.Length; i++ ) {
				graphicsApi.DeleteTexture( ref textures[i] );
			}
		}
		
		public override void MoveTo( int newX, int newY ) {
			int deltaX = newX - X;
			int deltaY = newY - Y;
			for( int i = 0; i < textures.Length; i++ ) {
				textures[i].X1 += deltaX;
				textures[i].Y1 += deltaY;
			}
			X = newX;
			Y = newY;
		}
	}
}