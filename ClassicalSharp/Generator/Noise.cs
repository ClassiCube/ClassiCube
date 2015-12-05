// Source from http://mrl.nyu.edu/~perlin/noise/
// Optimised form as we can always treat Z as being = 0.
// Octave and combined noise based on:
// https://github.com/UnknownShadow200/ClassicalSharp/wiki/Minecraft-Classic-map-generation-algorithm
using System;

namespace ClassicalSharp.Generator {
	
	public abstract class Noise {
		
		public abstract double Compute( double x, double y );
	}
	
	public sealed class ImprovedNoise : Noise {
		
		public ImprovedNoise( Random rnd ) {
			// make a random initial permutation based on seed,
			// instead of using fixed permutation table in original code.
			for( int i = 0; i < 256; i++ )
				p[i + 256] = p[i] = rnd.Next( 256 );
		}
		
		// TODO: need to half this maybe?
		public override double Compute( double x, double y ) {
			int xFloor = Utils.Floor( x ), yFloor = Utils.Floor( y );
			// Find unit rectangle that contains point
			int X = xFloor & 0xFF, Y = yFloor & 0xFF;
			// Find relative x, y of each point in rectangle.
			x -= Math.Floor( x ); y -= Math.Floor( y );
			
			// Compute fade curves for each of x, y.
			double u = Fade( x ), v = Fade( y );
			// Hash coordinates of the 4 rectangle corners.
			int A = p[X] + Y, AA = p[A], AB = p[A + 1],
			B = p[X + 1] + Y, BA = p[B], BB = p[B + 1];

			// and add blended results from 4 corners of rectangle.
			return Lerp(
				v,
				Lerp( u, Grad( p[AA], x, y ),
				     Grad( p[BA], x - 1, y ) ),
				Lerp( u, Grad( p[AB], x, y - 1 ),
				     Grad( p[BB], x - 1, y - 1 ) )
			);
		}
		
		static double Fade( double t ) {
			return t * t * t * (t * (t * 6 - 15) + 10);
		}
		
		static double Lerp( double t, double a, double b ) {
			return a + t * (b - a);
		}
		
		static double Grad( int hash, double x, double y ) {
			// convert low 4 bits of hash code into 12 gradient directions.
			int h = hash & 15;
			double u = h < 8 ? x : y;
			double v = h < 4 ? y : h == 12 || h == 14 ? x : 0;
			return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
		}
		
		int[] p = new int[512];
	}
	
	
	public sealed class OctaveNoise : Noise {
		
		readonly int octaves;
		readonly ImprovedNoise[] baseNoise;
		public OctaveNoise( int octaves, Random rnd ) {
			this.octaves = octaves;
			baseNoise = new ImprovedNoise[octaves];
			for( int i = 0; i < octaves; i++ )
				baseNoise[i] = new ImprovedNoise( rnd );
		}
		
		public override double Compute( double x, double y ) {
			double amplitude = 1, frequency = 1;
			double sum = 0;
			for( int i = 0; i < baseNoise.Length; i++ ) {
				sum += baseNoise[i].Compute( x * frequency, y * frequency ) * amplitude;
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
