// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.IO;
using System.IO.Compression;
using System.Text;

namespace ClassicalSharp.TexturePack {

	public struct ZipEntry {
		public int CompressedDataSize, UncompressedDataSize;
		public int LocalHeaderOffset, CentralHeaderOffset;
		public uint Crc32;
		public string Filename;
	}
	
	/// <summary> Extracts files from a stream that represents a .zip file. </summary>
	public sealed class ZipReader {
		
		public Action<string, byte[], ZipEntry> ProcessZipEntry;
		public Func<string, bool> ShouldProcessZipEntry;
		public ZipEntry[] entries;
		int index;
		
		static Encoding enc = Encoding.ASCII;
		public void Extract( Stream stream ) {
			
			BinaryReader reader = new BinaryReader( stream );
			reader.BaseStream.Seek( -22, SeekOrigin.End );
			uint sig = reader.ReadUInt32();
			if( sig != 0x06054b50 ) {
				Utils.LogDebug( "Comment in .zip file must be empty" );
				return;
			}
			int entriesCount, centralDirectoryOffset;
			ReadEndOfCentralDirectory( reader, out entriesCount, out centralDirectoryOffset );
			entries = new ZipEntry[entriesCount];
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
				ZipEntry entry = entries[i];
				reader.BaseStream.Seek( entry.LocalHeaderOffset, SeekOrigin.Begin );
				sig = reader.ReadUInt32();
				if( sig != 0x04034b50 )
					throw new NotSupportedException( "Unsupported signature: " + sig.ToString( "X8" ) );
				ReadLocalFileHeader( reader, entry );
			}
			entries = null;
			index = 0;
		}
		
		void ReadLocalFileHeader( BinaryReader reader, ZipEntry entry ) {
			ushort versionNeeded = reader.ReadUInt16();
			ushort flags = reader.ReadUInt16();
			ushort compressionMethod = reader.ReadUInt16();
			reader.ReadUInt32(); // last modified
			reader.ReadUInt32(); // CRC 32
			
			int compressedSize = reader.ReadInt32();
			if( compressedSize == 0 ) compressedSize = entry.CompressedDataSize;
			int uncompressedSize = reader.ReadInt32();
			if( uncompressedSize == 0 ) uncompressedSize = entry.UncompressedDataSize;
			ushort fileNameLen = reader.ReadUInt16();
			ushort extraFieldLen = reader.ReadUInt16();
			string fileName = enc.GetString( reader.ReadBytes( fileNameLen ) );
			if( !ShouldProcessZipEntry( fileName ) ) return;
			
			reader.ReadBytes( extraFieldLen );
			if( versionNeeded > 20 )
				Utils.LogDebug( "May not be able to properly extract a .zip enty with a version later than 2.0" );
			
			byte[] data = DecompressEntry( reader, compressionMethod, compressedSize, uncompressedSize );
			if( data != null )
				ProcessZipEntry( fileName, data, entry );
		}
		
		void ReadCentralDirectory( BinaryReader reader, ZipEntry[] entries ) {
			ZipEntry entry;
			entry.CentralHeaderOffset = (int)(reader.BaseStream.Position - 4);
			reader.ReadUInt16(); // OS
			ushort versionNeeded = reader.ReadUInt16();
			ushort flags = reader.ReadUInt16();
			ushort compressionMethod = reader.ReadUInt16();
			reader.ReadUInt32(); // last modified
			uint crc32 = reader.ReadUInt32();
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
						
			entry.CompressedDataSize = compressedSize;
			entry.UncompressedDataSize = uncompressedSize;
			entry.LocalHeaderOffset = localHeaderOffset;
			entry.Filename = fileName;
			entry.Crc32 = crc32;
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
				DeflateStream inflater = new DeflateStream( stream, CompressionMode.Decompress );
				
				while( index < uncompressedSize &&
				      (read = inflater.Read( data, index, data.Length - index)) > 0 ) {
					index += read;
				}
				
				inflater.Dispose();
				return data;
			} else {
				Utils.LogDebug( "Unsupported .zip entry compression method: " + compressionMethod );
				reader.ReadBytes( compressedSize );
				return null;
			}
		}
	}
}
