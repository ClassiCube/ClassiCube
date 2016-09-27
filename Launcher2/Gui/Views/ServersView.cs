// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp;
using Launcher.Drawing;
using Launcher.Gui.Widgets;
using OpenTK.Input;

namespace Launcher.Gui.Views {	
	public sealed class ServersView : IView {
		
		internal int searchIndex = 0, hashIndex = 1;
		internal int backIndex = 2, connectIndex = 3, tableIndex = 4;
		Font tableFont;
		const int tableX = 10, tableY = 50;
		
		public ServersView( LauncherWindow game ) : base( game ) {
			widgets = new Widget[5];
		}
		
		public override void Init() {
			titleFont = new Font( game.FontName, 15, FontStyle.Bold );
			textFont = new Font( game.FontName, 14, FontStyle.Regular );
			inputHintFont = new Font( game.FontName, 12, FontStyle.Italic );
			tableFont = new Font( game.FontName, 11, FontStyle.Regular );
			MakeWidgets();
		}
		
		public override void DrawAll() {
			DrawBackground();
			TableWidget table = (TableWidget)widgets[tableIndex];
			if( table != null ) table.ClampIndex();
			base.DrawAll();
		}
		
		protected override void MakeWidgets() {
			widgetIndex = 0;
			MakeInput( Get( 0 ), 475, false, 32, "&7Search servers.." )
				.SetLocation( Anchor.LeftOrTop, Anchor.LeftOrTop, 10, 10 );
			MakeInput( Get( 1 ), 475, false, 32, "&7classicube.net/server/play/..." )
				.SetLocation( Anchor.LeftOrTop, Anchor.BottomOrRight, 10, -10 );
			
			Makers.Button( this, "Back", 110, 30, titleFont )
				.SetLocation( Anchor.BottomOrRight, Anchor.LeftOrTop, -20, 10 );
			Makers.Button( this, "Connect", 110, 30, titleFont )
				.SetLocation( Anchor.BottomOrRight, Anchor.BottomOrRight, -20, -10 );
			MakeTableWidget();
		}
		
		string Get( int index ) {
			Widget widget = widgets[index];
			return widget == null ? "" : widget.Text;
		}
		
		void DrawBackground() {
			using( FastBitmap bmp = game.LockBits() ) {
				game.ResetArea( 0, 0, game.Width, tableY, bmp );
				DrawTableBackground( bmp );
			}
		}
		
		void DrawTableBackground( FastBitmap dst ) {
			int tableHeight = Math.Max( game.Height - tableY - 50, 1 );
			Rectangle rec = new Rectangle( tableX, tableY, game.Width - tableX, tableHeight );
			
			if( !game.ClassicBackground ) {
				FastColour col = TableView.backGridCol;
				Drawer2DExt.Clear( dst, rec, col );
			} else {
				game.ResetArea( rec.X, rec.Y, rec.Width, rec.Height, dst );
			}
		}
		
		void MakeTableWidget() {
			int tableHeight = Math.Max( game.Height - tableY - 50, 1 );
			TableWidget widget;
			if( widgets[tableIndex] != null ) {
				widget = (TableWidget)widgets[tableIndex];
			} else {
				widget = new TableWidget( game );
				widget.SetEntries( game.Session.Servers );
				widget.SetDrawData( drawer, tableFont, textFont,
				                   Anchor.LeftOrTop, Anchor.LeftOrTop, tableX, tableY );
				widget.SortDefault();
				widgets[widgetIndex] = widget;
			}
			
			widget.Height = tableHeight;
			widgetIndex++;
		}
		
		public override void Dispose() {
			base.Dispose();
			tableFont.Dispose();
		}
		
		internal void RedrawTable() {
			using( FastBitmap dst = game.LockBits() )
				DrawTableBackground( dst );
			TableWidget table = (TableWidget)widgets[tableIndex];
			table.ClampIndex();
			
			int tableHeight = Math.Max( game.Height - tableY - 50, 1 );
			table.Height = tableHeight;
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				table.RedrawData( drawer );
			}
		}
	}
}
