// Source from http://mrl.nyu.edu/~perlin/noise/
// Optimised form as we can always treat Z as being = 0.
// Octave and combined noise based on:
// https://github.com/UnknownShadow200/ClassicalSharp/wiki/Minecraft-Classic-map-generation-algorithm
using System;

namespace ClassicalSharp.Generator {
	
	// TODO:L calculate based on seed
	public abstract class Noise {
		
		public abstract double Compute( double x, double y );
	}
	
	public sealed class ImprovedNoise : Noise {
		
		public override double Compute( double x, double y ) {
			// Find unit rectangle that contains point
			int X = (int)Math.Floor( x ) & 255;
			int Y = (int)Math.Floor( y ) & 255;
			// Find relative x, y of each point in rectangle.
			x -= Math.Floor( x );
			y -= Math.Floor( y );
			
			// Compute fade curves for each of x, y.
			double u = Fade( x ), v = Fade( y );
			// Hash coordinates of the 4 rectangle corners.
			int A = p[X] + Y, AA = p[A], AB = p[A + 1],
			B = p[X + 1] + Y, BA = p[B], BB = p[B + 1];

			// and add blended results from 4 corners of rectangle.
			return Lerp(
				v,
				Lerp( u, Grad( p[AA], x, y, 0 ),
				     Grad( p[BA], x - 1, y, 0 ) ),
				Lerp( u, Grad( p[AB], x, y - 1, 0 ),
				     Grad( p[BB], x - 1, y - 1, 0 ) )
			);
		}
		
		static double Fade( double t ) {
			return t * t * t * (t * (t * 6 - 15) + 10);
		}
		
		static double Lerp( double t, double a, double b ) {
			return a + t * (b - a);
		}
		
		static double Grad( int hash, double x, double y, double z ) {
			// convert low 4 bits of hash code into 12 gradient directions.
			int h = hash & 15;
			double u = h < 8 ? x : y;
			double v = h < 4 ? y : h == 12 || h == 14 ? x : z;
			return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
		}
		
		static int[] p = new int[512];
		static int[] permutation = { 151,160,137,91,90,15,
			131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
			190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
			88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
			77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
			102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
			135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
			5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
			223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
			129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
			251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
			49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
			138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
		};
		
		static ImprovedNoise() {
			for( int i = 0; i < 256; i++ )
				p[256+i] = p[i] = permutation[i];
		}
	}
	
	
	public sealed class OctaveNoise : Noise {
		
		readonly int octaves;
		readonly ImprovedNoise baseNoise;
		public OctaveNoise( int octaves ) {
			this.octaves = octaves;
			baseNoise = new ImprovedNoise();
		}
		
		public override double Compute( double x, double y ) {
			double amplitude = 1, frequency = 1;
			double sum = 0;
			for( int i = 0; i < octaves; i++ ) {
				sum += baseNoise.Compute( x * frequency, y * frequency ) * amplitude;
				amplitude *= 2;
				frequency /= 2;
			}
			return sum;
		}
	}
	
	public sealed class CombinedNoise : Noise {
		
		readonly Noise noise1, noise2;
		public CombinedNoise( Noise noise1, Noise noise2 ) {
			this.noise1 = noise1;
			this.noise2 = noise2;
		}
		
		public override double Compute( double x, double y ) {
			double offset = noise2.Compute( x, y );
			return noise1.Compute( x + offset, y );
		}
	}
}
