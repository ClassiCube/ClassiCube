using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Net;
using System.Text;
using System.Threading;

namespace ClassicalSharp.Network {
	
	/// <summary> Specialised producer and consumer queue for downloading data asynchronously. </summary>
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
			string url = Utils.IsUrlPrefix( skinName ) ? skinName :
				skinServer + strippedSkinName + ".png";
			AddRequest( url, true, "skin_" + strippedSkinName, 0, DateTime.MinValue );
		}
		
		/// <summary> Asynchronously downloads a bitmap image from the specified url.  </summary>
		public void DownloadImage( string url, bool priority, string identifier ) {
			AddRequest( url, priority, identifier, 0, DateTime.MinValue );
		}
		
		/// <summary> Asynchronously downloads a string from the specified url.  </summary>
		public void DownloadPage( string url, bool priority, string identifier ) {
			AddRequest( url, priority, identifier, 1, DateTime.MinValue );
		}
		
		/// <summary> Asynchronously downloads a byte array from the specified url.  </summary>
		public void DownloadData( string url, bool priority, string identifier ) {
			AddRequest( url, priority, identifier, 2, DateTime.MinValue );
		}
		
		/// <summary> Asynchronously downloads a bitmap image from the specified url.  </summary>
		public void DownloadImage( string url, bool priority, string identifier, DateTime lastModified ) {
			AddRequest( url, priority, identifier, 0, lastModified );
		}
		
		/// <summary> Asynchronously downloads a byte array from the specified url.  </summary>
		public void DownloadData( string url, bool priority, string identifier, DateTime lastModified ) {
			AddRequest( url, priority, identifier, 2, lastModified );
		}
		
		void AddRequest( string url, bool priority, string identifier, byte type, DateTime lastModified ) {
			lock( requestLocker )  {
				Request request = new Request( url, identifier, type, lastModified );
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
			WebException webEx = null;		
			
			try {
				HttpWebRequest req = MakeRequest( request );
				HttpWebResponse response = (HttpWebResponse)req.GetResponse();
				MemoryStream data = DownloadBytes( response );
				
				if( type == 0 ) {
					value = new Bitmap( data );
				} else if( type == 1 ) {
					value = Encoding.UTF8.GetString( data.ToArray() );
				} else if( type == 2 ) {
					value = data.ToArray();
				}
			} catch( Exception ex ) {
				if( !( ex is WebException || ex is ArgumentException || ex is UriFormatException ) ) throw;
				Utils.LogDebug( "Failed to download from: " + url );
				
				if( ex is WebException )
					webEx = (WebException)ex;
			}
			
			// Mono seems to be returning a bitmap with a native pointer of zero in some weird cases.
			// We can detect this as every single property access raises an ArgumentException.
			try {
				Bitmap bmp = value as Bitmap;
				if( bmp != null ) {
					int height = bmp.Height;
				}
			} catch (ArgumentException) {
				Utils.LogDebug( "Failed to download from: " + url );
				value = null;
			}

			lock( downloadedLocker ) {
				DownloadedItem oldItem;
				DownloadedItem newItem = new DownloadedItem( value, request.TimeAdded, url, webEx );
				
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
		
		HttpWebRequest MakeRequest( Request request ) {
			HttpWebRequest req = (HttpWebRequest)WebRequest.Create( request.Url );
			req.AutomaticDecompression = DecompressionMethods.GZip;
			req.ReadWriteTimeout = 15 * 1000;
			req.Timeout = 15 * 1000;
			req.Proxy = null;
			req.UserAgent = Program.AppName;
			
			if( request.LastModified != DateTime.MinValue )
				req.IfModifiedSince = request.LastModified;
			return req;
		}
		
		static byte[] buffer = new byte[4096 * 8];
		MemoryStream DownloadBytes( HttpWebResponse response ) {
			int length = (int)response.ContentLength;
			MemoryStream dst = length >= 0 ?
				new MemoryStream( length ) : new MemoryStream();
			
			using( Stream src = response.GetResponseStream() ) {
				int read = 0;
				while( (read = src.Read( buffer, 0, buffer.Length )) > 0 ) {
					dst.Write( buffer, 0, read );
				}
			}
			return dst;
		}
		
		sealed class Request {
			
			public string Url;
			public string Identifier;
			public byte Type; // 0 = bitmap, 1 = string, 2 = byte[]
			public DateTime TimeAdded;
			public DateTime LastModified;
			
			public Request( string url, string identifier, byte type, DateTime lastModified ) {
				Url = url;
				Identifier = identifier;
				Type = type;
				TimeAdded = DateTime.UtcNow;
				LastModified = lastModified;
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
		
		/// <summary> Full URL this item was downloaded from. </summary>
		public string Url;
		
		/// <summary> Exception that occurred if this request failed, can be null. </summary>
		public WebException WebEx;
		
		public DownloadedItem( object data, DateTime timeAdded, string url, WebException webEx ) {
			Data = data;
			TimeAdded = timeAdded;
			TimeDownloaded = DateTime.UtcNow;
			Url = url;
			WebEx = webEx;
		}
	}
}