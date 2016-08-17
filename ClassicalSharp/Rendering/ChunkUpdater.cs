// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.Entities;
using ClassicalSharp.Events;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Map;
using OpenTK;

namespace ClassicalSharp.Renderers {
	
	/// <summary> Manages the process of building/deleting chunk meshes,
	/// in addition to calculating the visibility of chunks. </summary>
	public sealed class ChunkUpdater : IDisposable {
		
		Game game;
		ChunkMeshBuilder builder;
		BlockInfo info;
		
		int width, height, length;
		internal int[] distances;
		internal Vector3I chunkPos = new Vector3I( int.MaxValue );
		int elementsPerBitmap = 0;
		MapRenderer renderer;
		
		public ChunkUpdater( Game game, MapRenderer renderer ) {
			this.game = game;
			this.renderer = renderer;
			info = game.BlockInfo;
			InitMeshBuilder();
			
			game.Events.TerrainAtlasChanged += TerrainAtlasChanged;
			game.WorldEvents.OnNewMap += OnNewMap;
			game.WorldEvents.OnNewMapLoaded += OnNewMapLoaded;
			game.WorldEvents.EnvVariableChanged += EnvVariableChanged;
			game.Events.BlockDefinitionChanged += BlockDefinitionChanged;
			game.Events.ViewDistanceChanged += ViewDistanceChanged;
			game.Events.ProjectionChanged += ProjectionChanged;
		}
		
		public void InitMeshBuilder() {
			if( builder != null ) builder.Dispose();
			
			if( game.SmoothLighting )
				builder = new AdvLightingMeshBuilder();
			else
				builder = new NormalMeshBuilder();
			
			builder.Init( game );
			builder.OnNewMapLoaded();
		}
		
		public void Dispose() {
			ClearChunkCache();
			renderer.chunks = null;
			renderer.unsortedChunks = null;
			game.Events.TerrainAtlasChanged -= TerrainAtlasChanged;
			game.WorldEvents.OnNewMap -= OnNewMap;
			game.WorldEvents.OnNewMapLoaded -= OnNewMapLoaded;
			game.WorldEvents.EnvVariableChanged -= EnvVariableChanged;
			game.WorldEvents.BlockDefinitionChanged -= BlockDefinitionChanged;
			game.Events.ViewDistanceChanged -= ViewDistanceChanged;
			game.Events.ProjectionChanged -= ProjectionChanged;
			builder.Dispose();
		}
		
		public void Refresh() {
			chunkPos = new Vector3I( int.MaxValue );
			renderer.totalUsed = new int[game.TerrainAtlas1D.TexIds.Length];
			if( renderer.chunks == null || game.World.IsNotLoaded ) return;
			ClearChunkCache();
			ResetChunkCache();
		}
		
		void RefreshBorders( int clipLevel ) {
			chunkPos = new Vector3I( int.MaxValue );
			if( renderer.chunks == null || game.World.IsNotLoaded ) return;
			
			int index = 0;
			for( int z = 0; z < chunksZ; z++ )
				for( int y = 0; y < chunksY; y++ )
					for( int x = 0; x < chunksX; x++ )
			{
				bool isBorder = x == 0 || z == 0 || x == (chunksX - 1) || z == (chunksZ - 1);
				if( isBorder && (y * 16) < clipLevel )
					DeleteChunk( renderer.unsortedChunks[index], true );
				index++;
			}
		}
		
		void EnvVariableChanged( object sender, EnvVarEventArgs e ) {
			if( e.Var == EnvVar.SunlightColour || e.Var == EnvVar.ShadowlightColour ) {
				Refresh();
			} else if( e.Var == EnvVar.EdgeLevel ) {
				int oldClip = builder.clipLevel;
				builder.clipLevel = Math.Max( 0, game.World.Env.SidesHeight );
				RefreshBorders( Math.Max( oldClip, builder.clipLevel ) );
			}
		}

		void TerrainAtlasChanged( object sender, EventArgs e ) {
			if( renderer._1DUsed == -1 ) {
				renderer.totalUsed = new int[game.TerrainAtlas1D.TexIds.Length];
			} else {
				bool refreshRequired = elementsPerBitmap != game.TerrainAtlas1D.elementsPerBitmap;
				if( refreshRequired ) Refresh();
			}
			
			renderer._1DUsed = game.TerrainAtlas1D.CalcMaxUsedRow( game.TerrainAtlas, info );
			elementsPerBitmap = game.TerrainAtlas1D.elementsPerBitmap;
			ResetUsedFlags();
		}
		
		void BlockDefinitionChanged( object sender, EventArgs e ) {
			renderer._1DUsed = game.TerrainAtlas1D.CalcMaxUsedRow( game.TerrainAtlas, info );
			ResetUsedFlags();
			Refresh();
		}
		
		void ProjectionChanged( object sender, EventArgs e ) {
			lastCamPos = Utils.MaxPos();
		}
		
		void OnNewMap( object sender, EventArgs e ) {
			game.ChunkUpdates = 0;
			ClearChunkCache();
			for( int i = 0; i < renderer.totalUsed.Length; i++ )
				renderer.totalUsed[i] = 0;
			
			renderer.chunks = null;
			renderer.unsortedChunks = null;
			chunkPos = new Vector3I( int.MaxValue, int.MaxValue, int.MaxValue );
		}
		
		void ViewDistanceChanged( object sender, EventArgs e ) {
			lastCamPos = Utils.MaxPos();
			lastYaw = float.MaxValue;
			lastPitch = float.MaxValue;
		}
		
		internal void ResetUsedFlags() {
			int count = renderer._1DUsed;
			bool[] used = renderer.usedTranslucent;
			if( used == null || count > used.Length ) {
				renderer.usedTranslucent = new bool[count];
				renderer.usedNormal = new bool[count];
				renderer.pendingTranslucent = new bool[count];
				renderer.pendingNormal = new bool[count];
			}
			
			for( int i = 0; i < count; i++ ) {
				renderer.pendingTranslucent[i] = true;
				renderer.usedTranslucent[i] = false;
				renderer.pendingNormal[i] = true;
				renderer.usedNormal[i] = false;
			}
		}
		
		int chunksX, chunksY, chunksZ;
		void OnNewMapLoaded( object sender, EventArgs e ) {
			width = NextMultipleOf16( game.World.Width );
			height = NextMultipleOf16( game.World.Height );
			length = NextMultipleOf16( game.World.Length );
			chunksX = width >> 4;
			chunksY = height >> 4;
			chunksZ = length >> 4;
			
			int count = chunksX * chunksY * chunksZ;
			if( renderer.chunks == null || renderer.chunks.Length != count ) {
				renderer.chunks = new ChunkInfo[count];
				renderer.unsortedChunks = new ChunkInfo[count];
				renderer.renderChunks = new ChunkInfo[count];
				distances = new int[count];
			}
			CreateChunkCache();
			builder.OnNewMapLoaded();
			lastCamPos = Utils.MaxPos();
			lastYaw = float.MaxValue;
			lastPitch = float.MaxValue;
		}
		
		void CreateChunkCache() {
			int index = 0;
			for( int z = 0; z < length; z += 16 )
				for( int y = 0; y < height; y += 16 )
					for( int x = 0; x < width; x += 16 )
			{
				renderer.chunks[index] = new ChunkInfo( x, y, z );
				renderer.unsortedChunks[index] = renderer.chunks[index];
				renderer.renderChunks[index] = renderer.chunks[index];
				distances[index] = 0;
				index++;
			}
		}
		
		void ResetChunkCache() {
			int index = 0;
			for( int z = 0; z < length; z += 16 )
				for( int y = 0; y < height; y += 16 )
					for( int x = 0; x < width; x += 16 )
			{
				renderer.unsortedChunks[index].Reset( x, y, z );
				index++;
			}
		}
		
		void ClearChunkCache() {
			if( renderer.chunks == null ) return;
			for( int i = 0; i < renderer.chunks.Length; i++ )
				DeleteChunk( renderer.chunks[i], false );
			renderer.totalUsed = new int[game.TerrainAtlas1D.TexIds.Length];
		}
		
		void DeleteChunk( ChunkInfo info, bool decUsed ) {
			info.Empty = false;
			#if OCCLUSION
			info.OcclusionFlags = 0;
			info.OccludedFlags = 0;
			#endif
			
			if( info.NormalParts != null )
				DeleteData( ref info.NormalParts, decUsed );
			if( info.TranslucentParts != null )
				DeleteData( ref info.TranslucentParts, decUsed );
		}
		
		void DeleteData( ref ChunkPartInfo[] parts, bool decUsed ) {
			if( decUsed ) DecrementUsed( parts );
			
			for( int i = 0; i < parts.Length; i++ )
				game.Graphics.DeleteVb( parts[i].VbId );
			parts = null;
		}
		
		static int NextMultipleOf16( int value ) {
			return (value + 0x0F) & ~0x0F;
		}
		
		
		public void RedrawBlock( int x, int y, int z, byte block, int oldHeight, int newHeight ) {
			int cx = x >> 4, cy = y >> 4, cz = z >> 4;
			
			// NOTE: It's a lot faster to only update the chunks that are affected by the change in shadows,
			// rather than the entire column.
			int newCy = newHeight < 0 ? 0 : newHeight >> 4;
			int oldCy = oldHeight < 0 ? 0 : oldHeight >> 4;
			int minCy = Math.Min( oldCy, newCy ), maxCy = Math.Max( oldCy, newCy );
			ResetColumn( cx, cy, cz, minCy, maxCy );
			World world = game.World;
			
			int bX = x & 0x0F, bY = y & 0x0F, bZ = z & 0x0F;			
			if( bX == 0 && cx > 0 )
				ResetNeighbour( x - 1, y, z, block, cx - 1, cy, cz, minCy, maxCy );
			if( bY == 0 && cy > 0 && Needs( block, world.GetBlock( x, y - 1, z ) ) )
				ResetChunk( cx, cy - 1, cz );
			if( bZ == 0 && cz > 0 )
				ResetNeighbour( x, y, z - 1, block, cx, cy, cz - 1, minCy, maxCy );
			
			if( bX == 15 && cx < chunksX - 1 )
				ResetNeighbour( x + 1, y, z, block, cx + 1, cy, cz, minCy, maxCy );
			if( bY == 15 && cy < chunksY - 1 && Needs( block, world.GetBlock( x, y + 1, z ) ) )
				ResetChunk( cx, cy + 1, cz );
			if( bZ == 15 && cz < chunksZ - 1 )
				ResetNeighbour( x, y, z + 1, block, cx, cy, cz + 1, minCy, maxCy );
		}
		
		bool Needs( byte block, byte other ) { return !info.IsOpaque[block] || !info.IsAir[other]; }
		
		void ResetNeighbour( int x, int y, int z, byte block,
		                    int cx, int cy, int cz, int minCy, int maxCy ) {
			World world = game.World;
			if( minCy == maxCy ) {
				int index = x + world.Width * (z + y * world.Length);
				ResetNeighourChunk( cx, cy, cz, block, y, index, y );
			} else {
				for( cy = maxCy; cy >= minCy; cy-- ) {
					int maxY = Math.Min( world.Height - 1, (cy << 4) + 15 );
					int index = x + world.Width * (z + maxY * world.Length);
					ResetNeighourChunk( cx, cy, cz, block, maxY, index, y );
				}
			}
		}
		
		void ResetNeighourChunk( int cx, int cy, int cz, byte block,
		                        int y, int index, int nY ) {
			World world = game.World;
			int minY = cy << 4;
			
			// Update if any blocks in the chunk are affected by light change
			for( ; y >= minY; y--) {
				byte other = world.blocks[index];
				bool affected = y == nY ? Needs( block, other ) : !info.IsAir[other];
				if( affected ) { ResetChunk( cx, cy, cz ); return; }
				index -= world.Width * world.Length;
			}
		}
		
		void ResetColumn( int cx, int cy, int cz, int minCy, int maxCy ) {
			if( minCy == maxCy ) {
				ResetChunk( cx, cy, cz );
			} else {
				for( cy = maxCy; cy >= minCy; cy-- )
					ResetChunk( cx, cy, cz );
			}
		}
		
		void ResetChunk( int cx, int cy, int cz ) {
			if( cx < 0 || cy < 0 || cz < 0 ||
			   cx >= chunksX || cy >= chunksY || cz >= chunksZ ) return;
			DeleteChunk( renderer.unsortedChunks[cx + chunksX * (cy + cz * chunksY)], true );
		}
		

		int chunksTarget = 4;
		const double targetTime = (1.0 / 30) + 0.01;
		public void UpdateChunks( double delta ) {
			int chunkUpdates = 0;
			int viewDist = Utils.AdjViewDist( game.ViewDistance < 16 ? 16 : game.ViewDistance );
			int adjViewDistSqr = (viewDist + 24) * (viewDist + 24);
			chunksTarget += delta < targetTime ? 1 : -1; // build more chunks if 30 FPS or over, otherwise slowdown.
			Utils.Clamp( ref chunksTarget, 4, 12 );
			
			LocalPlayer p = game.LocalPlayer;
			Vector3 cameraPos = game.CurrentCameraPos;
			bool samePos = cameraPos == lastCamPos && p.HeadYawDegrees == lastYaw
				&& p.PitchDegrees == lastPitch;
			renderer.renderCount = samePos ? UpdateChunksStill( delta, ref chunkUpdates, adjViewDistSqr ) :
				UpdateChunksAndVisibility( delta, ref chunkUpdates, adjViewDistSqr );
			
			lastCamPos = cameraPos;
			lastYaw = p.HeadYawDegrees; lastPitch = p.PitchDegrees;
			if( !samePos || chunkUpdates != 0 )
				ResetUsedFlags();
		}
		Vector3 lastCamPos;
		float lastYaw, lastPitch;
		
		int UpdateChunksAndVisibility( double deltaTime, ref int chunkUpdats, int adjViewDistSqr ) {
			ChunkInfo[] chunks = renderer.chunks, render = renderer.renderChunks;
			int j = 0;
			for( int i = 0; i < chunks.Length; i++ ) {
				ChunkInfo info = chunks[i];
				if( info.Empty ) continue;
				int distSqr = distances[i];
				bool inRange = distSqr <= adjViewDistSqr;
				
				if( info.NormalParts == null && info.TranslucentParts == null ) {
					if( inRange && chunkUpdats < chunksTarget )
						BuildChunk( info, ref chunkUpdats );
				}
				info.Visible = inRange &&
					game.Culling.SphereInFrustum( info.CentreX, info.CentreY, info.CentreZ, 14 ); // 14 ~ sqrt(3 * 8^2)
				if( info.Visible && !info.Empty ) { render[j] = info; j++; }
			}
			return j;
		}
		
		int UpdateChunksStill( double deltaTime, ref int chunkUpdates, int adjViewDistSqr ) {
			ChunkInfo[] chunks = renderer.chunks, render = renderer.renderChunks;
			int j = 0;
			for( int i = 0; i < chunks.Length; i++ ) {
				ChunkInfo info = chunks[i];
				if( info.Empty ) continue;
				int distSqr = distances[i];
				bool inRange = distSqr <= adjViewDistSqr;
				
				if( info.NormalParts == null && info.TranslucentParts == null ) {
					if( inRange && chunkUpdates < chunksTarget ) {
						BuildChunk( info, ref chunkUpdates );
						// only need to update the visibility of chunks in range.
						info.Visible = inRange &&
							game.Culling.SphereInFrustum( info.CentreX, info.CentreY, info.CentreZ, 14 ); // 14 ~ sqrt(3 * 8^2)
						if( info.Visible && !info.Empty ) { render[j] = info; j++; }
					}
				} else if ( info.Visible ) {
					render[j] = info; j++;
				}
			}
			return j;
		}
		
		void BuildChunk( ChunkInfo info, ref int chunkUpdates ) {
			game.ChunkUpdates++;
			builder.GetDrawInfo( info.CentreX - 8, info.CentreY - 8, info.CentreZ - 8,
			                    ref info.NormalParts, ref info.TranslucentParts );
			
			if( info.NormalParts == null && info.TranslucentParts == null ) {
				info.Empty = true;
			} else {
				if( info.NormalParts != null )
					IncrementUsed( info.NormalParts );
				if (info.TranslucentParts != null )
					IncrementUsed( info.TranslucentParts );
			}
			chunkUpdates++;
		}
		
		void IncrementUsed( ChunkPartInfo[] parts ) {
			for( int i = 0; i < parts.Length; i++ ) {
				if( parts[i].IndicesCount == 0 ) continue;
				renderer.totalUsed[i]++;
			}
		}
		
		void DecrementUsed( ChunkPartInfo[] parts ) {
			for( int i = 0; i < parts.Length; i++ ) {
				if( parts[i].IndicesCount == 0 ) continue;
				renderer.totalUsed[i]--;
			}
		}
	}
}