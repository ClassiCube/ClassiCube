// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Collections.Generic;
using System.Drawing;
using ClassicalSharp.Events;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Map;
using OpenTK;

namespace ClassicalSharp.Renderers {
	
	public unsafe sealed class MapBordersRenderer : IGameComponent {
		
		World map;
		Game game;
		IGraphicsApi graphics;
		
		int sidesVb = -1, edgesVb = -1;
		int edgeTexId, sideTexId;
		int sidesVertices, edgesVertices;
		internal bool legacy;
		bool fullColSides, fullColEdge;
		
		public void UseLegacyMode( bool legacy ) {
			this.legacy = legacy;
			ResetSidesAndEdges( null, null );
		}
		
		public void Init( Game game ) {
			this.game = game;
			map = game.World;
			graphics = game.Graphics;
			
			game.WorldEvents.EnvVariableChanged += EnvVariableChanged;
			game.Events.ViewDistanceChanged += ResetSidesAndEdges;
			game.Events.TerrainAtlasChanged += ResetTextures;
		}
		
		public void RenderSides( double delta ) {
			if( sidesVb == -1 ) return;
			byte block = game.World.Env.SidesBlock;
			if( game.BlockInfo.IsAir[block] ) return;
			
			graphics.Texturing = true;
			graphics.AlphaTest = true;
			graphics.BindTexture( sideTexId );
			graphics.SetBatchFormat( VertexFormat.P3fT2fC4b );
			graphics.BindVb( sidesVb );
			graphics.DrawIndexedVb_TrisT2fC4b( sidesVertices * 6 / 4, 0 );
			graphics.Texturing = false;
			graphics.AlphaTest = false;
		}
		
		public void RenderEdges( double delta ) {
			if( edgesVb == -1 ) return;
			byte block = game.World.Env.EdgeBlock;
			if( game.BlockInfo.IsAir[block] ) return;
			
			Vector3 camPos = game.CurrentCameraPos;
			graphics.AlphaBlending = true;
			graphics.Texturing = true;
			graphics.AlphaTest = true;
			
			graphics.BindTexture( edgeTexId );
			graphics.SetBatchFormat( VertexFormat.P3fT2fC4b );
			graphics.BindVb( edgesVb );
			// Do not draw water when we cannot see it.
			// Fixes some 'depth bleeding through' issues with 16 bit depth buffers on large maps.
			float yVisible = Math.Min( 0, map.Env.SidesHeight );
			if( camPos.Y >= yVisible )
				graphics.DrawIndexedVb_TrisT2fC4b( edgesVertices * 6 / 4, 0 );
			
			graphics.AlphaBlending = false;
			graphics.Texturing = false;
			graphics.AlphaTest = false;
		}
		
		public void Dispose() {
			game.WorldEvents.EnvVariableChanged -= EnvVariableChanged;
			game.Events.ViewDistanceChanged -= ResetSidesAndEdges;
			game.Events.TerrainAtlasChanged -= ResetTextures;
			
			graphics.DeleteTexture( ref edgeTexId );
			graphics.DeleteTexture( ref sideTexId );
			graphics.DeleteVb( sidesVb );
			graphics.DeleteVb( edgesVb );
			sidesVb = edgesVb = -1;
		}

		public void Ready( Game game ) { }
		public void Reset( Game game ) { OnNewMap( game ); }
		
		public void OnNewMap( Game game ) {
			graphics.DeleteVb( sidesVb );
			graphics.DeleteVb( edgesVb );
			sidesVb = edgesVb = -1;
			
			MakeTexture( ref edgeTexId, ref lastEdgeTexLoc, map.Env.EdgeBlock );
			MakeTexture( ref sideTexId, ref lastSideTexLoc, map.Env.SidesBlock );
		}
		
		public void OnNewMapLoaded( Game game ) {
			CalculateRects( game.ViewDistance );
			RebuildSides( map.Env.SidesHeight, legacy ? 128 : 65536 );
			RebuildEdges( map.Env.EdgeHeight, legacy ? 128 : 65536 );
		}
		
		void EnvVariableChanged( object sender, EnvVarEventArgs e ) {
			if( e.Var == EnvVar.EdgeBlock ) {
				MakeTexture( ref edgeTexId, ref lastEdgeTexLoc, map.Env.EdgeBlock );
				if( game.BlockInfo.BlocksLight[map.Env.EdgeBlock] != fullColEdge )
					ResetSidesAndEdges( null, null );
			} else if( e.Var == EnvVar.SidesBlock ) {
				MakeTexture( ref sideTexId, ref lastSideTexLoc, map.Env.SidesBlock );
				if( game.BlockInfo.BlocksLight[map.Env.SidesBlock] != fullColSides )
					ResetSidesAndEdges( null, null );
			} else if( e.Var == EnvVar.EdgeLevel ) {
				ResetSidesAndEdges( null, null );
			} else if( e.Var == EnvVar.SunlightColour ) {
				ResetSidesAndEdges( null, null );
			}
		}
		
		void ResetTextures( object sender, EventArgs e ) {
			lastEdgeTexLoc = lastSideTexLoc = -1;
			MakeTexture( ref edgeTexId, ref lastEdgeTexLoc, map.Env.EdgeBlock );
			MakeTexture( ref sideTexId, ref lastSideTexLoc, map.Env.SidesBlock );
		}

		void ResetSidesAndEdges( object sender, EventArgs e ) {
			if( game.World.IsNotLoaded ) return;
			graphics.DeleteVb( sidesVb );
			graphics.DeleteVb( edgesVb );
			
			CalculateRects( game.ViewDistance );
			RebuildSides( map.Env.SidesHeight, legacy ? 128 : 65536 );
			RebuildEdges( map.Env.EdgeHeight, legacy ? 128 : 65536 );
		}
		
		void RebuildSides( int y, int axisSize ) {
			byte block = game.World.Env.SidesBlock;
			sidesVertices = 0;
			foreach( Rectangle r in rects )
				sidesVertices += Utils.CountVertices( r.Width, r.Height, axisSize ); // YQuads outside
			sidesVertices += Utils.CountVertices( map.Width, map.Length, axisSize ); // YQuads beneath map
			sidesVertices += 2 * Utils.CountVertices( map.Width, Math.Abs( y ), axisSize ); // ZQuads
			sidesVertices += 2 * Utils.CountVertices( map.Length, Math.Abs( y ), axisSize ); // XQuads
			VertexP3fT2fC4b* v = stackalloc VertexP3fT2fC4b[sidesVertices];
			IntPtr ptr = (IntPtr)v;
			
			fullColSides = game.BlockInfo.FullBright[block];
			FastColour col = fullColSides ? FastColour.White : map.Env.Shadowlight;
			foreach( Rectangle r in rects )
				DrawY( r.X, r.Y, r.X + r.Width, r.Y + r.Height, y, axisSize, col, 0, YOffset( block ), ref v );
			
			// Work properly for when ground level is below 0
			int y1 = 0, y2 = y;
			if( y < 0 ) { y1 = y; y2 = 0; }
			DrawY( 0, 0, map.Width, map.Length, 0, axisSize, col, 0, 0, ref v );
			DrawZ( 0, 0, map.Width, y1, y2, axisSize, col, ref v );
			DrawZ( map.Length, 0, map.Width, y1, y2, axisSize, col, ref v );
			DrawX( 0, 0, map.Length, y1, y2, axisSize, col, ref v );
			DrawX( map.Width, 0, map.Length, y1, y2, axisSize, col, ref v );
			sidesVb = graphics.CreateVb( ptr, VertexFormat.P3fT2fC4b, sidesVertices );
		}
		
		void RebuildEdges( int y, int axisSize ) {
			byte block = game.World.Env.EdgeBlock;
			edgesVertices = 0;
			foreach( Rectangle r in rects )
				edgesVertices += Utils.CountVertices( r.Width, r.Height, axisSize ); // YPlanes outside
			VertexP3fT2fC4b* v = stackalloc VertexP3fT2fC4b[edgesVertices];
			IntPtr ptr = (IntPtr)v;
			
			fullColEdge = game.BlockInfo.FullBright[block];
			FastColour col = fullColEdge ? FastColour.White : map.Env.Sunlight;
			foreach( Rectangle r in rects )
				DrawY( r.X, r.Y, r.X + r.Width, r.Y + r.Height, y, axisSize, col, -0.1f/16, YOffset( block ), ref v );
			edgesVb = graphics.CreateVb( ptr, VertexFormat.P3fT2fC4b, edgesVertices );
		}
		
		float YOffset( byte block ) {
			BlockInfo info = game.BlockInfo;
			float offset = 0;
			if( info.IsTranslucent[block] && info.Collide[block] != CollideType.Solid )
				offset -= 0.1f/16;
			if( info.IsLiquid[block] )
			   offset -= 1.5f/16;
			 return offset;
		}
		
		void DrawX( int x, int z1, int z2, int y1, int y2, int axisSize, 
		           FastColour col, ref VertexP3fT2fC4b* v ) {
			int endZ = z2, endY = y2, startY = y1;
			for( ; z1 < endZ; z1 += axisSize ) {
				z2 = z1 + axisSize;
				if( z2 > endZ ) z2 = endZ;
				y1 = startY;
				for( ; y1 < endY; y1 += axisSize ) {
					y2 = y1 + axisSize;
					if( y2 > endY ) y2 = endY;
					
					TextureRec rec = new TextureRec( 0, 0, z2 - z1, y2 - y1 );
					*v = new VertexP3fT2fC4b( x, y1, z1, rec.U1, rec.V2, col ); v++;
					*v = new VertexP3fT2fC4b( x, y2, z1, rec.U1, rec.V1, col ); v++;
					*v = new VertexP3fT2fC4b( x, y2, z2, rec.U2, rec.V1, col ); v++;
					*v = new VertexP3fT2fC4b( x, y1, z2, rec.U2, rec.V2, col ); v++;
				}
			}
		}
		
		void DrawZ( int z, int x1, int x2, int y1, int y2, int axisSize, 
		           FastColour col, ref VertexP3fT2fC4b* v ) {
			int endX = x2, endY = y2, startY = y1;
			for( ; x1 < endX; x1 += axisSize ) {
				x2 = x1 + axisSize;
				if( x2 > endX ) x2 = endX;
				y1 = startY;
				for( ; y1 < endY; y1 += axisSize ) {
					y2 = y1 + axisSize;
					if( y2 > endY ) y2 = endY;
					
					TextureRec rec = new TextureRec( 0, 0, x2 - x1, y2 - y1 );
					*v = new VertexP3fT2fC4b( x1, y1, z, rec.U1, rec.V2, col ); v++;
					*v = new VertexP3fT2fC4b( x1, y2, z, rec.U1, rec.V1, col ); v++;
					*v = new VertexP3fT2fC4b( x2, y2, z, rec.U2, rec.V1, col ); v++;
					*v = new VertexP3fT2fC4b( x2, y1, z, rec.U2, rec.V2, col ); v++;
				}
			}
		}
		
		void DrawY( int x1, int z1, int x2, int z2, float y, int axisSize, 
		           FastColour col, float offset, float yOffset, ref VertexP3fT2fC4b* v ) {
			int endX = x2, endZ = z2, startZ = z1;
			for( ; x1 < endX; x1 += axisSize ) {
				x2 = x1 + axisSize;
				if( x2 > endX ) x2 = endX;
				z1 = startZ;
				for( ; z1 < endZ; z1 += axisSize ) {
					z2 = z1 + axisSize;
					if( z2 > endZ ) z2 = endZ;
					
					TextureRec rec = new TextureRec( 0, 0, x2 - x1, z2 - z1 );
					*v = new VertexP3fT2fC4b( x1 + offset, y + yOffset, z1 + offset, rec.U1, rec.V1, col ); v++;
					*v = new VertexP3fT2fC4b( x1 + offset, y + yOffset, z2 + offset, rec.U1, rec.V2, col ); v++;
					*v = new VertexP3fT2fC4b( x2 + offset, y + yOffset, z2 + offset, rec.U2, rec.V2, col ); v++;
					*v = new VertexP3fT2fC4b( x2 + offset, y + yOffset, z1 + offset, rec.U2, rec.V1, col ); v++;
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
		void MakeTexture( ref int texId, ref int lastTexLoc, byte block ) {
			int texLoc = game.BlockInfo.GetTextureLoc( block, Side.Top );
			if( texLoc == lastTexLoc ) return;
			lastTexLoc = texLoc;
			game.Graphics.DeleteTexture( ref texId );
			texId = game.TerrainAtlas.LoadTextureElement( texLoc );
		}
	}
}