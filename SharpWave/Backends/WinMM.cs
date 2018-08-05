using System;
using System.Runtime.InteropServices;
using System.Security;
using System.Text;

namespace SharpWave {
	[SuppressUnmanagedCodeSecurity]
	internal static class WinMM {
		const string lib = "winmm.dll";

		[DllImport(lib, SetLastError = true)]
		internal static extern uint waveOutOpen(out IntPtr handle, IntPtr deviceID, ref WaveFormatEx format,
		                                        IntPtr callback, UIntPtr callbackInstance, uint flags);
		[DllImport(lib, SetLastError = true)]
		internal static extern uint waveOutClose(IntPtr handle);
		[DllImport(lib, SetLastError = true)]
		internal static extern uint waveOutGetNumDevs();
		
		[DllImport(lib, SetLastError = true)]
		internal static extern uint waveOutPrepareHeader(IntPtr handle, IntPtr header, int hdrSize);
		[DllImport(lib, SetLastError = true)]
		internal static extern uint waveOutUnprepareHeader(IntPtr handle, IntPtr header, int hdrSize);		
		[DllImport(lib, SetLastError = true)]
		internal static extern uint waveOutWrite(IntPtr handle, IntPtr header, int hdrSize);

		[DllImport(lib, SetLastError = true, CharSet = CharSet.Auto)]
		internal static extern uint waveOutGetErrorText(uint error, StringBuilder buffer, int bufferLen);
		internal static string GetErrorDescription(uint error) {
			StringBuilder message = new StringBuilder(1024);
			uint result = waveOutGetErrorText(error, message, message.Capacity);
			if(result == 0)
				return message.ToString();
			return "waveOutGetErrorText failed.";
		}
	}
	
	[StructLayout(LayoutKind.Sequential, Pack = 2)]
	public struct WaveFormatEx {
		public ushort FormatTag;
		public ushort Channels;
		public uint SampleRate;
		public int AverageBytesPerSecond;
		public ushort BlockAlign;
		public ushort BitsPerSample;
		public ushort ExtraSize;
	}
	
	[StructLayout(LayoutKind.Sequential, Pack = 2)]
	public struct WaveHeader {
		public IntPtr DataBuffer;
		public int BufferLength;
		public int BytesRecorded;
		public IntPtr UserData;
		public WaveHeaderFlags Flags;
		public int Loops;
		public IntPtr Next;
		public IntPtr Reserved;
	}
	
	[Flags]
	public enum WaveHeaderFlags : uint {
		Done = 0x01,
		Prepared = 0x02,
		BeginLoop = 0x04,
		EndLoop = 0x08,
		InQueue = 0x10,
	}
}
