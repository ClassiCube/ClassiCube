using System;
using System.IO;
using System.Threading;
using SharpWave;
using SharpWave.Codecs.Vorbis;

namespace ClassicalSharp.Audio {
	
	public sealed partial class AudioManager {
		
		IAudioOutput musicOut, soundOut;
		string[] musicFiles;
		Thread musicThread, soundThread;
		
		public AudioManager() {
			musicFiles = Directory.GetFiles( "audio", "*.ogg" );
		}
		
		public void SetMusic( bool enabled ) {
			if( enabled )
				InitMusic();
			else
				DisposeMusic();
		}
		
		void InitMusic() {
			disposingMusic = false;
			musicThread = MakeThread( DoMusicThread, ref musicOut,
			                         "ClassicalSharp.DoMusic" );
		}
		
		EventWaitHandle waitHandle = new EventWaitHandle( false, EventResetMode.AutoReset );
		void DoMusicThread() {
			Random rnd = new Random();
			while( !disposingMusic ) {
				string file = musicFiles[rnd.Next( 0, musicFiles.Length )];
				Console.WriteLine( "playing music file: " + file );
				using( FileStream fs = File.OpenRead( file ) ) {
					OggContainer container = new OggContainer( fs );
					musicOut.PlayStreaming( container );
				}
				if( disposingMusic ) break;
				
				int delay = 2000 * 60 + rnd.Next( 0, 5000 * 60 );
				waitHandle.WaitOne( delay );
			}
		}
		
		bool disposingMusic, disposingSound;
		public void Dispose() {
			DisposeMusic();
			DisposeSound();
			waitHandle.Close();
		}
		
		void DisposeMusic() {
			disposingMusic = true;
			waitHandle.Set();
			DisposeOf( ref musicOut, ref musicThread );
		}
		
		Thread MakeThread( ThreadStart func, ref IAudioOutput output, string name ) {
			if( OpenTK.Configuration.RunningOnWindows )
				output = new WinMmOut();
			else
				output = new OpenALOut();
			output.Create( 5 );
			
			Thread thread = new Thread( func );
			thread.Name = name;
			thread.IsBackground = true;
			thread.Start();
			return thread;
		}
		
		void DisposeOf( ref IAudioOutput output, ref Thread thread ) {
			if( output == null ) return;
			output.Stop();
			thread.Join();
			
			output.Dispose();
			output = null;
			thread = null;
		}
	}
}
