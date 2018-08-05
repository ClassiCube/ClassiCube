using System;
using System.Collections.Generic;
using System.IO;
using System.Threading;
using SharpWave.Codecs;
using SharpWave.Containers;

namespace SharpWave {
	
	public struct AudioFormat { 
		public int Channels, BitsPerSample, SampleRate;
		
		public bool Equals(AudioFormat a) {
			return Channels == a.Channels && BitsPerSample == a.BitsPerSample && SampleRate == a.SampleRate;
		}
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
		public void PlayStreaming(IMediaContainer container) {
			container.ReadMetadata();
			ICodec codec = container.GetAudioCodec();
			AudioFormat format = codec.ReadHeader(container);
			
			SetFormat(format);
			IEnumerator<AudioChunk> chunks =
				codec.StreamData(container).GetEnumerator();
			
			bool noData = false;
			for (;;) {
				// Have any of the buffers finished playing
				if (noData && IsFinished()) return;

				int next = -1;
				for (int i = 0; i < NumBuffers; i++) {
					if (IsCompleted(i)) { next = i; break; }
				}

				if (next == -1) {
				} else if (pendingStop || !chunks.MoveNext()) {
					noData = true;
				} else {
					PlayData(next, chunks.Current);
				}
				Thread.Sleep(1);
			}
		}
	}
	
	/// <summary> Outputs raw audio to the given stream in the constructor. </summary>
	public unsafe sealed partial class RawOut : IAudioOutput {
		public readonly Stream OutStream;
		public readonly bool LeaveOpen;
		public Action OnGotMetadata;
		
		public RawOut(FileStream outStream, bool leaveOpen) {
			OutStream = outStream;
			LeaveOpen = leaveOpen;
		}
		
		public override void Create(int numBuffers) { NumBuffers = numBuffers; }
		public override void SetVolume(float volume) { }
		
		public override void SetFormat(AudioFormat format) {
			Format = format;
			if (OnGotMetadata != null) OnGotMetadata();
		}
		
		public override void PlayData(int index, AudioChunk chunk) {
			OutStream.Write(chunk.Data, 0, chunk.Length);
		}
		
		public override void Dispose() {
			if (LeaveOpen) return;
			OutStream.Close();
		}
		
		public override bool IsCompleted(int index) { return true; }
		public override bool IsFinished() { return true; }
	}
}