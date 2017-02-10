// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Diagnostics;
using System.IO;
using System.Windows.Forms;
using OpenTK;

// NOTE: Need to duplicate this code here, otherwise will crash with an unhelpful error messagfe
// if ClassicalSharp.exe is missing.
namespace Launcher {
	
	/// <summary> Displays a message box when an unhandled exception occurs
	/// and also logs it to a specified log file. </summary>
	public static class ErrorHandler2 {

		static string logFile = "crash.log";
		static string fileName = "crash.log";
		
		/// <summary> Adds a handler for when a unhandled exception occurs, unless 
		/// a debugger is attached to the process in which case this does nothing. </summary>
		public static void InstallHandler(string logFile) {
			ErrorHandler2.logFile = logFile;
			fileName = Path.GetFileName(logFile);
			if (!Debugger.IsAttached)
				AppDomain.CurrentDomain.UnhandledException += UnhandledException;
		}
		
		/// <summary> Additional text that should be logged to the log file 
		/// when an unhandled exception occurs. </summary>
		public static string[] AdditionalInfo;
		
		static string Format(Exception ex) {
			return ex.GetType().FullName + ": " + ex.Message 
				+ Environment.NewLine + ex.StackTrace;
		}

		static void UnhandledException(object sender, UnhandledExceptionEventArgs e) {
			// So we don't get the normal unhelpful crash dialog on Windows.
			Exception ex = (Exception)e.ExceptionObject;
			bool wroteToCrashLog = true;
			try {
				using (StreamWriter writer = new StreamWriter(logFile, true)) {
					writer.WriteLine("=== crash occurred ===");
					writer.WriteLine("Time: " + DateTime.Now);
					
					string platform = Configuration.RunningOnMono ? "Mono " : ".NET ";
					platform += Environment.Version;
					
					if (Configuration.RunningOnWindows) platform += ", Windows - ";
					if (Configuration.RunningOnMacOS) platform += ", OSX - ";
					if (Configuration.RunningOnLinux) platform += ", Linux - ";
					platform += Environment.OSVersion.Version.ToString();
					writer.WriteLine("Running on: " + platform);
					
					while (ex != null) {
						writer.WriteLine(Format(ex));
						writer.WriteLine();
						ex = ex.InnerException;
					}
					
					if (AdditionalInfo != null) {
						foreach (string l in AdditionalInfo)
							writer.WriteLine(l);
						writer.WriteLine();
					}
				}
			} catch (Exception) {
				wroteToCrashLog = false;
			}
			
			string line1 = "ClassicalSharp crashed.";
			if (wroteToCrashLog) {
				line1 += " The cause of the crash has been logged to \"" + fileName + "\" in the client directory.";
			}
			string line2 = "Please report the crash to github.com/UnknownShadow200/ClassicalSharp/issues so we can fix it.";
			if (!wroteToCrashLog) {
				line2 += Environment.NewLine + Environment.NewLine + Format(ex);
			}

			MessageBox.Show(line1 + Environment.NewLine + Environment.NewLine + line2, "We're sorry");
			Environment.Exit(1);
		}
		
		/// <summary> Logs a handled exception that occured at the specified location to the log file. </summary>
		public static bool LogError(string location, Exception ex) {
			string error = ex.GetType().FullName + ": " + ex.Message 
				+ Environment.NewLine + ex.StackTrace;
			return LogError(location, error);
		}
		
		/// <summary> Logs an error that occured at the specified location to the log file. </summary>
		public static bool LogError(string location, string text) {
			try {
				using (StreamWriter writer = new StreamWriter(logFile, true)) {
					writer.WriteLine("=== handled error ===");
					writer.WriteLine("Occured when: " + location);
					writer.WriteLine(text);
					writer.WriteLine();
				}
			} catch (Exception) {
				return false;
			}
			return true;
		}
	}
}