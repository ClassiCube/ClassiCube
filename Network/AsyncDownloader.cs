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
		Dictionary<string, DownloadedRequest> downloaded = new Dictionary<string, DownloadedRequest>();
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
		
		class DownloadedRequest {
			
			public Bitmap Bmp;
			public string Page;			
			public DateTime Timestamp;
			
			public DownloadedRequest( Bitmap bmp, string page ) {
				Bmp = bmp;
				Page = page;
				Timestamp = DateTime.UtcNow;
			}
		}
		
		class DownloadRequest {
			
			public string Url;			
			public string Identifier;			
			public bool IsImage;
			
			public DownloadRequest( string url, string identifier, bool isImage ) {
				Url = url;
				Identifier = identifier;
				IsImage = isImage;
			}
		}
		
		public void DownloadSkin( string skinName ) {
			string strippedSkinName = Utils.StripColours( skinName );
			string url = skinServer + strippedSkinName + ".png";
			DownloadImage( url, false, "skin_" + strippedSkinName );
		}
		
		/// <summary> Downloads an image, asynchronously, from the specified url. </summary>
		/// <param name="url"> URL the image is located at. </param>
		/// <param name="priority"> If this is true, the request is added straight to the
		/// start of the requests queue. (otherwise added to the end)</param>
		/// <param name="identifier"> Unique identifier for the image. (e.g. skin_test)</param>
		public void DownloadImage( string url, bool priority, string identifier ) {
			lock( requestLocker )  {
				DownloadRequest request = new DownloadRequest( url, identifier, true );
				if( priority ) {
					requests.Insert( 0, request );
				} else {
					requests.Add( request );
				}
			}
			handle.Set();
		}
		
		/// <summary> Downloads a page, asynchronously, from the specified url. </summary>
		/// <param name="url"> URL the image is located at. </param>
		/// <param name="priority"> If this is true, the request is added straight to the
		/// start of the requests queue. (otherwise added to the end)</param>
		/// <param name="identifier"> Unique identifier for the image. (e.g. skin_test)</param>
		public void DownloadPage( string url, bool priority, string identifier ) {
			lock( requestLocker )  {
				DownloadRequest request = new DownloadRequest( url, identifier, false );
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
		
		/// <summary> Removes all entries from the downloaded images queue that are
		/// older (relative to the current time) than the given number of seconds. </summary>
		public void PurgeOldEntries( int seconds ) {
			lock( downloadedLocker ) {
				DateTime now = DateTime.UtcNow;
				List<string> requestsToRemove = new List<string>( downloaded.Count );
				
				foreach( var item in downloaded ) {
					DateTime timestamp = item.Value.Timestamp;
					if( ( now - timestamp ).TotalSeconds > seconds ) {
						requestsToRemove.Add( item.Key );
					}
				}
				
				for( int i = 0; i < requestsToRemove.Count; i++ ) {
					string key = requestsToRemove[i];
					DownloadedRequest req;
					downloaded.TryGetValue( key, out req );
					downloaded.Remove( key );
					if( req.Bmp != null ) {
						req.Bmp.Dispose();
					}
				}
			}
		}
		
		public bool TryGetImage( string identifier, out Bitmap bmp ) {
			bool success = false;
			bmp = null;
			DownloadedRequest req;
			lock( downloadedLocker ) {
				success = downloaded.TryGetValue( identifier, out req );
				if( success ) {
					bmp = req.Bmp;
					downloaded.Remove( identifier );
				}
			}
			return success;
		}
		
		public bool TryGetPage( string identifier, out string page ) {
			bool success = false;
			page = null;
			DownloadedRequest req;
			lock( downloadedLocker ) {
				success = downloaded.TryGetValue( identifier, out req );
				if( success ) {
					page = req.Page;
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
						if( request == null ) return;
					}
				}
				if( request != null ) {
					string url = request.Url;			
					bool isImage = request.IsImage;
					Utils.LogDebug( "Downloading " + ( isImage ? "image" : "page" ) + " from: " + url );
					Bitmap bmp = null;
					string page = null;
					
					try {
						if( isImage ) {
							bmp = DownloadImage( request.Url, client );
						} else {
							page = DownloadPage( request.Url, client );
						}
						Utils.LogDebug( "Downloaded from: " + request.Url );
					} catch( Exception ex ) {
						if( !( ex is WebException || ex is ArgumentException ) ) throw;
						bmp = null;
						page = null;
						Utils.LogDebug( "Failed to download from: " + request.Url );
					}

					lock( downloadedLocker ) {
						DownloadedRequest old;
						if( downloaded.TryGetValue( request.Identifier, out old ) ) {
							Utils.LogDebug( "{0} is already in the queue, replacing it..", request.Identifier );
							if( old.Bmp != null ) old.Bmp.Dispose();
						}
						downloaded[request.Identifier] = new DownloadedRequest( bmp, page );
					}
				} else {
					handle.WaitOne();
				}
			}
		}
		
		static Bitmap DownloadImage( string uri, WebClient client ) {
			byte[] data = client.DownloadData( uri );
			using( MemoryStream stream = new MemoryStream( data ) ) {
				return new Bitmap( stream );
			}
		}
		
		static string DownloadPage( string uri, WebClient client ) {
			return client.DownloadString( uri );
		}
	}
}