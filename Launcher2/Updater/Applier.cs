// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using ClassicalSharp;
using ClassicalSharp.Textures;

namespace Launcher.Updater {
	
	public static class Applier {
		
		public static DateTime PatchTime;

		public static void ApplyUpdate() {
			ProcessStartInfo info = new ProcessStartInfo();
			info.CreateNoWindow = false;
			info.UseShellExecute = true;
			info.WorkingDirectory = Platform.AppDirectory;
			
			if (OpenTK.Configuration.RunningOnWindows) {
				Platform.WriteAllText("update.bat", Scripts.BatchFile);
				info.FileName = "cmd"; info.Arguments = "/C start cmd /C update.bat";
				Process.Start(info);
 			} else {
				string path = Path.Combine(Platform.AppDirectory, "update.sh");
				Platform.WriteAllText("update.sh", Scripts.BashFile.Replace("\r\n", "\n"));
				
				const int flags = 0x7;// read | write | executable
				int code = chmod(path, (flags << 6) | (flags << 3) | 4);
				if (code != 0)
					throw new InvalidOperationException("chmod returned : " + code);
				
				//if (OpenTK.Configuration.RunningOnMacOS)
				//	info = new ProcessStartInfo("open -a Terminal ",
				//	                            '"' + path + '"');
				//else
				info.FileName = "xterm"; info.Arguments = '"' + path + '"';
				Process.Start(info);
			}
		}

		[DllImport("libc", SetLastError = true)]
		internal static extern int chmod(string path, int mode);
		
		public static void ExtractUpdate(byte[] zipData) {
			Platform.DirectoryCreate("CS_Update");
			using (MemoryStream stream = new MemoryStream(zipData)) {
				ZipReader reader = new ZipReader();				
				reader.ProcessZipEntry = ProcessZipEntry;
				reader.Extract(stream);
			}
		}
		
		static void ProcessZipEntry(string filename, byte[] data, ZipEntry entry) {
			string path = Path.Combine("CS_Update", Path.GetFileName(filename));
			Platform.WriteAllBytes(path, data);
			
			try {
				Platform.FileSetWriteTime(path, PatchTime);
			} catch (IOException ex) {
				ErrorHandler.LogError("I/O exception when trying to set modified time for: " + filename, ex);
			} catch (UnauthorizedAccessException ex) {
				ErrorHandler.LogError("Permissions exception when trying to set modified time for: " + filename, ex);
			}
		}
	}
}
