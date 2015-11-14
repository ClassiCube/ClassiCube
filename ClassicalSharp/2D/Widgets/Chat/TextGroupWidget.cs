using System;
using System.Collections.Generic;
using System.Drawing;

namespace ClassicalSharp {
	
	public sealed class TextGroupWidget : Widget {
		
		public TextGroupWidget( Game game, int elementsCount, Font font ) : base( game ) {
			ElementsCount = elementsCount;
			this.font = font;
		}
		
		public Texture[] Textures;
		string[] lines;
		int ElementsCount, defaultHeight;
		public int XOffset = 0, YOffset = 0;
		readonly Font font;
		
		public override void Init() {
			Textures = new Texture[ElementsCount];
			lines = new string[ElementsCount];
			DrawTextArgs args = new DrawTextArgs( "I", font, true );
			defaultHeight = game.Drawer2D.MeasureChatSize( ref args ).Height;
			
			for( int i = 0; i < Textures.Length; i++ )
				Textures[i].Height = defaultHeight;
			UpdateDimensions();
		}
		
		public void SetText( int index, string text ) {
			graphicsApi.DeleteTexture( ref Textures[index] );
			DrawTextArgs args = new DrawTextArgs( text, font, true );
			
			if( !String.IsNullOrEmpty( text ) ) {
				Texture tex = game.Drawer2D.UseBitmappedChat ?
					game.Drawer2D.MakeBitmappedTextTexture( ref args, 0, 0 ) :
					game.Drawer2D.MakeTextTexture( ref args, 0, 0 );
				
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
		
		public void PushUpAndReplaceLast( string text ) {
			int y = Y;
			graphicsApi.DeleteTexture( ref Textures[0] );
			for( int i = 0; i < Textures.Length - 1; i++ ) {
				Textures[i] = Textures[i + 1];
				lines[i] = lines[i + 1];
				Textures[i].Y1 = y;
				y += Textures[i].Height;
			}
			Textures[Textures.Length - 1].ID = 0; // Delete() is called by SetText otherwise.
			SetText( Textures.Length - 1, text );
		}
		
		int CalcY( int index, int newHeight ) {
			int y = 0;
			int deltaY = newHeight - Textures[index].Height;
			
			if( VerticalAnchor == Anchor.LeftOrTop ) {
				y = Y;
				for( int i = 0; i < index; i++ ) {
					y += Textures[i].Height;
				}
				for( int i = index + 1; i < Textures.Length; i++ ) {
					Textures[i].Y1 += deltaY;
				}
			} else {
				y = game.Height - YOffset;
				for( int i = index + 1; i < Textures.Length; i++ ) {
					y -= Textures[i].Height;
				}
				y -= newHeight;
				for( int i = 0; i < index; i++ ) {
					Textures[i].Y1 -= deltaY;
				}				
			}
			return y;
		}
		
		public int GetUsedHeight() {
			int sum = 0;
			for( int i = 0; i < Textures.Length; i++ ) {
				if( Textures[i].IsValid )
					sum += Textures[i].Height;
			}
			return sum;
		}
		
		void UpdateDimensions() {
			Width = 0; 
			Height = 0;
			for( int i = 0; i < Textures.Length; i++ ) {
				Width = Math.Max( Width, Textures[i].Width );
				Height += Textures[i].Height;				
			}
			
			X = CalcOffset( game.Width, Width, XOffset, HorizontalAnchor );
			Y = CalcOffset( game.Height, Height, YOffset, VerticalAnchor );
		}
		
		public override void Render( double delta ) {
			for( int i = 0; i < Textures.Length; i++ ) {
				Texture texture = Textures[i];
				if( texture.IsValid )
					texture.Render( graphicsApi );
			}
		}
		
		public override void Dispose() {
			for( int i = 0; i < Textures.Length; i++ )
				graphicsApi.DeleteTexture( ref Textures[i] );
		}
		
		public override void MoveTo( int newX, int newY ) {
			int diffX = newX - X, diffY = newY - Y;
			for( int i = 0; i < Textures.Length; i++ ) {
				Textures[i].X1 += diffX;
				Textures[i].Y1 += diffY;
			}
			X = newX; Y = newY;
		}
		
		public string GetSelected( int mouseX, int mouseY ) {
			for( int i = 0; i < Textures.Length; i++ ) {
				if( Textures[i].IsValid && 
				   Textures[i].Bounds.Contains( mouseX, mouseY ) )
					return lines[i];
			}
			return null;
		}
	}
}