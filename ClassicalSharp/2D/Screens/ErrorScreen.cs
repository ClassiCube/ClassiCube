// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp.Gui.Widgets;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	public class ErrorScreen : ClickableScreen {
		
		string title, message;
		readonly Font titleFont, messageFont;
		Widget[] widgets;
		DateTime initTime, clearTime;
		bool canReconnect;
		
		public ErrorScreen(Game game, string title, string message) : base(game) {
			this.title = title;
			this.message = message;
			
			string reason = Utils.StripColours(message);
			canReconnect = !(reason.StartsWith("Kicked ") || reason.StartsWith("Banned "));
			
			titleFont = new Font(game.FontName, 16, FontStyle.Bold);
			messageFont = new Font(game.FontName, 16, FontStyle.Regular);
		}
		
		public override void Render(double delta) {
			if (canReconnect) UpdateDelayLeft(delta);
			
			// NOTE: We need to make sure that both the front and back buffers have
			// definitely been drawn over, so we redraw the background multiple times.
			if (DateTime.UtcNow < clearTime)
				Redraw(delta);
		}
		
		public override void Init() {
			game.SkipClear = true;
			string msg = canReconnect ? "Reconnect in " + delay : "Reconnect";
			widgets = new Widget[] {
				TextWidget.Create(game, title, titleFont)
					.SetLocation(Anchor.Centre, Anchor.Centre, 0, -30),
				TextWidget.Create(game, message, messageFont)
					.SetLocation(Anchor.Centre, Anchor.Centre, 0, 10),
				ButtonWidget.Create(game, 301, 40, msg, titleFont, ReconnectClick)
					.SetLocation(Anchor.Centre, Anchor.Centre, 0, 80),
			};
			widgets[2].Disabled = !canReconnect;
			
			game.Graphics.ContextRecreated += ContextRecreated;
			initTime = DateTime.UtcNow;
			clearTime = DateTime.UtcNow.AddSeconds(0.5);
			lastSecsLeft = delay;
		}

		public override void Dispose() {
			game.SkipClear = false;
			game.Graphics.ContextRecreated -= ContextRecreated;
			titleFont.Dispose();
			messageFont.Dispose();
			for (int i = 0; i < widgets.Length; i++)
				widgets[i].Dispose();
		}
		
		public override void OnResize(int width, int height) {
			for (int i = 0; i < widgets.Length; i++)
				widgets[i].CalculatePosition();
			clearTime = DateTime.UtcNow.AddSeconds(0.5);
		}
		
		public override bool BlocksWorld { get { return true; } }
		
		public override bool HandlesAllInput { get { return true; } }
		
		public override bool HidesHud { get { return true; } }
		
		public override bool HandlesKeyDown(Key key) { return key < Key.F1 || key > Key.F35; }
		
		public override bool HandlesKeyPress(char key) { return true; }
		
		public override bool HandlesKeyUp(Key key) { return true; }
		
		public override bool HandlesMouseClick(int mouseX, int mouseY, MouseButton button) {
			return HandleMouseClick(widgets, mouseX, mouseY, button);
		}
		
		public override bool HandlesMouseMove(int mouseX, int mouseY) {
			return HandleMouseMove(widgets, mouseX, mouseY);
		}
		
		public override bool HandlesMouseScroll(int delta) { return true; }
		
		public override bool HandlesMouseUp(int mouseX, int mouseY, MouseButton button) { return true; }
		
		
		int lastSecsLeft;
		const int delay = 5;
		bool lastActive = false;
		void UpdateDelayLeft(double delta) {
			ButtonWidget btn = (ButtonWidget)widgets[2];
			double elapsed = (DateTime.UtcNow - initTime).TotalSeconds;
			int secsLeft = Math.Max(0, (int)(delay - elapsed));
			if (lastSecsLeft == secsLeft && btn.Active == lastActive) return;
			if (secsLeft == 0) btn.SetText("Reconnect");
			else btn.SetText("Reconnect in " + secsLeft);
			btn.Disabled = secsLeft != 0;
			
			Redraw(delta);
			lastSecsLeft = secsLeft;
			lastActive = btn.Active;
			clearTime = DateTime.UtcNow.AddSeconds(0.5);
		}
		
		readonly FastColour top = new FastColour(64, 32, 32), bottom = new FastColour(80, 16, 16);
		void Redraw(double delta) {
			gfx.Draw2DQuad(0, 0, game.Width, game.Height, top, bottom);
			gfx.Texturing = true;
			for (int i = 0; i < widgets.Length; i++)
				widgets[i].Render(delta);
			gfx.Texturing = false;
		}
		
		void ReconnectClick(Game g, Widget w, MouseButton btn, int x, int y) {
			if (btn != MouseButton.Left) return;
			string connectString = "Connecting to " + game.IPAddress + ":" + game.Port +  "..";
			for (int i = 0; i < game.Components.Count; i++)
				game.Components[i].Reset(game);
			game.BlockInfo.Reset(game);
			
			game.Gui.SetNewScreen(new LoadingMapScreen(game, connectString, "Waiting for handshake"));
			game.Server.Connect(game.IPAddress, game.Port);
		}
		
		void ContextRecreated() {
			clearTime = DateTime.UtcNow.AddSeconds(0.5);
		}
	}
}
