// Based on:
// https://github.com/UnknownShadow200/ClassicalSharp/wiki/Minecraft-Classic-map-generation-algorithm
using System;
using System;

namespace ClassicalSharp.Generator {
	
	// TODO: figure out how noise functions differ, probably based on rnd.
	public class NotchGenerator {
		
		int width, height, length;
		int waterLevel;
		
		
		short[] CreateHeightmap() {
			Noise n1 = new CombinedNoise(
				new OctaveNoise( 8 ), new OctaveNoise( 8 ) );
			Noise n2 = new CombinedNoise(
				new OctaveNoise( 8 ), new OctaveNoise( 8 ) );
			Noise n3 = new OctaveNoise( 6 );			
			int index = 0;
			short[] map = new short[width * length];
			
			for( int z = 0; z < length; z++ ) {
				for( int x = 0; x < width; x++ ) {
					float hLow = n1.Compute( x * 1.3f, z * 1.3f ) / 6 - 4;
					float hHigh = n2.Compute( x * 1.3f, z * 1.3f ) / 5 + 6;
					
					float height = n3.Compute( x, z ) > 0 ? hLow : Math.Max( hLow, hHigh );
					if( height < 0 ) height *= 0.8f;
					map[index++] = (short)(height + waterLevel);
				}
			}
		}
	}
