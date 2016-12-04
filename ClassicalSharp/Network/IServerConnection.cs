// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using System.IO;
using System.Net;
using ClassicalSharp.Gui.Screens;
using ClassicalSharp.Network;
using ClassicalSharp.Textures;
using OpenTK;
using OpenTK.Input;
#if ANDROID
using Android.Graphics;
#endif

namespace ClassicalSharp {
	
	/// <summary> Represents a connection to either a multiplayer server, or an internal single player server. </summary>
	public abstract class IServerConnection {
		
		public abstract bool IsSinglePlayer { get; }
		
		/// <summary> Opens a connection to the given IP address and port, and prepares the initial state of the client. </summary>
		public abstract void Connect(IPAddress address, int port);
		
		public abstract void SendChat(string text, bool partial);
		
		/// <summary> Informs the server of the client's current position and orientation. </summary>
		public abstract void SendPosition(Vector3 pos, float yaw, float pitch);
		
		/// <summary> Informs the server that using the given mouse button,
		/// the client clicked on a particular block or entity. </summary>
		public abstract void SendPlayerClick(MouseButton button, bool buttonDown, byte targetId, PickedPos pos);
		
		public abstract void Tick(ScheduledTask task);
		
		public abstract void Dispose();
		
		public string ServerName;
		public string ServerMotd;
		public string AppName = Program.AppName;
		
		/// <summary> Whether the network processor is currently connected to a server. </summary>
		public bool Disconnected;
		
		/// <summary> Whether the client should use extended player list management, with group names and ranks. </summary>
		public bool UsingExtPlayerList;
		
		/// <summary> Whether the server supports handling PlayerClick packets from the client. </summary>
		public bool UsingPlayerClick;
		
		/// <summary> Whether the server can handle partial message packets or not. </summary>
		public bool SupportsPartialMessages;
		
		/// <summary> Whether the server supports receiving all code page 437 characters from this client. </summary>
		public bool SupportsFullCP437;
		
		
		#region Texture pack / terrain.png

		protected Game game;
		
		protected void WarningScreenTick(WarningScreen screen) {
			string identifier = (string)screen.Metadata;
			DownloadedItem item;
			if (!game.AsyncDownloader.TryGetItem(identifier, out item) || item.Data == null) return;
			
			long contentLength = (long)item.Data;
			if (contentLength <= 0) return;
			string url = identifier.Substring(3);
			
			float contentLengthMB = (contentLength / 1024f / 1024f);
			string address = url;
			if (url.StartsWith("https://")) address = url.Substring(8);
			if (url.StartsWith("http://")) address = url.Substring(7);
			screen.SetText("Do you want to download the server's texture pack?",
			               "Texture pack url:", address,
			               "Download size: " + contentLengthMB.ToString("F3") + " MB");
		}
		
		protected internal void RetrieveTexturePack(string url) {
			if (!game.AcceptedUrls.HasEntry(url) && !game.DeniedUrls.HasEntry(url)) {
				game.AsyncDownloader.RetrieveContentLength(url, true, "CL_" + url);
				string address = url;
				if (url.StartsWith("https://")) address = url.Substring(8);
				if (url.StartsWith("http://")) address = url.Substring(7);
				
				WarningScreen warning = new WarningScreen(game, true, true);
				warning.Metadata = "CL_" + url;
				warning.SetHandlers(DownloadTexturePack, SkipTexturePack, WarningScreenTick);
				
				warning.SetTextData(
					"Do you want to download the server's texture pack?",
					"Texture pack url:", address,
					"Download size: Determining...");
				game.Gui.ShowWarning(warning);
			} else {
				DownloadTexturePack(url);
			}
		}
		
		void DownloadTexturePack(WarningScreen screen, bool always) {
			string url = ((string)screen.Metadata).Substring(3);
			DownloadTexturePack(url);
			if (always && !game.AcceptedUrls.HasEntry(url)) {
				game.AcceptedUrls.AddEntry(url);
			}
		}

		void SkipTexturePack(WarningScreen screen, bool always) {
			string url = ((string)screen.Metadata).Substring(3);
			if (always && !game.DeniedUrls.HasEntry(url)) {
				game.DeniedUrls.AddEntry(url);
			}
		}
		
		void DownloadTexturePack(string url) {
			if (game.DeniedUrls.HasEntry(url)) return;
			DateTime lastModified = TextureCache.GetLastModified(url, game.LastModified);
			string etag = TextureCache.GetETag(url, game.ETags);

			if (url.Contains(".zip"))
				game.AsyncDownloader.DownloadData(url, true, "texturePack",
				                                  lastModified, etag);
			else
				game.AsyncDownloader.DownloadImage(url, true, "terrain",
				                                   lastModified, etag);
		}
		
		protected void CheckAsyncResources() {
			DownloadedItem item;
			if (game.AsyncDownloader.TryGetItem("terrain", out item)) {
				TexturePack.ExtractTerrainPng(game, item.Url, item);
			}
			if (game.AsyncDownloader.TryGetItem("texturePack", out item)) {
				TexturePack.ExtractTexturePack(game, item.Url, item);
			}
		}
		#endregion
	}
}