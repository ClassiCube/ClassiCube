using System;
using ClassicalSharp.Blocks.Model;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Renderers {
	
	public class PickingRenderer {
		
		Game window;
		IGraphicsApi graphics;
		CubeModel cracks;
		
		public PickingRenderer( Game window ) {
			this.window = window;
			graphics = window.Graphics;
			cracks = new CubeModel( window, (byte)BlockId.Stone );
		}
		
		FastColour col = FastColour.White;
		double accumulator;
		
		int index = 0, crackIndex = 0;
		VertexPos3fCol4b[] vertices = new VertexPos3fCol4b[24 * ( 3 * 2 )];
		VertexPos3fTex2fCol4b[] crackVertices = new VertexPos3fTex2fCol4b[6 * 6];
		const float size = 0.0625f;
		const float offset = 0.01f;
		public void Render( double delta ) {
			accumulator += delta;
			index = crackIndex = 0;
			PickedPos pickedPos = window.SelectedPos;
			
			if( pickedPos != null ) {
				Vector3 min = pickedPos.Min;
				Vector3 max = pickedPos.Max;
				
				// bottom face
				DrawYPlane( min.Y - offset, min.X - offset, min.Z - offset, min.X + size - offset, max.Z + offset );
				DrawYPlane( min.Y - offset, max.X + offset, min.Z - offset, max.X - size + offset, max.Z + offset );
				DrawYPlane( min.Y - offset, min.X - offset, min.Z - offset, max.X + offset, min.Z + size - offset );
				DrawYPlane( min.Y - offset, min.X - offset, max.Z + offset, max.X + offset, max.Z - size + offset );
				// top face
				DrawYPlane( max.Y + offset, min.X - offset, min.Z - offset, min.X + size - offset, max.Z + offset );
				DrawYPlane( max.Y + offset, max.X + offset, min.Z - offset, max.X - size + offset, max.Z + offset );
				DrawYPlane( max.Y + offset, min.X - offset, min.Z - offset, max.X + offset, min.Z + size - offset );
				DrawYPlane( max.Y + offset, min.X - offset, max.Z + offset, max.X + offset, max.Z - size + offset );
				// left face
				DrawXPlane( min.X - offset, min.Z - offset, min.Y - offset, min.Z + size - offset, max.Y + offset );
				DrawXPlane( min.X - offset, max.Z + offset, min.Y - offset, max.Z - size + offset, max.Y + offset );
				DrawXPlane( min.X - offset, min.Z - offset, min.Y - offset, max.Z + offset, min.Y + size - offset );
				DrawXPlane( min.X - offset, min.Z - offset, max.Y + offset, max.Z + offset, max.Y - size + offset );
				// right face
				DrawXPlane( max.X + offset, min.Z - offset, min.Y - offset, min.Z + size - offset, max.Y + offset );
				DrawXPlane( max.X + offset, max.Z + offset, min.Y - offset, max.Z - size + offset, max.Y + offset );
				DrawXPlane( max.X + offset, min.Z - offset, min.Y - offset, max.Z + offset, min.Y + size - offset );
				DrawXPlane( max.X + offset, min.Z - offset, max.Y + offset, max.Z + offset, max.Y - size + offset );
				// front face
				DrawZPlane( min.Z - offset, min.X - offset, min.Y - offset, min.X + size - offset, max.Y + offset );
				DrawZPlane( min.Z - offset, max.X + offset, min.Y - offset, max.X - size + offset, max.Y + offset );
				DrawZPlane( min.Z - offset, min.X - offset, min.Y - offset, max.X + offset, min.Y + size - offset );
				DrawZPlane( min.Z - offset, min.X - offset, max.Y + offset, max.X + offset, max.Y - size + offset );
				// back face
				DrawZPlane( max.Z + offset, min.X - offset, min.Y - offset, min.X + size - offset, max.Y + offset );
				DrawZPlane( max.Z + offset, max.X + offset, min.Y - offset, max.X - size + offset, max.Y + offset );
				DrawZPlane( max.Z + offset, min.X - offset, min.Y - offset, max.X + offset, min.Y + size - offset );
				DrawZPlane( max.Z + offset, min.X - offset, max.Y + offset, max.X + offset, max.Y - size + offset );
				graphics.DrawVertices( DrawMode.Triangles, vertices );
				
				if( window.digging ) {
					int stage = (int)( window.digAccumulator * 9 );
					graphics.Texturing = true;
					graphics.AlphaTest = true;
					graphics.DepthTestFunc( DepthFunc.LessEqual );
					graphics.Bind2DTexture( window.TerrainAtlasTexId );
					int texIndex = 144 + 5 + stage;
					TextureRectangle rec = window.TerrainAtlas.GetTexRec( texIndex );				
					for( int i = 0; i < 6; i++ ) {
						cracks.recs[i] = rec;
						cracks.DrawFace( i, 0, new Neighbours(), ref crackIndex, min.X, min.Y, min.Z, crackVertices, FastColour.White );
					}
					graphics.DrawVertices( DrawMode.Triangles, crackVertices );
					graphics.AlphaTest = false;
					graphics.DepthTestFunc( DepthFunc.Less );
					graphics.Texturing = false;
				}
			}
		}
		
		void DrawXPlane( float x, float z1, float y1, float z2, float y2 ) {
			vertices[index++] = new VertexPos3fCol4b( x, y1, z1, col );
			vertices[index++] = new VertexPos3fCol4b( x, y2, z1, col );
			vertices[index++] = new VertexPos3fCol4b( x, y2, z2, col );
			
			vertices[index++] = new VertexPos3fCol4b( x, y2, z2, col );
			vertices[index++] = new VertexPos3fCol4b( x, y1, z2, col );
			vertices[index++] = new VertexPos3fCol4b( x, y1, z1, col );
		}
		
		void DrawZPlane( float z, float x1, float y1, float x2, float y2 ) {
			vertices[index++] = new VertexPos3fCol4b( x1, y1, z, col );
			vertices[index++] = new VertexPos3fCol4b( x1, y2, z, col );
			vertices[index++] = new VertexPos3fCol4b( x2, y2, z, col );
			
			vertices[index++] = new VertexPos3fCol4b( x2, y2, z, col );
			vertices[index++] = new VertexPos3fCol4b( x2, y1, z, col );
			vertices[index++] = new VertexPos3fCol4b( x1, y1, z, col );
		}
		
		void DrawYPlane( float y, float x1, float z1, float x2, float z2 ) {
			vertices[index++] = new VertexPos3fCol4b( x1, y, z1, col );
			vertices[index++] = new VertexPos3fCol4b( x1, y, z2, col );
			vertices[index++] = new VertexPos3fCol4b( x2, y, z2, col );
			
			vertices[index++] = new VertexPos3fCol4b( x2, y, z2, col );
			vertices[index++] = new VertexPos3fCol4b( x2, y, z1, col );
			vertices[index++] = new VertexPos3fCol4b( x1, y, z1, col );
		}
	}
}
