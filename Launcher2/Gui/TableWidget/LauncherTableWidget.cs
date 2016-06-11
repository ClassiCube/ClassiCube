// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Collections.Generic;
using System.Drawing;
using ClassicalSharp;

namespace Launcher.Gui.Widgets {

	internal struct TableEntry {
		public string Hash, Name, Players, Uptime, Software;
		public int Y, Height;
	}
	
	public partial class LauncherTableWidget : LauncherWidget {
		
		LauncherTableView view;
		public LauncherTableWidget( LauncherWindow window ) : base( window ) {
			OnClick = HandleOnClick;
			view = new LauncherTableView();
			view.Init( window, this );
		}
		
		public Action NeedRedraw;
		public Action<string> SelectedChanged;
		public int SelectedIndex = -1;
		public int CurrentIndex, Count;
		
		internal TableEntry[] entries, usedEntries;
		internal List<ServerListEntry> servers;
		
		public void SetEntries( List<ServerListEntry> servers ) {
			entries = new TableEntry[servers.Count];
			usedEntries = new TableEntry[servers.Count];
			this.servers = servers;
			int index = 0;
			
			foreach( ServerListEntry e in servers ) {
				TableEntry tableEntry = default(TableEntry);
				tableEntry.Hash = e.Hash;
				tableEntry.Name = e.Name;
				tableEntry.Players = e.Players + "/" + e.MaximumPlayers;
				tableEntry.Software = e.Software;
				tableEntry.Uptime = MakeUptime( e.Uptime );
				
				entries[index] = tableEntry;
				usedEntries[index] = tableEntry;
				index++;
			}
			Count = entries.Length;
		}
		
		/// <summary> Filters the table such that only rows with server names
		/// that contain the input (case insensitive) are shown. </summary>
		public void FilterEntries( string filter ) {
			string selHash = SelectedIndex >= 0 ? usedEntries[SelectedIndex].Hash : "";
			Count = 0;
			int index = 0;
			
			for( int i = 0; i < entries.Length; i++ ) {
				TableEntry entry = entries[i];
				if( entry.Name.IndexOf( filter, StringComparison.OrdinalIgnoreCase ) >= 0 ) {
					Count++;
					usedEntries[index++] = entry;
				}
			}
			SetSelected( selHash );
		}		
		
		internal void GetScrollbarCoords( out int y, out int height ) {
			if( Count == 0 ) { y = 0; height = 0; return; }
			
			float scale = Height / (float)Count;
			y = (int)Math.Ceiling( CurrentIndex * scale );
			height = (int)Math.Ceiling( (view.maxIndex - CurrentIndex) * scale );
			height = Math.Min( y + height, Height ) - y;
		}
		
		public void SetSelected( int index ) {
			if( index >= view.maxIndex ) CurrentIndex = index + 1 - view.numEntries;
			if( index < CurrentIndex ) CurrentIndex = index;
			if( index >= Count ) index = Count - 1;
			if( index < 0 ) index = 0;
			
			SelectedIndex = index;
			lastIndex = index;
			ClampIndex();
			if( Count > 0 )
				SelectedChanged( usedEntries[index].Hash );
		}
		
		public void SetSelected( string hash ) {
			SelectedIndex = -1;
			for( int i = 0; i < Count; i++ ) {
				if( usedEntries[i].Hash == hash ) {
					SetSelected( i );
					return;
				}
			}
		}
		
		public void ClampIndex() {
			if( CurrentIndex > Count - view.numEntries )
				CurrentIndex = Count - view.numEntries;
			if( CurrentIndex < 0 )
				CurrentIndex = 0;
		}
		
		string MakeUptime( string rawSeconds ) {
			TimeSpan t = TimeSpan.FromSeconds( Double.Parse( rawSeconds ) );
			if( t.TotalHours >= 24 * 7 )
				return (int)t.TotalDays + "d";
			if( t.TotalHours >= 1 )
				return (int)t.TotalHours + "h";
			if( t.TotalMinutes >= 1 )
				return (int)t.TotalMinutes + "m";
			return (int)t.TotalSeconds + "s";
		}
		
		
		public int[] ColumnWidths = { 340, 65, 65, 140 };
		public int[] DesiredColumnWidths = { 340, 65, 65, 140 };

		public void SetDrawData( IDrawer2D drawer, Font font, Font titleFont,
		                        Anchor horAnchor, Anchor verAnchor, int x, int y ) {
			CalculateOffset( x, y, horAnchor, verAnchor );
			view.SetDrawData( drawer, font, titleFont );
		}
		
		public void RecalculateDrawData() { view.RecalculateDrawData(); }
		
		public void RedrawData( IDrawer2D drawer ) { view.RedrawData( drawer ); }
		
		public override void Redraw( IDrawer2D drawer ) {
			RecalculateDrawData();
			RedrawData( drawer );
		}
	}
}
