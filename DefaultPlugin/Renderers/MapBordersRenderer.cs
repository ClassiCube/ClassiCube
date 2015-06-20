using System;
using System.Collections.Generic;
using System.Drawing;
using ClassicalSharp;
using ClassicalSharp.GraphicsAPI;

namespace DefaultPlugin {
	
	public sealed class StandardMapBordersRenderer : MapBordersRenderer {
		
		MapEnvShader shader;	
		public StandardMapBordersRenderer( Game window ) : base( window ) {
		}
		
		int sidesVboId = -1, edgesVboId = -1;
		int edgeTexId, sideTexId;
		int sidesVertices, edgesVertices, index;
		static readonly FastColour sidesCol = new FastColour( 128, 128, 128 ), edgesCol = FastColour.White;
		
		public override void Init() {
			base.Init();
			Window.ViewDistanceChanged += ResetSidesAndEdges;
			Window.TerrainAtlasChanged += ResetTextures;
			
			MakeTexture( ref edgeTexId, ref lastEdgeTexLoc, map.EdgeBlock );
			MakeTexture( ref sideTexId, ref lastSideTexLoc, map.SidesBlock );
			ResetSidesAndEdges( null, null );
			shader = new MapEnvShader();
			shader.Init( api );
		}

		public override void RenderMapSides( double deltaTime ) {
			if( sidesVboId == -1 ) return;
			
			api.UseProgram( shader.ProgramId );
			shader.UpdateFogAndMVPState( ref Window.MVP );
			
			api.Bind2DTexture( sideTexId );
			shader.Draw( DrawMode.Triangles, VertexPos3fTex2fCol4b.Size, sidesVboId, 0, sidesVertices );
		}
		
		public override void RenderMapEdges( double deltaTime ) {
			if( edgesVboId == -1 ) return;
			// Do not draw water when we cannot see it.
			// Fixes 'depth bleeding through' issues with 16 bit depth buffers on large maps.
			if( Window.LocalPlayer.EyePosition.Y < 0 ) return;
			
			api.AlphaBlending = true;
			api.Bind2DTexture( edgeTexId );
			shader.Draw( DrawMode.Triangles, VertexPos3fTex2fCol4b.Size, edgesVboId, 0, edgesVertices );
			api.AlphaBlending = false;
		}
		
		public override void Dispose() {
			base.Dispose();
			Window.ViewDistanceChanged -= ResetSidesAndEdges;
			Window.TerrainAtlasChanged -= ResetTextures;
			
			api.DeleteTexture( ref edgeTexId );
			api.DeleteTexture( ref sideTexId );
			api.DeleteVb( sidesVboId );
			api.DeleteVb( edgesVboId );
			sidesVboId = edgesVboId = -1;
		}
		
		protected override void OnNewMap( object sender, EventArgs e ) {
			api.DeleteVb( sidesVboId );
			api.DeleteVb( edgesVboId );
			sidesVboId = edgesVboId = -1;
			MakeTexture( ref edgeTexId, ref lastEdgeTexLoc, map.EdgeBlock );
			MakeTexture( ref sideTexId, ref lastSideTexLoc, map.SidesBlock );
		}
		
		protected override void OnNewMapLoaded( object sender, EventArgs e ) {
			RebuildSides();
			RebuildEdges();
		}
		
		protected override void EnvVariableChanged( object sender, EnvVariableEventArgs e ) {
			if( e.Variable == EnvVariable.EdgeBlock ) {
				MakeTexture( ref edgeTexId, ref lastEdgeTexLoc, map.EdgeBlock );
			} else if( e.Variable == EnvVariable.SidesBlock ) {
				MakeTexture( ref sideTexId, ref lastSideTexLoc, map.SidesBlock );
			} else if( e.Variable == EnvVariable.WaterLevel ) {
				ResetSidesAndEdges( null, null );
			}
		}
		
		void ResetTextures( object sender, EventArgs e ) {
			lastEdgeTexLoc = lastSideTexLoc = -1;
			MakeTexture( ref edgeTexId, ref lastEdgeTexLoc, map.EdgeBlock );
			MakeTexture( ref sideTexId, ref lastSideTexLoc, map.SidesBlock );
		}

		void ResetSidesAndEdges( object sender, EventArgs e ) {
			if( Window.Map.IsNotLoaded ) return;
			RebuildSides();
			RebuildEdges();
		}
		
		void RebuildSides() {
			index = 0;
			api.DeleteVb( sidesVboId );
			sidesVertices = 5 * 6 + 4 * 6;
			int groundLevel = map.GroundHeight;
			VertexPos3fTex2fCol4b[] vertices = new VertexPos3fTex2fCol4b[sidesVertices];
			
			foreach( Rectangle rec in OutsideMap( Window.ViewDistance ) ) {
				DrawYPlane( rec.X, rec.Y, rec.X + rec.Width, rec.Y + rec.Height, groundLevel, sidesCol, vertices );
			}
			DrawYPlane( 0, 0, map.Width, map.Length, 0, sidesCol, vertices );
			DrawZPlane( 0, 0, map.Width, 0, groundLevel, sidesCol, vertices );
			DrawZPlane( map.Length, 0, map.Width, 0, groundLevel, sidesCol, vertices );
			DrawXPlane( 0, 0, map.Length, 0, groundLevel, sidesCol, vertices );
			DrawXPlane( map.Width, 0, map.Length, 0, groundLevel, sidesCol, vertices  );			
			sidesVboId = api.InitVb( vertices, VertexPos3fTex2fCol4b.Size );
		}
		
		void RebuildEdges() {
			index = 0;
			api.DeleteVb( edgesVboId );
			edgesVertices = 4 * 6;
			int waterLevel = map.WaterHeight;
			VertexPos3fTex2fCol4b[] vertices = new VertexPos3fTex2fCol4b[edgesVertices];
			
			foreach( Rectangle rec in OutsideMap( Window.ViewDistance ) ) {
				DrawYPlane( rec.X, rec.Y, rec.X + rec.Width, rec.Y + rec.Height, waterLevel, edgesCol, vertices );
			}			
			edgesVboId = api.InitVb( vertices, VertexPos3fTex2fCol4b.Size );
		}

		int lastEdgeTexLoc, lastSideTexLoc;
		void MakeTexture( ref int texId, ref int lastTexLoc, Block block ) {
			int texLoc = Window.BlockInfo.GetOptimTextureLoc( (byte)block, TileSide.Top );
			if( texLoc != lastTexLoc ) {
				lastTexLoc = texLoc;
				Window.Graphics.DeleteTexture( ref texId );
				texId = Window.TerrainAtlas.LoadTextureElement( texLoc );
			}
		}	
		
		void DrawXPlane( int x, int z1, int z2, int y1, int y2, FastColour col, VertexPos3fTex2fCol4b[] vertices ) {
			TextureRectangle rec = new TextureRectangle( 0, 0, z2 - z1, y2 - y1 );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y1, z1, rec.U1, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y2, z1, rec.U1, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y2, z2, rec.U2, rec.V2, col );
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y2, z2, rec.U2, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y1, z2, rec.U2, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y1, z1, rec.U1, rec.V1, col );
		}
		
		void DrawYPlane( int x1, int z1, int x2, int z2, int y, FastColour col, VertexPos3fTex2fCol4b[] vertices ) {
			TextureRectangle rec = new TextureRectangle( 0, 0, x2 - x1, z2 - z1 );
			vertices[index++] = new VertexPos3fTex2fCol4b( x1, y, z1, rec.U1, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x1, y, z2, rec.U1, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x2, y, z2, rec.U2, rec.V2, col );
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x2, y, z2, rec.U2, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x2, y, z1, rec.U2, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x1, y, z1, rec.U1, rec.V1, col );
		}
		
		void DrawZPlane( int z, int x1, int x2, int y1, int y2, FastColour col, VertexPos3fTex2fCol4b[] vertices ) {
			TextureRectangle rec = new TextureRectangle( 0, 0, x2 - x1, y2 - y1 );
			vertices[index++] = new VertexPos3fTex2fCol4b( x1, y1, z, rec.U1, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x1, y2, z, rec.U1, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x2, y2, z, rec.U2, rec.V2, col );
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x2, y2, z, rec.U2, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x2, y1, z, rec.U2, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x1, y1, z, rec.U1, rec.V1, col );
		}
	}
	
	public sealed class MapEnvShader : FogAndMVPShader {
		
		public MapEnvShader() {
			VertexSource = @"
#version 130
in vec3 in_position;
in vec4 in_colour;
in vec2 in_texcoords;
flat out vec4 out_colour;
out vec2 out_texcoords;
uniform mat4 MVP;

void main() {
   gl_Position = MVP * vec4(in_position, 1.0);
   out_texcoords = in_texcoords;
   out_colour = in_colour;
}";
			
			FragmentSource = @"
#version 130
in vec2 out_texcoords;
flat in vec4 out_colour;
out vec4 final_colour;
uniform sampler2D texImage;
--IMPORT fog_uniforms

void main() {
   vec4 finalColour = texture2D(texImage, out_texcoords) * out_colour;
--IMPORT fog_code
   final_colour = finalColour;
}";
		}
		
		public int positionLoc, texCoordsLoc, colourLoc;
		public int texImageLoc;
		protected override void GetLocations() {
			positionLoc = api.GetAttribLocation( ProgramId, "in_position" );
			texCoordsLoc = api.GetAttribLocation( ProgramId, "in_texcoords" );
			colourLoc = api.GetAttribLocation( ProgramId, "in_colour" );
			
			texImageLoc = api.GetUniformLocation( ProgramId, "texImage" );
			base.GetLocations();
		}
		
		protected override void EnableVertexAttribStates( int stride ) {
			api.EnableVertexAttribF( positionLoc, 3, stride, 0 );
			api.EnableVertexAttribF( colourLoc, 4, VertexAttribType.UInt8, true, stride, 12 );
			api.EnableVertexAttribF( texCoordsLoc, 2, stride, 16 );
		}
		
		protected override void DisableVertexAttribStates( int stride ) {
			api.DisableVertexAttrib( positionLoc );
			api.DisableVertexAttrib( texCoordsLoc );
			api.DisableVertexAttrib( colourLoc );
		}
	}
}