// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Net;
using System.Reflection;
using System.Threading;
using ClassicalSharp;
using ClassicalSharp.Network;
using Launcher.Drawing;
using Launcher.Gui.Screens;
using Launcher.Patcher;
using Launcher.Web;
using OpenTK;
using OpenTK.Graphics;
using OpenTK.Input;
using OpenTK.Platform;

namespace Launcher {

	public sealed partial class LauncherWindow {
		
		public INativeWindow Window;
		public IDrawer2D Drawer;
		public Screen Screen;
		
		/// <summary> Whether the client drawing area needs to be redrawn/presented to the screen. </summary>
		public bool Dirty, pendingRedraw;
		/// <summary> The specific area/region of the window that needs to be redrawn. </summary>
		public Rectangle DirtyArea;
		
		public string Username;
		public AsyncDownloader Downloader;
		
		public int Width, Height;
		public Bitmap Framebuffer;
		
		/// <summary> Whether at the next tick, the launcher window should proceed to stop displaying frames and subsequently exit. </summary>
		public bool ShouldExit;
		public bool ShouldUpdate;
		
		public List<ServerListEntry> Servers = new List<ServerListEntry>();
		
		public string FontName = "Arial";
		
		public bool Minimised {
			get { return Window.WindowState == WindowState.Minimized || (Width == 1 && Height == 1); }
		}
		
		public bool IsKeyDown(Key key) { return Keyboard.Get(key); }
		
		internal ResourceFetcher fetcher;
		internal UpdateCheckTask checkTask;
		bool fullRedraw;
		
		Font logoFont;
		PlatformDrawer platformDrawer;
		void Init() {
			Window.Resize += Resize;
			Window.FocusedChanged += RedrawAll;
			Window.WindowStateChanged += Resize;
			Window.Redraw += RedrawPending;
			Keyboard.KeyDown += KeyDown;
			
			LoadSettings();
			logoFont = new Font(FontName, 32, FontStyle.Regular);
			
			string path = Assembly.GetExecutingAssembly().Location;
			try {
				Window.Icon = Icon.ExtractAssociatedIcon(path);
			} catch (Exception ex) {
				ErrorHandler.LogError("LauncherWindow.Init() - Icon", ex);
			}
			//Minimised = Window.WindowState == WindowState.Minimized;
			
			PlatformID platform = Environment.OSVersion.Platform;
			if (platform == PlatformID.Win32Windows) {
				platformDrawer = new WinOldPlatformDrawer();
			} else if (Configuration.RunningOnWindows) {
				platformDrawer = new WinPlatformDrawer();
			} else if (Configuration.RunningOnX11) {
				platformDrawer = new X11PlatformDrawer();
			} else if (Configuration.RunningOnMacOS) {
				platformDrawer = new OSXPlatformDrawer();
			}
			
			IDrawer2D.Cols['g'] = new PackedCol(125, 125, 125);
			
			if (Configuration.RunningOnWindows && Platform.FileExists(Client.GetExeName())) {
				ErrorHandler.ShowDialog("Deprecated Client",
				                        "ClassicalSharp is deprecated - use " + Client.GetExeName() + " instead");
			}
		}
		
		void LoadSettings() {
			Options.Load();
			Client.CClient = Options.GetBool(OptionsKey.CClient, false);
			FontName = Options.Get("gui-fontname", "Arial");
			
			try {
				using (Font f = new Font(FontName, 16)) { }
			} catch (Exception) {
				FontName = "Arial";
				Options.Set("gui-fontname", "Arial");
			}
		}

		void Resize() {
			UpdateClientSize();
			platformDrawer.Resize();
			RedrawAll();
		}
		
		void RedrawPending() {
			// in case we get multiple of these events
			pendingRedraw = true;
			Dirty = true;
		}
		
		void RedrawAll() {
			if (Program.ShowingErrorDialog) return;
			RedrawBackground();
			if (Screen != null) Screen.Resize();
			fullRedraw = true;
		}
		
		public void SetScreen(Screen screen) {
			if (Screen != null) Screen.Dispose();
			RedrawBackground();
			Screen = screen;
			screen.Init();
			// for selecting active button etc
			Screen.MouseMove(0, 0);
		}
		
		public bool ConnectToServer(List<ServerListEntry> publicServers, string hash) {
			if (String.IsNullOrEmpty(hash)) return false;
			
			ClientStartData data = null;
			for (int i = 0; i < publicServers.Count; i++) {
				ServerListEntry entry = publicServers[i];
				if (entry.Hash != hash) continue;
				
				data = new ClientStartData(Username, entry.Mppass,
				                           entry.IPAddress, entry.Port, entry.Name);
				Client.Start(data, true, ref ShouldExit);
				return true;
			}
			
			// Fallback to private server handling
			try {
				// TODO: Rewrite to be async
				FetchServerTask task = new FetchServerTask(Username, hash);
				task.RunAsync(this);
				
				while (!task.Completed) { task.Tick(); Thread.Sleep(10); }
				if (task.WebEx != null) throw task.WebEx;
				
				data = task.Info;
			} catch (WebException ex) {
				ErrorHandler.LogError("retrieving server information", ex);
				return false;
			} catch (ArgumentOutOfRangeException) {
				return false;
			}
			Client.Start(data, true, ref ShouldExit);
			return true;
		}
		
		void UpdateClientSize() {
			Size size = Window.ClientSize;
			Width  = Math.Max(size.Width,  1);
			Height = Math.Max(size.Height, 1);
		}
		
		public void Run() {
			Window = Factory.CreateWindow(640, 400, Program.AppName,
			                              GraphicsMode.Default, DisplayDevice.Default);
			Window.Visible = true;
			Drawer = new GdiPlusDrawer2D();
			UpdateClientSize();
			
			Init();
			TryLoadTexturePack();
			platformDrawer.window = Window;
			platformDrawer.Init();
			
			Downloader = new AsyncDownloader(Drawer);
			Downloader.Init("");
			Downloader.Cookies = new CookieContainer();
			Downloader.KeepAlive = true;
			
			fetcher = new ResourceFetcher();
			fetcher.CheckResourceExistence();
			checkTask = new UpdateCheckTask();
			checkTask.RunAsync(this);
			
			if (!fetcher.AllResourcesExist) {
				SetScreen(new ResourcesScreen(this));
			} else {
				SetScreen(new MainScreen(this));
			}
			
			while (true) {
				Window.ProcessEvents();
				if (!Window.Exists) break;
				if (ShouldExit) {
					if (Screen != null) {
						Screen.Dispose();
						Screen = null;
					}
					break;
				}
				
				checkTask.Tick();
				Screen.Tick();
				if (Dirty) Display();
				Thread.Sleep(10);
			}
			
			if (Options.Load()) {
				LauncherSkin.SaveToOptions();
				Options.Save();
			}
			
			if (ShouldUpdate)
				Updater.Applier.ApplyUpdate();
			if (Window.Exists)
				Window.Close();
		}
		
		void Display() {
			if (pendingRedraw) {
				RedrawAll();
				pendingRedraw = false;
			}
			
			Screen.OnDisplay();
			Dirty = false;
			
			Rectangle rec = new Rectangle(0, 0, Framebuffer.Width, Framebuffer.Height);
			if (!fullRedraw && DirtyArea.Width > 0) {
				rec = DirtyArea;
			}
			platformDrawer.Redraw(Framebuffer, rec);
			DirtyArea = Rectangle.Empty;
			fullRedraw = false;
		}
		
		void KeyDown(Key key) {
			if (IsShutdown(key)) ShouldExit = true;
		}
		
		void Dispose() {
			Window.Resize -= Resize;
			Window.FocusedChanged -= RedrawAll;
			Window.WindowStateChanged -= Resize;
			Window.Redraw -= RedrawPending;
			Keyboard.KeyDown -= KeyDown;
			
			List<FastBitmap> bitmaps = FetchFlagsTask.Bitmaps;
			for (int i = 0; i < bitmaps.Count; i++) {
				bitmaps[i].Dispose();
				bitmaps[i].Bitmap.Dispose();
			}
			logoFont.Dispose();
		}
		
		bool WinDown { get { return IsKeyDown(Key.WinLeft) || IsKeyDown(Key.WinRight); } }
		bool AltDown { get { return IsKeyDown(Key.AltLeft) || IsKeyDown(Key.AltRight); } }
		
		bool IsShutdown(Key key) {
			if (key == Key.F4 && AltDown) return true;
			
			// On OSX, Cmd+Q should also terminate the process.
			if (!OpenTK.Configuration.RunningOnMacOS) return false;
			return key == Key.Q && WinDown;
		}
		
		public FastBitmap LockBits() {
			return new FastBitmap(Framebuffer, true, false);
		}
	}
}
