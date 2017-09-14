// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Diagnostics;
using ClassicalSharp;
using Launcher.Gui.Views;
using Launcher.Gui.Widgets;
using Launcher.Updater;
using Launcher.Web;
using OpenTK.Input;

namespace Launcher.Gui.Screens {	
	// TODO: Download asynchronously
	public sealed class UpdatesScreen : Screen {
		
		UpdatesView view;
		public UpdatesScreen(LauncherWindow game) : base(game) {
			view = new UpdatesView(game);
			widgets = view.widgets;
		}

		UpdateCheckTask checkTask;
		public override void Init() {
			base.Init();
			view.Init();
			
			widgets[view.relIndex].OnClick = UpdateStableD3D9;
			widgets[view.relIndex + 1].OnClick = UpdateStableOpenGL;
			widgets[view.devIndex].OnClick = UpdateDevD3D9;
			widgets[view.devIndex + 1].OnClick = UpdateStableOpenGL;	
			widgets[view.backIndex].OnClick = SwitchToSettings;			
			Resize();
			
			if (game.checkTask != null && game.checkTask.Done && game.checkTask.Success) {
				SuccessfulUpdateCheck(game.checkTask);
			}			
			checkTask = new UpdateCheckTask();
			checkTask.CheckForUpdatesAsync();
		}

		Build dev, stable;
		public override void Tick() {
			if (checkTask != null && checkTask.Done) {				
				if (checkTask.Success) SuccessfulUpdateCheck(checkTask);
				else FailedUpdateCheck(checkTask);
				checkTask = null;
			}
		}
		
		void SuccessfulUpdateCheck(UpdateCheckTask task) {
			if (task.LatestDev == null || task.LatestStable == null) return;
			dev = task.LatestDev; view.LastDev = dev.TimeBuilt;
			stable = task.LatestStable; view.LastStable = stable.TimeBuilt;
			game.RedrawBackground();
			Resize();
		}
		
		void FailedUpdateCheck(UpdateCheckTask task) {
			view.LastStable = DateTime.MaxValue;
			view.LastDev = DateTime.MaxValue;
			task.Exception = null;
			
			Widget w = widgets[view.devIndex - 1];
			game.ResetArea(w.X, w.Y, w.Width, w.Height);
			w = widgets[view.relIndex - 1];
			game.ResetArea(w.X, w.Y, w.Width, w.Height);
			game.RedrawBackground();
			Resize();
		}
		
		public override void Resize() {
			view.DrawAll();
			game.Dirty = true;
		}
		
		void UpdateStableD3D9(int x, int y) { UpdateBuild(true, true); }
		void UpdateStableOpenGL(int x, int y) { UpdateBuild(true, false); }
		void UpdateDevD3D9(int x, int y) { UpdateBuild(false, true); }
		void UpdateDevOpenGL(int x, int y) { UpdateBuild(false, false); }
		void SwitchToSettings(int x, int y) { game.SetScreen(new MainScreen(game)); }
		
		void UpdateBuild(bool release, bool dx) {
			DateTime last = release ? view.LastStable : view.LastDev;
			Build build = release ? stable : dev;
			if (last == DateTime.MinValue || build.DirectXSize < 50000
			   || build.OpenGLSize < 50000) return;
			
			view.gameOpen = CheckClientInstances();
			view.SetWarning();
			Widget widget = widgets[view.statusIndex];
			game.ResetArea(widget.X, widget.Y, widget.Width, widget.Height);
			RedrawWidget(widgets[view.statusIndex]);
			if (view.gameOpen) return;
			
			string path = dx ? build.DirectXPath : build.OpenGLPath;
			Utils.LogDebug("Updating to: " + path);
			Applier.PatchTime = build.TimeBuilt;
			Applier.FetchUpdate(path);
			game.ShouldExit = true;
			game.ShouldUpdate = true;
		}
		
		bool CheckClientInstances() {
			Process[] processes = Process.GetProcesses();
			for (int i = 0; i < processes.Length; i++) {
				string name = null;
				try {
					name = processes[i].ProcessName;
				} catch {
					continue;
				}
				
				if (Utils.CaselessEquals(name, "ClassicalSharp")
				   || Utils.CaselessEquals(name, "ClassicalSharp.exe"))
					return true;
			}
			return false;
		}
		
		public override void Dispose() {
			base.Dispose();
			view.Dispose();
		}
	}
}
