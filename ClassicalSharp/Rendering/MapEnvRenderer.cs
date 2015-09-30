using System;
using System.Collections.Generic;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {
	
	public unsafe sealed class MapEnvRenderer : IDisposable {
		
		Map map;
		Game game;
		IGraphicsApi graphics;
		
		public MapEnvRenderer( Game game ) {
			this.game = game;
			map = game.Map;
			graphics = game.Graphics;
		}
		
		int sidesVb = -1, edgesVb = -1;
		int edgeTexId, sideTexId;
		int sidesIndices, edgesIndices;
		bool legacy;
		
		public void SetUseLegacyMode( bool legacy ) {
			this.legacy = legacy;
			ResetSidesAndEdges( null, null );
		}
		
		public void Init() {
			game.OnNewMap += OnNewMap;
			game.OnNewMapLoaded += OnNewMapLoaded;
			game.EnvVariableChanged += EnvVariableChanged;
			game.ViewDistanceChanged += ResetSidesAndEdges;
			game.TerrainAtlasChanged += ResetTextures;
			
			MakeTexture( ref edgeTexId, ref lastEdgeTexLoc, map.EdgeBlock );
			MakeTexture( ref sideTexId, ref lastSideTexLoc, map.SidesBlock );
			ResetSidesAndEdges( null, null );
		}
		
		public void Render( double deltaTime ) {
			if( sidesVb == -1 || edgesVb == -1 ) return;
			graphics.Texturing = true;
			graphics.BindTexture( sideTexId );
			graphics.BeginVbBatch( VertexFormat.Pos3fTex2fCol4b );
			graphics.BindVb( sidesVb );
			graphics.DrawIndexedVb_TrisT2fC4b( sidesIndices, 0 );
			
			// Do not draw water when we cannot see it.
			// Fixes 'depth bleeding through' issues with 16 bit depth buffers on large maps.
			if( game.LocalPlayer.EyePosition.Y >= 0 ) {
				graphics.AlphaBlending = true;
				graphics.BindTexture( edgeTexId );
				graphics.BindVb( edgesVb );
				graphics.DrawIndexedVb_TrisT2fC4b( edgesIndices, 0 );
				graphics.AlphaBlending = false;
			}
			graphics.Texturing = false;
		}
		
		public void Dispose() {
			game.OnNewMap -= OnNewMap;
			game.OnNewMapLoaded -= OnNewMapLoaded;
			game.EnvVariableChanged -= EnvVariableChanged;
			game.ViewDistanceChanged -= ResetSidesAndEdges;
			game.TerrainAtlasChanged -= ResetTextures;
			
			graphics.DeleteTexture( ref edgeTexId );
			graphics.DeleteTexture( ref sideTexId );
			graphics.DeleteVb( sidesVb );
			graphics.DeleteVb( edgesVb );
			sidesVb = edgesVb = -1;
		}
		
		void OnNewMap( object sender, EventArgs e ) {
			graphics.DeleteVb( sidesVb );
			graphics.DeleteVb( edgesVb );
			sidesVb = edgesVb = -1;
			MakeTexture( ref edgeTexId, ref lastEdgeTexLoc, map.EdgeBlock );
			MakeTexture( ref sideTexId, ref lastSideTexLoc, map.SidesBlock );
		}
		
		void OnNewMapLoaded( object sender, EventArgs e ) {
			CalculateRects( game.ViewDistance );
			RebuildSides( map.GroundHeight, legacy ? 128 : 65536 );
			RebuildEdges( map.WaterHeight, legacy ? 128 : 65536 );
		}
		
		void EnvVariableChanged( object sender, EnvVariableEventArgs e ) {
			if( e.Var == EnvVariable.EdgeBlock ) {
				MakeTexture( ref edgeTexId, ref lastEdgeTexLoc, map.EdgeBlock );
			} else if( e.Var == EnvVariable.SidesBlock ) {
				MakeTexture( ref sideTexId, ref lastSideTexLoc, map.SidesBlock );
			} else if( e.Var == EnvVariable.WaterLevel ) {
				ResetSidesAndEdges( null, null );
			} else if( e.Var == EnvVariable.SunlightColour ) {
				ResetSidesAndEdges( null, null );
			}
		}
		
		void ResetTextures( object sender, EventArgs e ) {
			lastEdgeTexLoc = lastSideTexLoc = -1;
			MakeTexture( ref edgeTexId, ref lastEdgeTexLoc, map.EdgeBlock );
			MakeTexture( ref sideTexId, ref lastSideTexLoc, map.SidesBlock );
		}

		void ResetSidesAndEdges( object sender, EventArgs e ) {
			if( game.Map.IsNotLoaded ) return;
			graphics.DeleteVb( sidesVb );
			graphics.DeleteVb( edgesVb );
			CalculateRects( game.ViewDistance );
			RebuildSides( map.GroundHeight, legacy ? 128 : 65536 );		
			RebuildEdges( map.WaterHeight, legacy ? 128 : 65536 );
		}
		
		void RebuildSides( int groundLevel, int axisSize ) {
			sidesIndices = 0;
			foreach( Rectangle rec in rects ) {
				sidesIndices += Utils.CountIndices( rec.Width, rec.Height, axisSize ); // YPlanes outside
			}
			sidesIndices += Utils.CountIndices( map.Width, map.Length, axisSize ); // YPlane beneath map
			sidesIndices += 2 * Utils.CountIndices( map.Width, groundLevel, axisSize ); // ZPlanes
			sidesIndices += 2 * Utils.CountIndices( map.Length, groundLevel, axisSize ); // XPlanes
			VertexPos3fTex2fCol4b* vertices = stackalloc VertexPos3fTex2fCol4b[sidesIndices / 6 * 4];
			IntPtr ptr = (IntPtr)vertices;
			
			FastColour col = map.SunlightYBottom;
			foreach( Rectangle rec in rects ) {
				DrawY( rec.X, rec.Y, rec.X + rec.Width, rec.Y + rec.Height, groundLevel, axisSize, col, ref vertices );
			}			
			DrawY( 0, 0, map.Width, map.Length, 0, axisSize, col, ref vertices );
			DrawZ( 0, 0, map.Width, 0, groundLevel, axisSize, col, ref vertices );
			DrawZ( map.Length, 0, map.Width, 0, groundLevel, axisSize, col, ref vertices );
			DrawX( 0, 0, map.Length, 0, groundLevel, axisSize, col, ref vertices );
			DrawX( map.Width, 0, map.Length, 0, groundLevel, axisSize, col, ref vertices );
			sidesVb = graphics.CreateVb( ptr, VertexFormat.Pos3fTex2fCol4b, sidesIndices / 6 * 4 );
		}
		
		void RebuildEdges( int waterLevel, int axisSize ) {
			edgesIndices = 0;
			foreach( Rectangle rec in rects ) {
				edgesIndices += Utils.CountIndices( rec.Width, rec.Height, axisSize ); // YPlanes outside
			}
			VertexPos3fTex2fCol4b* vertices = stackalloc VertexPos3fTex2fCol4b[edgesIndices / 6 * 4];
			IntPtr ptr = (IntPtr)vertices;
			
			foreach( Rectangle rec in rects ) {
				DrawY( rec.X, rec.Y, rec.X + rec.Width, rec.Y + rec.Height, waterLevel, axisSize, game.Map.Sunlight, ref vertices );
			}
			edgesVb = graphics.CreateVb( ptr, VertexFormat.Pos3fTex2fCol4b, edgesIndices / 6 * 4 );
		}
		
		void DrawX( int x, int z1, int z2, int y1, int y2, int axisSize, FastColour col, ref VertexPos3fTex2fCol4b* vertices ) {
			int endZ = z2, endY = y2, startY = y1;
			for( ; z1 < endZ; z1 += axisSize ) {
				z2 = z1 + axisSize;
				if( z2 > endZ ) z2 = endZ;
				y1 = startY;
				for( ; y1 < endY; y1 += axisSize ) {
					y2 = y1 + axisSize;
					if( y2 > endY ) y2 = endY;
					
					TextureRectangle rec = new TextureRectangle( 0, 0, z2 - z1, y2 - y1 );
					*vertices++ = new VertexPos3fTex2fCol4b( x, y1, z1, rec.U1, rec.V1, col );
					*vertices++ = new VertexPos3fTex2fCol4b( x, y2, z1, rec.U1, rec.V2, col );
					*vertices++ = new VertexPos3fTex2fCol4b( x, y2, z2, rec.U2, rec.V2, col );
					*vertices++ = new VertexPos3fTex2fCol4b( x, y1, z2, rec.U2, rec.V1, col );
				}
			}
		}
		
		void DrawZ( int z, int x1, int x2, int y1, int y2, int axisSize, FastColour col, ref VertexPos3fTex2fCol4b* vertices ) {
			int endX = x2, endY = y2, startY = y1;
			for( ; x1 < endX; x1 += axisSize ) {
				x2 = x1 + axisSize;
				if( x2 > endX ) x2 = endX;
				y1 = startY;
				for( ; y1 < endY; y1 += axisSize ) {
					y2 = y1 + axisSize;
					if( y2 > endY ) y2 = endY;
					
					TextureRectangle rec = new TextureRectangle( 0, 0, x2 - x1, y2 - y1 );
					*vertices++ = new VertexPos3fTex2fCol4b( x1, y1, z, rec.U1, rec.V1, col );
					*vertices++ = new VertexPos3fTex2fCol4b( x1, y2, z, rec.U1, rec.V2, col );
					*vertices++ = new VertexPos3fTex2fCol4b( x2, y2, z, rec.U2, rec.V2, col );
					*vertices++ = new VertexPos3fTex2fCol4b( x2, y1, z, rec.U2, rec.V1, col );
				}
			}
		}
		
		void DrawY( int x1, int z1, int x2, int z2, int y, int axisSize, FastColour col, ref VertexPos3fTex2fCol4b* vertices ) {
			int endX = x2, endZ = z2, startZ = z1;
			for( ; x1 < endX; x1 += axisSize ) {
				x2 = x1 + axisSize;
				if( x2 > endX ) x2 = endX;
				z1 = startZ;
				for( ; z1 < endZ; z1 += axisSize ) {
					z2 = z1 + axisSize;
					if( z2 > endZ ) z2 = endZ;
					
					TextureRectangle rec = new TextureRectangle( 0, 0, x2 - x1, z2 - z1 );
					*vertices++ = new VertexPos3fTex2fCol4b( x1, y, z1, rec.U1, rec.V1, col );
					*vertices++ = new VertexPos3fTex2fCol4b( x1, y, z2, rec.U1, rec.V2, col );
					*vertices++ = new VertexPos3fTex2fCol4b( x2, y, z2, rec.U2, rec.V2, col );
					*vertices++ = new VertexPos3fTex2fCol4b( x2, y, z1, rec.U2, rec.V1, col );
				}
			}
		}
		
		Rectangle[] rects = new Rectangle[4];
		void CalculateRects( int extent ) {
			rects[0] = new Rectangle( -extent, -extent, extent + map.Width + extent, extent );
			rects[1] = new Rectangle( -extent, map.Length, extent + map.Width + extent, extent );
			
			rects[2] = new Rectangle( -extent, 0, extent, map.Length );
			rects[3] = new Rectangle( map.Width, 0, extent, map.Length );			
		}
		
		int lastEdgeTexLoc, lastSideTexLoc;
		void MakeTexture( ref int texId, ref int lastTexLoc, Block block ) {
			int texLoc = game.BlockInfo.GetTextureLoc( (byte)block, TileSide.Top );
			if( texLoc != lastTexLoc ) {
				lastTexLoc = texLoc;
				game.Graphics.DeleteTexture( ref texId );
				texId = game.TerrainAtlas.LoadTextureElement( texLoc );
			}
		}
	}
}