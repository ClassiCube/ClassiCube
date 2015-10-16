using System;
using System.Drawing;
using ClassicalSharp;
using OpenTK;
using OpenTK.Graphics;
using OpenTK.Platform.Windows;
using OpenTK.Platform;
using OpenTK.Input;

namespace Launcher2 {
	
	public sealed class MainScreen {
		public IDrawer2D Drawer;
		public NativeWindow Window;
		
		public void Init() {
			Window.Resize += HandleResize;
			Window.Mouse.Move += MouseMove;
			Window.Mouse.ButtonDown += MouseButtonDown;
			
			logoFont = new Font( "Times New Roman", 28, FontStyle.Bold );
			logoItalicFont = new Font( "Times New Roman", 28, FontStyle.Italic );
			textFont = new Font( "Arial", 16, FontStyle.Bold );
		}

		FastButtonWidget selectedWidget;
		void MouseMove( object sender, MouseMoveEventArgs e ) {
			//System.Diagnostics.Debug.Print( "moved" );
			for( int i = 0; i < widgets.Length; i++ ) {
				FastButtonWidget widget = widgets[i];
				if( e.X >= widget.X && e.Y >= widget.Y &&
				   e.X < widget.X + widget.Width && e.Y < widget.Y + widget.Height ) {
					if( selectedWidget != widget && selectedWidget != null ) {
						using( IDrawer2D drawer = Drawer ) {
							drawer.SetBitmap( background );
							selectedWidget.Redraw( Drawer, selectedWidget.Text, textFont );
							FilterButton( selectedWidget.X, selectedWidget.Y,
							             selectedWidget.Width, selectedWidget.Height, 180 );
							widget.Redraw( Drawer, widget.Text, textFont );
						}
					}
					selectedWidget = widget;
					break;
				}
			}
		}
		
		void MouseButtonDown( object sender, MouseButtonEventArgs e ) {
			if( e.Button != MouseButton.Left ) return;
			
			if( selectedWidget != null && selectedWidget.OnClick != null )
				selectedWidget.OnClick();
		}
		
		public void Display() {
			WinWindowInfo info = ((WinWindowInfo)Window.WindowInfo);
			IntPtr dc = info.DeviceContext;
			
			
			using( Graphics g = Graphics.FromHdc( dc ) ) {
				g.DrawImage( background, 0, 0, background.Width, background.Height );
			}
		}
		
		Bitmap background;
		Font textFont, logoFont, logoItalicFont;
		static FastColour clearColour = new FastColour( 30, 30, 30 );
		static uint clearColourBGRA = (uint)(new FastColour( 30, 30, 30 ).ToArgb());
		public void RecreateBackground() {
			System.Diagnostics.Debug.Print( "DISPLAY" );
			if( background != null )
				background.Dispose();
			
			background = new Bitmap( Window.Width, Window.Height );
			using( IDrawer2D drawer = Drawer ) {
				drawer.SetBitmap( background );
				drawer.Clear( clearColour );
				
				Size size1 = drawer.MeasureSize( "&eClassical", logoItalicFont, true );
				Size size2 = drawer.MeasureSize( "&eSharp", logoFont, true );
				int xStart = Window.Width / 2 - (size1.Width + size2.Width ) / 2;
				
				DrawTextArgs args = new DrawTextArgs( "&eClassical", true );
				drawer.DrawText( logoItalicFont, ref args, xStart, 20 );
				args.Text = "&eSharp";
				drawer.DrawText( logoFont, ref args, xStart + size1.Width, 20 );
				DrawButtons( drawer );
			}
			
			logoFont.Dispose();
			logoItalicFont.Dispose();
		}
		
		static FastColour boxCol = new FastColour( 169, 143, 192 ), shadowCol = new FastColour( 97, 81, 110 );
		void DrawButtons( IDrawer2D drawer ) {
			widgetIndex = 0;
			MakeButtonAt( drawer, "Direct connect", Anchor.Centre, Anchor.Centre,
			             buttonWidth, buttonHeight, 0, -100 );
			MakeButtonAt( drawer, "ClassiCube.net", Anchor.Centre, Anchor.Centre,
			             buttonWidth, buttonHeight, 0, -50 );
			MakeButtonAt( drawer, "Default texture pack", Anchor.Centre, Anchor.Centre,
			             buttonWidth, buttonHeight, 0, 50 );
			
			MakeButtonAt( drawer, "Singleplayer", Anchor.LeftOrTop, Anchor.BottomOrRight,
			             sideButtonWidth, buttonHeight, 10, -10 );
			widgets[widgetIndex - 1].OnClick
				= () => Program.StartClient( "default.zip" );
			MakeButtonAt( drawer, "Resume", Anchor.BottomOrRight, Anchor.BottomOrRight,
			             sideButtonWidth, buttonHeight, -10, -10 );
		}
		
		FastButtonWidget[] widgets = new FastButtonWidget[5];
		int widgetIndex = 0;
		const int buttonWidth = 220, buttonHeight = 35, sideButtonWidth = 150;
		void MakeButtonAt( IDrawer2D drawer, string text, Anchor horAnchor,
		                  Anchor verAnchor, int width, int height, int x, int y ) {
			FastButtonWidget widget = new FastButtonWidget();
			widget.Window = Window;
			widget.Text = text;
			widget.DrawAt( drawer, text, textFont, horAnchor, verAnchor, width, height, x, y );
			FilterButton( widget.X, widget.Y, widget.Width, widget.Height, 180 );
			widgets[widgetIndex++] = widget;
		}
		
		void HandleResize( object sender, EventArgs e ) {
			RecreateBackground();
		}
		
		public void Dispose() {
			logoFont.Dispose();
			logoItalicFont.Dispose();
			textFont.Dispose();
		}
		
		unsafe void FilterButton( int x, int y, int width, int height, byte scale ) {
			using( FastBitmap bmp = new FastBitmap( background, true ) ) {
				for( int yy = y; yy < y + height; yy++ ) {
					int* row = bmp.GetRowPtr( yy ) + x;
					for( int xx = 0; xx < width; xx++ ) {
						uint pixel = (uint)row[xx];
						if( pixel == clearColourBGRA ) continue;
						uint a = pixel & 0xFF000000;
						uint r = (pixel >> 16) & 0xFF;
						uint g = (pixel >> 8) & 0xFF;
						uint b = pixel & 0xFF;
						
						r = (r * scale) / 255;
						g = (g * scale) / 255;
						b = (b * scale) / 255;
						row[xx] = (int)(a | (r << 16) | (g << 8) | b);
					}
				}
			}
		}
	}
}
