using System;
using System.IO;
using ClassicalSharp.TexturePack;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public sealed class TexturePackScreen : FilesScreen {
		
		public TexturePackScreen( Game game ) : base( game ) {
			titleText = "Select a texture pack zip";
			string directory = Environment.CurrentDirectory;
			files = Directory.GetFiles( directory, "*.zip", SearchOption.AllDirectories );
			
			for( int i = 0; i < files.Length; i++ ) {
				string absolutePath = files[i];
				files[i] = absolutePath.Substring( directory.Length + 1 );
			}
			Array.Sort( files );
		}
		
		public override void Init() {
			base.Init();
			buttons[buttons.Length - 1] =
				MakeBack( false, titleFont, (g, w) => g.SetNewScreen( new PauseScreen( g ) ) );
		}
		
		protected override void TextButtonClick( Game game, Widget widget ) {
			string path = ((ButtonWidget)widget).Text;
			if( File.Exists( path ) ) {
				game.DefaultTexturePack = path;
				TexturePackExtractor extractor = new TexturePackExtractor();
				extractor.Extract( path, game );
				Recreate();
			}
		}
	}
}