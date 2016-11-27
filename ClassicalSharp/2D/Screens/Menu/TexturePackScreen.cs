// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.IO;
using ClassicalSharp.Gui.Widgets;
using ClassicalSharp.Textures;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	public sealed class TexturePackScreen : FilesScreen {
		
		public TexturePackScreen(Game game) : base(game) {
			titleText = "Select a texture pack zip";
			string dir = Path.Combine(Program.AppDirectory, TexturePack.Dir);
			entries = Directory.GetFiles(dir, "*.zip");
			
			for (int i = 0; i < entries.Length; i++)
				entries[i] = Path.GetFileName(entries[i]);
			Array.Sort(entries);
		}
		
		protected override void TextButtonClick(Game game, Widget widget, MouseButton btn, int x, int y) {
			if (btn != MouseButton.Left) return;
			string file = ((ButtonWidget)widget).Text;
			string dir = Path.Combine(Program.AppDirectory, TexturePack.Dir);
			string path = Path.Combine(dir, file);
			if (!File.Exists(path)) return;
			
			int index = currentIndex;
			game.DefaultTexturePack = file;
			TexturePack.ExtractDefault(game);
			Recreate();
			SetCurrentIndex(index);
		}
	}
}