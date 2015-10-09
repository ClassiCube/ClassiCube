using System;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp {
	
	public partial class MapRenderer : IDisposable {
		
		void SimpleOcclusionCulling() { // TODO: broken
			Vector3 p = game.LocalPlayer.EyePosition;
			Vector3I chunkLoc = Vector3I.Floor( p );
			Utils.Clamp( ref chunkLoc.X, 0, game.Map.Width - 1 );
			Utils.Clamp( ref chunkLoc.Y, 0, game.Map.Height - 1 );
			Utils.Clamp( ref chunkLoc.Z, 0, game.Map.Length- 1 );
			
			int cx = chunkLoc.X >> 4;
			int cy = chunkLoc.Y >> 4;
			int cz = chunkLoc.Z >> 4;
			
			ChunkInfo chunkIn = unsortedChunks[cx + chunksX * (cy + cz * chunksY)];
			byte chunkInFlags = chunkIn.OcclusionFlags;
			chunkIn.OcclusionFlags = 0;
			
			ChunkQueue queue = new ChunkQueue( chunksX * chunksY * chunksZ );
			for( int i = 0; i < chunks.Length; i++ ) {
				chunks[i].Visited = false;
				chunks[i].Occluded = false;
				chunks[i].VisibilityFlags = 0;
			}
			
			chunkIn.Visited = true;
			QueueChunk( cx - 1, cy, cz, queue );
			QueueChunk( cx + 1, cy, cz, queue );
			QueueChunk( cx, cy - 1, cz, queue );
			QueueChunk( cx, cy + 1, cz, queue );
			QueueChunk( cx, cy, cz - 1, queue );
			QueueChunk( cx, cy, cz + 1, queue );
			
			ProcessQueue( chunkIn, queue );
			chunkIn.OcclusionFlags = chunkInFlags;
		}
		
		void ProcessQueue( ChunkInfo src, ChunkQueue queue ) {
			Vector3I p = new Vector3I( src.CentreX, src.CentreY, src.CentreZ );
			while( queue.Size > 0 ) {
				ChunkInfo chunk = queue.Dequeue();				
				chunk.VisibilityFlags = chunk.OcclusionFlags;
				int x1 = chunk.CentreX - 8, x2 = chunk.CentreX + 8;
				int y1 = chunk.CentreY - 8, y2 = chunk.CentreY + 8;
				int z1 = chunk.CentreZ - 8, z2 = chunk.CentreZ + 8;
				int cx = chunk.CentreX >> 4;
				int cy = chunk.CentreY >> 4;
				int cz = chunk.CentreZ >> 4;
				
				int xOffset, yOffset, zOffset;
				int dx = Math.Max( x1 - p.X, Math.Max( 0, p.X - x2 ) );
				int dy = Math.Max( y1 - p.Y, Math.Max( 0, p.Y - y2 ) );
				int dz = Math.Max( z1 - p.Z, Math.Max( 0, p.Z - z2 ) );
				int distX, distY, distZ;
				
				// X axis collisions
				int dxLeft = Math.Abs( x1 - p.X ), dxRight = Math.Abs( x2 - p.X );
				if( dxLeft < dxRight ) {				
					distX = dxLeft * dxLeft + dy * dy + dz * dz; xOffset = -1;
				} else {		
					distX = dxRight * dxRight + dy * dy + dz * dz; xOffset = 1;
				}
				
				// Z axis collisions
				int dxFront = Math.Abs( z1 - p.Z ), dxBack = Math.Abs( z2 - p.Z );
				if( dxFront < dxBack ) {			
					distZ = dx * dx + dy * dy + dxFront * dxFront; zOffset = -1;
				} else {			
					distZ = dx * dx + dy * dy + dxBack * dxBack; zOffset = 1;
				}
				
				// Y axis collisions
				int dxBottom = Math.Abs( y1 - p.Y ), dxTop = Math.Abs( y2 - p.Y );
				if( dxBottom < dxTop ) {					
					distY = dx * dx + dxBottom * dxBottom + dz * dz; yOffset = -1;
				} else {				
					distY = dx * dx + dxTop * dxTop + dz * dz; yOffset = 1;
				}
				
				int distMin = Math.Min( distX, Math.Min( distY, distZ ) );			
				bool occlude = true;
				byte flags = 0;
				if( distMin == distX )
					OccludeX( cx, cy, cz, xOffset, ref occlude, ref flags );
				if( distMin == distZ )
					OccludeZ( cx, cy, cz, zOffset, ref occlude, ref flags );
				if( distMin == distY )
					OccludeY( cx, cy, cz, yOffset, ref occlude, ref flags );
				
				if( occlude ) 
					chunk.Occluded = true;
				chunk.VisibilityFlags = (byte)( flags | chunk.OcclusionFlags );
				QueueChunk( cx - 1, cy, cz, queue );
				QueueChunk( cx + 1, cy, cz, queue );
				QueueChunk( cx, cy, cz - 1, queue );
				QueueChunk( cx, cy, cz + 1, queue );
				QueueChunk( cx, cy - 1, cz, queue );
				QueueChunk( cx, cy + 1, cz, queue );
			}
			Console.WriteLine( "======================" );
		}
		
		void OccludeX( int cx, int cy, int cz, int xOffset, ref bool occlude, ref byte flags ) {
			cx += xOffset;
			if( cx >= 0 && cx < chunksX ) {
				ChunkInfo neighbour = unsortedChunks[cx + chunksX * (cy + cz * chunksY)];
				if( (neighbour.VisibilityFlags & 1) == 0 )
					occlude = false;
				else
					flags |= 1;
			}
		}
		
		void OccludeZ( int cx, int cy, int cz, int zOffset, ref bool occlude, ref byte flags ) {
			cz += zOffset;
			if( cz >= 0 && cz < chunksZ ) {
				ChunkInfo neighbour = unsortedChunks[cx + chunksX * (cy + cz * chunksY)];
				if( (neighbour.VisibilityFlags & 2) == 0 )
					occlude = false;
				else
					flags |= 2;
			}
		}
		
		void OccludeY( int cx, int cy, int cz, int yOffset, ref bool occlude, ref byte flags ) {
			cy += yOffset;
			if( cy >= 0 && cy< chunksY ) {
				ChunkInfo neighbour = unsortedChunks[cx + chunksX * (cy + cz * chunksY)];
				if( (neighbour.VisibilityFlags & 4) == 0 )
					occlude = false;
				else
					flags |= 4;
			}
		}
		
		static float DistToRecSquared( Vector3 p, int x1, int y1, int z1, int x2, int y2, int z2 ) {
			float dx = Math.Max( x1 - p.X, Math.Max( 0, p.X - x2 ) );
			float dy = Math.Max( y1 - p.Y, Math.Max( 0, p.Y - y2 ) );
			float dz = Math.Max( z1 - p.Z, Math.Max( 0, p.Z - z2 ) );
			return dx * dx + dy * dy + dz * dz;
		}

		void QueueChunk( int cx, int cy, int cz, ChunkQueue queue ) {
			if( cx >= 0 && cy >= 0 && cz >= 0 && cx < chunksX && cy < chunksY && cz < chunksZ ) {
				ChunkInfo info = unsortedChunks[cx + chunksX * (cy + cz * chunksY)];
				if( !info.Visited ) 
					queue.Enqueue( info );
				info.Visited = true;
			}
		}
		
		class ChunkQueue {
			ChunkInfo[] array;
			int head, tail;
			public int Size;
			
			public ChunkQueue( int capacity ) {
				array = new ChunkInfo[capacity];
			}
			
			public void Clear() {
				Array.Clear( array, 0, Size );
				head = 0;
				tail = 0;
				Size = 0;
			}
			
			public void Enqueue( ChunkInfo item ) {
				if( Size == array.Length )
					throw new InvalidOperationException( "Queue limit reached" );
				
				array[tail] = item;
				tail = (tail + 1) % array.Length;
				Size++;
			}

			public ChunkInfo Dequeue() {
				if( Size == 0 )
					throw new InvalidOperationException( "No elements left in queue" );
				
				ChunkInfo item = array[head];
				array[head] = null;
				head = (head + 1) % array.Length;
				Size--;
				return item;
			}
		}
	}
}

