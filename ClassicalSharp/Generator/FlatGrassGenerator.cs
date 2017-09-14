// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using OpenTK;

#if USE16_BIT
using BlockID = System.UInt16;
#else
using BlockID = System.Byte;
#endif

namespace ClassicalSharp.Generator {
	
	public unsafe sealed class FlatGrassGenerator : IMapGenerator {
		
		public override string GeneratorName { get { return "Flatgrass"; } }
		
		public override BlockID[] Generate() {
			BlockID[] map = new BlockID[Width * Height * Length];
			
			fixed(BlockID* ptr = map) {
				CurrentState = "Setting dirt blocks";
				MapSet(ptr, 0, Height / 2 - 2, Block.Dirt);
				
				CurrentState = "Setting grass blocks";
				MapSet(ptr, Height / 2 - 1, Height / 2 - 1, Block.Grass);
			}
			return map;
		}
		
		unsafe void MapSet(BlockID* ptr, int yStart, int yEnd, BlockID block) {
			yStart = Math.Max(yStart, 0); yEnd = Math.Max(yEnd, 0);
			int startIndex = yStart * Length * Width;
			int endIndex = (yEnd * Length + (Length - 1)) * Width + (Width - 1);
			int count = (endIndex - startIndex) + 1, offset = 0;
			
			while (offset < count) {
				int bytes = Math.Min(count - offset, Width * Length) * sizeof(BlockID);
				#if USE16_BIT
				MemUtils.memset((IntPtr)ptr, (byte)block, startIndex + offset, bytes);
				#else
				MemUtils.memset((IntPtr)ptr, block, startIndex + offset, bytes);
				#endif
				
				offset += bytes;
				CurrentProgress = (float)offset / count;
			}
		}
	}
}
