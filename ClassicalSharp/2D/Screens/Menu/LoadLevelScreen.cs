using System;
using System.IO;

namespace ClassicalSharp {
	
	public sealed class LoadLevelScreen : FilesScreen {
		
		public LoadLevelScreen( Game game ) : base( game ) {
			titleText = "Select a level";
			string directory = Environment.CurrentDirectory;
			string[] cwFiles = Directory.GetFiles( directory, "*.cw", SearchOption.AllDirectories );
			string[] datFiles = Directory.GetFiles( directory, "*.dat", SearchOption.AllDirectories );
			files = new string[cwFiles.Length + datFiles.Length];
			Array.Copy( cwFiles, 0, files, 0, cwFiles.Length );
			Array.Copy( datFiles, 0, files, cwFiles.Length, datFiles.Length );
			
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
			if( File.Exists( path ) )
				LoadMap( path );
		}
		
		void LoadMap( string path ) {
			IMapFile mapFile = null;
			if( path.EndsWith( ".dat" ) ) {
				mapFile = new MapDat();
			} else if( path.EndsWith( ".fcm" ) ) {
				mapFile = new MapFcm3();
			} else if( path.EndsWith( ".cw" ) ) {
				mapFile = new MapCw();
			}
			
			try {
				using( FileStream fs = new FileStream( path, FileMode.Open, FileAccess.Read, FileShare.Read ) ) {
					int width, height, length;
					game.Map.Reset();
					
					byte[] blocks = mapFile.Load( fs, game, out width, out height, out length );
					game.Map.UseRawMap( blocks, width, height, length );
					game.Events.RaiseOnNewMapLoaded();
					
					LocalPlayer p = game.LocalPlayer;
					LocationUpdate update = LocationUpdate.MakePos( p.SpawnPoint, false );
					p.SetLocation( update, false );
				}
			} catch( Exception ex ) {
				Utils.LogError( "Error while trying to load map: {0}{1}", Environment.NewLine, ex );
				game.Chat.Add( "&e/client loadmap: Failed to load map \"" + path + "\"" );
			}
		}
	}
}