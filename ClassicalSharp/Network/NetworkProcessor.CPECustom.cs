// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.Model;
using OpenTK;

namespace ClassicalSharp.Net {

	public partial class NetworkProcessor : INetworkProcessor {
		
		void HandleCpeDefineBlock() {
			if( !game.AllowCustomBlocks ) {
				SkipPacketData( Opcode.CpeDefineBlock ); return;
			}
			byte id = HandleCpeDefineBlockCommonStart( false );
			BlockInfo info = game.BlockInfo;
			byte shape = reader.ReadUInt8();
			if( shape == 0 ) {
				info.IsSprite[id] = true;
				info.IsOpaque[id] = false;
				info.IsTransparent[id] = true;
			} else if( shape <= 16 ) {
				info.MaxBB[id].Y = shape / 16f;
			}
			
			HandleCpeDefineBlockCommonEnd( id );
			// Update sprite BoundingBox if necessary
			if( info.IsSprite[id] ) {
				using( FastBitmap dst = new FastBitmap( game.TerrainAtlas.AtlasBitmap, true, true ) )
					info.RecalculateBB( id, dst );
			}
			info.DefinedCustomBlocks[id >> 5] |= (1u << (id & 0x1F));
		}
		
		void HandleCpeRemoveBlockDefinition() {
			if( !game.AllowCustomBlocks ) {
				SkipPacketData( Opcode.CpeRemoveBlockDefinition ); return;
			}
			game.BlockInfo.ResetBlockInfo( reader.ReadUInt8(), true );
			game.BlockInfo.InitLightOffsets();
			game.Events.RaiseBlockDefinitionChanged();
		}
		
		void HandleCpeDefineBlockExt() {
			if( !game.AllowCustomBlocks ) {
				SkipPacketData( Opcode.CpeDefineBlockExt ); return;
			}
			byte id = HandleCpeDefineBlockCommonStart( blockDefinitionsExtVer >= 2 );
			BlockInfo info = game.BlockInfo;
			Vector3 min, max;
			
			min.X = reader.ReadUInt8() / 16f; Utils.Clamp( ref min.X, 0, 15/16f );
			min.Y = reader.ReadUInt8() / 16f; Utils.Clamp( ref min.Y, 0, 15/16f );
			min.Z = reader.ReadUInt8() / 16f; Utils.Clamp( ref min.Z, 0, 15/16f );
			max.X = reader.ReadUInt8() / 16f; Utils.Clamp( ref max.X, 1/16f, 1 );
			max.Y = reader.ReadUInt8() / 16f; Utils.Clamp( ref max.Y, 1/16f, 1 );
			max.Z = reader.ReadUInt8() / 16f; Utils.Clamp( ref max.Z, 1/16f, 1 );
			
			info.MinBB[id] = min;
			info.MaxBB[id] = max;
			HandleCpeDefineBlockCommonEnd( id );
			info.DefinedCustomBlocks[id >> 5] |= (1u << (id & 0x1F));
		}
		
		byte HandleCpeDefineBlockCommonStart( bool uniqueSideTexs ) {
			byte block = reader.ReadUInt8();
			BlockInfo info = game.BlockInfo;
			info.ResetBlockInfo( block, false );
			
			info.Name[block] = reader.ReadAsciiString();
			info.Collide[block] = (CollideType)reader.ReadUInt8();
			if( info.Collide[block] != CollideType.Solid ) {
				info.IsTransparent[block] = true;
				info.IsOpaque[block] = false;
			}
			
			info.SpeedMultiplier[block] = (float)Math.Pow( 2, (reader.ReadUInt8() - 128) / 64f );
			info.SetTex( reader.ReadUInt8(), TileSide.Top, (Block)block );
			if( uniqueSideTexs ) {
				info.SetTex( reader.ReadUInt8(), TileSide.Left, (Block)block );
				info.SetTex( reader.ReadUInt8(), TileSide.Right, (Block)block );
				info.SetTex( reader.ReadUInt8(), TileSide.Front, (Block)block );
				info.SetTex( reader.ReadUInt8(), TileSide.Back, (Block)block );
			} else {
				info.SetSide( reader.ReadUInt8(), (Block)block );
			}
			info.SetTex( reader.ReadUInt8(), TileSide.Bottom, (Block)block );
			
			info.BlocksLight[block] = reader.ReadUInt8() == 0;
			byte sound = reader.ReadUInt8();
			if( sound < breakSnds.Length ) {
				info.StepSounds[block] = stepSnds[sound];
				info.DigSounds[block] = breakSnds[sound];
			}
			info.FullBright[block] = reader.ReadUInt8() != 0;
			return block;
		}
		
		void HandleCpeDefineBlockCommonEnd( byte block ) {
			BlockInfo info = game.BlockInfo;
			byte blockDraw = reader.ReadUInt8();
			SetBlockDraw( info, block, blockDraw );
			
			byte fogDensity = reader.ReadUInt8();
			info.FogDensity[block] = fogDensity == 0 ? 0 : (fogDensity + 1) / 128f;
			info.FogColour[block] = new FastColour(
				reader.ReadUInt8(), reader.ReadUInt8(), reader.ReadUInt8() );
			info.SetupCullingCache( block );
			info.InitLightOffsets();
			game.Events.RaiseBlockDefinitionChanged();
		}
		
		void HandleDefineModel() {
			int start = reader.index - 1;
			byte id = reader.ReadUInt8();
			CustomModel model = null;
			
			switch( reader.ReadUInt8() ) {
				case 0:
					model = new CustomModel( game );
					model.ReadSetupPacket( reader );
					game.ModelCache.CustomModels[id] = model;
					break;
				case 1:
					model = game.ModelCache.CustomModels[id];
					if( model != null ) model.ReadMetadataPacket( reader );
					break;
				case 2:
					model = game.ModelCache.CustomModels[id];
					if( model != null ) model.ReadDefinePartPacket( reader );
					break;
				case 3:
					model = game.ModelCache.CustomModels[id];
					if( model != null ) model.ReadRotationPacket( reader );
					break;
			}
			int total = packetSizes[(byte)Opcode.CpeDefineModel];
			reader.Skip( total - (reader.index - start) );
		}
		
		internal static SoundType[] stepSnds, breakSnds;
		static NetworkProcessor() {
			stepSnds = new SoundType[10];
			breakSnds = new SoundType[10];
			stepSnds[0] = SoundType.None; breakSnds[0] = SoundType.None;
			stepSnds[1] = SoundType.Wood; breakSnds[1] = SoundType.Wood;
			stepSnds[2] = SoundType.Gravel; breakSnds[2] = SoundType.Gravel;
			stepSnds[3] = SoundType.Grass; breakSnds[3] = SoundType.Grass;
			stepSnds[4] = SoundType.Stone; breakSnds[4] = SoundType.Stone;
			// TODO: metal sound type, just use stone for now.
			stepSnds[5] = SoundType.Stone; breakSnds[5] = SoundType.Stone;
			stepSnds[6] = SoundType.Stone; breakSnds[6] = SoundType.Glass;
			stepSnds[7] = SoundType.Cloth; breakSnds[7] = SoundType.Cloth;
			stepSnds[8] = SoundType.Sand; breakSnds[8] = SoundType.Sand;
			stepSnds[9] = SoundType.Snow; breakSnds[9] = SoundType.Snow;
		}
		
		internal static void SetBlockDraw( BlockInfo info, byte block, byte blockDraw ) {
			if( blockDraw == 1 ) {
				info.IsTransparent[block] = true;
			} else if( blockDraw == 2 ) {
				info.IsTransparent[block] = true;
				info.CullWithNeighbours[block] = false;
			} else if( blockDraw == 3 ) {
				info.IsTranslucent[block] = true;
			} else if( blockDraw == 4 ) {
				info.IsTransparent[block] = true;
				info.IsAir[block] = true;
			}
			if( info.IsOpaque[block] )
				info.IsOpaque[block] = blockDraw == 0;
		}
	}
}