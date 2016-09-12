// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp;
using Launcher.Gui.Views;
using Launcher.Gui.Widgets;
using OpenTK.Input;

namespace Launcher.Gui.Screens {
	
	public sealed class ServersScreen : LauncherInputScreen {
		
		const int tableX = 10, tableY = 50;
		ServersView view;
		
		public ServersScreen( LauncherWindow game ) : base( game, true ) {
			enterIndex = 3;
			view = new ServersView( game );
			widgets = view.widgets;
		}
		
		public override void Tick() {
			base.Tick();
			LauncherTableWidget table = (LauncherTableWidget)widgets[view.tableIndex];
			if( !game.Window.Mouse[MouseButton.Left] ) {
				table.DraggingColumn = -1;
				table.DraggingScrollbar = false;
				table.mouseOffset = 0;
			}
		}
		
		protected override void MouseMove( object sender, MouseMoveEventArgs e ) {
			base.MouseMove( sender, e );
			if( selectedWidget != null && selectedWidget == widgets[view.tableIndex] ) {
				LauncherTableWidget table = (LauncherTableWidget)widgets[view.tableIndex];
				table.MouseMove( e.X, e.Y, e.XDelta, e.YDelta );
			}
		}
		
		void MouseButtonUp( object sender, MouseButtonEventArgs e ) {
			LauncherTableWidget table = (LauncherTableWidget)widgets[view.tableIndex];
			table.DraggingColumn = -1;
			table.DraggingScrollbar = false;
			table.mouseOffset = 0;
		}
		
		protected override void OnAddedChar() { FilterList(); }
		
		protected override void OnRemovedChar() { FilterList(); }
		
		protected override void KeyDown( object sender, KeyboardKeyEventArgs e ) {
			LauncherTableWidget table = (LauncherTableWidget)widgets[view.tableIndex];
			if( e.Key == Key.Enter ) {
				string curServer = Get( view.hashIndex ) ?? "";
				if( table.Count >= 1 && curServer == "" ) {
					widgets[view.hashIndex].Text = table.usedEntries[0].Hash;
					ConnectToServer( 0, 0 );
				} else if( curServer != "" &&
				          (selectedWidget == null || selectedWidget == widgets[view.tableIndex]) ) {
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
			if( curInput != widgets[view.hashIndex] )
				return;
			LauncherTableWidget table = (LauncherTableWidget)widgets[view.tableIndex];
			table.SetSelected( widgets[view.hashIndex].Text );
			MarkPendingRedraw();
		}

		public override void Init() {
			base.Init();
			game.Window.Mouse.ButtonUp += MouseButtonUp;
			view.Init();
			SetupWidgetHandlers();
			
			Resize();
			selectedWidget = widgets[view.searchIndex];
			InputClick( 0, 0 );
			lastClicked = curInput;
		}
		
		public override void Resize() {
			view.DrawAll();
			game.Dirty = true;
		}
		
		void SetupWidgetHandlers() {
			LauncherInputWidget hashWidget = (LauncherInputWidget)widgets[view.hashIndex];
			hashWidget.Chars.ClipboardFilter = HashFilter;
			
			widgets[view.backIndex].OnClick =
				(x, y) => game.SetScreen( new MainScreen( game ) );
			widgets[view.connectIndex].OnClick = ConnectToServer;
			
			LauncherTableWidget widget = (LauncherTableWidget)widgets[view.tableIndex];
			widget.NeedRedraw = MarkPendingRedraw;
			widget.SelectedChanged = SelectedChanged;
			SetupInputHandlers();
		}
		
		void FilterList() {
			if( curInput != widgets[view.searchIndex] )
				return;
			LauncherTableWidget table = (LauncherTableWidget)widgets[view.tableIndex];
			table.FilterEntries( curInput.Text );
			MarkPendingRedraw();
		}

		void SelectedChanged( string hash ) {
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				Set( view.hashIndex, hash );
			}
			game.Dirty = true;
		}
		
		void ConnectToServer( int mouseX, int mouseY ) {
			LauncherTableWidget table = (LauncherTableWidget)widgets[view.tableIndex];
			game.ConnectToServer( table.servers, Get( view.hashIndex ) );
		}
		
		protected override void MouseWheelChanged( object sender, MouseWheelEventArgs e ) {
			LauncherTableWidget table = (LauncherTableWidget)widgets[view.tableIndex];
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
			view.Dispose();
			
			LauncherTableWidget table = widgets[view.tableIndex] as LauncherTableWidget;
			if( table != null ) {
				table.DraggingColumn = -1;
				table.DraggingScrollbar = false;
			}
		}
		
		bool pendingRedraw;
		public override void OnDisplay() {
			if( pendingRedraw ) {
				view.RedrawTable();
				game.Dirty = true;
			}
			pendingRedraw = false;
		}
		
		void MarkPendingRedraw() {
			LauncherTableWidget table = (LauncherTableWidget)widgets[view.tableIndex];
			table.ClampIndex();
			table.RecalculateDrawData();
			pendingRedraw = true;
			game.Dirty = true;
		}
	}
}
