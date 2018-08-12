using System;
using System.Collections.Generic;
using System.IO;
using System.Threading;

namespace SharpWave {
	
	public struct AudioFormat { 
		public int Channels, BitsPerSample, SampleRate;
		
		public bool Equals(AudioFormat a) {
			return Channels == a.Channels && BitsPerSample == a.BitsPerSample && SampleRate == a.SampleRate;
		}
	}
	
	public sealed class AudioChunk {
		public byte[] Data;
		public int Length;
	}
	
	public delegate void Action();
	
	public abstract class IAudioOutput : IDisposable {
		public abstract void Create(int numBuffers);
		public abstract void SetVolume(float volume);
		public abstract void Dispose();
		
		public AudioFormat Format;
		public abstract void SetFormat(AudioFormat format);
		public abstract void PlayData(int index, AudioChunk chunk);
		public abstract bool IsCompleted(int index);
		public abstract bool IsFinished();
		
		public int NumBuffers;
		public bool pendingStop;
		public void PlayStreaming(Stream src) {
			VorbisCodec codec = new VorbisCodec();
			AudioFormat format = codec.ReadHeader(src);
			
			SetFormat(format);
			IEnumerator<AudioChunk> chunks = codec.StreamData(src).GetEnumerator();
			
			bool reachedEnd = false;
			for (;;) {
				// Have any of the buffers finished playing
				if (reachedEnd && IsFinished()) return;

				int next = -1;
				for (int i = 0; i < NumBuffers; i++) {
					if (IsCompleted(i)) { next = i; break; }
				}

				if (next == -1) {
				} else if (pendingStop || !chunks.MoveNext()) {
					reachedEnd = true;
				} else {
					PlayData(next, chunks.Current);
				}
				Thread.Sleep(1);
			}
		}
	}
}