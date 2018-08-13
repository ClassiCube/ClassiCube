// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
#if !ANDROID
using System;
using ClassicalSharp.Gui.Widgets;
using ClassicalSharp.Hotkeys;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	public class EditHotkeyScreen : MenuScreen {
		
		const int keyI = 0, modifyI = 1, actionI = 2;
		Hotkey curHotkey, origHotkey;
		int selectedI = -1;
		static PackedCol grey = new PackedCol(150, 150, 150);
		
		public EditHotkeyScreen(Game game, Hotkey original) : base(game) {
			origHotkey = original;
			curHotkey = original;
		}
		
		static string MakeFlagsString(byte flags) {
			if (flags == 0) return " None";			
			return HotkeyListScreen.MakeFlagsString(flags);
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
			Keyboard.KeyRepeat = true;
			ContextRecreated();
		}
		
		protected override void ContextRecreated() {
			string flags = MakeFlagsString(curHotkey.Flags);
			if (curHotkey.Text == null) curHotkey.Text = "";
			string staysOpen = curHotkey.StaysOpen ? "ON" : "OFF";
			bool existed = origHotkey.Trigger != Key.None;
			
			InputWidget input;
			input = MenuInputWidget.Create(game, 500, 30, curHotkey.Text, textFont, new StringValidator())
					.SetLocation(Anchor.Centre, Anchor.Centre, 0, -35);
			input.ShowCaret = true;
			
			widgets = new Widget[] {
				Make(0, -150, "Key: " + curHotkey.Trigger, BaseKeyClick),
				Make(0, -100, "Modifiers:" + flags, ModifiersClick),
				input,
				Make(-100, 10, "Input stays open: " + staysOpen, LeaveOpenClick),
				
				Make(0, 80, existed ? "Save changes" : "Add hotkey", SaveChangesClick),
				Make(0, 130, existed ? "Remove hotkey" : "Cancel", RemoveHotkeyClick),
				MakeBack(false, titleFont, SwitchPause),
			};
		}
		
		public override void Dispose() {
			Keyboard.KeyRepeat = false;
			selectedI = -1;
			base.Dispose();
		}
		
		ButtonWidget Make(int x, int y, string text, ClickHandler onClick) {
			return ButtonWidget.Create(game, 300, text, titleFont, onClick)
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
			if (origHotkey.Trigger != Key.None) {
				HotkeyList.Remove(origHotkey.Trigger, origHotkey.Flags);
				HotkeyList.UserRemovedHotkey(origHotkey.Trigger, origHotkey.Flags);
			}
			MenuInputWidget input = (MenuInputWidget)widgets[actionI];
			
			if (curHotkey.Trigger != Key.None) {
				HotkeyList.Add(curHotkey.Trigger, curHotkey.Flags,
				                  input.Text.ToString(), curHotkey.StaysOpen);
				HotkeyList.UserAddedHotkey(curHotkey.Trigger, curHotkey.Flags,
				                        curHotkey.StaysOpen, input.Text.ToString());
			}
			game.Gui.SetNewScreen(new HotkeyListScreen(game));
		}
		
		void RemoveHotkeyClick(Game game, Widget widget) {
			if (origHotkey.Trigger != Key.None) {
				HotkeyList.Remove(origHotkey.Trigger, origHotkey.Flags);
				HotkeyList.UserRemovedHotkey(origHotkey.Trigger, origHotkey.Flags);
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
				curHotkey.Trigger = key;
				SetButton(keyI, "Key: " + curHotkey.Trigger);
				supressNextPress = true;
			} else if (selectedI == modifyI) {
				if (key == Key.ControlLeft || key == Key.ControlRight) curHotkey.Flags |= 1;
				else if (key == Key.ShiftLeft || key == Key.ShiftRight) curHotkey.Flags |= 2;
				else if (key == Key.AltLeft || key == Key.AltRight) curHotkey.Flags |= 4;
				else curHotkey.Flags = 0;
				
				string flags = MakeFlagsString(curHotkey.Flags);
				SetButton(modifyI, "Modifiers:" + flags);
				supressNextPress = true;
			}
			selectedI = -1;
		}
		
		void LostFocus() {
			if (selectedI == -1) return;
			
			if (selectedI == keyI) {
				SetButton(keyI, "Key: " + curHotkey.Trigger);
			} else if (selectedI == modifyI) {
				string flags = MakeFlagsString(curHotkey.Flags);
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