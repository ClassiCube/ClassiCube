﻿// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using OpenTK;

namespace ClassicalSharp.Generator {
	
	public unsafe sealed class FlatGrassGenerator : IMapGenerator {
		
		public override string GeneratorName { get { return "Flatgrass"; } }
		
		int width, length;
		public override byte[] Generate(Game game, int width, int height, int length, int seed, bool winter) {
			byte[] map = new byte[width * height * length];
			this.width = width;
			this.length = length;
			bool winterMode = winter; // TODO: add winterMode to FlatGrass.
			fixed(byte* ptr = map) {
				CurrentState = "Setting dirt blocks";
				MapSet(ptr, 0, height / 2 - 2, Block.Dirt);
				
				CurrentState = "Setting grass blocks";
				MapSet(ptr, height / 2 - 1, height / 2 - 1, Block.Grass);
			}
			return map;
		}
		
		unsafe void MapSet(byte* ptr, int yStart, int yEnd, byte block) {
			yStart = Math.Max(yStart, 0); yEnd = Math.Max(yEnd, 0);			
			int startIndex = yStart * length * width;
			int endIndex = (yEnd * length + (length - 1)) * width + (width - 1);
			int count = (endIndex - startIndex) + 1, offset = 0;
			
			while (offset < count) {
				int bytes = Math.Min(count - offset, width * length);
				MemUtils.memset((IntPtr)ptr, block, startIndex + offset, bytes);
				offset += bytes;
				CurrentProgress = (float)offset / count;
			}
		}
	}
}
