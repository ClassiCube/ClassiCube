// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
// Based on:
// https://github.com/UnknownShadow200/ClassicalSharp/wiki/Minecraft-Classic-map-generation-algorithm
// Thanks to Jerralish for originally reverse engineering classic's algorithm, then preparing a high level overview of the algorithm.
// I believe this process adheres to clean room reverse engineering.
using System;
using System.Collections.Generic;

namespace ClassicalSharp.Generator {
	
	// TODO: figure out how noise functions differ, probably based on rnd.
	public sealed partial class NotchyGenerator {
		
		void FillOblateSpheroid( int x, int y, int z, float radius, byte block ) {
			int xStart = Utils.Floor( Math.Max( x - radius, 0 ) );
			int xEnd = Utils.Floor( Math.Min( x + radius, width - 1 ) );
			int yStart = Utils.Floor( Math.Max( y - radius, 0 ) );
			int yEnd = Utils.Floor( Math.Min( y + radius, height - 1 ) );
			int zStart = Utils.Floor( Math.Max( z - radius, 0 ) );
			int zEnd = Utils.Floor( Math.Min( z + radius, length - 1 ) );
			float radiusSq = radius * radius;
			
			for( int yy = yStart; yy <= yEnd; yy++ )
				for( int zz = zStart; zz <= zEnd; zz++ )
					for( int xx = xStart; xx <= xEnd; xx++ )
			{
				int dx = xx - x, dy = yy - y, dz = zz - z;
				if( (dx * dx + 2 * dy * dy + dz * dz) < radiusSq ) {
					int index = (yy * length + zz) * width + xx;
					if( blocks[index] == (byte)Block.Stone )
						blocks[index] = block;
				}
			}
		}
		
		void FloodFill( int startIndex, byte block ) {
			FastIntStack stack = new FastIntStack( 4 );
			stack.Push( startIndex );
			while( stack.Size > 0 ) {
				int index = stack.Pop();
				if( blocks[index] == 0 ) {
					blocks[index] = block;
					
					int x = index % width;
					int y = index / oneY;
					int z = (index / width) % length;
					if( x > 0 ) stack.Push( index - 1 );
					if( x < width - 1 ) stack.Push( index + 1 );
					if( z > 0 ) stack.Push( index - width );
					if( z < length - 1 ) stack.Push( index + width );
					if( y > 0 ) stack.Push( index - oneY );
				}
			}
		}
		
		sealed class FastIntStack {		
			public int[] Values;
			public int Size;
			
			public FastIntStack( int capacity ) {
				Values = new int[capacity];
				Size = 0;
			}
			
			public int Pop() {
				return Values[--Size];
			}
			
			public void Push( int item ) {
				if( Size == Values.Length ) {
					int[] array = new int[Values.Length * 2];
					Buffer.BlockCopy( Values, 0, array, 0, Size * sizeof(int) );
					Values = array;
				}
				Values[Size++] = item;
			}
		}
	}
}