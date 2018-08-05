using System;
using System.Runtime.InteropServices;
using System.Threading;
using SharpWave.Codecs;

namespace SharpWave {
	
	/// <summary> Outputs audio to the default sound playback device using the
	/// native WinMm library. Windows only. </summary>
	public unsafe sealed class WinMmOut : IAudioOutput {	
		IntPtr devHandle, headers;
		IntPtr[] dataHandles;
		int[] dataSizes;
		int volumePercent = 100, waveHeaderSize;

		public override void SetVolume(float volume) { volumePercent = (int)(volume * 100); }

		public override void Create(int numBuffers) {
			waveHeaderSize = Marshal.SizeOf(default(WaveHeader));
			headers = Marshal.AllocHGlobal(waveHeaderSize * numBuffers);
			dataHandles = new IntPtr[numBuffers];
			dataSizes = new int[numBuffers];
			
			for (int i = 0; i < numBuffers; i++) {
				WaveHeader* hdr = (WaveHeader*)headers + i;
				hdr->Flags = WaveHeaderFlags.Done;
			}
			NumBuffers = numBuffers;
		}

		public override bool IsCompleted(int index) {
			WaveHeader* hdr = (WaveHeader*)headers + index;
			if ((hdr->Flags & WaveHeaderFlags.Done) == 0) return false;

			if ((hdr->Flags & WaveHeaderFlags.Prepared) != 0) {
				uint result = WinMM.waveOutUnprepareHeader(devHandle, (IntPtr)hdr, waveHeaderSize);
				CheckError(result, "UnprepareHeader");
			}
			return true;
		}

		const ushort pcmFormat = 1;
		public override bool IsFinished() {
			for (int i = 0; i < NumBuffers; i++) {
				if (!IsCompleted(i)) return false;
			}
			return true;
		}
		
		public override void SetFormat(AudioFormat format) {
			// Don't need to recreate device if it's the same
			if (Format.Equals(format)) return;
			Format = format;
			
			Console.WriteLine("init");
			DisposeDevice();
			WaveFormatEx fmt = default(WaveFormatEx);
			
			fmt.Channels = (ushort)format.Channels;
			fmt.FormatTag = pcmFormat;
			fmt.BitsPerSample = (ushort)format.BitsPerSample;
			fmt.BlockAlign = (ushort)(fmt.Channels * fmt.BitsPerSample / 8);
			fmt.SampleRate = (uint)format.SampleRate;
			fmt.AverageBytesPerSecond = (int)fmt.SampleRate * fmt.BlockAlign;
			
			uint devices = WinMM.waveOutGetNumDevs();
			if (devices == 0) throw new InvalidOperationException("No audio devices found");

			uint result = WinMM.waveOutOpen(out devHandle, (IntPtr)(-1), ref fmt,
			                                      IntPtr.Zero, UIntPtr.Zero, 0);
			CheckError(result, "Open");
		}
		
		public override void PlayData(int index, AudioChunk chunk) {			
			if (chunk.Length > dataSizes[index]) {
				IntPtr ptr = dataHandles[index];
				if (ptr != IntPtr.Zero) Marshal.FreeHGlobal(ptr);
				dataHandles[index] = Marshal.AllocHGlobal(chunk.Length);
			}
			
			IntPtr handle = dataHandles[index];
			fixed (byte* data = chunk.Data) {
				MemUtils.memcpy((IntPtr)data, handle, chunk.Length);
				ApplyVolume(handle, chunk);
			}
			
			WaveHeader header = default(WaveHeader);
			header.DataBuffer = handle;
			header.BufferLength = chunk.Length;
			header.Loops = 1;
			
			WaveHeader* hdr = (WaveHeader*)headers + index;
			*hdr = header;
			
			uint result = WinMM.waveOutPrepareHeader(devHandle, (IntPtr)hdr, waveHeaderSize);
			CheckError(result, "PrepareHeader");
			result = WinMM.waveOutWrite(devHandle, (IntPtr)hdr, waveHeaderSize);
			CheckError(result, "Write");
		}
		
		void ApplyVolume(IntPtr handle, AudioChunk chunk) {
			if (volumePercent == 100) return;
			
			if (Format.BitsPerSample == 16) {
				VolumeMixer.Mix16((short*)handle, chunk.Length / sizeof(short), volumePercent);
			} else if (Format.BitsPerSample == 8) {
				VolumeMixer.Mix8((byte*)handle, chunk.Length, volumePercent);
			}
		}

		void CheckError(uint result, string func) {
			if (result == 0) return;
			
			string description = WinMM.GetErrorDescription(result);
			const string format = "{0} at {1} ({2})";
			string text = String.Format(format, result, func, description);
			throw new InvalidOperationException(text);
		}
		
		public override void Dispose() {
			Console.WriteLine("dispose");
			DisposeDevice();
			for (int i = 0; i < dataHandles.Length; i++)
				Marshal.FreeHGlobal(dataHandles[i]);
		}
		
		void DisposeDevice() {
			if (devHandle == IntPtr.Zero) return;
			
			Console.WriteLine("disposing device");
			uint result = WinMM.waveOutClose(devHandle);
			CheckError(result, "Close");
			devHandle = IntPtr.Zero;
		}
	}
}
