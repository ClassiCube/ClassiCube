using System;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp {
	
	public partial class MapRenderer : IDisposable {

		void UpdateSortOrder() {
			Vector3 cameraPos = game.CurrentCameraPos;
			Vector3I newChunkPos = Vector3I.Floor( cameraPos );
			newChunkPos.X = (newChunkPos.X & ~0x0F) + 8;
			newChunkPos.Y = (newChunkPos.Y & ~0x0F) + 8;
			newChunkPos.Z = (newChunkPos.Z & ~0x0F) + 8;
			if( newChunkPos == chunkPos ) return;
			
			chunkPos = newChunkPos;
			for( int i = 0; i < distances.Length; i++ ) {
				ChunkInfo info = chunks[i];
				distances[i] = Utils.DistanceSquared( info.CentreX, info.CentreY, info.CentreZ, chunkPos.X, chunkPos.Y, chunkPos.Z );
			}
			
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
			RecalcBooleans( false );
		}
	}
}