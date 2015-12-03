using System;
using System.IO;
using System.Threading;
using OpenTK;
using SharpWave;
using SharpWave.Codecs;
using SharpWave.Codecs.Vorbis;

namespace ClassicalSharp.Audio {
	
	public sealed partial class AudioPlayer {
		
		Soundboard digBoard, stepBoard;
		const int maxSounds = 10;
		Sound[] pending;
		int pendingCount;
		
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
			
			int freq = digBoard.rawSounds[0].SampleRate;
			soundContainer = new BinContainer( freq, maxSounds );
			pending = new Sound[maxSounds];
			soundCodec = (BinCodec)soundContainer.GetAudioCodec();
			soundThread = MakeThread( DoSoundThread, ref soundOut,
			                         "ClassicalSharp.DoSound" );
		}

		EventWaitHandle soundHandle = new EventWaitHandle( false, EventResetMode.AutoReset );
		BinContainer soundContainer;
		BinCodec soundCodec;
		
		void DoSoundThread() {
			while( !disposingSound ) {
				soundHandle.WaitOne();
				if( disposingSound ) break;
				
				soundOut.PlayStreaming( soundContainer );
			}
		}
		
		public void Tick( double delta ) {
			if( pendingCount > 0 ) {
				Sound snd = pending[0];
				byte[] data = snd.Metadata == 1 ? digBoard.Data : stepBoard.Data;
				
				soundCodec.AddSound( data, snd.Offset, snd.Length, snd.Channels );
				RemoveFirstPendingSound();
				soundHandle.Set();
			}
		}
		
		DateTime lastDig = DateTime.MinValue;
		public void PlayDigSound( SoundType type ) {
			bool immediate = (DateTime.UtcNow - lastDig).TotalMilliseconds > 100;
			PlaySound( type, digBoard, immediate );
			lastDig = DateTime.UtcNow;
		}
		
		public void PlayStepSound( SoundType type ) {
			PlaySound( type, stepBoard, true );
		}
		
		void PlaySound( SoundType type, Soundboard board, bool immediate ) {
			if( type == SoundType.None || soundOut == null )
				return;
			Sound snd = board.PlayRandomSound( type );
			snd.Metadata = board == digBoard ? (byte)1 : (byte)2;
			
			if( immediate ) {
				byte[] data = board == digBoard ? digBoard.Data : stepBoard.Data;
				soundCodec.AddSound( data, snd.Offset, snd.Length, snd.Channels );
				soundHandle.Set();
			} else {
				if( pendingCount == maxSounds )
					RemoveFirstPendingSound();
				pending[pendingCount++] = snd;
			}
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
		
		void RemoveFirstPendingSound() {
			for( int i = 1; i < maxSounds - 1; i++ )
				pending[i] = pending[i + 1];
			pending[maxSounds - 1] = null;
			pendingCount--;
		}
		
		void DisposeSound() {
			disposingSound = true;
			soundHandle.Set();
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
