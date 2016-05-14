// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.IO;
using ClassicalSharp.TexturePack;
using OpenTK.Input;

namespace ClassicalSharp.Gui {
	
	public sealed class TexturePackScreen : FilesScreen {
		
		public TexturePackScreen( Game game ) : base( game ) {
			titleText = "Select a texture pack zip";
			string dir = Path.Combine( Program.AppDirectory, TexturePackExtractor.Dir );
			entries = Directory.GetFiles( dir, "*.zip" );
			
			for( int i = 0; i < entries.Length; i++ )
				entries[i] = Path.GetFileName( entries[i] );
			Array.Sort( entries );
		}
		
		protected override void TextButtonClick( Game game, Widget widget, MouseButton mouseBtn ) {
			if( mouseBtn != MouseButton.Left ) return;
			string file = ((ButtonWidget)widget).Text;
			string dir = Path.Combine( Program.AppDirectory, TexturePackExtractor.Dir );
			string path = Path.Combine( dir, file );
			if( !File.Exists( path ) ) return;
			
			int index = currentIndex;
			game.DefaultTexturePack = file;
			game.World.TextureUrl = null;
			TexturePackExtractor extractor = new TexturePackExtractor();
			extractor.Extract( path, game );
			Recreate();
			SetCurrentIndex( index );
		}
	}
}