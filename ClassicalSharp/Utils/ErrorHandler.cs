// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Diagnostics;
using System.IO;
using System.Windows.Forms;
using OpenTK;

namespace ClassicalSharp {
	
	/// <summary> Displays a message box when an unhandled exception occurs
	/// and also logs it to a specified log file. </summary>
	public static class ErrorHandler {

		static string fileName = "crash.log";
		
		/// <summary> Adds a handler for when a unhandled exception occurs, unless
		/// a debugger is attached to the process in which case this does nothing. </summary>
		public static void InstallHandler(string logFile) {
			fileName = logFile;
			if (!Debugger.IsAttached)
				AppDomain.CurrentDomain.UnhandledException += UnhandledException;
		}
		
		/// <summary> Additional text that should be logged to the log file
		/// when an unhandled exception occurs. </summary>
		public static string[] AdditionalInfo;
		
		static string Format(Exception ex) {
			try {
				return ex.GetType().FullName + ": " + ex.Message + Environment.NewLine + ex.StackTrace;
			} catch (Exception) {
				return "";
			}
		}
		
		static void UnhandledException(object sender, UnhandledExceptionEventArgs e) {
			// So we don't get the normal unhelpful crash dialog on Windows.
			Exception ex = (Exception)e.ExceptionObject;
			bool wroteToCrashLog = true;
			try {
				using (Stream fs = Platform.FileAppend(fileName))
					using (StreamWriter w = new StreamWriter(fs))
				{
					w.WriteLine("=== crash occurred ===");
					w.WriteLine("Time: " + DateTime.Now);
					
					string platform = Configuration.RunningOnMono ? "Mono " : ".NET ";
					platform += Environment.Version;
					
					if (Configuration.RunningOnWindows) platform += ", Windows - ";
					if (Configuration.RunningOnMacOS) platform += ", OSX - ";
					if (Configuration.RunningOnLinux) platform += ", Linux - ";
					platform += Environment.OSVersion.Version.ToString();
					w.WriteLine("Running on: " + platform);
					
					Exception ex2 = ex;
					while (ex2 != null) {
						w.WriteLine(Format(ex2));
						w.WriteLine();
						ex2 = ex2.InnerException;
					}
					
					if (AdditionalInfo != null) {
						for (int i = 0; i < AdditionalInfo.Length; i++)
							w.WriteLine(AdditionalInfo[i]);
						w.WriteLine();
					}
				}
			} catch (Exception) {
				wroteToCrashLog = false;
			}
			
			string line1 = "ClassicalSharp crashed.";
			if (wroteToCrashLog) {
				line1 += " The cause has also been logged to \"" + fileName + "\" in " + Platform.AppDirectory;
			}
			string line2 = "Please report the crash to github.com/UnknownShadow200/ClassicalSharp/issues so we can fix it.";
			line2 += Environment.NewLine + Environment.NewLine + Format(ex);

			ShowDialog("We're sorry", line1 + Environment.NewLine + Environment.NewLine + line2);
			Environment.Exit(1);
		}
		
		/// <summary> Logs a handled exception that occured at the specified location to the log file. </summary>
		public static bool LogError(string location, Exception ex) {
			string error = Format(ex);
			if (ex.InnerException != null) {
				error += Environment.NewLine + Format(ex.InnerException);
			}
			return LogError(location, error);
		}
		
		/// <summary> Logs an error that occured at the specified location to the log file. </summary>
		public static bool LogError(string location, string text) {
			try {
				using (Stream fs = Platform.FileAppend(fileName))
					using (StreamWriter w = new StreamWriter(fs))
				{
					w.WriteLine("=== handled error ===");
					w.WriteLine("Occured when: " + location);
					w.WriteLine(text);
					w.WriteLine();
				}
			} catch (Exception) {
				return false;
			}
			return true;
		}
		
		// put in separate function, because we don't want to load winforms assembly if possible
		public static void ShowDialog(string title, string msg) {
			MessageBox.Show(msg, title);
		}
	}
}