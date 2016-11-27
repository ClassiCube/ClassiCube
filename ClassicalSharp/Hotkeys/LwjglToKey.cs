// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using OpenTK.Input;

namespace ClassicalSharp.Hotkeys {

	/// <summary> Maps LWJGL keycodes to OpenTK keys. </summary>
	public static class LwjglToKey {
		
		public static Key[] Map = new Key[256];
		
		static int curIndex = 0;
		static void Add(Key key) {
			Map[curIndex++] = key;
		}
		
		static void Add(string value) {
			for (int i = 0; i < value.Length; i++) {
				Add((Key)(value[i] - 'A' + (int)Key.A));
			}
		}
		
		static void Skip(int amount) {
			curIndex += amount;
		}
		
		static LwjglToKey() {
			Add(Key.Unknown); Add(Key.Escape);
			for (int i = 0; i < 9; i++)
				Add((Key)(i + Key.Number1));
			Add(Key.Number0); Add(Key.Minus);
			Add(Key.Plus); Add(Key.BackSpace);
			Add(Key.Tab); Add("QWERTYUIOP");
			Add(Key.BracketLeft); Add(Key.BracketRight);
			Add(Key.Enter); Add(Key.ControlLeft);
			Add("ASDFGHJKL"); Add(Key.Semicolon);
			Add(Key.Quote); Add(Key.Tilde);
			Add(Key.ShiftLeft); Add(Key.BackSlash);
			Add("ZXCVBNM"); Add(Key.Comma);
			Add(Key.Period); Add(Key.Slash);
			Add(Key.ShiftRight); Skip(1); // TODO: multiply
			Add(Key.AltLeft); Add(Key.Space);
			Add(Key.CapsLock);
			for (int i = 0; i < 10; i++)
				Add((Key)(i + Key.F1));
			Add(Key.NumLock); Add(Key.ScrollLock);
			Add(Key.Number7); Add(Key.Number8);
			Add(Key.Number9); Add(Key.KeypadSubtract);
			Add(Key.Number4); Add(Key.Number5);
			Add(Key.Number6); Add(Key.KeypadAdd);
			Add(Key.Number1); Add(Key.Number2);
			Add(Key.Number3); Add(Key.Number0);
			Add(Key.KeypadDecimal); Skip(3);
			Add(Key.F11); Add(Key.F12);
			Skip(11);
			for (int i = 0; i < 6; i++)
				Add((Key)(i + Key.F13));
			Skip(35); Add(Key.KeypadAdd);
			Skip(14); Add(Key.KeypadEnter);
			Add(Key.ControlRight); Skip(23);
			Add(Key.KeypadDivide); Skip(2);
			Add(Key.AltRight); Skip(12);
			Add(Key.Pause); Skip(1);
			Add(Key.Home); Add(Key.Up);
			Add(Key.PageUp); Skip(1);
			Add(Key.Left); Skip(1);
			Add(Key.Right); Skip(1);
			Add(Key.End); Add(Key.Down);
			Add(Key.PageDown); Add(Key.Insert);
			Add(Key.Delete); Skip(7);
			Add(Key.WinLeft); Add(Key.WinRight);
		}
	}
}