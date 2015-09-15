using System;
using System.IO;
using System.IO.Compression;
using System.Text;

namespace ClassicalSharp {

	public sealed class MapLvl {
		const uint Identifier = 0x271BB788;
		const byte Revision = 2;

		public static byte[] Load( Stream stream, Game game, out int width, out int height, out int length ) {
			using( GZipStream decompressed = new GZipStream( stream, CompressionMode.Decompress ) ) {
				BinaryReader reader = new BinaryReader( stream );
				if( reader.ReadInt32() != Identifier || reader.ReadByte() != Revision ) {
					throw new InvalidDataException( "Unexpected constant in .lvl file" );
				}
				
				
			}
			width = 0;
			length = 0;
			height = 0;
			return null;
		}
	}
}