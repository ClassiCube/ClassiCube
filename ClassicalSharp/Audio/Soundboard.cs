// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Collections.Generic;
using System.IO;

namespace ClassicalSharp.Audio {

	public class Sound {
		public int SampleRate, BitsPerSample, Channels;
		public byte[] Data;
	}
	
	public class SoundGroup {
		public string Name;
		public List<Sound> Sounds;
	}
	
	public class Soundboard {
		
		public byte[] Data;
		List<SoundGroup> groups = new List<SoundGroup>();
		Random rnd = new Random();
		
		const StringComparison comp = StringComparison.OrdinalIgnoreCase;
		public void Init(string sndGroup, string[] files) {
			for (int i = 0; i < files.Length; i++) {
				string name = Path.GetFileNameWithoutExtension(files[i]);
				if (!name.StartsWith(sndGroup, comp)) continue;
				
				// Convert dig_grass1.wav to grass
				name = Utils.ToLower(name.Substring(sndGroup.Length));
				name = name.Substring(0, name.Length - 1);
				
				SoundGroup group = Find(name);
				if (group == null) {
					group = new SoundGroup();
					group.Name = name;
					group.Sounds = new List<Sound>();
					groups.Add(group);
				}
				
				try {
					Sound snd = ReadWave(files[i]);
					group.Sounds.Add(snd);
				} catch (Exception ex) {
					ErrorHandler.LogError("Soundboard.ReadWave()", ex);
				}
			}
		}
		
		SoundGroup Find(string name) {
			for (int i = 0; i < groups.Count; i++) {
				if (groups[i].Name == name) return groups[i];
			}
			return null;
		}
		
		public Sound PickRandomSound(SoundType type) {
			if (type == SoundType.None)  return null;
			if (type == SoundType.Metal) type = SoundType.Stone;
			string name = soundNames[(int)type];
			
			SoundGroup group = Find(name);
			if (group == null) return null;
			return group.Sounds[rnd.Next(group.Sounds.Count)];
		}
		
		Sound ReadWave(string file) {
			using (FileStream fs = File.OpenRead(file))
				using (BinaryReader r = new BinaryReader(fs))
			{
				string fourCC = GetFourCC(r);
				if (fourCC != "RIFF") throw new InvalidDataException("Expected RIFF, got " + fourCC);
				r.ReadInt32(); // file size, but we don't care
				
				fourCC = GetFourCC(r);
				if (fourCC != "WAVE") throw new InvalidDataException("Expected WAVE, got " + fourCC);
				Sound snd = new Sound();
				
				while (fs.Position < fs.Length) {
					fourCC = GetFourCC(r);
					int size = r.ReadInt32();
					
					if (fourCC == "fmt ") {
						HandleFormat(r, ref size, snd);
					} else if (fourCC == "data") {
						snd.Data = r.ReadBytes(size);
						return snd;
					}
					
					// Skip over unhandled data
					if (size > 0) fs.Seek(size, SeekOrigin.Current);
				}
				return null;
			}
		}
		
		
		static void HandleFormat(BinaryReader r, ref int size, Sound snd) {
			if (r.ReadUInt16() != 1)
				throw new InvalidDataException("Only PCM audio is supported.");
			size -= 2;
			
			snd.Channels = r.ReadUInt16(); size -= 2;
			snd.SampleRate = r.ReadInt32(); size -= 4;
			r.ReadInt32(); r.ReadUInt16(); size -= 6;
			snd.BitsPerSample = r.ReadUInt16(); size -= 2;
		}
		
		unsafe string GetFourCC(BinaryReader r) {
			sbyte* fourCC = stackalloc sbyte[4];
			fourCC[0] = r.ReadSByte(); fourCC[1] = r.ReadSByte();
			fourCC[2] = r.ReadSByte(); fourCC[3] = r.ReadSByte();
			return new string(fourCC, 0, 4);
		}
		
		static string[] soundNames;
		static Soundboard() {
			soundNames = Enum.GetNames(typeof(SoundType));
			for (int i = 0; i < soundNames.Length; i++)
				soundNames[i] = soundNames[i].ToLower();
		}
	}
}
