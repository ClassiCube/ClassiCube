// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using System.IO;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Model;

namespace ClassicalSharp.TexturePack {
	
	/// <summary> Extracts resources from a .zip texture pack. </summary>
	public sealed class TexturePackExtractor {
		
		public const string Dir = "texpacks";
		Game game;
		public void Extract( string path, Game game ) {
			path = Path.Combine( Dir, path );
			path = Path.Combine( Program.AppDirectory, path );
			using( Stream fs = new FileStream( path, FileMode.Open, FileAccess.Read, FileShare.Read ) )
				Extract( fs, game );
		}
		
		public void Extract( byte[] data, Game game ) {
			using( Stream fs = new MemoryStream( data ) )
				Extract( fs, game );
		}
		
		void Extract( Stream stream, Game game ) {
			this.game = game;
			game.Events.RaiseTexturePackChanged();
			ZipReader reader = new ZipReader();
			
			reader.ShouldProcessZipEntry = (f) => true;
			reader.ProcessZipEntry = ProcessZipEntry;
			reader.Extract( stream );
		}
		
		void ProcessZipEntry( string filename, byte[] data, ZipEntry entry ) {
			MemoryStream stream = new MemoryStream( data );
			ModelCache cache = game.ModelCache;
			
			// Ignore directories: convert x/name to name and x\name to name.
			string name = filename.ToLower();
			int i = name.LastIndexOf( '\\' );
			if( i >= 0 ) name = name.Substring( i + 1, name.Length - 1 - i );
			i = name.LastIndexOf( '/' );
			if( i >= 0 ) name = name.Substring( i + 1, name.Length - 1 - i );
			
			switch( name ) {
				case "terrain.png":
					Bitmap atlas = Platform.ReadBmp( stream );
					if( !game.ChangeTerrainAtlas( atlas ) ) atlas.Dispose();
					break;
				case "clouds.png":
				case "cloud.png":
					game.UpdateTexture( ref game.CloudsTex, name, data, false ); break;
				case "gui.png":
					game.UpdateTexture( ref game.GuiTex, name, data, false ); break;
				case "gui_classic.png":
					game.UpdateTexture( ref game.GuiClassicTex, name, data, false ); break;
				case "icons.png":
					game.UpdateTexture( ref game.IconsTex, name, data, false ); break;
				case "default.png":
					SetFontBitmap( game, stream ); break;
			}		
			game.Events.RaiseTextureChanged( name, data );
		}
		
		void SetFontBitmap( Game game, Stream stream ) {
			Bitmap bmp = Platform.ReadBmp( stream );
			if( !FastBitmap.CheckFormat( bmp.PixelFormat ) )
				game.Drawer2D.ConvertTo32Bpp( ref bmp );
			game.Drawer2D.SetFontBitmap( bmp );
			game.Events.RaiseChatFontChanged();
		}
	}
}
