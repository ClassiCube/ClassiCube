using System;
using System.Drawing;
using System.IO;
using ClassicalSharp.Generator;
using ClassicalSharp.Singleplayer;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public sealed class GenLevelScreen : MenuScreen {
		
		public GenLevelScreen( Game game ) : base( game ) {
		}
		
		TextWidget[] labels;
		MenuInputWidget[] inputs;
		MenuInputWidget selectedWidget;
		
		public override void Render( double delta ) {
			RenderMenuBounds();
			graphicsApi.Texturing = true;
			RenderMenuButtons( delta );
			for( int i = 0; i < inputs.Length; i++ )
				inputs[i].Render( delta );
			for( int i = 0; i < labels.Length; i++ )
				labels[i].Render( delta );
			graphicsApi.Texturing = false;
		}
		
		public override bool HandlesMouseClick( int mouseX, int mouseY, MouseButton button ) {
			return HandleMouseClick( widgets, mouseX, mouseY, button ) ||
				HandleMouseClick( inputs, mouseX, mouseY, button );
		}
		
		public override bool HandlesKeyPress( char key ) {
			return selectedWidget == null ? true :
				selectedWidget.HandlesKeyPress( key );
		}
		
		public override bool HandlesKeyDown( Key key ) {
			if( key == Key.Escape ) {
				game.SetNewScreen( null );
				return true;
			}
			return selectedWidget == null ? (key < Key.F1 || key > Key.F35) :
				selectedWidget.HandlesKeyDown( key );
		}
		
		public override bool HandlesKeyUp( Key key ) {
			return selectedWidget == null ? true :
				selectedWidget.HandlesKeyUp( key );
		}
		
		public override void Init() {
			game.Keyboard.KeyRepeat = true;
			base.Init();
			int size = game.Drawer2D.UseBitmappedChat ? 14 : 18;
			titleFont = new Font( "Arial", size, FontStyle.Bold );			
			regularFont = new Font( "Arial", 16, FontStyle.Regular );
			
			inputs = new [] { MakeInput( -80 ), MakeInput( -40 ), 
				MakeInput( 0 ), MakeInput( 40 )
			};
			labels = new [] { MakeLabel( -80, "Width:" ), MakeLabel( -40, "Height:" ), 
				MakeLabel( 0, "Length:" ), MakeLabel( 40, "Seed:" ),
			};
			widgets = new [] {
				ButtonWidget.Create( game, 0, 90, 250, 30, "Generate flatgrass", Anchor.Centre,
				                    Anchor.Centre, titleFont, GenFlatgrassClick ),
				ButtonWidget.Create( game, 0, 140, 250, 30, "Generate notchy", Anchor.Centre,
				                    Anchor.Centre, titleFont, GenNotchyClick ),
				MakeBack( false, titleFont,
				         (g, w) => g.SetNewScreen( new PauseScreen( g ) ) ),
			};
		}
		
		MenuInputWidget MakeInput( int y ) {
			MenuInputWidget widget = MenuInputWidget.Create(
				game, 0, y, 200, 25, "", Anchor.Centre, Anchor.Centre,
				regularFont, titleFont, new IntegerValidator( 1, 8192 ) );
			widget.Active = false;
			widget.OnClick = InputClick;
			return widget;
		}
		
		TextWidget MakeLabel( int y, string text ) {
			return TextWidget.Create( game, -140, y, text,
			                         Anchor.Centre, Anchor.Centre, titleFont );
		}
		
		public override void OnResize( int oldWidth, int oldHeight, int width, int height ) {
			for( int i = 0; i < inputs.Length; i++ )
				inputs[i].OnResize( oldWidth, oldHeight, width, height );
			for( int i = 0; i < labels.Length; i++ )
				labels[i].OnResize( oldWidth, oldHeight, width, height );
			base.OnResize( oldWidth, oldHeight, width, height );
		}
		
		public override void Dispose() {
			game.Keyboard.KeyRepeat = false;
			for( int i = 0; i < inputs.Length; i++ )
				inputs[i].Dispose();
			for( int i = 0; i < labels.Length; i++ )
				labels[i].Dispose();
			base.Dispose();
		}
		
		void InputClick( Game game, Widget widget, MouseButton mouseBtn ) {
			if( mouseBtn != MouseButton.Left ) return;
			if( selectedWidget != null )
				selectedWidget.Active = false;
			
			selectedWidget = (MenuInputWidget)widget;
			selectedWidget.Active = true;
		}
		
		void GenFlatgrassClick( Game game, Widget widget, MouseButton mouseBtn ) {
			if( mouseBtn != MouseButton.Left ) return; 
			GenerateMap( new FlatGrassGenerator() );
		}
		
		void GenNotchyClick( Game game, Widget widget, MouseButton mouseBtn ) {
			if( mouseBtn != MouseButton.Left ) return;
			GenerateMap( new NotchyGenerator() );
		}
		
		void GenerateMap( IMapGenerator gen ) {
			SinglePlayerServer server = (SinglePlayerServer)game.Network;
			int width = GetInt( 0 ), height = GetInt( 1 );
			int length = GetInt( 2 ), seed = GetInt( 3 );
			
			long volume = (long)width * height * length;
			if( volume > 800 * 800 * 800 ) {
				game.Chat.Add( "&cThe generated map's volume is too big." );
			} else if( width == 0 || height == 0 || length == 0 ) {
				game.Chat.Add( "&cOne of the map dimensions is invalid.");
			} else {
				server.GenMap( width, height, length, seed, gen );
			}
		}
		
		int GetInt( int index ) {
			string text = inputs[index].GetText();
			if( !inputs[index].Validator.IsValidValue( text ) )
				return 0;
			return text == "" ? 0 : Int32.Parse( text );
		}
	}
}