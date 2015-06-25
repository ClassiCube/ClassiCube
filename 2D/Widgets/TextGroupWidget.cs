using System;
using System.Collections.Generic;
using System.Drawing;

namespace ClassicalSharp {
	
	public sealed class TextGroupWidget : Widget {
		
		public TextGroupWidget( Game window, int elementsCount, Font font ) : base( window ) {
			ElementsCount = elementsCount;
			this.font = font;
		}
		
		Texture2D[] textures;
		string[] textCache;
		int ElementsCount, defaultHeight;
		public int XOffset = 0, YOffset = 0;
		readonly Font font;
		
		public override void Init() {
			textures = new Texture2D[ElementsCount];
			textCache = new string[ElementsCount];
			defaultHeight = Utils2D.MeasureSize( "I", font, true ).Height;
			for( int i = 0; i < textures.Length; i++ ) {
				textures[i].Height = defaultHeight;
			}
			UpdateDimensions();
		}
		
		public void SetText( int index, string text ) {
			GraphicsApi.DeleteTexture( ref textures[index] );
			List<DrawTextArgs> parts = null;
			Size size = new Size( 0, defaultHeight );
			if( !String.IsNullOrEmpty( text ) ) {
				parts = Utils2D.SplitText( GraphicsApi, text, true );
				size = Utils2D.MeasureSize( parts, font, true );
			}
			
			int x = HorizontalDocking == Docking.LeftOrTop ? XOffset : Window.Width - size.Width - XOffset;
			int y = CalcY( index, size.Height );
			if( !String.IsNullOrEmpty( text ) ) {
				textures[index] = Utils2D.MakeTextTexture( parts, font, size, x, y );
			} else {
				textures[index] = new Texture2D( -1, 0, 0, 0, defaultHeight, 0, 0 );
			}
			textCache[index] = text;
			UpdateDimensions();
		}
		
		public void SetText( int startIndex, params string[] texts ) {
			for( int i = 0; i < texts.Length; i++ ) {
				SetText( startIndex + i, texts[i] );
			}
		}
		
		public string GetText( int index ) {
			return textCache[index];
		}
		
		public int CalcUsedY() {
			int y = Y;
			for( int i = 0; i < textures.Length; i++ ) {
				if( textures[i].IsValid ) break;
				y += defaultHeight;
			}
			return y;
		}
		
		public void PushUpAndReplaceLast( string text ) {
			int y = Y;
			GraphicsApi.DeleteTexture( ref textures[0] );
			for( int i = 0; i < textures.Length - 1; i++ ) {
				textures[i] = textures[i + 1];
				textures[i].Y1 = y;
				textCache[i] = textCache[i + 1];
				y += textures[i].Height;
			}
			textures[textures.Length - 1].ID = 0; // Delete() is called by SetText otherwise.
			SetText( textures.Length - 1, text );
		}
		
		int CalcY( int index, int newHeight ) {
			if( VerticalDocking == Docking.LeftOrTop ) {
				int y = Y;
				for( int i = 0; i < index; i++ ) {
					y += textures[i].Height;
				}
				int deltaY = newHeight - textures[index].Height;
				for( int i = index + 1; i < textures.Length; i++ ) {
					textures[i].Y1 += deltaY;
				}
				return y;
			} else {
				int y = Window.Height - YOffset;
				for( int i = index + 1; i < textures.Length; i++ ) {
					y -= textures[i].Height;
				}
				y -= newHeight;
				int deltaY = newHeight - textures[index].Height;
				for( int i = 0; i < index; i++ ) {
					textures[i].Y1 -= deltaY;
				}
				return y;
			}
		}
		
		void UpdateDimensions() {
			Height = 0;
			for( int i = 0; i < textures.Length; i++ ) {
				Height += textures[i].Height;
			}
			Y = VerticalDocking == Docking.LeftOrTop ? YOffset : Window.Height - Height - YOffset;
			
			Width = 0;
			for( int i = 0; i < textures.Length; i++ ) {
				int width = textures[i].Width;
				if( width > Width ) {
					Width = width;
				}
			}
			X = HorizontalDocking == Docking.LeftOrTop ? XOffset : Window.Width - Width - XOffset;
		}
		
		public override void Render( double delta ) {
			for( int i = 0; i < textures.Length; i++ ) {
				Texture2D texture = textures[i];
				if( texture.IsValid ) {
					texture.Render( GraphicsApi );
				}
			}
		}
		
		public override void Dispose() {
			for( int i = 0; i < textures.Length; i++ ) {
				GraphicsApi.DeleteTexture( ref textures[i] );
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