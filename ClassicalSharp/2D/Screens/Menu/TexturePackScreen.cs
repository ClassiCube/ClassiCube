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
				Make( 0, 5, "Back to menu", (g, w) => g.SetNewScreen( new PauseScreen( g ) ) );
		}
		
		ButtonWidget Make( int x, int y, string text, Action<Game, ButtonWidget> onClick ) {
			return ButtonWidget.Create( game, x, y, 240, 35, text,
			                           Anchor.Centre, Anchor.BottomOrRight, titleFont, onClick );
		}
		
		protected override void TextButtonClick( Game game, ButtonWidget widget ) {
			string path = widget.Text;
			if( File.Exists( path ) ) {
				TexturePackExtractor extractor = new TexturePackExtractor();
				extractor.Extract( path, game );
			}
		}
	}
}