using System;
using System.Collections.Generic;
using ClassicalSharp;

namespace Launcher2 {

	public partial class LauncherTableWidget : LauncherWidget {

		NameComparer nameComp = new NameComparer();
		PlayersComparer playerComp = new PlayersComparer();
		public bool DraggingWidth = false;
		
		void HandleOnClick( int mouseX, int mouseY ) {
			if( mouseX >= Window.Width - 10 ) {
				ScrollbarClick( mouseY );
				lastIndex = -10;
				return;
			}
			
			if( mouseY >= headerStartY && mouseY < headerEndY ) {
				if( mouseX < ColumnWidths[0] - 10 ) {				
					Array.Sort( usedEntries, 0, Count, nameComp );
					Array.Sort( entries, 0, entries.Length, nameComp );
					nameComp.Invert = !nameComp.Invert;
					NeedRedraw();
				} else if( mouseX > ColumnWidths[0] + 10 ) {					
					Array.Sort( usedEntries, 0, Count, playerComp );
					Array.Sort( entries, 0, entries.Length, playerComp );
					playerComp.Invert = !playerComp.Invert;
					NeedRedraw();
				} else {
					DraggingWidth = true;
				}
				lastIndex = -10;
			} else {
				for( int i = 0; i < Count; i++ ) {
					TableEntry entry = usedEntries[i];
					if( mouseY >= entry.Y && mouseY < entry.Y + entry.Height ) {
						if( lastIndex == i && (DateTime.UtcNow - lastPress).TotalSeconds > 1 ) {
							Window.ConnectToServer( servers, entry.Hash );
							lastPress = DateTime.UtcNow;
						}
						SelectedChanged( entry.Hash );
						lastIndex = i;
						break;
					}
				}
			}
		}
		
		int lastIndex = -10;
		DateTime lastPress;
		public void MouseMove( int deltaX, int deltaY ) {
			if( DraggingWidth ) {
				ColumnWidths[0] += deltaX;
				Utils.Clamp( ref ColumnWidths[0], 20, Window.Width - 20 );
				NeedRedraw();
			}
		}
		
		void ScrollbarClick( int mouseY ) {
			mouseY -= Y;
			float scale = (Window.Height - 10) / (float)Count;
			
			int currentIndex = (int)(mouseY / scale);
			CurrentIndex = currentIndex;
			NeedRedraw();
		}
	}
}
