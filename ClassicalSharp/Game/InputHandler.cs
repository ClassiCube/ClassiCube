// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Entities;
using ClassicalSharp.Gui;
using ClassicalSharp.Gui.Screens;
using ClassicalSharp.Hotkeys;
using ClassicalSharp.Map;
using OpenTK;
using OpenTK.Input;

namespace ClassicalSharp {

	public sealed class InputHandler {
		
		Game game;
		bool[] buttonsDown = new bool[3];
		PickingHandler picking;
		public InputHandler(Game game) {
			this.game = game;
			RegisterInputHandlers();
			Keys = new KeyMap();
			picking = new PickingHandler(game, this);
			HotkeyList.LoadSavedHotkeys();
		}
		
		void RegisterInputHandlers() {
			Keyboard.KeyDown += KeyDownHandler;
			Keyboard.KeyUp += KeyUpHandler;
			game.window.KeyPress += KeyPressHandler;
			Mouse.WheelChanged += MouseWheelChanged;
			Mouse.Move += MouseMove;
			Mouse.ButtonDown += MouseButtonDown;
			Mouse.ButtonUp += MouseButtonUp;
		}
		
		public bool WinDown { get { return IsKeyDown(Key.WinLeft) || IsKeyDown(Key.WinRight); } }
		public bool AltDown { get { return IsKeyDown(Key.AltLeft) || IsKeyDown(Key.AltRight); } }
		public bool ControlDown { get { return IsKeyDown(Key.ControlLeft) || IsKeyDown(Key.ControlRight); } }
		public bool ShiftDown { get { return IsKeyDown(Key.ShiftLeft) || IsKeyDown(Key.ShiftRight); } }
		
		public KeyMap Keys;
		public bool IsKeyDown(Key key) { return Keyboard.Get(key); }
		
		/// <summary> Returns whether the key associated with the given key binding is currently held down. </summary>
		public bool IsKeyDown(KeyBind binding) { return Keyboard.Get(Keys[binding]); }
		
		public bool IsMousePressed(MouseButton button) {
			bool down = Mouse.Get(button);
			if (down) return true;
			
			// Key --> mouse mappings
			if (button == MouseButton.Left   && IsKeyDown(KeyBind.MouseLeft)) return true;
			if (button == MouseButton.Middle && IsKeyDown(KeyBind.MouseMiddle)) return true;
			if (button == MouseButton.Right  && IsKeyDown(KeyBind.MouseRight)) return true;
			return false;
		}
		
		public void PickBlocks(bool cooldown, bool left, bool middle, bool right) {
			picking.PickBlocks(cooldown, left, middle, right);
		}
		
		// defer getting the targeted entity as it's a costly operation
		internal int pickingId = -1;
		internal void ButtonStateChanged(MouseButton button, bool pressed) {
			if (pressed) {
				// Can send multiple Pressed events
				ButtonStateUpdate(button, true);
			} else {
				if (!buttonsDown[(int)button]) return;
				ButtonStateUpdate(button, false);
			}
		}
		
		void ButtonStateUpdate(MouseButton button, bool pressed) {
			if (pickingId == -1) {
				pickingId = game.Entities.GetClosetPlayer(game.LocalPlayer);
			}
			
			game.Server.SendPlayerClick(button, pressed, (byte)pickingId, game.SelectedPos);
			buttonsDown[(int)button] = pressed;
		}
		
		internal void ScreenChanged(Screen oldScreen, Screen newScreen) {
			if (oldScreen != null && oldScreen.HandlesAllInput)
				picking.lastClick = DateTime.UtcNow;
			
			if (game.Server.UsingPlayerClick) {
				pickingId = -1;
				ButtonStateChanged(MouseButton.Left, false);
				ButtonStateChanged(MouseButton.Right, false);
				ButtonStateChanged(MouseButton.Middle, false);
			}
		}
		
		
		#region Event handlers
		
		void MouseButtonUp(MouseButton btn) {
			int x = Mouse.X, y = Mouse.Y;
			if (!game.Gui.ActiveScreen.HandlesMouseUp(x, y, btn)) {
				if (game.Server.UsingPlayerClick && btn <= MouseButton.Middle) {
					pickingId = -1;
					ButtonStateChanged(btn, false);
				}
			}
		}

		void MouseButtonDown(MouseButton btn) {
			int x = Mouse.X, y = Mouse.Y;
			if (!game.Gui.ActiveScreen.HandlesMouseDown(x, y, btn)) {
				bool left   = btn == MouseButton.Left;
				bool middle = btn == MouseButton.Middle;
				bool right  = btn == MouseButton.Right;
				PickBlocks(false, left, middle, right);
			} else {
				picking.lastClick = DateTime.UtcNow;
			}
		}

		void MouseMove(int deltaX, int deltaY) {
			int x = Mouse.X, y = Mouse.Y;
			game.Gui.ActiveScreen.HandlesMouseMove(x, y);
		}

		void MouseWheelChanged(float delta) {
			if (game.Gui.ActiveScreen.HandlesMouseScroll(delta)) return;
			
			Inventory inv = game.Inventory;
			bool hotbar = AltDown || ControlDown || ShiftDown;
			if ((!hotbar && game.Camera.Zoom(delta)) || DoFovZoom(delta) || !inv.CanChangeHeldBlock)
				return;
			
			game.Gui.hudScreen.hotbar.HandlesMouseScroll(delta);
		}

		void KeyPressHandler(char keyChar) {
			game.Gui.ActiveScreen.HandlesKeyPress(keyChar);
		}
		
		void KeyUpHandler(Key key) {
			if (SimulateMouse(key, false)) return;
			
			if (key == Keys[KeyBind.ZoomScrolling]) {
				SetFOV(game.DefaultFov, false);
			}
			game.Gui.ActiveScreen.HandlesKeyUp(key);
		}

		static int[] normViewDists = new int[] { 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096 };
		static int[] classicViewDists = new int[] { 8, 32, 128, 512 };
		void KeyDownHandler(Key key) {
			if (SimulateMouse(key, true)) return;
			
			if (IsShutdown(key)) {
				game.Exit();
			} else if (key == Keys[KeyBind.Screenshot]) {
				game.screenshotRequested = true;
			} else if (game.Gui.ActiveScreen.HandlesKeyDown(key)) {
			} else if (HandleCoreKey(key)) {
			} else if (game.LocalPlayer.HandlesKey(key)) {
			} else {
				int idx = HotkeyList.FindPartial(key, game.Input);
				if (idx == -1) return;
				
				Hotkey hotkey = HotkeyList.Hotkeys[idx];
				string text = hotkey.Text;
				
				if (!hotkey.StaysOpen) {
					game.Chat.Send(text, false);
				} else if (game.Gui.activeScreen == null) {
					game.Gui.hudScreen.OpenInput(text);
				}
			}
		}
		
		bool IsShutdown(Key key) {
			if (key == Key.F4 && AltDown) return true;
			// On OSX, Cmd+Q should also terminate the process.
			if (!OpenTK.Configuration.RunningOnMacOS) return false;
			return key == Key.Q && WinDown;
		}
		
		bool SimulateMouse(Key key, bool pressed) {
			Key left = Keys[KeyBind.MouseLeft], middle = Keys[KeyBind.MouseMiddle],
			right = Keys[KeyBind.MouseRight];
			
			if (!(key == left || key == middle || key == right)) return false;
			MouseButton btn = key == left ? MouseButton.Left : key == middle ? MouseButton.Middle : MouseButton.Right;
			
			if (pressed) MouseButtonDown(btn);
			else MouseButtonUp(btn);
			return true;
		}
		
		bool HandleNonClassicKey(Key key) {
			if (key == Keys[KeyBind.HideGui]) {
				game.HideGui = !game.HideGui;
			} else if (key == Keys[KeyBind.SmoothCamera]) {
				Toggle(key, ref game.SmoothCamera,
				       "  &eSmooth camera is &aenabled",
				       "  &eSmooth camera is &cdisabled");
			} else if (key == Keys[KeyBind.AxisLines]) {
				Toggle(key, ref game.ShowAxisLines,
				       "  &eAxis lines (&4X&e, &2Y&e, &1Z&e) now show",
				       "  &eAxis lines no longer show");
			} else if (key == Keys[KeyBind.Autorotate]) {
				Toggle(key, ref game.AutoRotate,
				       "  &eAuto rotate is &aenabled",
				       "  &eAuto rotate is &cdisabled");
			} else if (key == Keys[KeyBind.ThirdPerson]) {
				game.CycleCamera();
			} else if (key == game.Mapping(KeyBind.DropBlock)) {
				Inventory inv = game.Inventory;
				if (inv.CanChangeSelected() && inv.Selected != Block.Air) {
					// Don't assign Selected directly, because we don't want held block
					// switching positions if they already have air in their inventory hotbar.
					inv[inv.SelectedIndex] = Block.Air;
					Events.RaiseHeldBlockChanged();
				}
			} else if (key == Keys[KeyBind.IDOverlay]) {
				if (game.Gui.overlays.Count > 0) return true;
				game.Gui.ShowOverlay(new TexIdsOverlay(game), false);
			} else if (key == Keys[KeyBind.BreakableLiquids]) {
				Toggle(key, ref game.BreakableLiquids,
				       "  &eBreakable liquids is &aenabled",
				       "  &eBreakable liquids is &cdisabled");
			} else {
				return false;
			}
			return true;
		}
		
		bool HandleCoreKey(Key key) {
			if (key == Keys[KeyBind.HideFps]) {
				game.ShowFPS = !game.ShowFPS;
			} else if (key == Keys[KeyBind.Fullscreen]) {
				WindowState state = game.window.WindowState;
				if (state != WindowState.Minimized) {
					game.window.WindowState = state == WindowState.Fullscreen ?
						WindowState.Normal : WindowState.Fullscreen;
				}
			} else if (key == Keys[KeyBind.ToggleFog]) {
				int[] viewDists = game.UseClassicOptions ? classicViewDists : normViewDists;
				if (game.Input.ShiftDown) {
					CycleDistanceBackwards(viewDists);
				} else {
					CycleDistanceForwards(viewDists);
				}
			} else if ((key == Keys[KeyBind.PauseOrExit] || key == Key.Pause) && !game.Gui.ActiveScreen.HandlesAllInput) {
				game.Gui.SetNewScreen(new PauseScreen(game));
			} else if (key == game.Mapping(KeyBind.Inventory) && game.Gui.ActiveScreen == game.Gui.hudScreen) {
				game.Gui.SetNewScreen(new InventoryScreen(game));
			} else if (key == Key.F5 && game.ClassicMode) {
				Weather weather = game.World.Env.Weather == Weather.Sunny ? Weather.Rainy : Weather.Sunny;
				game.World.Env.SetWeather(weather);
			} else {
				if (game.ClassicMode) return false;
				return HandleNonClassicKey(key);
			}
			return true;
		}

		void Toggle(Key key, ref bool target, string enableMsg, string disableMsg) {
			target = !target;
			if (target) {
				game.Chat.Add(enableMsg + ". &ePress &a" + key + " &eto disable.");
			} else {
				game.Chat.Add(disableMsg + ". &ePress &a" + key + " &eto re-enable.");
			}
		}
		
		void CycleDistanceForwards(int[] viewDists) {
			for (int i = 0; i < viewDists.Length; i++) {
				int dist = viewDists[i];
				if (dist > game.UserViewDistance) {
					game.UserSetViewDistance(dist); return;
				}
			}
			game.UserSetViewDistance(viewDists[0]);
		}
		
		void CycleDistanceBackwards(int[] viewDists) {
			for (int i = viewDists.Length - 1; i >= 0; i--) {
				int dist = viewDists[i];
				if (dist < game.UserViewDistance) {
					game.UserSetViewDistance(dist); return;
				}
			}
			game.UserSetViewDistance(viewDists[viewDists.Length - 1]);
		}
		
		float fovIndex = -1;
		bool DoFovZoom(float deltaPrecise) {
			if (!game.IsKeyDown(KeyBind.ZoomScrolling)) return false;
			LocalPlayer p = game.LocalPlayer;
			if (!p.Hacks.Enabled || !p.Hacks.CanAnyHacks || !p.Hacks.CanUseThirdPersonCamera)
				return false;
			
			if (fovIndex == -1) fovIndex = game.ZoomFov;
			fovIndex -= deltaPrecise * 5;
			
			Utils.Clamp(ref fovIndex, 1, game.DefaultFov);
			return SetFOV((int)fovIndex, true);
		}
		
		public bool SetFOV(int fov, bool setZoom) {
			LocalPlayer p = game.LocalPlayer;
			if (game.Fov == fov) return true;
			if (!p.Hacks.Enabled || !p.Hacks.CanAnyHacks || !p.Hacks.CanUseThirdPersonCamera)
				return false;
			
			game.Fov = fov;
			if (setZoom) game.ZoomFov = fov;
			game.UpdateProjection();
			return true;
		}
		#endregion
	}
}