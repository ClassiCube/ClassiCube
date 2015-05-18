using System;
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
			client = new WebClient();
			client.Proxy = null;
			
			worker = new Thread( DownloadThreadWorker );
			worker.Name = "ClassicalSharp.ImageDownloader";
			worker.IsBackground = true;
			worker.Start();
		}
		
		public void DownloadSkin( string skinName ) {
			string strippedSkinName = Utils.StripColours( skinName );
			string url = Utils.IsUrl( skinName ) ? skinName :
				skinServer + strippedSkinName + ".png";
			AddRequestToQueue( url, true, "skin_" + strippedSkinName, true );
		}
		
		public void DownloadImage( string url, bool priority, string identifier ) {
			AddRequestToQueue( url, priority, identifier, true );
		}
		
		public void DownloadPage( string url, bool priority, string identifier ) {
			AddRequestToQueue( url, priority, identifier, false );
		}
		
		void AddRequestToQueue( string url, bool priority, string identifier, bool image ) {
			lock( requestLocker )  {
				DownloadRequest request = new DownloadRequest( url, identifier, image );
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
					if( ( now - timestamp ).TotalSeconds > seconds ) {
						itemsToRemove.Add( item.Key );
					}
				}
				
				for( int i = 0; i < itemsToRemove.Count; i++ ) {
					string key = itemsToRemove[i];
					DownloadedItem item;
					downloaded.TryGetValue( key, out item );
					downloaded.Remove( key );
					if( item.Bmp != null ) {
						item.Bmp.Dispose();
					}
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
			bool isImage = request.IsImage;
			Utils.LogDebug( "Downloading " + ( isImage ? "image" : "page" ) + " from: " + url );
			Bitmap bmp = null;
			string page = null;
			
			try {
				if( isImage ) {
					byte[] data = client.DownloadData( url );
					using( MemoryStream stream = new MemoryStream( data ) ) {
						bmp = new Bitmap( stream );
					}
				} else {
					page = client.DownloadString( url );
				}
				Utils.LogDebug( "Downloaded from: " + request.Url );
			} catch( Exception ex ) {
				if( !( ex is WebException || ex is ArgumentException ) ) throw;
				Utils.LogDebug( "Failed to download from: " + request.Url );
			}

			lock( downloadedLocker ) {
				DownloadedItem oldItem;
				DownloadedItem newItem = new DownloadedItem( bmp, page, request.TimeAdded, request.Url );
				
				if( downloaded.TryGetValue( request.Identifier, out oldItem ) ) {
					if( oldItem.TimeAdded > newItem.TimeAdded ) {
						DownloadedItem old = oldItem;
						oldItem = newItem;
						newItem = old;
					}

					if( oldItem.Bmp != null )
						oldItem.Bmp.Dispose();
				}
				downloaded[request.Identifier] = newItem;
			}
		}
		
		class DownloadRequest {
			
			public string Url;
			public string Identifier;
			public bool IsImage;
			public DateTime TimeAdded;
			
			public DownloadRequest( string url, string identifier, bool isImage ) {
				Url = url;
				Identifier = identifier;
				IsImage = isImage;
				TimeAdded = DateTime.UtcNow;
			}
		}
	}
	
	public class DownloadedItem {
		
		public Bitmap Bmp;
		public string Page;
		public DateTime TimeAdded, TimeDownloaded;
		public string Url;
		
		public DownloadedItem( Bitmap bmp, string page, DateTime timeAdded, string url ) {
			Bmp = bmp;
			Page = page;
			TimeAdded = timeAdded;
			TimeDownloaded = DateTime.UtcNow;
			Url = url;
		}
	}
}