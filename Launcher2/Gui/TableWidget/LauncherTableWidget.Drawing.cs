// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Collections.Generic;
using System.Drawing;
using ClassicalSharp;

namespace Launcher {

	public partial class LauncherTableWidget : LauncherWidget {
		
		internal static FastColour backGridCol = new FastColour( 20, 20, 10 ),
		foreGridCol = new FastColour( 40, 40, 40 );
		
		public int[] ColumnWidths = { 340, 65, 65, 140 };
		public int[] DesiredColumnWidths = { 340, 65, 65, 140 };
		int defaultInputHeight, defaultHeaderHeight;
		
		internal struct TableEntry {
			public string Hash, Name, Players, Uptime, Software;
			public int Y, Height;
		}
		
		Font font, titleFont;
		public void SetDrawData( IDrawer2D drawer, Font font, Font titleFont,
		                        Anchor horAnchor, Anchor verAnchor, int x, int y ) {
			CalculateOffset( x, y, horAnchor, verAnchor );
			this.font = font;
			this.titleFont = titleFont;
			
			DrawTextArgs args = new DrawTextArgs( "IMP", titleFont, true );
			defaultHeaderHeight = drawer.MeasureSize( ref args ).Height;
			args = new DrawTextArgs( "IMP", font, true );
			defaultInputHeight = drawer.MeasureSize( ref args ).Height;
		}
		
		public void RecalculateDrawData() {
			for( int i = 0; i < ColumnWidths.Length; i++ ) {
				ColumnWidths[i] = DesiredColumnWidths[i];
				Utils.Clamp( ref ColumnWidths[i], 20, Window.Width - 20 );
			}
			Width = Window.Width - X;
			ResetEntries();
			
			int y = Y + 3;
			y += defaultHeaderHeight + 2;
			maxIndex = Count;
			y += 5;

			for( int i = CurrentIndex; i < Count; i++ ) {
				if( y + defaultInputHeight > Y + Height ) {
					maxIndex = i; return;
				}
				
				usedEntries[i].Y = y;
				usedEntries[i].Height = defaultInputHeight;
				y += defaultInputHeight + 2;
			}
		}
		
		public void RedrawData( IDrawer2D drawer ) {
			int x = X + 5;
			DrawGrid( drawer, font, titleFont );
			x += DrawColumn( drawer, false, font, titleFont,
			                "Name", ColumnWidths[0], x, e => e.Name ) + 5;
			x += DrawColumn( drawer, true, font, titleFont,
			                "Players", ColumnWidths[1], x, e => e.Players ) + 5;
			x += DrawColumn( drawer, true, font, titleFont,
			                "Uptime", ColumnWidths[2], x, e => e.Uptime ) + 5;
			x += DrawColumn( drawer, true, font, titleFont,
			                "Software", ColumnWidths[3], x, e => e.Software ) + 5;
			DrawScrollbar( drawer );
		}
		
		public override void Redraw( IDrawer2D drawer ) {				
			RecalculateDrawData();			
			RedrawData( drawer );
		}
		
		int DrawColumn( IDrawer2D drawer, bool separator, Font font, Font titleFont,
		               string header, int maxWidth, int x, Func<TableEntry, string> filter ) {
			int y = Y + 3;
			DrawTextArgs args = new DrawTextArgs( header, titleFont, true );
			TableEntry headerEntry = default( TableEntry );
			DrawColumnEntry( drawer, ref args, maxWidth, x, ref y, ref headerEntry );
			maxIndex = Count;
			y += 5;
			
			for( int i = CurrentIndex; i < Count; i++ ) {
				args = new DrawTextArgs( filter( usedEntries[i] ), font, true );
				if( i == SelectedIndex && !separator ) {
					int startY = y - 3;
					int height = Math.Min( startY + (defaultInputHeight + 4), Y + Height ) - startY;
					drawer.Clear( foreGridCol, X, startY, Width, height );
				}
				
				if( !DrawColumnEntry( drawer, ref args, maxWidth, x, ref y, ref usedEntries[i] ) ) {
					maxIndex = i; break;
				}
			}
			
			if( separator && !Window.ClassicBackground )
				drawer.Clear( LauncherSkin.BackgroundCol, x - 7, Y, 2, Height );
			return maxWidth + 5;
		}
		
		bool DrawColumnEntry( IDrawer2D drawer, ref DrawTextArgs args,
		                     int maxWidth, int x, ref int y, ref TableEntry entry ) {
			Size size = drawer.MeasureSize( ref args );
			bool empty = args.Text == "";
			if( empty )
				size.Height = defaultInputHeight;
			if( y + size.Height > Y + Height ) {
				y = Y + Height + 2; return false;
			}
			
			entry.Y = y; entry.Height = size.Height;
			if( !empty ) {
				size.Width = Math.Min( maxWidth, size.Width );
				args.SkipPartsCheck = false;
				drawer.DrawClippedText( ref args, x, y, maxWidth, 30 );
			}
			y += size.Height + 2;
			return true;
		}
		
		void ResetEntries() {
			for( int i = 0; i < Count; i++ ) {
				entries[i].Height = 0; usedEntries[i].Height = 0;
				entries[i].Y = -10; usedEntries[i].Y = -10;
			}
		}
		
		int headerStartY, headerEndY;
		int numEntries = 0;
		void DrawGrid( IDrawer2D drawer, Font font, Font titleFont ) {
			DrawTextArgs args = new DrawTextArgs( "I", titleFont, true );
			Size size = drawer.MeasureSize( ref args );
			if( !Window.ClassicBackground )
				drawer.Clear( LauncherSkin.BackgroundCol, X, Y + size.Height + 5, Width, 2 );
			headerStartY = Y;
			
			headerEndY = Y + size.Height + 5;
			int startY = headerEndY + 3;
			numEntries = (Y + Height - startY) / (defaultInputHeight + 3);
		}
		
		int maxIndex;
		void DrawScrollbar( IDrawer2D drawer ) {
			FastColour col = Window.ClassicBackground ? new FastColour( 80, 80, 80 ) : LauncherSkin.ButtonBorderCol;
			drawer.Clear( col, Window.Width - 10, Y, 10, Height );
			col = Window.ClassicBackground ? new FastColour( 160, 160, 160 ) : LauncherSkin.ButtonForeActiveCol;
			int yOffset, height;
			GetScrollbarCoords( out yOffset, out height );
			drawer.Clear( col, Window.Width - 10, Y + yOffset, 10, height );
		}
	}
}
