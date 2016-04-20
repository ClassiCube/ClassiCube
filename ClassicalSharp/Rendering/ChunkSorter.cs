// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Renderers {
	
	public static class ChunkSorter {

		public static void UpdateSortOrder( Game game, ChunkUpdater updater ) {
			Vector3 cameraPos = game.CurrentCameraPos;
			Vector3I newChunkPos = Vector3I.Floor( cameraPos );
			newChunkPos.X = (newChunkPos.X & ~0x0F) + 8;
			newChunkPos.Y = (newChunkPos.Y & ~0x0F) + 8;
			newChunkPos.Z = (newChunkPos.Z & ~0x0F) + 8;			
			if( newChunkPos == updater.chunkPos ) return;
			
			ChunkInfo[] chunks = game.MapRenderer.chunks;
			int[] distances = updater.distances;		
			updater.chunkPos = newChunkPos;
			
			for( int i = 0; i < distances.Length; i++ ) {
				ChunkInfo info = chunks[i];
				distances[i] = Utils.DistanceSquared( info.CentreX, info.CentreY, info.CentreZ, 
				                                     updater.chunkPos.X, updater.chunkPos.Y, updater.chunkPos.Z );
			}

			// NOTE: Over 5x faster compared to normal comparison of IComparer<ChunkInfo>.Compare
			if( distances.Length > 1 )
				QuickSort( distances, chunks, 0, chunks.Length - 1 );
			
			Vector3I pPos = newChunkPos;
			for( int i = 0; i < chunks.Length; i++ ) {
				ChunkInfo info = chunks[i];
				int dX1 = (info.CentreX - 8) - pPos.X, dX2 = (info.CentreX + 8) - pPos.X;
				int dY1 = (info.CentreY - 8) - pPos.Y, dY2 = (info.CentreY + 8) - pPos.Y;
				int dZ1 = (info.CentreZ - 8) - pPos.Z, dZ2 = (info.CentreZ + 8) - pPos.Z;
				
				// Back face culling: make sure that the chunk is definitely entirely back facing.
				info.DrawLeft = !(dX1 <= 0 && dX2 <= 0);
				info.DrawRight = !(dX1 >= 0 && dX2 >= 0);
				info.DrawFront = !(dZ1 <= 0 && dZ2 <= 0);
				info.DrawBack = !(dZ1 >= 0 && dZ2 >= 0);
				info.DrawBottom = !(dY1 <= 0 && dY2 <= 0);
				info.DrawTop = !(dY1 >= 0 && dY2 >= 0);
			}
			updater.RecalcBooleans( false );
			//SimpleOcclusionCulling();
		}
		
		static void QuickSort( int[] keys, ChunkInfo[] values, int left, int right ) {
			while( left < right ) {
				int i = left, j = right;
				int pivot = keys[(i + j) / 2];
				// partition the list
				while( i <= j ) {
					while( pivot > keys[i] ) i++;
					while( pivot < keys[j] ) j--;
					
					if( i <= j ) {
						int key = keys[i]; keys[i] = keys[j]; keys[j] = key;
						ChunkInfo value = values[i]; values[i] = values[j]; values[j] = value;
						i++; j--;
					}
				}
				
				// recurse into the smaller subset
				if( j - left <= right - i ) {
					if( left < j )
						QuickSort( keys, values, left, j );
					left = i;
				} else {
					if( i < right )
						QuickSort( keys, values, i, right );
					right = j;
				}
			}
		}
	}
}