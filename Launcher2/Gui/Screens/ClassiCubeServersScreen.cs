using System;
using System.Drawing;
using ClassicalSharp;
using OpenTK.Input;

namespace Launcher2 {
	
	public sealed class ClassiCubeServersScreen : LauncherInputScreen {
		
		const int tableIndex = 4, searchIndex = 0, hashIndex = 1;
		Font boldInputFont, tableFont;
		public ClassiCubeServersScreen( LauncherWindow game ) : base( game, true ) {
			boldInputFont = new Font( "Arial", 14, FontStyle.Bold );		
			tableFont = new Font( "Arial", 12, FontStyle.Regular );
			enterIndex = 3;
			widgets = new LauncherWidget[7];
		}
		
		public override void Tick() {
			base.Tick();
			LauncherTableWidget table = (LauncherTableWidget)widgets[tableIndex];
			if( !game.Window.Mouse[MouseButton.Left] ) {
				table.DraggingColumn = -1;
				table.DraggingScrollbar = false;
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
		}
		
		protected override void OnAddedChar() { FilterList(); }
		
		protected override void OnRemovedChar() { FilterList(); }
		
		protected override void KeyDown( object sender, KeyboardKeyEventArgs e ) {
			LauncherTableWidget table = (LauncherTableWidget)widgets[tableIndex];
			if( e.Key == Key.Enter ) {
				if( table.Count == 1 && String.IsNullOrEmpty( Get( hashIndex ) ) ) {
					widgets[hashIndex].Text = table.usedEntries[0].Hash;
					ConnectToServer( 0, 0 );
				} else {
					base.KeyDown( sender, e );
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
			Resize();
		}

		public override void Init() {
			base.Init();
			game.Window.Mouse.WheelChanged += MouseWheelChanged;
			game.Window.Mouse.ButtonUp += MouseButtonUp;
			
			Resize();
			selectedWidget = widgets[searchIndex];
			InputClick( 0, 0 );
		}
		
		public override void Resize() {
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				game.ClearArea( 0, 0, game.Width, 100 );
				FastColour col = LauncherTableWidget.backGridCol;
				drawer.Clear( col, 0, 100, game.Width, game.Height - 100 );
				
				Draw();
				LauncherTableWidget table = (LauncherTableWidget)widgets[tableIndex];
				table.ClampIndex();
				table.Redraw( drawer, tableFont, inputFont, inputFont );
			}
			Dirty = true;
		}
		
		void Draw() {
			widgetIndex = 0;
			MakeInput( Get(), 475, Anchor.LeftOrTop, Anchor.LeftOrTop, 
			          false, 10, 10, 32, "&7Search servers.." );
			MakeInput( Get(), 475, Anchor.LeftOrTop, Anchor.LeftOrTop, 
			          false, 10, 55, 32, "&7classicube.net/server/play/..." );
			((LauncherInputWidget)widgets[hashIndex]).ClipboardFilter = HashFilter;
			
			MakeButtonAt( "Back", 110, 30, titleFont, Anchor.BottomOrRight, Anchor.LeftOrTop,
			             -20, 10, (x, y) => game.SetScreen( new MainScreen( game ) ) );
			MakeButtonAt( "Connect", 110, 35, titleFont, Anchor.BottomOrRight, Anchor.LeftOrTop,
			             -20, 55, ConnectToServer );
			MakeTableWidget();
		}
		
		void MakeTableWidget() {
			if( widgets[tableIndex] != null ) {
				LauncherTableWidget table = (LauncherTableWidget)widgets[tableIndex];
				table.Redraw( drawer, tableFont, inputFont, inputFont );
				return;
			}
			
			LauncherTableWidget widget = new LauncherTableWidget( game );
			widget.CurrentIndex = 0;
			widget.SetEntries( game.Session.Servers );
			widget.DrawAt( drawer, tableFont, inputFont, inputFont,
			              Anchor.LeftOrTop, Anchor.LeftOrTop, 0, 100 );
			
			widget.NeedRedraw = Resize;
			widget.SelectedChanged = SelectedChanged;
			widgets[widgetIndex++] = widget;
		}
		
		void FilterList() {
			if( lastInput != widgets[searchIndex] )
				return;
			LauncherTableWidget table = (LauncherTableWidget)widgets[tableIndex];
			table.FilterEntries( lastInput.Text );
			table.ClampIndex();
			Resize();
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
		
		void MouseWheelChanged( object sender, MouseWheelEventArgs e ) {
			LauncherTableWidget table = (LauncherTableWidget)widgets[tableIndex];
			table.CurrentIndex -= e.Delta;
			table.ClampIndex();
			Resize();
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
			boldInputFont.Dispose();
			tableFont.Dispose();
			game.Window.Mouse.WheelChanged -= MouseWheelChanged;
			
			LauncherTableWidget table = widgets[tableIndex] as LauncherTableWidget;
			if( table != null ) {
				table.DraggingColumn = -1;
				table.DraggingScrollbar = false;
			}
		}
	}
}
