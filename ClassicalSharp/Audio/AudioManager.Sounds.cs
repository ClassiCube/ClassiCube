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
			if( digBoard == null ) 
				InitSoundboards();
			
			digChunk.Data = digBoard.Data;
			stepChunk.Data = stepBoard.Data;			
			soundThread = MakeThread( DoSoundThread, ref soundOut,
			                         "ClassicalSharp.DoSound" );
		}

		object soundLock = new object();
		Sound digSound = null, stepSound = null;
		AudioChunk digChunk = new AudioChunk(), 
		stepChunk = new AudioChunk();
		
		void DoSoundThread() {
			while( !disposingSound ) {
				bool playDig = false, playStep = false;
				lock( soundLock ) {
					MakeSound( ref digSound, ref playDig, digChunk );
					MakeSound( ref stepSound, ref playStep, stepChunk );
				}
				if( playDig )
					soundOut.PlayRaw( digChunk );
				if( playStep )
					soundOut.PlayRaw( stepChunk );
				
				if( !(playDig || playStep) )
					Thread.Sleep( 1 );
			}
		}
		
		public void PlayDigSound( SoundType type ) {
			PlaySound( type, digBoard, ref digSound );
		}
		
		public void PlayStepSound( SoundType type ) {
			PlaySound( type, stepBoard, ref stepSound );
		}
		
		void PlaySound( SoundType type, Soundboard board, ref Sound target ) {
			if( type == SoundType.None || soundOut == null ) 
				return;
			lock( soundLock )
				target = board.PlayRandomSound( type );
		}
		
		void MakeSound( ref Sound src, ref bool play, AudioChunk target ) {
			if( src == null ) return;
			play = true;
			
			target.Frequency = src.SampleRate;
			target.BitsPerSample = src.BitsPerSample;
			target.Channels = src.Channels;
			
			target.BytesOffset = src.Offset;
			target.BytesUsed = src.Length;
			src = null;
		}
		
		void DisposeSound() {
			disposingSound = true;
			DisposeOf( ref soundOut, ref soundThread );
			digChunk.Data = null;
			stepChunk.Data = null;
		}
		
		void InitSoundboards() {
			digBoard = new Soundboard();
			digBoard.Init( "dig" );
			stepBoard = new Soundboard();
			stepBoard.Init( "step" );
		}
	}
}
