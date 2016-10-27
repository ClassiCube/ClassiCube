// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp.Gui.Widgets;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	public abstract class KeyBindingsScreen : MenuScreen {
		
		public KeyBindingsScreen( Game game ) : base( game ) { }
		
		Font keyFont;
		static string[] keyNames;
		protected string[] leftDesc, rightDesc;
		protected KeyBind[] left, right;
		
		protected int btnDistance = 45, btnWidth = 260, btnHeight = 35;
		protected string title = "Controls";
		protected int index;
		protected Action<Game, Widget> leftPage, rightPage;
		
		public override void Init() {
			titleFont = new Font( game.FontName, 16, FontStyle.Bold );
			keyFont = new Font( game.FontName, 16, FontStyle.Bold );
			regularFont = new Font( game.FontName, 16, FontStyle.Italic );
			
			if( keyNames == null )
				keyNames = Enum.GetNames( typeof( Key ) );
		}

		protected void MakeWidgets( int y ) {
			int origin = y;
			MakeOthers();
			
			if( right == null ) {
				for( int i = 0; i < left.Length; i++ )
					Make( i, 0, ref y );
			} else {
				for( int i = 0; i < left.Length; i++ )
					Make( i, -btnWidth / 2 - 5, ref y );
				
				y = origin;
				for( int i = 0; i < right.Length; i++ )
					Make( i + left.Length, btnWidth / 2 + 5, ref y );
			}
			MakePages( origin );
		}
		
		void MakeOthers() {
			widgets[index++] = ChatTextWidget.Create( game, 0, -180, title,
			                                         Anchor.Centre, Anchor.Centre, keyFont );
			if( game.ClassicMode ) {
				widgets[index++] = MakeBack( false, titleFont,
				                            (g, w) => g.Gui.SetNewScreen( new ClassicOptionsScreen( g ) ) );
			} else {
				widgets[index++] = MakeBack( "Back to menu", 5, titleFont,
				                            (g, w) => g.Gui.SetNewScreen( new OptionsGroupScreen( g ) ) );
			}
		}
		
		void MakePages( int origin ) {
			if( leftPage == null && rightPage == null ) return;
			int btnY = origin + btnDistance * (left.Length / 2);
			widgets[index++] = ButtonWidget.Create( game, -btnWidth - 35, btnY, btnHeight, btnHeight, "<",
			                                       Anchor.Centre, Anchor.Centre, keyFont, LeftOnly( leftPage ) );
			widgets[index++] = ButtonWidget.Create( game, btnWidth + 35, btnY, btnHeight, btnHeight, ">",
			                                       Anchor.Centre, Anchor.Centre, keyFont, LeftOnly( rightPage ) );
			if( leftPage == null ) widgets[index - 2].Disabled = true;
			if( rightPage == null ) widgets[index - 1].Disabled = true;
		}
		
		void Make( int i, int x, ref int y ) {
			string text = ButtonText( i );
			widgets[index++] = ButtonWidget.Create( game, x, y, btnWidth, btnHeight, text,
			                                       Anchor.Centre, Anchor.Centre, keyFont, OnBindingClick );
			y += btnDistance;
		}
		
		
		ButtonWidget curWidget;
		void OnBindingClick( Game game, Widget widget, MouseButton mouseBtn ) {
			int index = 0;
			if( mouseBtn == MouseButton.Right && (curWidget == null || curWidget == widget) ) {
				curWidget = (ButtonWidget)widget;
				index = Array.IndexOf<Widget>( widgets, curWidget ) - 2;
				KeyBind mapping = Get( index, left, right );
				HandlesKeyDown( game.InputHandler.Keys.GetDefault( mapping ) );
			}
			if( mouseBtn != MouseButton.Left ) return;
			
			if( curWidget != null ) {
				index = Array.IndexOf<Widget>( widgets, curWidget ) - 2;
				curWidget.SetText( ButtonText( index ) );
				curWidget = null;
			}
			
			index = Array.IndexOf<Widget>( widgets, widget ) - 2;
			string text = ButtonText( index );
			curWidget = (ButtonWidget)widget;
			curWidget.SetText( "> " + text + " <" );
		}
		
		string ButtonText( int i ) {
			KeyBind mapping = Get( i, left, right );
			Key key = game.InputHandler.Keys[mapping];
			string desc = Get( i, leftDesc, rightDesc );
			return desc + ": " + keyNames[(int)key];
		}
		
		public override bool HandlesKeyDown( Key key ) {
			if( key == Key.Escape ) {
				game.Gui.SetNewScreen( null );
			} else if( curWidget != null ) {
				int index = Array.IndexOf<Widget>( widgets, curWidget ) - 2;
				KeyBind mapping = Get( index, left, right );
				game.InputHandler.Keys[mapping] = key;			
				curWidget.SetText( ButtonText( index ) );
				curWidget = null;
			}
			return true;
		}
		
		T Get<T>( int index, T[] a, T[] b ) {
			return index < a.Length ? a[index] : b[index - a.Length];
		}
		
		public override void Dispose() {
			keyFont.Dispose();
			base.Dispose();
		}
	}
}