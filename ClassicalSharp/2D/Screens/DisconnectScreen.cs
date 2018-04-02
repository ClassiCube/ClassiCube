// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using ClassicalSharp.Gui.Widgets;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	public class DisconnectScreen : Screen {
		
		string title, message;
		readonly Font titleFont, messageFont;
		TextWidget titleWidget, messageWidget;
		ButtonWidget reconnect;
		DateTime initTime, clearTime;
		bool canReconnect;
		
		public DisconnectScreen(Game game, string title, string message) : base(game) {
			this.title = title;
			this.message = message;
			
			string reason = Utils.StripColours(message);
			canReconnect = !(reason.StartsWith("Kicked ") || reason.StartsWith("Banned "));
			
			titleFont = new Font(game.FontName, 16, FontStyle.Bold);
			messageFont = new Font(game.FontName, 16);
			BlocksWorld = true;
			HidesHud = true;
			HandlesAllInput = true;
		}
		
		public override void Render(double delta) {
			if (canReconnect) UpdateDelayLeft(delta);
			
			// NOTE: We need to make sure that both the front and back buffers have
			// definitely been drawn over, so we redraw the background multiple times.
			if (DateTime.UtcNow < clearTime) Redraw(delta);
		}
		
		public override void Init() {
			game.SkipClear = true;
			game.Graphics.ContextLost += ContextLost;
			game.Graphics.ContextRecreated += ContextRecreated;
			
			ContextRecreated();
			initTime = DateTime.UtcNow;
			lastSecsLeft = delay;
		}

		public override void Dispose() {
			game.SkipClear = false;
			game.Graphics.ContextLost -= ContextLost;
			game.Graphics.ContextRecreated -= ContextRecreated;
			
			ContextLost();
			titleFont.Dispose();
			messageFont.Dispose();
		}
		
		public override void OnResize(int width, int height) {
			titleWidget.Reposition();
			messageWidget.Reposition();
			reconnect.Reposition();
			clearTime = DateTime.UtcNow.AddSeconds(0.5);
		}
		
		public override bool HandlesKeyDown(Key key) { return key < Key.F1 || key > Key.F35; }
		
		public override bool HandlesKeyPress(char key) { return true; }
		
		public override bool HandlesKeyUp(Key key) { return true; }
		
		public override bool HandlesMouseDown(int mouseX, int mouseY, MouseButton button) {
			if (button != MouseButton.Left) return true;
			
			if (!reconnect.Disabled && reconnect.Bounds.Contains(mouseX, mouseY)) {
				string connect = "Connecting to " + game.IPAddress + ":" + game.Port +  "..";
				for (int i = 0; i < game.Components.Count; i++)
					game.Components[i].Reset(game);
				BlockInfo.Reset();
				
				game.Gui.SetNewScreen(new LoadingMapScreen(game, connect, ""));
				game.Server.Connect(game.IPAddress, game.Port);
			}
			return true;
		}
		
		public override bool HandlesMouseMove(int mouseX, int mouseY) {
			reconnect.Active = !reconnect.Disabled && reconnect.Bounds.Contains(mouseX, mouseY);
			return true;
		}
		
		public override bool HandlesMouseScroll(float delta) { return true; }
		
		public override bool HandlesMouseUp(int mouseX, int mouseY, MouseButton button) { return true; }		
		
		int lastSecsLeft;
		const int delay = 5;
		bool lastActive = false;
		void UpdateDelayLeft(double delta) {
			double elapsed = (DateTime.UtcNow - initTime).TotalSeconds;
			int secsLeft = Math.Max(0, (int)(delay - elapsed));
			if (lastSecsLeft == secsLeft && reconnect.Active == lastActive) return;
			
			reconnect.SetText(ReconnectMessage());
			reconnect.Disabled = secsLeft != 0;
			
			Redraw(delta);
			lastSecsLeft = secsLeft;
			lastActive = reconnect.Active;
			clearTime = DateTime.UtcNow.AddSeconds(0.5);
		}
		
		readonly FastColour top = new FastColour(64, 32, 32), bottom = new FastColour(80, 16, 16);
		void Redraw(double delta) {
			game.Graphics.Draw2DQuad(0, 0, game.Width, game.Height, top, bottom);
			game.Graphics.Texturing = true;
			titleWidget.Render(delta);
			messageWidget.Render(delta);
			if (canReconnect) reconnect.Render(delta);
			game.Graphics.Texturing = false;
		}
		
		string ReconnectMessage() {
			if (!canReconnect) return "Reconnect";
			
			double elapsed = (DateTime.UtcNow - initTime).TotalSeconds;
			int secsLeft = (int)(delay - elapsed);
			return secsLeft > 0 ? "Reconnect in " + secsLeft : "Reconnect";
		}
		
		protected override void ContextLost() {
			titleWidget.Dispose();
			messageWidget.Dispose();
			reconnect.Dispose();
		}
		
		protected override void ContextRecreated() {
			if (game.Graphics.LostContext) return;
			clearTime = DateTime.UtcNow.AddSeconds(0.5);
			
			titleWidget = TextWidget.Create(game, title, titleFont)
				.SetLocation(Anchor.Centre, Anchor.Centre, 0, -30);
			messageWidget = TextWidget.Create(game, message, messageFont)
				.SetLocation(Anchor.Centre, Anchor.Centre, 0, 10);
			
			string msg = ReconnectMessage();
			reconnect = ButtonWidget.Create(game, 300, msg, titleFont, null)
				.SetLocation(Anchor.Centre, Anchor.Centre, 0, 80);
			reconnect.Disabled = !canReconnect;
		}
	}
}
