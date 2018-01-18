// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;

namespace ClassicalSharp {
	
	public class StringBuffer {
		
		public char[] value;
		public int Capacity, Length;
		
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
		
		static char[] numBuffer = new char[20];
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
		
		int MakeNum(int num) {
			int len = 0;
			do {
				numBuffer[len] = (char)('0' + (num % 10)); 
				num /= 10; len++;
			} while (num > 0);
			return len;
		}
		
		public StringBuffer Clear() {
			Length = 0;
			return this;
		}
		
		public override string ToString() {
			return new String(value, 0, Length);
		}
	}
}
