using System;
using System.Drawing;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public class OptionsGroupScreen : MenuScreen {
		
		public OptionsGroupScreen( Game game ) : base( game ) {
		}
		TextWidget descWidget;
		ButtonWidget selectedWidget;
		
		public override void Render( double delta ) {
			RenderMenuBounds();
			graphicsApi.Texturing = true;
			RenderMenuWidgets( delta );
			if( descWidget != null )
				descWidget.Render( delta );
			graphicsApi.Texturing = false;
		}
		
		public override void Init() {
			base.Init();
			game.Events.HackPermissionsChanged += CheckHacksAllowed;
			regularFont = new Font( game.FontName, 16, FontStyle.Regular );
			
			MakeNormal();
			CheckHacksAllowed( null, null );
		}
		
		void MakeNormal() {
			widgets = new Widget[] {
				Make( -140, -100, "Misc options",
				     (g, w) => g.SetNewScreen( new MiscOptionsScreen( g ) ) ),
				Make( -140, -50, "Gui options",
				     (g, w) => g.SetNewScreen( new GuiOptionsScreen( g ) ) ),
				Make( -140, 0, "Graphics options",
				     (g, w) => g.SetNewScreen( new GraphicsOptionsScreen( g ) ) ),
				Make( -140, 50, "Key bindings",
				     (g, w) => g.SetNewScreen( new NormalKeyBindingsScreen( g ) ) ),
				Make( 140, -50, "Hacks settings",
				     (g, w) => g.SetNewScreen( new HacksSettingsScreen( g ) ) ),
				Make( 140, 0, "Env settings",
				     (g, w) => g.SetNewScreen( new EnvSettingsScreen( g ) ) ),
				Make( 140, 50, "Nostalgia options",
				     (g, w) => g.SetNewScreen( new NostalgiaScreen( g ) ) ),
				
				MakeBack( true, titleFont, (g, w) => g.SetNewScreen( null ) ),
			};
		}
		
		void CheckHacksAllowed( object sender, EventArgs e ) {
			if( game.UseClassicOptions ) return;
			widgets[5].Disabled = !game.LocalPlayer.Hacks.CanAnyHacks; // env settings
		}
		
		protected override void WidgetSelected( Widget widget ) {
			ButtonWidget button = widget as ButtonWidget;
			if( selectedWidget == widget || button == null ||
			   button == widgets[widgets.Length - 1] ) return;
			
			selectedWidget = button;
			if( descWidget != null ) descWidget.Dispose();
			if( button == null ) return;
			
			string text = descriptions[Array.IndexOf<Widget>(widgets, button)];
			descWidget = TextWidget.Create( game, 0, 100, text, Anchor.Centre, Anchor.Centre, regularFont );
		}
		
		ButtonWidget Make( int x, int y, string text, Action<Game, Widget> onClick ) {
			return ButtonWidget.Create( game, x, y, 240, 35, text,
			                           Anchor.Centre, Anchor.Centre, titleFont, LeftOnly( onClick ) );
		}
		
		ButtonWidget MakeOther( int x, int y, int width, string text, Anchor hAnchor, Action<Game, Widget> onClick ) {
			return ButtonWidget.Create( game, x, y, width, 35, text,
			                           hAnchor, Anchor.BottomOrRight, titleFont, LeftOnly( onClick ) );
		}
		
		public override bool HandlesKeyDown( Key key ) {
			if( key == Key.Escape )
				game.SetNewScreen( null );
			return key < Key.F1 || key > Key.F35;
		}
		
		public override void OnResize( int oldWidth, int oldHeight, int width, int height ) {
			if( descWidget != null )
				descWidget.OnResize( oldWidth, oldHeight, width, height );
			base.OnResize( oldWidth, oldHeight, width, height );
		}
		
		public override void Dispose() {
			if( descWidget != null )
				descWidget.Dispose();
			base.Dispose();
			game.Events.HackPermissionsChanged -= CheckHacksAllowed;
		}
		
		static string[] descriptions = {
			"&eMusic/Sound, view bobbing, and more",
			"&eChat options, gui scale, font settings, and more",
			"&eFPS limit, view distance, entity names/shadows",
			"&eControls, bind keys to act as mouse clicks",
			"&eHacks allowed, jump settings, and more",
			"&eEnv colours, water level, weather, and more",
			"&eSettings for resembling the original classic",
		};
	}
}