using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Net;
using System.Text;

namespace ClassicalSharp {

	public sealed partial class MapCw : IMapFileFormat {
		
		public override bool SupportsLoading {
			get { return true; }
		}

		BinaryReader reader;
		Game game;
		Map map;
		NbtTag invalid = default( NbtTag );
		
		public override byte[] Load( Stream stream, Game game, out int width, out int height, out int length ) {
			using( GZipStream wrapper = new GZipStream( stream, CompressionMode.Decompress ) ) {
				reader = new BinaryReader( wrapper );
				if( reader.ReadByte() != (byte)NbtTagType.Compound )
					throw new InvalidDataException( "Nbt file must start with Tag_Compound" );
				this.game = game;
				map = game.Map;
				
				invalid.TagId = NbtTagType.Invalid;
				NbtTag root = ReadTag( (byte)NbtTagType.Compound, true );
				Dictionary<string, NbtTag> children = (Dictionary<string, NbtTag>)root.Value;
				if( children.ContainsKey( "Metadata" ) )
					ParseMetadata( children );
				
				Dictionary<string, NbtTag> spawn = (Dictionary<string, NbtTag>)children["Spawn"].Value;
				LocalPlayer p = game.LocalPlayer;
				p.SpawnPoint.X = (short)spawn["X"].Value / 32f;
				p.SpawnPoint.Y = (short)spawn["Y"].Value / 32f;
				p.SpawnPoint.Z = (short)spawn["Z"].Value / 32f;
				
				map.Uuid = new Guid( (byte[])children["UUID"].Value );
				width = (short)children["X"].Value;
				height = (short)children["Y"].Value;
				length = (short)children["Z"].Value;
				return (byte[])children["BlockArray"].Value;
			}
		}
		
		void ParseMetadata( Dictionary<string, NbtTag> children ) {
			Dictionary<string, NbtTag> metadata = (Dictionary<string, NbtTag>)children["Metadata"].Value;
			NbtTag cpeTag;
			LocalPlayer p = game.LocalPlayer;			
			if( !metadata.TryGetValue( "CPE", out cpeTag ) ) return;
			
			metadata = (Dictionary<string, NbtTag>)cpeTag.Value;
			if( CheckKey( "ClickDistance", 1, metadata ) ) {
				p.ReachDistance = (short)curCpeExt["Distance"].Value / 32f;
			}
			if( CheckKey( "EnvColors", 1, metadata ) ) {
				map.SetSkyColour( GetColour( "Sky", Map.DefaultSkyColour ) );
				map.SetCloudsColour( GetColour( "Cloud", Map.DefaultCloudsColour ) );
				map.SetFogColour( GetColour( "Fog", Map.DefaultFogColour ) );
				map.SetSunlight( GetColour( "Sunlight", Map.DefaultSunlight ) );
				map.SetShadowlight( GetColour( "Ambient", Map.DefaultShadowlight ) );
			}
			if( CheckKey( "EnvMapAppearance", 1, metadata ) ) {
				string url = (string)curCpeExt["TextureURL"].Value;
				byte sidesBlock = (byte)curCpeExt["SideBlock"].Value;
				byte edgeBlock = (byte)curCpeExt["EdgeBlock"].Value;
				map.SetSidesBlock( (Block)sidesBlock );
				map.SetEdgeBlock( (Block)edgeBlock );
				map.SetEdgeLevel( (short)curCpeExt["SideLevel"].Value );
			}
			if( CheckKey( "EnvWeatherType", 1, metadata ) ) {
				byte weather = (byte)curCpeExt["WeatherType"].Value;
				map.SetWeather( (Weather)weather );
			}
		}
		
		FastColour GetColour( string key, FastColour def ) {
			NbtTag tag;
			if( !curCpeExt.TryGetValue( key, out tag ) )
				return def;
			
			Dictionary<string, NbtTag> compound = (Dictionary<string, NbtTag>)tag.Value;
			short r = (short)compound["R"].Value;
			short g = (short)compound["G"].Value;
			short b = (short)compound["B"].Value;
			bool invalid = r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255;
			return invalid ? def : new FastColour( r, g, b );
		}
		
		Dictionary<string, NbtTag> curCpeExt;
		bool CheckKey( string key, int version, Dictionary<string, NbtTag> tag ) {
			NbtTag value;
			if( !tag.TryGetValue( key, out value ) ) return false;
			if( value.TagId != NbtTagType.Compound ) return false;
			
			tag = (Dictionary<string, NbtTag>)value.Value;
			if( !tag.TryGetValue( "ExtensionVersion", out value ) ) return false;
			if( (int)value.Value == version ) {
				curCpeExt = tag;
				return true;
			}
			return false;
		}
		
		unsafe NbtTag ReadTag( byte typeId, bool readTagName ) {
			if( typeId == 0 ) return invalid;
			
			NbtTag tag = default( NbtTag );
			tag.Name = readTagName ? ReadString() : null;
			tag.TagId = (NbtTagType)typeId;
			
			switch( (NbtTagType)typeId ) {
				case NbtTagType.Int8:
					tag.Value = reader.ReadByte(); break;
				case NbtTagType.Int16:
					tag.Value = ReadInt16(); break;
				case NbtTagType.Int32:
					tag.Value = ReadInt32(); break;
				case NbtTagType.Int64:
					tag.Value = ReadInt64(); break;
				case NbtTagType.Real32:
					int temp32 = ReadInt32();
					tag.Value = *(float*)&temp32; break;
				case NbtTagType.Real64:
					long temp64 = ReadInt64();
					tag.Value = *(double*)&temp64; break;
				case NbtTagType.Int8Array:
					tag.Value = reader.ReadBytes( ReadInt32() ); break;
				case NbtTagType.String:
					tag.Value = ReadString(); break;
					
				case NbtTagType.List:
					NbtList list = new NbtList();
					list.ChildTagId = (NbtTagType)reader.ReadByte();
					list.ChildrenValues = new object[ReadInt32()];
					for( int i = 0; i < list.ChildrenValues.Length; i++ ) {
						list.ChildrenValues[i] = ReadTag( (byte)list.ChildTagId, false );
					}
					tag.Value = list; break;
					
				case NbtTagType.Compound:
					Dictionary<string, NbtTag> children = new Dictionary<string, NbtTag>();
					NbtTag child;
					while( (child = ReadTag( reader.ReadByte(), true )).TagId != NbtTagType.Invalid ) {
						children[child.Name] = child;
					}
					tag.Value = children; break;
					
				case NbtTagType.Int32Array:
					int[] array = new int[ReadInt32()];
					for( int i = 0; i < array.Length; i++ )
						array[i] = ReadInt32();
					tag.Value = array; break;
					
				default:
					throw new InvalidDataException( "Unrecognised tag id: " + typeId );
			}
			return tag;
		}
		
		struct NbtTag {
			public string Name;
			public object Value;
			public NbtTagType TagId;
		}
		
		class NbtList {
			public NbtTagType ChildTagId;
			public object[] ChildrenValues;
		}		
		
		enum NbtTagType : byte {
			End, Int8, Int16, Int32, Int64, 
			Real32, Real64, Int8Array, String, 
			List, Compound, Int32Array,
			Invalid = 255,
		}
		
		long ReadInt64() { return IPAddress.HostToNetworkOrder( reader.ReadInt64() ); }
		int ReadInt32() { return IPAddress.HostToNetworkOrder( reader.ReadInt32() ); }
		short ReadInt16() { return IPAddress.HostToNetworkOrder( reader.ReadInt16() ); }
		
		string ReadString() {
			int len = (ushort)ReadInt16();
			byte[] data = reader.ReadBytes( len );
			return Encoding.UTF8.GetString( data ); 
		}
	}
}