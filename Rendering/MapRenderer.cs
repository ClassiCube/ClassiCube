using System;
using System.Collections.Generic;
using ClassicalSharp.GraphicsAPI;
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
		List<SectionInfo> sections = new List<SectionInfo>();
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
			for( int i = 0; i < sections.Count; i++ ) {
				ResetSection( sections[i] );
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
		
		static int NextMultipleOf16( int value ) {
			int remainder = value % 16;
			return remainder == 0 ? value : value + 16 - remainder;
		}
		
		public void RedrawBlock( int x, int y, int z, byte block, int oldHeight, int newHeight ) {
			int cx = x >> 4;
			int cy = y >> 4;
			int cz = z >> 4;
			// NOTE: It's a lot faster to only update the chunks that are affected by the change in shadows,
			// rather than the entire column.
			int newLightcy = newHeight == -1 ? 0 : newHeight >> 4;
			int oldLightcy = oldHeight == -1 ? 0 : oldHeight >> 4;
			
			ResetSectionAndBelow( cx, cy, cz, newLightcy, oldLightcy );
			int bX = x & 0x0F; // % 16
			int bY = y & 0x0F;
			int bZ = z & 0x0F;
			
			if( bX == 0 ) ResetSectionAndBelow( cx - 1, cy, cz, newLightcy, oldLightcy );
			if( bY == 0 ) ResetSectionAndBelow( cx, cy - 1, cz, newLightcy, oldLightcy );
			if( bZ == 0 ) ResetSectionAndBelow( cx, cy, cz - 1, newLightcy, oldLightcy );
			if( bX == 15 ) ResetSectionAndBelow( cx + 1, cy, cz, newLightcy, oldLightcy );
			if( bY == 15 ) ResetSectionAndBelow( cx, cy + 1, cz, newLightcy, oldLightcy );
			if( bZ == 15 ) ResetSectionAndBelow( cx, cy, cz + 1, newLightcy, oldLightcy );
		}
		
		void ResetSectionAndBelow( int cx, int cy, int cz, int newLightCy, int oldLightCy ) {
			if( UsesLighting ) {
				if( newLightCy == oldLightCy ) {
					ResetOrCreateSection( cx << 4, cy << 4, cz << 4 );
				} else {
					int cyMax = Math.Max( newLightCy, oldLightCy );
					int cyMin = Math.Min( oldLightCy, newLightCy );
					for( cy = cyMax; cy >= cyMin; cy-- ) {
						ResetOrCreateSection( cx << 4, cy << 4, cz << 4 );
					}
				}
			} else {
				ResetOrCreateSection( cx << 4, cy << 4, cz << 4 );
			}
		}
		
		void ResetOrCreateSection( int x, int y, int z ) {
			if( y < 0 || y >= Map.Height ) return;
			
			for( int i = 0; i < sections.Count; i++ ) {
				SectionInfo info = sections[i];
				Vector3I loc = info.Location;
				if( loc.X == x && loc.Y == y && loc.Z == z ) {
					ResetSection( info );
					return;
				}
			}
			sections.Add( new SectionInfo( x, y, z ) );
		}
		
		public void UnloadChunk( int chunkX, int chunkZ ) {
			int x = chunkX << 4;
			int z = chunkZ << 4;
			for( int y = 0; y < Map.Height; y += 16 ) {
				UnloadSection( x, y, z );
			}
		}
		
		void UnloadSection( int x, int y, int z ) {
			for( int i = 0; i < sections.Count; i++ ) {
				SectionInfo info = sections[i];
				Vector3I loc = info.Location;
				if( loc.X == x && loc.Y == y && loc.Z == z ) {
					ResetSection( info );
					sections.RemoveAt( i );
					return;
				}
			}
		}
		
		public void Render( double deltaTime ) {
			if( sections == null ) return;
			Window.Vertices = 0;
			UpdateSortOrder();
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

		void UpdateSortOrder() {
			Player p = Window.LocalPlayer;
			Vector3I newChunkPos = Vector3I.Floor( p.Position );
			newChunkPos.X = ( newChunkPos.X & ~0x0F ) + 8;
			newChunkPos.Y = ( newChunkPos.Y & ~0x0F ) + 8;
			newChunkPos.Z = ( newChunkPos.Z & ~0x0F ) + 8;
			if( newChunkPos != sectionPos ) {
				sections.Sort( CompareChunks );
			}
		}
		
		int CompareChunks( SectionInfo a, SectionInfo b ) {
			Vector3I aLoc = a.Location;
			Vector3I bLoc = b.Location;
			int distA = Utils.DistanceSquared( aLoc.X + 8, aLoc.Y + 8, aLoc.Z + 8, sectionPos.X, sectionPos.Y, sectionPos.Z );
			int distB = Utils.DistanceSquared( bLoc.X + 8, bLoc.Y + 8, bLoc.Z + 8, sectionPos.X, sectionPos.Y, sectionPos.Z );
			return distA.CompareTo( distB );
		}
		
		void UpdateChunks() {
			int chunksUpdatedThisFrame = 0;
			int adjViewDistSqr = ( Window.ViewDistance + 14 ) * ( Window.ViewDistance + 14 );
			for( int i = 0; i < sections.Count; i++ ) {
				SectionInfo info = sections[i];				
				Vector3I loc = info.Location;
				int distSqr = Utils.DistanceSquared( loc.X + 8, loc.Y + 8, loc.Z + 8, sectionPos.X, sectionPos.Y, sectionPos.Z );
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
			for( int i = 0; i < sections.Count; i++ ) {
				SectionInfo info = sections[i];
				if( info.DrawInfo == null || !info.Visible ) continue;

				ChunkPartInfo drawInfo = info.DrawInfo.SolidParts;
				if( drawInfo.VerticesCount == 0 ) continue;
				
				builder.Render( drawInfo );
				Window.Vertices += drawInfo.VerticesCount;
			}
		}
		
		void RenderSpriteBatch() {
			for( int i = 0; i < sections.Count; i++ ) {
				SectionInfo info = sections[i];
				if( info.DrawInfo == null || !info.Visible ) continue;

				ChunkPartInfo drawInfo = info.DrawInfo.SpriteParts;
				if( drawInfo.VerticesCount == 0 ) continue;
				
				builder.Render( drawInfo );
				Window.Vertices += drawInfo.VerticesCount;
			}
		}

		void RenderTranslucentBatch() {
			for( int i = 0; i < sections.Count; i++ ) {
				SectionInfo info = sections[i];
				if( info.DrawInfo == null || !info.Visible ) continue;

				ChunkPartInfo drawInfo = info.DrawInfo.TranslucentParts;
				if( drawInfo.VerticesCount == 0 ) continue;
				
				builder.Render( drawInfo );
				Window.Vertices += drawInfo.VerticesCount;
			}
		}
		
		void RenderTranslucentBatchNoAdd() {
			for( int i = 0; i < sections.Count; i++ ) {
				SectionInfo info = sections[i];
				if( info.DrawInfo == null || !info.Visible ) continue;

				ChunkPartInfo drawInfo = info.DrawInfo.TranslucentParts;
				if( drawInfo.VerticesCount == 0 ) continue;
				
				builder.Render( drawInfo );
			}
		}
	}
}