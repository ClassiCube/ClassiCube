using System;
using System.Collections.Generic;
using System.IO;
using ClassicalSharp.Commands;

namespace ClassicalSharp {

	public sealed class ChatLog : IDisposable {
		
		public string Status1, Status2, Status3, BottomRight1, 
		BottomRight2, BottomRight3, Announcement;
		
		Game game;
		public ChatLog( Game game ) {
			this.game = game;
		}
		
		/// <summary> List of chat messages received from the server and added by client commands. </summary>
		/// <remarks> index 0 is the oldest chat message, last index is newest. </remarks>
		public List<string> Log = new List<string>();
		
		/// <summary> List of chat messages sent by the user to the server. </summary>
		public List<string> InputLog = new List<string>();
		
		public int FontSize = 12;
		
		public void Send( string text ) {
			if( String.IsNullOrEmpty( text ) ) return;
			
			InputLog.Add( text );
			if( CommandManager.IsCommandPrefix( text ) ) {
				game.CommandManager.Execute( text );
				return;
			}
			game.Network.SendChat( text );
		}
		
		public void Add( string text ) {
			Log.Add( text );
			LogChatToFile( text );
			game.Events.RaiseChatReceived( text, CpeMessage.Normal );
		}
		
		public void Add( string text, CpeMessage type ) {
			if( type == CpeMessage.Normal ) {
				Log.Add( text );
				LogChatToFile( text );
			} else if( type == CpeMessage.Status1 ) {
				Status1 = text;
			} else if( type == CpeMessage.Status2 ) {
				Status2 = text;
			} else if( type == CpeMessage.Status3 ) {
				Status3 = text;
			} else if( type == CpeMessage.BottomRight1 ) {
				BottomRight1 = text;
			} else if( type == CpeMessage.BottomRight2 ) {
				BottomRight2 = text;
			} else if( type == CpeMessage.BottomRight3 ) {
				BottomRight3 = text;
			} else if( type == CpeMessage.Announcement ) {
				Announcement = text;
			}
			game.Events.RaiseChatReceived( text, type );
		}
		
		public void Dispose() {
			if( writer != null ) {
				writer.Dispose();
				writer = null;
			}
		}
		
		DateTime last = new DateTime( 1, 1, 1 );
		StreamWriter writer = null;
		void LogChatToFile( string text ) {
			DateTime now = DateTime.Now;			
			if( now.Day != last.Day || now.Month != last.Month || now.Year != last.Year ) {
				Dispose();
				OpenChatFile( now );				
			}
			
			last = now;
			if( writer != null ) {
				string data = Utils.StripColours( text );
				string entry = now.ToString( "[yyyy-MM-dd HH:mm:ss] " ) + data;
				writer.WriteLine( entry );
			}
		}
		
		void OpenChatFile( DateTime now ) {
			if( !Directory.Exists( "logs" ) )
				Directory.CreateDirectory( "logs" );

			string date = now.ToString( "yyyy-MM-dd" );
			// Cheap way of ensuring multiple instances do not end up overwriting each other's log entries.
			for( int i = 0; i < 20; i++ ) {
				string id = i == 0 ? "" : "  _" + i;
				string fileName = "chat-" + date + id + ".log";
				string path = Path.Combine( "logs", fileName );
				FileStream stream = null;
				try {
					stream = File.Open( path, FileMode.Append, FileAccess.Write, FileShare.Read );
				} catch( IOException ex ) {
					if( !ex.Message.Contains( "because it is being used by another process" ) )
						throw;
					continue;
				}
				Utils.LogDebug( "opening chat with id:" + id );
				writer = new StreamWriter( stream );
				writer.AutoFlush = true;
				return;
			}
			Utils.LogError( "Failed to open or create a chat log file after 20 tries, giving up." );
		}
	}
}