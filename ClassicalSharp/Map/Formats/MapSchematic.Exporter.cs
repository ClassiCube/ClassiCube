// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.IO;
using System.IO.Compression;

namespace ClassicalSharp.Map {

	public sealed class MapSchematicExporter : IMapFormatExporter {
		
		public void Save(Stream stream, Game game) {
			using (GZipStream wrapper = new GZipStream(stream, CompressionMode.Compress)) {
				BinaryWriter writer = new BinaryWriter(wrapper);
				NbtFile nbt = new NbtFile(writer);
				World map = game.World;
				
				nbt.Write(NbtTagType.Compound); nbt.Write("Schematic");
				
				nbt.Write(NbtTagType.String);
				nbt.Write("Materials"); nbt.Write("Classic");
				
				nbt.Write(NbtTagType.Int16);
				nbt.Write("Width"); nbt.WriteInt16((short)map.Width);
				
				nbt.Write(NbtTagType.Int16);
				nbt.Write("Height"); nbt.WriteInt16((short)map.Height);
				
				nbt.Write(NbtTagType.Int16);
				nbt.Write("Length"); nbt.WriteInt16((short)map.Length);
				
				WriteBlocks(nbt, map.blocks);
				
				WriteBlockData(nbt, map.blocks);
				
				nbt.Write(NbtTagType.List);
				nbt.Write("Entities");
				nbt.Write(NbtTagType.Compound); nbt.WriteInt32(0);
				
				nbt.Write(NbtTagType.List);
				nbt.Write("TileEntities");
				nbt.Write(NbtTagType.Compound); nbt.WriteInt32(0);
				
				nbt.Write(NbtTagType.End);
			}
		}
		
		
		void WriteBlocks(NbtFile nbt, byte[] blocks) {
			const int chunkSize = 64 * 1024 * 32;
			nbt.Write(NbtTagType.Int8Array);
			nbt.Write("Blocks");
			nbt.WriteInt32(blocks.Length);
			
			for (int i = 0; i < blocks.Length; i += chunkSize) {
				int count = Math.Min(chunkSize, blocks.Length - i);
				nbt.WriteBytes(blocks, i, count);
			}
		}
		
		void WriteBlockData(NbtFile nbt, byte[] blocks) {
			const int chunkSize = 64 * 1024;
			byte[] chunk = new byte[chunkSize];
			nbt.Write(NbtTagType.Int8Array);
			nbt.Write("Data");
			nbt.WriteInt32(blocks.Length);
			
			for (int i = 0; i < blocks.Length; i += chunkSize) {
				// All 0 so we can skip this.
				int count = Math.Min(chunkSize, blocks.Length - i);
				nbt.WriteBytes(chunk, 0, count);
			}
		}
	}
}