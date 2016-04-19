// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.IO;
using ClassicalSharp.Entities;
using ClassicalSharp.Map;
using OpenTK.Input;

namespace ClassicalSharp.Gui {
	
	public sealed class LoadLevelScreen : FilesScreen {
		
		public LoadLevelScreen( Game game ) : base( game ) {
			titleText = "Select a level";
			string dir = Path.Combine( Program.AppDirectory, "maps" );
			string[] rawFiles = Directory.GetFiles( dir );
			int count = 0;
			
			// Only add map files
			for( int i = 0; i < rawFiles.Length; i++ ) {
				string file = rawFiles[i];
				if( file.EndsWith( ".cw" ) || file.EndsWith( ".dat" ) 
				   || file.EndsWith( ".fcm" ) || file.EndsWith( ".lvl" ) ) {
					count++;
				} else {
					rawFiles[i] = null;
				}
			}
			
			files = new string[count];
			for( int i = 0, j = 0; i < rawFiles.Length; i++ ) {
				string file = rawFiles[i];
				if( file == null ) continue;
				files[j] = Path.GetFileName( file ); j++;
			}
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
			IMapFormatImporter importer = null;
			if( path.EndsWith( ".dat" ) ) {
				importer = new MapDatImporter();
			} else if( path.EndsWith( ".fcm" ) ) {
				importer = new MapFcm3Importer();
			} else if( path.EndsWith( ".cw" ) ) {
				importer = new MapCwImporter();
			} else if( path.EndsWith( ".lvl" ) ) {
				importer = new MapLvlImporter();
			}
			
			try {
				using( FileStream fs = new FileStream( path, FileMode.Open, FileAccess.Read, FileShare.Read ) ) {
					int width, height, length;
					game.World.Reset();
					game.World.TextureUrl = null;
					for( int tile = BlockInfo.CpeBlocksCount; tile < BlockInfo.BlocksCount; tile++ )
						game.BlockInfo.ResetBlockInfo( (byte)tile, false );
					game.BlockInfo.SetupCullingCache();
					game.BlockInfo.InitLightOffsets();
					
					byte[] blocks = importer.Load( fs, game, out width, out height, out length );
					game.World.SetData( blocks, width, height, length );
					game.WorldEvents.RaiseOnNewMapLoaded();
					if( game.AllowServerTextures && game.World.TextureUrl != null )
						game.Network.RetrieveTexturePack( game.World.TextureUrl );
					
					LocalPlayer p = game.LocalPlayer;
					LocationUpdate update = LocationUpdate.MakePosAndOri( p.Spawn, p.SpawnYaw, p.SpawnPitch, false );
					p.SetLocation( update, false );
				}
			} catch( Exception ex ) {
				ErrorHandler.LogError( "loading map", ex );
				string file = Path.GetFileName( path );
				game.Chat.Add( "&e/client loadmap: Failed to load map \"" + file + "\"" );
			}
		}
	}
}