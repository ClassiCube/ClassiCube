using System;
using System.Drawing;
using System.IO;
using System.IO.Compression;
using System.Text;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Model;

namespace ClassicalSharp.TexturePack {

	public sealed class ZipExtractor {
		
		Game game;
		public ZipExtractor( Game game ) {
			this.game = game;
		}
		
		static Encoding enc = Encoding.ASCII;
		public void Extract( Stream stream ) {
			
			BinaryReader reader = new BinaryReader( stream );
			reader.BaseStream.Seek( -22, SeekOrigin.End );
			uint sig = reader.ReadUInt32();
			if( sig != 0x06054b50 ) {
				Utils.LogWarning( "Comment in .zip file must be empty" );
				return;
			}
			int entriesCount, centralDirectoryOffset;
			ReadEndOfCentralDirectory( reader, out entriesCount, out centralDirectoryOffset );
			ZipData[] entries = new ZipData[entriesCount];
			reader.BaseStream.Seek( centralDirectoryOffset, SeekOrigin.Begin );
			
			// Read all the central directory entries
			while( true ) {
				sig = reader.ReadUInt32();
				if( sig == 0x02014b50 ) {
					ReadCentralDirectory( reader, entries );
				} else if( sig == 0x06054b50 ) {
					break;
				} else {
					throw new NotSupportedException( "Unsupported signature: " + sig.ToString( "X8" ) );
				}
			}
			
			// Now read the local file header entries
			for( int i = 0; i < entriesCount; i++ ) {
				ZipData entry = entries[i];
				reader.BaseStream.Seek( entry.LocalHeaderOffset, SeekOrigin.Begin );
				sig = reader.ReadUInt32();
				if( sig != 0x04034b50 )
					throw new NotSupportedException( "Unsupported signature: " + sig.ToString( "X8" ) );
				ReadLocalFileHeader( reader, entry );
			}
		}
		
		struct ZipData {
			public int CompressedSize, UncompressedSize;
			public int LocalHeaderOffset;
		}
		
		void ReadLocalFileHeader( BinaryReader reader, ZipData entry ) {
			ushort versionNeeded = reader.ReadUInt16();
			ushort flags = reader.ReadUInt16();
			ushort compressionMethod = reader.ReadUInt16();
			reader.ReadUInt32(); // last modified
			reader.ReadUInt32(); // CRC 32
			
			int compressedSize = reader.ReadInt32();
			if( compressedSize == 0 ) compressedSize = entry.CompressedSize;
			int uncompressedSize = reader.ReadInt32();
			if( uncompressedSize == 0 ) uncompressedSize = entry.UncompressedSize;
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
		
		int index;
		void ReadCentralDirectory( BinaryReader reader, ZipData[] entries ) {
			reader.ReadUInt16(); // OS
			ushort versionNeeded = reader.ReadUInt16();
			ushort flags = reader.ReadUInt16();
			ushort compressionMethod = reader.ReadUInt16();
			reader.ReadUInt32(); // last modified
			reader.ReadUInt32(); // CRC 32
			int compressedSize = reader.ReadInt32();
			int uncompressedSize = reader.ReadInt32();
			ushort fileNameLen = reader.ReadUInt16();
			ushort extraFieldLen = reader.ReadUInt16();
			
			ushort fileCommentLen = reader.ReadUInt16();
			ushort diskNum = reader.ReadUInt16();
			ushort internalAttributes = reader.ReadUInt16();
			uint externalAttributes = reader.ReadUInt32();
			int localHeaderOffset = reader.ReadInt32();
			string fileName = enc.GetString( reader.ReadBytes( fileNameLen ) );
			reader.ReadBytes( extraFieldLen );
			reader.ReadBytes( fileCommentLen );
			
			ZipData entry;
			entry.CompressedSize = compressedSize;
			entry.UncompressedSize = uncompressedSize;
			entry.LocalHeaderOffset = localHeaderOffset;
			entries[index++] = entry;
		}
		
		void ReadEndOfCentralDirectory( BinaryReader reader, out int entriesCount, out int centralDirectoryOffset ) {
			ushort diskNum = reader.ReadUInt16();
			ushort diskNumStart = reader.ReadUInt16();
			ushort diskEntries = reader.ReadUInt16();
			entriesCount = reader.ReadUInt16();
			int centralDirectorySize = reader.ReadInt32();
			centralDirectoryOffset = reader.ReadInt32();
			ushort commentLength = reader.ReadUInt16();
		}
		
		byte[] DecompressEntry( BinaryReader reader, ushort compressionMethod, int compressedSize, int uncompressedSize ) {
			if( compressionMethod == 0 ) { // Store/Raw
				return reader.ReadBytes( uncompressedSize );
			} else if( compressionMethod == 8 ) { // Deflate
				byte[] data = new byte[uncompressedSize];
				byte[] compressedData = reader.ReadBytes( compressedSize );
				MemoryStream stream = new MemoryStream( compressedData );
				int index = 0, read = 0;
				DeflateStream deflater = new DeflateStream( stream, CompressionMode.Decompress );
				
				while( index < uncompressedSize &&
				      ( read = deflater.Read( data, index, data.Length - index ) ) > 0 ) {
					index += read;
				}
				
				deflater.Close();
				return data;
			} else {
				Utils.LogWarning( "Unsupported .zip entry compression method: " + compressionMethod );
				reader.ReadBytes( compressedSize );
				return null;
			}
		}
		
		void HandleZipEntry( string filename, byte[] data ) {
			MemoryStream stream = new MemoryStream( data );
			ModelCache cache = game.ModelCache;
			IGraphicsApi api = game.Graphics;
			
			switch( filename ) {
				case "terrain.png":
					game.ChangeTerrainAtlas( new Bitmap( stream ) ); break;
				case "mob/chicken.png":
				case "chicken.png":
					UpdateTexture( ref cache.ChickenTexId, stream, false ); break;
				case "mob/creeper.png":
				case "creeper.png":
					UpdateTexture( ref cache.CreeperTexId, stream, false ); break;
				case "mob/pig.png":
				case "pig.png":
					UpdateTexture( ref cache.PigTexId, stream, false ); break;
				case "mob/sheep.png":
				case "sheep.png":
					UpdateTexture( ref cache.SheepTexId, stream, false ); break;
				case "mob/skeleton.png":
				case "skeleton.png":
					UpdateTexture( ref cache.SkeletonTexId, stream, false ); break;
				case "mob/spider.png":
				case "spider.png":
					UpdateTexture( ref cache.SpiderTexId, stream, false ); break;
				case "mob/zombie.png":
				case "zombie.png":
					UpdateTexture( ref cache.ZombieTexId, stream, false ); break;
				case "mob/sheep_fur.png":
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
