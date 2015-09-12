using System;
using System.Drawing;
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
		static int resourcesCount = 2;
		
		public void Run( MainForm form ) {
			using( WebClient client = new GZipWebClient() ) {
				WebRequest.DefaultWebProxy = null;
				int i = 0;
				DownloadData( classicJarUri, client, "classic.jar", form, ref i );
				DownloadData( terrainPatchUri, client, "terrain-patch.png", form, ref i );
			}
			
			reader = new ZipReader();
			reader.ShouldProcessZipEntry = ShouldProcessZipEntry_Classic;
			reader.ProcessZipEntry = ProcessZipEntry_Classic;
			using( FileStream src = new FileStream( "classic.jar", FileMode.Open, FileAccess.Read, FileShare.Read ),
			      dst = new FileStream( "default.zip", FileMode.Create, FileAccess.Write ) ) {
				writer = new ZipWriter( dst );
				reader.Extract( src );
				writer.WriteCentralDirectoryRecords();
			}
			File.Move( "terrain-patch.png", "terrain-patched.png" );
		}
		ZipReader reader;
		ZipWriter writer;
		
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
				writer.WriteTerrainImage( dstBitmap, entry );
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
