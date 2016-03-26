// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using System.Net;
using ClassicalSharp.Network;
using ClassicalSharp.TexturePack;
using OpenTK;
using OpenTK.Input;

namespace ClassicalSharp {
	
	/// <summary> Represents a connection to either a multiplayer server, or an internal single player server. </summary>
	public abstract class INetworkProcessor {
		
		public abstract bool IsSinglePlayer { get; }
		
		/// <summary> Opens a connection to the given IP address and port, and prepares the initial state of the client. </summary>
		public abstract void Connect( IPAddress address, int port );
		
		public abstract void SendChat( string text, bool partial );
		
		/// <summary> Informs the server of the client's current position and orientation. </summary>
		public abstract void SendPosition( Vector3 pos, float yaw, float pitch );
		
		/// <summary> Informs the server that the client placed or deleted a block at the given coordinates. </summary>
		public abstract void SendSetBlock( int x, int y, int z, bool place, byte block );
		
		/// <summary> Informs the server that using the given mouse button,
		/// the client clicked on a particular block or entity. </summary>
		public abstract void SendPlayerClick( MouseButton button, bool buttonDown, byte targetId, PickedPos pos );
		
		public abstract void Tick( double delta );
		
		public abstract void Dispose();
		
		public string ServerName;
		public string ServerMotd;
		
		/// <summary> Whether the network processor is currently connected to a server. </summary>
		public bool Disconnected;
		
		/// <summary> Whether the client should use extended player list management, with group names and ranks. </summary>
		public bool UsingExtPlayerList;
		
		/// <summary> Whether the server supports handling PlayerClick packets from the client. </summary>
		public bool UsingPlayerClick;
		
		/// <summary> Whether the server can handle partial message packets or not. </summary>
		public bool ServerSupportsPartialMessages;
		
		/// <summary> Whether the server supports receiving all code page 437 characters from this client. </summary>
		public bool ServerSupportsFullCP437;
		
		
		#region Texture pack / terrain.png 

		protected Game game;
		
		protected void WarningScreenTick( WarningScreen screen ) {
			string identifier = (string)screen.Metadata;
			DownloadedItem item;
			if( !game.AsyncDownloader.TryGetItem( identifier, out item ) || item.Data == null ) return;
			
			long contentLength = (long)item.Data;
			if( contentLength <= 0 ) return;
			string url = identifier.Substring( 3 );
			
			float contentLengthMB = (contentLength / 1024f / 1024f );
			screen.SetText( "Do you want to download the server's texture pack?",
			               "Texture pack url:", url,
			               "Download size: " + contentLengthMB.ToString( "F3" ) + " MB" );
		}
		
		protected internal void RetrieveTexturePack( string url ) {
			if( !game.AcceptedUrls.HasUrl( url ) && !game.DeniedUrls.HasUrl( url ) ) {
				game.AsyncDownloader.RetrieveContentLength( url, true, "CL_" + url );
				game.ShowWarning( new WarningScreen(
					game, "CL_" + url, true, "Do you want to download the server's texture pack?",
					DownloadTexturePack, null, WarningScreenTick,
					"Texture pack url:", url,
					"Download size: Determining..." ) );
			} else {
				DownloadTexturePack( url );
			}
		}
		
		void DownloadTexturePack( WarningScreen screen ) {
			DownloadTexturePack( ((string)screen.Metadata).Substring( 3 ) );
		}
		
		void DownloadTexturePack( string url ) {
			if( game.DeniedUrls.HasUrl( url ) ) return;
			game.Animations.Dispose();
			DateTime lastModified = TextureCache.GetLastModifiedFromCache( url );

			if( url.Contains( ".zip" ) )
				game.AsyncDownloader.DownloadData( url, true, "texturePack", lastModified );
			else
				game.AsyncDownloader.DownloadImage( url, true, "terrain", lastModified );
		}
		
		protected void ExtractDefault() {
			TexturePackExtractor extractor = new TexturePackExtractor();
			extractor.Extract( game.DefaultTexturePack, game );
		}
		
		static bool Is304Status( WebException ex ) {
			if( ex == null || ex.Status != WebExceptionStatus.ProtocolError )
				return false;
			HttpWebResponse response = (HttpWebResponse)ex.Response;
			return response.StatusCode == HttpStatusCode.NotModified;
		}
		
		protected void CheckAsyncResources() {
			DownloadedItem item;
			if( game.AsyncDownloader.TryGetItem( "terrain", out item ) ) {
				if( item.Data != null ) {
					Bitmap bmp = (Bitmap)item.Data;
					if( !FastBitmap.CheckFormat( bmp.PixelFormat ) ) {
						Utils.LogDebug( "Converting terrain atlas to 32bpp image" );
						game.Drawer2D.ConvertTo32Bpp( ref bmp );
					}
					game.ChangeTerrainAtlas( bmp );
					TextureCache.AddToCache( item.Url, bmp );
					game.Map.TextureUrl = item.Url;
				} else if( Is304Status( item.WebEx ) ) {
					Bitmap bmp = TextureCache.GetBitmapFromCache( item.Url );
					if( bmp == null ) // Should never happen, but handle anyways.
						ExtractDefault();
					else
						game.ChangeTerrainAtlas( bmp );
					game.Map.TextureUrl = item.Url;
				} else {
					ExtractDefault();
					game.Map.TextureUrl = null;
				}
			}
			
			if( game.AsyncDownloader.TryGetItem( "texturePack", out item ) ) {
				if( item.Data != null ) {
					TexturePackExtractor extractor = new TexturePackExtractor();
					extractor.Extract( (byte[])item.Data, game );
					TextureCache.AddToCache( item.Url, (byte[])item.Data );
					game.Map.TextureUrl = item.Url;
				} else if( Is304Status( item.WebEx ) ) {
					byte[] data = TextureCache.GetDataFromCache( item.Url );
					if( data == null ) { // Should never happen, but handle anyways.
						ExtractDefault();
					} else {
						TexturePackExtractor extractor = new TexturePackExtractor();
						extractor.Extract( data, game );
					}
					game.Map.TextureUrl = item.Url;
				} else {
					ExtractDefault();
					game.Map.TextureUrl = null;
				}
			}
		}
		#endregion
	}
}