// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using OpenTK.Input;

namespace ClassicalSharp.Gui {
	
	public abstract class KeyBindingsScreen : MenuScreen {
		
		public KeyBindingsScreen( Game game ) : base( game ) { }
		
		public override void Render( double delta ) {
			RenderMenuBounds();
			api.Texturing = true;
			RenderMenuWidgets( delta );
			statusWidget.Render( delta );
			api.Texturing = false;
		}
		
		Font keyFont;
		TextWidget statusWidget;
		static string[] keyNames;
		protected string[] leftDesc, rightDesc;
		protected KeyBind[] left, right;
		
		protected int btnDistance = 45, btnWidth = 260, btnHeight = 35;
		protected string title = "Controls";
		protected int index;
		protected Action<Game, Widget> leftPage, rightPage;
		
		public override void Init() {
			base.Init();
			if( keyNames == null )
				keyNames = Enum.GetNames( typeof( Key ) );
			keyFont = new Font( game.FontName, 16, FontStyle.Bold );
			regularFont = new Font( game.FontName, 16, FontStyle.Italic );
			statusWidget = ChatTextWidget.Create( game, 0, 130, "",
			                                     Anchor.Centre, Anchor.Centre, regularFont );
		}

		protected void MakeWidgets( int y ) {
			int origin = y;
			MakeOthers();
			
			if( right == null ) {
				for( int i = 0; i < left.Length; i++ )
					Make( leftDesc[i], left[i], 0, ref y );
			} else {
				for( int i = 0; i < left.Length; i++ )
					Make( leftDesc[i], left[i], -btnWidth / 2 - 5, ref y );
				
				y = origin;
				for( int i = 0; i < right.Length; i++ )
					Make( rightDesc[i], right[i], btnWidth / 2 + 5, ref y );
			}
			MakePages( origin );
		}
		
		void MakeOthers() {
			widgets[index++] = ChatTextWidget.Create( game, 0, -180, title,
			                                         Anchor.Centre, Anchor.Centre, keyFont );
			if( game.ClassicMode ) {
				widgets[index++] = MakeBack( false, titleFont,
				                            (g, w) => g.SetNewScreen( new ClassicOptionsScreen( g ) ) );
			} else {
				widgets[index++] = MakeBack( "Back to menu", 5, titleFont,
				                            (g, w) => g.SetNewScreen( new OptionsGroupScreen( g ) ) );
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
		
		void Make( string desc, KeyBind bind, int x, ref int y ) {
			string text = desc + ": " + keyNames[(int)game.Mapping( bind )];
			widgets[index++] = ButtonWidget.Create( game, x, y, btnWidth, btnHeight, text,
			                                       Anchor.Centre, Anchor.Centre, keyFont, OnBindingClick );
			y += btnDistance;
		}
		
		
		ButtonWidget curWidget;
		void OnBindingClick( Game game, Widget widget, MouseButton mouseBtn ) {
			if( mouseBtn == MouseButton.Right && (curWidget == null || curWidget == widget) ) {
				curWidget = (ButtonWidget)widget;
				int index = Array.IndexOf<Widget>( widgets, curWidget ) - 2;
				KeyBind mapping = Get( index, left, right );
				HandlesKeyDown( game.InputHandler.Keys.GetDefault( mapping ) );
			}
			if( mouseBtn != MouseButton.Left ) return;
			
			if( curWidget == widget ) {
				curWidget = null;
				statusWidget.SetText( "" );
			} else {
				curWidget = (ButtonWidget)widget;
				int index = Array.IndexOf<Widget>( widgets, curWidget ) - 2;
				string desc = Get( index, leftDesc, rightDesc );
				string text = "&ePress new key binding for " + desc + ":";
				statusWidget.SetText( text );
			}
		}
		
		public override bool HandlesKeyDown( Key key ) {
			if( key == Key.Escape ) {
				game.SetNewScreen( null );
			} else if( curWidget != null ) {
				int index = Array.IndexOf<Widget>( widgets, curWidget ) - 2;
				KeyBind mapping = Get( index, left, right );
				KeyMap map = game.InputHandler.Keys;
				Key oldKey = map[mapping];
				string reason;
				string desc = Get( index, leftDesc, rightDesc );
				
				if( !map.IsKeyOkay( oldKey, key, out reason ) ) {
					const string format = "&eFailed to change \"{0}\". &c({1})";
					statusWidget.SetText( String.Format( format, desc, reason ) );
				} else {
					const string format = "&e\"{0}\" changed from &7{1} &eto &7{2}&e.";
					statusWidget.SetText( String.Format( format, desc, oldKey, key ) );
					string text = desc + ": " + keyNames[(int)key];
					
					curWidget.SetText( text );
					map[mapping] = key;
				}
				curWidget = null;
			}
			return key < Key.F1 || key > Key.F35;
		}
		
		T Get<T>( int index, T[] a, T[] b ) {
			return index < a.Length ? a[index] : b[index - a.Length];
		}
		
		public override void OnResize( int oldWidth, int oldHeight, int width, int height ) {
			base.OnResize( oldWidth, oldHeight, width, height );
			statusWidget.OnResize( oldWidth, oldHeight, width, height );
		}
		
		public override void Dispose() {
			keyFont.Dispose();
			base.Dispose();
			statusWidget.Dispose();
		}
	}
}