using System;
using System.IO;
using System.IO.Compression;
using OpenTK;

namespace ClassicalSharp {

	public sealed partial class MapCw : IMapFileFormat {
		
		public override bool SupportsSaving { get { return true; } }

		BinaryWriter writer;
		public override void Save( Stream stream, Game game ) {
			using( GZipStream wrapper = new GZipStream( stream, CompressionMode.Compress ) ) {
				writer = new BinaryWriter( wrapper );
				this.game = game;
				map = game.Map;
				
				WriteTag( NbtTagType.Compound ); WriteString( "ClassicWorld" );
				
				WriteTag( NbtTagType.Int8 );
				WriteString( "FormatVersion" ); WriteInt8( 1 );
				
				WriteTag( NbtTagType.Int8Array );
				WriteString( "UUID" ); WriteInt32( 16 );
				WriteBytes( map.Uuid.ToByteArray() );
				
				WriteTag( NbtTagType.Int16 );
				WriteString( "X" ); WriteInt16( (short)map.Width );
				
				WriteTag( NbtTagType.Int16 );
				WriteString( "Y" ); WriteInt16( (short)map.Height );
				
				WriteTag( NbtTagType.Int16 );
				WriteString( "Z" ); WriteInt16( (short)map.Length );
				
				WriteSpawnCompoundTag();
				
				WriteTag( NbtTagType.Int8Array );
				WriteString( "BlockArray" ); WriteInt32( map.mapData.Length );
				WriteBytes( map.mapData );
				
				WriteMetadata();
				
				WriteTag( NbtTagType.End );
			}
		}
		
		void WriteSpawnCompoundTag() {
			WriteTag( NbtTagType.Compound ); WriteString( "Spawn" );
			LocalPlayer p = game.LocalPlayer;
			Vector3 spawn = p.Position; // TODO: Maybe also keep real spawn too?
			
			WriteTag( NbtTagType.Int16 );
			WriteString( "X" ); WriteInt16( (short)spawn.X );
			
			WriteTag( NbtTagType.Int16 );
			WriteString( "Y" ); WriteInt16( (short)spawn.Y );
			
			WriteTag( NbtTagType.Int16 );
			WriteString( "Z" ); WriteInt16( (short)spawn.Z );
			
			WriteTag( NbtTagType.Int8 );
			WriteString( "H" );
			WriteUInt8( (byte)Utils.DegreesToPacked( p.SpawnYaw, 256 ) );
			
			WriteTag( NbtTagType.Int8 );
			WriteString( "P" );
			WriteUInt8( (byte)Utils.DegreesToPacked( p.SpawnPitch, 256 ) );
			
			WriteTag( NbtTagType.End );
		}
		
		void WriteMetadata() {
			WriteTag( NbtTagType.Compound ); WriteString( "Metadata" );
			WriteTag( NbtTagType.Compound ); WriteString( "CPE" );
			LocalPlayer p = game.LocalPlayer;

			WriteCpeExtCompound( "ClickDistance", 1 );
			WriteTag( NbtTagType.Int16 );
			WriteString( "Distance" ); WriteInt16( (short)(p.ReachDistance * 32) );
			WriteTag( NbtTagType.End );
			
			WriteCpeExtCompound( "EnvWeatherType", 1 );
			WriteTag( NbtTagType.Int8 );
			WriteString( "WeatherType" ); WriteUInt8( (byte)map.Weather );
			WriteTag( NbtTagType.End );
			
			WriteCpeExtCompound( "EnvMapAppearance", 1 );
			WriteTag( NbtTagType.Int8 );
			WriteString( "SideBlock" ); WriteUInt8( (byte)map.SidesBlock );
			WriteTag( NbtTagType.Int8 );
			WriteString( "EdgeBlock" ); WriteUInt8( (byte)map.EdgeBlock );
			WriteTag( NbtTagType.Int16 );
			WriteString( "SideLevel" ); WriteInt16( (short)map.EdgeHeight );
			WriteTag( NbtTagType.String );
			string url = game.Map.TextureUrl == null ? "" : game.Map.TextureUrl;
			WriteString( "TextureURL" ); WriteString( url );
			WriteTag( NbtTagType.End );
			
			WriteCpeExtCompound( "EnvColors", 1 );
			WriteColourCompound( "Sky", map.SkyCol );
			WriteColourCompound( "Cloud", map.CloudsCol );
			WriteColourCompound( "Fog", map.FogCol );
			WriteColourCompound( "Ambient", map.Shadowlight );
			WriteColourCompound( "Sunlight", map.Sunlight );
			WriteTag( NbtTagType.End );
			
			WriteCpeExtCompound( "BlockDefinitions", 1 );
			uint[] flags = game.BlockInfo.DefinedCustomBlocks;
			for( int tile = 1; tile < 256; tile++ ) {
				if( (flags[tile >> 5] & (1u << (tile & 0x1F))) != 0 )
					WriteBlockDefinitionCompound( (byte)tile );
			}
			WriteTag( NbtTagType.End );
			
			WriteTag( NbtTagType.End );
			WriteTag( NbtTagType.End );
		}
		
		void WriteColourCompound( string name, FastColour col ) {
			WriteTag( NbtTagType.Compound ); WriteString( name );
			
			WriteTag( NbtTagType.Int16 );
			WriteString( "R" ); WriteInt16( col.R );
			WriteTag( NbtTagType.Int16 );
			WriteString( "G" ); WriteInt16( col.G );
			WriteTag( NbtTagType.Int16 );
			WriteString( "B" ); WriteInt16( col.B );
			
			WriteTag( NbtTagType.End );
		}
		
		unsafe void WriteBlockDefinitionCompound( byte id ) {
			BlockInfo info = game.BlockInfo;
			WriteTag( NbtTagType.Compound ); WriteString( "Block" + id );
			
			WriteTag( NbtTagType.Int8 );
			WriteString( "ID" ); WriteUInt8( id );
			WriteTag( NbtTagType.String );
			WriteString( "Name" ); WriteString( info.Name[id] );
			WriteTag( NbtTagType.Int8 );
			WriteString( "CollideType" ); WriteUInt8( (byte)info.CollideType[id] );		
			float speed = info.SpeedMultiplier[id];
			WriteTag( NbtTagType.Real32 );
			WriteString( "Speed" ); WriteInt32( *((int*)&speed) );
			
			WriteTag( NbtTagType.Int8Array );
			WriteString( "Textures" ); WriteInt32( 6 );
			WriteUInt8( info.GetTextureLoc( id, TileSide.Top ) );
			WriteUInt8( info.GetTextureLoc( id, TileSide.Bottom ) );
			WriteUInt8( info.GetTextureLoc( id, TileSide.Left ) );
			WriteUInt8( info.GetTextureLoc( id, TileSide.Right ) );
			WriteUInt8( info.GetTextureLoc( id, TileSide.Front ) );
			WriteUInt8( info.GetTextureLoc( id, TileSide.Back ) );
			
			WriteTag( NbtTagType.Int8 );
			WriteString( "TransmitsLight" ); WriteUInt8( info.BlocksLight[id] ? 0 : 1 );
			WriteTag( NbtTagType.Int8 );
			WriteString( "WalkSound" ); WriteUInt8( (byte)info.DigSounds[id] );
			WriteTag( NbtTagType.Int8 );
			WriteString( "FullBright" ); WriteUInt8( info.FullBright[id] ? 1 : 0 );
			WriteTag( NbtTagType.Int8 );
			WriteString( "Shape" ); WriteUInt8( GetShape( info, id ) );
			WriteTag( NbtTagType.Int8 );
			WriteString( "BlockDraw" ); WriteUInt8( GetDraw( info, id ) );
			
			FastColour col = info.FogColour[id];
			WriteTag( NbtTagType.Int8Array );
			WriteString( "Fog" ); WriteInt32( 4 );
			WriteUInt8( (byte)(128 * info.FogDensity[id] - 1) );
			WriteUInt8( col.R ); WriteUInt8( col.G ); WriteUInt8( col.B );
			
			Vector3 min = info.MinBB[id], max = info.MaxBB[id];
			WriteTag( NbtTagType.Int8Array );
			WriteString( "Coords" ); WriteInt32( 6 );
			WriteUInt8( (byte)(min.X * 16) ); WriteUInt8( (byte)(min.Y * 16) ); 
			WriteUInt8( (byte)(min.Z * 16) ); WriteUInt8( (byte)(max.X * 16) );
			WriteUInt8( (byte)(max.Y * 16) ); WriteUInt8( (byte)(max.Z * 16) );
			WriteTag( NbtTagType.End );
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