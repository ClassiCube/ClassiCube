// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using System.IO;
using OpenTK;
using OpenTK.Graphics;
using Clipboard = System.Windows.Forms.Clipboard;

namespace ClassicalSharp {
	
	/// <summary> Implementation of a native window and native input handling mechanism on Windows, OSX, and Linux. </summary>
	public sealed class DesktopWindow : GameWindow, IPlatformWindow {
		
		public int Width { get { return ClientSize.Width; } }
		public int Height { get { return ClientSize.Height; } }
		
		Game game;
		public DesktopWindow(Game game, string username, bool nullContext, int width, int height) :
			base(width, height, GraphicsMode.Default, Program.AppName + " (" + username + ")", nullContext, 0, DisplayDevice.Default) {
			this.game = game;
		}
		
		protected override void OnLoad(EventArgs e) {
			game.OnLoad();
			base.OnLoad(e);
		}
		
		public override void Dispose() {
			game.Dispose();
			base.Dispose();
		}
		
		protected override void OnRenderFrame(FrameEventArgs e) {
			game.RenderFrame(e.Time);
			base.OnRenderFrame(e);
		}
		
		protected override void OnResize(object sender, EventArgs e) {
			game.OnResize();
			base.OnResize(sender, e);
		}
		
		public void LoadIcon() {
			string launcherPath = Path.Combine(Program.AppDirectory, "Launcher2.exe");
			if (!File.Exists(launcherPath)) {
				launcherPath = Path.Combine(Program.AppDirectory, "Launcher.exe");
			}
			if (!File.Exists(launcherPath)) return;
			
			try {
				Icon = Icon.ExtractAssociatedIcon(launcherPath);
			} catch (Exception ex) {
				ErrorHandler.LogError("DesktopWindow.LoadIcon()", ex);
			}
		}
		
		// TODO: retry when clipboard returns null.
		public string ClipboardText {
			get {
				if (OpenTK.Configuration.RunningOnMacOS)
					return GetClipboardText();
				else
					return Clipboard.GetText();
			}
			set {
				if (OpenTK.Configuration.RunningOnMacOS)
					SetClipboardText(value);
				else
					Clipboard.SetText(value);
			}
		}
	}
}
