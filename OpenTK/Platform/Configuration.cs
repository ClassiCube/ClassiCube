#region License
//
// The Open Toolkit Library License
//
// Copyright (c) 2006 - 2009 the Open Toolkit library.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
//
#endregion

using System;
using System.Reflection;
using System.Runtime.InteropServices;

namespace OpenTK {
	
	/// <summary>Provides information about the underlying OS and runtime.</summary>
	public static class Configuration {
		
		public static bool RunningOnWindows, RunningOnUnix, RunningOnX11,
		RunningOnMacOS, RunningOnLinux, RunningOnMono;

		// Detects the underlying OS and runtime.
		unsafe static Configuration() {
			PlatformID platform = Environment.OSVersion.Platform;
			if( platform == PlatformID.Win32NT || platform == PlatformID.Win32S ||
			   platform == PlatformID.Win32Windows || platform == PlatformID.WinCE )
				RunningOnWindows = true;
			else if ( platform == PlatformID.Unix || platform == (PlatformID)4 ) {
				sbyte* ascii = stackalloc sbyte[8192];
				uname( ascii );
				// Distinguish between Linux, Mac OS X and other Unix operating systems.
				string kernel = new String( ascii );
				if( kernel == "Linux" ) {
					RunningOnLinux = RunningOnUnix = true;
				} else if( kernel == "Darwin" ) {
					RunningOnMacOS = RunningOnUnix = true;
				} else if( !String.IsNullOrEmpty( kernel ) ) {
					RunningOnUnix = true;
				} else {
					throw new PlatformNotSupportedException("Unknown platform. Please file a bug report at http://www.opentk.com/");
				}
			}
			else
				throw new PlatformNotSupportedException("Unknown platform. Please report this error at http://www.opentk.com.");

			// Detect whether X is present.
			// Hack: it seems that this check will cause X to initialize itself on Mac OS X Leopard and newer.
			// We don't want that (we'll be using the native interfaces anyway), so we'll avoid this check when we detect Mac OS X.
			if( !RunningOnMacOS && !RunningOnWindows ) {
				try { RunningOnX11 = OpenTK.Platform.X11.API.DefaultDisplay != IntPtr.Zero; }
				catch { }
			}

			// Detect the Mono runtime (code taken from http://mono.wikia.com/wiki/Detecting_if_program_is_running_in_Mono).
			if( Type.GetType("Mono.Runtime") != null )
				RunningOnMono = true;
			
			Debug.Print("Detected configuration: {0} / {1}",
			            RunningOnWindows ? "Windows" : RunningOnLinux ? "Linux" : RunningOnMacOS ? "MacOS" :
			            RunningOnUnix ? "Unix" : RunningOnX11 ? "X11" : "Unknown Platform",
			            RunningOnMono ? "Mono" : ".Net");
		}

		[DllImport("libc")]
		unsafe static extern void uname(sbyte* uname_struct);
		
		public static void SkipPerfCountersHack() {
			if (RunningOnMono || !RunningOnWindows) return;
			
			// On XP (maybe other versions) with guest account, we get this error
			/*System.UnauthorizedAccessException: Access to the path 'Global\.net clr networking' is denied.
				at System.IO.__Error.WinIOError(Int32 errorCode, String maybeFullPath)
				at System.Threading.Mutex.<>c__DisplayClass3.<.ctor>b__0(Object userData)
				at System.Runtime.CompilerServices.RuntimeHelpers.ExecuteCodeWithGuaranteedCleanup(TryCode code, CleanupCode backoutCode, Object userData)
				at System.Threading.Mutex..ctor(Boolean initiallyOwned, String name, Boolean& createdNew, MutexSecurity mutexSecurity)
				at System.Diagnostics.SharedUtils.EnterMutexWithoutGlobal(String mutexName, Mutex& mutex)
				at System.Diagnostics.SharedPerformanceCounter.Verify(CategoryEntry* currentCategoryPointer)
				at System.Diagnostics.SharedPerformanceCounter.FindCategory(CategoryEntry** returnCategoryPointerReference)
				at System.Diagnostics.SharedPerformanceCounter.GetCounter(String counterName, String instanceName, Boolean enableReuse, PerformanceCounterInstanceLifetime lifetime)
				at System.Diagnostics.SharedPerformanceCounter..ctor(String catName, String counterName, String instanceName, PerformanceCounterInstanceLifetime lifetime)
				at System.Diagnostics.PerformanceCounter.Initialize()
				at System.Diagnostics.PerformanceCounter.set_RawValue(Int64 value)
				at System.Net.NetworkingPerfCounters.Initialize()*/
			
			// So we hack around and prevent them ever being initalised.
			// TODO: only seems to work before .NET 4.0, not sure if still needed by then
			try {
				Assembly assem = typeof(System.Net.IPAddress).Assembly;
				Type perfType = assem.GetType("System.Net.NetworkingPerfCounters");
				if (perfType == null) return;
				
				FieldInfo field = perfType.GetField("initialized", BindingFlags.NonPublic | BindingFlags.Static);
				if (field == null) return;
				
				field.SetValue(null, true);
			} catch {				
			}
		}
	}
}
