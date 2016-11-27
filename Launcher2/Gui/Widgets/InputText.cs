// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using System.Windows.Forms;
using ClassicalSharp;

namespace Launcher.Gui.Widgets {
	/// <summary> Widget that represents text can have modified by the user. </summary>
	public sealed class InputText {
		
		/// <summary> Maximum number of characters that the 'Text' field can contain. </summary>
		public int MaxChars = 32;
		
		/// <summary> Filter applied to text received from the clipboard. Can be null. </summary>
		public Func<string, string> ClipboardFilter;
		
		/// <summary> Delegate invoked when the text changes. </summary>
		public Action<InputWidget> TextChanged;
		
		/// <summary> Delegate that only lets certain characters be entered. </summary>
		public Func<char, bool> TextFilter;
		
		/// <summary> Specifies the position that characters are inserted/deleted from. </summary>
		/// <remarks> -1 to insert/delete characters at end of the text. </remarks>
		public int CaretPos = -1;
		
		InputWidget input;
		public InputText(InputWidget input) {
			this.input = input;
		}
		
		/// <summary> Appends a character to the end of the currently entered text. </summary>
		/// <returns> true if a redraw is necessary, false otherwise. </returns>
		public bool Append(char c) {
			if (TextFilter != null && !TextFilter(c))
				return false;
			if (c >= ' ' && c <= '~' && c != '&' && input.Text.Length < MaxChars) {
				if (CaretPos == -1) {
					input.Text += c;
				} else {
					input.Text = input.Text.Insert(CaretPos, new String(c, 1));
					CaretPos++;
				}
				if (TextChanged != null) TextChanged(input);
				return true;
			}
			return false;
		}
		
		/// <summary> Removes the character preceding the caret in the currently entered text. </summary>
		/// <returns> true if a redraw is necessary, false otherwise. </returns>
		public bool Backspace() {
			if (input.Text.Length == 0) return false;
			
			if (CaretPos == -1) {
				input.Text = input.Text.Substring(0, input.Text.Length - 1);
			} else {
				if (CaretPos == 0) return false;
				input.Text = input.Text.Remove(CaretPos - 1, 1);
				CaretPos--;
				if (CaretPos == -1) CaretPos = 0;
			}
			
			if (TextChanged != null) TextChanged(input);
			if (CaretPos >= input.Text.Length)
				CaretPos = -1;
			return true;
		}
		
		/// <summary> Removes the haracter at the caret in the currently entered text. </summary>
		/// <returns> true if a redraw is necessary, false otherwise. </returns>
		public bool Delete() {
			if (input.Text.Length == 0 || CaretPos == -1) return false;
			
			input.Text = input.Text.Remove(CaretPos, 1);
			if (CaretPos == -1) CaretPos = 0;
			
			if (TextChanged != null) TextChanged(input);
			if (CaretPos >= input.Text.Length)
				CaretPos = -1;
			return true;
		}
		
		/// <summary> Resets the currently entered text to an empty string </summary>
		/// <returns> true if a redraw is necessary, false otherwise. </returns>
		public bool Clear() {
			if (input.Text.Length == 0) return false;
			
			input.Text = "";
			if (TextChanged != null) TextChanged(input);
			CaretPos = -1;
			return true;
		}
		
		/// <summary> Copies the contents of the currently entered text to the system clipboard. </summary>
		public void CopyToClipboard() {
			if (String.IsNullOrEmpty(input.Text)) return;
			Clipboard.SetText(input.Text);
		}
		static char[] trimChars = {'\r', '\n', '\v', '\f', ' ', '\t', '\0'};
		
		/// <summary> Sets the currently entered text to the contents of the system clipboard. </summary>
		/// <returns> true if a redraw is necessary, false otherwise. </returns>
		public bool CopyFromClipboard() {
			string text = Clipboard.GetText().Trim(trimChars);
			if (String.IsNullOrEmpty(text)) return false;
			if (input.Text.Length >= MaxChars) return false;
			
			if (ClipboardFilter != null)
				text = ClipboardFilter(text);
			
			int len = input.Text.Length + text.Length;
			if (len > MaxChars) {
				text = text.Substring(0, len - MaxChars);
			}
			
			input.Text += text;
			if (TextChanged != null) TextChanged(input);
			return true;
		}
	}
}
