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
		public abstract void BufferData(int index, AudioChunk chunk);
		public abstract void Play();
		public abstract bool IsCompleted(int index);
		public abstract bool IsFinished();
		
		public int NumBuffers;
		public bool pendingStop;
		public void PlayData(int index, AudioChunk chunk) {
			BufferData(index, chunk);
			Play();
		}
		
		bool BufferBlock(AudioChunk tmp, AudioFormat fmt, IEnumerator<AudioChunk> chunks) {
			// decode up to around a second
			int secondSize = fmt.SampleRate * fmt.Channels * sizeof(short);
			tmp.Length = 0;			
			
			while (tmp.Length < secondSize) {
				if (!chunks.MoveNext()) return true;
				AudioChunk src = chunks.Current;
				
				Buffer.BlockCopy(src.Data, 0, tmp.Data, tmp.Length, src.Length);
				tmp.Length += src.Length;
			}
			return false;
		}
		
		public void PlayStreaming(Stream stream) {
			VorbisCodec codec = new VorbisCodec();
			AudioFormat fmt = codec.ReadHeader(stream);
			
			SetFormat(fmt);
			IEnumerator<AudioChunk> chunks = codec.StreamData(stream).GetEnumerator();
			AudioChunk tmp = new AudioChunk();
			
			// largest possible vorbis frame decodes to blocksize1 samples
			// so we may end up decoding slightly over a second of audio
			int chunkSize = (fmt.SampleRate + 8192) * fmt.Channels * sizeof(short);
			byte[][] data = new byte[NumBuffers][];
			for (int i = 0; i < NumBuffers; i++) { data[i] = new byte[chunkSize]; }
			
			bool reachedEnd = false;
			for (int i = 0; i < NumBuffers && !reachedEnd; i++) {
				tmp.Data = data[i];
				reachedEnd = BufferBlock(tmp, fmt, chunks);
				BufferData(i, tmp);
			}
			Play();
			
			for (; !reachedEnd;) {
				int next = -1;
				for (int i = 0; i < NumBuffers; i++) {
					if (IsCompleted(i)) { next = i; break; }
				}

				if (next == -1) { Thread.Sleep(10); continue; }
				if (pendingStop) break;

				tmp.Data = data[next];
				reachedEnd = BufferBlock(tmp, fmt, chunks);
				BufferData(next, tmp);
			}

			while (!IsFinished()) { Thread.Sleep(10); }
		}
	}
}