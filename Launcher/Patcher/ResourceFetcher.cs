using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Net;
using System.Windows.Forms;
using ClassicalSharp;
using ClassicalSharp.TexturePack;

namespace Launcher {
	
	public class ResourceFetcher {
		
		const string classicJarUri = "http://s3.amazonaws.com/Minecraft.Download/versions/c0.30_01c/c0.30_01c.jar";
		const string modernJarUri = "http://s3.amazonaws.com/Minecraft.Download/versions/1.6.2/1.6.2.jar";
		const string terrainPatchUri = "http://static.classicube.net/terrain-patch.png";
		static int resourcesCount = 3;
		
		public void Run( MainForm form ) {
			using( WebClient client = new GZipWebClient() ) {
				WebRequest.DefaultWebProxy = null;
				int i = 0;
				DownloadData( classicJarUri, client, "classic.jar", form, ref i );
				DownloadData( modernJarUri, client, "1.6.2.jar", form, ref i );
				DownloadData( terrainPatchUri, client, "terrain-patch.png", form, ref i );
			}
			
			reader = new ZipReader();
			reader.ShouldProcessZipEntry = ShouldProcessZipEntry_Classic;
			reader.ProcessZipEntry = ProcessZipEntry_Classic;
			
			using( FileStream srcClassic = File.OpenRead( "classic.jar" ),
			      srcModern = File.OpenRead( "1.6.2.jar" ),
			      dst = new FileStream( "default.zip", FileMode.Create, FileAccess.Write ) ) {
				writer = new ZipWriter( dst );
				reader.Extract( srcClassic );
				
				// Grab animations and snow
				animBitmap = new Bitmap( 512, 32, PixelFormat.Format32bppArgb );
				reader.ShouldProcessZipEntry = ShouldProcessZipEntry_Modern;
				reader.ProcessZipEntry = ProcessZipEntry_Modern;
				reader.Extract( srcModern );
				writer.WriteNewImage( animBitmap, "animations.png" );
				animBitmap.Dispose();
				writer.WriteCentralDirectoryRecords();
			}
			
			if( !File.Exists( "terrain-patched.png" ) )
				File.Move( "terrain-patch.png", "terrain-patched.png" );
		}
		ZipReader reader;
		ZipWriter writer;
		Bitmap animBitmap;
		
		bool ShouldProcessZipEntry_Classic( string filename ) {
			return filename.StartsWith( "mob" ) || ( filename.IndexOf( '/' ) < 0 );
		}
		
		void ProcessZipEntry_Classic( string filename, byte[] data, ZipEntry entry ) {
			if( writer.entries == null )
				writer.entries = new ZipEntry[reader.entries.Length];
			if( filename != "terrain.png" ) {
				writer.WriteZipEntry( entry, data );
				return;
			}
			
			using( Bitmap dstBitmap = new Bitmap( new MemoryStream( data ) ),
			      maskBitmap = new Bitmap( "terrain-patch.png" ) ) {
				PatchImage( dstBitmap, maskBitmap );
				writer.WriteNewImage( dstBitmap, "terrain.png" );
			}
		}
		
		bool ShouldProcessZipEntry_Modern( string filename ) {
			return filename.StartsWith( "assets/minecraft/textures" ) &&
				( filename == "assets/minecraft/textures/environment/snow.png" ||
				 filename == "assets/minecraft/textures/blocks/water_still.png" ||
				 filename == "assets/minecraft/textures/blocks/lava_still.png" ||
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
					PatchAnimation( data, 0 );
					break;
				case "assets/minecraft/textures/blocks/lava_still.png":
					PatchAnimation( data, 16 );
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
		
		unsafe void PatchAnimation( byte[] data, int y ) {
			// Sadly files in modern are 24 rgb, so we can't use fastbitmap here
			using( Bitmap bmp = new Bitmap( new MemoryStream( data ) ) ) {			
				for( int tile = 0; tile < bmp.Height; tile += 16 ) {
					for( int yy = 0; yy < 16; yy++ ) {
						for( int xx = 0; xx < 16; xx++ ) {
							animBitmap.SetPixel( tile + xx, y + yy,
							                    bmp.GetPixel( xx, tile + yy ) );
						}
					}
				}
			}
		}
		
		public bool CheckAllResourcesExist() {
			return File.Exists( "default.zip" ) && File.Exists( "terrain-patched.png" );
		}
		
		class GZipWebClient : WebClient {
			
			protected override WebRequest GetWebRequest( Uri address ) {
				HttpWebRequest request = (HttpWebRequest)base.GetWebRequest( address );
				request.AutomaticDecompression = DecompressionMethods.GZip;
				return request;
			}
		}
		
		static bool DownloadData( string uri, WebClient client, string output, MainForm form, ref int i ) {
			i++;
			if( File.Exists( output ) ) return true;
			form.Text = MainForm.AppName + " - fetching " + output + "(" + i + "/" + resourcesCount + ")";
			
			try {
				client.DownloadFile( uri, output );
			} catch( WebException ex ) {
				Program.LogException( ex );
				MessageBox.Show( "Unable to download or save " + output, "Failed to download or save resource", MessageBoxButtons.OK, MessageBoxIcon.Error );
				return false;
			}
			return true;
		}
	}
}
