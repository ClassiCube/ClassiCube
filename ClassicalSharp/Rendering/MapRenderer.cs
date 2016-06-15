// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.Entities;
using ClassicalSharp.Events;
using ClassicalSharp.Map;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Renderers {
	
	public class ChunkInfo {
		
		public ushort CentreX, CentreY, CentreZ;
		public bool Visible = true, Empty = false;
		public bool DrawLeft, DrawRight, DrawFront, DrawBack, DrawBottom, DrawTop;
		#if OCCLUSION
		public bool Visited = false, Occluded = false;
		public byte OcclusionFlags, OccludedFlags, DistanceFlags;
		#endif
		
		public ChunkPartInfo[] NormalParts;
		public ChunkPartInfo[] TranslucentParts;
		
		public ChunkInfo( int x, int y, int z ) {
			CentreX = (ushort)(x + 8);
			CentreY = (ushort)(y + 8);
			CentreZ = (ushort)(z + 8);
		}
		
		public void Reset( int x, int y, int z ) {
			CentreX = (ushort)(x + 8);
			CentreY = (ushort)(y + 8);
			CentreZ = (ushort)(z + 8);
			
			Visible = true; Empty = false;
			DrawLeft = false; DrawRight = false; DrawFront = false;
			DrawBack = false; DrawBottom = false; DrawTop = false;
		}
	}
	
	public partial class MapRenderer : IDisposable {
		
		Game game;
		IGraphicsApi api;
		
		internal int _1DUsed = -1;
		internal ChunkInfo[] chunks, unsortedChunks;
		internal bool[] usedTranslucent, usedNormal;
		internal bool[] pendingTranslucent, pendingNormal;
		internal int[] totalUsed;
		ChunkUpdater updater;
		bool drawAllFaces = false, underWater = false;
		
		public MapRenderer( Game game ) {
			this.game = game;
			api = game.Graphics;
			updater = new ChunkUpdater( game, this );
		}
		
		public void Dispose() { updater.Dispose(); }
		
		public void Refresh() { updater.Refresh(); }
		
		public void RedrawBlock( int x, int y, int z, byte block, int oldHeight, int newHeight ) {
			updater.RedrawBlock( x, y, z, block, oldHeight, newHeight );
		}
		
		public void Render( double deltaTime ) {
			if( chunks == null ) return;
			ChunkSorter.UpdateSortOrder( game, updater );
			updater.UpdateChunks( deltaTime );			
			RenderNormalChunks( deltaTime );
		}
		
		public void RenderTranslucent( double deltaTime ) {
			if( chunks == null ) return;
			RenderTranslucentChunks( deltaTime );
		}
		
		
		// Render solid and fully transparent to fill depth buffer.
		// These blocks are treated as having an alpha value of either none or full.
		void RenderNormalChunks( double deltaTime ) {
			int[] texIds = game.TerrainAtlas1D.TexIds;
			api.SetBatchFormat( VertexFormat.P3fT2fC4b );
			api.Texturing = true;
			api.AlphaTest = true;
			
			for( int batch = 0; batch < _1DUsed; batch++ ) {
				if( totalUsed[batch] <= 0 ) continue;
				if( pendingNormal[batch] || usedNormal[batch] ) {
					api.BindTexture( texIds[batch] );
					RenderNormalBatch( batch );
					pendingNormal[batch] = false;
				}
			}
			
			CheckWeather( deltaTime );
			api.AlphaTest = false;
			api.Texturing = false;
			#if DEBUG_OCCLUSION
			DebugPickedPos();
			#endif
		}
		
		void CheckWeather( double deltaTime ) {
			WorldEnv env = game.World.Env;
			Vector3 pos = game.CurrentCameraPos;
			Vector3I coords = Vector3I.Floor( pos );
			
			byte block = game.World.SafeGetBlock( coords );
			drawAllFaces = game.BlockInfo.IsTranslucent[block];
			bool outside = !game.World.IsValidPos( Vector3I.Floor( pos ) );
			underWater = drawAllFaces || (pos.Y < env.EdgeHeight && outside);
			
			// If we are under water, render weather before to blend properly
			if( !underWater || env.Weather == Weather.Sunny ) return;
			api.AlphaBlending = true;
			game.WeatherRenderer.Render( deltaTime );
			api.AlphaBlending = false;
		}
		
		// Render translucent(liquid) blocks. These 'blend' into other blocks.
		void RenderTranslucentChunks( double deltaTime ) {			
			// First fill depth buffer
			int[] texIds = game.TerrainAtlas1D.TexIds;
			api.SetBatchFormat( VertexFormat.P3fT2fC4b );
			api.Texturing = false;
			api.AlphaBlending = false;
			api.ColourWrite = false;
			for( int batch = 0; batch < _1DUsed; batch++ ) {
				if( totalUsed[batch] <= 0 ) continue;
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
				if( totalUsed[batch] <= 0 ) continue;
				if( !usedTranslucent[batch] ) continue;
				api.BindTexture( texIds[batch] );
				RenderTranslucentBatch( batch );
			}
			
			api.DepthWrite = true;
			// If we weren't under water, render weather after to blend properly
			if( !underWater && game.World.Env.Weather != Weather.Sunny ) {
				api.AlphaTest = true;
				game.WeatherRenderer.Render( deltaTime );
				api.AlphaTest = false;
			}
			api.AlphaBlending = false;
			api.Texturing = false;
		}
		
		const DrawMode mode = DrawMode.Triangles;
		const int maxVertex = 65536;
		const int maxIndices = maxVertex / 4 * 6;
		void RenderNormalBatch( int batch ) {
			for( int i = 0; i < chunks.Length; i++ ) {
				ChunkInfo info = chunks[i];
				#if OCCLUSION
				if( info.NormalParts == null || !info.Visible || info.Occluded ) continue;
				#else
				if( info.NormalParts == null || !info.Visible ) continue;
				#endif

				ChunkPartInfo part = info.NormalParts[batch];
				if( part.IndicesCount == 0 ) continue;
				usedNormal[batch] = true;
				if( part.IndicesCount > maxIndices )
					DrawBigPart( info, ref part );
				else
					DrawPart( info, ref part );
				
				if( part.SpriteCount > 0 ) {
					int count = part.SpriteCount / 4; // 4 per sprite
					api.FaceCulling = true;
					if( info.DrawRight || info.DrawFront ) {
						api.DrawIndexedVb_TrisT2fC4b( count, 0 ); game.Vertices += count;
					}
					if( info.DrawLeft || info.DrawBack ) {
						api.DrawIndexedVb_TrisT2fC4b( count, count ); game.Vertices += count;
					}
					if( info.DrawLeft || info.DrawFront ) {
						api.DrawIndexedVb_TrisT2fC4b( count, count * 2 ); game.Vertices += count;
					}
					if( info.DrawRight || info.DrawBack ) {
						api.DrawIndexedVb_TrisT2fC4b( count, count * 3 ); game.Vertices += count;
					}
					api.FaceCulling = false;
				}
			}
		}

		void RenderTranslucentBatch( int batch ) {
			for( int i = 0; i < chunks.Length; i++ ) {
				ChunkInfo info = chunks[i];
				#if OCCLUSION
				if( info.TranslucentParts == null || !info.Visible || info.Occluded ) continue;
				#else
				if( info.TranslucentParts == null || !info.Visible ) continue;
				#endif
				ChunkPartInfo part = info.TranslucentParts[batch];
				
				if( part.IndicesCount == 0 ) continue;
				DrawTranslucentPart( info, ref part, 1 );
			}
		}
		
		void RenderTranslucentBatchDepthPass( int batch ) {
			for( int i = 0; i < chunks.Length; i++ ) {
				ChunkInfo info = chunks[i];
				#if OCCLUSION
				if( info.TranslucentParts == null || !info.Visible || info.Occluded ) continue;
				#else
				if( info.TranslucentParts == null || !info.Visible ) continue;
				#endif

				ChunkPartInfo part = info.TranslucentParts[batch];
				if( part.IndicesCount == 0 ) continue;
				usedTranslucent[batch] = true;
				DrawTranslucentPart( info, ref part, 0 );
			}
		}
		
		void DrawPart( ChunkInfo info, ref ChunkPartInfo part ) {
			api.BindVb( part.VbId );
			bool drawLeft = info.DrawLeft && part.LeftCount > 0;
			bool drawRight = info.DrawRight && part.RightCount > 0;
			bool drawBottom = info.DrawBottom && part.BottomCount > 0;
			bool drawTop = info.DrawTop && part.TopCount > 0;
			bool drawFront = info.DrawFront && part.FrontCount > 0;
			bool drawBack = info.DrawBack && part.BackCount > 0;
			
			if( drawLeft && drawRight ) {
				api.FaceCulling = true;
				api.DrawIndexedVb_TrisT2fC4b( part.LeftCount + part.RightCount, part.LeftIndex );
				api.FaceCulling = false;
				game.Vertices += part.LeftCount + part.RightCount;
			} else if( drawLeft ) {
				api.DrawIndexedVb_TrisT2fC4b( part.LeftCount, part.LeftIndex );
				game.Vertices += part.LeftCount;
			} else if( drawRight ) {
				api.DrawIndexedVb_TrisT2fC4b( part.RightCount, part.RightIndex ); 
				game.Vertices += part.RightCount;
			}
			
			if( drawFront && drawBack ) {
				api.FaceCulling = true;
				api.DrawIndexedVb_TrisT2fC4b( part.FrontCount + part.BackCount, part.FrontIndex );
				api.FaceCulling = false;
				 game.Vertices += part.FrontCount + part.BackCount;
			} else if( drawFront ) {
				api.DrawIndexedVb_TrisT2fC4b( part.FrontCount, part.FrontIndex );
				game.Vertices += part.FrontCount;
			} else if( drawBack ) {
				api.DrawIndexedVb_TrisT2fC4b( part.BackCount, part.BackIndex ); 
				game.Vertices += part.BackCount;
			}
			
			if( drawBottom && drawTop ) {
				api.FaceCulling = true;
				api.DrawIndexedVb_TrisT2fC4b( part.BottomCount + part.TopCount, part.BottomIndex );
				api.FaceCulling = false;
				 game.Vertices += part.BottomCount + part.TopCount;
			} else if( drawBottom ) {
				api.DrawIndexedVb_TrisT2fC4b( part.BottomCount, part.BottomIndex ); 
				game.Vertices += part.BottomCount;
			} else if( drawTop ) {
				api.DrawIndexedVb_TrisT2fC4b( part.TopCount, part.TopIndex ); 
				game.Vertices += part.TopCount;
			}
		}
		
		void DrawTranslucentPart( ChunkInfo info, ref ChunkPartInfo part, int m ) {
			api.BindVb( part.VbId );
			bool drawLeft = (drawAllFaces || info.DrawLeft) && part.LeftCount > 0;
			bool drawRight = (drawAllFaces || info.DrawRight) && part.RightCount > 0;
			bool drawBottom = (drawAllFaces || info.DrawBottom) && part.BottomCount > 0;
			bool drawTop = (drawAllFaces || info.DrawTop) && part.TopCount > 0;
			bool drawFront = (drawAllFaces || info.DrawFront) && part.FrontCount > 0;
			bool drawBack = (drawAllFaces || info.DrawBack) && part.BackCount > 0;
			
			if( drawLeft && drawRight ) {
				api.DrawIndexedVb_TrisT2fC4b( part.LeftCount + part.RightCount, part.LeftIndex ); 
				game.Vertices += m * (part.LeftCount + part.RightCount);
			} else if( drawLeft ) {
				api.DrawIndexedVb_TrisT2fC4b( part.LeftCount, part.LeftIndex );
				game.Vertices += m * part.LeftCount;
			} else if( drawRight ) {
				api.DrawIndexedVb_TrisT2fC4b( part.RightCount, part.RightIndex );
				game.Vertices += m * part.RightCount;
			}
			
			if( drawFront && drawBack ) {
				api.DrawIndexedVb_TrisT2fC4b( part.FrontCount + part.BackCount, part.FrontIndex );
				game.Vertices += m * (part.FrontCount + part.BackCount);
			} else if( drawFront ) {
				api.DrawIndexedVb_TrisT2fC4b( part.FrontCount, part.FrontIndex );
				game.Vertices += m * part.FrontCount;
			} else if( drawBack ) {
				api.DrawIndexedVb_TrisT2fC4b( part.BackCount, part.BackIndex );
				game.Vertices += m * part.BackCount;
			}
			
			if( drawBottom && drawTop ) {
				api.DrawIndexedVb_TrisT2fC4b( part.BottomCount + part.TopCount, part.BottomIndex );
				game.Vertices += m * (part.BottomCount + part.TopCount);
			} else if( drawBottom ) {
				api.DrawIndexedVb_TrisT2fC4b( part.BottomCount, part.BottomIndex ); 
				game.Vertices += m * part.BottomCount;
			} else if( drawTop ) {
				api.DrawIndexedVb_TrisT2fC4b( part.TopCount, part.TopIndex ); 
				game.Vertices += m * part.TopCount;
			}
		}
		
		void DrawBigPart( ChunkInfo info, ref ChunkPartInfo part ) {
			api.BindVb( part.VbId );
			bool drawLeft = info.DrawLeft && part.LeftCount > 0;
			bool drawRight = info.DrawRight && part.RightCount > 0;
			bool drawBottom = info.DrawBottom && part.BottomCount > 0;
			bool drawTop = info.DrawTop && part.TopCount > 0;
			bool drawFront = info.DrawFront && part.FrontCount > 0;
			bool drawBack = info.DrawBack && part.BackCount > 0;
			
			if( drawLeft && drawRight ) {
				api.FaceCulling = true;
				api.DrawIndexedVb_TrisT2fC4b( part.LeftCount + part.RightCount, part.LeftIndex );
				api.FaceCulling = false;
				game.Vertices += part.LeftCount + part.RightCount;
			} else if( drawLeft ) {
				api.DrawIndexedVb_TrisT2fC4b( part.LeftCount, part.LeftIndex ); 
				game.Vertices += part.LeftCount;
			} else if( drawRight ) {
				api.DrawIndexedVb_TrisT2fC4b( part.RightCount, part.RightIndex ); 
				game.Vertices += part.RightCount;
			}
			
			if( drawFront && drawBack ) {
				api.FaceCulling = true;
				api.DrawIndexedVb_TrisT2fC4b( part.FrontCount + part.BackCount, part.FrontIndex );
				api.FaceCulling = false;
				game.Vertices += part.FrontCount + part.BackCount;
			} else if( drawFront ) {
				api.DrawIndexedVb_TrisT2fC4b( part.FrontCount, part.FrontIndex ); 
				game.Vertices += part.FrontCount;
			} else if( drawBack ) {
				api.DrawIndexedVb_TrisT2fC4b( part.BackCount, part.BackIndex ); 
				game.Vertices += part.BackCount;
			}
			
			// Special handling for top and bottom as these can go over 65536 vertices and we need to adjust the indices in this case.
			if( drawBottom && drawTop ) {
				api.FaceCulling = true;
				if( part.IndicesCount > maxIndices ) {
					int part1Count = maxIndices - part.BottomIndex;
					api.DrawIndexedVb_TrisT2fC4b( part1Count, part.BottomIndex );
					api.DrawIndexedVb_TrisT2fC4b( part.BottomCount + part.TopCount - part1Count, maxVertex, 0 );
				} else {
					api.DrawIndexedVb_TrisT2fC4b( part.BottomCount + part.TopCount, part.BottomIndex );
				}
				api.FaceCulling = false;
				game.Vertices += part.TopCount + part.BottomCount;
			} else if( drawBottom ) {
				int part1Count;
				if( part.IndicesCount > maxIndices &&
				   ( part1Count = maxIndices - part.BottomIndex ) < part.BottomCount ) {
					api.DrawIndexedVb_TrisT2fC4b( part1Count, part.BottomIndex );
					api.DrawIndexedVb_TrisT2fC4b( part.BottomCount - part1Count, maxVertex, 0 );
				} else {
					api.DrawIndexedVb_TrisT2fC4b( part.BottomCount, part.BottomIndex );
				}
				game.Vertices += part.BottomCount;
			} else if( drawTop ) {
				int part1Count;
				if( part.IndicesCount > maxIndices &&
				   ( part1Count = maxIndices - part.TopIndex ) < part.TopCount ) {
					api.DrawIndexedVb_TrisT2fC4b( part1Count, part.TopIndex );
					api.DrawIndexedVb_TrisT2fC4b( part.TopCount - part1Count, maxVertex, 0 );
				} else {
					api.DrawIndexedVb_TrisT2fC4b( part.TopCount, part.TopIndex );
				}
				game.Vertices += part.TopCount;
			}
		}
	}
}