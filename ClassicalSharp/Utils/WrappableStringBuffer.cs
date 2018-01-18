// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;

namespace ClassicalSharp {
	
	public unsafe sealed class WrappableStringBuffer {
		
		public int Capacity;
		public char[] value;
		char[] wrap;	
		
		public WrappableStringBuffer(int capacity) {
			this.Capacity = capacity;
			value = new char[capacity];
			wrap = new char[capacity];
		}
		
		public void DeleteAt(int index) {
			for (int i = index; i < Capacity - 1; i++)
				value[i] = value[i + 1];
			value[Capacity - 1] = '\0';
		}
		
		public void InsertAt(int index, char c) {
			for (int i = Capacity - 1; i > index; i--)
				value[i] = value[i - 1];
			value[index] = c;
		}
		
		public void Set(string s) {
			for (int i = 0; i < s.Length; i++) value[i] = s[i];
		}
		
		public void Clear() {
			for (int i = 0; i < Capacity; i++) value[i] = '\0';
		}
		
		public bool Empty {
			get {
				for (int i = 0; i < Capacity; i++) {
					if (value[i] != '\0') return false;
				}
				return true;
			}
		}
		
		public int Length {
			get {
				int len = Capacity;
				for (int i = Capacity - 1; i >= 0; i--) {
					if (value[i] != '\0') break;
					len--;
				}
				return len;
			}
		}
		
		public int TextLength {
			get {
				int len = Capacity;
				for (int i = Capacity - 1; i >= 0; i--) {
					if (value[i] != '\0' && value[i] != ' ') break;
					len--;
				}
				return len;
			}
		}
		
		public override string ToString() {
			return new String(value, 0, Length);
		}

		
		public void WordWrap(IDrawer2D drawer, string[] lines, int maxLines, int maxPerLine) {
			int len = Length;
			int* lineLens = stackalloc int[lines.Length];
			for (int i = 0; i < lines.Length; i++) {
				lines[i] = null;
				lineLens[i] = 0;
			}
			
			// Need to make a copy because we mutate the characters.
			char[] realText = value;
			MakeWrapCopy();
			
			int usedLines = 0, totalChars = maxPerLine * maxLines;
			for (int index = 0; index < totalChars; index += maxPerLine) {
				if (value[index] == '\0') break;
				
				int lineEnd = index + (maxPerLine - 1), nextStart = lineEnd + 1;
				usedLines++;
				
				// Do we need word wrapping?
				bool needWrap = !IsWrapper(value[lineEnd]) 
					&& nextStart < totalChars && !IsWrapper(value[nextStart]);
				int wrappedLen = needWrap ? WrapLine(index, maxPerLine) : maxPerLine;
				
				// Calculate the maximum size of this line
				int lineLen = maxPerLine;
				for (int i = lineEnd; i >= index; i--) {
					if (value[i] != '\0') break;
					lineLen--;
				}
				lineLens[index / maxPerLine] = Math.Min(lineLen, wrappedLen);
			}
			
			// Output the used lines
			OutputLines(drawer, lines, lineLens, usedLines, maxLines, maxPerLine);
			value = realText;
		}
		
		void MakeWrapCopy() {
			int len = Length;
			for (int i = 0; i < len; i++)
				wrap[i] = value[i];
			
			for (int i = len; i < Capacity; i++)
				wrap[i] = '\0';
			value = wrap;
		}
		
		void OutputLines(IDrawer2D drawer, string[] lines, int* lineLens, int usedLines, int maxLines, int charsPerLine) {
			int totalChars = charsPerLine * maxLines;
			for (int i = 0; i < totalChars; i++) {
				if (value[i] == '\0') value[i] = ' ';
			}
			// convert %0-f to &0-f for colour preview.
			for (int i = 0; i < totalChars - 1; i++) {
				if (value[i] == '%' && IDrawer2D.ValidColCode(value[i + 1]))
					value[i] = '&';
			}
			
			usedLines = Math.Max(1, usedLines);
			for (int i = 0; i < usedLines; i++)
				lines[i] = new String(value, i * charsPerLine, lineLens[i]);
		}
		
		int WrapLine(int index, int lineSize) {
			int lineEnd = index + (lineSize - 1);
			// wrap - but we don't want to wrap if the entire line is filled.
			for (int i = lineEnd; i >= index + 1; i--) {
				if (IsWrapper(value[i])) {
					for (int j = lineEnd; j >= i + 1; j--) {
						InsertAt(index + lineSize, value[j]);
						value[j] = ' ';
					}
					return (i + 1) - index;
				}
			}
			return lineSize;
		}
		
		bool IsWrapper(char c) {
			return c == '\0' || c == ' ' || c == '-' || c == '>'
				|| c == '<' || c == '/' || c == '\\';
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
