using System;
using System.IO;
using System.IO.Compression;
using OpenTK;
using NbtCompound = System.Collections.Generic.Dictionary<string, ClassicalSharp.MapCw.NbtTag>;

namespace ClassicalSharp {

	public sealed partial class MapCw : IMapFileFormat {
		
		public override bool SupportsLoading { get { return true; } }

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
				NbtCompound children = (NbtCompound)root.Value;
				if( children.ContainsKey( "Metadata" ) )
					ParseMetadata( children );
				
				NbtCompound spawn = (NbtCompound)children["Spawn"].Value;
				LocalPlayer p = game.LocalPlayer;
				p.SpawnPoint.X = (short)spawn["X"].Value;
				p.SpawnPoint.Y = (short)spawn["Y"].Value;
				p.SpawnPoint.Z = (short)spawn["Z"].Value;
				if( spawn.ContainsKey( "H" ) )
					p.SpawnYaw = (float)Utils.PackedToDegrees( (byte)spawn["H"].Value );
				if( spawn.ContainsKey( "P" ) )
					p.SpawnPitch = (float)Utils.PackedToDegrees( (byte)spawn["P"].Value );
				
				map.Uuid = new Guid( (byte[])children["UUID"].Value );
				width = (short)children["X"].Value;
				height = (short)children["Y"].Value;
				length = (short)children["Z"].Value;
				
				// Older versions incorrectly multiplied spawn coords by * 32, so we check for that.
				if( p.SpawnPoint.X < 0 || p.SpawnPoint.X >= width || p.SpawnPoint.Y < 0 ||
				   p.SpawnPoint.Y >= height || p.SpawnPoint.Z < 0 || p.SpawnPoint.Z >= length ) {
					p.SpawnPoint.X /= 32; p.SpawnPoint.Y /= 32; p.SpawnPoint.Z /= 32;
				}
				return (byte[])children["BlockArray"].Value;
			}
		}
		
		void ParseMetadata( NbtCompound children ) {
			NbtCompound metadata = (NbtCompound)children["Metadata"].Value;
			NbtTag cpeTag;
			LocalPlayer p = game.LocalPlayer;
			if( !metadata.TryGetValue( "CPE", out cpeTag ) ) return;
			
			metadata = (NbtCompound)cpeTag.Value;
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
				if( curCpeExt.ContainsKey( "TextureURL" ) )
					map.TextureUrl = (string)curCpeExt["TextureURL"].Value;
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
			
			if( game.AllowCustomBlocks && CheckKey( "BlockDefinitions", 1, metadata ) ) {
				foreach( var pair in curCpeExt ) {
					if( pair.Value.TagId != NbtTagType.Compound ) continue;
					if( !Utils.CaselessStarts( pair.Key, "Block" ) ) continue;
					ParseBlockDefinition( (NbtCompound)pair.Value.Value );
				}
			}
		}
		
		NbtCompound curCpeExt;
		bool CheckKey( string key, int version, NbtCompound tag ) {
			NbtTag value;
			if( !tag.TryGetValue( key, out value ) ) return false;
			if( value.TagId != NbtTagType.Compound ) return false;
			
			tag = (NbtCompound)value.Value;
			if( !tag.TryGetValue( "ExtensionVersion", out value ) ) return false;
			if( (int)value.Value == version ) {
				curCpeExt = tag;
				return true;
			}
			return false;
		}
		
		FastColour GetColour( string key, FastColour def ) {
			NbtTag tag;
			if( !curCpeExt.TryGetValue( key, out tag ) )
				return def;
			
			NbtCompound compound = (NbtCompound)tag.Value;
			short r = (short)compound["R"].Value;
			short g = (short)compound["G"].Value;
			short b = (short)compound["B"].Value;
			bool invalid = r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255;
			return invalid ? def : new FastColour( r, g, b );
		}
		
		void ParseBlockDefinition( NbtCompound compound ) {
			byte id = (byte)compound["ID"].Value;
			BlockInfo info = game.BlockInfo;
			info.Name[id] = (string)compound["Name"].Value;
			info.CollideType[id] = (BlockCollideType)compound["CollideType"].Value;
			info.SpeedMultiplier[id] = (float)compound["Speed"].Value;
			
			byte[] data = (byte[])compound["Textures"].Value;
			info.SetTex( data[0], TileSide.Top, (Block)id );
			info.SetTex( data[1], TileSide.Bottom, (Block)id );
			info.SetTex( data[2], TileSide.Left, (Block)id );
			info.SetTex( data[3], TileSide.Right, (Block)id );
			info.SetTex( data[4], TileSide.Front, (Block)id );
			info.SetTex( data[5], TileSide.Back, (Block)id );
			
			info.BlocksLight[id] = (byte)compound["TransmitsLight"].Value == 0;
			byte soundId = (byte)compound["WalkSound"].Value;
			info.DigSounds[id] = NetworkProcessor.breakSnds[soundId];
			info.StepSounds[id] = NetworkProcessor.stepSnds[soundId];
			info.FullBright[id] = (byte)compound["FullBright"].Value != 0;
			info.IsSprite[id] = (byte)compound["Shape"].Value == 0;
			NetworkProcessor.SetBlockDraw( info, id, (byte)compound["BlockDraw"].Value );
			
			data = (byte[])compound["Fog"].Value;
			info.FogDensity[id] = (data[0] + 1) / 128f;
			info.FogColour[id] = new FastColour( data[1], data[2], data[3] );

			data = (byte[])compound["Coords"].Value;
			info.MinBB[id] = new Vector3( data[0] / 16f, data[1] / 16f, data[2] / 16f );
			info.MaxBB[id] = new Vector3( data[3] / 16f, data[4] / 16f, data[5] / 16f );
			
			if( info.CollideType[id] != BlockCollideType.Solid ) {
				info.IsTransparent[id] = true;
				info.IsOpaque[id] = false;
			}
			info.SetupCullingCache( id );
			info.InitLightOffsets();
			game.Events.RaiseBlockDefinitionChanged();
			info.DefinedCustomBlocks[id >> 5] |= (1u << (id & 0x1F));
			
			game.Inventory.CanPlace.SetNotOverridable( true, id );
			game.Inventory.CanDelete.SetNotOverridable( true, id );
			game.Events.RaiseBlockPermissionsChanged();
		}
	}
}