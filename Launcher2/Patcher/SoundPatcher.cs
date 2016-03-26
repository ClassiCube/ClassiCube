// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.IO;
using ClassicalSharp.Network;
using SharpWave;
using SharpWave.Codecs.Vorbis;

namespace Launcher {
	
	public sealed class SoundPatcher {

		string[] files, identifiers;
		string prefix, nextAction;
		FileStream outData;
		StreamWriter outText;
		RawOut outDecoder;
		public bool Done;
		
		public SoundPatcher( string[] files, string prefix, 
		                    string nextAction, string outputPath ) {
			this.files = files;
			this.prefix = prefix;
			this.nextAction = nextAction;
			InitOutput( outputPath );
		}
		
		const StringComparison comp = StringComparison.OrdinalIgnoreCase;
 		public void FetchFiles( string baseUrl, string altBaseUrl, ResourceFetcher fetcher ) {
			identifiers = new string[files.Length];
			for( int i = 0; i < files.Length; i++ )
				identifiers[i] = prefix + files[i].Substring( 1 );
			
			for( int i = 0; i < files.Length; i++ ) {
				string loc = files[i][0] == 'A' ? baseUrl : altBaseUrl;
				string url = loc + files[i].Substring( 1 ) + ".ogg";
				fetcher.downloader.DownloadData( url, false, identifiers[i] );
			}
		}
		
		public bool CheckDownloaded( ResourceFetcher fetcher, Action<string> setStatus ) {
			if( Done ) return true;
			for( int i = 0; i < identifiers.Length; i++ ) {
				DownloadedItem item;
				if( fetcher.downloader.TryGetItem( identifiers[i], out item ) ) {
					Console.WriteLine( "got sound " + identifiers[i] );
					if( item.Data == null ) {
						setStatus( "&cFailed to download " + identifiers[i] );
						return false;
					}
					DecodeSound( files[i].Substring( 1 ), (byte[])item.Data );
					
					// TODO: setStatus( next );
					if( i == identifiers.Length - 1 ) {
						Dispose();
						Done = true;
						setStatus( fetcher.MakeNext( nextAction ) );
					} else {
						setStatus( fetcher.MakeNext( identifiers[i + 1] ) );
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
