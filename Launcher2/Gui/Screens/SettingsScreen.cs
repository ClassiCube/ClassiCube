// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp;
using Launcher.Gui.Views;
using Launcher.Gui.Widgets;
using OpenTK.Input;

namespace Launcher.Gui.Screens {	
	public sealed class SettingsScreen : Screen {
		
		SettingsView view;
		public SettingsScreen( LauncherWindow game ) : base( game ) {
			view = new SettingsView( game );
			widgets = view.widgets;
		}

		public override void Init() {
			base.Init();
			view.Init();
			
			widgets[view.modeIndex].OnClick = (x, y) => 
				game.SetScreen( new ChooseModeScreen( game, false ) );
			widgets[view.updatesIndex].OnClick = (x, y) => 
				game.SetScreen( new UpdatesScreen( game ) );
			widgets[view.coloursIndex].OnClick = (x, y) =>
				game.SetScreen( new ColoursScreen( game ) );
			widgets[view.backIndex].OnClick = (x, y) =>
				game.SetScreen( new MainScreen( game ) );
			Resize();
		}
		
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
