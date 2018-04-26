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
	public abstract class IServerConnection : IGameComponent {
		
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
		
		public virtual void Init(Game game) { }
		public virtual void Ready(Game game) { }		
		public virtual void OnNewMapLoaded(Game game) { }
		
		public abstract void Reset(Game game);
		public abstract void OnNewMap(Game game);
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
		
		protected internal void RetrieveTexturePack(string url) {
			if (!TextureCache.HasAccepted(url) && !TextureCache.HasDenied(url)) {
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
			if (url.Contains(".zip")) {
				game.Downloader.AsyncGetData(url, true, "texturePack", lastModified, etag);
			} else {
				game.Downloader.AsyncGetImage(url, true, "terrain", lastModified, etag);
			}
		}
		
		protected void CheckAsyncResources() {
			Request item;
			if (game.Downloader.TryGetItem("terrain", out item)) {
				TexturePack.ExtractTerrainPng(game, item);
			}
			if (game.Downloader.TryGetItem("texturePack", out item)) {
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