// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp;
using Launcher.Gui.Views;
using Launcher.Gui.Widgets;
using OpenTK.Input;

namespace Launcher.Gui.Screens {
	
	public sealed class ColoursScreen : LauncherInputScreen {
		
		ColoursView view;
		public ColoursScreen( LauncherWindow game ) : base( game, true ) {
			enterIndex = 6;
			view = new ColoursView( game );
			widgets = view.widgets;
		}

		public override void Init() {
			base.Init();
			view.Init();
			
			widgets[view.defIndex].OnClick = (x, y) => ResetColours();
			widgets[view.defIndex + 1].OnClick = (x, y) => game.SetScreen( new MainScreen( game ) );
			SetupInputHandlers();
			for( int i = 0; i < widgets.Length; i++ ) {
				LauncherInputWidget input = widgets[i] as LauncherInputWidget;
				if( input == null ) continue;
				input.TextChanged = TextChanged;
			}
			Resize();
		}

		public override void Resize() {
			view.DrawAll();
			Dirty = true;
		}
		
		public override void Dispose() {
			view.Dispose();
			base.Dispose();
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
			if( !Byte.TryParse( lastInput.Text, out col ) )  return;
			int newCol = col + delta;
			
			Utils.Clamp( ref newCol, 0, 255 );
			lastInput.Text = newCol.ToString();
			if( lastInput.CaretPos >= lastInput.Text.Length )
				lastInput.CaretPos = -1;
			TextChanged( lastInput );
		}
		
		void ResetColours() {
			LauncherSkin.ResetToDefault();
			view.MakeAllRGBTriplets( true );
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
			
			if( !changed ) return;
			game.MakeBackground();
			Resize();
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
