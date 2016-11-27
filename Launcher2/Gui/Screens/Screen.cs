// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp;
using Launcher.Gui.Widgets;
using OpenTK.Input;

namespace Launcher.Gui.Screens {
	public abstract class Screen {
		protected internal LauncherWindow game;
		protected internal IDrawer2D drawer;

		protected bool mouseMoved = false;
		static bool supressMove = true;
		
		public Screen(LauncherWindow game) {
			this.game = game;
			drawer = game.Drawer;
		}
		
		/// <summary> Function called to setup the widgets and other data for this screen. </summary>
		public virtual void Init() {
			game.Window.Mouse.Move += MouseMove;
			game.Window.Mouse.ButtonDown += MouseButtonDown;
			game.Window.Keyboard.KeyDown += KeyDown;
			game.Window.Keyboard.KeyUp += KeyUp;
		}
		
		/// <summary> Function that is repeatedly called multiple times every second. </summary>
		public abstract void Tick();
		
		/// <summary> Function called to redraw all widgets in this screen. </summary>
		public abstract void Resize();
		
		/// <summary> Cleans up all native resources held by this screen. </summary>
		public virtual void Dispose() {
			game.Window.Mouse.Move -= MouseMove;
			game.Window.Mouse.ButtonDown -= MouseButtonDown;
			game.Window.Keyboard.KeyDown -= KeyDown;
			game.Window.Keyboard.KeyUp -= KeyUp;
		}
		
		/// <summary> Function called when the pixels from the framebuffer
		/// are about to be transferred to the window. </summary>
		public virtual void OnDisplay() { }
		
		protected Widget selectedWidget;
		protected Widget[] widgets;
		
		/// <summary> Called when the user has moved their mouse away from a previously selected widget. </summary>
		protected virtual void UnselectWidget(Widget widget) {
			if (widget != null) widget.Active = false;
			ChangedActiveState(widget);
		}
		
		/// <summary> Called when user has moved their mouse over a given widget. </summary>
		protected virtual void SelectWidget(Widget widget, int mouseX, int mouseY) {
			if (widget != null) widget.Active = true;
			ChangedActiveState(widget);
		}
		
		void ChangedActiveState(Widget widget) {
			ButtonWidget button = widget as ButtonWidget;
			if (button != null)  {
				button.RedrawBackground();
				RedrawWidget(button);
			}
			
			LabelWidget label = widget as LabelWidget;
			if (label != null && label.DarkenWhenInactive) {
				game.ResetArea(label.X, label.Y, label.Width, label.Height);
				RedrawWidget(label);
			}
			
			BitmapWidget bitmap = widget as BitmapWidget;
			if (bitmap != null) RedrawWidget(bitmap);
		}
		
		/// <summary>Redraws the given widget and marks the window as needing to be redrawn. </summary>
		protected void RedrawWidget(Widget widget) {
			using (drawer) {
				drawer.SetBitmap(game.Framebuffer);
				widget.Redraw(drawer);
			}
			game.Dirty = true;
		}
		
		
		protected Widget lastClicked;
		void MouseButtonDown(object sender, MouseButtonEventArgs e) {
			MouseButtonDown(e.X, e.Y, e.Button);
		}
		
		void MouseMove(object sender, MouseMoveEventArgs e) { 
			MouseMove(e.X, e.Y, e.XDelta, e.YDelta); 
		}
		
		protected virtual void MouseButtonDown(int x, int y, MouseButton button) {
			if (button != MouseButton.Left) return;
			
			if (lastClicked != null && lastClicked != selectedWidget)
				WidgetUnclicked(lastClicked);
			if (selectedWidget != null && selectedWidget.OnClick != null)
				selectedWidget.OnClick(x, y);
			lastClicked = selectedWidget;
		}
		
		protected virtual void WidgetUnclicked(Widget widget) { }
		
		protected virtual void MouseMove(int x, int y, int xDelta, int yDelta) {
			if (supressMove) { supressMove = false; return; }
			mouseMoved = true;
			
			for (int i = 0; i < widgets.Length; i++) {
				Widget widget = widgets[i];
				if (!widget.Visible) continue;
				int width = widget.Width, height = widget.Height;
				if (widgets[i] is InputWidget)
					width = ((InputWidget)widgets[i]).RealWidth;
				
				if (x >= widget.X && y >= widget.Y &&
				   x < widget.X + width && y < widget.Y + height) {
					if (selectedWidget == widget) return;
					
					if (selectedWidget != null)
						UnselectWidget(selectedWidget);
					SelectWidget(widget, x, y);
					selectedWidget = widget;
					return;
				}
			}
			
			if (selectedWidget == null) return;
			UnselectWidget(selectedWidget);
			selectedWidget = null;
		}
		
		
		protected virtual void KeyDown(object sender, KeyboardKeyEventArgs e) {
			if (e.Key == Key.Tab) {
				HandleTab();
			} else if (e.Key == Key.Enter) {
				Widget widget = selectedWidget;
				if (widget != null && widget.OnClick != null)
					widget.OnClick(0, 0);
			}
		}
		
		protected virtual void KeyUp(object sender, KeyboardKeyEventArgs e) {
			if (e.Key == Key.Tab)
				tabDown = false;
		}
		
		protected bool tabDown = false;
		protected void HandleTab() {
			if (tabDown) return;
			tabDown = true;
			bool shiftDown = game.Window.Keyboard[Key.ShiftLeft]
				|| game.Window.Keyboard[Key.ShiftRight];
			
			int dir = shiftDown ? -1 : 1;
			int index = 0;
			if (lastClicked != null) {
				index = Array.IndexOf<Widget>(widgets, lastClicked);
				index += dir;
			}
			
			for (int j = 0; j < widgets.Length * 2; j++) {
				int i = (index + j * dir) % widgets.Length;
				if (i < 0) i += widgets.Length;
				if (!widgets[i].Visible || !widgets[i].TabSelectable) continue;
				
				Widget widget = widgets[i];
				int width = widget.Width;
				if (widgets[i] is InputWidget)
					width = ((InputWidget)widgets[i]).RealWidth;
				int mouseX = widget.X + width / 2;
				int mouseY = widget.Y + widget.Height / 2;
				
				MouseMove(mouseX, mouseY, 0, 0);
				Point p = game.Window.PointToScreen(Point.Empty);
				p.Offset(mouseX, mouseY);
				game.Window.DesktopCursorPos = p;
				
				lastClicked = widget;				
				if (widgets[i] is InputWidget) {
					MouseButtonDown(mouseX, mouseY, MouseButton.Left);
					((InputWidget)widgets[i]).Chars.CaretPos = -1;
				}
				break;
			}
		}
	}
}
