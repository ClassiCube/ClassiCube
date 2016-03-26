// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp;
using OpenTK.Input;

namespace Launcher {
	
	public sealed class ColoursScreen : LauncherInputScreen {
		
		public ColoursScreen( LauncherWindow game ) : base( game, true ) {
			enterIndex = 6;
			widgets = new LauncherWidget[25];
		}

		public override void Init() {
			base.Init();
			Resize();
		}

		public override void Resize() {
			MakeWidgets();
			RedrawAllButtonBackgrounds();
			
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				RedrawAll();
			}
			Dirty = true;
		}
		
		string GetCol( byte col, bool force ) {
			if( force )
				return col.ToString();
			LauncherWidget widget = widgets[widgetIndex];
			return widget == null ? col.ToString() : widget.Text;
		}
		
		void MakeWidgets() {
			widgetIndex = 0;
			MakeAllRGBTriplets( false );
			MakeLabelAt( "Background", inputFont, Anchor.Centre, Anchor.Centre, -60, -100 );
			MakeLabelAt( "Button border", inputFont, Anchor.Centre, Anchor.Centre, -65, -60 );
			MakeLabelAt( "Button highlight", inputFont, Anchor.Centre, Anchor.Centre, -75, -20 );
			MakeLabelAt( "Button foreground", inputFont, Anchor.Centre, Anchor.Centre, -85, 20 );
			MakeLabelAt( "Active button foreground", inputFont, Anchor.Centre, Anchor.Centre, -110, 60 );
			MakeLabelAt( "Red", titleFont, Anchor.Centre, Anchor.Centre, 30, -130 );
			MakeLabelAt( "Green", titleFont, Anchor.Centre, Anchor.Centre, 95, -130 );
			MakeLabelAt( "Blue", titleFont, Anchor.Centre, Anchor.Centre, 160, -130 );
			
			MakeButtonAt( "Default colours", 160, 35, titleFont, Anchor.Centre,
			             0, 120, (x, y) => ResetColours() );
			MakeButtonAt( "Back", 80, 35, titleFont, Anchor.Centre,
			             0, 170, (x, y) => game.SetScreen( new MainScreen( game ) ) );
			for( int i = 0; i < widgets.Length; i++ ) {
				LauncherInputWidget input = widgets[i] as LauncherInputWidget;
				if( input != null ) input.TextChanged = TextChanged;
			}
		}
		
		protected override void MouseWheelChanged( object sender, MouseWheelEventArgs e ) {
			AdjustSelectedColour( e.Delta );
		}
		
		protected override void KeyDown( object sender, KeyboardKeyEventArgs e ) {
			if( e.Key == Key.Left )
				AdjustSelectedColour( -1 );
			else if( e.Key == Key.Right)
				AdjustSelectedColour( +1 );
			else if( e.Key == Key.Up )
				AdjustSelectedColour( +10 );
			else if( e.Key == Key.Down )
				AdjustSelectedColour( -10 );
			else
				base.KeyDown( sender, e );
		}
		
		void AdjustSelectedColour( int delta ) {
			if( lastInput == null ) return;
			int index = Array.IndexOf<LauncherWidget>( widgets, lastInput );
			if( index >= 15 ) return;
			
			byte col;
			if( !Byte.TryParse( lastInput.Text, out col ) ) 
				return;
			int newCol = col + delta;
			
			Utils.Clamp( ref newCol, 0, 255 );
			lastInput.Text = newCol.ToString();
			if( lastInput.CaretPos >= lastInput.Text.Length )
				lastInput.CaretPos = -1;
			TextChanged( lastInput );
		}
		
		void MakeAllRGBTriplets( bool force ) {
			MakeRGBTriplet( LauncherSkin.BackgroundCol, force, -100 );
			MakeRGBTriplet( LauncherSkin.ButtonBorderCol, force, -60 );
			MakeRGBTriplet( LauncherSkin.ButtonHighlightCol, force, -20 );
			MakeRGBTriplet( LauncherSkin.ButtonForeCol, force, 20 );
			MakeRGBTriplet( LauncherSkin.ButtonForeActiveCol, force, 60 );
		}
		
		void MakeRGBTriplet( FastColour defCol, bool force, int y ) {
			MakeInput( GetCol( defCol.R, force ), 55, Anchor.Centre, false, 30, y, 3, null );
			MakeInput( GetCol( defCol.G, force ), 55, Anchor.Centre, false, 95, y, 3, null );
			MakeInput( GetCol( defCol.B, force ), 55, Anchor.Centre, false, 160, y, 3, null );
		}
		
		void ResetColours() {
			LauncherSkin.ResetToDefault();
			widgetIndex = 0;
			MakeAllRGBTriplets( true );
			
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );			
				RedrawAll();
			}
			widgetIndex = widgets.Length;
			game.MakeBackground();
			Resize();
		}
		
		void TextChanged( LauncherInputWidget widget ) {
			bool changed = false;
			int index = Array.IndexOf<LauncherWidget>( widgets, widget );
			if( index < 3 ) changed |= Parse( 0, ref LauncherSkin.BackgroundCol );
			else if( index < 6 ) changed |= Parse( 3, ref LauncherSkin.ButtonBorderCol );
			else if( index < 9 ) changed |= Parse( 6, ref LauncherSkin.ButtonHighlightCol );
			else if( index < 12 ) changed |= Parse( 9, ref LauncherSkin.ButtonForeCol );
			else if( index < 15 ) changed |= Parse( 12, ref LauncherSkin.ButtonForeActiveCol );
			
			if( changed ) {
				game.MakeBackground();
				Resize();
			}
		}
		
		bool Parse( int index, ref FastColour dst ) {
			byte r, g, b;
			if( !Byte.TryParse( widgets[index + 0].Text, out r )
			   || !Byte.TryParse( widgets[index + 1].Text, out g )
			   || !Byte.TryParse( widgets[index + 2].Text, out b ) )
				return false;
			dst.R = r; dst.G = g; dst.B = b;
			return true;
		}
	}
}
