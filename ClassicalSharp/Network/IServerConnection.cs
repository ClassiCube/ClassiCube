// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using System.IO;
using System.Net;
using ClassicalSharp.Entities;
using ClassicalSharp.Generator;
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
		
		public bool IsSinglePlayer;
		
		/// <summary> Opens a connection to the given IP address and port, and prepares the initial state of the client. </summary>
		public abstract void Connect(IPAddress address, int port);
		
		public abstract void SendChat(string text);
		
		/// <summary> Informs the server of the client's current position and orientation. </summary>
		public abstract void SendPosition(Vector3 pos, float rotY, float headX);
		
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
		protected int netTicks;
		
		protected void WarningScreenTick(Overlay warning) {
			string identifier = warning.Metadata;
			Request item;
			if (!game.AsyncDownloader.TryGetItem(identifier, out item) || item.Data == null) return;
			
			long contentLength = (long)item.Data;
			if (contentLength <= 0) return;
			string url = identifier.Substring(3);
			
			float contentLengthMB = (contentLength / 1024f / 1024f);
			warning.lines[3] = "Download size: " + contentLengthMB.ToString("F3") + " MB";
			warning.RedrawText();
		}
		
		protected internal void RetrieveTexturePack(string url) {
			if (!game.AcceptedUrls.HasEntry(url) && !game.DeniedUrls.HasEntry(url)) {
				game.AsyncDownloader.RetrieveContentLength(url, true, "CL_" + url);
				string address = url;
				if (url.StartsWith("https://")) address = url.Substring(8);
				if (url.StartsWith("http://")) address = url.Substring(7);
				
				WarningOverlay warning = new WarningOverlay(game, true, true);
				warning.Metadata = "CL_" + url;
				warning.SetHandlers(DownloadTexturePack, SkipTexturePack);
				warning.OnRenderFrame = WarningScreenTick;
				warning.lines[0] = "Do you want to download the server's texture pack?";
				
				warning.lines[1] = "Texture pack url:";
				warning.lines[2] = address;
				warning.lines[3] = "Download size: Determining...";
				game.Gui.ShowOverlay(warning);
			} else {
				DownloadTexturePack(url);
			}
		}
		
		void DownloadTexturePack(Overlay texPackOverlay, bool always) {
			string url = texPackOverlay.Metadata.Substring(3);
			DownloadTexturePack(url);
			if (always && !game.AcceptedUrls.HasEntry(url)) {
				game.AcceptedUrls.AddEntry(url);
			}
		}

		void SkipTexturePack(Overlay texPackOverlay, bool always) {
			string url = texPackOverlay.Metadata.Substring(3);
			if (always && !game.DeniedUrls.HasEntry(url)) {
				game.DeniedUrls.AddEntry(url);
			}
		}
		
		void DownloadTexturePack(string url) {
			if (game.DeniedUrls.HasEntry(url)) return;
			DateTime lastModified = TextureCache.GetLastModified(url, game.LastModified);
			string etag = TextureCache.GetETag(url, game.ETags);

			TexturePack.ExtractCurrent(game, url);
			if (url.Contains(".zip")) {
				game.AsyncDownloader.DownloadData(url, true, "texturePack",
				                                  lastModified, etag);
			} else {
				game.AsyncDownloader.DownloadImage(url, true, "terrain",
				                                   lastModified, etag);
			}
		}
		
		protected void CheckAsyncResources() {
			Request item;
			if (game.AsyncDownloader.TryGetItem("terrain", out item)) {
				TexturePack.ExtractTerrainPng(game, item);
			}
			if (game.AsyncDownloader.TryGetItem("texturePack", out item)) {
				TexturePack.ExtractTexturePack(game, item);
			}
		}
		#endregion
		
		IMapGenerator gen;
		internal void BeginGeneration(int width, int height, int length, int seed, IMapGenerator gen) {
			game.World.Reset();
			game.WorldEvents.RaiseOnNewMap();
			
			GC.Collect();
			this.gen = gen;
			game.Gui.SetNewScreen(new GeneratingMapScreen(game, gen));
			gen.Width = width; gen.Height = height; gen.Length = length; gen.Seed = seed;
			gen.GenerateAsync(game);
		}
		
		internal void EndGeneration() {
			game.Gui.SetNewScreen(null);
			if (gen.Blocks == null) {
				game.Chat.Add("&cFailed to generate the map.");
			} else {
				game.World.SetNewMap(gen.Blocks, gen.Width, gen.Height, gen.Length);
				gen.Blocks = null;
				ResetPlayerPosition();
				
				game.WorldEvents.RaiseOnNewMapLoaded();
				gen.ApplyEnv(game.World);
			}
			
			gen = null;
			GC.Collect();
		}

		void ResetPlayerPosition() {
			float x = (game.World.Width  / 2) + 0.5f;
			float z = (game.World.Length / 2) + 0.5f;
			Vector3 spawn = Respawn.FindSpawnPosition(game, x, z, game.LocalPlayer.Size);
			
			LocationUpdate update = LocationUpdate.MakePosAndOri(spawn, 0, 0, false);
			game.LocalPlayer.SetLocation(update, false);
			game.LocalPlayer.Spawn = spawn;
			game.CurrentCameraPos = game.Camera.GetCameraPos(0);
		}
	}
}