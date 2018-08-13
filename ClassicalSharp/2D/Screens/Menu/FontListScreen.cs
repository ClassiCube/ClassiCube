// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using ClassicalSharp.Gui.Widgets;

namespace ClassicalSharp.Gui.Screens {
	public class FontListScreen : ListScreen {
		
		public FontListScreen(Game game) : base(game) {
			titleText = "Select a font";
			FontFamily[] families = FontFamily.Families;
			entries = new string[families.Length];
			
			for (int i = 0; i < families.Length; i++) {
				entries[i] = families[i].Name;
				families[i].Dispose();
			}
			Array.Sort(entries);
		}
		
		public override void Init() {
			base.Init();
			Select(game.FontName);
		}
		
		protected override void EntryClick(Game game, Widget widget) {			
			string fontName = GetCur(widget);
			if (fontName == empty) return;
			
			// Some fonts don't support Regular style
			try {
				using (Font tmp = new Font(fontName, font.Size)) { }
			} catch { return; }
			
			game.FontName = fontName;
			Options.Set(OptionsKey.FontName, fontName);
			
			int cur = currentIndex;
			HandleFontChange();
			SetCurrentIndex(cur);
		}
		
		protected override void UpdateEntry(ButtonWidget widget, string text) {
			try {
				using (Font tmp = new Font(text, font.Size)) {
					widget.font = tmp;
					widget.SetText(text);
					widget.font = font;
				}
			} catch {
				widget.SetText(empty);
			}
		}
	}
}