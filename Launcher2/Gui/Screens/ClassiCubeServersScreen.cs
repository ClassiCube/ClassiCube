using System;
using System.Drawing;
using ClassicalSharp;
using OpenTK.Input;

namespace Launcher2 {
	
	public sealed class ClassiCubeServersScreen : LauncherInputScreen {
		
		const int tableIndex = 6;
		Font boldInputFont;
		public ClassiCubeServersScreen( LauncherWindow game ) : base( game ) {
			titleFont = new Font( "Arial", 16, FontStyle.Bold );
			inputFont = new Font( "Arial", 13, FontStyle.Regular );
			boldInputFont = new Font( "Arial", 13, FontStyle.Bold );
			enterIndex = 5;
			widgets = new LauncherWidget[7];
		}
		
		public override void Tick() {
			LauncherTableWidget table = (LauncherTableWidget)widgets[tableIndex];
			if( table.DraggingWidth && !game.Window.Mouse[MouseButton.Left] )
			   table.DraggingWidth = false;
		}
		
		protected override void MouseMove( object sender, MouseMoveEventArgs e ) {
			base.MouseMove( sender, e );
			if( selectedWidget != null && selectedWidget == widgets[tableIndex] ) {
				LauncherTableWidget table = (LauncherTableWidget)widgets[tableIndex];
				table.MouseMove( e.XDelta, e.YDelta );
			}
		}
		
		void MouseButtonUp( object sender, MouseButtonEventArgs e ) {
			LauncherTableWidget table = (LauncherTableWidget)widgets[tableIndex];
			table.DraggingWidth = false;
		}
		
		protected override void OnAddedChar() { FilterList(); }
		
		protected override void OnRemovedChar() { FilterList(); }
		
		protected override void KeyDown( object sender, KeyboardKeyEventArgs e ) {
			if( e.Key == Key.Enter ) {
				LauncherTableWidget table = (LauncherTableWidget)widgets[tableIndex];
				if( table.Count == 1 && String.IsNullOrEmpty( Get( 3 ) ) )
					widgets[3].Text = table.usedEntries[0].Hash;
				ConnectToServer( 0, 0 );
			} else {
				base.KeyDown( sender, e );
			}
		}
		
		void FilterList() {
			if( lastInput == widgets[1] ) {
				LauncherTableWidget table = (LauncherTableWidget)widgets[tableIndex];
				table.FilterEntries( lastInput.Text );
				table.ClampIndex();
				Resize();
			}
		}
		
		protected override void RedrawLastInput() {
			base.RedrawLastInput();
			if( lastInput == widgets[3] ) {
				LauncherTableWidget table = (LauncherTableWidget)widgets[tableIndex];
				table.SelectedHash = widgets[3].Text;
				Resize();
			}
		}

		public override void Init() {
			base.Init();
			game.Window.Mouse.WheelChanged += MouseWheelChanged;
			game.Window.Mouse.ButtonUp += MouseButtonUp;
			
			Resize();
			selectedWidget = widgets[1];
			InputClick( 0, 0 );		
		}
		
		public override void Resize() {
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				game.ClearArea( 0, 0, game.Width, 100 );
				drawer.Clear( game.clearColour, 0, 100,
				             game.Width, game.Height - 100 );
				
				Draw();
				LauncherTableWidget table = (LauncherTableWidget)widgets[tableIndex];
				table.ClampIndex();
				table.Redraw( drawer, inputFont, titleFont, boldInputFont );
			}
			Dirty = true;
		}
		
		void Draw() {
			widgetIndex = 0;
			
			MakeLabelAt( "Search", titleFont, Anchor.Centre, Anchor.LeftOrTop, -190, 10 );
			MakeInput( Get(), 270, Anchor.LeftOrTop, false, -5, 5, 32 );
			
			MakeLabelAt( "/play/", inputFont, Anchor.Centre, Anchor.LeftOrTop, -215, 55 );
			MakeInput( Get(), 310, Anchor.LeftOrTop, false, -35, 50, 32 );
			((LauncherInputWidget)widgets[3]).ClipboardFilter = HashFilter;
			
			MakeButtonAt( "Back", 70, 30, titleFont, Anchor.LeftOrTop,
			             195, 5, (x, y) => game.SetScreen( new ClassiCubeScreen( game ) ) );
			MakeButtonAt( "Connect", 100, 30, titleFont, Anchor.LeftOrTop,
			             180, 50, ConnectToServer );			
			MakeTableWidget();
		}
		
		void MakeTableWidget() {
			if( widgets[tableIndex] != null ) {
				LauncherTableWidget table = (LauncherTableWidget)widgets[tableIndex];
				table.Redraw( drawer, inputFont, titleFont, boldInputFont );
				return;
			}
			
			LauncherTableWidget widget = new LauncherTableWidget( game );
			widget.CurrentIndex = 0;
			widget.SetEntries( game.Session.Servers );
			widget.DrawAt( drawer, inputFont, titleFont, boldInputFont,
			              Anchor.LeftOrTop, Anchor.LeftOrTop, 0, 100 );
			
			widget.NeedRedraw = Resize;
			widget.SelectedChanged = SelectedChanged;
			widgets[widgetIndex++] = widget;
		}

		void SelectedChanged( string hash ) {
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				Set( 3, hash );
			}
			Dirty = true;
		}
		
		void ConnectToServer( int mouseX, int mouseY ) {
			LauncherTableWidget table = (LauncherTableWidget)widgets[tableIndex];
			game.ConnectToServer( table.servers, Get( 3 ) );
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
			game.Window.Mouse.WheelChanged -= MouseWheelChanged;
		}
	}
}
