// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;

namespace ClassicalSharp.Gui {
	
	public abstract class PlayerListWidget : Widget {
		
		protected readonly Font font;
		public PlayerListWidget( Game game, Font font ) : base( game ) {
			HorizontalAnchor = Anchor.Centre;
			VerticalAnchor = Anchor.Centre;
			this.font = font;
		}
		
		protected int columnPadding = 5;
		protected int elementOffset = 0;
		
		protected const int boundsSize = 10;
		protected const int namesPerColumn = 20;
		protected int namesCount = 0;
		protected Texture[] textures;
		protected int columns;
		protected int xMin, xMax, yHeight;
		protected static FastColour tableCol = new FastColour( 50, 50, 50, 205 );
		
		public override void Init() {
			CreateInitialPlayerInfo();
			SortPlayerInfo();
		}
		
		public abstract string GetNameUnder( int mouseX, int mouseY );
		
		public override void Render( double delta ) {
			api.Texturing = false;
			api.Draw2DQuad( X, Y, Width, Height, tableCol );
			api.Texturing = true;
			for( int i = 0; i < namesCount; i++ ) {
				Texture texture = textures[i];
				if( texture.IsValid )
					texture.Render( api );
			}
		}
		
		public override void Dispose() {
			for( int i = 0; i < namesCount; i++ ) {
				Texture tex = textures[i];
				api.DeleteTexture( ref tex );
				textures[i] = tex;
			}
		}
		
		protected void UpdateTableDimensions() {
			int width = xMax - xMin;
			X = xMin - boundsSize;
			Y = game.Height / 2 - yHeight / 2 - boundsSize;
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
			
			for( ; i < maxIndex; i++ )
				maxWidth = Math.Max( maxWidth, textures[i].Width );
			return maxWidth + columnPadding + elementOffset;
		}
		
		protected int GetColumnHeight( int column ) {
			int i = column * namesPerColumn;
			int total = 0;
			int maxIndex = Math.Min( namesCount, i + namesPerColumn );
			
			for( ; i < maxIndex; i++ )
				total += textures[i].Height + 1;
			return total;
		}
		
		protected void SetColumnPos( int column, int x, int y ) {
			int i = column * namesPerColumn;
			int maxIndex = Math.Min( namesCount, i + namesPerColumn );
			
			for( ; i < maxIndex; i++ ) {
				Texture tex = textures[i];
				tex.X1 = x; tex.Y1 = y;
				
				y += tex.Height + 1;
				if( ShouldOffset( i ) ) 
					tex.X1 += elementOffset;
				textures[i] = tex;
			}
		}
		
		protected virtual bool ShouldOffset( int i ) { return true; }
		
		public override void MoveTo( int newX, int newY ) {
			int diffX = newX - X; int diffY = newY - Y;
			for( int i = 0; i < namesCount; i++ ) {
				textures[i].X1 += diffX;
				textures[i].Y1 += diffY;
			}
			X = newX; Y = newY;
		}
		
		protected abstract void CreateInitialPlayerInfo();
		
		protected abstract void SortInfoList();
		
		protected void RemoveInfoAt<T>( T[] info, int i ) {
			Texture tex = textures[i];
			api.DeleteTexture( ref tex );
			RemoveItemAt( info, i );
			RemoveItemAt( textures, i );
			namesCount--;
			SortPlayerInfo();
		}
		
		protected void RemoveItemAt<T>( T[] array, int index ) {
			for( int i = index; i < namesCount - 1; i++ ) {
				array[i] = array[i + 1];
			}
			array[namesCount - 1] = default( T );
		}
		
		protected void SortPlayerInfo() {
			columns = Utils.CeilDiv( namesCount, namesPerColumn );
			SortInfoList();
			columns = Utils.CeilDiv( namesCount, namesPerColumn );
			CalcMaxColumnHeight();
			int y = game.Height / 2 - yHeight / 2;
			int midCol = columns / 2;
			
			int centreX = game.Width / 2;
			int offset = 0;
			if( columns % 2 != 0 ) {
				// For an odd number of columns, the middle column is centred.
				offset = Utils.CeilDiv( GetColumnWidth( midCol ), 2 );
			}
			
			xMin = centreX - offset;
			for( int col = midCol - 1; col >= 0; col-- ) {
				xMin -= GetColumnWidth( col );
				SetColumnPos( col, xMin, y );
			}
			xMax = centreX - offset;
			for( int col = midCol; col < columns; col++ ) {
				SetColumnPos( col, xMax, y );
				xMax += GetColumnWidth( col );
			}
			
			OnSort();
			UpdateTableDimensions();
			MoveTo( X, game.Height / 4 );
		}
		
		protected virtual void OnSort() {
		}
	}
}