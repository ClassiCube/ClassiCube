// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Collections.Generic;
using System.IO;

namespace ClassicalSharp.Audio {

	public class Sound {
		public int SampleRate, BitsPerSample, Channels;
		public byte[] Data;
	}
	
	public class Soundboard {
		
		public byte[] Data;
		Dictionary<string, List<Sound>> allSounds = new Dictionary<string, List<Sound>>();
		Random rnd = new Random();
		
		const StringComparison comp = StringComparison.OrdinalIgnoreCase;
		public void Init( string group, string[] files ) {
			for( int i = 0; i < files.Length; i++ ) {
				string name = Path.GetFileNameWithoutExtension( files[i] );
				if( !name.StartsWith( group, comp )) continue;
				
				// Convert dig_grass1.wav to grass
				name = name.Substring( group.Length ).ToLower();
				name = name.Substring( 0, name.Length - 1 );
				
				List<Sound> sounds = null;
				if( !allSounds.TryGetValue( name, out sounds ) ) {
					sounds = new List<Sound>();
					allSounds[name] = sounds;
				}
				
				try {
					Sound snd = ReadWave( files[i] );
					sounds.Add( snd );
				} catch ( Exception ex ) {
					ErrorHandler.LogError( "Soundboard.ReadWave()", ex );
				}
			}
		}
		
		public Sound PickRandomSound( SoundType type ) {
			if( type == SoundType.None )  return null;
			string name = soundNames[(int)type];
			
			List<Sound> sounds;
			if( !allSounds.TryGetValue( name, out sounds ) ) return null;
			return sounds[rnd.Next( sounds.Count )];
		}
		
		Sound ReadWave( string file ) {
			using( FileStream fs = File.OpenRead( file ) )
				using( BinaryReader r = new BinaryReader( fs ) )
			{
				CheckFourCC( r, "RIFF" );
				r.ReadInt32(); // file size, but we don't care
				CheckFourCC( r, "WAVE" );
				Sound snd = new Sound();
				
				CheckFourCC( r, "fmt " );
				int size = r.ReadInt32();
				if( r.ReadUInt16() != 1 )
					throw new InvalidDataException( "Only PCM audio is supported." );
				size -= 2;
				
				snd.Channels = r.ReadUInt16(); size -= 2;
				snd.SampleRate = r.ReadInt32(); size -= 4;
				r.ReadInt32(); r.ReadUInt16(); size -= 6;
				snd.BitsPerSample = r.ReadUInt16(); size -= 2;
				if( size > 0 )
					fs.Seek( size, SeekOrigin.Current );
				
				CheckFourCC( r, "data" );
				size = r.ReadInt32();
				byte[] data = r.ReadBytes( size );
				snd.Data = data;
				return snd;
			}
		}
		
		void CheckFourCC( BinaryReader r, string fourCC ) {
			if( r.ReadByte() != (byte)fourCC[0] || r.ReadByte() != (byte)fourCC[1]
			   || r.ReadByte() != (byte)fourCC[2] || r.ReadByte() != (byte)fourCC[3] )
				throw new InvalidDataException( "Expected " + fourCC + " fourcc" );
		}
		
		static string[] soundNames;
		static Soundboard() {
			soundNames = Enum.GetNames( typeof( SoundType ) );
			for( int i = 0; i < soundNames.Length; i++ )
				soundNames[i] = soundNames[i].ToLower();
		}
	}
}
