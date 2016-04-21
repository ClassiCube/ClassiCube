// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Collections.Generic;
using System.Drawing;
using ClassicalSharp.Events;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Map;
using OpenTK;

namespace ClassicalSharp.Renderers {
	
	public unsafe sealed class MapBordersRenderer : IDisposable {
		
		World map;
		Game game;
		IGraphicsApi graphics;
		
		public MapBordersRenderer( Game game ) {
			this.game = game;
			map = game.World;
			graphics = game.Graphics;
		}
		
		int sidesVb = -1, edgesVb = -1;
		int edgeTexId, sideTexId;
		int sidesVertices, edgesVertices;
		bool legacy, fullColSides, fullColEdge;
		
		public void UseLegacyMode( bool legacy ) {
			this.legacy = legacy;
			ResetSidesAndEdges( null, null );
		}
		
		public void Init() {
			game.WorldEvents.OnNewMap += OnNewMap;
			game.WorldEvents.OnNewMapLoaded += OnNewMapLoaded;
			game.WorldEvents.EnvVariableChanged += EnvVariableChanged;
			game.Events.ViewDistanceChanged += ResetSidesAndEdges;
			game.Events.TerrainAtlasChanged += ResetTextures;
			
			MakeTexture( ref edgeTexId, ref lastEdgeTexLoc, map.EdgeBlock );
			MakeTexture( ref sideTexId, ref lastSideTexLoc, map.SidesBlock );
			ResetSidesAndEdges( null, null );
		}
		
		public void Render( double deltaTime ) {
			if( sidesVb == -1 || edgesVb == -1 ) return;
			graphics.Texturing = true;
			graphics.AlphaTest = true;
			graphics.BindTexture( sideTexId );
			graphics.SetBatchFormat( VertexFormat.P3fT2fC4b );
			if( game.World.SidesBlock != Block.Air ) {
				graphics.BindVb( sidesVb );
				graphics.DrawIndexedVb_TrisT2fC4b( sidesVertices * 6 / 4, 0 );
			}
			
			Vector3 camPos = game.CurrentCameraPos;
			bool underWater = camPos.Y < game.World.EdgeHeight;
			graphics.AlphaBlending = true;
			if( underWater ) game.WeatherRenderer.Render( deltaTime );
			
			graphics.BindTexture( edgeTexId );
			graphics.BindVb( edgesVb );			
			// Do not draw water when we cannot see it.
			// Fixes some 'depth bleeding through' issues with 16 bit depth buffers on large maps.
			float yVisible = Math.Min( 0, map.SidesHeight );
			if( game.World.EdgeBlock != Block.Air && camPos.Y >= yVisible )
				graphics.DrawIndexedVb_TrisT2fC4b( edgesVertices * 6 / 4, 0 );
			
			if( !underWater ) game.WeatherRenderer.Render( deltaTime );
			graphics.AlphaBlending = false;
			graphics.Texturing = false;
			graphics.AlphaTest = false;
		}
		
		public void Dispose() {
			game.WorldEvents.OnNewMap -= OnNewMap;
			game.WorldEvents.OnNewMapLoaded -= OnNewMapLoaded;
			game.WorldEvents.EnvVariableChanged -= EnvVariableChanged;
			game.Events.ViewDistanceChanged -= ResetSidesAndEdges;
			game.Events.TerrainAtlasChanged -= ResetTextures;
			
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
			RebuildSides( map.SidesHeight, legacy ? 128 : 65536 );
			RebuildEdges( map.EdgeHeight, legacy ? 128 : 65536 );
		}
		
		void EnvVariableChanged( object sender, EnvVarEventArgs e ) {
			if( e.Var == EnvVar.EdgeBlock ) {
				MakeTexture( ref edgeTexId, ref lastEdgeTexLoc, map.EdgeBlock );
				if( game.BlockInfo.BlocksLight[(byte)map.EdgeBlock] != fullColEdge )
					ResetSidesAndEdges( null, null );
			} else if( e.Var == EnvVar.SidesBlock ) {
				MakeTexture( ref sideTexId, ref lastSideTexLoc, map.SidesBlock );
				if( game.BlockInfo.BlocksLight[(byte)map.SidesBlock] != fullColSides )
					ResetSidesAndEdges( null, null );
			} else if( e.Var == EnvVar.EdgeLevel ) {
				ResetSidesAndEdges( null, null );
			} else if( e.Var == EnvVar.SunlightColour ) {
				ResetSidesAndEdges( null, null );
			}
		}
		
		void ResetTextures( object sender, EventArgs e ) {
			lastEdgeTexLoc = lastSideTexLoc = -1;
			MakeTexture( ref edgeTexId, ref lastEdgeTexLoc, map.EdgeBlock );
			MakeTexture( ref sideTexId, ref lastSideTexLoc, map.SidesBlock );
		}

		void ResetSidesAndEdges( object sender, EventArgs e ) {
			if( game.World.IsNotLoaded ) return;
			graphics.DeleteVb( sidesVb );
			graphics.DeleteVb( edgesVb );
			
			CalculateRects( game.ViewDistance );
			RebuildSides( map.SidesHeight, legacy ? 128 : 65536 );
			RebuildEdges( map.EdgeHeight, legacy ? 128 : 65536 );
		}
		
		void RebuildSides( int groundLevel, int axisSize ) {
			sidesVertices = 0;
			foreach( Rectangle rec in rects ) {
				sidesVertices += Utils.CountVertices( rec.Width, rec.Height, axisSize ); // YQuads outside
			}
			sidesVertices += Utils.CountVertices( map.Width, map.Length, axisSize ); // YQuads beneath map
			sidesVertices += 2 * Utils.CountVertices( map.Width, Math.Abs( groundLevel ), axisSize ); // ZQuads
			sidesVertices += 2 * Utils.CountVertices( map.Length, Math.Abs( groundLevel ), axisSize ); // XQuads
			VertexP3fT2fC4b* vertices = stackalloc VertexP3fT2fC4b[sidesVertices];
			IntPtr ptr = (IntPtr)vertices;
			
			fullColSides = game.BlockInfo.FullBright[(byte)game.World.SidesBlock];
			FastColour col = fullColSides ? FastColour.White : map.Shadowlight;
			foreach( Rectangle rec in rects ) {
				DrawY( rec.X, rec.Y, rec.X + rec.Width, rec.Y + rec.Height, groundLevel, axisSize, col, 0, ref vertices );
			}
			// Work properly for when ground level is below 0
			int y1 = 0, y2 = groundLevel;
			if( groundLevel < 0 ) {
				y1 = groundLevel;
				y2 = 0;
			}
			DrawY( 0, 0, map.Width, map.Length, 0, axisSize, col, 0, ref vertices );
			DrawZ( 0, 0, map.Width, y1, y2, axisSize, col, ref vertices );
			DrawZ( map.Length, 0, map.Width, y1, y2, axisSize, col, ref vertices );
			DrawX( 0, 0, map.Length, y1, y2, axisSize, col, ref vertices );
			DrawX( map.Width, 0, map.Length, y1, y2, axisSize, col, ref vertices );
			sidesVb = graphics.CreateVb( ptr, VertexFormat.P3fT2fC4b, sidesVertices );
		}
		
		void RebuildEdges( int waterLevel, int axisSize ) {
			edgesVertices = 0;
			foreach( Rectangle rec in rects ) {
				edgesVertices += Utils.CountVertices( rec.Width, rec.Height, axisSize ); // YPlanes outside
			}
			VertexP3fT2fC4b* vertices = stackalloc VertexP3fT2fC4b[edgesVertices];
			IntPtr ptr = (IntPtr)vertices;
			
			fullColEdge = game.BlockInfo.FullBright[(byte)game.World.EdgeBlock];
			FastColour col = fullColEdge ? FastColour.White : map.Sunlight;
			foreach( Rectangle rec in rects ) {
				DrawY( rec.X, rec.Y, rec.X + rec.Width, rec.Y + rec.Height, waterLevel, axisSize, col, -0.1f/16f, ref vertices );
			}
			edgesVb = graphics.CreateVb( ptr, VertexFormat.P3fT2fC4b, edgesVertices );
		}
		
		void DrawX( int x, int z1, int z2, int y1, int y2, int axisSize, FastColour col, ref VertexP3fT2fC4b* vertices ) {
			int endZ = z2, endY = y2, startY = y1;
			for( ; z1 < endZ; z1 += axisSize ) {
				z2 = z1 + axisSize;
				if( z2 > endZ ) z2 = endZ;
				y1 = startY;
				for( ; y1 < endY; y1 += axisSize ) {
					y2 = y1 + axisSize;
					if( y2 > endY ) y2 = endY;
					
					TextureRec rec = new TextureRec( 0, 0, z2 - z1, y2 - y1 );
					*vertices++ = new VertexP3fT2fC4b( x, y1, z1, rec.U1, rec.V2, col );
					*vertices++ = new VertexP3fT2fC4b( x, y2, z1, rec.U1, rec.V1, col );
					*vertices++ = new VertexP3fT2fC4b( x, y2, z2, rec.U2, rec.V1, col );
					*vertices++ = new VertexP3fT2fC4b( x, y1, z2, rec.U2, rec.V2, col );
				}
			}
		}
		
		void DrawZ( int z, int x1, int x2, int y1, int y2, int axisSize, FastColour col, ref VertexP3fT2fC4b* vertices ) {
			int endX = x2, endY = y2, startY = y1;
			for( ; x1 < endX; x1 += axisSize ) {
				x2 = x1 + axisSize;
				if( x2 > endX ) x2 = endX;
				y1 = startY;
				for( ; y1 < endY; y1 += axisSize ) {
					y2 = y1 + axisSize;
					if( y2 > endY ) y2 = endY;
					
					TextureRec rec = new TextureRec( 0, 0, x2 - x1, y2 - y1 );
					*vertices++ = new VertexP3fT2fC4b( x1, y1, z, rec.U1, rec.V2, col );
					*vertices++ = new VertexP3fT2fC4b( x1, y2, z, rec.U1, rec.V1, col );
					*vertices++ = new VertexP3fT2fC4b( x2, y2, z, rec.U2, rec.V1, col );
					*vertices++ = new VertexP3fT2fC4b( x2, y1, z, rec.U2, rec.V2, col );
				}
			}
		}
		
		void DrawY( int x1, int z1, int x2, int z2, float y, int axisSize, FastColour col, float offset, ref VertexP3fT2fC4b* vertices ) {
			int endX = x2, endZ = z2, startZ = z1;
			for( ; x1 < endX; x1 += axisSize ) {
				x2 = x1 + axisSize;
				if( x2 > endX ) x2 = endX;
				z1 = startZ;
				for( ; z1 < endZ; z1 += axisSize ) {
					z2 = z1 + axisSize;
					if( z2 > endZ ) z2 = endZ;
					
					TextureRec rec = new TextureRec( 0, 0, x2 - x1, z2 - z1 );
					*vertices++ = new VertexP3fT2fC4b( x1 + offset, y + offset, z1 + offset, rec.U1, rec.V1, col );
					*vertices++ = new VertexP3fT2fC4b( x1 + offset, y + offset, z2 + offset, rec.U1, rec.V2, col );
					*vertices++ = new VertexP3fT2fC4b( x2 + offset, y + offset, z2 + offset, rec.U2, rec.V2, col );
					*vertices++ = new VertexP3fT2fC4b( x2 + offset, y + offset, z1 + offset, rec.U2, rec.V1, col );
				}
			}
		}
		
		Rectangle[] rects = new Rectangle[4];
		void CalculateRects( int extent ) {
			extent = Utils.AdjViewDist( extent );
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