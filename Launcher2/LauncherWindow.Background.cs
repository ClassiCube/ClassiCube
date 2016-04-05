// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using System.IO;
using ClassicalSharp;
using ClassicalSharp.TexturePack;

namespace Launcher {

	public sealed partial class LauncherWindow {

		internal bool ClassicBackground = false;
		
		internal void TryLoadTexturePack() {
			Options.Load();
			LauncherSkin.LoadFromOptions();
			if( Options.Get( "nostalgia-classicbg" ) != null )
				ClassicBackground = Options.GetBool( "nostalgia-classicbg", false );
			else
				ClassicBackground = Options.GetBool( "mode-classic", false );
			
			string texDir = Path.Combine( Program.AppDirectory, "texpacks" );
			string texPack = Options.Get( OptionsKey.DefaultTexturePack ) ?? "default.zip";
			texPack = Path.Combine( texDir, texPack );
			
			if( !File.Exists( texPack ) )
				texPack = Path.Combine( texDir, "default.zip" );
			if( !File.Exists( texPack ) )
				return;
			
			using( Stream fs = new FileStream( texPack, FileMode.Open, FileAccess.Read, FileShare.Read ) ) {
				ZipReader reader = new ZipReader();
				
				reader.ShouldProcessZipEntry = (f) => f == "default.png" || f == "terrain.png";
				reader.ProcessZipEntry = ProcessZipEntry;
				reader.Extract( fs );
			}
		}
		
		bool useBitmappedFont;
		Bitmap terrainBmp;
		FastBitmap terrainPixels;
		const int tileSize = 48;
		
		void ProcessZipEntry( string filename, byte[] data, ZipEntry entry ) {
			MemoryStream stream = new MemoryStream( data );
			
			if( filename == "default.png" ) {
				Bitmap bmp = new Bitmap( stream );
				Drawer.SetFontBitmap( bmp );
				useBitmappedFont = !Options.GetBool( OptionsKey.ArialChatFont, false );
			} else if( filename == "terrain.png" ) {
				using( Bitmap bmp = new Bitmap( stream ) )
					MakeClassicTextures( bmp );
			}
		}
		
		void MakeClassicTextures( Bitmap bmp ) {
			int elemSize = bmp.Width / 16;
			Size size = new Size( tileSize, tileSize );
			terrainBmp = new Bitmap( tileSize * 2, tileSize );
			terrainPixels = new FastBitmap( terrainBmp, true );
			
			// Precompute the scaled background
			using( FastBitmap src = new FastBitmap( bmp, true ) ) {
				Drawer2DExt.DrawScaledPixels( src, terrainPixels, size,
				                             new Rectangle( 2 * elemSize, 0, elemSize, elemSize ),
				                             new Rectangle( tileSize, 0, tileSize, tileSize ), 128, 64 );
				Drawer2DExt.DrawScaledPixels( src, terrainPixels, size,
				                             new Rectangle( 1 * elemSize, 0, elemSize, elemSize ),
				                             new Rectangle( 0, 0, tileSize, tileSize ), 96, 96 );
			}
		}
		
		public void MakeBackground() {
			if( Framebuffer == null || (Framebuffer.Width != Width || Framebuffer.Height != Height) ) {
				if( Framebuffer != null )
					Framebuffer.Dispose();
				Framebuffer = new Bitmap( Width, Height );
			}
			
			if( ClassicBackground ) {
				using( FastBitmap dst = new FastBitmap( Framebuffer, true ) ) {
					ClearTile( 0, 0, Width, 48, tileSize, dst );
					ClearTile( 0, 48, Width, Height - 48, 0, dst );
				}
			} else {
				ClearArea( 0, 0, Width, Height );
			}
			
			using( IDrawer2D drawer = Drawer ) {
				drawer.SetBitmap( Framebuffer );

				drawer.UseBitmappedChat = useBitmappedFont;
				DrawTextArgs args = new DrawTextArgs( "&eClassical&fSharp", logoFont, false );
				Size size = drawer.MeasureChatSize( ref args );
				int xStart = Width / 2 - size.Width / 2;
				
				args.Text = "&0Classical&0Sharp";
				drawer.DrawChatText( ref args, xStart + 4, 8 + 4 );
				args.Text = "&eClassical&fSharp";
				drawer.DrawChatText( ref args, xStart, 8 );
				drawer.UseBitmappedChat = false;
			}
			Dirty = true;
		}
		
		public void ClearArea( int x, int y, int width, int height ) {
			using( FastBitmap dst = new FastBitmap( Framebuffer, true ) )
				ClearArea( x, y, width, height, dst );
		}
		
		public void ClearArea( int x, int y, int width, int height, FastBitmap dst ) {
			if( ClassicBackground ) {
				ClearTile( x, y, width, height, 0, dst );
			} else {
				FastColour col = LauncherSkin.BackgroundCol;
				Drawer2DExt.DrawNoise( dst, new Rectangle( x, y, width, height ), col, 6 );
			}
		}
		
		void ClearTile( int x, int y, int width, int height, int srcX, FastBitmap dst ) {
			Rectangle srcRect = new Rectangle( srcX, 0, tileSize, tileSize );	
			Drawer2DExt.DrawTiledPixels( terrainPixels, dst, srcRect, 
			                            new Rectangle( x, y, width, height ) );
		}
	}
}
