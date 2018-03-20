// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using OpenTK;
using BlockRaw = System.Byte;

namespace ClassicalSharp.Generator {
	
	public unsafe sealed class FlatGrassGenerator : IMapGenerator {
		
		public override string GeneratorName { get { return "Flatgrass"; } }
		
		public override BlockRaw[] Generate() {
			BlockRaw[] map = new BlockRaw[Width * Height * Length];
			
			fixed (BlockRaw* ptr = map) {
				CurrentState = "Setting dirt blocks";
				MapSet(ptr, 0, Height / 2 - 2, Block.Dirt);
				
				CurrentState = "Setting grass blocks";
				MapSet(ptr, Height / 2 - 1, Height / 2 - 1, Block.Grass);
			}
			return map;
		}
		
		unsafe void MapSet(BlockRaw* ptr, int yStart, int yEnd, BlockRaw block) {
			yStart = Math.Max(yStart, 0); yEnd = Math.Max(yEnd, 0);
			int startIndex = yStart * Length * Width;
			int endIndex = (yEnd * Length + (Length - 1)) * Width + (Width - 1);
			int count = (endIndex - startIndex) + 1, offset = 0;
			
			while (offset < count) {
				int bytes = Math.Min(count - offset, Width * Length);
				MemUtils.memset((IntPtr)ptr, block, startIndex + offset, bytes);				
				offset += bytes;
				CurrentProgress = (float)offset / count;
			}
		}
	}
}
