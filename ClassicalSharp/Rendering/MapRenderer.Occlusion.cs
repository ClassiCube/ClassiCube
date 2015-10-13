using System;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp {
	
	public partial class MapRenderer : IDisposable {
		
		void SimpleOcclusionCulling() { // TODO: still broken
			Vector3 p = game.LocalPlayer.EyePosition;
			Vector3I mapLoc = Vector3I.Floor( p );
			Utils.Clamp( ref mapLoc.X, 0, game.Map.Width - 1 );
			Utils.Clamp( ref mapLoc.Y, 0, game.Map.Height - 1 );
			Utils.Clamp( ref mapLoc.Z, 0, game.Map.Length- 1 );
			
			int cx = mapLoc.X >> 4;
			int cy = mapLoc.Y >> 4;
			int cz = mapLoc.Z >> 4;
			
			ChunkInfo chunkIn = unsortedChunks[cx + chunksX * (cy + cz * chunksY)];
			byte chunkInOcclusionFlags = chunkIn.OcclusionFlags;
			chunkIn.OcclusionFlags = 0;
			ChunkQueue queue = new ChunkQueue( chunksX * chunksY * chunksZ );
			for( int i = 0; i < chunks.Length; i++ ) {
				ChunkInfo chunk = chunks[i];
				chunk.Visited = false;
				chunk.Occluded = false;
				chunk.OccludedFlags = chunk.OcclusionFlags;
				chunk.DistanceFlags = 0;
			}
			
			chunkIn.Visited = true;
			mapLoc = Vector3I.Floor( p );
			if( game.Map.IsValidPos( mapLoc ) ) {
				chunkIn.DistanceFlags = flagX | flagY | flagZ;
			} else {
				chunkIn.OccludedFlags = chunkIn.OcclusionFlags = chunkInOcclusionFlags;
				chunkIn.DistanceFlags |= (mapLoc.X < 0 || mapLoc.X >= game.Map.Width) ? flagX : (byte)0;
				chunkIn.DistanceFlags |= (mapLoc.Y < 0 || mapLoc.Y >= game.Map.Height) ? flagY : (byte)0;
				chunkIn.DistanceFlags |= (mapLoc.Z < 0 || mapLoc.Z >= game.Map.Length) ? flagZ : (byte)0;
			}
			
			QueueChunk( cx - 1, cy, cz, queue );
			QueueChunk( cx + 1, cy, cz, queue );
			QueueChunk( cx, cy - 1, cz, queue );
			QueueChunk( cx, cy + 1, cz, queue );
			QueueChunk( cx, cy, cz - 1, queue );
			QueueChunk( cx, cy, cz + 1, queue );			
			ProcessQueue( chunkPos, queue );
			chunkIn.OcclusionFlags = chunkInOcclusionFlags;
		}
		
		void ProcessQueue( Vector3I p, ChunkQueue queue ) {
			while( queue.Size > 0 ) {
				ChunkInfo chunk = queue.Dequeue();
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
				
				chunk.Occluded = true;
				int distMin = Math.Min( distX, Math.Min( distY, distZ ) );
				if( distMin == distX ) OccludeX( cx, cy, cz, xOffset, chunk );
				if( distMin == distZ ) OccludeZ( cx, cy, cz, zOffset, chunk );
				if( distMin == distY ) OccludeY( cx, cy, cz, yOffset, chunk );
				
				//Console.WriteLine( distMin + " , " + distX + " , " + distY + " , " + distZ );
				//Console.WriteLine( chunk.DistanceFlags + " : " + chunk.OccludedFlags + " : " + chunk.OcclusionFlags );
				QueueChunk( cx - 1, cy, cz, queue );
				QueueChunk( cx + 1, cy, cz, queue );
				QueueChunk( cx, cy, cz - 1, queue );
				QueueChunk( cx, cy, cz + 1, queue );
				QueueChunk( cx, cy - 1, cz, queue );
				QueueChunk( cx, cy + 1, cz, queue );
			}
			Console.WriteLine( "======================" );
		}
		const byte flagX = 1, flagZ = 2, flagY = 4;
		
		public void DebugPickedPos() {
			if( game.SelectedPos.Valid ) {
				Vector3I p = game.SelectedPos.BlockPos;
				int cx = p.X >> 4;
				int cy = p.Y >> 4;
				int cz = p.Z >> 4;
				ChunkInfo chunk = unsortedChunks[cx + chunksX * (cy + cz * chunksY)];
				//Console.WriteLine( chunk.DistanceFlags + " : " + chunk.OccludedFlags + " : " + chunk.OcclusionFlags + " , " + chunk.Occluded );
			}
		}
		
		void OccludeX( int cx, int cy, int cz, int xOffset, ChunkInfo info ) {
			cx += xOffset;
			if( cx >= 0 && cx < chunksX ) {
				ChunkInfo neighbour = unsortedChunks[cx + chunksX * (cy + cz * chunksY)];
				if( (neighbour.OccludedFlags & neighbour.DistanceFlags) != neighbour.DistanceFlags )
					info.Occluded = false;
				else
					info.OccludedFlags |= flagX;
			} else {
				info.Occluded = false;
			}
			info.DistanceFlags |= flagX;
		}
		
		void OccludeZ( int cx, int cy, int cz, int zOffset, ChunkInfo info ) {
			cz += zOffset;
			if( cz >= 0 && cz < chunksZ ) {
				ChunkInfo neighbour = unsortedChunks[cx + chunksX * (cy + cz * chunksY)];
				if( (neighbour.OccludedFlags & neighbour.DistanceFlags) != neighbour.DistanceFlags )
					info.Occluded = false;
				else
					info.OccludedFlags |= flagZ;
			} else {
				info.Occluded = false;
			}
			info.DistanceFlags |= flagZ;
		}
		
		void OccludeY( int cx, int cy, int cz, int yOffset, ChunkInfo info ) {
			cy += yOffset;
			if( cy >= 0 && cy < chunksY ) {
				ChunkInfo neighbour = unsortedChunks[cx + chunksX * (cy + cz * chunksY)];
				if( (neighbour.OccludedFlags & neighbour.DistanceFlags) != neighbour.DistanceFlags )
					info.Occluded = false;
				else
					info.OccludedFlags |= flagY;
			} else {
				info.Occluded = false;
			}
			info.DistanceFlags |= flagY;
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

