// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;

namespace ClassicalSharp {
	
	public class StringBuffer {
		
		public char[] value;
		public int Capacity, Length;
		public bool Empty { get { return Length == 0; } }
		
		public StringBuffer(int capacity) {
			this.Capacity = capacity;
			value = new char[capacity];
		}
		
		public StringBuffer Append(char c) {
			value[Length++] = c;
			return this;
		}

		public StringBuffer Append(string s) {
			for (int i = 0; i < s.Length; i++)
				value[Length++] = s[i];
			return this;
		}
		
		public StringBuffer AppendColourless(string s) {
			for (int i = 0; i < s.Length; i++) {
				char token = s[i];
				if (token == '&')
					i++;// Skip over the following colour code.
				else
					value[Length++] = token;
			}
			return this;
		}
		
		internal static char[] numBuffer = new char[20];
		internal static int MakeNum(int num) {
			int len = 0;
			do {
				numBuffer[len] = (char)('0' + (num % 10));
				num /= 10; len++;
			} while (num > 0);
			return len;
		}
		
		public StringBuffer AppendNum(int num) {
			int numLen = MakeNum(num);
			for (int i = numLen - 1; i >= 0; i--)
				value[Length++] = numBuffer[i];
			return this;
		}
		
		public StringBuffer AppendPaddedNum(int minDigits, int num) {
			for (int i = 0; i < minDigits; i++)
				numBuffer[i] = '0';
			int numLen = Math.Max(minDigits, MakeNum(num));
			for (int i = numLen - 1; i >= 0; i--)
				value[Length++] = numBuffer[i];
			return this;
		}
		
		public StringBuffer DeleteAt(int index) {
			for (int i = index; i < Length - 1; i++) {
				value[i] = value[i + 1];
			}
			
			value[Length - 1] = '\0';
			Length--;
			return this;
		}
		
		public StringBuffer InsertAt(int index, char c) {
			for (int i = Length; i > index; i--) {
				value[i] = value[i - 1];
			}
			
			value[index] = c;
			Length++;
			return this;
		}
		
		public StringBuffer Set(string s) {
			for (int i = 0; i < s.Length; i++) value[i] = s[i];
			Length = s.Length;
			return this;
		}
		
		public StringBuffer Clear() {
			Length = 0;
			return this;
		}
		
		public override string ToString() {
			return new String(value, 0, Length);
		}
		
		bool IsWrapper(char c) {
			return c == '\0' || c == ' ' || c == '-' || c == '>'
				|| c == '<' || c == '/' || c == '\\';
		}
		
		unsafe string Substring(int offset, int len) {
			if (len == 0) return "";
			char* tmp = stackalloc char[len];
			
			// convert %0-f to &0-f for colour preview.
			for (int i = 0; i < len; i++) {
				tmp[i] = value[offset + i];
				if (tmp[i] != '%' || (i + 1) >= len) continue;			
				if (IDrawer2D.ValidColCode(value[offset + i + 1])) tmp[i] = '&';
			}
			return new String(tmp, 0, len);
		}
		
		public void WordWrap(IDrawer2D drawer, string[] lines, int numLines, int lineLen) {
			for (int i = 0; i < numLines; i++) { lines[i] = null; }

			int lineStart = 0, lineEnd;
			for (int i = 0; i < numLines; i++) {
				int nextLineStart = lineStart + lineLen;
				// No more text to wrap
				if (nextLineStart >= Length) {
					lines[i] = Substring(lineStart, Length - lineStart); return;
				}

				// Find beginning of last word on current line
				for (lineEnd = nextLineStart; lineEnd >= lineStart; lineEnd--) {
					if (IsWrapper(value[lineEnd])) break;
				}
				lineEnd++; // move after wrapper char (i.e. beginning of last word)

				if (lineEnd <= lineStart || lineEnd >= nextLineStart) {
					// Three special cases handled by this:
					// - Entire line is filled with a single word 
					// - Last character(s) on current line are wrapper characters
					// - First character on next line is a wrapper character (last word ends at current line end)
					lines[i] = Substring(lineStart, lineLen);
					lineStart += lineLen;
				} else {
					// Last word in current line does not end in current line (extends onto next line)
					// Trim current line to end at beginning of last word
					// Set next line to start at beginning of last word
					lines[i] = Substring(lineStart, lineEnd - lineStart);
					lineStart = lineEnd;
				}
			}
		}

		/// <summary> Calculates where the given raw index is located in the wrapped lines. </summary>
		public void GetCoords(int index, string[] lines, out int coordX, out int coordY) {
			if (index == -1) index = Int32.MaxValue;
			int total = 0; coordX = -1; coordY = 0;
			
			for (int y = 0; y < lines.Length; y++) {
				int lineLength = LineLength(lines[y]);
				if (lineLength == 0) break;
				
				coordY = y;
				if (index < total + lineLength) {
					coordX = index - total; break;
				}
				total += lineLength;
			}
			if (coordX == -1) coordX = LineLength(lines[coordY]);
		}
		
		static int LineLength(string line) { return line == null ? 0 : line.Length; }
		
		public int GetBackLength(int index) {
			if (index <= 0) return 0;
			int start = index;
			
			bool lookingSpace = value[index] == ' ';
			// go back to the end of the previous word
			if (lookingSpace) {
				while (index > 0 && value[index] == ' ')
					index--;
			}
			
			// go back to the start of the current word
			while (index > 0 && value[index] != ' ')
				index--;
			return (start - index);
		}
		
		public int GetForwardLength(int index) {
			if (index == -1) return 0;
			int start = index;
			
			bool lookingLetter = value[index] != ' ';
			// go forward to the end of the current word
			if (lookingLetter) {
				while (index < Length && value[index] != ' ')
					index++;
			}
			
			// go forward to the start of the next word
			while (index < Length && value[index] == ' ')
				index++;
			return index - start;
		}
	}
}
