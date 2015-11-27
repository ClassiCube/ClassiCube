using System;
using System.IO;
using System.Threading;
using OpenTK;
using SharpWave;
using SharpWave.Codecs.Vorbis;

namespace ClassicalSharp.Audio {
	
	public sealed class AudioManager {
		
		IAudioOutput musicOut, soundOut;
		Soundboard digBoard, stepBoard;
		string[] musicFiles;
		
		public AudioManager() {
			digBoard = new Soundboard();
			digBoard.Init( "dig" );
			stepBoard = new Soundboard();
			stepBoard.Init( "step" );
			musicFiles = Directory.GetFiles( "audio", "*.ogg" );
		}
		
		public void SetState( bool state ) {
			if( state )
				Init();
			else
				Dispose();
		}
		
		public void Init() {
			// TODO: why is waveOut crashing?
			if( false/*Configuration.RunningOnWindows*/ ) {
				musicOut = new WinMmOut();			
				soundOut = new WinMmOut();				
			} else {
				musicOut = new OpenALOut();
				soundOut = new OpenALOut();
			}
			
			musicOut.Create( 5 );
			soundOut.Create( 5, musicOut );
			InitThreads();
		}
		
		Thread musicThread, soundThread;
		void InitThreads() {
			disposingMusic = false;
			musicThread = new Thread( DoMusicThread );
			musicThread.Name = "ClassicalSharp.DoMusicThread";
			musicThread.IsBackground = true;
			musicThread.Start();
		}
		
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
				Thread.Sleep( delay );
			}
		}
		
		bool disposingMusic;
		public void Dispose() {
			DisposeMusic();
			DisposeSound();
		}
		
		public void DisposeMusic() {
			disposingMusic = true;
			musicOut.Stop();
			musicThread.Join();
			musicOut.Dispose();
		}
		
		public void DisposeSound() {
			// TODO: stop playing current sound and/or music
			soundOut.Dispose();
		}
	}
}
