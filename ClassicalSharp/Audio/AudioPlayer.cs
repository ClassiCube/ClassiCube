using System;
using System.IO;
using System.Threading;
using SharpWave;
using SharpWave.Codecs.Vorbis;

namespace ClassicalSharp.Audio {
	
	public sealed partial class AudioPlayer {
		
		IAudioOutput musicOut;
		IAudioOutput[] monoOutputs, stereoOutputs;
		string[] musicFiles;
		Thread musicThread;
		
		public AudioPlayer( Game game ) {
			game.UseMusic = Options.GetBool( OptionsKey.UseMusic, false );
			SetMusic( game.UseMusic );
			game.UseSound = Options.GetBool( OptionsKey.UseSound, false );
			SetSound( game.UseSound );
		}
		
		public void SetMusic( bool enabled ) {
			if( enabled )
				InitMusic();
			else
				DisposeMusic();
		}
		
		void InitMusic() {
			musicFiles = Directory.GetFiles( "audio", "*.ogg" );
			disposingMusic = false;
			musicThread = MakeThread( DoMusicThread, ref musicOut,
			                         "ClassicalSharp.DoMusic" );
		}
		
		EventWaitHandle musicHandle = new EventWaitHandle( false, EventResetMode.AutoReset );
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
				musicHandle.WaitOne( delay );
			}
		}
		
		bool disposingMusic;
		public void Dispose() {
			DisposeMusic();
			DisposeSound();
			musicHandle.Close();
		}
		
		void DisposeMusic() {
			disposingMusic = true;
			musicHandle.Set();
			DisposeOf( ref musicOut, ref musicThread );
		}
		
		Thread MakeThread( ThreadStart func, ref IAudioOutput output, string name ) {
			output = GetPlatformOut();
			output.Create( 5 );
			
			Thread thread = new Thread( func );
			thread.Name = name;
			thread.IsBackground = true;
			thread.Start();
			return thread;
		}
		
		IAudioOutput GetPlatformOut() {
			if( OpenTK.Configuration.RunningOnWindows )
				return new WinMmOut();
			return new OpenALOut();
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
