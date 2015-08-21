using System;
using System.IO;
using System.Windows.Forms;

namespace Launcher {

	internal sealed class Program {
		
		[STAThread]
		private static void Main( string[] args ) {
			Application.EnableVisualStyles();
			Application.SetCompatibleTextRenderingDefault( false );
			Application.Run( new MainForm() );
		}
		
		internal static void LogException( Exception ex ) {
			string error = ex.GetType().FullName + ": " + ex.Message + Environment.NewLine + ex.StackTrace;
			try {
				using( StreamWriter writer = new StreamWriter( "crash_launcher.log", true ) ) {
					writer.WriteLine( "Crash time: " + DateTime.Now.ToString() );
					writer.WriteLine( error );
					writer.WriteLine();
				}
			} catch( Exception ) {
			}
		}
	}
}