// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.Events;

namespace ClassicalSharp.Map {
	
	/// <summary> Manages lighting through a simple heightmap, where each block is either in sun or shadow. </summary>
	public sealed partial class VolumeLighting : IWorldLighting {
		
		void CastInitial() {
			//initial loop for making fullbright spots
			int offset = 0, oneY = width * length, maxY = height - 1;
			byte[] blocks = game.World.blocks;
			
			for( int z = 0; z < length; z++ ) {
				for( int x = 0; x < width; x++ ) {
					int index = (maxY * oneY) + offset;
					offset++; // increase horizontal position
					int lightHeight = CalcHeightAt(x, z);
						
					for( int y = maxY; y >= 0; y-- ) {
						byte curBlock = blocks[index];
						index -= oneY; // reduce y position
						
						//if the current block is in sunlight assign the fullest sky brightness to the higher 4 bits
						if( (y - 1) > lightHeight ) { lightLevels[x, y, z] = (byte)(maxLight << 4); }
						//if the current block is fullbright assign the fullest block brightness to the higher 4 bits
						if( info.FullBright[curBlock] ) { lightLevels[x, y, z] |= maxLight; }
					}
				}
			}
		}
		
		int CalcHeightAt(int x, int z) {
			int mapIndex = ((height - 1) * length + z) * width + x;
			byte[] blocks = game.World.blocks;
			
			for (int y = height - 1; y >= 0; y--) {
				byte block = blocks[mapIndex];
				if (info.BlocksLight[block]) {
					int offset = (info.LightOffset[block] >> Side.Top) & 1;
					return y - offset;
				}
				mapIndex -= width * length;
			}
			return -10;
		}
		
		void DoPass( int pass ) {
			int index = 0;
			bool[] lightPasses = new bool[Block.Count];
			byte[] blocks = game.World.blocks;
			int maxX = width - 1, maxY = height - 1, maxZ = length - 1;
			
			for (int i = 0; i < lightPasses.Length; i++) {
				// Light passes through a block if a) doesn't block light b) block isn't full block
				lightPasses[i] =
					!game.BlockInfo.BlocksLight[i] ||
					game.BlockInfo.MinBB[i] != OpenTK.Vector3.Zero ||
					game.BlockInfo.MaxBB[i] != OpenTK.Vector3.One;
			}
			
			for( int y = 0; y < height; y++ )
				for( int z = 0; z < length; z++ )
					for( int x = 0; x < width; x++ )
			{
				byte curBlock = blocks[index];
				
				int skyLight = lightLevels[x, y, z] >> 4;
				//if the current block is not a light blocker AND the current spot is less than i
				if( !info.BlocksLight[curBlock] && skyLight == pass ) {
					//check the six neighbors sky light value,
					if( y < maxY && skyLight > (lightLevels[x, y+1, z] >> 4) ) {
						if( lightPasses[blocks[index + width * length]] ){
							lightLevels[x, y+1, z] &= 0x0F; // reset skylight bits to 0
							lightLevels[x, y+1, z] |= (byte)((skyLight - 1) << 4); // set skylight bits
						}
					}
					if( y > 0 && skyLight > (lightLevels[x, y-1, z] >> 4) ) {
						if( lightPasses[blocks[index - width * length]] ) {
							lightLevels[x, y-1, z] &= 0x0F;
							lightLevels[x, y-1, z] |= (byte)((skyLight - 1) << 4);
						}
					}
					if( x < maxX && skyLight > (lightLevels[x+1, y, z] >> 4) ) {
						if( lightPasses[blocks[index + 1]] ) {
							lightLevels[x+1, y, z] &= 0x0F;
							lightLevels[x+1, y, z] |= (byte)((skyLight - 1) << 4);
						}
					}
					if( x > 0 && skyLight > (lightLevels[x-1, y, z] >> 4) ) {
						if( lightPasses[blocks[index - 1]]) {
							lightLevels[x-1, y, z] &= 0x0F;
							lightLevels[x-1, y, z] |= (byte)((skyLight - 1) << 4);
						}
					}
					if( z < maxZ && skyLight > (lightLevels[x, y, z+1] >> 4) ) {
						if( lightPasses[blocks[index + width]]) {
							lightLevels[x, y, z+1] &= 0x0F;
							lightLevels[x, y, z+1] |= (byte)((skyLight - 1) << 4);
						}
					}
					if( z > 0 && skyLight > (lightLevels[x, y, z-1] >> 4) ) {
						if( lightPasses[blocks[index - width]]) {
							lightLevels[x, y, z-1] &= 0x0F;
							lightLevels[x, y, z-1] |= (byte)((skyLight - 1) << 4);
						}
					}
				}
				
				int blockLight = lightLevels[x, y, z] & 0x0F;
				//if the current block is not a light blocker AND the current spot is less than i
				if( (info.FullBright[curBlock] || !info.BlocksLight[curBlock]) && blockLight == pass ) {
					//check the six neighbors sky light value,
					if( y < maxY && blockLight > (lightLevels[x, y+1, z] & 0x0F) ) {
						if( lightPasses[blocks[index + width * length]] ){
							lightLevels[x, y+1, z] &= 0xF0; // reset blocklight bits to 0
							lightLevels[x, y+1, z] |= (byte)(blockLight - 1); // set blocklight bits
						}
					}
					if( y > 0 && blockLight > (lightLevels[x, y-1, z] & 0x0F) ) {
						if( lightPasses[blocks[index - width * length]] ) {
							lightLevels[x, y-1, z] &= 0xF0;
							lightLevels[x, y-1, z] |= (byte)(blockLight - 1);
						}
					}
					if( x < maxX && blockLight > (lightLevels[x+1, y, z] & 0x0F) ) {
						if( lightPasses[blocks[index + 1]] ) {
							lightLevels[x+1, y, z] &= 0xF0;
							lightLevels[x+1, y, z] |= (byte)(blockLight - 1);
						}
					}
					if( x > 0 && blockLight > (lightLevels[x-1, y, z] & 0x0F) ) {
						if( lightPasses[blocks[index - 1]] ) {
							lightLevels[x-1, y, z] &= 0xF0;
							lightLevels[x-1, y, z] |= (byte)(blockLight - 1);
						}
					}
					if( z < maxZ && blockLight > (lightLevels[x, y, z+1] & 0x0F) ) {
						if( lightPasses[blocks[index + width]] ) {
							lightLevels[x, y, z+1] &= 0xF0;
							lightLevels[x, y, z+1] |= (byte)(blockLight - 1);
						}
					}
					if( z > 0 && blockLight > (lightLevels[x, y, z-1] & 0x0F) ) {
						if( lightPasses[blocks[index - width]] ) {
							lightLevels[x, y, z-1] &= 0xF0;
							lightLevels[x, y, z-1] |= (byte)(blockLight - 1);
						}
					}
				}
				index++; // increase one coord
			}
		}
	}
}
