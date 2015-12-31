using System;
using System.Drawing;
using ClassicalSharp;

namespace Launcher2 {
	
	public sealed class ColoursScreen : LauncherInputScreen {
		
		public ColoursScreen( LauncherWindow game ) : base( game, true ) {
			enterIndex = 6;
			widgets = new LauncherWidget[12];
		}

		public override void Init() {
			base.Init();
			Resize();
		}

		public override void Resize() {
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				Draw();
			}
			Dirty = true;
		}
		
		string GetCol( FastColour defCol ) {
			LauncherWidget widget = widgets[widgetIndex];
			return widget == null ? defCol.ToRGBHexString() : widget.Text;
		}
		
		void Draw() {
			widgetIndex = 0;
			MakeLabelAt( "Background", titleFont, Anchor.Centre, Anchor.Centre, -70, -120 );
			MakeInput( GetCol( LauncherSkin.BackgroundCol ), 90, Anchor.Centre, false, 45, -120, 6, null );
			
			MakeLabelAt( "Button border", titleFont, Anchor.Centre, Anchor.Centre, -80, -80 );
			MakeInput( GetCol( LauncherSkin.ButtonBorderCol ), 90, Anchor.Centre, false, 45, -80, 6, null );
			
			MakeLabelAt( "Button highlight", titleFont, Anchor.Centre, Anchor.Centre, -90, -40 );
			MakeInput( GetCol( LauncherSkin.ButtonHighlightCol ), 90, Anchor.Centre, false, 45, -40, 6, null );
			
			MakeLabelAt( "Button foreground", titleFont, Anchor.Centre, Anchor.Centre, -100, 0 );
			MakeInput( GetCol( LauncherSkin.ButtonForeCol ), 90, Anchor.Centre, false, 45, 2, 6, null );
			
			MakeLabelAt( "Active button foreground", titleFont, Anchor.Centre, Anchor.Centre, -130, 40 );
			MakeInput( GetCol( LauncherSkin.ButtonForeActiveCol ), 90, Anchor.Centre, false, 45, 40, 6, null );
			
			MakeButtonAt( "Default colours", 160, 35, titleFont, Anchor.Centre,
			             0, 100, (x, y) => ResetColours() );
			MakeButtonAt( "Back", 80, 35, titleFont, Anchor.Centre,
			             0, 170, (x, y) => game.SetScreen( new MainScreen( game ) ) );
			for( int i = 0; i < widgets.Length; i++ ) {
				LauncherInputWidget input = widgets[i] as LauncherInputWidget;
				if( input != null ) input.TextChanged = TextChanged;
			}
		}
		
		void ResetColours() {
			LauncherSkin.ResetToDefault();
			widgets[1].Text = LauncherSkin.BackgroundCol.ToRGBHexString();
			widgets[3].Text = LauncherSkin.ButtonBorderCol.ToRGBHexString();
			widgets[5].Text =  LauncherSkin.ButtonHighlightCol.ToRGBHexString();
			widgets[7].Text = LauncherSkin.ButtonForeCol.ToRGBHexString();
			widgets[9].Text = LauncherSkin.ButtonForeActiveCol.ToRGBHexString();
			
			game.MakeBackground();
			Resize();
		}
		
		void TextChanged( LauncherInputWidget widget ) {
			bool changed = false;
			if( widget == widgets[1] ) changed |= Parse( widget, ref LauncherSkin.BackgroundCol );
			else if( widget == widgets[3] ) changed |= Parse( widget, ref LauncherSkin.ButtonBorderCol );
			else if( widget == widgets[5] ) changed |= Parse( widget, ref LauncherSkin.ButtonHighlightCol );
			else if( widget == widgets[7] ) changed |= Parse( widget, ref LauncherSkin.ButtonForeCol );
			else if( widget == widgets[9] ) changed |= Parse( widget, ref LauncherSkin.ButtonForeActiveCol );
			
			if( changed ) {
				game.MakeBackground();
				Resize();
			}
		}
		
		bool Parse( LauncherInputWidget widget, ref FastColour dst ) {
			FastColour col;
			if( !FastColour.TryParse( widget.Text, out col ) )
				return false;
			dst = col;
			return true;
		}
	}
}
