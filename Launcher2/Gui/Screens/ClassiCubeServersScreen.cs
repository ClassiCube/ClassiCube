using System;
using System.Collections.Generic;
using System.Drawing;
using System.Net;
using ClassicalSharp;
using OpenTK.Input;

namespace Launcher2 {
	
	public sealed class ClassiCubeServersScreen : LauncherInputScreen {
		
		const int tableIndex = 6;
		public ClassiCubeServersScreen( LauncherWindow game ) : base( game ) {
			titleFont = new Font( "Arial", 16, FontStyle.Bold );
			inputFont = new Font( "Arial", 13, FontStyle.Regular );
			enterIndex = 4;	
			widgets = new LauncherWidget[7];		
		}
		
		public override void Tick() {
		}
		
		protected override void MouseMove( object sender, MouseMoveEventArgs e ) {
			base.MouseMove( sender, e );
			if( selectedWidget != null && selectedWidget == widgets[tableIndex] ) {
				LauncherTableWidget table = widgets[tableIndex] as LauncherTableWidget;
				table.MouseMove( e.XDelta, e.YDelta );
			}
		}
		
		void MouseButtonUp( object sender, MouseButtonEventArgs e ) {
			LauncherTableWidget table = widgets[tableIndex] as LauncherTableWidget;
			table.DraggingWidth = false;
		}
		
		protected override void OnAddedChar() { FilterList(); }
		
		protected override void OnRemovedChar() { FilterList(); }
		
		void FilterList() {
			if( lastInput == widgets[1] ) {
				LauncherTableWidget table = widgets[tableIndex] as LauncherTableWidget;
				table.FilterEntries( lastInput.Text );
				ClampIndex();
				Resize();
			}
		}

		public override void Init() { 
			base.Init();
			game.Window.Mouse.WheelChanged += MouseWheelChanged;
			game.Window.Mouse.ButtonUp += MouseButtonUp;
			Resize(); 
		}
		
		public override void Resize() {
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				drawer.Clear( LauncherWindow.clearColour );
				Draw();
			}
			Dirty = true;
		}
		
		void Draw() {
			widgetIndex = 0;
			int lastIndex = GetLastInputIndex();
			
			MakeTextAt( titleFont, "Search", -200, 10 );
			MakeTextInputAt( Get( widgetIndex ), 270, -25, 5 );
			
			MakeTextAt( inputFont, "../play/", -210, 55 );
			MakeTextInputAt( Get( 3 ), 320, -20, 50 );
			
			MakeButtonAt( "Connect", 100, 30, 180, 5, ConnectToServer );
			MakeButtonAt( "Back", 70, 30, 195, 50,
			             (x, y) => game.SetScreen( new MainScreen( game ) ) );
			MakeTableWidget();
			
			if( lastIndex >= 0 )
				lastInput = widgets[lastIndex] as LauncherTextInputWidget;
		}
		
		int GetLastInputIndex() {
			return lastInput == null ? -1 :
				Array.IndexOf<LauncherWidget>( widgets, lastInput );
		}

		void MakeTextAt( Font font, string text, int x, int y ) {
			LauncherTextWidget widget = new LauncherTextWidget( game, text );
			widget.DrawAt( drawer, text, font, Anchor.Centre, Anchor.LeftOrTop, x, y );
			widgets[widgetIndex++] = widget;
		}
		
		void MakeTextInputAt( string text, int width, int x, int y ) {
			LauncherTextInputWidget widget = new LauncherTextInputWidget( game );
			widget.OnClick = InputClick;
			
			widget.DrawAt( drawer, text, inputFont, Anchor.Centre, Anchor.LeftOrTop, width, 30, x, y );
			widgets[widgetIndex++] = widget;
		}
		
		void MakeButtonAt( string text, int width, int height,
		                  int x, int y, Action<int, int> onClick ) {
			LauncherButtonWidget widget = new LauncherButtonWidget( game );
			widget.Text = text;
			widget.OnClick = onClick;
			
			widget.Active = false;
			widget.DrawAt( drawer, text, titleFont, Anchor.Centre, Anchor.LeftOrTop, width, height, x, y );
			widgets[widgetIndex++] = widget;
		}
		
		void MakeTableWidget() {
			if( widgets[tableIndex] != null ) {
				LauncherTableWidget table = widgets[tableIndex] as LauncherTableWidget;
				table.Redraw( drawer, inputFont, titleFont );
				return;
			}
			
			LauncherTableWidget widget = new LauncherTableWidget( game );
			widget.CurrentIndex = 0;
			widget.SetEntries( game.Session.Servers );
			widget.DrawAt( drawer, inputFont, titleFont, Anchor.LeftOrTop, Anchor.LeftOrTop, 0, 100 );
			
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
			game.ConnectToServer( Get( 3 ) );
		}
		
		void MouseWheelChanged( object sender, MouseWheelEventArgs e ) {
			LauncherTableWidget table = widgets[tableIndex] as LauncherTableWidget;
			table.CurrentIndex -= e.Delta;
			ClampIndex();
			Resize();
		}
		
		void ClampIndex() {
			LauncherTableWidget table = widgets[tableIndex] as LauncherTableWidget;
			if( table.CurrentIndex >= table.Count )
				table.CurrentIndex = table.Count - 1;
			if( table.CurrentIndex < 0 )
				table.CurrentIndex = 0;
		}
		
		public override void Dispose() {
			base.Dispose();
			game.Window.Mouse.WheelChanged -= MouseWheelChanged;
		}
	}
}
