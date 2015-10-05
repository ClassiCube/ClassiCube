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
			public byte OcclusionFlags;
			
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
		int _1Dcount = 1, _1DUsed = 1;
		ChunkMeshBuilder builder;
		BlockInfo info;
		
		int width, height, length;
		ChunkInfo[] chunks, unsortedChunks;
		Vector3I chunkPos = new Vector3I( int.MaxValue, int.MaxValue, int.MaxValue );
		int elementsPerBitmap = 0;
		
		public MapRenderer( Game game ) {
			this.game = game;
			_1Dcount = game.TerrainAtlas1D.TexIds.Length;
			_1DUsed = game.TerrainAtlas1D.CalcMaxUsedRow( game.TerrainAtlas, game.BlockInfo );
			
			builder = new ChunkMeshBuilder( game );
			api = game.Graphics;
			elementsPerBitmap = game.TerrainAtlas1D.elementsPerBitmap;
			info = game.BlockInfo;
			
			game.Events.TerrainAtlasChanged += TerrainAtlasChanged;
			game.Events.OnNewMap += OnNewMap;
			game.Events.OnNewMapLoaded += OnNewMapLoaded;
			game.Events.EnvVariableChanged += EnvVariableChanged;
			game.Events.BlockDefinitionChanged += BlockDefinitionChanged;
		}
		
		public void Dispose() {
			ClearChunkCache();
			chunks = null;
			unsortedChunks = null;
			game.Events.TerrainAtlasChanged -= TerrainAtlasChanged;
			game.Events.OnNewMap -= OnNewMap;
			game.Events.OnNewMapLoaded -= OnNewMapLoaded;
			game.Events.EnvVariableChanged -= EnvVariableChanged;
			game.Events.BlockDefinitionChanged -= BlockDefinitionChanged;
			builder.Dispose();
		}
		
		public void Refresh() {
			if( chunks != null && !game.Map.IsNotLoaded ) {
				ClearChunkCache();
				CreateChunkCache();
			}
			chunkPos = new Vector3I( int.MaxValue, int.MaxValue, int.MaxValue );
		}
		
		void EnvVariableChanged( object sender, EnvVarEventArgs e ) {
			if( e.Var == EnvVar.SunlightColour || e.Var == EnvVar.ShadowlightColour ) {
				Refresh();
			} else if( e.Var == EnvVar.WaterLevel ) {
				builder.clipLevel = Math.Max( 0, game.Map.GroundHeight );
				Refresh();
			}
		}

		void TerrainAtlasChanged( object sender, EventArgs e ) {
			_1Dcount = game.TerrainAtlas1D.TexIds.Length;
			bool fullResetRequired = elementsPerBitmap != game.TerrainAtlas1D.elementsPerBitmap;
			if( fullResetRequired ) {
				Refresh();
			}
			elementsPerBitmap = game.TerrainAtlas1D.elementsPerBitmap;
			_1DUsed = game.TerrainAtlas1D.CalcMaxUsedRow( game.TerrainAtlas, game.BlockInfo );
		}
		
		void BlockDefinitionChanged( object sender, EventArgs e ) {
			_1DUsed = game.TerrainAtlas1D.CalcMaxUsedRow( game.TerrainAtlas, game.BlockInfo );
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
			info.OcclusionFlags = 0;
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
			return (value + 0x0F) & ~0x0F;
		}
		
		public void RedrawBlock( int x, int y, int z, byte block, int oldHeight, int newHeight ) {
			int cx = x >> 4, bX = x & 0x0F;
			int cy = y >> 4, bY = y & 0x0F;
			int cz = z >> 4, bZ = z & 0x0F;
			// NOTE: It's a lot faster to only update the chunks that are affected by the change in shadows,
			// rather than the entire column.
			int newLightcy = newHeight < 0 ? 0 : newHeight >> 4;
			int oldLightcy = oldHeight < 0 ? 0 : oldHeight >> 4;
			ResetChunkAndBelow( cx, cy, cz, newLightcy, oldLightcy );
			
			if( bX == 0 && cx > 0 && NeedsUpdate( x, y, z, x - 1, y, z ) )
				ResetChunkAndBelow( cx - 1, cy, cz, newLightcy, oldLightcy );
			if( bY == 0 && cy > 0 && NeedsUpdate( x, y, z, x, y - 1, z ) )
				ResetChunkAndBelow( cx, cy - 1, cz, newLightcy, oldLightcy );
			if( bZ == 0 && cz > 0 && NeedsUpdate( x, y, z, x, y, z - 1 ) )
				ResetChunkAndBelow( cx, cy, cz - 1, newLightcy, oldLightcy );
			
			if( bX == 15 && cx < chunksX - 1 && NeedsUpdate( x, y, z, x + 1, y, z ) )
				ResetChunkAndBelow( cx + 1, cy, cz, newLightcy, oldLightcy );
			if( bY == 15 && cy < chunksY - 1 && NeedsUpdate( x, y, z, x, y + 1, z ) )
				ResetChunkAndBelow( cx, cy + 1, cz, newLightcy, oldLightcy );
			if( bZ == 15 && cz < chunksZ - 1 && NeedsUpdate( x, y, z, x, y, z + 1 ) )
				ResetChunkAndBelow( cx, cy, cz + 1, newLightcy, oldLightcy );
		}
		
		bool NeedsUpdate( int x1, int y1, int z1, int x2, int y2, int z2 ) {
			byte b1 = game.Map.SafeGetBlock( x1, y1, z1 );
			byte b2 = game.Map.SafeGetBlock( x2, y2, z2 );
			return (!info.IsOpaque[b1] && info.IsOpaque[b2]) || !(info.IsOpaque[b1] && b2 == 0);
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
			UpdateChunks( deltaTime );
			//SimpleOcclusionCulling();
			
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
		}
		
		int chunksTarget = 4;
		const double targetTime = (1.0 / 30) + 0.01;
		void UpdateChunks( double deltaTime ) {
			int chunksUpdatedThisFrame = 0;
			int adjViewDistSqr = ( game.ViewDistance + 14 ) * ( game.ViewDistance + 14 );
			chunksTarget += deltaTime < targetTime ? 1 : -1; // build more chunks if 30 FPS or over, otherwise slowdown.
			Utils.Clamp( ref chunksTarget, 4, 12 );
			
			for( int i = 0; i < chunks.Length; i++ ) {
				ChunkInfo info = chunks[i];
				if( info.Empty ) continue;
				int distSqr = distances[i];
				bool inRange = distSqr <= adjViewDistSqr;
				
				if( info.NormalParts == null && info.TranslucentParts == null ) {
					if( inRange && chunksUpdatedThisFrame < chunksTarget ) {
						game.ChunkUpdates++;
						builder.GetDrawInfo( info.CentreX - 8, info.CentreY - 8, info.CentreZ - 8,
						                    ref info.NormalParts, ref info.TranslucentParts, ref info.OcclusionFlags );
						
						if( info.NormalParts == null && info.TranslucentParts == null )
							info.Empty = true;
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
			
			for( int batch = 0; batch < _1DUsed; batch++ ) {
				api.BindTexture( texIds[batch] );
				RenderNormalBatch( batch );
			}
			api.AlphaTest = false;
			api.Texturing = false;
		}
		
		// Render translucent(liquid) blocks. These 'blend' into other blocks.
		void RenderTranslucent() {
			Block block = game.LocalPlayer.BlockAtHead;
			drawAllFaces = block == Block.Water || block == Block.StillWater;
			// First fill depth buffer
			int[] texIds = game.TerrainAtlas1D.TexIds;
			api.BeginVbBatch( VertexFormat.Pos3fTex2fCol4b );
			api.Texturing = false;
			api.AlphaBlending = false;
			api.ColourWrite = false;
			for( int batch = 0; batch < _1DUsed; batch++ ) {
				RenderTranslucentBatchDepthPass( batch );
			}
			
			// Then actually draw the transluscent blocks
			api.AlphaBlending = true;
			api.Texturing = true;
			api.ColourWrite = true;
			api.DepthWrite = false; // we already calculated depth values in depth pass
			
			for( int batch = 0; batch < _1DUsed; batch++ ) {
				api.BindTexture( texIds[batch] );
				RenderTranslucentBatch( batch );
			}
			api.DepthWrite = true;
			api.AlphaTest = false;
			api.AlphaBlending = false;
			api.Texturing = false;
		}
		
		void SimpleOcclusionCulling() { // TODO: broken
			Vector3 p = game.LocalPlayer.EyePosition;
			for( int i = 0; i < chunks.Length; i++ ) {
				ChunkInfo chunk = chunks[i];
				chunk.Visible = true;
				int cx = chunk.CentreX >> 4;
				int cy = chunk.CentreY >> 4;
				int cz = chunk.CentreZ >> 4;
				
				int x1 = chunk.CentreX - 8, x2 = chunk.CentreX + 8;
				int y1 = chunk.CentreY - 8, y2 = chunk.CentreY + 8;
				int z1 = chunk.CentreZ - 8, z2 = chunk.CentreZ + 8;
				float minDist = float.PositiveInfinity;
				int xOffset = -1, yOffset = 0, zOffset = 0;
				int flags = 0x1;
				
				// TODO: two axes with same distance
				minDist = DistToRecSquared( p, x1, y1, z1, x1, y2, z2 ); // left
				float rightDist = DistToRecSquared( p, x2, y1, z1, x2, y2, z2 );
				if( rightDist < minDist ) {
					minDist = rightDist; xOffset = 1;
				}
				
				float frontDist = DistToRecSquared( p, x1, y1, z1, x2, y2, z1 );
				if( frontDist < minDist ) {
					minDist = frontDist; xOffset = 0; zOffset = -1; flags = 2;
				}
				
				float backDist = DistToRecSquared( p, x1, y1, z2, x2, y2, z2 );
				if( backDist < minDist ) {
					minDist = backDist; xOffset = 0; zOffset = 1; flags = 2;
				}
				
				float bottomDist = DistToRecSquared( p, x1, y1, z1, x2, y1, z2 );
				if( bottomDist < minDist ) {
					minDist = bottomDist; xOffset = 0; zOffset = 0; yOffset = -1; flags = 4;
				}
				
				float topDist = DistToRecSquared( p, x1, y2, z1, x2, y2, z2 );
				if( topDist < minDist ) {
					minDist = topDist; xOffset = 0; zOffset = 0; yOffset = -1; flags = 4;
				}
				
				if( (cx + xOffset) >= 0 && (cy + yOffset) >= 0 && (cz + zOffset) >= 0 &&
				   (cx + xOffset) < chunksX && (cy + yOffset) < chunksY && (cz + zOffset) < chunksZ ) {
					cx += xOffset; cy += yOffset; cz += zOffset;
					ChunkInfo neighbour = unsortedChunks[cx + chunksX * (cy + cz * chunksY)];
					if( (neighbour.OcclusionFlags & flags) != 0 ) {
						Console.WriteLine( "HIDE" );
						chunks[i].Visible = false;
					}
				}
			}
			chunks[0].Visible = true;
		}
		
		static float DistToRecSquared( Vector3 p, int x1, int y1, int z1, int x2, int y2, int z2 ) {
			float dx = Math.Max( x1 - p.X, Math.Max( 0, p.X - x2 ) );
			float dy = Math.Max( y1 - p.Y, Math.Max( 0, p.Y - y2 ) );
			float dz = Math.Max( z1 - p.Z, Math.Max( 0, p.Z - z2 ) );
			return dx * dx + dy * dy + dz * dz;
		}
	}
}