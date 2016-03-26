// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using ClassicalSharp;
using ClassicalSharp.TexturePack;

namespace Launcher {
	
	public partial class ResourcePatcher {

		public ResourcePatcher( ResourceFetcher fetcher ) {
			jarClassic = fetcher.jarClassic;
			jar162 = fetcher.jar162;
			pngTerrainPatch = fetcher.pngTerrainPatch;
			pngGuiPatch = fetcher.pngGuiPatch;
		}
		
		byte[] jarClassic, jar162, pngTerrainPatch, pngGuiPatch;	
		public void Run() {			
			reader = new ZipReader();
			reader.ShouldProcessZipEntry = ShouldProcessZipEntry_Classic;
			reader.ProcessZipEntry = ProcessZipEntry_Classic;
			string texDir = Path.Combine( Program.AppDirectory, "texpacks" );
			string path = Path.Combine( texDir, "default.zip" );
			
			using( Stream srcClassic = new MemoryStream( jarClassic ),
			      srcModern = new MemoryStream( jar162 ),
			      dst = new FileStream( path, FileMode.Create, FileAccess.Write ) ) {
				writer = new ZipWriter( dst );
				reader.Extract( srcClassic );
				
				// Grab animations and snow
				animBitmap = new Bitmap( 1024, 64, PixelFormat.Format32bppArgb );
				reader.ShouldProcessZipEntry = ShouldProcessZipEntry_Modern;
				reader.ProcessZipEntry = ProcessZipEntry_Modern;
				reader.Extract( srcModern );
				
				writer.WriteNewImage( animBitmap, "animations.png" );
				writer.WriteNewString( animationsTxt, "animations.txt" );
				using( Bitmap guiBitmap = new Bitmap( new MemoryStream( pngGuiPatch ) ) ) {
					writer.WriteNewImage( guiBitmap, "gui.png" );
				}
				animBitmap.Dispose();
				writer.WriteCentralDirectoryRecords();
			}
		}
		ZipReader reader;
		ZipWriter writer;
		Bitmap animBitmap;
		
		bool ShouldProcessZipEntry_Classic( string filename ) {
			return filename == "gui/gui.png" || filename.StartsWith( "mob" ) || filename.IndexOf( '/' ) < 0;
		}
		
		void ProcessZipEntry_Classic( string filename, byte[] data, ZipEntry entry ) {
			if( writer.entries == null )
				writer.entries = new ZipEntry[reader.entries.Length];
			if( filename != "terrain.png" ) {
				int lastSlash = filename.LastIndexOf( '/' );
				if( lastSlash >= 0 )
					entry.Filename = filename.Substring( lastSlash + 1 );
				if( entry.Filename == "gui.png" ) 
					entry.Filename = "gui_classic.png";
				writer.WriteZipEntry( entry, data );
				return;
			}
			
			using( Bitmap dstBitmap = new Bitmap( new MemoryStream( data ) ),
			      maskBitmap = new Bitmap( new MemoryStream( pngTerrainPatch ) ) ) {
				PatchImage( dstBitmap, maskBitmap );
				writer.WriteNewImage( dstBitmap, "terrain.png" );
			}
		}
		
		bool ShouldProcessZipEntry_Modern( string filename ) {
			return filename.StartsWith( "assets/minecraft/textures" ) &&
				( filename == "assets/minecraft/textures/environment/snow.png" ||
				 filename == "assets/minecraft/textures/blocks/water_still.png" ||
				 filename == "assets/minecraft/textures/blocks/lava_still.png" ||
				 filename == "assets/minecraft/textures/blocks/fire_layer_1.png" ||
				 filename == "assets/minecraft/textures/entity/chicken.png" );
		}
		
		void ProcessZipEntry_Modern( string filename, byte[] data, ZipEntry entry ) {
			switch( filename ) {
				case "assets/minecraft/textures/environment/snow.png":
					entry.Filename = "snow.png";
					writer.WriteZipEntry( entry, data );
					break;
				case "assets/minecraft/textures/entity/chicken.png":
					entry.Filename = "mob/chicken.png";
					writer.WriteZipEntry( entry, data );
					break;
				case "assets/minecraft/textures/blocks/water_still.png":
					PatchDefault( data, 0 );
					break;
				case "assets/minecraft/textures/blocks/lava_still.png":
					PatchCycle( data, 16 );
					break;
				case "assets/minecraft/textures/blocks/fire_layer_1.png":
					PatchDefault( data, 32 );
					break;
			}
		}
		
		unsafe void PatchImage( Bitmap dstBitmap, Bitmap maskBitmap ) {
			using( FastBitmap dst = new FastBitmap( dstBitmap, true ),
			      src = new FastBitmap( maskBitmap, true ) ) {
				int size = src.Width, tileSize = size / 16;
				
				for( int y = 0; y < size; y += tileSize ) {
					int* row = src.GetRowPtr( y );
					for( int x = 0; x < size; x += tileSize ) {
						if( row[x] != unchecked((int)0x80000000) ) {
							FastBitmap.MovePortion( x, y, x, y, src, dst, tileSize );
						}
					}
				}
			}
		}
		
		void CopyTile( int src, int dst, int y, Bitmap bmp ) {
			for( int yy = 0; yy < 16; yy++ ) {
				for( int xx = 0; xx < 16; xx++ ) {
					animBitmap.SetPixel( dst + xx, y + yy,
					                    bmp.GetPixel( xx, src + yy ) );
				}
			}
		}
	}
}
