// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using OpenTK;
#if USE16_BIT
using BlockID = System.UInt16;
#else
using BlockID = System.Byte;
#endif

namespace ClassicalSharp.Network.Protocols {

	/// <summary> Implements the packets for BlockDefinitions extension in CPE. </summary>
	public sealed class CPEProtocolBlockDefs : IProtocol {
		
		public CPEProtocolBlockDefs(Game game) : base(game) { }
		
		public override void Init() { Reset(); }
		
		public override void Reset() {
			if (!game.UseCPE || !game.UseCustomBlocks) return;
			net.Set(Opcode.CpeDefineBlock, HandleDefineBlock, 80);
			net.Set(Opcode.CpeUndefineBlock, HandleRemoveBlockDefinition, 2);
			net.Set(Opcode.CpeDefineBlockExt, HandleDefineBlockExt, 85);
		}
		
		internal void HandleDefineBlock() {
			BlockID block = HandleDefineBlockCommonStart(reader, false);
			
			byte shape = reader.ReadUInt8();
			if (shape > 0 && shape <= 16) {
				BlockInfo.MaxBB[block].Y = shape / 16f;
			}
			
			HandleDefineBlockCommonEnd(reader, shape, block);
			// Update sprite BoundingBox if necessary
			if (BlockInfo.Draw[block] == DrawType.Sprite) {
				using (FastBitmap dst = new FastBitmap(TerrainAtlas2D.Atlas, true, true))
					BlockInfo.RecalculateBB(block, dst);
			}
		}
		
		void HandleRemoveBlockDefinition() {
			BlockID block = reader.ReadBlock();
			bool didBlockLight = BlockInfo.BlocksLight[block];
			
			BlockInfo.ResetBlockProps(block);
			OnBlockUpdated(block, didBlockLight);
			BlockInfo.UpdateCulling(block);
			
			game.Inventory.Reset(block);
			if (block < Block.CpeCount) {
				game.Inventory.AddDefault(block);
			}
			
			BlockInfo.SetCustomDefined(block, false);
			game.Events.RaiseBlockDefinitionChanged();
		}
		
		void OnBlockUpdated(BlockID block, bool didBlockLight) {
			if (!game.World.HasBlocks) return;
			
			// Need to refresh lighting when a block's light blocking state changes
			if (BlockInfo.BlocksLight[block] != didBlockLight) {
				game.Lighting.Refresh();
			}
		}
		
		void HandleDefineBlockExt() {
			if (!game.UseCustomBlocks) {
				net.SkipPacketData(Opcode.CpeDefineBlockExt); return;
			}
			BlockID block = HandleDefineBlockCommonStart(reader, net.cpeData.blockDefsExtVer >= 2);
			Vector3 min, max;
			
			min.X = reader.ReadUInt8() / 16f; Utils.Clamp(ref min.X, 0, 15/16f);
			min.Y = reader.ReadUInt8() / 16f; Utils.Clamp(ref min.Y, 0, 15/16f);
			min.Z = reader.ReadUInt8() / 16f; Utils.Clamp(ref min.Z, 0, 15/16f);
			max.X = reader.ReadUInt8() / 16f; Utils.Clamp(ref max.X, 1/16f, 1);
			max.Y = reader.ReadUInt8() / 16f; Utils.Clamp(ref max.Y, 1/16f, 1);
			max.Z = reader.ReadUInt8() / 16f; Utils.Clamp(ref max.Z, 1/16f, 1);
			
			BlockInfo.MinBB[block] = min;
			BlockInfo.MaxBB[block] = max;
			HandleDefineBlockCommonEnd(reader, 1, block);
		}
		
		BlockID HandleDefineBlockCommonStart(NetReader reader, bool uniqueSideTexs) {
			BlockID block = reader.ReadBlock();
			bool didBlockLight = BlockInfo.BlocksLight[block];
			BlockInfo.ResetBlockProps(block);
			
			BlockInfo.Name[block] = reader.ReadString();
			BlockInfo.SetCollide(block, reader.ReadUInt8());
			
			BlockInfo.SpeedMultiplier[block] = (float)Math.Pow(2, (reader.ReadUInt8() - 128) / 64f);
			BlockInfo.SetTex(reader.ReadUInt8(), Side.Top, block);
			if (uniqueSideTexs) {
				BlockInfo.SetTex(reader.ReadUInt8(), Side.Left, block);
				BlockInfo.SetTex(reader.ReadUInt8(), Side.Right, block);
				BlockInfo.SetTex(reader.ReadUInt8(), Side.Front, block);
				BlockInfo.SetTex(reader.ReadUInt8(), Side.Back, block);
			} else {
				BlockInfo.SetSide(reader.ReadUInt8(), block);
			}
			BlockInfo.SetTex(reader.ReadUInt8(), Side.Bottom, block);
			
			BlockInfo.BlocksLight[block] = reader.ReadUInt8() == 0;
			OnBlockUpdated(block, didBlockLight);
			
			byte sound = reader.ReadUInt8();
			if (sound < breakSnds.Length) {
				BlockInfo.StepSounds[block] = stepSnds[sound];
				BlockInfo.DigSounds[block] = breakSnds[sound];
			}
			BlockInfo.FullBright[block] = reader.ReadUInt8() != 0;
			return block;
		}
		
		void HandleDefineBlockCommonEnd(NetReader reader, byte shape, BlockID block) {
			byte blockDraw = reader.ReadUInt8();
			if (shape == 0) {
				BlockInfo.SpriteOffset[block] = blockDraw;
				blockDraw = DrawType.Sprite;
			}
			BlockInfo.LightOffset[block] = BlockInfo.CalcLightOffset(block);
			
			byte fogDensity = reader.ReadUInt8();
			BlockInfo.FogDensity[block] = fogDensity == 0 ? 0 : (fogDensity + 1) / 128f;
			BlockInfo.FogColour[block] = new FastColour(
				reader.ReadUInt8(), reader.ReadUInt8(), reader.ReadUInt8());
			BlockInfo.Tinted[block] = BlockInfo.FogColour[block] != FastColour.Black && BlockInfo.Name[block].IndexOf('#') >= 0;
			
			BlockInfo.SetBlockDraw(block, blockDraw);
			BlockInfo.CalcRenderBounds(block);
			BlockInfo.UpdateCulling(block);
			
			game.Inventory.AddDefault(block);
			BlockInfo.SetCustomDefined(block, true);
			game.Events.RaiseBlockDefinitionChanged();
		}
		
		#if FALSE
		void HandleDefineModel() {
			int start = reader.index - 1;
			byte id = reader.ReadUInt8();
			CustomModel model = null;
			
			switch (reader.ReadUInt8()) {
				case 0:
					model = new CustomModel(game);
					model.ReadSetupPacket(reader);
					game.ModelCache.CustomModels[id] = model;
					break;
				case 1:
					model = game.ModelCache.CustomModels[id];
					if (model != null) model.ReadMetadataPacket(reader);
					break;
				case 2:
					model = game.ModelCache.CustomModels[id];
					if (model != null) model.ReadDefinePartPacket(reader);
					break;
				case 3:
					model = game.ModelCache.CustomModels[id];
					if (model != null) model.ReadRotationPacket(reader);
					break;
			}
			int total = packetSizes[(byte)Opcode.CpeDefineModel];
			reader.Skip(total - (reader.index - start));
		}
		#endif
		
		internal static byte[] stepSnds = new byte[10] {
			SoundType.None, SoundType.Wood, SoundType.Gravel,
			SoundType.Grass, SoundType.Stone, SoundType.Metal,
			SoundType.Stone, SoundType.Cloth, SoundType.Sand,
			SoundType.Snow,
		};
		
		internal static byte[] breakSnds = new byte[10] {
			SoundType.None, SoundType.Wood, SoundType.Gravel,
			SoundType.Grass, SoundType.Stone, SoundType.Metal,
			SoundType.Glass, SoundType.Cloth, SoundType.Sand,
			SoundType.Snow,
		};
	}
}
