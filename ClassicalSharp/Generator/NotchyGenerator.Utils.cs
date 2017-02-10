// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
// Based on:
// https://github.com/UnknownShadow200/ClassicalSharp/wiki/Minecraft-Classic-map-generation-algorithm
// Thanks to Jerralish for originally reverse engineering classic's algorithm, then preparing a high level overview of the algorithm.
// I believe this process adheres to clean room reverse engineering.
using System;
using System.Collections.Generic;

namespace ClassicalSharp.Generator {
	
	public sealed partial class NotchyGenerator {
		
		void FillOblateSpheroid(int x, int y, int z, float radius, byte block) {
			int xStart = Utils.Floor(Math.Max(x - radius, 0));
			int xEnd = Utils.Floor(Math.Min(x + radius, Width - 1));
			int yStart = Utils.Floor(Math.Max(y - radius, 0));
			int yEnd = Utils.Floor(Math.Min(y + radius, Height - 1));
			int zStart = Utils.Floor(Math.Max(z - radius, 0));
			int zEnd = Utils.Floor(Math.Min(z + radius, Length - 1));
			float radiusSq = radius * radius;
			
			for (int yy = yStart; yy <= yEnd; yy++)
				for (int zz = zStart; zz <= zEnd; zz++)
					for (int xx = xStart; xx <= xEnd; xx++)
			{
				int dx = xx - x, dy = yy - y, dz = zz - z;
				if ((dx * dx + 2 * dy * dy + dz * dz) < radiusSq) {
					int index = (yy * Length + zz) * Width + xx;
					if (blocks[index] == Block.Stone)
						blocks[index] = block;
				}
			}
		}
		
		void FloodFill(int startIndex, byte block) {
			if (startIndex < 0) return; // y below map, immediately ignore
			FastIntStack stack = new FastIntStack(4);
			stack.Push(startIndex);			
			
			while (stack.Size > 0) {
				int index = stack.Pop();
				if (blocks[index] != 0) continue;				
				blocks[index] = block;
				
				int x = index % Width;
				int y = index / oneY;
				int z = (index / Width) % Length;
				if (x > 0) stack.Push(index - 1);
				if (x < Width - 1) stack.Push(index + 1);
				if (z > 0) stack.Push(index - Width);
				if (z < Length - 1) stack.Push(index + Width);
				if (y > 0) stack.Push(index - oneY);
			}
		}
		
		sealed class FastIntStack {
			public int[] Values;
			public int Size;
			
			public FastIntStack(int capacity) {
				Values = new int[capacity];
				Size = 0;
			}
			
			public int Pop() {
				return Values[--Size];
			}
			
			public void Push(int item) {
				if (Size == Values.Length) {
					int[] array = new int[Values.Length * 2];
					Buffer.BlockCopy(Values, 0, array, 0, Size * sizeof(int));
					Values = array;
				}
				Values[Size++] = item;
			}
		}
	}
	
	// Based on https://docs.oracle.com/javase/7/docs/api/java/util/Random.html
	public sealed class JavaRandom {
		
		long seed;
		const long value = 0x5DEECE66DL;
		const long mask = (1L << 48) - 1;
		
		public JavaRandom(int seed) {
			this.seed = (seed ^ value) & mask;
		}
		
		public int Next(int min, int max) { return min + Next(max - min); }
		
		public int Next(int n) {
			if ((n & -n) == n) { // i.e., n is a power of 2
				seed = (seed * value + 0xBL) & mask;
				long raw = (long)((ulong)seed >> (48 - 31));
				return (int)((n * raw) >> 31);
			}

			int bits, val;
			do {
				seed = (seed * value + 0xBL) & mask;
				bits = (int)((ulong)seed >> (48 - 31));
				val = bits % n;
			} while (bits - val + (n - 1) < 0);
			return val;
		}
		
		public float NextFloat() {
			seed = (seed * value + 0xBL) & mask;
			int raw = (int)((ulong)seed >> (48 - 24));
			return raw / ((float)(1 << 24));
		}
	}
}