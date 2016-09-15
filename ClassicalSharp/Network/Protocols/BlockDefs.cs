// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using OpenTK;

namespace ClassicalSharp.Network.Protocols {

	/// <summary> Implements the packets for BlockDefinitions extension in CPE. </summary>
	public sealed class CPEProtocolBlockDefs : IProtocol {
		
		public CPEProtocolBlockDefs( Game game ) : base( game ) { }
		
		public override void Init() {
			if( !game.UseCPE || !game.AllowCustomBlocks ) return;
			net.Set( Opcode.CpeDefineBlock, HandleDefineBlock, 80 );
			net.Set( Opcode.CpeRemoveBlockDefinition, HandleRemoveBlockDefinition, 2 );
			net.Set( Opcode.CpeDefineBlockExt, HandleDefineBlockExt, 85 );
		}
		
		internal void HandleDefineBlock() {
			byte id = HandleDefineBlockCommonStart( reader, false );
			BlockInfo info = game.BlockInfo;
			byte shape = reader.ReadUInt8();
			if( shape == 0 ) {
				info.IsSprite[id] = true;
				info.IsOpaque[id] = false;
				info.IsTransparent[id] = true;
			} else if( shape <= 16 ) {
				info.MaxBB[id].Y = shape / 16f;
			}
			
			HandleDefineBlockCommonEnd( reader, id );
			// Update sprite BoundingBox if necessary
			if( info.IsSprite[id] ) {
				using( FastBitmap dst = new FastBitmap( game.TerrainAtlas.AtlasBitmap, true, true ) )
					info.RecalculateBB( id, dst );
			}
			info.DefinedCustomBlocks[id >> 5] |= (1u << (id & 0x1F));
		}
		
		void HandleRemoveBlockDefinition() {
			game.BlockInfo.ResetBlockInfo( reader.ReadUInt8(), true );
			game.BlockInfo.InitLightOffsets();
			game.Events.RaiseBlockDefinitionChanged();
		}
		
		void HandleDefineBlockExt() {
			if( !game.AllowCustomBlocks ) {
				net.SkipPacketData( Opcode.CpeDefineBlockExt ); return;
			}
			byte id = HandleDefineBlockCommonStart( reader, 
			                                       net.cpeData.blockDefsExtVer >= 2 );
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
			HandleDefineBlockCommonEnd( reader, id );
			info.DefinedCustomBlocks[id >> 5] |= (1u << (id & 0x1F));
		}
		
		byte HandleDefineBlockCommonStart( NetReader reader, bool uniqueSideTexs ) {
			byte block = reader.ReadUInt8();
			BlockInfo info = game.BlockInfo;
			info.ResetBlockInfo( block, false );
			
			info.Name[block] = reader.ReadCp437String();
			info.Collide[block] = (CollideType)reader.ReadUInt8();
			if( info.Collide[block] != CollideType.Solid ) {
				info.IsTransparent[block] = true;
				info.IsOpaque[block] = false;
			}
			
			info.SpeedMultiplier[block] = (float)Math.Pow( 2, (reader.ReadUInt8() - 128) / 64f );
			info.SetTex( reader.ReadUInt8(), Side.Top, block );
			if( uniqueSideTexs ) {
				info.SetTex( reader.ReadUInt8(), Side.Left, block );
				info.SetTex( reader.ReadUInt8(), Side.Right, block );
				info.SetTex( reader.ReadUInt8(), Side.Front, block );
				info.SetTex( reader.ReadUInt8(), Side.Back, block );
			} else {
				info.SetSide( reader.ReadUInt8(), block );
			}
			info.SetTex( reader.ReadUInt8(), Side.Bottom, block );
			
			info.BlocksLight[block] = reader.ReadUInt8() == 0;
			byte sound = reader.ReadUInt8();
			if( sound < breakSnds.Length ) {
				info.StepSounds[block] = stepSnds[sound];
				info.DigSounds[block] = breakSnds[sound];
			}
			info.FullBright[block] = reader.ReadUInt8() != 0;
			return block;
		}
		
		void HandleDefineBlockCommonEnd( NetReader reader, byte block ) {
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
		
		#if FALSE
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
		#endif
		
		internal static SoundType[] stepSnds, breakSnds;
		static CPEProtocolBlockDefs() {
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
