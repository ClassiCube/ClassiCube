// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Diagnostics;
using System.IO;
using System.Net;
using System.Runtime.InteropServices;
using System.Threading;
using ClassicalSharp;
using ClassicalSharp.Textures;
using Launcher.Web;

namespace Launcher.Updater {
	
	public static class Applier {
		
		public static DateTime PatchTime;
		
		public static void FetchUpdate(string dir) {
			WebRequest.DefaultWebProxy = null;
			using (WebClient client = new WebClient()) {
				byte[] zipData = client.DownloadData(UpdateCheckTask.UpdatesUri + dir);
				MakeUpdatesFolder(zipData);
			}
		}

		public static void ApplyUpdate() {
			ProcessStartInfo info = new ProcessStartInfo();
			info.CreateNoWindow = false;
			info.UseShellExecute = true;
			info.WorkingDirectory = Program.AppDirectory;
			
			if (OpenTK.Configuration.RunningOnWindows) {
				string path = Path.Combine(Program.AppDirectory, "update.bat");
				File.WriteAllText(path, Scripts.BatchFile);
				info.FileName = "cmd"; info.Arguments = "/C start cmd /C update.bat";
				Process.Start(info);
 			} else {
				string path = Path.Combine(Program.AppDirectory, "update.sh");
				File.WriteAllText(path, Scripts.BashFile.Replace("\r\n", "\n"));
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
		
		static void MakeUpdatesFolder(byte[] zipData) {
			using (MemoryStream stream = new MemoryStream(zipData)) {
				ZipReader reader = new ZipReader();
				string path = Path.Combine(Program.AppDirectory, "CS_Update");
				Directory.CreateDirectory(path);
				
				reader.ShouldProcessZipEntry = (f) => true;
				reader.ProcessZipEntry = ProcessZipEntry;
				reader.Extract(stream);
			}
		}
		
		static void ProcessZipEntry(string filename, byte[] data, ZipEntry entry) {
			string path = Path.Combine(Program.AppDirectory, "CS_Update");
			path = Path.Combine(path, Path.GetFileName(filename));
			File.WriteAllBytes(path, data);
			
			try {
				File.SetLastWriteTimeUtc(path, PatchTime);
			} catch (IOException ex) {
				ErrorHandler.LogError("I/O exception when trying to set modified time for: " + filename, ex);
			} catch (UnauthorizedAccessException ex) {
				ErrorHandler.LogError("Permissions exception when trying to set modified time for: " + filename, ex);
			}
		}
	}
}
