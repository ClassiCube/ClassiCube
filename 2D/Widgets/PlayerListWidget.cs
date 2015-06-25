using System;
using System.Collections.Generic;
using System.Drawing;

namespace ClassicalSharp {
	
	public abstract class PlayerListWidget : Widget {
		
		protected readonly Font font;
		public PlayerListWidget( Game window, Font font ) : base( window ) {
			HorizontalDocking = Docking.Centre;
			VerticalDocking = Docking.Centre;
			this.font = font;
		}
		
		protected const int boundsSize = 10;
		protected const int namesPerColumn = 20;
		protected int namesCount = 0;
		protected Texture2D[] textures = new Texture2D[256];
		protected int columns;
		protected int xMin, xMax, yHeight;
		protected static FastColour tableCol = new FastColour( 100, 100, 100, 80 );
		
		public override void Init() {
			CreateInitialPlayerInfo();
			columns = (int)Math.Ceiling( (double)namesCount / namesPerColumn );
			SortPlayerInfo();
		}
		
		public override void Render( double delta ) {
			GraphicsApi.Draw2DQuad( X, Y, Width, Height, tableCol );
			for( int i = 0; i < namesCount; i++ ) {
				Texture2D texture = textures[i];
				if( texture.IsValid ) {
					texture.Render( GraphicsApi );
				}
			}
		}
		
		public override void Dispose() {
			for( int i = 0; i < namesCount; i++ ) {
				textures[i].Delete();
			}
		}
		
		protected void UpdateTableDimensions() {
			int width = xMax - xMin;
			X = xMin - boundsSize;
			Y = Window.Height / 2 - yHeight / 2 - boundsSize;
			Width = width + boundsSize * 2;
			Height = yHeight + boundsSize * 2;
		}
		
		protected void CalcMaxColumnHeight() {
			yHeight = 0;
			for( int col = 0; col < columns; col++ ) {
				yHeight = Math.Max( GetColumnHeight( col ), yHeight );
			}
		}
		
		protected int GetColumnWidth( int column ) {
			int i = column * namesPerColumn;
			int maxWidth = 0;
			int maxIndex = Math.Min( namesCount, i + namesPerColumn );
			
			for( ; i < maxIndex; i++ ) {
				maxWidth = Math.Max( maxWidth, textures[i].Width );
			}
			return maxWidth;
		}
		
		protected int GetColumnHeight( int column ) {
			int i = column * namesPerColumn;
			int total = 0;
			int maxIndex = Math.Min( namesCount, i + namesPerColumn );
			
			for( ; i < maxIndex; i++ ) {
				total += textures[i].Height;
			}
			return total;
		}
		
		protected void SetColumnPos( int column, int x, int y ) {
			int i = column * namesPerColumn;
			int maxIndex = Math.Min( namesCount, i + namesPerColumn );
			
			for( ; i < maxIndex; i++ ) {
				Texture2D tex = textures[i];
				tex.X1 = x;
				tex.Y1 = y;
				y += tex.Height;
				textures[i] = tex;
			}
		}
		
		public override void MoveTo( int newX, int newY ) {
			int deltaX = newX - X;
			int deltaY = newY - Y;
			for( int i = 0; i < namesCount; i++ ) {
				Texture2D tex = textures[i];
				tex.X1 += deltaX;
				tex.Y1 += deltaY;
				textures[i] = tex;
			}
			X = newX;
			Y = newY;
		}
		
		protected abstract void CreateInitialPlayerInfo();
		
		protected abstract void SortInfoList();
		
		protected void RemoveItemAt<T>( T[] array, int index ) {
			for( int i = index; i < namesCount - 1; i++ ) {
				array[i] = array[i + 1];
			}
			array[namesCount - 1] = default( T );
		}
		
		protected void SortPlayerInfo() {
			SortInfoList();
			int centreX = Window.Width / 2;
			CalcMaxColumnHeight();
			int y = Window.Height / 2 - yHeight / 2;
			
			int midCol = columns / 2;
			int offset = 0;
			if( columns % 2 != 0 ) {
				// For an odd number of columns, the middle column is centred.
				offset = ( GetColumnWidth( midCol ) + 1 ) / 2; // ceiling divide by 2
			}
			
			int x = centreX - offset;			
			for( int col = midCol - 1; col >= 0; col-- ) {
				x -= GetColumnWidth( col );
				SetColumnPos( col, x, y );
			}
			xMin = x;
			
			x = centreX - offset;
			for( int col = midCol; col < columns; col++ ) {
				SetColumnPos( col, x, y );
				x += GetColumnWidth( col );
			}
			xMax = x;
			UpdateTableDimensions();
		}
	}
}