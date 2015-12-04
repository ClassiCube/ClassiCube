// Based on:
// https://github.com/UnknownShadow200/ClassicalSharp/wiki/Minecraft-Classic-map-generation-algorithm
// Thanks to Jerralish for originally reverse engineering classic's algorithm, then preparing a high level overview of the algorithm.
// I believe this process adheres to clean room reverse engineering.
using System;

namespace ClassicalSharp.Generator {
	
	// TODO: figure out how noise functions differ, probably based on rnd.
	public class NotchyGenerator {
		
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
					double hLow = n1.Compute( x * 1.3f, z * 1.3f ) / 6 - 4;
					double hHigh = n2.Compute( x * 1.3f, z * 1.3f ) / 5 + 6;
					
					double height = n3.Compute( x, z ) > 0 ? hLow : Math.Max( hLow, hHigh );
					if( height < 0 ) height *= 0.8f;
					map[index++] = (short)(height + waterLevel);
				}
			}
			return map;
		}
	}
}