// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp;
using Launcher.Gui.Views;
using Launcher.Gui.Widgets;
using OpenTK.Input;

namespace Launcher.Gui.Screens {	
	public sealed class ServersScreen : InputScreen {
		
		const int tableX = 10, tableY = 50;
		ServersView view;
		
		public ServersScreen(LauncherWindow game) : base(game) {
			enterIndex = 3;
			view = new ServersView(game);
			widgets = view.widgets;
		}
		
		public override void Tick() {
			base.Tick();
			if (fetchingList) CheckFetchStatus();
			
			TableWidget table = (TableWidget)widgets[view.tableIndex];
			if (!game.Window.Mouse[MouseButton.Left]) {
				table.DraggingColumn = -1;
				table.DraggingScrollbar = false;
				table.mouseOffset = 0;
			}
		}
		
		protected override void MouseMove(int x, int y, int xDelta, int yDelta) {
			base.MouseMove(x, y, xDelta, yDelta);
			if (selectedWidget != null && selectedWidget == widgets[view.tableIndex]) {
				TableWidget table = (TableWidget)widgets[view.tableIndex];
				table.MouseMove(x, y, xDelta, yDelta);
			}
		}
		
		void MouseButtonUp(object sender, MouseButtonEventArgs e) {
			TableWidget table = (TableWidget)widgets[view.tableIndex];
			table.DraggingColumn = -1;
			table.DraggingScrollbar = false;
			table.mouseOffset = 0;
		}
		
		protected override void OnAddedChar() { FilterList(); }
		
		protected override void OnRemovedChar() { FilterList(); }
		
		protected override void KeyDown(object sender, KeyboardKeyEventArgs e) {
			TableWidget table = (TableWidget)widgets[view.tableIndex];
			if (e.Key == Key.Enter) {
				string curServer = Get(view.hashIndex) ?? "";
				if (table.Count >= 1 && curServer == "") {
					widgets[view.hashIndex].Text = table.usedEntries[0].Hash;
					ConnectToServer(0, 0);
				} else if (curServer != "" &&
				          (selectedWidget == null || selectedWidget == widgets[view.tableIndex])) {
					ConnectToServer(0, 0);
				}
			} else if (e.Key == Key.Up) {
				table.SetSelected(table.SelectedIndex - 1);
				table.NeedRedraw();
			} else if (e.Key == Key.Down) {
				table.SetSelected(table.SelectedIndex + 1);
				table.NeedRedraw();
			} else {
				base.KeyDown(sender, e);
			}
		}
		
		protected override void RedrawLastInput() {
			base.RedrawLastInput();
			if (curInput != widgets[view.hashIndex])
				return;
			TableWidget table = (TableWidget)widgets[view.tableIndex];
			table.SetSelected(widgets[view.hashIndex].Text);
			MarkPendingRedraw();
		}

		public override void Init() {
			base.Init();
			game.Window.Mouse.ButtonUp += MouseButtonUp;
			view.Init();
			SetupWidgetHandlers();
			
			Resize();
			selectedWidget = widgets[view.searchIndex];
			InputClick(0, 0);
			lastClicked = curInput;
		}
		
		public override void Resize() {
			view.DrawAll();
			game.Dirty = true;
		}
		
		void SetupWidgetHandlers() {
			InputWidget hashWidget = (InputWidget)widgets[view.hashIndex];
			hashWidget.Chars.ClipboardFilter = HashFilter;
			
			widgets[view.backIndex].OnClick =
				(x, y) => game.SetScreen(new MainScreen(game));
			widgets[view.connectIndex].OnClick = ConnectToServer;
			widgets[view.refreshIndex].OnClick = RefreshList;
			
			TableWidget widget = (TableWidget)widgets[view.tableIndex];
			widget.NeedRedraw = MarkPendingRedraw;
			widget.SelectedChanged = SelectedChanged;
			SetupInputHandlers();
		}
		
		void FilterList() {
			if (curInput != widgets[view.searchIndex])
				return;
			TableWidget table = (TableWidget)widgets[view.tableIndex];
			table.FilterEntries(curInput.Text);
			MarkPendingRedraw();
		}

		void SelectedChanged(string hash) {
			using (drawer) {
				drawer.SetBitmap(game.Framebuffer);
				Set(view.hashIndex, hash);
			}
			game.Dirty = true;
		}
		
		void ConnectToServer(int mouseX, int mouseY) {
			TableWidget table = (TableWidget)widgets[view.tableIndex];
			game.ConnectToServer(table.servers, Get(view.hashIndex));
		}
		
		bool fetchingList = false;
		void RefreshList(int mouseX, int mouseY) {
			if (fetchingList) return;
			fetchingList = true;
			game.Session.FetchServersAsync();

			view.RefreshText = "&eWorking..";
			Resize();
		}
		
		protected override void MouseWheelChanged(object sender, MouseWheelEventArgs e) {
			TableWidget table = (TableWidget)widgets[view.tableIndex];
			table.CurrentIndex -= e.Delta;
			MarkPendingRedraw();
		}
		
		string HashFilter(string input) {
			// Server url look like http://www.classicube.net/server/play/aaaaa/
			
			// Trim off the last / if it exists
			if (input[input.Length - 1] == '/')
				input = input.Substring(0, input.Length - 1);
			
			// Trim the parts before the hash
			int lastIndex = input.LastIndexOf('/');
			if (lastIndex >= 0)
				input = input.Substring(lastIndex + 1);
			return input;
		}
		
		public override void Dispose() {
			base.Dispose();
			view.Dispose();
			
			TableWidget table = widgets[view.tableIndex] as TableWidget;
			if (table != null) {
				table.DraggingColumn = -1;
				table.DraggingScrollbar = false;
			}
		}
		
		bool pendingRedraw;
		public override void OnDisplay() {
			if (pendingRedraw) {
				view.RedrawTable();
				game.Dirty = true;
			}
			pendingRedraw = false;
		}
		
		void CheckFetchStatus() {
			if (!game.Session.Done) return;
			fetchingList = false;
			
			view.RefreshText = game.Session.Exception == null ? "Refresh" : "&cFailed";
			Resize();
		}
		
		void MarkPendingRedraw() {
			TableWidget table = (TableWidget)widgets[view.tableIndex];
			table.ClampIndex();
			table.RecalculateDrawData();
			pendingRedraw = true;
			game.Dirty = true;
		}
	}
}
