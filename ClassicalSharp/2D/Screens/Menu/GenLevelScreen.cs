// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using System.IO;
using ClassicalSharp.Generator;
using ClassicalSharp.Singleplayer;
using OpenTK.Input;

namespace ClassicalSharp.Gui {
	
	public sealed class GenLevelScreen : MenuScreen {
		
		public GenLevelScreen( Game game ) : base( game ) {
		}
		
		TextWidget[] labels;
		MenuInputWidget[] inputs;
		MenuInputWidget selectedWidget;
		Font labelFont;
		
		public override void Render( double delta ) {
			RenderMenuBounds();
			graphicsApi.Texturing = true;
			RenderMenuWidgets( delta );
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
			labelFont = new Font( game.FontName, size, FontStyle.Regular );
			titleFont = new Font( game.FontName, size, FontStyle.Bold );
			regularFont = new Font( game.FontName, 16, FontStyle.Regular );
			
			inputs = new [] { 
				MakeInput( -100, false, game.World.Width.ToString() ), 
				MakeInput( -60, false, game.World.Height.ToString() ),
				MakeInput( -20, false, game.World.Length.ToString() ),
				MakeInput( 20, true, "" )
			};
			labels = new [] { 
				MakeLabel( -150, -100, "Width:" ), MakeLabel( -150, -60, "Height:" ),
				MakeLabel( -150, -20, "Length:" ), MakeLabel( -140, 20, "Seed:" ),
			};
			widgets = new [] {
				ButtonWidget.Create( game, 0, 80, 250, 35, "Generate flatgrass", Anchor.Centre,
				                    Anchor.Centre, titleFont, GenFlatgrassClick ),
				ButtonWidget.Create( game, 0, 130, 250, 35, "Generate notchy", Anchor.Centre,
				                    Anchor.Centre, titleFont, GenNotchyClick ),
				MakeBack( false, titleFont,
				         (g, w) => g.SetNewScreen( new PauseScreen( g ) ) ),
			};
		}
		
		MenuInputWidget MakeInput( int y, bool seed, string value ) {
			MenuInputValidator validator = seed ? new SeedValidator() : new IntegerValidator( 1, 8192 );
			MenuInputWidget widget = MenuInputWidget.Create(
				game, 0, y, 200, 30, value, Anchor.Centre, Anchor.Centre,
				regularFont, titleFont, validator );
			widget.Active = false;
			widget.OnClick = InputClick;
			return widget;
		}
		
		TextWidget MakeLabel( int x, int y, string text ) {
			return TextWidget.Create( game, x, y, text,
			                         Anchor.Centre, Anchor.Centre, labelFont );
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
			labelFont.Dispose();
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
			int length = GetInt( 2 ), seed = GetSeedInt( 3 );
			
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
		
		int GetSeedInt( int index ) {
			string text = inputs[index].GetText();
			if( text == "" ) return new Random().Next();
			
			if( !inputs[index].Validator.IsValidValue( text ) )
				return 0;
			return text == "" ? 0 : Int32.Parse( text );
		}
	}
}