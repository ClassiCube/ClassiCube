using System;
using System.IO;
using ClassicalSharp.Network;
using SharpWave;
using SharpWave.Codecs;
using SharpWave.Codecs.Vorbis;
using SharpWave.Containers;

namespace Launcher2 {
	
	public sealed class SoundPatcher {

		string[] files, identifiers;
		string prefix;
		FileStream outData;
		StreamWriter outText;
		RawOut outDecoder;
		
		public SoundPatcher( string[] files, string prefix, string outputPath ) {
			this.files = files;
			this.prefix = prefix;
			InitOutput( outputPath );
		}
		
		public void FetchFiles( string baseUrl, ResourceFetcher fetcher ) {
			identifiers = new string[files.Length];
			for( int i = 0; i < files.Length; i++ )
				identifiers[i] = prefix + files[i];
			
			for( int i = 0; i < files.Length; i++ ) {
				string url = baseUrl + files[i] + ".ogg";
				fetcher.downloader.DownloadData( url, false, identifiers[i] );
			}
		}
		
		public bool CheckDownloaded( ResourceFetcher fetcher, Action<string> setStatus ) {
			for( int i = 0; i < identifiers.Length; i++ ) {
				DownloadedItem item;
				if( fetcher.downloader.TryGetItem( identifiers[i], out item ) ) {
					Console.WriteLine( "found sound " + identifiers[i] );
					if( item.Data == null ) {
						setStatus( "&cFailed to download " + identifiers[i] );
						return false;
					}
					DecodeSound( files[i], (byte[])item.Data );
					
					// TODO: setStatus( next );
					if( i == identifiers.Length - 1 ) {
						Dispose();
					}
				}
			}
			return true;
		}
		
		void DecodeSound( string name, byte[] rawData ) {
			long start = outData.Position;
			using( MemoryStream ms = new MemoryStream( rawData ) ) {
				OggContainer container = new OggContainer( ms );
				outDecoder.PlayStreaming( container );
			}
			
			long len = outData.Position - start;
			outText.WriteLine( format, name, outDecoder.Frequency,
			                  outDecoder.BitsPerSample, outDecoder.Channels,
			                  start, len );
		}
		
		const string format = "{0},{1},{2},{3},{4},{5}";
		void InitOutput( string outputPath ) {
			outData = File.Create( outputPath + ".bin" );
			outText = new StreamWriter( outputPath + ".txt" );
			outDecoder = new RawOut( outData, true );
			
			outText.WriteLine( "# This file indicates where the various raw decompressed sound data " +
			                  "are found in the corresponding raw .bin file." );
			outText.WriteLine( "# Each line is in the following format:" );
			outText.WriteLine( "# Identifier, Frequency/Sample rate, BitsPerSample, " +
			                  "Channels, Offset from start, Length in bytes" );
		}
		
		void Dispose() {
			outDecoder.Dispose();
			outData.Close();
			outText.Close();
		}
	}
}
