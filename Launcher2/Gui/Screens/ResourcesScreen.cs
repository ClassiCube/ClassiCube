// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.IO;
using ClassicalSharp.Network;
using Launcher.Gui.Views;
using Launcher.Patcher;

namespace Launcher.Gui.Screens {	
	public sealed class ResourcesScreen : Screen {
		
		ResourceFetcher fetcher;
		ResourcesView view;
		int lastProgress = int.MinValue;

		public ResourcesScreen(LauncherWindow game) : base(game) {
			view = new ResourcesView(game);
			widgets = view.widgets;
		}

		public override void Init() {
			base.Init();
			view.Init();
			
			SetWidgetHandlers();
			Resize();
		}
		
		void SetWidgetHandlers() {
			widgets[view.yesIndex].OnClick = DownloadResources;
			widgets[view.noIndex].OnClick = GotoNextMenu;
			widgets[view.cancelIndex].OnClick = GotoNextMenu;
		}
		
		bool failed;
		public override void Tick() {
			if (fetcher == null || failed) return;
			CheckCurrentProgress();
			
			if (!fetcher.Check(SetStatus))
				failed = true;
			
			if (!fetcher.Done) return;
			if (ResourceList.GetFetchFlags() != 0) {
				ResourcePatcher patcher = new ResourcePatcher(fetcher, drawer);
				patcher.Run();
			}
			
			fetcher = null;
			GC.Collect();
			game.TryLoadTexturePack();
			GotoNextMenu(0, 0);
		}
		
		public override void Resize() {
			view.DrawAll();
			game.Dirty = true;
		}
		
		void CheckCurrentProgress() {
			Request item = fetcher.downloader.CurrentItem;
			if (item == null) { lastProgress = int.MinValue; return; }
			
			int progress = fetcher.downloader.CurrentItemProgress;
			if (progress == lastProgress) return;
			lastProgress = progress;
			SetFetchStatus(progress);
		}
		
		void SetFetchStatus(int progress) {
			if (progress >= 0 && progress <= 100) {
				view.DrawProgressBox(progress);
				game.Dirty = true;
			}
		}
		
		void DownloadResources(int mouseX, int mouseY) {
			if (fetcher != null) return;
			
			fetcher = game.fetcher;
			fetcher.DownloadItems(game.Downloader, SetStatus);
			selectedWidget = null;
			
			widgets[view.yesIndex].Visible = false;
			widgets[view.noIndex].Visible = false;
			widgets[view.textIndex].Visible = false;
			widgets[view.cancelIndex].Visible = true;
			widgets[view.sliderIndex].Visible = true;
			Resize();
		}
		
		void GotoNextMenu(int x, int y) {
			if (File.Exists("options.txt")) {
				game.SetScreen(new MainScreen(game));
			} else {
				game.SetScreen(new ChooseModeScreen(game, true));
			}
		}
		
		void SetStatus(string text) {
			view.downloadingItems = true;
			view.RedrawStatus(text);
			game.Dirty = true;
		}
		
		public override void Dispose() {
			base.Dispose();
			view.Dispose();
		}
	}
}
