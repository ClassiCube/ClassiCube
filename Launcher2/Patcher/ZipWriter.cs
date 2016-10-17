// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Text;
using ClassicalSharp.Textures;

namespace Launcher.Patcher {
	
	public sealed class ZipWriter {
		
		BinaryWriter writer;
		Stream stream;
		public ZipWriter( Stream stream ) {
			this.stream = stream;
			writer = new BinaryWriter( stream );
		}
		
		internal ZipEntry[] entries;
		internal int entriesCount;
		
		public void WriteZipEntry( ZipEntry entry, byte[] data ) {
			entry.CompressedDataSize = (int)entry.UncompressedDataSize;
			entry.LocalHeaderOffset = (int)stream.Position;
			entries[entriesCount++] = entry;
			WriteLocalFileEntry( entry, data, data.Length );
		}
		
		public void WriteNewImage( Bitmap bmp, string filename ) {
			MemoryStream data = new MemoryStream();
			bmp.Save( data, ImageFormat.Png );
			byte[] buffer = data.GetBuffer();
			WriteNewEntry( filename, buffer, (int)data.Length );
		}
		
		public void WriteNewString( string text, string filename ) {
			byte[] data = Encoding.ASCII.GetBytes( text );
			WriteNewEntry( filename, data, data.Length );
		}
		
		public void WriteNewEntry( string filename, byte[] data, int dataLength ) {
			ZipEntry entry = new ZipEntry();
			entry.UncompressedDataSize = dataLength;
			entry.Crc32 = CRC32( data, dataLength );
			entry.CompressedDataSize = dataLength;
			entry.LocalHeaderOffset = (int)stream.Position;
			
			entry.Filename = filename;
			entries[entriesCount++] = entry;
			WriteLocalFileEntry( entry, data, dataLength );
		}
		
		public void WriteCentralDirectoryRecords() {
			int dirOffset = (int)stream.Position;
			for( int i = 0; i < entriesCount; i++ ) {
				WriteCentralDirectoryHeaderEntry( entries[i] );
			}
			int dirSize = (int)( stream.Position - dirOffset );
			WriteEndOfCentralDirectoryRecord( (ushort)entriesCount, dirSize, dirOffset );
		}
		
		void WriteLocalFileEntry( ZipEntry entry, byte[] data, int length ) {
			writer.Write( 0x04034b50 ); // signature
			writer.Write( (ushort)20 ); // version needed
			writer.Write( (ushort)0 );  // bitflags
			writer.Write( (ushort)0 );  // compression method
			writer.Write( 0 );          // last modified
			writer.Write( entry.Crc32 );
			writer.Write( entry.CompressedDataSize );
			writer.Write( entry.UncompressedDataSize );
			writer.Write( (ushort)entry.Filename.Length );
			writer.Write( (ushort)0 );  // extra field length
			for( int i = 0; i < entry.Filename.Length; i++ )
				writer.Write( (byte)entry.Filename[i] );
			
			writer.Write( data, 0, length );
		}
		
		void WriteCentralDirectoryHeaderEntry( ZipEntry entry ) {
			writer.Write( 0x02014b50 ); // signature
			writer.Write( (ushort)20 ); // version		
			writer.Write( (ushort)20 ); // version needed
			writer.Write( (ushort)0 );  // bitflags
			writer.Write( (ushort)0 );  // compression method
			writer.Write( 0 );          // last modified
			writer.Write( entry.Crc32 );
			writer.Write( entry.CompressedDataSize );
			writer.Write( entry.UncompressedDataSize );
			
			writer.Write( (ushort)entry.Filename.Length );
			writer.Write( (ushort)0 );  // extra field length
			writer.Write( (ushort)0 );  // file comment length
			writer.Write( (ushort)0 );  // disk number
			writer.Write( (ushort)0 );  // internal attributes
			writer.Write( 0 );          // external attributes
			writer.Write( entry.LocalHeaderOffset );
			for( int i = 0; i < entry.Filename.Length; i++ )
				writer.Write( (byte)entry.Filename[i] );
		}
		
		void WriteEndOfCentralDirectoryRecord( ushort entries, int centralDirSize, int centralDirOffset ) {
			writer.Write( 0x06054b50 ); // signature
			writer.Write( (ushort)0 );  // disk number	
			writer.Write( (ushort)0 );  // disk number of start
			writer.Write( entries );    // disk entries
			writer.Write( entries );    // total entries
			writer.Write( centralDirSize );
			writer.Write( centralDirOffset );
			writer.Write( (ushort)0 );  // comment length
		}
		
		static uint CRC32( byte[] data, int length ) {
			uint crc = 0xffffffffU;
			for( int i = 0; i < length; i++ ) {
				crc ^= data[i];
				for( int j = 0; j < 8; j++ )
					crc = (crc >> 1) ^ (crc & 1) * 0xEDB88320;
			}
			return crc ^ 0xffffffffU;
		}
	}
}
