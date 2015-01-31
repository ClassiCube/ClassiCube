using System;
using System.Collections.Generic;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.World;
using OpenTK;

namespace ClassicalSharp {
	
	// TODO: optimise chunk rendering
	//  --> reduce the two passes: liquid pass only needs 1 small texture
	//    |--> divide into 'solid', 'transparent', 'translucent passes'
	//       |--> don't render solid back facing polygons. may help with low performance computers.
	// |--> use indices.
	public class MapRenderer : IDisposable {
		
		class SectionInfo {
			
			public Vector3I Location;
			
			public bool Visible = true;
			
			public SectionDrawInfo DrawInfo;
			
			public SectionInfo( int x, int y, int z ) {
				Location = new Vector3I( x, y, z );
			}
		}
		
		public Game Window;
		public IGraphicsApi Graphics;
		
		ChunkMeshBuilder builder;
		Dictionary<Vector3I, SectionInfo> sections = new Dictionary<Vector3I, SectionInfo>();
		Vector3I sectionPos = new Vector3I( int.MaxValue, int.MaxValue, int.MaxValue );
		
		public readonly bool UsesLighting;
		
		public MapRenderer( Game window ) {
			Window = window;
			builder = new ChunkMeshBuilderTex2Col4( window );
			UsesLighting = builder.UsesLighting;
			Graphics = window.Graphics;
			Window.OnNewMap += OnNewMap;
			Window.OnNewMapLoaded += OnNewMapLoaded;
			Window.EnvVariableChanged += EnvVariableChanged;
		}
		
		public void Dispose() {
			ClearSectionCache();
			sections = null;
			Window.OnNewMap -= OnNewMap;
			Window.OnNewMapLoaded -= OnNewMapLoaded;
			Window.EnvVariableChanged -= EnvVariableChanged;
		}
		
		public void Refresh() {
			if( sections != null && !Window.Map.IsNotLoaded ) {
				ClearSectionCache();
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
			ClearSectionCache();
			sectionPos = new Vector3I( int.MaxValue, int.MaxValue, int.MaxValue );
			builder.OnNewMap();
		}
		
		void OnNewMapLoaded( object sender, EventArgs e ) {
			builder.OnNewMapLoaded();
		}
		
		void ClearSectionCache() {
			foreach( var pair in sections ) {
				ResetSection( pair.Value );
			}
			sections.Clear();
		}
		
		void ResetSection( SectionInfo info ) {
			SectionDrawInfo drawInfo = info.DrawInfo;
			if( drawInfo == null ) return;
			
			Graphics.DeleteVb( drawInfo.SpriteParts.VboID );
			Graphics.DeleteVb( drawInfo.TranslucentParts.VboID );
			Graphics.DeleteVb( drawInfo.SolidParts.VboID );
			info.DrawInfo = null;
		}

		public void RedrawBlock( int x, int y, int z ) {
			int cx = x >> 4;
			int cy = y >> 4;
			int cz = z >> 4;
			
			ResetSectionAt( cx, cy, cz );
			int bX = x & 0x0F; // % 16
			int bY = y & 0x0F;
			int bZ = z & 0x0F;
			
			if( bX == 0 ) ResetSectionAt( cx - 1, cy, cz );
			if( bY == 0 ) ResetSectionAt( cx, cy - 1, cz );
			if( bZ == 0 ) ResetSectionAt( cx, cy, cz - 1 );
			if( bX == 15 ) ResetSectionAt( cx + 1, cy, cz );
			if( bY == 15 ) ResetSectionAt( cx, cy + 1, cz );
			if( bZ == 15 ) ResetSectionAt( cx, cy, cz + 1 );
		}
		
		public void RedrawSection( int cx, int cy, int cz, ChunkPartialUpdate update ) {
			ResetSectionAt( cx, cy, cz );
			if( update.X0Modified ) ResetSectionAt( cx - 1, cy, cz );
			if( update.Y0Modified ) ResetSectionAt( cx, cy - 1, cz );
			if( update.Z0Modified ) ResetSectionAt( cx, cy, cz - 1 );
			if( update.X15Modified ) ResetSectionAt( cx + 1, cy, cz );
			if( update.Y15Modified ) ResetSectionAt( cx, cy + 1, cz );
			if( update.Z15Modified ) ResetSectionAt( cx, cy, cz + 1 );
		}
		
		void ResetSectionAt( int cx, int cy, int cz ) {
			ResetOrCreateSection( cx << 4, cy << 4, cz << 4 );
		}
		
		void ResetOrCreateSection( int x, int y, int z ) {
			if( y < 0 || y >= Map.Height ) return;
			
			SectionInfo info;
			if( sections.TryGetValue( new Vector3I( x, y, z ), out info ) ) {
				ResetSection( info );
			} else {
				AddSection( new SectionInfo( x, y, z ) );
			}
		}
		
		void AddSection( SectionInfo info ) {
			// TODO: Find out why sections are getting duplicated sometimes..
			//sections.Add( info.Location, info );
			sections[info.Location] = info;
			cacheNeedsReloading = true;
		}
		
		public void LoadChunk( Chunk chunk ) {
			int x = chunk.ChunkX << 4;
			int z = chunk.ChunkZ << 4;
			for( int y = 0; y < Map.Height; y += 16 ) {
				AddSection( new SectionInfo( x, y, z ) );
			}
		}
		
		public void UnloadChunk( int chunkX, int chunkZ ) {
			int x = chunkX << 4;
			int z = chunkZ << 4;
			for( int y = 0; y < Map.Height; y += 16 ) {
				UnloadSection( x, y, z );
			}
		}
		
		void UnloadSection( int x, int y, int z ) {
			SectionInfo info;
			if( sections.TryGetValue( new Vector3I( x, y, z ), out info ) ) {
				ResetSection( info );
				sections.Remove( new Vector3I( x, y, z ) );
				cacheNeedsReloading = true;
			}
		}
		
		public void Render( double deltaTime ) {
			if( sections == null ) return;
			Window.Vertices = 0;
			UpdatePosition();
			UpdateCache();
			UpdateChunks();
			
			// Render solid and fully transparent to fill depth buffer.
			// These blocks are treated as having an alpha value of either none or full.
			Graphics.Bind2DTexture( Window.TerrainAtlasTexId );
			builder.BeginRender();
			Graphics.Texturing = true;
			Graphics.AlphaTest = true;
			Graphics.FaceCulling = true;
			RenderSolidBatch();
			Graphics.FaceCulling = false;
			RenderSpriteBatch();
			Graphics.AlphaTest = false;
			Graphics.Texturing = false;
			builder.EndRender();
			
			// Render translucent(liquid) blocks. These 'blend' into other blocks.
			builder.BeginRender();
			Graphics.AlphaTest = false;
			Graphics.Texturing = false;
			Graphics.AlphaBlending = false;
			
			// First fill depth buffer
			Graphics.DepthTestFunc( DepthFunc.LessEqual );
			Graphics.ColourMask( false, false, false, false );
			RenderTranslucentBatchNoAdd();
			// Then actually draw the transluscent blocks
			Graphics.AlphaBlending = true;
			Graphics.Texturing = true;
			Graphics.ColourMask( true, true, true, true );
			RenderTranslucentBatch();
			Graphics.DepthTestFunc( DepthFunc.Less );
			
			Graphics.AlphaTest = false;
			Graphics.AlphaBlending = false;
			Graphics.Texturing = false;
			builder.EndRender();
		}

		SectionInfo[] sectionCache;
		int[] distances;
		internal int sectionsCacheCount = 0;
		bool cacheNeedsReloading = true;
		
		void UpdatePosition() {
			Player p = Window.LocalPlayer;
			Vector3I newSectionPos = Vector3I.Floor( p.Position );
			newSectionPos.X = ( newSectionPos.X & ~0x0F ) + 8;
			newSectionPos.Y = ( newSectionPos.Y & ~0x0F ) + 8;
			newSectionPos.Z = ( newSectionPos.Z & ~0x0F ) + 8;
			if( newSectionPos != sectionPos ) {
				sectionPos = newSectionPos;
				cacheNeedsReloading = true;
			}
		}
		
		void UpdateCache() {
			if( !cacheNeedsReloading ) return;
			
			sectionsCacheCount = sections.Count;
			if( sectionCache == null || sectionsCacheCount > sectionCache.Length ) {
				sectionCache = new SectionInfo[sectionsCacheCount];
				distances = new int[sectionsCacheCount];
			}
			
			int i = 0;
			foreach( var pair in sections ) {
				sectionCache[i] = pair.Value;
				Vector3I loc = pair.Key;
				distances[i] = Utils.DistanceSquared( loc.X + 8, loc.Y + 8, loc.Z + 8, 
				                                     sectionPos.X, sectionPos.Y, sectionPos.Z );
				i++;
			}
			Array.Sort( distances, sectionCache, 0, sectionsCacheCount );
		}
		
		void UpdateChunks() {
			int chunksUpdatedThisFrame = 0;
			int adjViewDistSqr = ( Window.ViewDistance + 14 ) * ( Window.ViewDistance + 14 );
			for( int i = 0; i < sectionsCacheCount; i++ ) {
				SectionInfo info = sectionCache[i];
				Vector3I loc = info.Location;
				int distSqr = distances[i];
				bool inRange = distSqr <= adjViewDistSqr;
				
				if( info.DrawInfo == null ) {
					if( inRange && chunksUpdatedThisFrame < 4 ) {
						Window.ChunkUpdates++;
						info.DrawInfo = builder.GetDrawInfo( loc.X, loc.Y, loc.Z );
						chunksUpdatedThisFrame++;
					}
				}
				
				if( !inRange ) {
					info.Visible = false;
				} else {
					info.Visible = Window.Culling.SphereInFrustum( loc.X + 8, loc.Y + 8, loc.Z + 8, 14 ); // 14 ~ sqrt(3 * 8^2)
				}
			}
		}
		
		// TODO: there's probably a better way of doing this.
		void RenderSolidBatch() {
			for( int i = 0; i < sectionsCacheCount; i++ ) {
				SectionInfo info = sectionCache[i];
				if( info.DrawInfo == null || !info.Visible ) continue;

				ChunkPartInfo drawInfo = info.DrawInfo.SolidParts;
				if( drawInfo.VerticesCount == 0 ) continue;
				
				builder.Render( drawInfo );
				Window.Vertices += drawInfo.VerticesCount;
			}
		}
		
		void RenderSpriteBatch() {
			for( int i = 0; i < sectionsCacheCount; i++ ) {
				SectionInfo info = sectionCache[i];
				if( info.DrawInfo == null || !info.Visible ) continue;

				ChunkPartInfo drawInfo = info.DrawInfo.SpriteParts;
				if( drawInfo.VerticesCount == 0 ) continue;
				
				builder.Render( drawInfo );
				Window.Vertices += drawInfo.VerticesCount;
			}
		}

		void RenderTranslucentBatch() {
			for( int i = 0; i < sectionsCacheCount; i++ ) {
				SectionInfo info = sectionCache[i];
				if( info.DrawInfo == null || !info.Visible ) continue;

				ChunkPartInfo drawInfo = info.DrawInfo.TranslucentParts;
				if( drawInfo.VerticesCount == 0 ) continue;
				
				builder.Render( drawInfo );
				Window.Vertices += drawInfo.VerticesCount;
			}
		}
		
		void RenderTranslucentBatchNoAdd() {
			for( int i = 0; i < sectionsCacheCount; i++ ) {
				SectionInfo info = sectionCache[i];
				if( info.DrawInfo == null || !info.Visible ) continue;

				ChunkPartInfo drawInfo = info.DrawInfo.TranslucentParts;
				if( drawInfo.VerticesCount == 0 ) continue;
				
				builder.Render( drawInfo );
			}
		}
	}
}