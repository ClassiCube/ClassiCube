using System;
using System.Collections.Generic;
using System.Drawing;
using ClassicalSharp;

namespace Launcher2 {

	public partial class LauncherTableWidget : LauncherWidget {
		
		public LauncherTableWidget( LauncherWindow window ) : base( window ) {
			OnClick = HandleOnClick;
		}
		
	    internal static FastColour backGridCol = new FastColour( 20, 20, 10 ),
	    foreGridCol = new FastColour( 40, 40, 40 );
		public Action NeedRedraw;
		public Action<string> SelectedChanged;
		public int SelectedIndex = -1;
		
		internal TableEntry[] entries, usedEntries;
		internal List<ServerListEntry> servers;
		public void SetEntries( List<ServerListEntry> servers ) {
			entries = new TableEntry[servers.Count];
			usedEntries = new TableEntry[servers.Count];
			this.servers = servers;
			int index = 0;
			
			foreach( ServerListEntry e in servers ) {
				TableEntry tableEntry = default( TableEntry );
				tableEntry.Hash = e.Hash;
				tableEntry.Name = e.Name;
				tableEntry.Players = e.Players + "/" + e.MaximumPlayers;
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
				entries[i] = entry;
			}
			SetSelected( selHash );
		}
		
		public int CurrentIndex, Count;
		public int[] ColumnWidths = { 360, 80, 80 };
		public int[] DesiredColumnWidths = { 360, 80, 80 };
		int defaultInputHeight;
		
		internal struct TableEntry {
			public string Hash, Name, Players, Uptime;
			public int Y, Height;
		}
		
		public void DrawAt( IDrawer2D drawer, Font font, Font titleFont, Font boldFont,
		                   Anchor horAnchor, Anchor verAnchor, int x, int y ) {
			DrawTextArgs args = new DrawTextArgs( "I", font, true );
			defaultInputHeight = drawer.MeasureSize( ref args ).Height;
			
			CalculateOffset( x, y, horAnchor, verAnchor );
			Redraw( drawer, font, titleFont, boldFont );
		}
		
		static FastColour backCol = new FastColour( 120, 85, 151 ), foreCol = new FastColour( 160, 133, 186 );
		static FastColour scrollCol = new FastColour( 200, 184, 216 );
		public void Redraw( IDrawer2D drawer, Font font, Font titleFont, Font boldFont ) {
			ColumnWidths[0] = DesiredColumnWidths[0];
			Utils.Clamp( ref ColumnWidths[0], 20, Window.Width - 20 );
			int x = X + 5;
			
			DrawGrid( drawer, font, titleFont );
			x += DrawColumn( drawer, false, font, titleFont, boldFont, 
			                "Name", ColumnWidths[0], x, e => e.Name ) + 5;
			x += DrawColumn( drawer, true, font, titleFont, boldFont, 
			                "Players", ColumnWidths[1], x, e => e.Players ) + 5;
			x += DrawColumn( drawer, true, font, titleFont, boldFont,
			                "Uptime", ColumnWidths[2], x, e => e.Uptime ) + 5;
			
			Width = Window.Width;
			DrawScrollbar( drawer );
		}
		
		int DrawColumn( IDrawer2D drawer, bool separator, Font font, Font titleFont, Font boldFont,
		               string header, int maxWidth, int x, Func<TableEntry, string> filter ) {
			int y = Y + 3;
			DrawTextArgs args = new DrawTextArgs( header, titleFont, true );
			TableEntry headerEntry = default( TableEntry );
			DrawColumnEntry( drawer, ref args, maxWidth, x, ref y, ref headerEntry );
			maxIndex = Count;
			y += 2;
			int bodyStartY = y;
			y += 3;
			
			for( int i = CurrentIndex; i < Count; i++ ) {
				args = new DrawTextArgs( filter( usedEntries[i] ), font, true );
				if( i == SelectedIndex && !separator )
					drawer.DrawRect( foreGridCol, 0, y - 3, Width, defaultInputHeight + 4 );
				
				if( !DrawColumnEntry( drawer, ref args, maxWidth, x, ref y, ref usedEntries[i] ) ) {
					maxIndex = i;
					break;
				}
			}
			
			Height = Window.Height - Y;
			if( separator )
				drawer.DrawRect( foreCol, x - 7, bodyStartY, 2, Height );
			return maxWidth + 5;
		}
		
		bool DrawColumnEntry( IDrawer2D drawer, ref DrawTextArgs args,
		                     int maxWidth, int x, ref int y, ref TableEntry entry ) {
			Size size = drawer.MeasureSize( ref args );
			if( y + size.Height > Window.Height ) {
				y = Window.Height + 3; return false;
			}
			size.Width = Math.Min( maxWidth, size.Width );
			entry.Y = y; entry.Height = size.Height;
			
			args.SkipPartsCheck = false;
			drawer.DrawClippedText( ref args, x, y, maxWidth, 30 );
			y += size.Height + 3;
			return true;
		}
		
		int headerStartY, headerEndY;
		int numEntries = 0;
		void DrawGrid( IDrawer2D drawer, Font font, Font titleFont ) {
			DrawTextArgs args = new DrawTextArgs( "I", titleFont, true );
			Size size = drawer.MeasureSize( ref args );
			drawer.DrawRect( foreCol, 0, Y + size.Height + 5, Window.Width, 2 );
			headerStartY = Y;
			
			headerEndY = Y + size.Height + 5;	
			int startY = headerEndY + 3;		
			numEntries = (Window.Height - startY) / (defaultInputHeight + 3);
		}
		
		int maxIndex;
		void DrawScrollbar( IDrawer2D drawer ) {
			drawer.DrawRect( backCol, Window.Width - 10, Y, 10, Window.Height - Y );
			float scale = (Window.Height - Y) / (float)Count;
			
			int y1 = (int)(Y + CurrentIndex * scale);
			int height = (int)((maxIndex - CurrentIndex) * scale);
			drawer.DrawRect( scrollCol, Window.Width - 10, y1, 10, height );
		}
		
		public void SetSelected( int index ) {
			if( index >= maxIndex ) CurrentIndex = index + 1 - numEntries;
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
			if( CurrentIndex > Count - numEntries )
				CurrentIndex = Count - numEntries;
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
		
		abstract class TableEntryComparer : IComparer<TableEntry> {
			
			public bool Invert = true;
			
			public abstract int Compare( TableEntry a, TableEntry b );
		}
		
		sealed class NameComparer : TableEntryComparer {
			
			public override int Compare( TableEntry a, TableEntry b ) {
				StringComparison comparison = StringComparison.CurrentCultureIgnoreCase;
				int value = String.Compare( a.Name, b.Name, comparison );
				return Invert ? -value : value;
			}
		}
		
		sealed class PlayersComparer : TableEntryComparer {
			
			public override int Compare( TableEntry a, TableEntry b ) {
				long valX = Int64.Parse( a.Players.Substring( 0, a.Players.IndexOf( '/' ) ) );
				long valY = Int64.Parse( b.Players.Substring( 0, b.Players.IndexOf( '/' ) ) );
				int value = valX.CompareTo( valY );
				return Invert ? -value : value;
			}
		}
		
		sealed class UptimeComparer : TableEntryComparer {
			
			public override int Compare( TableEntry a, TableEntry b ) {
				TimeSpan valX = ParseUptimeString( a.Uptime );
				TimeSpan valY = ParseUptimeString( b.Uptime );
				int value = valX.CompareTo( valY );
				return Invert ? -value : value;
			}
			
			static TimeSpan ParseUptimeString( string s ) {
				int sum = 0;
				for( int i = 0; i < s.Length - 1; i++ ) {
					sum *= 10;
					sum += s[i] - '0';
				}
				
				char timeType = s[s.Length - 1];
				switch( timeType ) {
						case 'w' : return TimeSpan.FromDays( sum * 7 );
						case 'd' : return TimeSpan.FromDays( sum );
						case 'h' : return TimeSpan.FromHours( sum );
						case 'm' : return TimeSpan.FromMinutes( sum );
						case 's' : return TimeSpan.FromSeconds( sum );
						default: throw new NotSupportedException( "unsupported uptime type: " + timeType );
				}
			}
		}
	}
}
