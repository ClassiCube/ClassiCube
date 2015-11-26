using System;
using OpenTK;
using SharpWave;

namespace ClassicalSharp.Audio {
	
	public sealed class AudioManager {
		
		IAudioOutput musicOut, soundOut;
		public AudioManager( Game game ) {
			Init( game );
		}
		
		void Init( Game game ) {
			if( Configuration.RunningOnWindows ) {
				musicOut = new WinMmOut();			
				soundOut = new WinMmOut();				
			} else {
				musicOut = new OpenALOut();
				soundOut = new OpenALOut();					
			}
			musicOut.Create( 4 );
			soundOut.Create( 4, musicOut );
		}
		
		double accumulator;
		Random rnd = new Random();
		public void Tick( double delta ) {
			accumulator += delta;
		}
		
		public void Dispose() {
			// TODO: stop playing current sound and/or music
			musicOut.Dispose();
			soundOut.Dispose();
		}
	}
}
