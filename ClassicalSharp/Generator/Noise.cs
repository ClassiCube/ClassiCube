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
			// shuffle randomly using fisher-yates		
			for( int i = 0; i < 256; i++ )
				p[i] = (byte)i;
			
			for( int i = 0; i < 256; i++ ) {
				int j = rnd.Next( i, 256 );
				byte temp = p[i]; p[i] = p[j]; p[j] = temp;
			}
			for( int i = 0; i < 256; i++ )
				p[i + 256] = p[i];
		}
		
		public override double Compute( double x, double y ) {
			int xFloor = x >= 0 ? (int)x : (int)x - 1;
			int yFloor = y >= 0 ? (int)y : (int)y - 1;
			int X = xFloor & 0xFF, Y = yFloor & 0xFF;
			x -= xFloor; y -= yFloor;
			
			double u = x * x * x * (x * (x * 6 - 15) + 10); // Fade(x)
			double v = y * y * y * (y * (y * 6 - 15) + 10); // Fade(y)
			int A = p[X] + Y, B = p[X + 1] + Y;
			
			double g22 = Grad( p[p[A]], x, y ), g12 = Grad( p[p[B]], x - 1, y );
			double c1 = g22 + u * (g12 - g22);
			double g21 = Grad( p[p[A + 1]], x, y - 1 ), g11 = Grad( p[p[B + 1]], x - 1, y - 1 );
			double c2 = g21 + u * (g11 - g21);
			return c1 + v * (c2 - c1);
		}
		
		static double Grad( int hash, double x, double y ) {
			switch( hash & 15 ) {
					case 0: return x + y;
					case 1: return -x + y;
					case 2: return x - y;
					case 3: return -x - y;
					case 4: return x;
					case 5: return -x;
					case 6: return x;
					case 7: return -x;
					case 8: return y;
					case 9: return -y;
					case 10: return y;
					case 11: return -y;
					case 12: return x + y;
					case 13: return -y;
					case 14: return -x + y;
					case 15: return -y;
					default: return 0;
			}
		}
		
		byte[] p = new byte[512];
	}
	
	public sealed class OctaveNoise : Noise {
		
		readonly ImprovedNoise[] baseNoise;
		public OctaveNoise( int octaves, Random rnd ) {
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
