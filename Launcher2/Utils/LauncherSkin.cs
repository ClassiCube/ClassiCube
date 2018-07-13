// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp;

namespace Launcher {

	public static class LauncherSkin {
		
		public static PackedCol BackgroundCol = new PackedCol(153, 127, 172);
		public static PackedCol ButtonBorderCol = new PackedCol(97, 81, 110);
		public static PackedCol ButtonForeActiveCol = new PackedCol(189, 168, 206);
		public static PackedCol ButtonForeCol = new PackedCol(141, 114, 165);
		public static PackedCol ButtonHighlightCol = new PackedCol(162, 131, 186);
		
		public static void ResetToDefault() {
			BackgroundCol = new PackedCol(153, 127, 172);
			ButtonBorderCol = new PackedCol(97, 81, 110);
			ButtonForeActiveCol = new PackedCol(189, 168, 206);
			ButtonForeCol = new PackedCol(141, 114, 165);
			ButtonHighlightCol = new PackedCol(162, 131, 186);
		}
		
		public static void LoadFromOptions() {
			Get("launcher-back-col", ref BackgroundCol);
			Get("launcher-btn-border-col", ref ButtonBorderCol);
			Get("launcher-btn-fore-active-col", ref ButtonForeActiveCol);
			Get("launcher-btn-fore-inactive-col", ref ButtonForeCol);
			Get("launcher-btn-highlight-inactive-col", ref ButtonHighlightCol);
		}
		
		public static void SaveToOptions() {
			Options.Set("launcher-back-col", BackgroundCol.ToHex());
			Options.Set("launcher-btn-border-col", ButtonBorderCol.ToHex());
			Options.Set("launcher-btn-fore-active-col", ButtonForeActiveCol.ToHex());
			Options.Set("launcher-btn-fore-inactive-col", ButtonForeCol.ToHex());
			Options.Set("launcher-btn-highlight-inactive-col", ButtonHighlightCol.ToHex());
		}
		
		static void Get(string key, ref PackedCol col) {
			PackedCol defaultCol = col;
			string value = Options.Get(key, null);
			if (String.IsNullOrEmpty(value)) return;
			
			if (!PackedCol.TryParse(value, out col))
				col = defaultCol;
		}
	}
}