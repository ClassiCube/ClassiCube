// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
#if ANDROID
using System;

namespace ClassicalSharp {

	// TODO: Actually apply this to the text
	public class Font : IDisposable {

		public int Size;

		public FontStyle Style;

		public Font(string name, int size, FontStyle style) {
			Size = size;
			Style = style;
		}

		public Font(Font font, FontStyle style) {
			Size = font.Size;
			Style = style;
		}

		public Font(string name, int size) {
			Size = size;
			Style = FontStyle.Regular;
		}

		public void Dispose() {
		}
	}

	public enum FontStyle {
		Regular,
		Italic,
		Bold,
		Underline,
	}
}
#endif