// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using ClassicalSharp;
using ClassicalSharp.Textures;

namespace Launcher.Patcher {
	
	public static class BinUnpacker {

		public static void Unpack( string dir, string group ) {
			string binFile = Path.Combine( dir, group + ".bin" );
			if( !File.Exists( binFile ) ) return;
			
			byte[] data = File.ReadAllBytes( binFile );
			UnpackBin( data, dir, group );
			File.Delete( binFile );
		}
		
		static void UnpackBin( byte[] data, string dir, string group ) {
			string propsFile = Path.Combine( dir, group + ".txt" );
			using( StreamReader reader = new StreamReader( propsFile ) ) {
				string line;
				while( (line = reader.ReadLine()) != null ) {
					if( line.Length == 0 || line[0] == '#' ) continue;
					string[] parts = line.Split( ',' );
					if( parts.Length < 6 ) continue;
					string name = parts[0].ToLower();
					int sampleRate, bitsPerSample, channels;
					int offset, length;
					
					if( !Int32.TryParse( parts[1], out sampleRate ) ||
					   !Int32.TryParse( parts[2], out bitsPerSample ) ||
					   !Int32.TryParse( parts[3], out channels ) ||
					   !Int32.TryParse( parts[4], out offset ) ||
					   !Int32.TryParse( parts[5], out length ) )
						continue;
					
					string file = Path.Combine( dir, group + "_" + name + ".wav" );
					using( FileStream fs = File.Create( file ) )
						WriteWave( fs, data, offset, length,
						          channels, sampleRate, bitsPerSample );
				}
			}
			File.Delete( propsFile );
		}
		
		static void WriteWave( Stream stream, byte[] data, int dataOffset, int dataLen,
		                      int channels, int frequency, int bitsPerSample ) {
			BinaryWriter w = new BinaryWriter( stream );
			WriteFourCC( w, "RIFF" );
			w.Write( 4 + (8 + 16) + (8 + dataLen) );
			WriteFourCC( w, "WAVE" );
			
			WriteFourCC( w, "fmt " );
			w.Write( 16 );
			w.Write( (ushort)1 ); // audio format, PCM
			w.Write( (ushort)channels );
			w.Write( frequency );
			w.Write((frequency * channels * bitsPerSample) / 8 ); // byte rate
			w.Write( (ushort)((channels * bitsPerSample) / 8) ); // block align
			w.Write( (ushort)bitsPerSample );
			
			WriteFourCC( w, "data" );
			w.Write( dataLen );
			w.Write( data, dataOffset, dataLen );
		}
		
		static void WriteFourCC( BinaryWriter w, string fourCC ) {
			w.Write( (byte)fourCC[0] ); w.Write( (byte)fourCC[1] );
			w.Write( (byte)fourCC[2] ); w.Write( (byte)fourCC[3] );
		}
	}
}
