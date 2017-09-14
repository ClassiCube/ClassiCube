// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp;
using Launcher.Gui.Views;
using Launcher.Gui.Widgets;
using OpenTK.Input;

namespace Launcher.Gui.Screens {	
	public sealed class SettingsScreen : Screen {
		
		SettingsView view;
		public SettingsScreen(LauncherWindow game) : base(game) {
			view = new SettingsView(game);
			widgets = view.widgets;
		}

		public override void Init() {
			base.Init();
			view.Init();
			
			widgets[view.modeIndex].OnClick = SwitchToChooseMode;
			widgets[view.updatesIndex].OnClick = SwitchToUpdates;
			widgets[view.coloursIndex].OnClick = SwitchToColours;
			widgets[view.backIndex].OnClick = SwitchToMain;
			Resize();
		}
		
		void SwitchToChooseMode(int x, int y) { game.SetScreen(new ChooseModeScreen(game, false)); }
		void SwitchToUpdates(int x, int y) { game.SetScreen(new UpdatesScreen(game)); }
		void SwitchToColours(int x, int y) { game.SetScreen(new ColoursScreen(game)); }
		void SwitchToMain(int x, int y) { game.SetScreen(new MainScreen(game)); }
		
		public override void Tick() { }

		public override void Resize() {
			view.DrawAll();
			game.Dirty = true;
		}

		public override void Dispose() {
			base.Dispose();
			view.Dispose();
		}
	}
}
