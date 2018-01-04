// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Net;
using System.Text;
using System.Threading;
#if ANDROID
using Android.Graphics;
#endif

namespace ClassicalSharp.Network {

#if !LAUNCHER	
	/// <summary> Specialised producer and consumer queue for downloading data asynchronously. </summary>
	public class AsyncDownloader : IGameComponent {
#else
	/// <summary> Specialised producer and consumer queue for downloading data asynchronously. </summary>
	public class AsyncDownloader {
#endif
		
		EventWaitHandle handle = new EventWaitHandle(false, EventResetMode.AutoReset);
		Thread worker;
		readonly object pendingLocker = new object();
		List<Request> pending = new List<Request>();
		readonly object processedLocker = new object();
		List<Request> processed = new List<Request>();
		string skinServer = null;
		readonly IDrawer2D drawer;
		
		public Request CurrentItem;
		public int CurrentItemProgress = -3;
		public IDrawer2D Drawer;	
		public AsyncDownloader(IDrawer2D drawer) { this.drawer = drawer; }
		
#if !LAUNCHER
		public void Init(Game game) { Init(game.skinServer); }
		public void Ready(Game game) { }
		public void Reset(Game game) {
			lock (pendingLocker)
				pending.Clear();
			handle.Set();
		}
		
		public void OnNewMap(Game game) { }
		public void OnNewMapLoaded(Game game) { }
#endif

		public void Init(string skinServer) {
			this.skinServer = skinServer;
			WebRequest.DefaultWebProxy = null;
			
			worker = new Thread(DownloadThreadWorker, 256 * 1024);
			worker.Name = "ClassicalSharp.AsyncDownloader";
			worker.IsBackground = true;
			worker.Start();
		}
		

#if !LAUNCHER
		/// <summary> Asynchronously downloads a skin. If 'skinName' points to the url then the skin is
		/// downloaded from that url, otherwise it is downloaded from the url 'defaultSkinServer'/'skinName'.png </summary>
		/// <remarks> Identifier is skin_'skinName'.</remarks>
		public void DownloadSkin(string identifier, string skinName) {
			string strippedSkinName = Utils.StripColours(skinName);
			string url = Utils.IsUrlPrefix(skinName, 0) ? skinName :
				skinServer + strippedSkinName + ".png";
			AddRequest(url, false, identifier, RequestType.Bitmap,
			           DateTime.MinValue , null);
		}
#endif
		
		/// <summary> Asynchronously downloads a bitmap image from the specified url.  </summary>
		public void DownloadImage(string url, bool priority, string identifier) {
			AddRequest(url, priority, identifier, RequestType.Bitmap,
			           DateTime.MinValue, null);
		}
		
		/// <summary> Asynchronously downloads a string from the specified url.  </summary>
		public void DownloadPage(string url, bool priority, string identifier) {
			AddRequest(url, priority, identifier, RequestType.String,
			           DateTime.MinValue, null);
		}
		
		/// <summary> Asynchronously downloads a byte array. </summary>
		public void DownloadData(string url, bool priority, string identifier) {
			AddRequest(url, priority, identifier, RequestType.ByteArray,
			           DateTime.MinValue, null);
		}
		
		/// <summary> Asynchronously downloads a bitmap image. </summary>
		public void DownloadImage(string url, bool priority, string identifier,
		                          DateTime lastModified, string etag) {
			AddRequest(url, priority, identifier, RequestType.Bitmap,
			           lastModified, etag);
		}
		
		/// <summary> Asynchronously downloads a byte array. </summary>
		public void DownloadData(string url, bool priority, string identifier,
		                         DateTime lastModified, string etag) {
			AddRequest(url, priority, identifier, RequestType.ByteArray,
			           lastModified, etag);
		}

#if !LAUNCHER		
		/// <summary> Asynchronously retrieves the content length of the body response. </summary>
		public void RetrieveContentLength(string url, bool priority, string identifier) {
			AddRequest(url, priority, identifier, RequestType.ContentLength,
			           DateTime.MinValue, null);
		}
#endif
		
		void AddRequest(string url, bool priority, string identifier,
		                RequestType type, DateTime lastModified, string etag) {
			lock (pendingLocker) {
				Request request = new Request(url, identifier, type, lastModified, etag);
				if (priority) {
					pending.Insert(0, request);
				} else {
					pending.Add(request);
				}
			}
			handle.Set();
		}
		
		/// <summary> Informs the asynchronous thread that it should stop processing further requests
		/// and can consequentially exit the for loop.<br/>
		/// Note that this will *block** the calling thread as the method waits until the asynchronous
		/// thread has exited the for loop. </summary>
		public void Dispose() {
			lock (pendingLocker)  {
				pending.Insert(0, null);
			}
			
			handle.Set();
			worker.Join();
			((IDisposable)handle).Dispose();
		}
		
		#if !LAUNCHER		
		/// <summary> Removes older entries that were downloaded a certain time ago
		/// but were never removed from the downloaded queue. </summary>
		public void PurgeOldEntriesTask(ScheduledTask task) {
			lock (processedLocker) {
				DateTime now = DateTime.UtcNow;
				for (int i = processed.Count - 1; i >= 0; i--) {
					Request item = processed[i];
					if ((now - item.TimeDownloaded).TotalSeconds < 10) continue;
					
					item.Dispose();
					processed.RemoveAt(i);
				}
			}
		}
		#endif
		
		/// <summary> Returns whether the requested item exists in the downloaded queue.
		/// If it does, it removes the item from the queue and outputs it. </summary>
		/// <remarks> If the asynchronous thread failed to download the item, this method
		/// will return 'true' and 'item' will be set. However, the contents of the 'item' object will be null.</remarks>
		public bool TryGetItem(string identifier, out Request item) {
			bool success = false;
			lock (processedLocker) {
				int i = FindRequest(identifier, out item);
				success = i >= 0;
				if (success) processed.RemoveAt(i);
			}
			return success;
		}

		void DownloadThreadWorker() {
			while (true) {
				Request request = null;
				lock (pendingLocker) {
					if (pending.Count > 0) {
						request = pending[0];
						pending.RemoveAt(0);
						if (request == null) return;
					}
				}
				
				if (request != null) {
					CurrentItem = request;
					CurrentItemProgress = -2;
					ProcessRequest(request);
					
					CurrentItem = null;
					CurrentItemProgress = -3;
				} else {
					handle.WaitOne();
				}
			}
		}
		
		int FindRequest(string identifer, out Request item) {
			item = null;
			for (int i = 0; i < processed.Count; i++) {
				if (processed[i].Identifier != identifer) continue;				
				item = processed[i]; 
				return i;
			}
			return -1;
		}
		
		void ProcessRequest(Request request) {
			string url = request.Url;
			Utils.LogDebug("Downloading {0} from: {1}", request.Type, url);
			HttpStatusCode status = HttpStatusCode.OK;
			request.Data = null;
			
			try {
				HttpWebRequest req = MakeRequest(request);
				using (HttpWebResponse response = (HttpWebResponse)req.GetResponse()) {
					request.ETag = response.Headers[HttpResponseHeader.ETag];
					if (response.Headers[HttpResponseHeader.LastModified] != null) {
						request.LastModified = response.LastModified;					
					}
					request.Data = DownloadContent(request, response);
				}
			} catch (Exception ex) {
				if (!(ex is WebException || ex is ArgumentException || ex is UriFormatException || ex is IOException)) throw;

				if (ex is WebException) {
					WebException webEx = (WebException)ex;
					if (webEx.Response != null) {
						status = ((HttpWebResponse)webEx.Response).StatusCode;
						webEx.Response.Close();
					}
				}
				
				if (status != HttpStatusCode.OK) {
					Utils.LogDebug("Failed to download (" + (int)status + ") from: " + url);
				} else {
					Utils.LogDebug("Failed to download from: " + url);
				}
			}
			request.TimeDownloaded = DateTime.UtcNow;
			
			lock (processedLocker) {
				Request older;
				int index = FindRequest(request.Identifier, out older);
				
				if (index >= 0) {
					if (older.TimeAdded > request.TimeAdded) {
						Request tmp = older; older = request; request = tmp;
					}
					
					older.Dispose();
					processed[index] = request;
				} else {
					processed.Add(request);
				}
			}
		}
		
		object DownloadContent(Request request, HttpWebResponse response) {
			if (request.Type == RequestType.Bitmap) {
				MemoryStream data = DownloadBytes(response);
				Bitmap bmp = Platform.ReadBmp32Bpp(drawer, data);
				
				if (bmp == null) {
					Utils.LogDebug("Failed to download from: " + request.Url);
				}
				return bmp;
			} else if (request.Type == RequestType.String) {
				MemoryStream data = DownloadBytes(response);
				byte[] rawBuffer = data.GetBuffer();
				return Encoding.UTF8.GetString(rawBuffer, 0, (int)data.Length);
			} else if (request.Type == RequestType.ByteArray) {
				MemoryStream data = DownloadBytes(response);
				return data.ToArray();
			} else if (request.Type == RequestType.ContentLength) {
				return response.ContentLength;
			}
			return null;
		}
		
		HttpWebRequest MakeRequest(Request request) {
			HttpWebRequest req = (HttpWebRequest)WebRequest.Create(request.Url);
			req.AutomaticDecompression = DecompressionMethods.GZip;
			req.ReadWriteTimeout = 90 * 1000;
			req.Timeout = 90 * 1000;
			req.Proxy = null;
			req.UserAgent = Program.AppName;
			
			if (request.LastModified != DateTime.MinValue)
				req.IfModifiedSince = request.LastModified;
			if (request.ETag != null)
				req.Headers["If-None-Match"] = request.ETag;
			return req;
		}
		
		static byte[] buffer = new byte[4096 * 8];
		MemoryStream DownloadBytes(HttpWebResponse response) {
			int length = (int)response.ContentLength;
			MemoryStream dst = length > 0 ?
				new MemoryStream(length) : new MemoryStream();
			CurrentItemProgress = length > 0 ? 0 : -1;
			
			using (Stream src = response.GetResponseStream()) {
				int read = 0;
				while ((read = src.Read(buffer, 0, buffer.Length)) > 0) {
					dst.Write(buffer, 0, read);
					if (length <= 0) continue;
					CurrentItemProgress = (int)(100 * (float)dst.Length / length);
				}
			}
			return dst;
		}
	}
	
	public enum RequestType { Bitmap, String, ByteArray, ContentLength }
	
	public sealed class Request {
		
		/// <summary> Full url to GET from. </summary>
		public string Url;		
		/// <summary> Unique identifier for this request. </summary>
		public string Identifier;		
		/// <summary> Type of data to return for this request. </summary>
		public RequestType Type;
		
		/// <summary> Point in time this request was added to the fetch queue. </summary>
		public DateTime TimeAdded;
		/// <summary> Point in time the item was fully downloaded. </summary>
		public DateTime TimeDownloaded;
		/// <summary> Contents that were downloaded. </summary>
		public object Data;
		
		/// <summary> Point in time the item most recently cached. (if at all) </summary>
		public DateTime LastModified;
		/// <summary> ETag of the item most recently cached. (if any) </summary>
		public string ETag;
		
		public Request(string url, string identifier, RequestType type, DateTime lastModified, string etag) {
			Url = url;
			Identifier = identifier;
			Type = type;
			TimeAdded = DateTime.UtcNow;
			LastModified = lastModified;
			ETag = etag;
		}
		
		public void Dispose() {
			Bitmap bmp = Data as Bitmap;
			if (bmp != null) bmp.Dispose();
		}
	}
}