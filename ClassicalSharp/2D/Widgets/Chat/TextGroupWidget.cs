using System;
using System.Collections.Generic;
using System.Drawing;

namespace ClassicalSharp {
	
	public sealed partial class TextGroupWidget : Widget {
		
		public TextGroupWidget( Game game, int elementsCount, Font font, 
		                       Font underlineFont, Anchor horAnchor, Anchor verAnchor ) : base( game ) {
			ElementsCount = elementsCount;
			this.font = font;
			this.underlineFont = underlineFont;
			HorizontalAnchor = horAnchor;
			VerticalAnchor = verAnchor;
		}
		
		public Texture[] Textures;
		public bool[] PlaceholderHeight;
		string[] lines;
		LinkData[] linkData;
		int ElementsCount, defaultHeight;
		public int XOffset = 0, YOffset = 0;
		readonly Font font, underlineFont;
		
		public override void Init() {
			Textures = new Texture[ElementsCount];
			PlaceholderHeight = new bool[ElementsCount];
			lines = new string[ElementsCount];
			linkData = new LinkData[ElementsCount];
			DrawTextArgs args = new DrawTextArgs( "I", font, true );
			defaultHeight = game.Drawer2D.MeasureChatSize( ref args ).Height;
			
			for( int i = 0; i < Textures.Length; i++ ) {
				Textures[i].Height = defaultHeight;
				PlaceholderHeight[i] = true;
			}
			UpdateDimensions();
		}
		
		public void SetUsePlaceHolder( int index, bool placeHolder ) {
			PlaceholderHeight[index] = placeHolder;
			if( Textures[index].ID > 0 ) return;
			
			int newHeight = placeHolder ? defaultHeight : 0;
			Textures[index].Y1 = CalcY( index, newHeight );
			Textures[index].Height = newHeight;
		}
		
		public void PushUpAndReplaceLast( string text ) {
			int y = Y;
			graphicsApi.DeleteTexture( ref Textures[0] );
			for( int i = 0; i < Textures.Length - 1; i++ ) {
				Textures[i] = Textures[i + 1];
				lines[i] = lines[i + 1];
				Textures[i].Y1 = y;
				y += Textures[i].Height;
				linkData[i] = linkData[i + 1];
			}
			
			linkData[Textures.Length - 1] = default(LinkData);
			Textures[Textures.Length - 1].ID = 0; // Delete() is called by SetText otherwise.
			SetText( Textures.Length - 1, text );
		}
		
		int CalcY( int index, int newHeight ) {
			int y = 0;
			int deltaY = newHeight - Textures[index].Height;
			
			if( VerticalAnchor == Anchor.LeftOrTop ) {
				y = Y;
				for( int i = 0; i < index; i++ )
					y += Textures[i].Height;
				for( int i = index + 1; i < Textures.Length; i++ )
					Textures[i].Y1 += deltaY;
			} else {
				y = game.Height - YOffset;
				for( int i = index + 1; i < Textures.Length; i++ )
					y -= Textures[i].Height;
				
				y -= newHeight;
				for( int i = 0; i < index; i++ )
					Textures[i].Y1 -= deltaY;
			}
			return y;
		}
		
		public int GetUsedHeight() {
			int sum = 0, max = Textures.Length;
			for( int i = 0; i < Textures.Length; i++ ) {
				if( Textures[i].IsValid ) {
					max = i; break;
				}
			}
			
			for( int i = max; i < Textures.Length; i++ )
				sum += Textures[i].Height;
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
			int dx = newX - X, dy = newY - Y;
			for( int i = 0; i < Textures.Length; i++ ) {
				Textures[i].X1 += dx;
				Textures[i].Y1 += dy;
			}
			X = newX; Y = newY;
		}
		
		public string GetSelected( int mouseX, int mouseY ) {
			for( int i = 0; i < Textures.Length; i++ ) {
				Texture tex = Textures[i];
				if( tex.IsValid && tex.Bounds.Contains( mouseX, mouseY ) )
					return GetUrl( i, mouseX ) ?? lines[i];					
			}
			return null;
		}
		
		string GetUrl( int index, int mouseX ) {
			Rectangle[] partBounds = linkData[index].bounds;
			if( partBounds == null ) return null;
			Texture tex = Textures[index];	
			mouseX -= tex.X1;
			
			for( int i = 1; i < partBounds.Length; i += 2 ) {
				if( mouseX >= partBounds[i].Left && mouseX < partBounds[i].Right )
					return linkData[index].urls[i];
			}
			return null;
		}
	}
}