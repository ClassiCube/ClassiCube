// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
#if !ANDROID
using System;
using System.Drawing;
using ClassicalSharp.Gui.Widgets;
using ClassicalSharp.Hotkeys;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	public sealed class EditHotkeyScreen : MenuScreen {
		
		const int keyI = 0, modifyI = 1, actionI = 2;
		HotkeyList hotkeys;
		Hotkey curHotkey, origHotkey;
		int selectedI = -1;
		static FastColour grey = new FastColour(150, 150, 150);
		
		public EditHotkeyScreen(Game game, Hotkey original) : base(game) {
			hotkeys = game.Input.Hotkeys;
			origHotkey = original;
			curHotkey = original;
		}
		
		public override void Render(double delta) {
			base.Render(delta);
			int cX = game.Width / 2, cY = game.Height / 2;
			game.Graphics.Draw2DQuad(cX - 250, cY - 65, 500, 2, grey);
			game.Graphics.Draw2DQuad(cX - 250, cY + 45, 500, 2, grey);
		}
		
		bool supressNextPress;
		public override bool HandlesKeyPress(char key) {
			if (supressNextPress) {
				supressNextPress = false;
				return true;
			}
			return widgets[actionI].HandlesKeyPress(key);
		}
		
		public override bool HandlesKeyDown(Key key) {
			if (selectedI >= 0) {
				FocusKeyDown(key); return true;
			}
			return widgets[actionI].HandlesKeyDown(key) || base.HandlesKeyDown(key);
		}
		
		public override bool HandlesKeyUp(Key key) {
			return widgets[actionI].HandlesKeyUp(key);
		}
		
		public override void Init() {
			base.Init();
			game.Keyboard.KeyRepeat = true;
			titleFont = new Font(game.FontName, 16, FontStyle.Bold);
			regularFont = new Font(game.FontName, 16);
			ContextRecreated();
		}
		
		protected override void ContextRecreated() {
			string flags = HotkeyListScreen.MakeFlagsString(curHotkey.Flags);
			if (curHotkey.Text == null) curHotkey.Text = "";
			string staysOpen = curHotkey.StaysOpen ? "ON" : "OFF";
			bool existed = origHotkey.BaseKey != Key.Unknown;
			
			InputWidget input;
			input = MenuInputWidget.Create(game, 500, 30, curHotkey.Text, regularFont, new StringValidator())
					.SetLocation(Anchor.Centre, Anchor.Centre, 0, -35);
			input.ShowCaret = true;
			
			widgets = new Widget[] {
				Make(0, -150, "Key: " + curHotkey.BaseKey, 300, titleFont, BaseKeyClick),
				Make(0, -100, "Modifiers:" + flags, 300, titleFont, ModifiersClick),			
				input,
				Make(-100, 10, "Input stays open: " + staysOpen, 300, titleFont, LeaveOpenClick),
				
				Make(0, 80, existed ? "Save changes" : "Add hotkey", 300, titleFont, SaveChangesClick),
				Make(0, 130, existed ? "Remove hotkey" : "Cancel", 300, titleFont, RemoveHotkeyClick),				
				MakeBack(false, titleFont, SwitchPause),
			};
		}
		
		public override void Dispose() {
			game.Keyboard.KeyRepeat = false;
			selectedI = -1;
			base.Dispose();
		}
		
		ButtonWidget Make(int x, int y, string text, int width, Font font, ClickHandler onClick) {
			return ButtonWidget.Create(game, width, text, font, onClick)
				.SetLocation(Anchor.Centre, Anchor.Centre, x, y);
		}
		
		void LeaveOpenClick(Game game, Widget widget) {
			LostFocus();
			curHotkey.StaysOpen = !curHotkey.StaysOpen;
			string staysOpen = curHotkey.StaysOpen ? "ON" : "OFF";
			staysOpen = "Input stays open: " + staysOpen;
			SetButton(3, staysOpen);
		}
		
		void SaveChangesClick(Game game, Widget widget) {
			if (origHotkey.BaseKey != Key.Unknown) {
				hotkeys.RemoveHotkey(origHotkey.BaseKey, origHotkey.Flags);
				hotkeys.UserRemovedHotkey(origHotkey.BaseKey, origHotkey.Flags);
			}
			MenuInputWidget input = (MenuInputWidget)widgets[actionI];
			
			if (curHotkey.BaseKey != Key.Unknown) {
				hotkeys.AddHotkey(curHotkey.BaseKey, curHotkey.Flags,
				                  input.Text.ToString(), curHotkey.StaysOpen);
				hotkeys.UserAddedHotkey(curHotkey.BaseKey, curHotkey.Flags,
				                        curHotkey.StaysOpen, input.Text.ToString());
			}
			game.Gui.SetNewScreen(new HotkeyListScreen(game));
		}
		
		void RemoveHotkeyClick(Game game, Widget widget) {
			if (origHotkey.BaseKey != Key.Unknown) {
				hotkeys.RemoveHotkey(origHotkey.BaseKey, origHotkey.Flags);
				hotkeys.UserRemovedHotkey(origHotkey.BaseKey, origHotkey.Flags);
			}
			game.Gui.SetNewScreen(new HotkeyListScreen(game));
		}
		
		void BaseKeyClick(Game game, Widget widget) {
			selectedI = keyI;
			SetButton(keyI, "Key: press a key..");
			supressNextPress = true;
		}
		
		void ModifiersClick(Game game, Widget widget) {
			selectedI = modifyI;
			SetButton(modifyI, "Modifiers: press a key..");
			supressNextPress = true;
		}
		
		void FocusKeyDown(Key key) {
			if (selectedI == keyI) {
				curHotkey.BaseKey = key;
				SetButton(keyI, "Key: " + curHotkey.BaseKey);
				supressNextPress = true;
			} else if (selectedI == modifyI) {
				if (key == Key.ControlLeft || key == Key.ControlRight) curHotkey.Flags |= 1;
				else if (key == Key.ShiftLeft || key == Key.ShiftRight) curHotkey.Flags |= 2;
				else if (key == Key.AltLeft || key == Key.AltRight) curHotkey.Flags |= 4;
				else curHotkey.Flags = 0;
				
				string flags = HotkeyListScreen.MakeFlagsString(curHotkey.Flags);
				SetButton(modifyI, "Modifiers:" + flags);
				supressNextPress = true;
			}
			selectedI = -1;
		}
		
		void LostFocus() {
			if (selectedI == -1) return;
			
			if (selectedI == keyI) {
				SetButton(keyI, "Key: " + curHotkey.BaseKey);
			} else if (selectedI == modifyI) {
				string flags = HotkeyListScreen.MakeFlagsString(curHotkey.Flags);
				SetButton(modifyI, "Modifiers:" + flags);
			}
			selectedI = -1;
			supressNextPress = false;
		}
		
		void SetButton(int i, string text) {
			((ButtonWidget)widgets[i]).SetText(text);
		}
	}
}
#endif