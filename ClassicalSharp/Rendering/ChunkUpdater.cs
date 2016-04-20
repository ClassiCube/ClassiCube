// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.Entities;
using ClassicalSharp.Events;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Renderers {
	
	/// <summary> Manages the process of building/deleting chunk meshes,
	/// in addition to calculating the visibility of chunks. </summary>
	public sealed class ChunkUpdater : IDisposable {
		
		Game game;
		IGraphicsApi api;
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
			
			renderer._1DUsed = game.TerrainAtlas1D.CalcMaxUsedRow( game.TerrainAtlas, info );
			renderer.totalUsed = new int[game.TerrainAtlas1D.TexIds.Length];
			RecalcBooleans( true );
			
			builder = new ChunkMeshBuilder( game );
			api = game.Graphics;
			elementsPerBitmap = game.TerrainAtlas1D.elementsPerBitmap;
			
			game.Events.TerrainAtlasChanged += TerrainAtlasChanged;
			game.WorldEvents.OnNewMap += OnNewMap;
			game.WorldEvents.OnNewMapLoaded += OnNewMapLoaded;
			game.WorldEvents.EnvVariableChanged += EnvVariableChanged;
			game.Events.BlockDefinitionChanged += BlockDefinitionChanged;
			game.Events.ViewDistanceChanged += ViewDistanceChanged;
			game.Events.ProjectionChanged += ProjectionChanged;
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
			CreateChunkCache();
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
				builder.clipLevel = Math.Max( 0, game.World.SidesHeight );
				RefreshBorders( Math.Max( oldClip, builder.clipLevel ) );
			}
		}

		void TerrainAtlasChanged( object sender, EventArgs e ) {
			bool refreshRequired = elementsPerBitmap != game.TerrainAtlas1D.elementsPerBitmap;
			elementsPerBitmap = game.TerrainAtlas1D.elementsPerBitmap;
			renderer._1DUsed = game.TerrainAtlas1D.CalcMaxUsedRow( game.TerrainAtlas, info );
			
			if( refreshRequired ) Refresh();
			RecalcBooleans( true );
		}
		
		void BlockDefinitionChanged( object sender, EventArgs e ) {
			renderer._1DUsed = game.TerrainAtlas1D.CalcMaxUsedRow( game.TerrainAtlas, info );
			RecalcBooleans( true );
			Refresh();
		}
		
		void ProjectionChanged( object sender, EventArgs e ) {
			lastCamPos = new Vector3( float.MaxValue );
		}
		
		void OnNewMap( object sender, EventArgs e ) {
			game.ChunkUpdates = 0;
			ClearChunkCache();
			for( int i = 0; i < renderer.totalUsed.Length; i++ )
				renderer.totalUsed[i] = 0;
			
			renderer.chunks = null;
			renderer.unsortedChunks = null;
			chunkPos = new Vector3I( int.MaxValue, int.MaxValue, int.MaxValue );
			builder.OnNewMap();
		}
		
		void ViewDistanceChanged( object sender, EventArgs e ) {
			lastCamPos = new Vector3( float.MaxValue );
			lastYaw = float.MaxValue;
			lastPitch = float.MaxValue;
		}
		
		internal void RecalcBooleans( bool sizeChanged ) {
			int used = renderer._1DUsed;
			if( sizeChanged ) {
				renderer.usedTranslucent = new bool[used];
				renderer.usedNormal = new bool[used];
				renderer.pendingTranslucent = new bool[used];
				renderer.pendingNormal = new bool[used];
			}
			
			for( int i = 0; i < used; i++ ) {
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
			
			renderer.chunks = new ChunkInfo[chunksX * chunksY * chunksZ];
			renderer.unsortedChunks = new ChunkInfo[chunksX * chunksY * chunksZ];
			distances = new int[renderer.chunks.Length];
			CreateChunkCache();
			builder.OnNewMapLoaded();
			lastCamPos = new Vector3( float.MaxValue );
			lastYaw = float.MaxValue;
			lastPitch = float.MaxValue;
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
			DeleteData( ref info.NormalParts, decUsed );
			DeleteData( ref info.TranslucentParts, decUsed );
		}
		
		void DeleteData( ref ChunkPartInfo[] parts, bool decUsed ) {
			if( decUsed ) DecrementUsed( parts );
			if( parts == null ) return;
			
			for( int i = 0; i < parts.Length; i++ )
				api.DeleteVb( parts[i].VbId );
			parts = null;
		}
		
		void CreateChunkCache() {
			int index = 0;
			for( int z = 0; z < length; z += 16 )
				for( int y = 0; y < height; y += 16 )
					for( int x = 0; x < width; x += 16 )
			{
				renderer.chunks[index] = new ChunkInfo( x, y, z );
				renderer.unsortedChunks[index] = renderer.chunks[index];
				index++;
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
			byte b1 = game.World.GetBlock( x1, y1, z1 );
			byte b2 = game.World.GetBlock( x2, y2, z2 );
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
			DeleteChunk( renderer.unsortedChunks[cx + chunksX * ( cy + cz * chunksY )], true );
		}

		int chunksTarget = 4;
		const double targetTime = (1.0 / 30) + 0.01;
		public void UpdateChunks( double deltaTime ) {
			int chunkUpdates = 0;
			int viewDist = Utils.AdjViewDist( game.ViewDistance < 16 ? 16 : game.ViewDistance );
			int adjViewDistSqr = (viewDist + 24) * (viewDist + 24);
			chunksTarget += deltaTime < targetTime ? 1 : -1; // build more chunks if 30 FPS or over, otherwise slowdown.
			Utils.Clamp( ref chunksTarget, 4, 12 );
			
			LocalPlayer p = game.LocalPlayer;
			Vector3 cameraPos = game.CurrentCameraPos;
			bool samePos = cameraPos == lastCamPos && p.HeadYawDegrees == lastYaw
				&& p.PitchDegrees == lastPitch;
			if( samePos )
				UpdateChunksStill( deltaTime, ref chunkUpdates, adjViewDistSqr );
			else
				UpdateChunksAndVisibility( deltaTime, ref chunkUpdates, adjViewDistSqr );
			
			lastCamPos = cameraPos;
			lastYaw = p.HeadYawDegrees; lastPitch = p.PitchDegrees;
			if( !samePos || chunkUpdates != 0 )
				RecalcBooleans( false );
		}
		Vector3 lastCamPos;
		float lastYaw, lastPitch;
		
		void UpdateChunksAndVisibility( double deltaTime, ref int chunkUpdats, int adjViewDistSqr ) {
			ChunkInfo[] chunks = renderer.chunks;
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
			}
		}
		
		void UpdateChunksStill( double deltaTime, ref int chunkUpdates, int adjViewDistSqr ) {
			ChunkInfo[] chunks = renderer.chunks;
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
					}
				}
			}
		}
		
		void BuildChunk( ChunkInfo info, ref int chunkUpdates ) {
			game.ChunkUpdates++;
			builder.GetDrawInfo( info.CentreX - 8, info.CentreY - 8, info.CentreZ - 8,
			                    ref info.NormalParts, ref info.TranslucentParts );
			
			if( info.NormalParts == null && info.TranslucentParts == null ) {
				info.Empty = true;
			} else {
				IncrementUsed( info.NormalParts );
				IncrementUsed( info.TranslucentParts );
			}
			chunkUpdates++;
		}
		
		void IncrementUsed( ChunkPartInfo[] parts ) {
			if( parts == null ) return;
			for( int i = 0; i < parts.Length; i++ ) {
				if( parts[i].IndicesCount == 0 ) continue;
				renderer.totalUsed[i]++;
			}
		}
		
		void DecrementUsed( ChunkPartInfo[] parts ) {
			if( parts == null ) return;
			for( int i = 0; i < parts.Length; i++ ) {
				if( parts[i].IndicesCount == 0 ) continue;
				renderer.totalUsed[i]--;
			}
		}
	}
}