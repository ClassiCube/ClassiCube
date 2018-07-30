// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Net;
using ClassicalSharp.Gui.Screens;
using ClassicalSharp.Network;
using ClassicalSharp.Textures;
using OpenTK;
using OpenTK.Input;

namespace ClassicalSharp {
	
	/// <summary> Represents a connection to either a multiplayer server, or an internal single player server. </summary>
	public abstract class IServerConnection : IGameComponent {
		
		public bool IsSinglePlayer;
		public bool Disconnected;
		public abstract void BeginConnect();
		public abstract void Tick(ScheduledTask task);
		
		public abstract void SendChat(string text);
		public abstract void SendPosition(Vector3 pos, float rotY, float headX);
		public abstract void SendPlayerClick(MouseButton button, bool buttonDown, byte targetId, PickedPos pos);

		public virtual void Init(Game game) { }
		public virtual void Ready(Game game) { }
		public virtual void OnNewMapLoaded(Game game) { }
		
		public abstract void Reset(Game game);
		public abstract void OnNewMap(Game game);
		public abstract void Dispose();
		
		public string ServerName;
		public string ServerMotd;
		public string AppName = Program.AppName;
		
		public bool UsingExtPlayerList;
		public bool UsingPlayerClick;
		public bool SupportsPartialMessages;
		public bool SupportsFullCP437;

		protected Game game;
		protected int netTicks;
		
		internal void RetrieveTexturePack(string url) {
			if (TextureCache.HasDenied(url)) {
				// nothing to do here
			} else if (!TextureCache.HasAccepted(url)) {
				Overlay warning = new TexPackOverlay(game, url);
				game.Gui.ShowOverlay(warning, false);
			} else {
				DownloadTexturePack(url);
			}
		}

		internal void DownloadTexturePack(string url) {
			if (TextureCache.HasDenied(url)) return;
			string etag = null;
			DateTime lastModified = DateTime.MinValue;
			
			if (TextureCache.HasUrl(url)) {
				lastModified = TextureCache.GetLastModified(url);
				etag = TextureCache.GetETag(url);
			}

			TexturePack.ExtractCurrent(game, url);
			game.Downloader.AsyncGetData(url, true, "texturePack", lastModified, etag);
		}
		
		protected void CheckAsyncResources() {
			Request item;
			if (!game.Downloader.TryGetItem("texturePack", out item)) return;
			
			if (item.Data != null) {
				TexturePack.Extract(game, item);
			} else {
				LogResourceFail(item);
			}
		}
		
		void LogResourceFail(Request item) {
			WebException ex = item.WebEx;
			if (ex == null) return;
			
			if (ex.Response != null) {
				int status = (int)((HttpWebResponse)ex.Response).StatusCode;
				if (status == 304) return; // Not an error if no data when "Not modified" status
				game.Chat.Add("&c" + status + " error when trying to download texture pack");
			} else {
				game.Chat.Add("&c" + ex.Status + " when trying to download texture pack");
			}
		}
	}
}