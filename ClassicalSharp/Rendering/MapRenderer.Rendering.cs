using System;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp {
	
	// TODO: optimise chunk rendering
	//  --> reduce iterations: liquid and sprite pass only need 1 row
	public partial class MapRenderer : IDisposable {
		
		const DrawMode mode = DrawMode.Triangles;
		const int maxVertex = 65536;
		const int maxIndices = maxVertex / 4 * 6;
		void RenderNormalBatch( int batch ) {
			for( int i = 0; i < chunks.Length; i++ ) {
				ChunkInfo info = chunks[i];
				if( info.NormalParts == null || !info.Visible ) continue;

				ChunkPartInfo part = info.NormalParts[batch];
				if( part.IndicesCount == 0 ) continue;
				DrawPart( info, ref part );
				
				if( part.spriteCount > 0 )
					api.DrawIndexedVb( mode, part.spriteCount, 0, 0 );
				game.Vertices += part.IndicesCount;
			}
		}

		void RenderTranslucentBatch( int batch ) {
			for( int i = 0; i < chunks.Length; i++ ) {
				ChunkInfo info = chunks[i];
				if( info.TranslucentParts == null || !info.Visible ) continue;
				ChunkPartInfo part = info.TranslucentParts[batch];
				
				if( part.IndicesCount == 0 ) continue;
				DrawPart( info, ref part );
				game.Vertices += part.IndicesCount;
			}
		}
		
		void RenderTranslucentBatchDepthPass( int batch ) {
			for( int i = 0; i < chunks.Length; i++ ) {
				ChunkInfo info = chunks[i];
				if( info.TranslucentParts == null || !info.Visible ) continue;

				ChunkPartInfo part = info.TranslucentParts[batch];
				if( part.IndicesCount == 0 ) continue;
				DrawPart( info, ref part );
			}
		}
		
		void DrawPart( ChunkInfo info, ref ChunkPartInfo part ) {
			api.BindIndexedVb( part.VbId, part.IbId );
			bool drawLeft = info.DrawLeft && part.leftCount > 0;
			bool drawRight = info.DrawRight && part.rightCount > 0;
			bool drawBottom = info.DrawBottom && part.bottomCount > 0;
			bool drawTop = info.DrawTop && part.topCount > 0;
			bool drawFront = info.DrawFront && part.frontCount > 0;
			bool drawBack = info.DrawBack && part.backCount > 0;
			
			if( drawLeft && drawRight ) {
				api.FaceCulling = true;
				api.DrawIndexedVb( mode, part.leftCount + part.rightCount, 0, part.leftIndex );
				api.FaceCulling = false;
			} else if( drawLeft ) {
				api.DrawIndexedVb( mode, part.leftCount, 0, part.leftIndex );
			} else if( drawRight ) {
				api.DrawIndexedVb( mode, part.rightCount, 0, part.rightIndex );
			}
			
			if( drawFront && drawBack ) {
				api.FaceCulling = true;
				api.DrawIndexedVb( mode, part.frontCount + part.backCount, 0, part.frontIndex );
				api.FaceCulling = false;
			} else if( drawFront ) {
				api.DrawIndexedVb( mode, part.frontCount, 0, part.frontIndex );
			} else if( drawBack ) {
				api.DrawIndexedVb( mode, part.backCount, 0, part.backIndex );
			}
			
			if( drawBottom && drawTop ) {
				api.FaceCulling = true;
				if( part.IndicesCount > maxIndices ) {
					int part1Count = maxIndices - part.bottomIndex;
					api.DrawIndexedVb( mode, part1Count, 0, part.bottomIndex );
					api.DrawIndexedVb( mode, part.bottomCount + part.topCount - part1Count, maxVertex, part.bottomIndex );
				} else {
					api.DrawIndexedVb( mode, part.bottomCount + part.topCount, 0, part.bottomIndex );
				}
				api.FaceCulling = false;
			} else if( drawBottom ) {
				int part1Count;
				if( part.IndicesCount > maxIndices &&
				   ( part1Count = maxIndices - part.bottomIndex ) < part.bottomCount ) {					
					api.DrawIndexedVb( mode, part1Count, 0, part.bottomIndex );
					api.DrawIndexedVb( mode, part.bottomCount - part1Count, maxVertex, part.bottomIndex );
				} else {
					api.DrawIndexedVb( mode, part.bottomCount, 0, part.bottomIndex );
				}
			} else if( drawTop ) {
				int part1Count;
				if( part.IndicesCount > maxIndices &&
				   ( part1Count = maxIndices - part.topIndex ) < part.topCount ) {
					api.DrawIndexedVb( mode, part1Count, 0, part.topIndex );
					api.DrawIndexedVb( mode, part.topCount - part1Count, maxVertex, part.topIndex );
				} else {
					api.DrawIndexedVb( mode, part.topCount, 0, part.topIndex );
				}
				
			}
			/*if( info.DrawLeft && part.leftCount > 0 )
				api.DrawIndexedVb( mode, part.leftCount, 0, part.leftIndex );
			if( info.DrawRight && part.rightCount > 0 )
				api.DrawIndexedVb( mode, part.rightCount, 0, part.rightIndex );
			if( info.DrawBottom && part.bottomCount > 0 )
				api.DrawIndexedVb( mode, part.bottomCount, 0, part.bottomIndex );
			if( info.DrawTop && part.topCount > 0 )
				api.DrawIndexedVb( mode, part.topCount, 0, part.topIndex );
			if( info.DrawFront && part.frontCount > 0 )
				api.DrawIndexedVb( mode, part.frontCount, 0, part.frontIndex );
			if( info.DrawBack && part.backCount > 0 )
				api.DrawIndexedVb( mode, part.backCount, 0, part.backIndex );*/
		}
	}
}