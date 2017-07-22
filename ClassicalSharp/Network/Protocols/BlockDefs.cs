// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using OpenTK;

namespace ClassicalSharp.Network.Protocols {

	/// <summary> Implements the packets for BlockDefinitions extension in CPE. </summary>
	public sealed class CPEProtocolBlockDefs : IProtocol {
		
		public CPEProtocolBlockDefs(Game game) : base(game) { }
		
		public override void Init() { Reset(); }
		
		public override void Reset() {
			if (!game.UseCPE || !game.AllowCustomBlocks) return;
			net.Set(Opcode.CpeDefineBlock, HandleDefineBlock, 80);
			net.Set(Opcode.CpeRemoveBlockDefinition, HandleRemoveBlockDefinition, 2);
			net.Set(Opcode.CpeDefineBlockExt, HandleDefineBlockExt, 85);
		}
		
		internal void HandleDefineBlock() {
			byte id = HandleDefineBlockCommonStart(reader, false);
			BlockInfo info = game.BlockInfo;
			
			byte shape = reader.ReadUInt8();
			if (shape > 0 && shape <= 16)
				info.MaxBB[id].Y = shape / 16f;
			
			HandleDefineBlockCommonEnd(reader, shape, id);
			// Update sprite BoundingBox if necessary
			if (info.Draw[id] == DrawType.Sprite) {
				using (FastBitmap dst = new FastBitmap(game.TerrainAtlas.AtlasBitmap, true, true))
					info.RecalculateBB(id, dst);
			}
		}
		
		void HandleRemoveBlockDefinition() {
			byte block = reader.ReadUInt8();
			BlockInfo info = game.BlockInfo;
			bool didBlockLight = info.BlocksLight[block];
			
			
			info.ResetBlockProps(block);
			OnBlockUpdated(block, didBlockLight);			
			info.UpdateCulling(block);
			
			info.DefinedCustomBlocks[block >> 5] &= ~(1u << (block & 0x1F));
			game.Events.RaiseBlockDefinitionChanged();
		}
		
		void OnBlockUpdated(byte block, bool didBlockLight) {
			if (game.World.blocks == null) return;
			
			// Need to refresh lighting when a block's light blocking state changes
			if (game.BlockInfo.BlocksLight[block] != didBlockLight) {
				game.Lighting.Refresh();
			}			
		}
		
		void HandleDefineBlockExt() {
			if (!game.AllowCustomBlocks) {
				net.SkipPacketData(Opcode.CpeDefineBlockExt); return;
			}
			byte block = HandleDefineBlockCommonStart(reader, 
			                                       net.cpeData.blockDefsExtVer >= 2);
			BlockInfo info = game.BlockInfo;
			Vector3 min, max;
			
			min.X = reader.ReadUInt8() / 16f; Utils.Clamp(ref min.X, 0, 15/16f);
			min.Y = reader.ReadUInt8() / 16f; Utils.Clamp(ref min.Y, 0, 15/16f);
			min.Z = reader.ReadUInt8() / 16f; Utils.Clamp(ref min.Z, 0, 15/16f);
			max.X = reader.ReadUInt8() / 16f; Utils.Clamp(ref max.X, 1/16f, 1);
			max.Y = reader.ReadUInt8() / 16f; Utils.Clamp(ref max.Y, 1/16f, 1);
			max.Z = reader.ReadUInt8() / 16f; Utils.Clamp(ref max.Z, 1/16f, 1);
			
			info.MinBB[block] = min;
			info.MaxBB[block] = max;
			HandleDefineBlockCommonEnd(reader, 1, block);
		}
		
		byte HandleDefineBlockCommonStart(NetReader reader, bool uniqueSideTexs) {
			byte block = reader.ReadUInt8();
			BlockInfo info = game.BlockInfo;
			bool didBlockLight = info.BlocksLight[block];
			info.ResetBlockProps(block);
			
			info.Name[block] = reader.ReadString();
			info.SetCollide(block, reader.ReadUInt8());
			
			info.SpeedMultiplier[block] = (float)Math.Pow(2, (reader.ReadUInt8() - 128) / 64f);
			info.SetTex(reader.ReadUInt8(), Side.Top, block);
			if (uniqueSideTexs) {
				info.SetTex(reader.ReadUInt8(), Side.Left, block);
				info.SetTex(reader.ReadUInt8(), Side.Right, block);
				info.SetTex(reader.ReadUInt8(), Side.Front, block);
				info.SetTex(reader.ReadUInt8(), Side.Back, block);
			} else {
				info.SetSide(reader.ReadUInt8(), block);
			}
			info.SetTex(reader.ReadUInt8(), Side.Bottom, block);
			
			info.BlocksLight[block] = reader.ReadUInt8() == 0;
			OnBlockUpdated(block, didBlockLight);
			
			byte sound = reader.ReadUInt8();
			if (sound < breakSnds.Length) {
				info.StepSounds[block] = stepSnds[sound];
				info.DigSounds[block] = breakSnds[sound];
			}
			info.FullBright[block] = reader.ReadUInt8() != 0;
			return block;
		}
		
		void HandleDefineBlockCommonEnd(NetReader reader, byte shape, byte block) {
			BlockInfo info = game.BlockInfo;
			byte blockDraw = reader.ReadUInt8();
			if (shape == 0) blockDraw = DrawType.Sprite;
			info.LightOffset[block] = info.CalcLightOffset(block);
			
			byte fogDensity = reader.ReadUInt8();
			info.FogDensity[block] = fogDensity == 0 ? 0 : (fogDensity + 1) / 128f;
			info.FogColour[block] = new FastColour(
				reader.ReadUInt8(), reader.ReadUInt8(), reader.ReadUInt8());
			info.Tinted[block] = info.FogColour[block] != FastColour.Black && info.Name[block].IndexOf('#') >= 0;
			
			info.SetBlockDraw(block, blockDraw);
			info.CalcRenderBounds(block);
			info.UpdateCulling(block);
			
			info.DefinedCustomBlocks[block >> 5] |= (1u << (block & 0x1F));
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
