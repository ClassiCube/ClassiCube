using System;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp {
	
	// TODO: optimise chunk rendering
	//  --> reduce the two passes: liquid pass only needs 1 small texture
	//  --> use indices.
	public class MapRenderer : IDisposable {
		
		struct Point3S {
			
			public short X, Y, Z;
			
			public Point3S( int x, int y, int z ) {
				X = (short)x;
				Y = (short)y;
				Z = (short)z;
			}
			
			public override string ToString() {
				return X + "," + Y + "," + Z;
			}
		}
		
		class ChunkInfo {
			
			public Point3S Location;
			
			public bool Visible = true;
			public bool Empty = false;
			
			public ChunkDrawInfo DrawInfo;
			
			public ChunkInfo( int x, int y, int z ) {
				Location = new Point3S( x, y, z );
			}
		}
		
		public Game Window;
		public OpenGLApi Graphics;
		ChunkMeshBuilder builder;
		
		int width, height, length;
		ChunkInfo[] chunks, unsortedChunks;
		Vector3I chunkPos = new Vector3I( int.MaxValue, int.MaxValue, int.MaxValue );
		
		public readonly bool UsesLighting;
		internal MapShader shader;
		internal MapPackedLiquidDepthShader transluscentShader;
		internal MapPackedShader packedShader;
		
		public MapRenderer( Game window ) {
			Window = window;
			Graphics = window.Graphics;
			shader = new MapShader();
			shader.Initialise( Graphics );
			transluscentShader = new MapPackedLiquidDepthShader();
			transluscentShader.Initialise( Graphics );
			packedShader = new MapPackedShader();
			packedShader.Initialise( Graphics );
			builder = new ChunkMeshBuilderTex2Col4( window, this );
			
			UsesLighting = builder.UsesLighting;
			Window.OnNewMap += OnNewMap;
			Window.OnNewMapLoaded += OnNewMapLoaded;
			Window.EnvVariableChanged += EnvVariableChanged;
		}
		
		public void Dispose() {
			ClearChunkCache();
			chunks = null;
			unsortedChunks = null;
			Window.OnNewMap -= OnNewMap;
			Window.OnNewMapLoaded -= OnNewMapLoaded;
			Window.EnvVariableChanged -= EnvVariableChanged;
		}
		
		public void Refresh() {
			if( chunks != null && !Window.Map.IsNotLoaded ) {
				ClearChunkCache();
				CreateChunkCache();
			}
		}
		
		void EnvVariableChanged( object sender, EnvVariableEventArgs e ) {
			if( ( e.Variable == EnvVariable.SunlightColour ||
			     e.Variable == EnvVariable.ShadowlightColour ) && UsesLighting ) {
				Refresh();
			}
		}
		
		void OnNewMap( object sender, EventArgs e ) {
			Window.ChunkUpdates = 0;
			ClearChunkCache();
			chunks = null;
			unsortedChunks = null;
			chunkPos = new Vector3I( int.MaxValue, int.MaxValue, int.MaxValue );
			builder.OnNewMap();
		}
		
		int chunksX, chunksY, chunksZ;
		void OnNewMapLoaded( object sender, EventArgs e ) {
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
			builder.OnNewMapLoaded();
		}
		
		void ClearChunkCache() {
			if( chunks == null ) return;
			for( int i = 0; i < chunks.Length; i++ ) {
				ChunkInfo info = chunks[i];
				DeleteChunk( chunks[i] );
			}
		}
		
		void DeleteChunk( ChunkInfo info ) {
			ChunkDrawInfo drawInfo = info.DrawInfo;
			info.Empty = false;
			if( drawInfo == null ) return;
			
			Graphics.DeleteIndexedVb( drawInfo.SpritePart.Id );
			Graphics.DeleteIndexedVb( drawInfo.TranslucentPart.Id );
			Graphics.DeleteIndexedVb( drawInfo.SolidPart.Id );
			info.DrawInfo = null;
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
		
		public void RedrawBlock( int x, int y, int z, byte block, int oldHeight, int newHeight ) {
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
			if( UsesLighting ) {
				if( newLightCy == oldLightCy ) {
					ResetChunk( cx, cy, cz );
				} else {
					int cyMax = Math.Max( newLightCy, oldLightCy );
					int cyMin = Math.Min( oldLightCy, newLightCy );
					for( cy = cyMax; cy >= cyMin; cy-- ) {
						ResetChunk( cx, cy, cz );
					}
				}
			} else {
				ResetChunk( cx, cy, cz );
			}
		}
		
		void ResetChunk( int cx, int cy, int cz ) {
			if( cx < 0 || cy < 0 || cz < 0 ||
			   cx >= chunksX || cy >= chunksY || cz >= chunksZ ) return;
			DeleteChunk( unsortedChunks[cx + chunksX * ( cy + cz * chunksY )] );
		}
		
		void SetAttributes() {
			Graphics.UseProgram( packedShader.ProgramId );
			Graphics.SetUniform( packedShader.mvpLoc, ref Window.mvp );
			Graphics.SetUniform( packedShader.fogColLoc, ref Graphics.FogColour );
			Graphics.SetUniform( packedShader.fogDensityLoc, Graphics.FogDensity );
			Graphics.SetUniform( packedShader.fogEndLoc, Graphics.FogEnd );
			Graphics.SetUniform( packedShader.fogModeLoc, (int)Graphics.FogMode );
		}
		
		public void Render( double deltaTime ) {
			if( chunks == null ) return;
			Window.Vertices = 0;
			UpdateSortOrder();
			UpdateChunks();
			
			// Render solid and fully transparent to fill depth buffer.
			// These blocks are treated as having an alpha value of either none or full.
			Graphics.UseProgram( packedShader.ProgramId );
			SetAttributes();
			Graphics.Bind2DTexture( Window.TerrainAtlasTexId );
			Graphics.FaceCulling = true;
			RenderSolidBatch();
			Graphics.FaceCulling = false;
			RenderSpriteBatch();
			
			Graphics.UseProgram( shader.ProgramId );
			Window.MapEnvRenderer.RenderMapSides( deltaTime );
			Window.MapEnvRenderer.RenderMapEdges( deltaTime );
			
			// Render translucent(liquid) blocks. These 'blend' into other blocks.
			Graphics.AlphaBlending = false;
			
			// First fill depth buffer
			Graphics.UseProgram( transluscentShader.ProgramId );
			Graphics.SetUniform( transluscentShader.mvpLoc, ref Window.mvp );
			Graphics.DepthTestFunc( CompareFunc.LessEqual );
			Graphics.ColourMask( false, false, false, false );
			RenderTranslucentBatchNoAdd();
			
			// Then actually draw the transluscent blocks
			Graphics.UseProgram( packedShader.ProgramId );
			Graphics.AlphaBlending = true;
			Graphics.ColourMask( true, true, true, true );
			Graphics.Bind2DTexture( Window.TerrainAtlasTexId );
			RenderTranslucentBatch();
			Graphics.DepthTestFunc( CompareFunc.Less );
			
			Graphics.AlphaBlending = false;
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
					Point3S loc = info.Location;
					distances[i] = Utils.DistanceSquared( loc.X + 8, loc.Y + 8, loc.Z + 8, chunkPos.X, chunkPos.Y, chunkPos.Z );
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
				Point3S loc = info.Location;
				int distSqr = distances[i];
				bool inRange = distSqr <= adjViewDistSqr;
				
				if( info.DrawInfo == null ) {
					if( inRange && chunksUpdatedThisFrame < 4 ) {
						Window.ChunkUpdates++;
						info.DrawInfo = builder.GetDrawInfo( loc.X, loc.Y, loc.Z );
						if( info.DrawInfo == null ) {
							info.Empty = true;
						}
						chunksUpdatedThisFrame++;
					}
				}
				info.Visible = inRange && Window.Culling.SphereInFrustum( loc.X + 8, loc.Y + 8, loc.Z + 8, 14 ); // 14 ~ sqrt(3 * 8^2)
			}
		}
		
		// TODO: there's probably a better way of doing this.
		void RenderSolidBatch() {
			for( int i = 0; i < chunks.Length; i++ ) {
				ChunkInfo info = chunks[i];
				if( info.DrawInfo == null || !info.Visible ) continue;

				ChunkPartInfo drawInfo = info.DrawInfo.SolidPart;
				if( drawInfo.IndicesCount == 0 ) continue;
				
				Vector3 offset = new Vector3( info.Location.X, info.Location.Y, info.Location.Z );
				Graphics.SetUniform( packedShader.offsetLoc, ref offset );
				builder.Render( drawInfo );
				Window.Vertices += drawInfo.IndicesCount;
			}
		}
		
		void RenderSpriteBatch() {
			for( int i = 0; i < chunks.Length; i++ ) {
				ChunkInfo info = chunks[i];
				if( info.DrawInfo == null || !info.Visible ) continue;

				ChunkPartInfo drawInfo = info.DrawInfo.SpritePart;
				if( drawInfo.IndicesCount == 0 ) continue;
				
				Vector3 offset = new Vector3( info.Location.X, info.Location.Y, info.Location.Z );
				Graphics.SetUniform( packedShader.offsetLoc, ref offset );
				builder.Render( drawInfo );
				Window.Vertices += drawInfo.IndicesCount;
			}
		}

		void RenderTranslucentBatch() {
			for( int i = 0; i < chunks.Length; i++ ) {
				ChunkInfo info = chunks[i];
				if( info.DrawInfo == null || !info.Visible ) continue;

				ChunkPartInfo drawInfo = info.DrawInfo.TranslucentPart;
				if( drawInfo.IndicesCount == 0 ) continue;
				
				Vector3 offset = new Vector3( info.Location.X, info.Location.Y, info.Location.Z );
				Graphics.SetUniform( packedShader.offsetLoc, ref offset );
				builder.Render( drawInfo );
				Window.Vertices += drawInfo.IndicesCount;
			}
		}
		
		void RenderTranslucentBatchNoAdd() {
			for( int i = 0; i < chunks.Length; i++ ) {
				ChunkInfo info = chunks[i];
				if( info.DrawInfo == null || !info.Visible ) continue;

				ChunkPartInfo drawInfo = info.DrawInfo.TranslucentPart;
				if( drawInfo.IndicesCount == 0 ) continue;
				
				Vector3 offset = new Vector3( info.Location.X, info.Location.Y, info.Location.Z );
				Graphics.SetUniform( transluscentShader.offsetLoc, ref offset );
				builder.RenderLiquidDepthPass( drawInfo );
			}
		}
	}
}