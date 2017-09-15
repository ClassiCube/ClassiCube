// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using OpenTK;

namespace ClassicalSharp.Network.Protocols {

	/// <summary> Implements the packets for BlockDefinitions extension in CPE. </summary>
	public sealed class CPEProtocolBlockDefs : IProtocol {
		
		public CPEProtocolBlockDefs(Game game) : base(game) { }
		
		public override void Init() { Reset(); }
		
		public override void Reset() {
			if (!game.UseCPE || !game.UseCustomBlocks) return;
			net.Set(Opcode.CpeDefineBlock, HandleDefineBlock, 80);
			net.Set(Opcode.CpeRemoveBlockDefinition, HandleRemoveBlockDefinition, 2);
			net.Set(Opcode.CpeDefineBlockExt, HandleDefineBlockExt, 85);
		}
		
		internal void HandleDefineBlock() {
			byte id = HandleDefineBlockCommonStart(reader, false);
			
			byte shape = reader.ReadUInt8();
			if (shape > 0 && shape <= 16)
				BlockInfo.MaxBB[id].Y = shape / 16f;
			
			HandleDefineBlockCommonEnd(reader, shape, id);
			// Update sprite BoundingBox if necessary
			if (BlockInfo.Draw[id] == DrawType.Sprite) {
				using (FastBitmap dst = new FastBitmap(game.TerrainAtlas.AtlasBitmap, true, true))
					BlockInfo.RecalculateBB(id, dst);
			}
		}
		
		void HandleRemoveBlockDefinition() {
			byte block = reader.ReadUInt8();
			bool didBlockLight = BlockInfo.BlocksLight[block];
			
			BlockInfo.ResetBlockProps(block);
			OnBlockUpdated(block, didBlockLight);
			BlockInfo.UpdateCulling(block);
			
			game.Inventory.Reset(block);
			if (block < Block.CpeCount) {
				game.Inventory.AddDefault(block);
			}
			
			BlockInfo.DefinedCustomBlocks[block >> 5] &= ~(1u << (block & 0x1F));
			game.Events.RaiseBlockDefinitionChanged();
		}
		
		void OnBlockUpdated(byte block, bool didBlockLight) {
			if (game.World.blocks == null) return;
			
			// Need to refresh lighting when a block's light blocking state changes
			if (BlockInfo.BlocksLight[block] != didBlockLight) {
				game.Lighting.Refresh();
			}
		}
		
		void HandleDefineBlockExt() {
			if (!game.UseCustomBlocks) {
				net.SkipPacketData(Opcode.CpeDefineBlockExt); return;
			}
			byte block = HandleDefineBlockCommonStart(reader, net.cpeData.blockDefsExtVer >= 2);
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
		
		byte HandleDefineBlockCommonStart(NetReader reader, bool uniqueSideTexs) {
			byte block = reader.ReadUInt8();
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
		
		void HandleDefineBlockCommonEnd(NetReader reader, byte shape, byte block) {
			byte blockDraw = reader.ReadUInt8();
			if (shape == 0) blockDraw = DrawType.Sprite;
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
			BlockInfo.DefinedCustomBlocks[block >> 5] |= (1u << (block & 0x1F));
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
		
		internal static SoundType[] stepSnds, breakSnds;
		static CPEProtocolBlockDefs() {
			stepSnds = new SoundType[10];
			breakSnds = new SoundType[10];
			stepSnds[0] = SoundType.None; breakSnds[0] = SoundType.None;
			stepSnds[1] = SoundType.Wood; breakSnds[1] = SoundType.Wood;
			stepSnds[2] = SoundType.Gravel; breakSnds[2] = SoundType.Gravel;
			stepSnds[3] = SoundType.Grass; breakSnds[3] = SoundType.Grass;
			stepSnds[4] = SoundType.Stone; breakSnds[4] = SoundType.Stone;
			stepSnds[5] = SoundType.Metal; breakSnds[5] = SoundType.Metal;
			stepSnds[6] = SoundType.Stone; breakSnds[6] = SoundType.Glass;
			stepSnds[7] = SoundType.Cloth; breakSnds[7] = SoundType.Cloth;
			stepSnds[8] = SoundType.Sand; breakSnds[8] = SoundType.Sand;
			stepSnds[9] = SoundType.Snow; breakSnds[9] = SoundType.Snow;
		}
	}
}
