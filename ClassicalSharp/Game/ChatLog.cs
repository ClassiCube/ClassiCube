// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;
using ClassicalSharp.Commands;

namespace ClassicalSharp {

	public sealed class Chat : IGameComponent {
		
		public ChatLine Status1, Status2, Status3, BottomRight1,
		BottomRight2, BottomRight3, Announcement;
		public ChatLine[] ClientStatus = new ChatLine[3];
		
		Game game;
		void IGameComponent.Init(Game game) {
			this.game = game;
		}

		void IGameComponent.Ready(Game game) { }
		void IGameComponent.Reset(Game game) { logName = null; }
		void IGameComponent.OnNewMap(Game game) { }
		void IGameComponent.OnNewMapLoaded(Game game) { }
		
		/// <summary> List of chat messages received from the server and added by client commands. </summary>
		/// <remarks> index 0 is the oldest chat message, last index is newest. </remarks>
		public List<ChatLine> Log = new List<ChatLine>();
		
		/// <summary> List of chat messages sent by the user to the server. </summary>
		public List<string> InputLog = new List<string>();
		
		public void Send(string text) {
			if (String.IsNullOrEmpty(text)) return;
			
			InputLog.Add(text);
			if (game.CommandList.IsCommandPrefix(text)) {
				game.CommandList.Execute(text);
			} else {
				game.Server.SendChat(text);
			}
		}
		
		static char[] trimChars = new char[] { ' ', '\0' };
		StringBuffer logBuffer = new StringBuffer(128);
		public void Add(string text) { Add(text, MessageType.Normal); }
		
		public void Add(string text, MessageType type) {
			if (type == MessageType.Normal) {
				Log.Add(text);
				LogChatToFile(text);
			} else if (type == MessageType.Status1) {
				Status1 = text;
			} else if (type == MessageType.Status2) {
				Status2 = text;
			} else if (type == MessageType.Status3) {
				Status3 = text;
			} else if (type == MessageType.BottomRight1) {
				BottomRight1 = text;
			} else if (type == MessageType.BottomRight2) {
				BottomRight2 = text;
			} else if (type == MessageType.BottomRight3) {
				BottomRight3 = text;
			} else if (type == MessageType.Announcement) {
				Announcement = text;
			} else if (type >= MessageType.ClientStatus1 && type <= MessageType.ClientStatus3) {
				ClientStatus[(int)(type - MessageType.ClientStatus1)] = text;
			}
			game.Events.RaiseChatReceived(text, type);
		}
		
		public void Dispose() {
			if (writer == null) return;
			writer.Dispose();
			writer = null;
		}
		
		#region Chat logger
		string logName;
		
		public void SetLogName(string name) {
			if (logName != null) return;
			
			name = Utils.StripColours(name);
			StringBuffer buffer = new StringBuffer(name.Length);

			for (int i = 0; i < name.Length; i++) {
				if (Allowed(name[i])) buffer.Append(name[i]);
			}
			logName = buffer.ToString();
		}
		
		static bool Allowed(char c) {
			return
				c == '{' || c == '}' || c == '[' || c == ']' || c == '(' || c == ')' ||
				(c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
		}
		
		DateTime last;
		StreamWriter writer = null;
		void LogChatToFile(string text) {
			if (logName == null || !game.ChatLogging) return;
			DateTime now = DateTime.Now;
			
			if (now.Day != last.Day || now.Month != last.Month || now.Year != last.Year) {
				Dispose();
				OpenChatFile(now);
			}
			
			last = now;
			if (writer == null) return;
			
			if (32 + text.Length > logBuffer.Capacity)
				logBuffer = new StringBuffer(32 + text.Length);
			
			logBuffer.Clear() // [HH:mm:ss] text
				.Append('[').AppendPaddedNum(2, now.Hour)
				.Append(':').AppendPaddedNum(2, now.Minute)
				.Append(':').AppendPaddedNum(2, now.Second)
				.Append("] ").AppendColourless(text)
				.Append(Environment.NewLine);
			writer.Write(logBuffer.value, 0, logBuffer.Length);
		}
		
		void OpenChatFile(DateTime now) {
			if (!Platform.DirectoryExists("logs")) {
				Platform.DirectoryCreate("logs");
			}

			string date = now.ToString("yyyy-MM-dd");
			// Ensure multiple instances do not end up overwriting each other's log entries.
			for (int i = 0; i < 20; i++) {
				string id = i == 0 ? "" : " _" + i;
				string path = Path.Combine("logs", date + " " + logName + id + ".log");
				
				Stream stream = null;
				try {
					stream = Platform.FileAppend(path);
				} catch (IOException ex) {
					int hresult = Marshal.GetHRForException(ex);
					uint errorCode = (uint)hresult & 0xFFFF;
					if (errorCode != 32) // ERROR_SHARING_VIOLATION
						throw;
					continue;
				}
				
				writer = new StreamWriter(stream);
				writer.AutoFlush = true;
				return;
			}
			ErrorHandler.LogError("creating chat log",
			                      "Failed to open or create a chat log file after 20 tries, giving up.");
		}
		#endregion
	}
	
	public struct ChatLine {
		public string Text;
		public DateTime Received;
		
		public ChatLine(string text) {
			Text = text;
			Received = DateTime.UtcNow;
		}
		
		public static implicit operator ChatLine(string text) {
			return new ChatLine(text);
		}
	}
}