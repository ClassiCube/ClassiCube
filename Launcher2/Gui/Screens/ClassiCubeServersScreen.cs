// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp;
using OpenTK.Input;

namespace Launcher {
	
	public sealed class ClassiCubeServersScreen : LauncherInputScreen {
		
		const int tableIndex = 4, searchIndex = 0, hashIndex = 1;
		Font tableFont;
		const int tableX = 10, tableY = 50;
		
		public ClassiCubeServersScreen( LauncherWindow game ) : base( game, true ) {
			tableFont = new Font( game.FontName, 11, FontStyle.Regular );
			enterIndex = 3;
			widgets = new LauncherWidget[7];
		}
		
		public override void Tick() {
			base.Tick();
			LauncherTableWidget table = (LauncherTableWidget)widgets[tableIndex];
			if( !game.Window.Mouse[MouseButton.Left] ) {
				table.DraggingColumn = -1;
				table.DraggingScrollbar = false;
				table.mouseOffset = 0;
			}
		}
		
		protected override void MouseMove( object sender, MouseMoveEventArgs e ) {
			base.MouseMove( sender, e );
			if( selectedWidget != null && selectedWidget == widgets[tableIndex] ) {
				LauncherTableWidget table = (LauncherTableWidget)widgets[tableIndex];
				table.MouseMove( e.X, e.Y, e.XDelta, e.YDelta );
			}
		}
		
		void MouseButtonUp( object sender, MouseButtonEventArgs e ) {
			LauncherTableWidget table = (LauncherTableWidget)widgets[tableIndex];
			table.DraggingColumn = -1;
			table.DraggingScrollbar = false;
			table.mouseOffset = 0;
		}
		
		protected override void OnAddedChar() { FilterList(); }
		
		protected override void OnRemovedChar() { FilterList(); }
		
		protected override void KeyDown( object sender, KeyboardKeyEventArgs e ) {
			LauncherTableWidget table = (LauncherTableWidget)widgets[tableIndex];
			if( e.Key == Key.Enter ) {
				string curServer = Get( hashIndex ) ?? "";
				if( table.Count >= 1 && curServer == "" ) {
					widgets[hashIndex].Text = table.usedEntries[0].Hash;
					ConnectToServer( 0, 0 );
				} else if( curServer != "" &&
				          (selectedWidget == null || selectedWidget == widgets[tableIndex]) ) {
					ConnectToServer( 0, 0 );
				}
			} else if( e.Key == Key.Up ) {
				table.SetSelected( table.SelectedIndex - 1 );
				table.NeedRedraw();
			} else if( e.Key == Key.Down ) {
				table.SetSelected( table.SelectedIndex + 1 );
				table.NeedRedraw();
			} else {
				base.KeyDown( sender, e );
			}
		}
		
		protected override void RedrawLastInput() {
			base.RedrawLastInput();
			if( lastInput != widgets[hashIndex] )
				return;
			LauncherTableWidget table = (LauncherTableWidget)widgets[tableIndex];
			table.SetSelected( widgets[hashIndex].Text );
			MarkPendingRedraw();
		}

		public override void Init() {
			base.Init();
			game.Window.Mouse.ButtonUp += MouseButtonUp;
			
			Resize();
			selectedWidget = widgets[searchIndex];
			InputClick( 0, 0 );
			lastClicked = lastInput;
		}
		
		public override void Resize() {
			DrawBackground();
			LauncherTableWidget table = (LauncherTableWidget)widgets[tableIndex];
			if( table != null )
				table.ClampIndex();
			MakeWidgets();
			RedrawAllButtonBackgrounds();
			
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				RedrawAll();
			}
			Dirty = true;
		}
		
		void MakeWidgets() {
			widgetIndex = 0;
			MakeInput( Get(), 475, Anchor.LeftOrTop, Anchor.LeftOrTop,
			          false, 10, 10, 32, "&7Search servers.." );
			MakeInput( Get(), 475, Anchor.LeftOrTop, Anchor.BottomOrRight,
			          false, 10, -10, 32, "&7classicube.net/server/play/..." );
			((LauncherInputWidget)widgets[hashIndex]).ClipboardFilter = HashFilter;
			
			MakeButtonAt( "Back", 110, 30, titleFont, Anchor.BottomOrRight, Anchor.LeftOrTop,
			             -20, 10, (x, y) => game.SetScreen( new MainScreen( game ) ) );
			MakeButtonAt( "Connect", 110, 30, titleFont, Anchor.BottomOrRight, Anchor.BottomOrRight,
			             -20, -10, ConnectToServer );
			MakeTableWidget();
		}
		
		void DrawBackground() {
			using( FastBitmap dst = new FastBitmap( game.Framebuffer, true, false ) ) {
				game.ClearArea( 0, 0, game.Width, tableY, dst );
				DrawTableBackground( dst );
			}
		}
		
		void DrawTableBackground( FastBitmap dst ) {
			int tableHeight = Math.Max( game.Height - tableY - 50, 1 );
			Rectangle rec = new Rectangle( tableX, tableY, game.Width - tableX, tableHeight );
			
			if( !game.ClassicBackground ) {
				FastColour col = LauncherTableWidget.backGridCol;
				Drawer2DExt.FastClear( dst, rec, col );
			} else {
				game.ClearArea( rec.X, rec.Y, rec.Width, rec.Height, dst );
			}
		}
		
		void MakeTableWidget() {
			int tableHeight = Math.Max( game.Height - tableY - 50, 1 );
			LauncherTableWidget widget;
			if( widgets[tableIndex] != null ) {
				widget = (LauncherTableWidget)widgets[tableIndex];
			} else {
				widget = new LauncherTableWidget( game );
				widget.CurrentIndex = 0;
				widget.SetEntries( game.Session.Servers );
				
				widget.SetDrawData( drawer, tableFont, inputFont,
				                   Anchor.LeftOrTop, Anchor.LeftOrTop, tableX, tableY );
				widget.NeedRedraw = MarkPendingRedraw;
				widget.SelectedChanged = SelectedChanged;
				widget.SortDefault();
				widgets[widgetIndex] = widget;
			}
			
			widget.Height = tableHeight;
			widgetIndex++;
		}
		
		void FilterList() {
			if( lastInput != widgets[searchIndex] )
				return;
			LauncherTableWidget table = (LauncherTableWidget)widgets[tableIndex];
			table.FilterEntries( lastInput.Text );
			MarkPendingRedraw();
		}

		void SelectedChanged( string hash ) {
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				Set( hashIndex, hash );
			}
			Dirty = true;
		}
		
		void ConnectToServer( int mouseX, int mouseY ) {
			LauncherTableWidget table = (LauncherTableWidget)widgets[tableIndex];
			game.ConnectToServer( table.servers, Get( hashIndex ) );
		}
		
		protected override void MouseWheelChanged( object sender, MouseWheelEventArgs e ) {
			LauncherTableWidget table = (LauncherTableWidget)widgets[tableIndex];
			table.CurrentIndex -= e.Delta;
			MarkPendingRedraw();
		}
		
		string HashFilter( string input ) {
			// Server url look like http://www.classicube.net/server/play/aaaaa/
			
			// Trim off the last / if it exists
			if( input[input.Length - 1] == '/' )
				input = input.Substring( 0, input.Length - 1 );
			
			// Trim the parts before the hash
			int lastIndex = input.LastIndexOf( '/' );
			if( lastIndex >= 0 )
				input = input.Substring( lastIndex + 1 );
			return input;
		}
		
		public override void Dispose() {
			base.Dispose();
			tableFont.Dispose();
			
			LauncherTableWidget table = widgets[tableIndex] as LauncherTableWidget;
			if( table != null ) {
				table.DraggingColumn = -1;
				table.DraggingScrollbar = false;
			}
		}
		
		bool pendingRedraw;
		public override void OnDisplay() {
			if( pendingRedraw ) RedrawTable();
			pendingRedraw = false;
		}
		
		void MarkPendingRedraw() {
			LauncherTableWidget table = (LauncherTableWidget)widgets[tableIndex];
			table.ClampIndex();
			table.RecalculateDrawData();
			pendingRedraw = true;
			Dirty = true;
		}
		
		void RedrawTable() {
			using( FastBitmap dst = new FastBitmap( game.Framebuffer, true, false ) )
				DrawTableBackground( dst );
			LauncherTableWidget table = (LauncherTableWidget)widgets[tableIndex];
			table.ClampIndex();
			
			int tableHeight = Math.Max( game.Height - tableY - 50, 1 );
			table.Height = tableHeight;
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				table.RedrawData( drawer );
			}
			Dirty = true;
		}
	}
}
