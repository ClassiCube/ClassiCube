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
		DateTime initTime;
		bool canReconnect;
		
		public DisconnectScreen(Game game, string title, string message) : base(game) {
			this.title = title;
			this.message = message;
			
			string why = Utils.StripColours(message);
			canReconnect = 
				!(Utils.CaselessStarts(why, "Kicked ") || Utils.CaselessStarts(why, "Banned "));
			
			titleFont = new Font(game.FontName, 16, FontStyle.Bold);
			messageFont = new Font(game.FontName, 16);
			BlocksWorld = true;
			HidesHud = true;
		}
		
		public override void Init() {
			// NOTE: changing VSync can't be done within frame, causes crash on some GPUs
			game.limitMillis = 1000 / 5f;			
			Events.ContextLost      += ContextLost;
			Events.ContextRecreated += ContextRecreated;
			
			ContextRecreated();
			initTime = DateTime.UtcNow;
			lastSecsLeft = delay;
		}
		
		readonly PackedCol top = new PackedCol(64, 32, 32), bottom = new PackedCol(80, 16, 16);
		public override void Render(double delta) {
			if (canReconnect) UpdateDelayLeft(delta);
			game.Graphics.Draw2DQuad(0, 0, game.Width, game.Height, top, bottom);
			
			game.Graphics.Texturing = true;
			titleWidget.Render(delta);
			messageWidget.Render(delta);
			
			if (canReconnect) reconnect.Render(delta);
			game.Graphics.Texturing = false;
		}
		
		public override void Dispose() {
			game.limitMillis = game.CalcLimitMillis(game.FpsLimit);
			Events.ContextLost      -= ContextLost;
			Events.ContextRecreated -= ContextRecreated;
			
			ContextLost();
			titleFont.Dispose();
			messageFont.Dispose();
		}
		
		public override void OnResize() {
			titleWidget.Reposition();
			messageWidget.Reposition();
			reconnect.Reposition();
		}
		
		public override bool HandlesKeyDown(Key key) { return key < Key.F1 || key > Key.F35; }
		
		public override bool HandlesKeyPress(char key) { return true; }
		
		public override bool HandlesKeyUp(Key key) { return true; }
		
		public override bool HandlesMouseDown(int mouseX, int mouseY, MouseButton button) {
			if (button != MouseButton.Left) return true;
			
			if (!reconnect.Disabled && reconnect.Contains(mouseX, mouseY)) {
				string connect = "Connecting to " + game.IPAddress + ":" + game.Port +  "..";
				game.Gui.SetNewScreen(new LoadingScreen(game, connect, ""));
				game.Server.BeginConnect();
			}
			return true;
		}
		
		public override bool HandlesMouseUp(int mouseX, int mouseY, MouseButton button) { return true; }		
		
		public override bool HandlesMouseMove(int mouseX, int mouseY) {
			if (reconnect == null) return true;
			reconnect.Active = !reconnect.Disabled && reconnect.Contains(mouseX, mouseY);
			return true;
		}
		
		public override bool HandlesMouseScroll(float delta) { return true; }
		
		int lastSecsLeft;
		const int delay = 5;
		bool lastActive = false;
		void UpdateDelayLeft(double delta) {
			double elapsed = (DateTime.UtcNow - initTime).TotalSeconds;
			int secsLeft = Math.Max(0, (int)(delay - elapsed));
			if (lastSecsLeft == secsLeft && reconnect.Active == lastActive) return;
			
			reconnect.Set(ReconnectMessage(), titleFont);
			reconnect.Disabled = secsLeft != 0;
			
			lastSecsLeft = secsLeft;
			lastActive = reconnect.Active;
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
