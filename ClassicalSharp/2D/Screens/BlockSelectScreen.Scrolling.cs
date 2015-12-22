using System;

namespace ClassicalSharp {
	
	public partial class BlockSelectScreen : Screen {
		
		const int scrollbarWidth = 10;
		static FastColour scrollCol = new FastColour( 10, 10, 10, 220 );
		static FastColour scrollUsedCol = new FastColour( 100, 100, 100, 220 );
		void DrawScrollbar() {
			graphicsApi.Draw2DQuad( TableX, TableY, scrollbarWidth,
			                       TableHeight, scrollCol );
			float scale = TableHeight / (float)rows;
			int yOffset = (int)Math.Ceiling(scrollY * scale);
			int height = (int)Math.Ceiling(maxRows * scale);
			
			if( yOffset + height > TableHeight )
				height = TableHeight - yOffset;
			graphicsApi.Draw2DQuad( TableX, TableY + yOffset, scrollbarWidth,
			                       height, scrollUsedCol );
		}
		
		public override bool HandlesMouseScroll( int delta ) {
			bool bounds = Contains( TableX, TableY, TableWidth, TableHeight,
			                       game.Mouse.X, game.Mouse.Y );
			if( !bounds || selIndex == -1 ) return false;
			delta = -delta;
			int startScrollY = scrollY;
			selIndex += delta * blocksPerRow;
			
			if( selIndex >= blocksTable.Length || selIndex < 0 ) 
				selIndex -= delta * blocksPerRow;
			Utils.Clamp( ref selIndex, 0, blocksTable.Length - 1 );
			int endScrollY = selIndex / blocksPerRow;
			
			UpdateSelectedState();
			return true;
		}
		int scrollY;
		
		void UpdateScrollY() {
			scrollY = selIndex / blocksPerRow;
			ClampScrollY();
		}
		
		void ClampScrollY() {
			if( scrollY >= rows - maxRows )
				scrollY = rows - maxRows;
			if( scrollY < 0 )
				scrollY = 0;
		}
		
		void ScrollbarClick( int mouseY ) {
			mouseY -= TableY;
			float scale = TableHeight / (float)rows;
			scrollY = (int)(mouseY / scale);
			ClampScrollY();
		}
	}
}
