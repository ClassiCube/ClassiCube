using System;
using ClassicalSharp;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace DefaultPlugin {
	
	// TODO: optimise chunk rendering
	//  --> reduce iterations: liquid and sprite pass only need 1 row
	public sealed class StandardMapRenderer : MapRenderer {
		
		class ChunkInfo {
			
			public short CentreX, CentreY, CentreZ;
			public bool Visible = true;
			public bool Empty = false;
			
			public ChunkPartInfo[] SolidParts;
			public ChunkPartInfo[] SpriteParts;
			public ChunkPartInfo[] TranslucentParts;
			
			public ChunkInfo( int x, int y, int z ) {
				CentreX = (short)( x + 8 );
				CentreY = (short)( y + 8 );
				CentreZ = (short)( z + 8 );
			}
		}
		
		int _1Dcount = 1;
		ChunkMeshBuilder builder;
		MapShader shader;
		MapDepthPassShader lShader;
		bool needToUpdateColours = true;
		
		int width, height, length;
		ChunkInfo[] chunks, unsortedChunks;
		Vector3I chunkPos = new Vector3I( int.MaxValue, int.MaxValue, int.MaxValue );
		int elementsPerBitmap = 0;
		
		public StandardMapRenderer( Game window ) : base( window ) {
			_1Dcount = window.TerrainAtlas1D.TexIds.Length;
			builder = new StandardChunkMeshBuilder( window );
			elementsPerBitmap = window.TerrainAtlas1D.elementsPerBitmap;
		}
		
		public override void Init() {
			base.Init();
			shader = new MapShader();
			shader.Init( api );
			lShader = new MapDepthPassShader();
			lShader.Init( api );
			
			Window.TerrainAtlasChanged += TerrainAtlasChanged;
			Window.EnvVariableChanged += EnvVariableChanged;
		}
		
		public override void Dispose() {
			ClearChunkCache();
			chunks = null;
			unsortedChunks = null;
			Window.EnvVariableChanged -= EnvVariableChanged;
			base.Dispose();
		}
		
		public override void Refresh() {
			if( chunks != null && !Window.Map.IsNotLoaded ) {
				ClearChunkCache();
				CreateChunkCache();
			}
		}
		
		void EnvVariableChanged( object sender, EnvVariableEventArgs e ) {
			if( e.Variable == EnvVariable.SunlightColour || e.Variable == EnvVariable.ShadowlightColour ) {
				needToUpdateColours = true;
			}
		}

		void TerrainAtlasChanged( object sender, EventArgs e ) {
			_1Dcount = Window.TerrainAtlas1D.TexIds.Length;
			bool fullResetRequired = elementsPerBitmap != Window.TerrainAtlas1D.elementsPerBitmap;
			if( fullResetRequired ) {
				Refresh();
			}
			elementsPerBitmap = Window.TerrainAtlas1D.elementsPerBitmap;
		}
		
		protected override void OnNewMap( object sender, EventArgs e ) {
			Window.ChunkUpdates = 0;
			ClearChunkCache();
			chunks = null;
			unsortedChunks = null;
			chunkPos = new Vector3I( int.MaxValue, int.MaxValue, int.MaxValue );
		}
		
		int chunksX, chunksY, chunksZ;
		protected override void OnNewMapLoaded( object sender, EventArgs e ) {
			width = NextMultipleOf16( Window.Map.Width );
			height = NextMultipleOf16( Window.Map.Height );
			length = NextMultipleOf16( Window.Map.Length );
			chunksX = width >> 4;
			chunksY = height >> 4;
			chunksZ = length >> 4;
			
			chunks = new ChunkInfo[chunksX * chunksY * chunksZ];
			unsortedChunks = new ChunkInfo[chunksX * chunksY * chunksZ];
			distances = new int[chunks.Length];
			CreateChunkCache();
		}
		
		void ClearChunkCache() {
			if( chunks == null ) return;
			for( int i = 0; i < chunks.Length; i++ ) {
				DeleteChunk( chunks[i] );
			}
		}
		
		void DeleteChunk( ChunkInfo info ) {
			info.Empty = false;
			DeleteData( ref info.SolidParts );
			DeleteData( ref info.SpriteParts );
			DeleteData( ref info.TranslucentParts );
		}
		
		void DeleteData( ref ChunkPartInfo[] parts ) {
			if( parts == null ) return;
			
			for( int i = 0; i < parts.Length; i++ ) {
				api.DeleteVb( parts[i].VbId );
				api.DeleteIb( parts[i].IbId );
			}
			parts = null;
		}
		
		void CreateChunkCache() {
			int index = 0;
			for( int z = 0; z < length; z += 16 ) {
				for( int y = 0; y < height; y += 16 ) {
					for( int x = 0; x < width; x += 16 ) {
						chunks[index] = new ChunkInfo( x, y, z );
						unsortedChunks[index] = chunks[index];
						index++;
					}
				}
			}
		}
		
		static int NextMultipleOf16( int value ) {
			return ( value + 0x0F ) & ~0x0F;
		}
		
		public override void RedrawBlock( int x, int y, int z, byte block, int oldHeight, int newHeight ) {
			int cx = x >> 4;
			int cy = y >> 4;
			int cz = z >> 4;
			// NOTE: It's a lot faster to only update the chunks that are affected by the change in shadows,
			// rather than the entire column.
			int newLightcy = newHeight == -1 ? 0 : newHeight >> 4;
			int oldLightcy = oldHeight == -1 ? 0 : oldHeight >> 4;
			
			ResetChunkAndBelow( cx, cy, cz, newLightcy, oldLightcy );
			int bX = x & 0x0F; // % 16
			int bY = y & 0x0F;
			int bZ = z & 0x0F;
			
			if( bX == 0 && cx > 0 ) ResetChunkAndBelow( cx - 1, cy, cz, newLightcy, oldLightcy );
			if( bY == 0 && cy > 0 ) ResetChunkAndBelow( cx, cy - 1, cz, newLightcy, oldLightcy );
			if( bZ == 0 && cz > 0 ) ResetChunkAndBelow( cx, cy, cz - 1, newLightcy, oldLightcy );
			if( bX == 15 && cx < chunksX - 1 ) ResetChunkAndBelow( cx + 1, cy, cz, newLightcy, oldLightcy );
			if( bY == 15 && cy < chunksY - 1 ) ResetChunkAndBelow( cx, cy + 1, cz, newLightcy, oldLightcy );
			if( bZ == 15 && cz < chunksZ - 1 ) ResetChunkAndBelow( cx, cy, cz + 1, newLightcy, oldLightcy );
		}
		
		void ResetChunkAndBelow( int cx, int cy, int cz, int newLightCy, int oldLightCy ) {
			if( newLightCy == oldLightCy ) {
				ResetChunk( cx, cy, cz );
			} else {
				int cyMax = Math.Max( newLightCy, oldLightCy );
				int cyMin = Math.Min( oldLightCy, newLightCy );
				for( cy = cyMax; cy >= cyMin; cy-- ) {
					ResetChunk( cx, cy, cz );
				}
			}
		}
		
		void ResetChunk( int cx, int cy, int cz ) {
			if( cx < 0 || cy < 0 || cz < 0 ||
			   cx >= chunksX || cy >= chunksY || cz >= chunksZ ) return;
			DeleteChunk( unsortedChunks[cx + chunksX * ( cy + cz * chunksY )] );
		}
		
		Vector4[] lightCols = new Vector4[8];		
		void MakeCol( FastColour col, ref int i ) {
			Vector4 baseCol = new Vector4( col.R / 255f, col.G / 255f, col.B / 255f, 1 );
			lightCols[i++] = baseCol;
			lightCols[i++] = new Vector4( baseCol.Xyz * 0.6f, 1 );
			lightCols[i++] = new Vector4( baseCol.Xyz * 0.8f, 1 );
			lightCols[i++] = new Vector4( baseCol.Xyz * 0.5f, 1 );
		}
		
		public override void Render( double deltaTime ) {
			if( chunks == null ) return;
			UpdateSortOrder();
			UpdateChunks();
			int[] texIds = Window.TerrainAtlas1D.TexIds;
			api.UseProgram( shader.ProgramId );
			if( needToUpdateColours ) {
				int i = 0;
				needToUpdateColours = false;
				MakeCol( Window.Map.Sunlight, ref i );
				MakeCol( Window.Map.Shadowlight, ref i );
				api.SetUniformArray( shader.lightsColourLoc, lightCols );
			}
			shader.UpdateFogAndMVPState( ref Window.MVP );
			
			// Render solid and fully transparent to fill depth buffer.
			// These blocks are treated as having an alpha value of either none or full.
			api.FaceCulling = true;
			for( int batch = 0; batch < _1Dcount; batch++ ) {
				api.Bind2DTexture( texIds[batch] );
				RenderSolidBatch( batch );
			}
			api.FaceCulling = false;
			for( int batch = 0; batch < _1Dcount; batch++ ) {
				api.Bind2DTexture( texIds[batch] );
				RenderSpriteBatch( batch );
			}

			Window.MapBordersRenderer.RenderMapSides( deltaTime );
			Window.MapBordersRenderer.RenderMapEdges( deltaTime );
			
			// Render translucent(liquid) blocks. These 'blend' into other blocks.
			api.UseProgram( lShader.ProgramId );
			api.SetUniform( lShader.mvpLoc, ref Window.MVP );
			api.AlphaBlending = false;
			// First fill depth buffer
			api.ColourWrite = false;
			for( int batch = 0; batch < _1Dcount; batch++ ) {
				RenderTranslucentBatchDepthPass( batch );
			}
			
			// Then actually draw the transluscent blocks
			api.UseProgram( shader.ProgramId );
			api.AlphaBlending = true;
			api.ColourWrite = true;
			//Graphics.DepthWrite = false; TODO: test if this makes a difference.
			for( int batch = 0; batch < _1Dcount; batch++ ) {
				api.Bind2DTexture( texIds[batch] );
				RenderTranslucentBatch( batch );
			}
			//Graphics.DepthWrite = true;
			api.AlphaBlending = false;
		}

		int[] distances;
		void UpdateSortOrder() {
			Player p = Window.LocalPlayer;
			Vector3I newChunkPos = Vector3I.Floor( p.Position );
			newChunkPos.X = ( newChunkPos.X & ~0x0F ) + 8;
			newChunkPos.Y = ( newChunkPos.Y & ~0x0F ) + 8;
			newChunkPos.Z = ( newChunkPos.Z & ~0x0F ) + 8;
			if( newChunkPos != chunkPos ) {
				chunkPos = newChunkPos;
				for( int i = 0; i < distances.Length; i++ ) {
					ChunkInfo info = chunks[i];
					distances[i] = Utils.DistanceSquared( info.CentreX, info.CentreY, info.CentreZ, chunkPos.X, chunkPos.Y, chunkPos.Z );
				}
				// NOTE: Over 5x faster compared to normal comparison of IComparer<ChunkInfo>.Compare
				Array.Sort( distances, chunks );
			}
		}
		
		void UpdateChunks() {
			int chunksUpdatedThisFrame = 0;
			int adjViewDistSqr = ( Window.ViewDistance + 14 ) * ( Window.ViewDistance + 14 );
			for( int i = 0; i < chunks.Length; i++ ) {
				ChunkInfo info = chunks[i];
				if( info.Empty ) continue;
				int distSqr = distances[i];
				bool inRange = distSqr <= adjViewDistSqr;
				
				if( info.SolidParts == null && info.SpriteParts == null && info.TranslucentParts == null ) {
					if( inRange && chunksUpdatedThisFrame < 4 ) {
						Window.ChunkUpdates++;
						builder.GetDrawInfo( info.CentreX - 8, info.CentreY - 8, info.CentreZ - 8,
						                    ref info.SolidParts, ref info.SpriteParts, ref info.TranslucentParts );
						if( info.SolidParts == null && info.SpriteParts == null && info.TranslucentParts == null ) {
							info.Empty = true;
						}
						chunksUpdatedThisFrame++;
					}
				}
				info.Visible = inRange &&
					Window.Culling.SphereInFrustum( info.CentreX, info.CentreY, info.CentreZ, 14 ); // 14 ~ sqrt(3 * 8^2)
			}
		}
		
		const DrawMode mode = DrawMode.Triangles;
		const int stride = VertexPos3fTex2fCol4b.Size;
		const int maxVertex = 65536;
		const int maxIndices = maxVertex / 4 * 6;
		void RenderSolidBatch( int batch ) {
			for( int i = 0; i < chunks.Length; i++ ) {
				ChunkInfo info = chunks[i];
				if( info.SolidParts == null || !info.Visible ) continue;

				ChunkPartInfo part = info.SolidParts[batch];
				if( part.IndicesCount > 0 ) {
					if( part.IndicesCount > maxIndices ) {
						shader.DrawIndexed( mode, stride, part.VbId, part.IbId, maxIndices, 0, 0 );
						shader.DrawIndexed( mode, stride, part.VbId, part.IbId, part.IndicesCount - maxIndices, maxVertex, maxIndices );
					} else {
						shader.DrawIndexed( mode, stride, part.VbId, part.IbId, part.IndicesCount, 0, 0 );
					}
					Window.Vertices += part.IndicesCount;
				}
			}
		}
		
		void RenderSpriteBatch( int batch ) {
			for( int i = 0; i < chunks.Length; i++ ) {
				ChunkInfo info = chunks[i];
				if( info.SpriteParts == null || !info.Visible ) continue;

				ChunkPartInfo part = info.SpriteParts[batch];
				if( part.IndicesCount > 0 ) {
					shader.DrawIndexed( mode, stride, part.VbId, part.IbId, part.IndicesCount, 0, 0 );
					Window.Vertices += part.IndicesCount;
				}
			}
		}

		void RenderTranslucentBatch( int batch ) {
			for( int i = 0; i < chunks.Length; i++ ) {
				ChunkInfo info = chunks[i];
				if( info.TranslucentParts == null || !info.Visible ) continue;

				ChunkPartInfo part = info.TranslucentParts[batch];
				if( part.IndicesCount > 0 ) {
					if( part.IndicesCount > maxIndices ) {
						shader.DrawIndexed( mode, stride, part.VbId, part.IbId, maxIndices, 0, 0 );
						shader.DrawIndexed( mode, stride, part.VbId, part.IbId, part.IndicesCount - maxIndices, maxVertex, maxIndices );
					} else {
						shader.DrawIndexed( mode, stride, part.VbId, part.IbId, part.IndicesCount, 0, 0 );
					}
					Window.Vertices += part.IndicesCount;
				}
			}
		}
		
		void RenderTranslucentBatchDepthPass( int batch ) {
			for( int i = 0; i < chunks.Length; i++ ) {
				ChunkInfo info = chunks[i];
				if( info.TranslucentParts == null || !info.Visible ) continue;

				ChunkPartInfo part = info.TranslucentParts[batch];
				if( part.IndicesCount > 0 ) {
					if( part.IndicesCount > maxIndices ) {
						lShader.DrawIndexed( mode, stride, part.VbId, part.IbId, maxIndices, 0, 0 );
						lShader.DrawIndexed( mode, stride, part.VbId, part.IbId, part.IndicesCount - maxIndices, maxVertex, maxIndices );
					} else {
						lShader.DrawIndexed( mode, stride, part.VbId, part.IbId, part.IndicesCount, 0, 0 );
					}
				}
			}
		}
	}
}