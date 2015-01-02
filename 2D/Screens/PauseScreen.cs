using System;
using System.Drawing;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public class PauseScreen : Screen {
		
		public PauseScreen( Game window ) : base( window ) {
		}
		
		TextWidget controlsWidget, gameWidget, exitWidget;
		TextWidget[] keysLeft, keysRight;
		
		public override void Render( double delta ) {
			GraphicsApi.Draw2DQuad( 0, 0, Window.Width, Window.Height, new FastColour( 255, 255, 255, 100 ) );
			controlsWidget.Render( delta );
			gameWidget.Render( delta );
			exitWidget.Render( delta );
			for( int i = 0; i < keysLeft.Length; i++ ) {
				keysLeft[i].Render( delta );
			}
			for( int i = 0; i < keysRight.Length; i++ ) {
				keysRight[i].Render( delta );
			}
		}
		
		public override void Init() {
			controlsWidget = CreateTextWidget( 0, 30, "&eControls list", Docking.LeftOrTop, 16, FontStyle.Bold );
			gameWidget = CreateTextWidget( 0, -60, "&eBack to game", Docking.BottomOrRight, 16, FontStyle.Bold );
			exitWidget = CreateTextWidget( 0, -10, "&eExit", Docking.BottomOrRight, 16, FontStyle.Bold );
			
			KeyMapping[] mappingsLeft = { KeyMapping.Forward, KeyMapping.Back, KeyMapping.Left, KeyMapping.Right,
				KeyMapping.Jump, KeyMapping.Respawn, KeyMapping.SetSpawn, KeyMapping.OpenChat, KeyMapping.SendChat,
				KeyMapping.PauseOrExit, KeyMapping.OpenInventory };
			string[] descriptionsLeft = { "Forward", "Back", "Left", "Right", "Jump", "Respawn", "Set spawn",
				"Open chat", "Send chat", "Pause", "Open inventory" };
			MakeKeysLeft( mappingsLeft, descriptionsLeft );
			
			KeyMapping[] mappingsRight = { KeyMapping.Screenshot, KeyMapping.Fullscreen, KeyMapping.ThirdPersonCamera, 
				KeyMapping.VSync, KeyMapping.ViewDistance, KeyMapping.Fly, KeyMapping.Speed, KeyMapping.NoClip, 
				KeyMapping.FlyUp, KeyMapping.FlyDown, KeyMapping.PlayerList };
			string[] descriptionsRight = { "Take screenshot", "Toggle fullscreen", "Toggle 3rd person camera", "Toggle VSync",
				"Change view distance", "Toggle fly", "Speed", "Toggle noclip", "Fly up", "Fly down", "Display player list" };
			MakeKeysRight( mappingsRight, descriptionsRight );
		}
		
		void MakeKeysLeft( KeyMapping[] mappings, string[] descriptions ) {
			int startY = controlsWidget.BottomRight.Y + 10;
			keysLeft = new TextWidget[mappings.Length];
			for( int i = 0; i < keysLeft.Length; i++ ) {
				string text = descriptions[i] + ": " + Window.Keys[mappings[i]];
				TextWidget widget = CreateTextWidget( 0, startY, text, Docking.LeftOrTop, 14, FontStyle.Bold );
				widget.XOffset = -widget.Width / 2 - 20;
				widget.MoveTo( widget.X + widget.XOffset, widget.Y );
				keysLeft[i] = widget;
				startY += widget.Height + 5;
			}
		}

		void MakeKeysRight( KeyMapping[] mappings, string[] descriptions ) {
			int startY = controlsWidget.BottomRight.Y + 10;
			keysRight = new TextWidget[mappings.Length];
			for( int i = 0; i < keysRight.Length; i++ ) {
				string text = descriptions[i] + ": " + Window.Keys[mappings[i]];
				TextWidget widget = CreateTextWidget( 0, startY, text, Docking.LeftOrTop, 14, FontStyle.Bold );
				widget.XOffset = widget.Width / 2 + 20;
				widget.MoveTo( widget.X + widget.XOffset, widget.Y );
				keysRight[i] = widget;
				startY += widget.Height + 5;
			}
		}
		
		public override void Dispose() {
			gameWidget.Dispose();
			controlsWidget.Dispose();
			exitWidget.Dispose();
			for( int i = 0; i < keysLeft.Length; i++ ) {
				keysLeft[i].Dispose();
			}
			for( int i = 0; i < keysRight.Length; i++ ) {
				keysRight[i].Dispose();
			}
		}
		
		TextWidget CreateTextWidget( int x, int y, string text, Docking vertical, int fontSize, FontStyle style ) {
			TextWidget widget = new TextWidget( Window, fontSize );
			widget.Style = style;
			widget.Init();
			widget.HorizontalDocking = Docking.Centre;
			widget.VerticalDocking = vertical;
			widget.XOffset = x;
			widget.YOffset = y;
			widget.SetText( text );
			return widget;
		}
		
		public override void OnResize( int oldWidth, int oldHeight, int width, int height ) {
			gameWidget.OnResize( oldWidth, oldHeight, width, height );
			controlsWidget.OnResize( oldWidth, oldHeight, width, height );
			exitWidget.OnResize( oldWidth, oldHeight, width, height );
			for( int i = 0; i < keysLeft.Length; i++ ) {
				keysLeft[i].OnResize( oldWidth, oldHeight, width, height );
			}
			for( int i = 0; i < keysRight.Length; i++ ) {
				keysRight[i].OnResize( oldWidth, oldHeight, width, height );
			}
		}
		
		public override bool HandlesAllInput {
			get { return true; }
		}
		
		public override bool HandlesMouseClick( int mouseX, int mouseY, MouseButton button ) {
			if( button != MouseButton.Left ) return false;
			if( exitWidget.ContainsPoint( mouseX, mouseY ) ) {
				Window.Exit();
				return true;
			} else if( gameWidget.ContainsPoint( mouseX, mouseY ) ) {
				Window.SetNewScreen( new NormalScreen( Window ) );
				return true;
			}
			return false;
		}
	}
}
