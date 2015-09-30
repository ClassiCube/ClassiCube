﻿using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Net;
using System.Threading;

namespace ClassicalSharp.Network {
	
	/// <summary> Specialised producer and consumer queue for downloading images and pages asynchronously. </summary>
	public class AsyncDownloader : IDisposable {
		
		EventWaitHandle handle = new AutoResetEvent( false );
		Thread worker;
		readonly object requestLocker = new object();
		List<DownloadRequest> requests = new List<DownloadRequest>();
		readonly object downloadedLocker = new object();
		Dictionary<string, DownloadedItem> downloaded = new Dictionary<string, DownloadedItem>();
		string skinServer = null;
		
		public AsyncDownloader( string skinServer ) {
			this.skinServer = skinServer;
			WebRequest.DefaultWebProxy = null;
			client = new WebClient();
			
			worker = new Thread( DownloadThreadWorker, 256 * 1024 );
			worker.Name = "ClassicalSharp.ImageDownloader";
			worker.IsBackground = true;
			worker.Start();
		}
		
		public void DownloadSkin( string skinName ) {
			string strippedSkinName = Utils.StripColours( skinName );
			string url = Utils.IsUrl( skinName ) ? skinName :
				skinServer + strippedSkinName + ".png";
			AddRequest( url, true, "skin_" + strippedSkinName, 0 );
		}
		
		public void DownloadImage( string url, bool priority, string identifier ) {
			AddRequest( url, priority, identifier, 0 );
		}
		
		public void DownloadPage( string url, bool priority, string identifier ) {
			AddRequest( url, priority, identifier, 1 );
		}
		
		public void DownloadData( string url, bool priority, string identifier ) {
			AddRequest( url, priority, identifier, 2 );
		}
		
		void AddRequest( string url, bool priority, string identifier, byte type ) {
			lock( requestLocker )  {
				DownloadRequest request = new DownloadRequest( url, identifier, type );
				if( priority ) {
					requests.Insert( 0, request );
				} else {
					requests.Add( request );
				}
			}
			handle.Set();
		}
		
		public void Dispose() {
			lock( requestLocker )  {
				requests.Insert( 0, null );
			}
			handle.Set();
			
			worker.Join();
			handle.Close();
			client.Dispose();
		}
		
		public void PurgeOldEntries( int seconds ) {
			lock( downloadedLocker ) {
				DateTime now = DateTime.UtcNow;
				List<string> itemsToRemove = new List<string>( downloaded.Count );
				
				foreach( var item in downloaded ) {
					DateTime timestamp = item.Value.TimeDownloaded;
					if( (now - timestamp).TotalSeconds > seconds ) {
						itemsToRemove.Add( item.Key );
					}
				}
				
				for( int i = 0; i < itemsToRemove.Count; i++ ) {
					string key = itemsToRemove[i];
					DownloadedItem item;
					downloaded.TryGetValue( key, out item );
					downloaded.Remove( key );
					Bitmap bmp = item.Data as Bitmap;
					if( bmp != null )
						bmp.Dispose();
				}
			}
		}
		
		public bool TryGetItem( string identifier, out DownloadedItem item ) {
			bool success = false;
			lock( downloadedLocker ) {
				success = downloaded.TryGetValue( identifier, out item );
				if( success ) {
					downloaded.Remove( identifier );
				}
			}
			return success;
		}

		WebClient client;
		void DownloadThreadWorker() {
			while( true ) {
				DownloadRequest request = null;
				lock( requestLocker ) {
					if( requests.Count > 0 ) {
						request = requests[0];
						requests.RemoveAt( 0 );
						if( request == null )
							return;
					}
				}
				if( request != null ) {
					DownloadItem( request );
				} else {
					handle.WaitOne();
				}
			}
		}
		
		void DownloadItem( DownloadRequest request ) {
			string url = request.Url;
			byte type = request.Type;
			string dataType = type == 0 ? "image" : (type == 1 ? "string" : "raw");
			Utils.LogDebug( "Downloading " + dataType + " from: " + url );
			object value = null;
			
			try {
				if( type == 0 ) {
					byte[] data = client.DownloadData( url );
					using( MemoryStream stream = new MemoryStream( data ) )
						value = new Bitmap( stream );
				} else if( type == 1 ) {
					value = client.DownloadString( url );
				} else if( type == 2 ) {
					value = client.DownloadData( url );
				}
				Utils.LogDebug( "Downloaded from: " + url );
			} catch( Exception ex ) {
				if( !( ex is WebException || ex is ArgumentException ) ) throw;
				Utils.LogDebug( "Failed to download from: " + url );
			}

			lock( downloadedLocker ) {
				DownloadedItem oldItem;
				DownloadedItem newItem = new DownloadedItem( value, request.TimeAdded, url );
				
				if( downloaded.TryGetValue( request.Identifier, out oldItem ) ) {
					if( oldItem.TimeAdded > newItem.TimeAdded ) {
						DownloadedItem old = oldItem;
						oldItem = newItem;
						newItem = old;
					}

					Bitmap oldBmp = oldItem.Data as Bitmap;
					if( oldBmp != null )
						oldBmp.Dispose();
				}
				downloaded[request.Identifier] = newItem;
			}
		}
		
		class DownloadRequest {
			
			public string Url;
			public string Identifier;
			public byte Type; // 0 = bitmap, 1 = string, 2 = byte[]
			public DateTime TimeAdded;
			
			public DownloadRequest( string url, string identifier, byte type ) {
				Url = url;
				Identifier = identifier;
				Type = type;
				TimeAdded = DateTime.UtcNow;
			}
		}
	}
	
	public class DownloadedItem {
		
		public object Data;
		public DateTime TimeAdded, TimeDownloaded;
		public string Url;
		
		public DownloadedItem( object data, DateTime timeAdded, string url ) {
			Data = data;
			TimeAdded = timeAdded;
			TimeDownloaded = DateTime.UtcNow;
			Url = url;
		}
	}
}