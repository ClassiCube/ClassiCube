// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using OpenTK.Input;

namespace ClassicalSharp.Gui {
	
	public partial class BlockSelectScreen : Screen {
		
		const int scrollbarWidth = 10;
		static FastColour scrollCol = new FastColour( 10, 10, 10, 220 );
		static FastColour scrollUsedCol = new FastColour( 100, 100, 100, 220 );
		void DrawScrollbar() {
			graphicsApi.Draw2DQuad( TableX, TableY, scrollbarWidth,
			                       TableHeight, scrollCol );
			float scale = TableHeight / (float)rows;
			int yOffset = (int)Math.Ceiling( scrollY * scale );
			int height = (int)Math.Ceiling( maxRows * scale );
			
			if( yOffset + height > TableHeight )
				height = TableHeight - yOffset;
			graphicsApi.Draw2DQuad( TableX, TableY + yOffset, scrollbarWidth,
			                       height, scrollUsedCol );
		}
		
		public override bool HandlesMouseScroll( int delta ) {
			bool bounds = Contains( TableX, TableY, TableWidth, TableHeight,
			                       game.Mouse.X, game.Mouse.Y );
			if( !bounds ) return false;
			int rowY = (selIndex / blocksPerRow) - scrollY;
			scrollY -= delta;
			ClampScrollY();
			if( selIndex == - 1 ) return true;
			
			selIndex = scrollY * blocksPerRow + (selIndex % blocksPerRow);
			for( int row = 0; row < rowY; row++ ) {
				if( selIndex + blocksPerRow >= blocksTable.Length ) break;
				selIndex += blocksPerRow;
			}
			MoveCursorToSelected();
			RecreateBlockInfoTexture();
			return true;
		}
		int scrollY;
		
		void ClampScrollY() {
			if( scrollY >= rows - maxRows ) scrollY = rows - maxRows;
			if( scrollY < 0 ) scrollY = 0;
		}
		
		void ScrollbarClick( int mouseY ) {
			mouseY -= TableY;
			float scale = TableHeight / (float)rows;
			scrollY = (int)(mouseY / scale);
			ClampScrollY();
		}
		
		bool draggingMouse = false;
		
		public override bool HandlesMouseUp( int mouseX, int mouseY, MouseButton button ) {
			draggingMouse = false;
			return true;
		}
	}
}
