using System;
using System.Collections.Generic;
using System.IO;
using ClassicalSharp.Commands;
using OpenTK;

namespace ClassicalSharp {

	public partial class Game : GameWindow {
		
		public string Status1, Status2, Status3,
		BottomRight1, BottomRight2, BottomRight3,
		Announcement;
		
		/// <summary> List of chat messages received from the server
		/// (and messages added by the program, except for user input). </summary>
		/// <remarks> The string at index 0 is the oldest chat message,
		/// the string at index count - 1 is the newest chat message. </remarks>
		public List<string> ChatLog = new List<string>();
		
		public List<string> ChatInputLog = new List<string>();
		
		public int ChatFontSize = 12;
		
		public void SendChat( string text ) {
			if( String.IsNullOrEmpty( text ) ) return;
			
			ChatInputLog.Add( text );
			if( CommandManager.IsCommandPrefix( text ) ) {
				CommandManager.Execute( text );
				return;
			}
			Network.SendChat( text );
		}
		
		public void AddChat( string text ) {
			ChatLog.Add( text );
			LogChatToFile( text );
			RaiseEvent( ChatReceived, new TextEventArgs( text ) );
		}
		
		const string fileNameFormat = "yyyy-MM-dd";
		const string chatEntryFormat = "[yyyy-MM-dd HH:mm:ss]";
		const string dateFormat = "dd-MM-yyyy-HH-mm-ss";
		DateTime last = new DateTime( 1, 1, 1 );
		StreamWriter writer = null;
		void LogChatToFile( string text ) {
			DateTime now = DateTime.Now;
			if( !Directory.Exists( "logs" ) ) {
				Directory.CreateDirectory( "logs" );
			}
			
			if( now.Day != last.Day || now.Month != last.Month || now.Year != last.Year ) {				
				if( writer != null ) {
					writer.Close();
					writer = null;
				}
				
				// Cheap way of ensuring multiple instances do not end up overwriting each other's log entries.
				int counter = 0;
				while( true ) {
					string id = counter == 0 ? "" : "  _" + counter;
					string fileName = "chat-" + now.ToString( fileNameFormat ) + id + ".log";
					string path = Path.Combine( "logs", fileName );
					FileStream stream = null;
					try {
						stream = File.Open( path, FileMode.Append, FileAccess.Write, FileShare.Read );
					} catch( IOException ex ) {
						if( !ex.Message.Contains( "because it is being used by another process" ) ) {
							throw;
						}
						if( stream != null ) {
							stream.Close();
						}
						counter++;
						if( counter >= 20 ) {
							Utils.LogError( "Failed to open or create a chat log file after 20 tries, giving up." );
							break;
						}
						continue;
					}
					Utils.LogDebug( "opening chat with id:" + id );
					writer = new StreamWriter( stream );
					writer.AutoFlush = true;					
					break;
				}
				last = now;
			}
			
			if( writer != null ) {
				string data = Utils.StripColours( text );
				string entry = now.ToString( chatEntryFormat ) + " " + data;
				writer.WriteLine( entry );
			}
		}
	}
}