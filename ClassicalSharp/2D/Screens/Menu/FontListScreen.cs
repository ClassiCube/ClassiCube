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
			string font = game.FontName;
			
			for (int i = 0; i < entries.Length; i++) {
				if (!Utils.CaselessEq(font, entries[i])) continue;
				SetCurrentIndex(i);
				return;
			}
		}
		
		protected override void TextButtonClick(Game game, Widget widget) {			
			string font = GetCur(widget);
			if (font == empty) return;
			
			game.FontName = font;
			Options.Set(OptionsKey.FontName, font);
			
			int cur = currentIndex;
			HandleFontChange();
			SetCurrentIndex(cur);
		}
		
		protected override void UpdateText(ButtonWidget widget, string text) {
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