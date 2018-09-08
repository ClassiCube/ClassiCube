/* AlFunctions.cs
 * C header: \OpenAL 1.1 SDK\include\Al.h
 * Spec: http://www.openal.org/openal_webstf/specs/OpenAL11Specification.pdf
 * Copyright (c) 2008 Christoph Brandtner and Stefanos Apostolopoulos
 * See license.txt for license details
 * http://www.OpenTK.net */

using System;
using System.Runtime.InteropServices;
using System.Security;

namespace OpenTK.Audio.OpenAL {
	
	[SuppressUnmanagedCodeSecurity]
	public unsafe static class AL {
		const string lib = "openal32.dll";
		const CallingConvention style = CallingConvention.Cdecl;
		
		[DllImport(lib, CallingConvention = style)]
		public static extern void alDistanceModel(ALDistanceModel param);
		[DllImport(lib, CallingConvention = style)]
		public static extern ALError alGetError();
		[DllImport(lib, CallingConvention = style)]
		public static extern AlcError alcGetError(IntPtr device);

		[DllImport(lib, CallingConvention = style)]
		public static extern void alGenSources(int n, uint* sids);
		[DllImport(lib, CallingConvention = style)]
		public static extern void alDeleteSources(int n, uint* sids);

		[DllImport(lib, CallingConvention = style)]
		public static extern void alSourcef(uint sid, ALSourcef param, float value);
		[DllImport(lib, CallingConvention = style)]
		public static extern void alGetSourcei(uint sid, ALGetSourcei param, int* value);
		[DllImport(lib, CallingConvention = style)]
		public static extern void alSourcePlay(uint sid);

		[DllImport(lib, CallingConvention = style)]
		public static extern void alSourceQueueBuffers(uint sid, int numEntries, uint* bids);
		[DllImport(lib, CallingConvention = style)]
		public static extern void alSourceUnqueueBuffers(uint sid, int numEntries, uint* bids);

		[DllImport(lib, CallingConvention = style)]
		public static extern void alGenBuffers(int n, uint* bids);
		[DllImport(lib, CallingConvention = style)]
		public static extern void alDeleteBuffers(int n, uint* bids);
		[DllImport(lib, CallingConvention = style)]
		public static extern void alBufferData(uint bid, ALFormat format, IntPtr buffer, int size, int freq);
		
		[DllImport(lib, CallingConvention = style)]
		public static extern IntPtr alcCreateContext(IntPtr device, int* attrlist);
		[DllImport(lib, CallingConvention = style)]
		public static extern bool alcMakeContextCurrent(IntPtr context);
		[DllImport(lib, CallingConvention = style)]
		public static extern void alcDestroyContext(IntPtr context);

		[DllImport(lib, CallingConvention = style)]
		public static extern IntPtr alcOpenDevice(IntPtr deviceName);
		[DllImport(lib, CallingConvention = style)]
		public static extern bool alcCloseDevice(IntPtr device);
	}
	
	public enum ALSourcef { Gain = 0x100A }
	public enum ALSourceState { Playing = 0x1012 }
	public enum ALDistanceModel { None = 0 }
	
	public enum ALGetSourcei {
		SourceState = 0x1010,
		BuffersProcessed = 0x1016,
	}

	public enum ALFormat {
		Mono8 = 0x1100,
		Mono16 = 0x1101,
		Stereo8 = 0x1102,
		Stereo16 = 0x1103,
	}

	public enum ALError {
		NoError = 0,
		InvalidName = 0xA001,
		InvalidEnum = 0xA002,
		InvalidValue = 0xA003,
		InvalidOperation = 0xA004,
		OutOfMemory = 0xA005,
	}

	public enum AlcError {
		NoError = 0,
		InvalidDevice = 0xA001,
		InvalidContext = 0xA002,
		InvalidEnum = 0xA003,
		InvalidValue = 0xA004,
		OutOfMemory = 0xA005,
	}
	
	public class AudioException : Exception {
		public AudioException() : base() { }
		public AudioException(string message) : base(message) { }
	}
}
