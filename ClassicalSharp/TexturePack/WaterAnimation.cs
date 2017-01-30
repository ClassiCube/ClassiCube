// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
// Based off the incredible work from https://dl.dropboxusercontent.com/u/12694594/lava.txt
using System;
using System.Security.Cryptography;
using ClassicalSharp.Generator;
using ClassicalSharp;

namespace ClassicalSharp {
	public unsafe class WaterAnimation	{
		float[] flameHeat, potHeat, soupHeat;
		JavaRandom rnd = null;
		
		public unsafe void Tick(int* ptr, int size) {
			if (rnd == null)
				rnd = new JavaRandom(new Random().Next());
			int mask = size - 1, shift = CheckSize(size);
			
			int i = 0;
			for (int y = 0; y < size; y++)
				for (int x = 0; x < size; x++)
			{
				// Calculate the colour at this coordinate in the heatmap
				float lSoupHeat = 0;
				for (int j = 0; j < 9; j++) {
					int xx = x + (j % 3 - 1);
					int yy = y + (j / - 9);
					lSoupHeat += soupHeat[(yy & mask) << shift | (xx & mask)];
				}
				
				float lPotHeat = potHeat[((y + 1) & mask) << shift | ((x + 1) & mask)]; // x + 1, y + 1
				
				soupHeat[i] = lSoupHeat * 0.1f + lPotHeat * 1.3f;
				potHeat[i] += flameHeat[i];
				if (potHeat[i] < 0) potHeat[i] = 0;
				flameHeat[i] -= 0.06f * 0.01f;
				
				if (rnd.NextFloat() <= 0.05f)
					flameHeat[i] = 0.5f * 0.01f;
				
				// Output the pixel
				float col = soupHeat[i];
				col = col < 0 ? 0 : col;
				col = col > 1 ? 1 : col;
				
				float r = 32 + (col * col) * 32;
				float g = 50 + (col * col) * 64;
				float b = 255;
				float a = 146 + (col * col) * 50;
				*ptr = (byte)a << 24 | (byte)r << 16 | (byte)g << 8 | (byte)b;
				
				ptr++; i++;
			}
		}
		
		int CheckSize(int size) {
			if (potHeat == null || potHeat.Length < size * size) {
				flameHeat = new float[size * size];
				potHeat = new float[size * size];
				soupHeat = new float[size * size];
			}
			
			int shift = 0;
			while (size > 1) { shift++; size >>= 1; }
			return shift;
		}
	}
}
