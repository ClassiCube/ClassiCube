// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Collections.Generic;
using System.IO;

namespace ClassicalSharp.Textures {
	
	public sealed class EntryList {
		
		public List<string> Entries = new List<string>();
		string folder, file;
		
		public EntryList(string folder, string file) {
			this.folder = folder;
			this.file = file;
		}
		
		public void Add(string entry) {
			Entries.Add(entry);
			Save();
		}
		
		public bool Has(string entry) {
			return Entries.Contains(entry);
		}
		
		public bool Load() {
			string path = Path.Combine(folder, file);
			if (!Platform.FileExists(path)) return true;
			
			try {
				using (Stream fs = Platform.FileOpen(path))
					using (StreamReader reader = new StreamReader(fs, false))
				{
					string line;
					while ((line = reader.ReadLine()) != null) {
						line = line.Trim();
						if (line.Length == 0) continue;
						Entries.Add(line);
					}
				}
				return true;
			} catch (IOException ex) {
				ErrorHandler.LogError("loading " + file, ex);
				return false;
			}
		}
		
		public bool Save() {
			try {				
				if (!Platform.DirectoryExists(folder)) {
					Platform.DirectoryCreate(folder);
				}
				
				string path = Path.Combine(folder, file);
				using (Stream fs = Platform.FileCreate(path))
					using (StreamWriter writer = new StreamWriter(fs))
				{
					for (int i = 0; i < Entries.Count; i++)
						writer.WriteLine(Entries[i]);
				}
				return true;
			} catch (IOException ex) {
				ErrorHandler.LogError("saving " + file, ex);
				return false;
			}
		}
	}
}
