// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Renderers {
	
	public partial class MapRenderer : IDisposable {
		
		// Render solid and fully transparent to fill depth buffer.
		// These blocks are treated as having an alpha value of either none or full.
		void RenderNormal() {
			int[] texIds = game.TerrainAtlas1D.TexIds;
			api.SetBatchFormat( VertexFormat.Pos3fTex2fCol4b );
			api.Texturing = true;
			api.AlphaTest = true;
			
			for( int batch = 0; batch < _1DUsed; batch++ ) {
				if( pendingNormal[batch] || usedNormal[batch] ) {
					api.BindTexture( texIds[batch] );
					RenderNormalBatch( batch );
					pendingNormal[batch] = false;
				}
			}
			api.AlphaTest = false;
			api.Texturing = false;
			DebugPickedPos();
		}
		
		// Render translucent(liquid) blocks. These 'blend' into other blocks.
		void RenderTranslucent() {
			Block block = game.LocalPlayer.BlockAtHead;
			drawAllFaces = block == Block.Water || block == Block.StillWater;
			// First fill depth buffer
			int[] texIds = game.TerrainAtlas1D.TexIds;
			api.SetBatchFormat( VertexFormat.Pos3fTex2fCol4b );
			api.Texturing = false;
			api.AlphaBlending = false;
			api.ColourWrite = false;
			for( int batch = 0; batch < _1DUsed; batch++ ) {
				if( pendingTranslucent[batch] || usedTranslucent[batch] ) {
					RenderTranslucentBatchDepthPass( batch );
					pendingTranslucent[batch] = false;
				}
			}
			
			// Then actually draw the transluscent blocks
			api.AlphaBlending = true;
			api.Texturing = true;
			api.ColourWrite = true;
			api.DepthWrite = false; // we already calculated depth values in depth pass
			
			for( int batch = 0; batch < _1DUsed; batch++ ) {
				if( !usedTranslucent[batch] ) continue;
				api.BindTexture( texIds[batch] );
				RenderTranslucentBatch( batch );
			}
			api.DepthWrite = true;
			api.AlphaTest = false;
			api.AlphaBlending = false;
			api.Texturing = false;
		}
		
		const DrawMode mode = DrawMode.Triangles;
		const int maxVertex = 65536;
		const int maxIndices = maxVertex / 4 * 6;
		void RenderNormalBatch( int batch ) {
			for( int i = 0; i < chunks.Length; i++ ) {
				ChunkInfo info = chunks[i];
				if( info.NormalParts == null || !info.Visible || info.Occluded ) continue;

				ChunkPartInfo part = info.NormalParts[batch];
				if( part.IndicesCount == 0 ) continue;
				usedNormal[batch] = true;
				if( part.IndicesCount > maxIndices )
					DrawBigPart( info, ref part );
				else
					DrawPart( info, ref part );			
				
				if( part.spriteCount > 0 ) {
					api.FaceCulling = true;
					api.DrawIndexedVb_TrisT2fC4b( part.spriteCount, 0 );
					api.FaceCulling = false;
				}
				game.Vertices += part.IndicesCount;
			}
		}

		void RenderTranslucentBatch( int batch ) {
			for( int i = 0; i < chunks.Length; i++ ) {
				ChunkInfo info = chunks[i];
				if( info.TranslucentParts == null || !info.Visible || info.Occluded ) continue;
				ChunkPartInfo part = info.TranslucentParts[batch];
				
				if( part.IndicesCount == 0 ) continue;
				DrawTranslucentPart( info, ref part );
				game.Vertices += part.IndicesCount;
			}
		}
		
		void RenderTranslucentBatchDepthPass( int batch ) {
			for( int i = 0; i < chunks.Length; i++ ) {
				ChunkInfo info = chunks[i];
				if( info.TranslucentParts == null || !info.Visible || info.Occluded ) continue;

				ChunkPartInfo part = info.TranslucentParts[batch];
				if( part.IndicesCount == 0 ) continue;
				usedTranslucent[batch] = true;
				DrawTranslucentPart( info, ref part );
			}
		}
		
		void DrawPart( ChunkInfo info, ref ChunkPartInfo part ) {
			api.BindVb( part.VbId );
			bool drawLeft = info.DrawLeft && part.leftCount > 0;
			bool drawRight = info.DrawRight && part.rightCount > 0;
			bool drawBottom = info.DrawBottom && part.bottomCount > 0;
			bool drawTop = info.DrawTop && part.topCount > 0;
			bool drawFront = info.DrawFront && part.frontCount > 0;
			bool drawBack = info.DrawBack && part.backCount > 0;
			
			if( drawLeft && drawRight ) {
				api.FaceCulling = true;
				api.DrawIndexedVb_TrisT2fC4b( part.leftCount + part.rightCount, part.leftIndex );
				api.FaceCulling = false;
			} else if( drawLeft ) {
				api.DrawIndexedVb_TrisT2fC4b( part.leftCount, part.leftIndex );
			} else if( drawRight ) {
				api.DrawIndexedVb_TrisT2fC4b( part.rightCount, part.rightIndex );
			}
			
			if( drawFront && drawBack ) {
				api.FaceCulling = true;
				api.DrawIndexedVb_TrisT2fC4b( part.frontCount + part.backCount, part.frontIndex );
				api.FaceCulling = false;
			} else if( drawFront ) {
				api.DrawIndexedVb_TrisT2fC4b( part.frontCount, part.frontIndex );
			} else if( drawBack ) {
				api.DrawIndexedVb_TrisT2fC4b( part.backCount, part.backIndex );
			}
			
			if( drawBottom && drawTop ) {
				api.FaceCulling = true;
				api.DrawIndexedVb_TrisT2fC4b( part.bottomCount + part.topCount, part.bottomIndex );
				api.FaceCulling = false;
			} else if( drawBottom ) {
				api.DrawIndexedVb_TrisT2fC4b( part.bottomCount, part.bottomIndex );
			} else if( drawTop ) {
				api.DrawIndexedVb_TrisT2fC4b( part.topCount, part.topIndex );			
			}
		}
		
		bool drawAllFaces = false;
		void DrawTranslucentPart( ChunkInfo info, ref ChunkPartInfo part ) {
			api.BindVb( part.VbId );
			bool drawLeft = (drawAllFaces || info.DrawLeft) && part.leftCount > 0;
			bool drawRight = (drawAllFaces || info.DrawRight) && part.rightCount > 0;
			bool drawBottom = (drawAllFaces || info.DrawBottom) && part.bottomCount > 0;
			bool drawTop = (drawAllFaces || info.DrawTop) && part.topCount > 0;
			bool drawFront = (drawAllFaces || info.DrawFront) && part.frontCount > 0;
			bool drawBack = (drawAllFaces || info.DrawBack) && part.backCount > 0;
			
			if( drawLeft && drawRight ) {
				api.DrawIndexedVb_TrisT2fC4b( part.leftCount + part.rightCount, part.leftIndex );
			} else if( drawLeft ) {
				api.DrawIndexedVb_TrisT2fC4b( part.leftCount, part.leftIndex );
			} else if( drawRight ) {
				api.DrawIndexedVb_TrisT2fC4b( part.rightCount, part.rightIndex );
			}
			
			if( drawFront && drawBack ) {
				api.DrawIndexedVb_TrisT2fC4b( part.frontCount + part.backCount, part.frontIndex );
			} else if( drawFront ) {
				api.DrawIndexedVb_TrisT2fC4b( part.frontCount, part.frontIndex );
			} else if( drawBack ) {
				api.DrawIndexedVb_TrisT2fC4b( part.backCount, part.backIndex );
			}
			
			if( drawBottom && drawTop ) {
				api.DrawIndexedVb_TrisT2fC4b( part.bottomCount + part.topCount, part.bottomIndex );
			} else if( drawBottom ) {
				api.DrawIndexedVb_TrisT2fC4b( part.bottomCount, part.bottomIndex );
			} else if( drawTop ) {
				api.DrawIndexedVb_TrisT2fC4b( part.topCount, part.topIndex );			
			}
		}
		
		void DrawBigPart( ChunkInfo info, ref ChunkPartInfo part ) {
			api.BindVb( part.VbId );
			bool drawLeft = info.DrawLeft && part.leftCount > 0;
			bool drawRight = info.DrawRight && part.rightCount > 0;
			bool drawBottom = info.DrawBottom && part.bottomCount > 0;
			bool drawTop = info.DrawTop && part.topCount > 0;
			bool drawFront = info.DrawFront && part.frontCount > 0;
			bool drawBack = info.DrawBack && part.backCount > 0;
			
			if( drawLeft && drawRight ) {
				api.FaceCulling = true;
				api.DrawIndexedVb_TrisT2fC4b( part.leftCount + part.rightCount, part.leftIndex );
				api.FaceCulling = false;
			} else if( drawLeft ) {
				api.DrawIndexedVb_TrisT2fC4b( part.leftCount, part.leftIndex );
			} else if( drawRight ) {
				api.DrawIndexedVb_TrisT2fC4b( part.rightCount, part.rightIndex );
			}
			
			if( drawFront && drawBack ) {
				api.FaceCulling = true;
				api.DrawIndexedVb_TrisT2fC4b( part.frontCount + part.backCount, part.frontIndex );
				api.FaceCulling = false;
			} else if( drawFront ) {
				api.DrawIndexedVb_TrisT2fC4b( part.frontCount, part.frontIndex );
			} else if( drawBack ) {
				api.DrawIndexedVb_TrisT2fC4b( part.backCount, part.backIndex );
			}
			
			// Special handling for top and bottom as these can go over 65536 vertices and we need to adjust the indices in this case.
			if( drawBottom && drawTop ) {
				api.FaceCulling = true;
				if( part.IndicesCount > maxIndices ) {
					int part1Count = maxIndices - part.bottomIndex;
					api.DrawIndexedVb_TrisT2fC4b( part1Count, part.bottomIndex );
					api.DrawIndexedVb_TrisT2fC4b( part.bottomCount + part.topCount - part1Count, maxVertex, 0 );
				} else {
					api.DrawIndexedVb_TrisT2fC4b( part.bottomCount + part.topCount, part.bottomIndex );
				}
				api.FaceCulling = false;
			} else if( drawBottom ) {
				int part1Count;
				if( part.IndicesCount > maxIndices &&
				   ( part1Count = maxIndices - part.bottomIndex ) < part.bottomCount ) {					
					api.DrawIndexedVb_TrisT2fC4b( part1Count, part.bottomIndex );
					api.DrawIndexedVb_TrisT2fC4b( part.bottomCount - part1Count, maxVertex, 0 );
				} else {
					api.DrawIndexedVb_TrisT2fC4b( part.bottomCount, part.bottomIndex );
				}
			} else if( drawTop ) {
				int part1Count;
				if( part.IndicesCount > maxIndices &&
				   ( part1Count = maxIndices - part.topIndex ) < part.topCount ) {
					api.DrawIndexedVb_TrisT2fC4b( part1Count, part.topIndex );
					api.DrawIndexedVb_TrisT2fC4b( part.topCount - part1Count, maxVertex, 0 );
				} else {
					api.DrawIndexedVb_TrisT2fC4b( part.topCount, part.topIndex );
				}			
			}
		}
	}
}