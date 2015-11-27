using System;
using System.IO;
using System.Threading;
using OpenTK;
using SharpWave;
using SharpWave.Codecs;
using SharpWave.Codecs.Vorbis;

namespace ClassicalSharp.Audio {
	
	public sealed partial class AudioManager {
		
		Soundboard digBoard, stepBoard;
		
		public void SetSound( bool enabled ) {
			if( enabled )
				InitSound();
			else
				DisposeSound();
		}
		
		void InitSound() {
			disposingSound = false;
			if( digBoard == null ) InitSoundboards();
			soundThread = MakeThread( DoSoundThread, ref soundOut,
			                         "ClassicalSharp.DoSound" );
		}
		
		AudioChunk soundChunk = new AudioChunk();
		object soundLock = new object();
		volatile int soundsCount = 0;
		const int maxSounds = 5;
		Sound[] sounds = new Sound[maxSounds];
		byte[][] soundDatas = new byte[maxSounds][];
		
		void DoSoundThread() {
			while( !disposingSound ) {
				bool playSound = false;
				lock( soundLock ) {
					if( soundsCount > 0 ) {
						soundChunk.Data = soundDatas[0];
						Sound meta = sounds[0];
						playSound = true;
						RemoveOldestSound();
						
						soundChunk.Frequency = meta.SampleRate;
						soundChunk.BitsPerSample = meta.BitsPerSample;
						soundChunk.Channels = meta.Channels;
						soundChunk.BytesOffset = meta.Offset;
						soundChunk.BytesUsed = meta.Length;
					}
				}
				if( playSound )
					soundOut.PlayRaw( soundChunk );
				Thread.Sleep( 1 );
			}
		}
		
		public void PlayDigSound( SoundType type ) {
			PlaySound( type, digBoard );
		}
		
		public void PlayStepSound( SoundType type ) {
			PlaySound( type, stepBoard );
		}
		
		void PlaySound( SoundType type, Soundboard board ) {
			if( type == SoundType.None || soundOut == null ) 
				return;
			Sound sound = board.PlayRandomSound( type );
			
			lock( soundLock ) {
				if( soundsCount == maxSounds )
					RemoveOldestSound();
				sounds[soundsCount] = sound;
				soundDatas[soundsCount] = board.Data;
				soundsCount++;
			}
		}
		
		void RemoveOldestSound() {
			for( int i = 0; i < maxSounds - 1; i++ ) {
				sounds[i] = sounds[i + 1];
				soundDatas[i] = soundDatas[i + 1];
			}
			
			sounds[maxSounds - 1] = null;
			soundDatas[maxSounds - 1] = null;
			soundsCount--;
		}
		
		void DisposeSound() {
			disposingSound = true;
			DisposeOf( ref soundOut, ref soundThread );
		}
		
		void InitSoundboards() {
			digBoard = new Soundboard();
			digBoard.Init( "dig" );
			stepBoard = new Soundboard();
			stepBoard.Init( "step" );
		}
	}
}
