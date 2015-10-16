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
			Window.Mouse.Move += new EventHandler<MouseMoveEventArgs>(Window_Mouse_Move);
		}

		void Window_Mouse_Move(object sender, MouseMoveEventArgs e) {
			//System.Diagnostics.Debug.Print( "moved" );
		}
		
		public void Display() {
			WinWindowInfo info = ((WinWindowInfo)Window.WindowInfo);
			IntPtr dc = info.DeviceContext;
			
			
			using( Graphics g = Graphics.FromHdc( dc ) ) {
				g.DrawImage( background, 0, 0, background.Width, background.Height );
			}
		}
		
		Bitmap background;
		public void RecreateBackground() {
			System.Diagnostics.Debug.Print( "DISPLAY" );
			if( background != null )
				background.Dispose();
			
			background = new Bitmap( Window.Width, Window.Height );
			Font logoFont = new Font( "Times New Roman", 28, FontStyle.Bold );
			Font logoItalicFont = new Font( "Times New Roman", 28, FontStyle.Italic );
			
			using( IDrawer2D drawer = Drawer ) {
				drawer.SetBitmap( background );
				drawer.Clear( Color.FromArgb( 30, 30, 30 ) );
				
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
			using( Font font = new Font( "Arial", 16, FontStyle.Bold ) ) {
				DrawText( drawer, "Direct connect", font, Anchor.Centre, Anchor.Centre,
				         buttonWidth, buttonHeight, 0, -100 );
				DrawText( drawer, "ClassiCube.net", font, Anchor.Centre, Anchor.Centre,
				         buttonWidth, buttonHeight, 0, -50 );
				DrawText( drawer, "Default texture pack", font, Anchor.Centre, Anchor.Centre,
				         buttonWidth, buttonHeight, 0, 50 );
				
				DrawText( drawer, "Singleplayer", font, Anchor.LeftOrTop, Anchor.BottomOrRight,
				         sideButtonWidth, buttonHeight, 10, -10 );
				DrawText( drawer, "Resume", font, Anchor.BottomOrRight, Anchor.BottomOrRight,
				         sideButtonWidth, buttonHeight, -10, -10 );
			}
		}
		
		Widget[] widgets = new Widget[5];
		int widgetIndex = 0;
		const int buttonWidth = 220, buttonHeight = 35, sideButtonWidth = 150;
		void DrawText( IDrawer2D drawer, string text, Font font, Anchor horAnchor,
		                     Anchor verAnchor, int width, int height, int x, int y ) {
			Size textSize = drawer.MeasureSize( text, font, true );
			int xOffset = width - textSize.Width;
			int yOffset = height - textSize.Height;
			
			if( horAnchor == Anchor.Centre ) x = x + Window.Width / 2 - width / 2;
			else if( horAnchor == Anchor.BottomOrRight ) x = x + Window.Width - width;
			if( verAnchor == Anchor.Centre ) y = y + Window.Height / 2 - height / 2;
			else if( verAnchor == Anchor.BottomOrRight ) y = y + Window.Height - height;
			
			drawer.DrawRoundedRect( shadowCol, 3, x + IDrawer2D.Offset, y + IDrawer2D.Offset,
			                       width, height );
			drawer.DrawRoundedRect( boxCol, 3, x, y, width, height );
			
			DrawTextArgs args = new DrawTextArgs( text, true );
			args.SkipPartsCheck = true;
			drawer.DrawText( font, ref args,
			                x + 1 + xOffset / 2, y + 1 + yOffset / 2 );
			Widget widget = new Widget();
			// adjust for border size of 2
			widget.X = x; widget.Y = y;
			widget.Width = width + 2; widget.Height = height + 2;
			//FilterButton( widget.X, widget.Y, widget.Width, widget.Height, 150 );
			widgets[widgetIndex++] = widget;
		}
		
		class Widget {
			public int X, Y;
			public int Width, Height;
			public bool Active;
		}
		
		void HandleResize( object sender, EventArgs e ) {
			RecreateBackground();
		}
		
		unsafe void FilterButton( int x, int y, int width, int height, byte scale ) {
			using( FastBitmap bmp = new FastBitmap( background, true ) ) {
				for( int yy = y; yy < y + height; yy++ ) {
					int* row = bmp.GetRowPtr( yy ) + x;
					for( int xx = 0; xx < width; xx++ ) {
						uint pixel = (uint)row[xx];
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
