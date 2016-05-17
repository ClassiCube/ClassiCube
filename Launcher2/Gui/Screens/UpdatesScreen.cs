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
	public sealed class UpdatesScreen : LauncherScreen {
		
		UpdatesView view;
		public UpdatesScreen( LauncherWindow game ) : base( game ) {
			game.Window.Mouse.Move += MouseMove;
			game.Window.Mouse.ButtonDown += MouseButtonDown;
			
			view = new UpdatesView( game );
			widgets = view.widgets;
		}

		UpdateCheckTask checkTask;
		public override void Init() {
			view.Init();
			if( game.checkTask != null && game.checkTask.Done )
				SuccessfulUpdateCheck( game.checkTask );
			checkTask = new UpdateCheckTask();
			checkTask.CheckForUpdatesAsync();
			
			game.Window.Keyboard.KeyDown += KeyDown;
			game.Window.Keyboard.KeyUp += KeyUp;
			SetWidgetHandlers();
			Resize();
		}

		void KeyDown( object sender, KeyboardKeyEventArgs e ) {
			if( e.Key == Key.Tab ) {
				HandleTab();
			} else if( e.Key == Key.Enter ) {
				LauncherWidget widget = selectedWidget;
				if( widget != null && widget.OnClick != null )
					widget.OnClick( 0, 0 );
			}
		}
		
		void KeyUp( object sender, KeyboardKeyEventArgs e ) {
			if( e.Key == Key.Tab )
				tabDown = false;
		}

		Build dev, stable;
		public override void Tick() {;
			if( checkTask.Done && checkTask.Exception == null ) return;
			if( !checkTask.TaskTick( SuccessfulUpdateCheck ) ) {
				view.LastStable = DateTime.MaxValue;
				view.LastDev = DateTime.MaxValue;
				checkTask.Exception = null;
				
				LauncherWidget w = widgets[view.devIndex - 1];
				game.ClearArea( w.X, w.Y, w.Width, w.Height );
				w = widgets[view.relIndex - 1];
				game.ClearArea( w.X, w.Y, w.Width, w.Height );
				Resize();
			}
		}
		
		void SuccessfulUpdateCheck( UpdateCheckTask task ) {
			if( task.LatestDev == null || task.LatestStable == null ) return;
			dev = task.LatestDev; view.LastDev = dev.TimeBuilt;
			stable = task.LatestStable; view.LastStable = stable.TimeBuilt;
			Resize();
		}
		
		public override void Resize() {
			view.DrawAll();
			Dirty = true;
		}
		
		void SetWidgetHandlers() {
			widgets[view.relIndex].OnClick = (x, y) => UpdateBuild( true, true );
			widgets[view.relIndex + 1].OnClick = (x, y) => UpdateBuild( true, false );

			widgets[view.devIndex].OnClick =  (x, y) => UpdateBuild( false, true );
			widgets[view.devIndex + 1].OnClick = (x, y) => UpdateBuild( false, false );
			
			widgets[view.backIndex].OnClick =
				(x, y) => game.SetScreen( new MainScreen( game ) );
		}
		
		void UpdateBuild( bool release, bool dx ) {
			DateTime last = release ? view.LastStable : view.LastDev;
			Build build = release ? stable : dev;
			if( last == DateTime.MinValue || build.DirectXSize < 50000
			   || build.OpenGLSize < 50000 ) return;
			
			view.gameOpen = CheckClientInstances();
			view.SetWarning();
			LauncherWidget widget = widgets[view.statusIndex];
			game.ClearArea( widget.X, widget.Y, widget.Width, widget.Height );
			RedrawWidget( widgets[view.statusIndex] );
			if( view.gameOpen ) return;
			
			string path = dx ? build.DirectXPath : build.OpenGLPath;
			Utils.LogDebug( "Updating to: " + path );
			Applier.PatchTime = build.TimeBuilt;
			Applier.FetchUpdate( path );
			game.ShouldExit = true;
			game.ShouldUpdate = true;
		}
		
		bool CheckClientInstances() {
			Process[] processes = Process.GetProcesses();
			for( int i = 0; i < processes.Length; i++ ) {
				string name = null;
				try {
					name = processes[i].ProcessName;
				} catch {
					continue;
				}
				
				if( Utils.CaselessEquals( name, "ClassicalSharp" )
				   || Utils.CaselessEquals( name, "ClassicalSharp.exe" ) )
				   return true;
				}
			return false;
		}
		
		public override void Dispose() {
			game.Window.Keyboard.KeyDown -= KeyDown;
			game.Window.Keyboard.KeyUp -= KeyUp;
			game.Window.Mouse.Move -= MouseMove;
			game.Window.Mouse.ButtonDown -= MouseButtonDown;
			view.Dispose();
		}
	}
}
