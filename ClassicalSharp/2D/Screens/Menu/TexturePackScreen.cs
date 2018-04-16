// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.IO;
using ClassicalSharp.Gui.Widgets;
using ClassicalSharp.Textures;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	public sealed class TexturePackScreen : ListScreen {
		
		public TexturePackScreen(Game game) : base(game) {
			titleText = "Select a texture pack zip";
			entries = Platform.DirectoryFiles("texpacks", "*.zip");
			Array.Sort(entries);
		}
		
		protected override void TextButtonClick(Game game, Widget widget) {
			string file = GetCur(widget);
			string path = Path.Combine("texpacks", file);
			if (!Platform.FileExists(path)) return;
			
			int curPage = currentIndex;
			game.DefaultTexturePack = file;
			TexturePack.ExtractDefault(game);
			Recreate();
			SetCurrentIndex(curPage);
		}
	}
}