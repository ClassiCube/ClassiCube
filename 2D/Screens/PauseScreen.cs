using System;
using System.Drawing;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public class PauseScreen : Screen {
		
		public PauseScreen( Game window ) : base( window ) {
		}
		
		TextWidget controlsWidget, gameWidget, exitWidget, keyStatusWidget;
		KeyMapWidget[] keysLeft, keysRight;
		KeyMapWidget widgetToChange;
		
		public override void Render( double delta ) {
			GraphicsApi.Draw2DQuad( 0, 0, Window.Width, Window.Height, new FastColour( 255, 255, 255, 100 ) );
			controlsWidget.Render( delta );
			gameWidget.Render( delta );
			exitWidget.Render( delta );
			keyStatusWidget.Render( delta );
			for( int i = 0; i < keysLeft.Length; i++ ) {
				keysLeft[i].Render( delta );
			}
			for( int i = 0; i < keysRight.Length; i++ ) {
				keysRight[i].Render( delta );
			}
		}
		
		Font titleFont, keyStatusFont, textFont;
		static string[] keyNames;
		public override void Init() {
			if( keyNames == null ) {
				keyNames = Enum.GetNames( typeof( Key ) );
			}
			titleFont = new Font( "Arial", 16, FontStyle.Bold );
			keyStatusFont = new Font( "Arial", 13, FontStyle.Italic );
			textFont = new Font( "Arial", 14, FontStyle.Bold );
			controlsWidget = TextWidget.Create( Window, 0, 30, "&eControls list", Docking.Centre, Docking.LeftOrTop, titleFont );
			keyStatusWidget = TextWidget.Create( Window, 0, 80, "", Docking.Centre, Docking.BottomOrRight, keyStatusFont );
			gameWidget = TextWidget.Create( Window, 0, 50, "&eBack to game", Docking.Centre, Docking.BottomOrRight, titleFont );
			exitWidget = TextWidget.Create( Window, 0, 10, "&eExit", Docking.Centre, Docking.BottomOrRight, titleFont );
			
			string[] descriptionsLeft = { "Forward", "Back", "Left", "Right", "Jump", "Respawn", "Set spawn",
				"Open chat", "Send chat", "Pause", "Open inventory" };
			MakeKeys( KeyMapping.Forward, descriptionsLeft, 10, out keysLeft );
			leftEnd = CalculateMaxWidth( keysLeft );
			
			string[] descriptionsRight = { "Take screenshot", "Toggle fullscreen", "Toggle 3rd person camera", "Toggle VSync",
				"Change view distance", "Toggle fly", "Speed", "Toggle noclip", "Fly up", "Fly down", "Display player list" };
			MakeKeys( KeyMapping.Screenshot, descriptionsRight, leftEnd + 30, out keysRight );
		}
		
		int leftEnd;
		void MakeKeys( KeyMapping start, string[] descriptions, int offset, out KeyMapWidget[] widgets ) {
			int startY = controlsWidget.BottomRight.Y + 10;
			widgets = new KeyMapWidget[descriptions.Length];
			
			for( int i = 0; i < widgets.Length; i++ ) {
				KeyMapping mapping = (KeyMapping)( (int)start + i );
				Key tkKey = Window.Keys[mapping];
				string text = descriptions[i] + ": " + keyNames[(int)tkKey];
				TextWidget widget = TextWidget.Create( Window, 0, startY, text, Docking.LeftOrTop, Docking.LeftOrTop, textFont );
				widget.XOffset = offset;
				widget.MoveTo( widget.X + widget.XOffset, widget.Y );
				widgets[i] = new KeyMapWidget( widget, mapping, descriptions[i] );
				startY += widget.Height + 5;
			}
		}
		
		public override void Dispose() {
			textFont.Dispose();
			titleFont.Dispose();
			keyStatusFont.Dispose();
			gameWidget.Dispose();
			controlsWidget.Dispose();
			exitWidget.Dispose();
			for( int i = 0; i < keysLeft.Length; i++ ) {
				keysLeft[i].Dispose();
				keysRight[i].Dispose();
			}
		}
		
		public override void OnResize( int oldWidth, int oldHeight, int width, int height ) {
			gameWidget.OnResize( oldWidth, oldHeight, width, height );
			controlsWidget.OnResize( oldWidth, oldHeight, width, height );
			exitWidget.OnResize( oldWidth, oldHeight, width, height );
			keyStatusWidget.OnResize( oldWidth, oldHeight, width, height );
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
		
		public override bool HandlesKeyDown( Key key ) {
			if( widgetToChange != null ) {
				KeyMapWidget widget = widgetToChange;
				widgetToChange = null;
				string reason;
				if( !Window.Keys.IsKeyOkay( key, out reason ) ) {
					const string format = "&eFailed to change mapping \"{0}\". &c({1})";
					keyStatusWidget.SetText( String.Format( format, widget.Description, reason ) );
				} else {
					Key oldKey = Window.Keys[widget.Mapping];
					const string format = "&eChanged mapping \"{0}\" from &7{1} &eto &7{2}&e.";
					keyStatusWidget.SetText( String.Format( format, widget.Description, oldKey, key ) );
					Window.Keys[widget.Mapping] = key;
					widget.Widget.SetText( widget.Description + ": " + key );
					if( Array.IndexOf( keysLeft, widget ) >= 0 ) {
						ResizeKeysRight();
					}
				}
			}
			return true;
		}
		
		void ResizeKeysRight() {
			int newLeftEnd = CalculateMaxWidth( keysLeft );
			if( newLeftEnd != leftEnd ) {
				int diff = newLeftEnd - leftEnd;
				for( int i = 0; i < keysRight.Length; i++ ) {
					TextWidget textWidget = keysRight[i].Widget;
					textWidget.XOffset = newLeftEnd + 30;
					textWidget.MoveTo( textWidget.X + diff, textWidget.Y );
				}
			}
			leftEnd = newLeftEnd;
		}
		
		int CalculateMaxWidth( KeyMapWidget[] widgets ) {
			int maxWidth = 0;
			for( int i = 0; i < widgets.Length; i++ ) {
				maxWidth = Math.Max( widgets[i].Widget.Width, maxWidth );
			}
			return maxWidth;
		}
		
		public override bool HandlesMouseClick( int mouseX, int mouseY, MouseButton button ) {
			if( button != MouseButton.Left ) return false;
			if( exitWidget.ContainsPoint( mouseX, mouseY ) ) {
				Window.Exit();
				return true;
			} else if( gameWidget.ContainsPoint( mouseX, mouseY ) ) {
				Window.SetNewScreen( new NormalScreen( Window ) );
				return true;
			} else if( widgetToChange == null ) {
				for( int i = 0; i < keysLeft.Length; i++ ) {
					KeyMapWidget widget = keysLeft[i];
					if( widget.Widget.ContainsPoint( mouseX, mouseY ) ) {
						SetWidgetToChange( widget );
						return true;
					}
				}
				for( int i = 0; i < keysRight.Length; i++ ) {
					KeyMapWidget widget = keysRight[i];
					if( widget.Widget.ContainsPoint( mouseX, mouseY ) ) {
						SetWidgetToChange( widget );
						return true;
					}
				}
			}
			return false;
		}
		
		void SetWidgetToChange( KeyMapWidget widget ) {
			Key oldKey = Window.Keys[widget.Mapping];
			if( !Window.Keys.IsLockedKey( oldKey ) ) {
				const string format = "&ePress new key for \"{0}\".";
				keyStatusWidget.SetText( String.Format( format, widget.Description ) );
				widgetToChange = widget;
			} else {
				const string format = "&cCannot change mapping of &e\"{0}\".";
				keyStatusWidget.SetText( String.Format( format, widget.Description ) );
			}
		}
		
		class KeyMapWidget {
			public TextWidget Widget;
			public KeyMapping Mapping;
			public string Description;
			
			public KeyMapWidget( TextWidget widget, KeyMapping mapping, string desc ) {
				Widget = widget;
				Mapping = mapping;
				Description = desc;
			}
			
			public void Render( double delta ) {
				Widget.Render( delta );
			}
			
			public void OnResize( int oldWidth, int oldHeight, int width, int height ) {
				Widget.OnResize( oldWidth, oldHeight, width, height );
			}
			
			public void Dispose() {
				Widget.Dispose();
			}
		}
	}
}
