using System;
using System.Collections.Generic;
using System.IO;
using ClassicalSharp.Commands;

namespace ClassicalSharp {

	public sealed class ChatLog : IDisposable {
		
		public ChatLine Status1, Status2, Status3, BottomRight1,
		BottomRight2, BottomRight3, Announcement;
		
		Game game;
		public ChatLog( Game game ) {
			this.game = game;
		}
		
		/// <summary> List of chat messages received from the server and added by client commands. </summary>
		/// <remarks> index 0 is the oldest chat message, last index is newest. </remarks>
		public List<ChatLine> Log = new List<ChatLine>();
		
		/// <summary> List of chat messages sent by the user to the server. </summary>
		public List<string> InputLog = new List<string>();
		
		public int FontSize = 12;
		
		public void Send( string text, bool partial ) {
			text = text.TrimEnd( trimChars );
			if( String.IsNullOrEmpty( text ) ) return;
			InputLog.Add( text );
			
			if( CommandManager.IsCommandPrefix( text ) ) {
				game.CommandManager.Execute( text );
				return;
			}
			game.Network.SendChat( text, partial );
		}
		
		static char[] trimChars = new [] { ' ', '\0' };
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
		
		DateTime last;
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
			// Ensure multiple instances do not end up overwriting each other's log entries.
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
				
				writer = new StreamWriter( stream );
				writer.AutoFlush = true;
				return;
			}
			ErrorHandler.LogError( "creating chat log",
			               "Failed to open or create a chat log file after 20 tries, giving up." );
		}
	}
	
	public struct ChatLine {
		public string Text;
		public DateTime Received;
		
		public ChatLine( string text ) {
			Text = text;
			Received = DateTime.UtcNow;
		}
		
		public static implicit operator ChatLine( string text ) {
			return new ChatLine( text );
		}
	}
}