// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;
using ClassicalSharp.Commands;

namespace ClassicalSharp {

	public sealed class Chat : IGameComponent {
		
		public ChatLine Status1, Status2, Status3, BottomRight1,
		BottomRight2, BottomRight3, Announcement, ClientClock;
		public ChatLine[] ClientStatus = new ChatLine[6];
		
		Game game;
		public void Init( Game game ) {
			this.game = game;
		}

		public void Ready( Game game ) { }			
		public void Reset( Game game ) { }
		public void OnNewMap( Game game ) { }
		public void OnNewMapLoaded( Game game ) { }
		
		/// <summary> List of chat messages received from the server and added by client commands. </summary>
		/// <remarks> index 0 is the oldest chat message, last index is newest. </remarks>
		public List<ChatLine> Log = new List<ChatLine>();
		
		/// <summary> List of chat messages sent by the user to the server. </summary>
		public List<string> InputLog = new List<string>();
		
		public void Send( string text, bool partial ) {
			text = text.TrimEnd( trimChars );
			if( String.IsNullOrEmpty( text ) ) return;
			
			if( CommandManager.IsCommandPrefix( text ) ) {
				game.CommandManager.Execute( text );
				return;
			}
			game.Network.SendChat( text, partial );
		}
		
		static char[] trimChars = new [] { ' ', '\0' };
		StringBuffer logBuffer = new StringBuffer( 128 );
		public void Add( string text ) {
			Log.Add( text );
			LogChatToFile( text );
			game.Events.RaiseChatReceived( text, MessageType.Normal );
		}
		
		public void Add( string text, MessageType type ) {
			if( type == MessageType.Normal ) {
				Log.Add( text );
				LogChatToFile( text );
			} else if( type == MessageType.Status1 ) {
				Status1 = text;
			} else if( type == MessageType.Status2 ) {
				Status2 = text;
			} else if( type == MessageType.Status3 ) {
				Status3 = text;
			} else if( type == MessageType.BottomRight1 ) {
				BottomRight1 = text;
			} else if( type == MessageType.BottomRight2 ) {
				BottomRight2 = text;
			} else if( type == MessageType.BottomRight3 ) {
				BottomRight3 = text;
			} else if( type == MessageType.Announcement ) {
				Announcement = text;
			} else if( type >= MessageType.ClientStatus1 && type <= MessageType.ClientStatus6 ) {
				ClientStatus[(int)(type - MessageType.ClientStatus1)] = text;
			} else if( type == MessageType.ClientClock ) {
				ClientClock = text;
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
			if( writer == null ) return;
			
			if( 32 + text.Length > logBuffer.capacity )
				logBuffer = new StringBuffer( 32 + text.Length );
			int index = 0;
			logBuffer.Clear() // [yyyy-MM-dd HH:mm:ss] text
				.Append( ref index, '[' ).AppendPaddedNum( ref index, 4, now.Year )
				.Append( ref index, '-' ).AppendPaddedNum( ref index, 2, now.Month )
				.Append( ref index, '-' ).AppendPaddedNum( ref index, 2, now.Day )
				.Append( ref index, ' ' ).AppendPaddedNum( ref index, 2, now.Hour )
				.Append( ref index, ':' ).AppendPaddedNum( ref index, 2, now.Minute )
				.Append( ref index, ':' ).AppendPaddedNum( ref index, 2, now.Second )
				.Append( ref index, "] " ).AppendColourless( ref index, text )
				.Append( ref index, Environment.NewLine );
			writer.Write( logBuffer.value, 0, logBuffer.Length );
		}
		
		void OpenChatFile( DateTime now ) {
			string basePath = Path.Combine( Program.AppDirectory, "logs" );
			if( !Directory.Exists( basePath ) )
				Directory.CreateDirectory( basePath );

			string date = now.ToString( "yyyy-MM-dd" );
			// Ensure multiple instances do not end up overwriting each other's log entries.
			for( int i = 0; i < 20; i++ ) {
				string id = i == 0 ? "" : "  _" + i;
				string fileName = "chat-" + date + id + ".log";
				string path = Path.Combine( basePath, fileName );
				
				FileStream stream = null;
				try {
					stream = File.Open( path, FileMode.Append, FileAccess.Write, FileShare.Read );
				} catch( IOException ex ) {
					int hresult = Marshal.GetHRForException(ex);
					uint errorCode = (uint)hresult & 0xFFFF;
					if( errorCode != 32 ) // ERROR_SHARING_VIOLATION
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