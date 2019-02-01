// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Diagnostics;
using ClassicalSharp;
using ClassicalSharp.Network;
using Launcher.Gui.Views;
using Launcher.Gui.Widgets;
using Launcher.Updater;
using Launcher.Web;
using OpenTK.Input;

namespace Launcher.Gui.Screens {
	public sealed class UpdatesScreen : Screen {
		
		UpdatesView view;
		public UpdatesScreen(LauncherWindow game) : base(game) {
			view = new UpdatesView(game);
			widgets = view.widgets;
		}

		UpdateCheckTask checkTask;
		UpdateDownloadTask fetchTask;
		UpdateCClientTask fetchCClientTask;
		
		string buildName;
		int buildProgress = -1;
		
		public override void Init() {
			base.Init();
			view.Init();
			
			widgets[view.relIndex].OnClick = UpdateStableD3D9;
			widgets[view.relIndex + 1].OnClick = UpdateStableOpenGL;
			widgets[view.devIndex].OnClick = UpdateDevD3D9;
			widgets[view.devIndex + 1].OnClick = UpdateDevOpenGL;
			widgets[view.backIndex].OnClick = SwitchToSettings;
			Resize();
			
			if (game.checkTask.Completed && game.checkTask.Success) {
				checkTask = game.checkTask;
				TickUpdateCheck();
			}
			
			checkTask = new UpdateCheckTask();
			checkTask.RunAsync(game);
		}

		Build dev, stable;
		public override void Tick() {
			TickCClientFetchTask();
			TickFetchTask();
			TickUpdateCheck();
		}
		
		void TickUpdateCheck() {
			if (checkTask == null) return;
			checkTask.Tick();
			if (!checkTask.Completed) return;
			
			view.LastStable = DateTime.MaxValue;
			view.LastDev    = DateTime.MaxValue;
			UpdateCheckTask task = checkTask;
			
			if (task.Success && task.LatestDev != null && task.LatestStable != null) {
				dev    = checkTask.LatestDev;    view.LastDev    = dev.TimeBuilt;
				stable = checkTask.LatestStable; view.LastStable = stable.TimeBuilt;
			} else {
				Widget w = widgets[view.devIndex - 1];
				game.ResetArea(w.X, w.Y, w.Width, w.Height);
				w = widgets[view.relIndex - 1];
				game.ResetArea(w.X, w.Y, w.Width, w.Height);
			}
			
			game.RedrawBackground();
			Resize();
			checkTask = null;
		}
		
		public override void Resize() {
			view.DrawAll();
			game.Dirty = true;
		}
		
		void UpdateStableD3D9(int x, int y) { UpdateBuild(true, true); }
		void UpdateStableOpenGL(int x, int y) { UpdateBuild(true, false); }
		void UpdateDevD3D9(int x, int y) { UpdateBuild(false, true); }
		void UpdateDevOpenGL(int x, int y) { UpdateBuild(false, false); }
		void SwitchToSettings(int x, int y) { game.SetScreen(new SettingsScreen(game)); }
		
		void UpdateStatus() {
			Widget widget = widgets[view.statusIndex];			
			int oldX = widget.X, oldWidth  = widget.Width;
			int oldY = widget.Y, oldHeight = widget.Height;
			
			view.UpdateStatus();
			// if old text is smaller than new
			// TODO: This needs to be better. widgets should keep track of their 'last area'.
			// 			and 'RedrawWidget' should also call ResetArea. Widgets should have a bool for if they obscure background or not.
			game.ResetArea(Math.Min(widget.X, oldX), Math.Min(widget.Y, oldY), 
			               Math.Max(widget.Width, oldWidth), Math.Max(widget.Height, oldHeight));
			RedrawWidget(widgets[view.statusIndex]);
		}
		
		
		static bool IsClient(string name) {
			return
				Utils.CaselessEq(name, "ClassicalSharp")     || Utils.CaselessEq(name, "ClassiCube") ||
				Utils.CaselessEq(name, "ClassicalSharp.exe") || Utils.CaselessEq(name, "ClassiCube.exe");
		}
		
		static bool CheckClientInstances() {
			Process[] processes = Process.GetProcesses();
			for (int i = 0; i < processes.Length; i++) {
				string name = null;
				try {
					name = processes[i].ProcessName;
				} catch {
					continue;
				}

				if (IsClient(name)) return true;
			}
			return false;
		}
		
		static string GetCExe(bool dx) {
			// only a 32 bit build for OSX
			if (OpenTK.Configuration.RunningOnMacOS) return "ClassiCube.osx";
			
			if (!OpenTK.Configuration.RunningOnWindows) {
				return IntPtr.Size == 8 ? "ClassiCube" : "ClassiCube.32";
			}
			
			if (IntPtr.Size == 8) {
				return dx ? "ClassiCube.64.exe" : "ClassiCube.64-opengl.exe";
			} else {
				return dx ? "ClassiCube.exe" : "ClassiCube.opengl.exe";
			}
		}
		
		void UpdateBuild(bool release, bool dx) {
			DateTime last = release ? view.LastStable : view.LastDev;
			Build build   = release ? stable : dev;
			
			if (last == DateTime.MinValue || fetchTask != null) return;
			if (fetchCClientTask != null)                       return;
			if (build.DirectXSize < 50000 || build.OpenGLSize < 50000) return;
			
			bool gameOpen = CheckClientInstances();
			view.statusText = gameOpen ? "&cThe game must be closed before updating" : "";
			UpdateStatus();
			if (gameOpen) return;
			string path;

			path = dx ? build.DirectXPath : build.OpenGLPath;
			fetchTask = new UpdateDownloadTask(path);
			fetchTask.RunAsync(game);
			
			path = GetCExe(dx);
			fetchCClientTask = new UpdateCClientTask(path);
			fetchCClientTask.RunAsync(game);
			
			if (release && dx)   buildName = "&eFetching latest release (Direct3D9)";
			if (release && !dx)  buildName = "&eFetching latest release (OpenGL)";
			if (!release && dx)  buildName = "&eFetching latest dev build (Direct3D9)";
			if (!release && !dx) buildName = "&eFetching latest dev build (OpenGL)";
			
			buildProgress = -1;
			Applier.PatchTime = build.TimeBuilt;
			view.statusText = buildName + "..";
			UpdateStatus();
		}
		
		
		byte[] csZip, cExe;
		void ApplyUpdate() {
			Applier.ExtractUpdate(csZip);
			game.ShouldExit   = true;
			game.ShouldUpdate = true;
			
			if (cExe == null) return;
			string path = Client.GetCExeName();
			// TODO: Set last-modified time to actual time of dev build
			Platform.WriteAllBytes(path, cExe);
		}
		
		void TickCClientFetchTask() {
			if (fetchCClientTask == null) return;
			fetchCClientTask.Tick();
			UpdateProgress(fetchCClientTask);
			if (!fetchCClientTask.Completed) return;
			
			if (!fetchCClientTask.Success) {
				view.statusText = "&cFailed to fetch update";
				UpdateStatus();
			} else {
				cExe = fetchCClientTask.File;
				ApplyUpdate();
			}
			fetchCClientTask = null;
		}
		
		void TickFetchTask() {
			if (fetchTask == null) return;
			fetchTask.Tick();
			UpdateProgress(fetchTask);
			if (!fetchTask.Completed) return;
			
			if (!fetchTask.Success) {
				view.statusText = "&cFailed to fetch update";
				UpdateStatus();
			} else {
				csZip = fetchTask.ZipFile;
				buildName = "&eFetching latest C client";
				UpdateStatus();
			}
			fetchTask = null;
		}
		
		void UpdateProgress(WebTask task) {
			Request item = game.Downloader.CurrentItem;
			if (item == null || item.Identifier != task.identifier) return;
			int progress = game.Downloader.CurrentItemProgress;
			if (progress == buildProgress) return;
			
			buildProgress = progress;
			if (progress >= 0 && progress <= 100) {
				view.statusText = buildName + " &a" + progress + "%";
				UpdateStatus();
			}
		}
		
		public override void Dispose() {
			base.Dispose();
			view.Dispose();
		}
	}
}
