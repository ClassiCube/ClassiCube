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
		
		public void AddEntry(string entry) {
			Entries.Add(entry);
			Save();
		}
		
		public bool HasEntry(string entry) {
			return Entries.Contains(entry);
		}
		
		public bool Load() {
			string path = Path.Combine(Program.AppDirectory, folder);
			path = Path.Combine(path, file);
			if (!File.Exists(path)) return true;
			
			try {
				using (Stream fs = File.OpenRead(path))
					using (StreamReader reader = new StreamReader(fs, false))
				{
					string line;
					while ((line = reader.ReadLine()) != null) {
						line = line.Trim();
						if (line.Length == 0 || line[0] == '#') continue;
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
				string path = Path.Combine(Program.AppDirectory, folder);
				if (!Directory.Exists(path))
				   Directory.CreateDirectory(path);
				
				using (Stream fs = File.Create(Path.Combine(path, file)))
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
