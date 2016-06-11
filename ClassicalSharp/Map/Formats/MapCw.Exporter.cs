// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.IO;
using System.IO.Compression;
using ClassicalSharp.Entities;
using OpenTK;

namespace ClassicalSharp.Map {

	public sealed class MapCwExporter : IMapFormatExporter {

		BinaryWriter writer;
		NbtFile nbt;
		Game game;
		World map;
		
		public void Save( Stream stream, Game game ) {
			using( GZipStream wrapper = new GZipStream( stream, CompressionMode.Compress ) ) {
				writer = new BinaryWriter( wrapper );
				nbt = new NbtFile( writer );
				this.game = game;
				map = game.World;
				
				nbt.Write( NbtTagType.Compound ); nbt.Write( "ClassicWorld" );
				
				nbt.Write( NbtTagType.Int8 );
				nbt.Write( "FormatVersion" ); nbt.WriteInt8( 1 );
				
				nbt.Write( NbtTagType.Int8Array );
				nbt.Write( "UUID" ); nbt.WriteInt32( 16 );
				nbt.WriteBytes( map.Uuid.ToByteArray() );
				
				nbt.Write( NbtTagType.Int16 );
				nbt.Write( "X" ); nbt.WriteInt16( (short)map.Width );
				
				nbt.Write( NbtTagType.Int16 );
				nbt.Write( "Y" ); nbt.WriteInt16( (short)map.Height );
				
				nbt.Write( NbtTagType.Int16 );
				nbt.Write( "Z" ); nbt.WriteInt16( (short)map.Length );
				
				WriteSpawnCompoundTag();
				
				nbt.Write( NbtTagType.Int8Array );
				nbt.Write( "BlockArray" ); nbt.WriteInt32( map.blocks.Length );
				nbt.WriteBytes( map.blocks );
				
				WriteMetadata();
				
				nbt.Write( NbtTagType.End );
			}
		}
		
		void WriteSpawnCompoundTag() {
			nbt.Write( NbtTagType.Compound ); nbt.Write( "Spawn" );
			LocalPlayer p = game.LocalPlayer;
			Vector3 spawn = p.Position; // TODO: Maybe also keep real spawn too?
			
			nbt.Write( NbtTagType.Int16 );
			nbt.Write( "X" ); nbt.WriteInt16( (short)spawn.X );
			
			nbt.Write( NbtTagType.Int16 );
			nbt.Write( "Y" ); nbt.WriteInt16( (short)spawn.Y );
			
			nbt.Write( NbtTagType.Int16 );
			nbt.Write( "Z" ); nbt.WriteInt16( (short)spawn.Z );
			
			nbt.Write( NbtTagType.Int8 );
			nbt.Write( "H" );
			nbt.WriteUInt8( (byte)Utils.DegreesToPacked( p.SpawnYaw ) );
			
			nbt.Write( NbtTagType.Int8 );
			nbt.Write( "P" );
			nbt.WriteUInt8( (byte)Utils.DegreesToPacked( p.SpawnPitch ) );
			
			nbt.Write( NbtTagType.End );
		}
		
		void WriteMetadata() {
			nbt.Write( NbtTagType.Compound ); nbt.Write( "Metadata" );
			nbt.Write( NbtTagType.Compound ); nbt.Write( "CPE" );
			LocalPlayer p = game.LocalPlayer;

			nbt.WriteCpeExtCompound( "ClickDistance", 1 );
			nbt.Write( NbtTagType.Int16 );
			nbt.Write( "Distance" ); nbt.WriteInt16( (short)(p.ReachDistance * 32) );
			nbt.Write( NbtTagType.End );
			
			nbt.WriteCpeExtCompound( "EnvWeatherType", 1 );
			nbt.Write( NbtTagType.Int8 );
			nbt.Write( "WeatherType" ); nbt.WriteUInt8( (byte)map.Env.Weather );
			nbt.Write( NbtTagType.End );
			
			nbt.WriteCpeExtCompound( "EnvMapAppearance", 1 );
			nbt.Write( NbtTagType.Int8 );
			nbt.Write( "SideBlock" ); nbt.WriteUInt8( map.Env.SidesBlock );
			nbt.Write( NbtTagType.Int8 );
			nbt.Write( "EdgeBlock" ); nbt.WriteUInt8( map.Env.EdgeBlock );
			nbt.Write( NbtTagType.Int16 );
			nbt.Write( "SideLevel" ); nbt.WriteInt16( (short)map.Env.EdgeHeight );
			nbt.Write( NbtTagType.String );
			string url = game.World.TextureUrl == null ? "" : game.World.TextureUrl;
			nbt.Write( "TextureURL" ); nbt.Write( url );
			nbt.Write( NbtTagType.End );
			
			nbt.WriteCpeExtCompound( "EnvColors", 1 );
			WriteColourCompound( "Sky", map.Env.SkyCol );
			WriteColourCompound( "Cloud", map.Env.CloudsCol );
			WriteColourCompound( "Fog", map.Env.FogCol );
			WriteColourCompound( "Ambient", map.Env.Shadowlight );
			WriteColourCompound( "Sunlight", map.Env.Sunlight );
			nbt.Write( NbtTagType.End );
			
			nbt.WriteCpeExtCompound( "BlockDefinitions", 1 );
			uint[] flags = game.BlockInfo.DefinedCustomBlocks;
			for( int block = 1; block < 256; block++ ) {
				if( (flags[block >> 5] & (1u << (block & 0x1F))) != 0 )
					WriteBlockDefinitionCompound( (byte)block );
			}
			nbt.Write( NbtTagType.End );
			
			nbt.Write( NbtTagType.End );
			nbt.Write( NbtTagType.End );
		}
		
		void WriteColourCompound( string name, FastColour col ) {
			nbt.Write( NbtTagType.Compound ); nbt.Write( name );
			
			nbt.Write( NbtTagType.Int16 );
			nbt.Write( "R" ); nbt.WriteInt16( col.R );
			nbt.Write( NbtTagType.Int16 );
			nbt.Write( "G" ); nbt.WriteInt16( col.G );
			nbt.Write( NbtTagType.Int16 );
			nbt.Write( "B" ); nbt.WriteInt16( col.B );
			
			nbt.Write( NbtTagType.End );
		}
		
		unsafe void WriteBlockDefinitionCompound( byte id ) {
			BlockInfo info = game.BlockInfo;
			nbt.Write( NbtTagType.Compound ); nbt.Write( "Block" + id );
			
			nbt.Write( NbtTagType.Int8 );
			nbt.Write( "ID" ); nbt.WriteUInt8( id );
			nbt.Write( NbtTagType.String );
			nbt.Write( "Name" ); nbt.Write( info.Name[id] );
			nbt.Write( NbtTagType.Int8 );
			nbt.Write( "CollideType" ); nbt.WriteUInt8( (byte)info.Collide[id] );		
			float speed = info.SpeedMultiplier[id];
			nbt.Write( NbtTagType.Real32 );
			nbt.Write( "Speed" ); nbt.WriteInt32( *((int*)&speed) );
			
			nbt.Write( NbtTagType.Int8Array );
			nbt.Write( "Textures" ); nbt.WriteInt32( 6 );
			nbt.WriteUInt8( info.GetTextureLoc( id, Side.Top ) );
			nbt.WriteUInt8( info.GetTextureLoc( id, Side.Bottom ) );
			nbt.WriteUInt8( info.GetTextureLoc( id, Side.Left ) );
			nbt.WriteUInt8( info.GetTextureLoc( id, Side.Right ) );
			nbt.WriteUInt8( info.GetTextureLoc( id, Side.Front ) );
			nbt.WriteUInt8( info.GetTextureLoc( id, Side.Back ) );
			
			nbt.Write( NbtTagType.Int8 );
			nbt.Write( "TransmitsLight" ); nbt.WriteUInt8( info.BlocksLight[id] ? 0 : 1 );
			nbt.Write( NbtTagType.Int8 );
			nbt.Write( "WalkSound" ); nbt.WriteUInt8( (byte)info.DigSounds[id] );
			nbt.Write( NbtTagType.Int8 );
			nbt.Write( "FullBright" ); nbt.WriteUInt8( info.FullBright[id] ? 1 : 0 );
			nbt.Write( NbtTagType.Int8 );
			nbt.Write( "Shape" ); nbt.WriteUInt8( GetShape( info, id ) );
			nbt.Write( NbtTagType.Int8 );
			nbt.Write( "BlockDraw" ); nbt.WriteUInt8( GetDraw( info, id ) );
			
			FastColour col = info.FogColour[id];
			nbt.Write( NbtTagType.Int8Array );
			nbt.Write( "Fog" ); nbt.WriteInt32( 4 );
			nbt.WriteUInt8( (byte)(128 * info.FogDensity[id] - 1) );
			nbt.WriteUInt8( col.R ); nbt.WriteUInt8( col.G ); nbt.WriteUInt8( col.B );
			
			Vector3 min = info.MinBB[id], max = info.MaxBB[id];
			nbt.Write( NbtTagType.Int8Array );
			nbt.Write( "Coords" ); nbt.WriteInt32( 6 );
			nbt.WriteUInt8( (byte)(min.X * 16) ); nbt.WriteUInt8( (byte)(min.Y * 16) ); 
			nbt.WriteUInt8( (byte)(min.Z * 16) ); nbt.WriteUInt8( (byte)(max.X * 16) );
			nbt.WriteUInt8( (byte)(max.Y * 16) ); nbt.WriteUInt8( (byte)(max.Z * 16) );
			nbt.Write( NbtTagType.End );
		}
		
		int GetShape( BlockInfo info, byte id ) {
			return info.IsSprite[id] ? 0 : (byte)(info.MaxBB[id].Y * 16);
		}
		
		int GetDraw( BlockInfo info, byte id) {
			if( info.IsTranslucent[id] ) return 3;
			if( info.IsAir[id] ) return 4;
			if( info.IsTransparent[id] ) 
				return info.CullWithNeighbours[id] ? 1 : 2;
			return 0;
		}
	}
}