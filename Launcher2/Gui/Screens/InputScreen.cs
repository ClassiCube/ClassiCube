// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp;
using Launcher.Gui.Widgets;
using OpenTK;
using OpenTK.Input;

namespace Launcher.Gui.Screens {	
	public abstract class InputScreen : Screen {
		
		protected int enterIndex = -1;
		public InputScreen(LauncherWindow game) : base(game) { }
		
		public override void Init() {
			base.Init();
			Mouse.WheelChanged += MouseWheelChanged;
			game.Window.KeyPress += KeyPress;
			Keyboard.KeyRepeat = true;
			last = DateTime.UtcNow;
		}
		
		DateTime last;
		bool lastCaretShow = false;
		Rectangle lastRec;
		double elapsed;
		
		public override void Tick() {
			DateTime now = DateTime.UtcNow;
			elapsed += (now - last).TotalSeconds;
			last = now;
			
			bool caretShow = (elapsed % 1) < 0.5;
			if (caretShow == lastCaretShow || curInput == null) return;
			lastCaretShow = caretShow;
			
			using (drawer) {
				drawer.SetBitmap(game.Framebuffer);
				curInput.SetDrawData(drawer, curInput.Text);
				curInput.Redraw(drawer);
				
				Rectangle r = curInput.MeasureCaret(drawer);
				if (caretShow) {
					drawer.Clear(PackedCol.Black, r.X, r.Y, r.Width, r.Height);
				}
				
				if (lastRec == r) game.DirtyArea = r;
				lastRec = r;
				game.Dirty = true;
			}
		}
		
		protected override void KeyDown(Key key) {
			if (key == Key.Enter && enterIndex >= 0) {
				Widget widget = (selectedWidget != null && mouseMoved) ?
					selectedWidget : widgets[enterIndex];
				if (widget.OnClick != null)
					widget.OnClick(0, 0);
			} else if (key == Key.Tab) {
				HandleTab();
			}
			if (curInput == null) {
				if (key == Key.Escape)
					game.SetScreen(new MainScreen(game));
				return;
			}
			
			if (key == Key.BackSpace && curInput.Chars.Backspace()) {
				RedrawLastInput();
				OnRemovedChar();
			} else if (key == Key.Delete && curInput.Chars.Delete()) {
				RedrawLastInput();
				OnRemovedChar();
			} else if (key == Key.C && ControlDown) {
				if (String.IsNullOrEmpty(curInput.Text)) return;				
				game.Window.SetClipboardText(curInput.Text);
			} else if (key == Key.V && ControlDown) {
				string text = game.Window.GetClipboardText();
				if (curInput.Chars.CopyFromClipboard(text)) RedrawLastInput();
			} else if (key == Key.Escape) {
				if (curInput.Chars.Clear()) RedrawLastInput();
			} else if (key == Key.Left) {
				curInput.AdvanceCaretPos(false);
				RedrawLastInput();
			} else if (key == Key.Right) {
				curInput.AdvanceCaretPos(true);
				RedrawLastInput();
			}
		}
		
		bool ControlDown {
			get {
				return game.IsKeyDown(Key.ControlLeft)
					|| game.IsKeyDown(Key.ControlRight);
			}
		}

		protected void KeyPress(char keyChar) {
			if (curInput != null && curInput.Chars.Append(keyChar)) {
				RedrawLastInput();
				OnAddedChar();
			}
		}
		
		protected virtual void RedrawLastInput() {
			if (curInput.RealWidth > curInput.ButtonWidth) {
				game.ResetArea(curInput.X, curInput.Y, curInput.RealWidth, curInput.Height);
			}
			elapsed = 0; lastCaretShow = false;
			
			using (drawer) {
				drawer.SetBitmap(game.Framebuffer);
				curInput.Redraw(drawer);
				game.Dirty = true;
			}
		}
		
		protected virtual void OnAddedChar() { }
		
		protected virtual void OnRemovedChar() { }
		
		protected string Get(int index) {
			Widget widget = widgets[index];
			return widget == null ? "" : widget.Text;
		}
		
		protected void Set(int index, string text) {
			((InputWidget)widgets[index]).SetDrawData(drawer, text);
			((InputWidget)widgets[index]).Redraw(drawer);
		}
		
		protected virtual void MouseWheelChanged(float delta) { }
		
		protected InputWidget curInput;
		protected virtual void InputClick(int mouseX, int mouseY) {
			InputWidget input = (InputWidget)selectedWidget;
			using (drawer) {
				drawer.SetBitmap(game.Framebuffer);
				if (curInput != null) {
					curInput.Active = false;
					curInput.Redraw(drawer);
				}
				
				input.Active = true;
				elapsed = 0; lastCaretShow = false;
				input.SetCaretToCursor(mouseX, mouseY, drawer);
				input.Redraw(drawer);
			}
			curInput = input;
			game.Dirty = true;
		}
		
		protected override void WidgetUnclicked(Widget widget) {
			InputWidget input = widget as InputWidget;
			if (input == null) return;
			input.Active = false;
			RedrawWidget(input);
			curInput = null;
			game.Dirty = true;
		}
		
		protected void SetupInputHandlers() {
			for (int i = 0; i < widgets.Length; i++) {
				if (!(widgets[i] is InputWidget)) continue;
				widgets[i].OnClick = InputClick;
			}
		}
		
		public override void Dispose() {
			base.Dispose();
			Mouse.WheelChanged -= MouseWheelChanged;			
			game.Window.KeyPress -= KeyPress;
			Keyboard.KeyRepeat = false;
		}
	}
}
