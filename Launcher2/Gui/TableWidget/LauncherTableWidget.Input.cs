// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Collections.Generic;
using ClassicalSharp;

namespace Launcher {

	public partial class LauncherTableWidget : LauncherWidget {

		NameComparer nameComp = new NameComparer();
		PlayersComparer playerComp = new PlayersComparer();
		UptimeComparer uptimeComp = new UptimeComparer();
		SoftwareComparer softwareComp = new SoftwareComparer();
		public int DraggingColumn = -1;
		public bool DraggingScrollbar = false;
		
		void HandleOnClick( int mouseX, int mouseY ) {			
			if( mouseX >= Window.Width - 10 ) {
				ScrollbarClick( mouseY );
				DraggingScrollbar = true;
				lastIndex = -10; return;
			}
			
			if( mouseY >= headerStartY && mouseY < headerEndY ) {
				SelectHeader( mouseX, mouseY );
			} else {
				GetSelectedServer( mouseX, mouseY );
			}
			lastPress = DateTime.UtcNow;
		}
		
		void SelectHeader( int mouseX, int mouseY ) {
			int x = X;		
			for( int i = 0; i < ColumnWidths.Length; i++ ) {
				x += ColumnWidths[i] + 10;
				if( mouseX >= x - 8 && mouseX < x + 8 ) {
					DraggingColumn = i;
					lastIndex = -10; return;
				}
			}		
			TrySortColumns( mouseX );
		}
		
		void TrySortColumns( int mouseX ) {
			int x = X;
			if( mouseX >= x && mouseX < x + ColumnWidths[0] - 10 ) {
				SortEntries( nameComp ); return;
			}
			x += ColumnWidths[0];
			if( mouseX >= x && mouseX < x + ColumnWidths[1] ) {
				SortEntries( playerComp ); return;
			}
			x += ColumnWidths[1];
			if( mouseX >= x && mouseX < x + ColumnWidths[2] ) {
				SortEntries( uptimeComp ); return;
			}
			x += ColumnWidths[2];
			if( mouseX >= x ) {
				SortEntries( softwareComp ); return;
			}
		}
		
		void SortEntries( TableEntryComparer comparer ) {
			string selHash = SelectedIndex >= 0 ? usedEntries[SelectedIndex].Hash : "";
			Array.Sort( usedEntries, 0, Count, comparer );
			Array.Sort( entries, 0, entries.Length, comparer );
			lastIndex = -10;
			
			comparer.Invert = !comparer.Invert;
			SetSelected( selHash );
			NeedRedraw();
		}
		
		void GetSelectedServer( int mouseX, int mouseY ) {
			for( int i = 0; i < Count; i++ ) {
				TableEntry entry = usedEntries[i];
				if( mouseY < entry.Y || mouseY >= entry.Y + entry.Height + 2 ) continue;
				
				if( lastIndex == i && (DateTime.UtcNow - lastPress).TotalSeconds < 1 ) {
					Window.ConnectToServer( servers, entry.Hash );
					lastPress = DateTime.MinValue;
					return;
				}
				
				SetSelected( i );
				NeedRedraw();
				break;
			}
		}
		
		int lastIndex = -10;
		DateTime lastPress;
		public void MouseMove( int x, int y, int deltaX, int deltaY ) {
			if( DraggingScrollbar ) {
				ScrollbarClick( y );
			} else if( DraggingColumn >= 0 ) {
				if( x >= Window.Width - 20 ) return;
				int col = DraggingColumn;
				ColumnWidths[col] += deltaX;
				Utils.Clamp( ref ColumnWidths[col], 20, Window.Width - 20 );
				DesiredColumnWidths[col] = ColumnWidths[col];
				NeedRedraw();
			}
		}
		
		void ScrollbarClick( int mouseY ) {
			mouseY -= Y;
			float scale = (Window.Height - 10) / (float)Count;
			
			int currentIndex = (int)(mouseY / scale);
			CurrentIndex = currentIndex;
			ClampIndex();
			NeedRedraw();
		}
	}
}
