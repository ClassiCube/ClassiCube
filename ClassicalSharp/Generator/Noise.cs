// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
// Source from http://mrl.nyu.edu/~perlin/noise/
// Optimised form as we can always treat Z as being = 0.
// Octave and combined noise based on:
// https://github.com/UnknownShadow200/ClassicalSharp/wiki/Minecraft-Classic-map-generation-algorithm
using System;

namespace ClassicalSharp.Generator {

	public sealed class ImprovedNoise {
		
		public ImprovedNoise(JavaRandom rnd) {
			// shuffle randomly using fisher-yates
			for (int i = 0; i < 256; i++)
				p[i] = (byte)i;
			
			for (int i = 0; i < 256; i++) {
				int j = rnd.Next(i, 256);
				byte temp = p[i]; p[i] = p[j]; p[j] = temp;
			}
			for (int i = 0; i < 256; i++)
				p[i + 256] = p[i];
		}
		
		public double Compute(double x, double y) {
			int xFloor = x >= 0 ? (int)x : (int)x - 1;
			int yFloor = y >= 0 ? (int)y : (int)y - 1;
			int X = xFloor & 0xFF, Y = yFloor & 0xFF;
			x -= xFloor; y -= yFloor;
			
			double u = x * x * x * (x * (x * 6 - 15) + 10); // Fade(x)
			double v = y * y * y * (y * (y * 6 - 15) + 10); // Fade(y)
			int A = p[X] + Y, B = p[X + 1] + Y;
			
			// Normally, calculating Grad involves a function call. However, we can directly pack this table
			// (since each value indicates either -1, 0 1) into a set of bit flags. This way we avoid needing 
			// to call another function that performs branching
			const int xFlags = 0x46552222, yFlags = 0x2222550A;
			
			int hash = (p[p[A]] & 0xF) << 1; 
			double g22 = (((xFlags >> hash) & 3) - 1) * x + (((yFlags >> hash) & 3) - 1) * y; // Grad(p[p[A], x, y)
			hash = (p[p[B]] & 0xF) << 1; 
			double g12 = (((xFlags >> hash) & 3) - 1) * (x - 1) + (((yFlags >> hash) & 3) - 1) * y; // Grad(p[p[B], x - 1, y)
			double c1 = g22 + u * (g12 - g22);
			
			hash = (p[p[A + 1]] & 0xF) << 1; 
			double g21 = (((xFlags >> hash) & 3) - 1) * x + (((yFlags >> hash) & 3) - 1) * (y - 1); // Grad(p[p[A + 1], x, y - 1)
			hash = (p[p[B + 1]] & 0xF) << 1; 
			double g11 = (((xFlags >> hash) & 3) - 1) * (x - 1) + (((yFlags >> hash) & 3) - 1) * (y - 1); // Grad(p[p[B + 1], x - 1, y - 1)
			double c2 = g21 + u * (g11 - g21);
			
			return c1 + v * (c2 - c1);
		}
		
		byte[] p = new byte[512];
	}
	
	public sealed class OctaveNoise {
		
		readonly ImprovedNoise[] baseNoise;
		public OctaveNoise(int octaves, JavaRandom rnd) {
			baseNoise = new ImprovedNoise[octaves];
			for (int i = 0; i < octaves; i++)
				baseNoise[i] = new ImprovedNoise(rnd);
		}
		
		public double Compute(double x, double y) {
			double amplitude = 1, frequency = 1;
			double sum = 0;
			for (int i = 0; i < baseNoise.Length; i++) {
				sum += baseNoise[i].Compute(x * frequency, y * frequency) * amplitude;
				amplitude *= 2.0;
				frequency *= 0.5;
			}
			return sum;
		}
	}
	
	public sealed class CombinedNoise {
		
		readonly OctaveNoise noise1, noise2;
		public CombinedNoise(OctaveNoise noise1, OctaveNoise noise2) {
			this.noise1 = noise1;
			this.noise2 = noise2;
		}
		
		public double Compute(double x, double y) {
			double offset = noise2.Compute(x, y);
			return noise1.Compute(x + offset, y);
		}
	}
}
