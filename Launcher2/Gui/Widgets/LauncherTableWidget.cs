using System;
using System.Collections.Generic;
using System.Drawing;
using ClassicalSharp;

namespace Launcher2 {

	public class LauncherTableWidget : LauncherWidget {
		
		public LauncherTableWidget( LauncherWindow window ) : base( window ) {			
		}
		
		public List<ServerListEntry> Entries;
		public int[] ColumnWidths = { 300, 50, 50, 50, };
		
		public void Display( IDrawer2D drawer, Font font ) {
			int y = 0;
			for( int i = 0; i < 10; i++ ) {
				ServerListEntry entry = Entries[i];
				//Size size = drawer.MeasureSize( entry.Name, font, true );
				//DrawTextArgs args = new DrawTextArgs( entry.Name, true );
				//drawer.DrawText( 
			}
		}
	}
}
