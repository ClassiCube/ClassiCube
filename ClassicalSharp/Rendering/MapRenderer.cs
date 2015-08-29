using System;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp {
	
	// TODO: optimise chunk rendering
	//  --> reduce iterations: liquid and sprite pass only need 1 row
	public partial class MapRenderer : IDisposable {
		
		class ChunkInfo {
			
			public ushort CentreX, CentreY, CentreZ;
			public bool Visible = true;
			public bool Empty = false;
			public bool DrawLeft, DrawRight, DrawFront, DrawBack, DrawBottom, DrawTop;
			
			public ChunkPartInfo[] NormalParts;
			public ChunkPartInfo[] TranslucentParts;
			
			public ChunkInfo( int x, int y, int z ) {
				CentreX = (ushort)( x + 8 );
				CentreY = (ushort)( y + 8 );
				CentreZ = (ushort)( z + 8 );
			}
		}
		
		Game game;
		IGraphicsApi api;
		int _1Dcount = 1;
		ChunkMeshBuilder builder;
		
		int width, height, length;
		ChunkInfo[] chunks, unsortedChunks;
		Vector3I chunkPos = new Vector3I( int.MaxValue, int.MaxValue, int.MaxValue );
		int elementsPerBitmap = 0;
		
		public MapRenderer( Game window ) {
			game = window;
			_1Dcount = window.TerrainAtlas1D.TexIds.Length;
			builder = new ChunkMeshBuilder( window );
			api = window.Graphics;
			elementsPerBitmap = window.TerrainAtlas1D.elementsPerBitmap;
			game.TerrainAtlasChanged += TerrainAtlasChanged;
			game.OnNewMap += OnNewMap;
			game.OnNewMapLoaded += OnNewMapLoaded;
			game.EnvVariableChanged += EnvVariableChanged;
		}
		
		public void Dispose() {
			ClearChunkCache();
			chunks = null;
			unsortedChunks = null;
			game.TerrainAtlasChanged -= TerrainAtlasChanged;
			game.OnNewMap -= OnNewMap;
			game.OnNewMapLoaded -= OnNewMapLoaded;
			game.EnvVariableChanged -= EnvVariableChanged;
			builder.Dispose();
		}
		
		public void Refresh() {
			if( chunks != null && !game.Map.IsNotLoaded ) {
				ClearChunkCache();
				CreateChunkCache();
			}
		}
		
		void EnvVariableChanged( object sender, EnvVariableEventArgs e ) {
			if( e.Var == EnvVariable.SunlightColour || e.Var == EnvVariable.ShadowlightColour ) {
				Refresh();
			}
		}

		void TerrainAtlasChanged( object sender, EventArgs e ) {
			_1Dcount = game.TerrainAtlas1D.TexIds.Length;
			bool fullResetRequired = elementsPerBitmap != game.TerrainAtlas1D.elementsPerBitmap;
			if( fullResetRequired ) {
				Refresh();
				chunkPos = new Vector3I( int.MaxValue, int.MaxValue, int.MaxValue );
			}
			elementsPerBitmap = game.TerrainAtlas1D.elementsPerBitmap;			
		}
		
		void OnNewMap( object sender, EventArgs e ) {
			game.ChunkUpdates = 0;
			ClearChunkCache();
			chunks = null;
			unsortedChunks = null;
			chunkPos = new Vector3I( int.MaxValue, int.MaxValue, int.MaxValue );
			builder.OnNewMap();
		}
		
		int chunksX, chunksY, chunksZ;
		void OnNewMapLoaded( object sender, EventArgs e ) {
			width = NextMultipleOf16( game.Map.Width );
			height = NextMultipleOf16( game.Map.Height );
			length = NextMultipleOf16( game.Map.Length );
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
				DeleteChunk( chunks[i] );
			}
		}
		
		void DeleteChunk( ChunkInfo info ) {
			info.Empty = false;
			DeleteData( ref info.NormalParts );
			DeleteData( ref info.TranslucentParts );
		}
		
		void DeleteData( ref ChunkPartInfo[] parts ) {
			if( parts == null ) return;
			
			for( int i = 0; i < parts.Length; i++ ) {
				api.DeleteVb( parts[i].VbId );
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
		
		public void Render( double deltaTime ) {
			if( chunks == null ) return;
			UpdateSortOrder();
			UpdateChunks();
			
			RenderNormal();
			game.MapEnvRenderer.Render( deltaTime );
			RenderTranslucent();
		}

		int[] distances;
		void UpdateSortOrder() {
			Player p = game.LocalPlayer;
			Vector3I newChunkPos = Vector3I.Floor( p.EyePosition );
			newChunkPos.X = ( newChunkPos.X & ~0x0F ) + 8;
			newChunkPos.Y = ( newChunkPos.Y & ~0x0F ) + 8;
			newChunkPos.Z = ( newChunkPos.Z & ~0x0F ) + 8;
			if( newChunkPos == chunkPos ) return;
			
			chunkPos = newChunkPos;
			for( int i = 0; i < distances.Length; i++ ) {
				ChunkInfo info = chunks[i];
				distances[i] = Utils.DistanceSquared( info.CentreX, info.CentreY, info.CentreZ, chunkPos.X, chunkPos.Y, chunkPos.Z );
			}
			// NOTE: Over 5x faster compared to normal comparison of IComparer<ChunkInfo>.Compare
			Array.Sort( distances, chunks );
			
			Vector3I pPos = newChunkPos;
			for( int i = 0; i < chunks.Length; i++ ) {
				ChunkInfo info = chunks[i];
				int dX1 = ( info.CentreX - 8 ) - pPos.X, dX2 = ( info.CentreX + 8 ) - pPos.X;
				int dY1 = ( info.CentreY - 8 ) - pPos.Y, dY2 = ( info.CentreY + 8 ) - pPos.Y;
				int dZ1 = ( info.CentreZ - 8 ) - pPos.Z, dZ2 = ( info.CentreZ + 8 ) - pPos.Z;
				
				// Back face culling: make sure that the chunk is definitely entirely back facing.
				info.DrawLeft = !( dX1 <= 0 && dX2 <= 0 );
				info.DrawRight = !( dX1 >= 0 && dX2 >= 0 );
				info.DrawFront = !( dZ1 <= 0 && dZ2 <= 0 );
				info.DrawBack = !( dZ1 >= 0 && dZ2 >= 0 );
				info.DrawBottom = !( dY1 <= 0 && dY2 <= 0 );
				info.DrawTop = !( dY1 >= 0 && dY2 >= 0 );
			}
		}
		
		void UpdateChunks() {
			int chunksUpdatedThisFrame = 0;
			int adjViewDistSqr = ( game.ViewDistance + 14 ) * ( game.ViewDistance + 14 );
			for( int i = 0; i < chunks.Length; i++ ) {
				ChunkInfo info = chunks[i];
				if( info.Empty ) continue;
				int distSqr = distances[i];
				bool inRange = distSqr <= adjViewDistSqr;
				
				if( info.NormalParts == null && info.TranslucentParts == null ) {
					if( inRange && chunksUpdatedThisFrame < 4 ) {
						game.ChunkUpdates++;
						builder.GetDrawInfo( info.CentreX - 8, info.CentreY - 8, info.CentreZ - 8,
						                    ref info.NormalParts, ref info.TranslucentParts );
						if( info.NormalParts == null && info.TranslucentParts == null ) {
							info.Empty = true;
						}
						chunksUpdatedThisFrame++;
					}
				}
				info.Visible = inRange &&
					game.Culling.SphereInFrustum( info.CentreX, info.CentreY, info.CentreZ, 14 ); // 14 ~ sqrt(3 * 8^2)
			}
		}
		
		// Render solid and fully transparent to fill depth buffer.
		// These blocks are treated as having an alpha value of either none or full.
		void RenderNormal() {
			int[] texIds = game.TerrainAtlas1D.TexIds;
			api.BeginVbBatch( VertexFormat.Pos3fTex2fCol4b );
			api.Texturing = true;
			api.AlphaTest = true;
			
			for( int batch = 0; batch < _1Dcount; batch++ ) {
				api.BindTexture( texIds[batch] );
				RenderNormalBatch( batch );
			}
			api.AlphaTest = false;
			api.Texturing = false;
		}
		
		// Render translucent(liquid) blocks. These 'blend' into other blocks.
		void RenderTranslucent() {
			// First fill depth buffer
			int[] texIds = game.TerrainAtlas1D.TexIds;
			api.BeginVbBatch( VertexFormat.Pos3fTex2fCol4b );
			api.Texturing = false;
			api.AlphaBlending = false;
			api.ColourWrite = false;
			for( int batch = 0; batch < _1Dcount; batch++ ) {
				RenderTranslucentBatchDepthPass( batch );
			}
			
			// Then actually draw the transluscent blocks
			api.AlphaBlending = true;
			api.Texturing = true;
			api.ColourWrite = true;
			api.DepthWrite = false; // we already calculated depth values in depth pass
			
			for( int batch = 0; batch < _1Dcount; batch++ ) {
				api.BindTexture( texIds[batch] );
				RenderTranslucentBatch( batch );
			}
			api.DepthWrite = true;
			api.AlphaTest = false;
			api.AlphaBlending = false;
			api.Texturing = false;
		}
	}
}