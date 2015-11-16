using System;
using System.Collections.Generic;
using ClassicalSharp;

namespace Launcher2 {

	public partial class LauncherTableWidget : LauncherWidget {

		NameComparer nameComp = new NameComparer();
		PlayersComparer playerComp = new PlayersComparer();
		UptimeComparer uptimeComp = new UptimeComparer();
		public bool DraggingWidth = false;
		
		void HandleOnClick( int mouseX, int mouseY ) {
			if( mouseX >= Window.Width - 10 ) {
				ScrollbarClick( mouseY );
				lastIndex = -10; return;
			}
			
			if( mouseY >= headerStartY && mouseY < headerEndY ) {
				SelectHeader( mouseX, mouseY );
			} else {
				GetSelectedServer( mouseX, mouseY );
			}
		}
		
		void SelectHeader( int mouseX, int mouseY ) {
			int x = 0;
			lastIndex = -10;
			
			if( mouseX >= x && mouseX < x + ColumnWidths[0] - 10 ) {
				SortEntries( nameComp ); return;
			}
			x += ColumnWidths[0];		
			if( mouseX >= x + 15 && mouseX < x + ColumnWidths[1] ) {
				SortEntries( playerComp ); return;
			}
			x += ColumnWidths[1];
			if( mouseX >= x ) {
				SortEntries( uptimeComp ); return;
			}
			
			DraggingWidth = true;
		}
		
		void SortEntries( TableEntryComparer comparer ) {
			Array.Sort( usedEntries, 0, Count, comparer );
			Array.Sort( entries, 0, entries.Length, comparer );
			comparer.Invert = !comparer.Invert;
			NeedRedraw();
		}
		
		void GetSelectedServer( int mouseX, int mouseY ) {
			for( int i = 0; i < Count; i++ ) {
				TableEntry entry = usedEntries[i];
				if( mouseY < entry.Y || mouseY >= entry.Y + entry.Height ) continue;
				
				if( lastIndex == i && (DateTime.UtcNow - lastPress).TotalSeconds > 1 ) {
					Window.ConnectToServer( servers, entry.Hash );
					lastPress = DateTime.UtcNow;
				}
				SelectedChanged( entry.Hash );
				SelectedHash = entry.Hash;
				
				NeedRedraw();
				lastIndex = i;
				break;
			}
		}
		
		int lastIndex = -10;
		DateTime lastPress;
		public void MouseMove( int deltaX, int deltaY ) {
			if( DraggingWidth ) {
				ColumnWidths[0] += deltaX;
				Utils.Clamp( ref ColumnWidths[0], 20, Window.Width - 20 );
				DesiredColumnWidths[0] = ColumnWidths[0];
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
