using System;
using OpenTK.Audio;
using OpenTK.Audio.OpenAL;

namespace SharpWave {
	
	/// <summary> Outputs audio to the default sound playback device using the
	/// native OpenAL library. Cross platform. </summary>
	public unsafe sealed class OpenALOut : IAudioOutput {
		uint source = uint.MaxValue;
		uint[] bufferIDs;
		bool[] completed;
		
		ALFormat dataFormat;
		float volume = 1;
		static readonly object contextLock = new object();
		
		public override void SetVolume(float volume) {
			this.volume = volume;
			if (source == uint.MaxValue) return;
			
			AL.alSourcef(source, ALSourcef.Gain, volume);
			CheckError("SetVolume");
		}
		
		public override void Create(int numBuffers) {
			bufferIDs = new uint[numBuffers];
			completed = new bool[numBuffers];
			
			for (int i = 0; i < numBuffers; i++) {
				completed[i] = true;
			}
			NumBuffers = numBuffers;
			
			lock (contextLock) {
				if (context == IntPtr.Zero) CreateContext();
				contextRefs++;
			}
			
			AL.alDistanceModel(ALDistanceModel.None);
			CheckError("DistanceModel");
			Console.WriteLine("al context:" + context);
		}
		
		public override bool IsCompleted(int index) {
			int processed = 0;
			uint buffer = 0;

			AL.alGetSourcei(source, ALGetSourcei.BuffersProcessed, &processed);
			CheckError("GetSources");
			if (processed == 0) return completed[index];
			
			AL.alSourceUnqueueBuffers(source, 1, &buffer);
			CheckError("SourceUnqueueBuffers");
			
			for (int i = 0; i < NumBuffers; i++) {
				if (bufferIDs[i] == buffer) completed[i] = true;
			}
			return completed[index];
		}
		
		public override bool IsFinished() {
			if (source == uint.MaxValue) return true;
			for (int i = 0; i < NumBuffers; i++) {
				if (!IsCompleted(i)) return false;
			}
			
			int state = 0;
			AL.alGetSourcei(source, ALGetSourcei.SourceState, &state);
			return state != (int)ALSourceState.Playing;
		}
		
		public override void BufferData(int index, AudioChunk chunk) {
			fixed (byte* data = chunk.Data) {
				uint buffer = bufferIDs[index];
				completed[index] = false;

				AL.alBufferData(buffer, dataFormat, (IntPtr)data,
				                chunk.Length, Format.SampleRate);
				CheckError("BufferData");
				
				AL.alSourceQueueBuffers(source, 1, &buffer);
				CheckError("QueueBuffers");
			}
		}
		
		public override void Play() {
			AL.alSourcePlay(source);
			CheckError("SourcePlay");
		}
		
		void CheckError(string location) {
			ALError error = AL.alGetError();
			if (error == ALError.NoError) return;
			
			throw new AudioException("OpenAL error: " + error + " at " + location);
		}

		public override void Dispose() {
			DisposeSource();
			lock (contextLock) {
				contextRefs--;
				if (contextRefs == 0) DestroyContext();
			}
		}
		
		void DisposeSource() {
			if (source == uint.MaxValue) return;
			uint sourceU = source;
			
			AL.alDeleteSources(1, &sourceU);
			source = uint.MaxValue;
			CheckError("DeleteSources");
			
			fixed (uint* buffers = bufferIDs) {
				AL.alDeleteBuffers(NumBuffers, buffers);
				CheckError("DeleteBuffers");
			}
		}
		
		public override void SetFormat(AudioFormat format) {
			dataFormat = GetALFormat(format.Channels, format.BitsPerSample);
			// Don't need to recreate device if it's the same
			if (Format.Equals(format)) return;
			Format = format;
			
			DisposeSource();
			uint sourceU = 0;
			
			fixed (uint* buffers = bufferIDs) {
				AL.alGenSources(1, &sourceU);
				source = sourceU;
				CheckError("GenSources");
				
				AL.alGenBuffers(NumBuffers, buffers);
				CheckError("GenBuffers");
			}
			
			if (volume != 1) SetVolume(volume);
		}
		
		static ALFormat GetALFormat(int channels, int bitsPerSample) {
			if (bitsPerSample == 16) {
				if (channels == 1) return ALFormat.Mono16;
				if (channels == 2) return ALFormat.Stereo16;
			} else if (bitsPerSample == 8) {
				if (channels == 1) return ALFormat.Mono8;
				if (channels == 2) return ALFormat.Stereo8;
			}
			throw new NotSupportedException("Unsupported audio format: " + bitsPerSample + ", " + channels);
		}

		
		static IntPtr device, context;
		static int contextRefs = 0;
		
		static void CreateContext() {
			device = AL.alcOpenDevice(IntPtr.Zero);
			if (device == IntPtr.Zero) {
				throw new AudioException("Unable to open default OpenAL device");
			}
			CheckContextErrors();

			context = AL.alcCreateContext(device, null);
			if (context == IntPtr.Zero) {
				AL.alcCloseDevice(device);
				throw new AudioException("Audio context could not be created");
			}
			CheckContextErrors();

			AL.alcMakeContextCurrent(context);
			CheckContextErrors();
		}

		static void CheckContextErrors() {
			AlcError err = AL.alcGetError(device);
			if (err == AlcError.NoError) return;
			throw new AudioException("Error " + err + " when creating OpenAL context");
		}
		
		static void DestroyContext() {
			if (device == IntPtr.Zero) return;
			AL.alcMakeContextCurrent(IntPtr.Zero);

			if (context != IntPtr.Zero)
				AL.alcDestroyContext(context);
			if (device != IntPtr.Zero)
				AL.alcCloseDevice(device);
			
			context = IntPtr.Zero;
			device  = IntPtr.Zero;
		}
	}
}
