// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.IO;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public sealed class LoadLevelScreen : FilesScreen {
		
		public LoadLevelScreen( Game game ) : base( game ) {
			titleText = "Select a level";
			string dir = Path.Combine( Program.AppDirectory, "maps" );
			string[] cwFiles = Directory.GetFiles( dir, "*.cw" );
			string[] datFiles = Directory.GetFiles( dir, "*.dat" );
			files = new string[cwFiles.Length + datFiles.Length];
			Array.Copy( cwFiles, 0, files, 0, cwFiles.Length );
			Array.Copy( datFiles, 0, files, cwFiles.Length, datFiles.Length );
			
			for( int i = 0; i < files.Length; i++ )
				files[i] = Path.GetFileName( files[i] );
			Array.Sort( files );
		}
		
		public override void Init() {
			base.Init();
			buttons[buttons.Length - 1] =
				MakeBack( false, titleFont, (g, w) => g.SetNewScreen( new PauseScreen( g ) ) );
		}
		
		protected override void TextButtonClick( Game game, Widget widget, MouseButton mouseBtn ) {
			if( mouseBtn != MouseButton.Left ) return;
			string path = Path.Combine( Program.AppDirectory, "maps" );
			path = Path.Combine( path, ((ButtonWidget)widget).Text );
			if( File.Exists( path ) )
				LoadMap( path );
		}
		
		void LoadMap( string path ) {
			IMapFileFormat mapFile = null;
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
					game.Map.TextureUrl = null;
					for( int tile = BlockInfo.CpeBlocksCount; tile < BlockInfo.BlocksCount; tile++ )
						game.BlockInfo.ResetBlockInfo( (byte)tile, false );
					game.BlockInfo.SetupCullingCache();
					game.BlockInfo.InitLightOffsets();
					
					byte[] blocks = mapFile.Load( fs, game, out width, out height, out length );
					game.Map.SetData( blocks, width, height, length );
					game.MapEvents.RaiseOnNewMapLoaded();
					if( game.AllowServerTextures && game.Map.TextureUrl != null )
						game.Network.RetrieveTexturePack( game.Map.TextureUrl );
					
					LocalPlayer p = game.LocalPlayer;
					LocationUpdate update = LocationUpdate.MakePosAndOri( p.SpawnPoint, p.SpawnYaw, p.SpawnPitch, false );
					p.SetLocation( update, false );
				}
			} catch( Exception ex ) {
				ErrorHandler.LogError( "loading map", ex );
				game.Chat.Add( "&e/client loadmap: Failed to load map \"" + path + "\"" );
			}
		}
	}
}