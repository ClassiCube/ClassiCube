// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
// Based off the incredible work from https://dl.dropboxusercontent.com/u/12694594/lava.txt
// mirroed at https://github.com/UnknownShadow200/ClassicalSharp/wiki/Minecraft-Classic-lava-animation-algorithm.
using System;
using ClassicalSharp.Generator;

namespace ClassicalSharp {
	
	public abstract class LiquidAnimation {
		protected float[] flameHeat, potHeat, soupHeat;
		protected JavaRandom rnd;
		
		protected int CheckSize(int size) {
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
	
	public unsafe class LavaAnimation : LiquidAnimation {

		public void Tick(int* ptr, int size) {
			if (rnd == null)
				rnd = new JavaRandom(new Random().Next());
			int mask = size - 1, shift = CheckSize(size);
			
			int i = 0;
			for (int y = 0; y < size; y++)
				for (int x = 0; x < size; x++)
			{
				// Calculate the colour at this coordinate in the heatmap
				
				int xx = x + (int)(1.2 * Math.Sin(y * 22.5 * Utils.Deg2Rad));
				int yy = y + (int)(1.2 * Math.Sin(x * 22.5 * Utils.Deg2Rad));
				float lSoupHeat =
					soupHeat[((yy - 1) & mask) << shift | ((xx - 1) & mask)] +
					soupHeat[((yy - 1) & mask) << shift | (xx & mask)      ] +
					soupHeat[((yy - 1) & mask) << shift | ((xx + 1) & mask)] +
					
					soupHeat[(yy & mask) << shift       | ((xx - 1) & mask)] +
					soupHeat[(yy & mask) << shift       | (xx & mask)      ] +
					soupHeat[(yy & mask) << shift       | ((xx + 1) & mask)] +
					
					soupHeat[((yy + 1) & mask) << shift | ((xx - 1) & mask)] +
					soupHeat[((yy + 1) & mask) << shift | (xx & mask)      ] +
					soupHeat[((yy + 1) & mask) << shift | ((xx + 1) & mask)];
				
				float lPotHeat = 
					potHeat[i] +                                          // x    , y
					potHeat[y << shift | ((x + 1) & mask)] +              // x + 1, y
					potHeat[((y + 1) & mask) << shift | x] +              // x    , y + 1
					potHeat[((y + 1) & mask) << shift | ((x + 1) & mask)];// x + 1, y + 1
				
				soupHeat[i] = lSoupHeat * 0.1f + lPotHeat * 0.2f;
				potHeat[i] += flameHeat[i];
				if (potHeat[i] < 0) potHeat[i] = 0;
				flameHeat[i] -= 0.06f * 0.01f;
				
				if (rnd.NextFloat() <= 0.005f)
					flameHeat[i] = 1.5f * 0.01f;
				
				// Output the pixel
				float col = 2 * soupHeat[i];
				col = col < 0 ? 0 : col;
				col = col > 1 ? 1 : col;
				
				float r = col * 100 + 155;
				float g = col * col * 255;
				float b = col * col * col * col * 128;
				*ptr = 255 << 24 | (byte)r << 16 | (byte)g << 8 | (byte)b;
				
				ptr++; i++;
			}
		}
	}
	
	// Written by cybertoon, big thanks!
	public unsafe class WaterAnimation : LiquidAnimation {
		
		public void Tick(int* ptr, int size) {
			if (rnd == null)
				rnd = new JavaRandom(new Random().Next());
			int mask = size - 1, shift = CheckSize(size);
			
			int i = 0;
			for (int y = 0; y < size; y++)
				for (int x = 0; x < size; x++)
			{
				// Calculate the colour at this coordinate in the heatmap
				float lSoupHeat =
					soupHeat[y << shift | ((x - 1) & mask)] +
					soupHeat[y << shift | x               ] +
					soupHeat[y << shift | ((x + 1) & mask)];

				soupHeat[i] = lSoupHeat / 3.3f + potHeat[i] * 0.8f;
				potHeat[i] += flameHeat[i] * 0.05f;
				if (potHeat[i] < 0) potHeat[i] = 0;
				flameHeat[i] -= 0.1f;
				
				if (rnd.NextFloat() <= 0.05f)
					flameHeat[i] = 0.5f;
				
				// Output the pixel
				float col = soupHeat[i];
				col = col < 0 ? 0 : col;
				col = col > 1 ? 1 : col;
				col = col * col;
				
				float r = 32 + col * 32;
				float g = 50 + col * 64;
				float a = 146 + col * 50;
				
				*ptr = (byte)a << 24 | (byte)r << 16 | (byte)g << 8 | 255;
				
				ptr++; i++;
			}
		}
	}
}