// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Collections.Generic;
using System.IO;

namespace ClassicalSharp.Audio {
	
	public class Soundboard {
		
		public byte[] Data;
		internal List<Sound> rawSounds = new List<Sound>();
		Dictionary<string, int> groupFlags = new Dictionary<string, int>();
		Random rnd = new Random();
		
		static string[] soundNames;	
		static Soundboard() {
			soundNames = Enum.GetNames( typeof( SoundType ) );
			for( int i = 0; i < soundNames.Length; i++ )
				soundNames[i] = soundNames[i].ToLower();
		}
		
		public void Init( string group ) {
			string basePath = Path.Combine( Program.AppDirectory, "audio" );
			string binPath = Path.Combine( basePath, group + ".bin" );
			string txtPath = Path.Combine( basePath, group + ".txt" );
			
			Data = File.ReadAllBytes( binPath );
			ReadMetadata( txtPath );
			rawSounds.Sort( (a, b) => a.Name.CompareTo( b.Name ) );
			GetGroups();
		}
		
		public Sound PickRandomSound( SoundType type ) {
			if( type == SoundType.None ) 
				return null;
			string name = soundNames[(int)type];
			int flags = groupFlags[name];
			
			int offset = flags & 0xFFF, count = flags >> 12;
			return rawSounds[offset + rnd.Next( count )];
		}
		
		void ReadMetadata( string path ) {
			using( StreamReader reader = new StreamReader( path ) ) {
				string line;
				while( (line = reader.ReadLine()) != null ) {
					if( line.Length == 0 || line[0] == '#' ) continue;
					string[] parts = line.Split( ',' );
					if( parts.Length < 6 ) continue;
					string name = parts[0].ToLower();
					int sampleRate, bitsPerSample, channels;
					int offset, length;
					
					if( !Int32.TryParse( parts[1], out sampleRate ) ||
					   !Int32.TryParse( parts[2], out bitsPerSample ) ||
					   !Int32.TryParse( parts[3], out channels ) ||
					   !Int32.TryParse( parts[4], out offset ) ||
					   !Int32.TryParse( parts[5], out length ) )
						continue;
					Sound s = new Sound();
					s.Name = name; s.SampleRate = sampleRate;
					s.BitsPerSample = bitsPerSample; s.Channels = channels;
					s.Offset = offset; s.Length = length;
					rawSounds.Add( s );
				}
			}
		}
		
		void GetGroups() {
			string last = Group( rawSounds[0].Name );
			int offset = 0, count = 0;
			for( int i = 0; i < rawSounds.Count; i++ ) {
				string group = Group( rawSounds[i].Name );
				if( group != last ) {
					groupFlags[last] = (count << 12) | offset;
					offset = i;
					last = group;
					count = 0;
				}
				count++;
			}
			groupFlags[last] = (count << 12) | offset;
		}
		
		string Group( string name ) {
			return name.Substring( 0, name.Length - 1 );
		}
	}
	
	public class Sound {
		public string Name;
		public int SampleRate, BitsPerSample, Channels;
		public int Offset, Length;
		public byte Metadata;
	}
}
