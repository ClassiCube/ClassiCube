using System;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {
	
	public partial class ChunkMeshBuilder {

		unsafe void FloodFill( byte* chunkPtr, int startIndex ) {
			int* stack = stackalloc int[chunkSize3];
			ushort* didFlags = stackalloc ushort[chunkSize2];
			OpenTK.MemUtils.memset( (IntPtr)didFlags, 0x00, 0, chunkSize2 * 2 );
			int index = 0;
			stack[index++] = startIndex;
			
			while( index > 0 ) {
				int bIndex = stack[index--];
				int x = (bIndex & 0x0F);
				int z = ((bIndex >> 4) & 0x0F );
				int y = ((bIndex >> 8) & 0x0F);
				
				int chunkIndex = (y + 1) * extChunkSize2 + (z + 1) * extChunkSize + (x + 1);
				byte block = chunkPtr[chunkIndex];
				if( !info.IsOpaque[block] ) {
					if( x < 15 ) stack[index++] = (y << 8) | (z << 4) | (x + 1);
					if( x > 0 ) stack[index++] =  (y << 8) | (z << 4) | (x - 1);
					if( z < 15 ) stack[index++] = (y << 8) | ((z + 1) << 4) | x;
					if( z > 0 ) stack[index++] =  (y << 8) | ((z - 1) << 4) | x;
					if( y < 15 ) stack[index++] = ((y + 1) << 8) | (z << 4) | x;
					if( y > 0 ) stack[index++] =  ((y - 1) << 8) | (z << 4) | x;
				}
			}
		}
		
		// TODO: Move to maprenderer
		/*void SimpleOcclusionCulling() {
			Vector3 p = game.LocalPlayer.EyePosition;
			for( int i = 0; i < chunks.Length; i++ ) {
				ChunkInfo chunk = chunks[i];
				chunk.Visible = true;
				int cx = chunk.CentreX >> 4;
				int cy = chunk.CentreY >> 4;
				int cz = chunk.CentreZ >> 4;
				
				int x1 = chunk.CentreX - 8, x2 = chunk.CentreX + 8;
				int y1 = chunk.CentreY - 8, y2 = chunk.CentreY + 8;
				int z1 = chunk.CentreZ - 8, z2 = chunk.CentreZ + 8;
				float minDist = float.PositiveInfinity;
				int xOffset = -1, yOffset = 0, zOffset = 0;
				
				// TODO: two axes with same distance
				minDist = DistToRecSquared( p, x1, y1, z1, x1, y2, z2 ); // left
				float rightDist = DistToRecSquared( p, x2, y1, z1, x2, y2, z2 );
				if( rightDist < minDist ) {
					minDist = rightDist; xOffset = 1;
				}
				
				float frontDist = DistToRecSquared( p, x1, y1, z1, x2, y2, z1 );
				if( frontDist < minDist ) {
					minDist = frontDist; xOffset = 0; zOffset = -1;
				}
				
				float backDist = DistToRecSquared( p, x1, y1, z2, x2, y2, z2 );
				if( backDist < minDist ) {
					minDist = backDist; xOffset = 0; zOffset = 1;
				}
				
				float bottomDist = DistToRecSquared( p, x1, y1, z1, x2, y1, z2 );
				if( bottomDist < minDist ) {
					minDist = bottomDist; xOffset = 0; zOffset = 0; yOffset = -1;
				}
				
				float topDist = DistToRecSquared( p, x1, y2, z1, x2, y2, z2 );
				if( topDist < minDist ) {
					minDist = topDist; xOffset = 0; zOffset = 0; yOffset = -1;
				}
				
				if( (cx + xOffset) >= 0 && (cy + yOffset) >= 0 && (cz + zOffset) >= 0 &&
				   (cx + xOffset) < width && (cy + yOffset) < height && (cz + zOffset) < length ) {
					chunk.Visible = false;
				}
			}
			chunks[0].Visible = true;
		}
		
		static float DistToRecSquared( Vector3 p, int x1, int y1, int z1, int x2, int y2, int z2 ) {
			float dx = Math.Max( x1 - p.X, Math.Max( 0, p.X - x2 ) );
			float dy = Math.Max( y1 - p.Y, Math.Max( 0, p.Y - y2 ) );
			float dz = Math.Max( z1 - p.Z, Math.Max( 0, p.Z - z2 ) );
			return dx * dx + dy * dy + dz * dz;
		}*/
	}
}