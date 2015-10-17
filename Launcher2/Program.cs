using System;
using System.Diagnostics;
using System.IO;
using System.Windows.Forms;

namespace Launcher2 {

	internal sealed class Program {
		
		public const string AppName = "ClassicalSharp Launcher 0.95";
		
		[STAThread]
		private static void Main( string[] args ) {
			if( !Debugger.IsAttached )
				AppDomain.CurrentDomain.UnhandledException += UnhandledException;
			
			LauncherWindow window = new LauncherWindow();
			window.Run();
		}
		
		static void UnhandledException( object sender, UnhandledExceptionEventArgs e ) {
			// So we don't get the normal unhelpful crash dialog on Windows.
			Exception ex = (Exception)e.ExceptionObject;
			bool wroteToCrashLog = LogException( ex );
			string error = wroteToCrashLog ? null :
				(ex.GetType().FullName + ": " + ex.Message + Environment.NewLine + ex.StackTrace);
			
			string message = wroteToCrashLog ?
				"The cause of the crash has been logged to \"crash-launcher.log\". Please consider reporting the crash " +
				"(and the circumstances that caused it) to github.com/UnknownShadow200/ClassicalSharp/issues" :
				
				"Failed to write the cause of the crash to \"crash-launcher.log\". Please consider reporting the crash " +
				"(and the circumstances that caused it) to github.com/UnknownShadow200/ClassicalSharp/issues" +
				Environment.NewLine + Environment.NewLine + error;
			
			MessageBox.Show( "Oh dear. ClassicalSharp has crashed." + Environment.NewLine + Environment.NewLine + message, "ClassicalSharp has crashed" );
			Environment.Exit( 1 );
		}
		
		internal static bool LogException( Exception ex ) {
			string error = ex.GetType().FullName + ": " + ex.Message + Environment.NewLine + ex.StackTrace;
			try {
				using( StreamWriter writer = new StreamWriter( "crash_launcher.log", true ) ) {
					writer.WriteLine( "Crash time: " + DateTime.Now.ToString() );
					writer.WriteLine( error );
					writer.WriteLine();
				}
			} catch( Exception ) {
				return false;
			}
			return true;
		}
	}
}
