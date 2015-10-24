using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Net;
using System.Threading;

namespace ClassicalSharp.Network {
	
	/// <summary> Specialised producer and consumer queue for downloading images and pages asynchronously. </summary>
	public class AsyncDownloader : IDisposable {
		
		EventWaitHandle handle = new EventWaitHandle( false, EventResetMode.AutoReset );
		Thread worker;
		readonly object requestLocker = new object();
		List<Request> requests = new List<Request>();
		readonly object downloadedLocker = new object();
		Dictionary<string, DownloadedItem> downloaded = new Dictionary<string, DownloadedItem>();
		string skinServer = null;
		
		public AsyncDownloader( string skinServer ) {
			this.skinServer = skinServer;
			WebRequest.DefaultWebProxy = null;
			client = new GZipWebClient();
			
			worker = new Thread( DownloadThreadWorker, 256 * 1024 );
			worker.Name = "ClassicalSharp.ImageDownloader";
			worker.IsBackground = true;
			worker.Start();
		}
		
		/// <summary> Asynchronously downloads a skin. If 'skinName' points to the url then the skin is
		/// downloaded from that url, otherwise it is downloaded from the url 'defaultSkinServer'/'skinName'.png </summary>
		/// <remarks> Identifier is skin_'skinName'.</remarks>
		public void DownloadSkin( string skinName ) {
			string strippedSkinName = Utils.StripColours( skinName );
			string url = Utils.IsUrl( skinName ) ? skinName :
				skinServer + strippedSkinName + ".png";
			AddRequest( url, true, "skin_" + strippedSkinName, 0 );
		}
		
		/// <summary> Asynchronously downloads a bitmap image from the specified url.  </summary>
		public void DownloadImage( string url, bool priority, string identifier ) {
			AddRequest( url, priority, identifier, 0 );
		}
		
		/// <summary> Asynchronously downloads a string from the specified url.  </summary>
		public void DownloadPage( string url, bool priority, string identifier ) {
			AddRequest( url, priority, identifier, 1 );
		}
		
		/// <summary> Asynchronously downloads a byte array from the specified url.  </summary>
		public void DownloadData( string url, bool priority, string identifier ) {
			AddRequest( url, priority, identifier, 2 );
		}
		
		void AddRequest( string url, bool priority, string identifier, byte type ) {
			lock( requestLocker )  {
				Request request = new Request( url, identifier, type );
				if( priority ) {
					requests.Insert( 0, request );
				} else {
					requests.Add( request );
				}
			}
			handle.Set();
		}
		
		/// <summary> Informs the asynchronous thread that it should stop processing further requests
		/// and can consequentially exit the for loop.<br/>
		/// Note that this will *block** the calling thread as the method waits until the asynchronous
		/// thread has exited the for loop. </summary>
		public void Dispose() {
			lock( requestLocker )  {
				requests.Insert( 0, null );
			}
			handle.Set();
			
			worker.Join();
			((IDisposable)handle).Dispose();
			client.Dispose();
		}
		
		/// <summary> Removes older entries that were downloaded a certain time ago
		/// but were never removed from the downloaded queue. </summary>
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
		
		/// <summary> Returns whether the requested item exists in the downloaded queue.
		/// If it does, it removes the item from the queue and outputs it. </summary>
		/// <remarks> If the asynchronous thread failed to download the item, this method
		/// will return 'true' and 'item' will be set. However, the contents of the 'item' object will be null.</remarks>
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
				Request request = null;
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
		
		void DownloadItem( Request request ) {
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
		
		sealed class GZipWebClient : WebClient {
			
			protected override WebRequest GetWebRequest( Uri address ) {
				HttpWebRequest request = (HttpWebRequest)base.GetWebRequest( address );
				request.AutomaticDecompression = DecompressionMethods.GZip;
				return request;
			}
		}
		
		sealed class Request {
			
			public string Url;
			public string Identifier;
			public byte Type; // 0 = bitmap, 1 = string, 2 = byte[]
			public DateTime TimeAdded;
			
			public Request( string url, string identifier, byte type ) {
				Url = url;
				Identifier = identifier;
				Type = type;
				TimeAdded = DateTime.UtcNow;
			}
		}
	}
	
	/// <summary> Represents an item that was asynchronously downloaded. </summary>
	public class DownloadedItem {
		
		/// <summary> Contents that were downloaded. Can be a bitmap, string or byte array. </summary>
		public object Data;
		
		/// <summary> Instant in time the item was originally added to the download queue. </summary>
		public DateTime TimeAdded;
		
		/// <summary> Instant in time the item was fully downloaded. </summary>
		public DateTime TimeDownloaded;
		
		/// <summary>
		/// 
		/// </summary>
		public string Url;
		
		public DownloadedItem( object data, DateTime timeAdded, string url ) {
			Data = data;
			TimeAdded = timeAdded;
			TimeDownloaded = DateTime.UtcNow;
			Url = url;
		}
	}
}