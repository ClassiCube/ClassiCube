// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using ClassicalSharp;
using ClassicalSharp.Textures;

namespace Launcher.Patcher {
	
	public partial class ResourcePatcher {

		public ResourcePatcher( ResourceFetcher fetcher ) {
			jarClassic = fetcher.jarClassic;
			jar162 = fetcher.jar162;
			pngTerrainPatch = fetcher.pngTerrainPatch;
			pngGuiPatch = fetcher.pngGuiPatch;
		}
		ZipReader reader;
		ZipWriter writer;
		Bitmap animBitmap;
		List<string> existing = new List<string>();
		
		byte[] jarClassic, jar162, pngTerrainPatch, pngGuiPatch;
		public void Run() {
			reader = new ZipReader();
			reader.ShouldProcessZipEntry = ShouldProcessZipEntry_Classic;
			reader.ProcessZipEntry = ProcessZipEntry_Classic;
			string texDir = Path.Combine( Program.AppDirectory, "texpacks" );
			string path = Path.Combine( texDir, "default.zip" );
			ExtractExisting( path );
			
			using( Stream dst = new FileStream( path, FileMode.Create, FileAccess.Write ) ) {
				writer = new ZipWriter( dst );
				writer.entries = new ZipEntry[100];
				for( int i = 0; i < entries.Count; i++ )
					writer.WriteZipEntry( entries[i], datas[i] );
				
				ExtractClassic();
				ExtractModern();
				if( pngGuiPatch != null ) {
					using( Bitmap guiBitmap = new Bitmap( new MemoryStream( pngGuiPatch ) ) )
						writer.WriteNewImage( guiBitmap, "gui.png" );
				}
				writer.WriteCentralDirectoryRecords();
			}
		}
		
		
		#region From default.zip
		
		List<ZipEntry> entries = new List<ZipEntry>();
		List<byte[]> datas = new List<byte[]>();
		void ExtractExisting( string path ) {
			if( !File.Exists( path ) ) return;
			
			using( Stream src = new FileStream( path, FileMode.Open, FileAccess.Read ) ) {
				reader.ShouldProcessZipEntry = (file) => true;
				reader.ProcessZipEntry = ExtractExisting;
				reader.Extract( src );
			}
		}
		
		void ExtractExisting( string filename, byte[] data, ZipEntry entry ) {
			filename = ResourceList.GetFile( filename );
			entry.Filename = filename;
			existing.Add( filename );
			entries.Add( entry );
			datas.Add( data );
		}
		
		#endregion
		
		#region From classic
		
		void ExtractClassic() {
			if( jarClassic == null ) return;
			using( Stream src = new MemoryStream( jarClassic ) ) {
				reader.ShouldProcessZipEntry = ShouldProcessZipEntry_Classic;
				reader.ProcessZipEntry = ProcessZipEntry_Classic;
				reader.Extract( src );
			}
		}
		
		bool ShouldProcessZipEntry_Classic( string filename ) {
			return filename.StartsWith( "gui" )
				|| filename.StartsWith( "mob" ) || filename.IndexOf( '/' ) < 0;
		}
		
		StringComparison comp = StringComparison.OrdinalIgnoreCase;
		void ProcessZipEntry_Classic( string filename, byte[] data, ZipEntry entry ) {
			if( !filename.EndsWith( ".png", comp ) ) return;
			entry.Filename = ResourceList.GetFile( filename );
			
			if( entry.Filename != "terrain.png" ) {
				if( entry.Filename == "gui.png" )
					entry.Filename = "gui_classic.png";
				if( !existing.Contains( entry.Filename ) )
					writer.WriteZipEntry( entry, data );
				return;
			} else if( !existing.Contains( "terrain.png" ) ){
				using( Bitmap dstBitmap = new Bitmap( new MemoryStream( data ) ),
				      maskBitmap = new Bitmap( new MemoryStream( pngTerrainPatch ) ) ) {
					PatchImage( dstBitmap, maskBitmap );
					writer.WriteNewImage( dstBitmap, "terrain.png" );
				}
			}
		}
		
		#endregion
		
		#region From Modern
		
		void ExtractModern() {
			if( jar162 == null ) return;
			
			using( Stream src = new MemoryStream( jar162 ) ) {
				// Grab animations and snow
				animBitmap = new Bitmap( 1024, 64, PixelFormat.Format32bppArgb );
				reader.ShouldProcessZipEntry = ShouldProcessZipEntry_Modern;
				reader.ProcessZipEntry = ProcessZipEntry_Modern;
				reader.Extract( src );
				
				if( !existing.Contains( "animations.png" ) )
					writer.WriteNewImage( animBitmap, "animations.png" );
				if( !existing.Contains( "animations.txt" ) )
					writer.WriteNewString( animationsTxt, "animations.txt" );
				animBitmap.Dispose();
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
			entry.Filename = ResourceList.GetFile( filename );
			switch( filename ) {
				case "assets/minecraft/textures/environment/snow.png":
					if( !existing.Contains( "snow.png" ) )
						writer.WriteZipEntry( entry, data );
					break;
				case "assets/minecraft/textures/entity/chicken.png":
					if( !existing.Contains( "chicken.png" ) )
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
		
		#endregion
		
		unsafe void PatchImage( Bitmap dstBitmap, Bitmap maskBitmap ) {
			using( FastBitmap dst = new FastBitmap( dstBitmap, true, false ),
			      src = new FastBitmap( maskBitmap, true, true ) ) {
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
