using System;
using System.IO;
using ClassicalSharp.TexturePack;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public sealed class TexturePackScreen : FilesScreen {
		
		public TexturePackScreen( Game game ) : base( game ) {
			titleText = "Select a texture pack zip";
			string dir = Program.AppDirectory;
			files = Directory.GetFiles( dir, "*.zip", SearchOption.AllDirectories );
			
			for( int i = 0; i < files.Length; i++ ) {
				string absolutePath = files[i];
				files[i] = absolutePath.Substring( dir.Length );
			}
			Array.Sort( files );
		}
		
		public override void Init() {
			base.Init();
			buttons[buttons.Length - 1] =
				MakeBack( false, titleFont, (g, w) => g.SetNewScreen( new PauseScreen( g ) ) );
		}
		
		protected override void TextButtonClick( Game game, Widget widget ) {
			string file = ((ButtonWidget)widget).Text;
			string path = Path.Combine( Program.AppDirectory, file );
			if( !File.Exists( path ) )
				return;
			
			game.DefaultTexturePack = file;
			TexturePackExtractor extractor = new TexturePackExtractor();
			extractor.Extract( path, game );
			Recreate();
		}
	}
}