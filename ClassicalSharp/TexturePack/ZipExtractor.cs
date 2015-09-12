using System;
using System.Drawing;
using System.IO;
using System.IO.Compression;
using System.Text;
using ClassicalSharp.Model;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp.TexturePack {

	public sealed class ZipExtractor {
		
		Game game;
		public ZipExtractor( Game game ) {
			this.game = game;
		}
		
		static Encoding enc = Encoding.ASCII;
		public void Extract( Stream stream ) {
			
			BinaryReader reader = new BinaryReader( stream );
			while( true ) {
				uint sig = reader.ReadUInt32();
				if( sig == 0x04034b50 ) { // local file header
					ReadLocalFileHeader( reader );
				} else if( sig == 0x02014b50 ) { // central directory header
					break;
				} else {
					throw new NotSupportedException( "Unsupported signature: " + sig.ToString( "X8" ) );
				}
			}
		}
		
		void ReadLocalFileHeader( BinaryReader reader ) {
			ushort versionNeeded = reader.ReadUInt16();
			ushort flags = reader.ReadUInt16();
			ushort compressionMethod = reader.ReadUInt16();
			reader.ReadUInt32(); // last modified
			reader.ReadUInt32(); // CRC 32
			int compressedSize = reader.ReadInt32();
			int uncompressedSize = reader.ReadInt32();
			ushort fileNameLen = reader.ReadUInt16();
			ushort extraFieldLen = reader.ReadUInt16();
			string fileName = enc.GetString( reader.ReadBytes( fileNameLen ) );
			reader.ReadBytes( extraFieldLen );
			
			if( versionNeeded > 20 )
				Utils.LogWarning( "May not be able to properly extract a .zip enty with a version later than 2.0" );
			
			byte[] data = DecompressEntry( reader, compressionMethod, compressedSize, uncompressedSize );
			if( data != null )
				HandleZipEntry( fileName, data );
		}
		
		byte[] DecompressEntry( BinaryReader reader, ushort compressionMethod, int compressedSize, int uncompressedSize ) {
			if( compressionMethod == 0 ) { // Store/Raw
				return reader.ReadBytes( uncompressedSize );
			} else if( compressionMethod == 8 ) { // Deflate
				byte[] data = new byte[uncompressedSize];
				int index = 0, read = 0;
				DeflateStream deflater = new DeflateStream( reader.BaseStream, CompressionMode.Decompress );
				while( index < uncompressedSize &&
				      ( read = deflater.Read( data, index, data.Length - index ) ) > 0 ) {
					index += read;
				}
				return data;
			} else {
				Utils.LogWarning( "Unsupported .zip entry compression method: " + compressionMethod );
				reader.ReadBytes( compressedSize );
				return null;
			}
		}
		
		/*ChickenTexId, CreeperTexId, PigTexId, SheepTexId,
		SkeletonTexId, SpiderTexId, ZombieTexId, SheepFurTexId, HumanoidTexId;*/
		void HandleZipEntry( string filename, byte[] data ) {
			Console.WriteLine( filename );
			MemoryStream stream = new MemoryStream( data );
			ModelCache cache = game.ModelCache;
			IGraphicsApi api = game.Graphics;
			
			switch( filename ) {
				case "terrain.png":
					game.ChangeTerrainAtlas( new Bitmap( stream ) ); break;				
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
				case "animations.png":
				case "animation.png":
					game.Animations.SetAtlas( new Bitmap( stream ) ); break;
			}
		}
		
		void UpdateTexture( ref int texId, Stream stream, bool setSkinType ) {
			game.Graphics.DeleteTexture( ref texId );
			using( Bitmap bmp = new Bitmap( stream ) ) {
				if( setSkinType )
					game.DefaultPlayerSkinType = Utils.GetSkinType( bmp );
				texId = game.Graphics.CreateTexture( bmp );
			}
		}
	}
}
