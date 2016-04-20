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
			game.Animations.Dispose();
			ZipReader reader = new ZipReader();
			
			reader.ShouldProcessZipEntry = (f) => true;
			reader.ProcessZipEntry = ProcessZipEntry;
			reader.Extract( stream );
		}
		
		void ProcessZipEntry( string filename, byte[] data, ZipEntry entry ) {
			MemoryStream stream = new MemoryStream( data );
			ModelCache cache = game.ModelCache;
			filename = filename.ToLower();
			if( filename.StartsWith( "mob/" ) )
				filename = filename.Substring( 4 );
			
			switch( filename ) {
				case "terrain.png":
					game.ChangeTerrainAtlas( Platform.ReadBmp( stream ) ); break;
				case "chicken.png":
					UpdateTexture( ref cache.ChickenTexId, stream, false ); break;
				case "creeper.png":
					UpdateTexture( ref cache.CreeperTexId, stream, false ); break;
				case "pig.png":
					UpdateTexture( ref cache.PigTexId, stream, false ); break;
				case "sheep.png":
					UpdateTexture( ref cache.SheepTexId, stream, false ); break;
				case "skeleton.png":
					UpdateTexture( ref cache.SkeletonTexId, stream, false ); break;
				case "spider.png":
					UpdateTexture( ref cache.SpiderTexId, stream, false ); break;
				case "zombie.png":
					UpdateTexture( ref cache.ZombieTexId, stream, false ); break;
				case "sheep_fur.png":
					UpdateTexture( ref cache.SheepFurTexId, stream, false ); break;
				case "char.png":
					UpdateTexture( ref cache.HumanoidTexId, stream, true ); break;
				case "clouds.png":
				case "cloud.png":
					UpdateTexture( ref game.CloudsTexId, stream, false ); break;
				case "rain.png":
					UpdateTexture( ref game.RainTexId, stream, false ); break;
				case "snow.png":
					UpdateTexture( ref game.SnowTexId, stream, false ); break;
				case "gui.png":
					UpdateTexture( ref game.GuiTexId, stream, false ); break;
				case "gui_classic.png":
					UpdateTexture( ref game.GuiClassicTexId, stream, false ); break;
				case "animations.png":
				case "animation.png":
					game.Animations.SetAtlas( Platform.ReadBmp( stream ) ); break;
				case "animations.txt":
				case "animation.txt":
					StreamReader reader = new StreamReader( stream );
					game.Animations.ReadAnimationsDescription( reader );
					break;
				case "particles.png":
					UpdateTexture( ref game.ParticleManager.ParticlesTexId, 
					              stream, false ); break;
				case "default.png":
					SetFontBitmap( game, stream ); break;
			}
			
			if( !filename.EndsWith( ".png" ) ) return;
			string tex = filename.Substring( 0, filename.Length - 4 );
			game.Events.RaiseTextureChanged( tex );
		}
		
		void SetFontBitmap( Game game, Stream stream ) {
			Bitmap bmp = Platform.ReadBmp( stream );
			if( !FastBitmap.CheckFormat( bmp.PixelFormat ) )
				game.Drawer2D.ConvertTo32Bpp( ref bmp );
			game.Drawer2D.SetFontBitmap( bmp );
			game.Events.RaiseChatFontChanged();
		}
		
		void UpdateTexture( ref int texId, Stream stream, bool setSkinType ) {
			game.Graphics.DeleteTexture( ref texId );
			using( Bitmap bmp = Platform.ReadBmp( stream ) ) {
				if( setSkinType )
					game.DefaultPlayerSkinType = Utils.GetSkinType( bmp );
				
				if( !FastBitmap.CheckFormat( bmp.PixelFormat ) ) {
					using( Bitmap bmp32 = game.Drawer2D.ConvertTo32Bpp( bmp ) )
						texId = game.Graphics.CreateTexture( bmp32 );
				} else {
					texId = game.Graphics.CreateTexture( bmp );
				}
			}
		}
	}
}
