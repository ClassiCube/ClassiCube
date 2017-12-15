// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.IO;
using ClassicalSharp;
using ClassicalSharp.Network;
using SharpWave;
using SharpWave.Codecs.Vorbis;

namespace Launcher.Patcher {
	
	public sealed class SoundPatcher {

		string[] files, identifiers;
		string prefix;
		public bool Done;
		
		public SoundPatcher(string[] files, string prefix) {
			this.files = files;
			this.prefix = prefix;
		}
		
		public void FetchFiles(string baseUrl, string altBaseUrl, 
		                       ResourceFetcher fetcher, bool allExist) {
			if (allExist) { Done = true; return; }
			
			identifiers = new string[files.Length];
			for (int i = 0; i < files.Length; i++)
				identifiers[i] = prefix + files[i].Substring(1);
			
			for (int i = 0; i < files.Length; i++) {
				string loc = files[i][0] == 'A' ? baseUrl : altBaseUrl;
				string url = loc + files[i].Substring(1) + ".ogg";
				fetcher.QueueItem(url, identifiers[i]);
			}
		}
		
		public bool CheckDownloaded(ResourceFetcher fetcher, Action<string> setStatus) {
			if (Done) return true;
			for (int i = 0; i < identifiers.Length; i++) {
				Request item;
				if (fetcher.downloader.TryGetItem(identifiers[i], out item)) {
					fetcher.FilesToDownload.RemoveAt(0);
					Utils.LogDebug("got sound " + identifiers[i]);
					
					if (item.Data == null) {
						setStatus("&cFailed to download " + identifiers[i]);
					} else {
						DecodeSound(files[i].Substring(1), (byte[])item.Data);
					}
					
					if (i == identifiers.Length - 1)
						Done = true;
					setStatus(fetcher.MakeNext());
				}
			}
			return true;
		}
		
		void DecodeSound(string name, byte[] rawData) {
			string path = Path.Combine(Program.AppDirectory, "audio");
			path = Path.Combine(path, prefix + name + ".wav");
			
			using (FileStream dst = File.Create(path))
				using (MemoryStream src = new MemoryStream(rawData)) 
			{
				dst.SetLength(44);
				RawOut output = new RawOut(dst, true);
				OggContainer container = new OggContainer(src);
				output.PlayStreaming(container);
				
				dst.Position = 0;
				BinaryWriter w = new BinaryWriter(dst);
				WriteWaveHeader(w, dst, output);
			}
		}
		
		void WriteWaveHeader(BinaryWriter w, Stream stream, RawOut data) {
			WriteFourCC(w, "RIFF");
			w.Write((int)(stream.Length - 8));
			WriteFourCC(w, "WAVE");
			
			WriteFourCC(w, "fmt ");
			w.Write(16);
			w.Write((ushort)1); // audio format, PCM
			w.Write((ushort)data.Last.Channels);
			w.Write(data.Last.SampleRate);
			w.Write((data.Last.SampleRate * data.Last.Channels * data.Last.BitsPerSample) / 8); // byte rate
			w.Write((ushort)((data.Last.Channels * data.Last.BitsPerSample) / 8)); // block align
			w.Write((ushort)data.Last.BitsPerSample);
			
			WriteFourCC(w, "data");
			w.Write((int)(stream.Length - 44));
		}
		
		void WriteFourCC(BinaryWriter w, string fourCC) {
			w.Write((byte)fourCC[0]); w.Write((byte)fourCC[1]);
			w.Write((byte)fourCC[2]); w.Write((byte)fourCC[3]);
		}
	}
}
