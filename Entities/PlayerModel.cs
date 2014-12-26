using System;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp.Entities {

	public class PlayerModel {
		
		public ModelSet Set64x32, Set64x64, Set64x64Slim;
		static readonly FastColour col = new FastColour( 178, 178, 178 );
		IGraphicsApi graphics;
		VertexPos3fTex2fCol4b[] vertices;
		int index = 0;
		
		public PlayerModel( IGraphicsApi graphics ) {
			this.graphics = graphics;
			vertices = new VertexPos3fTex2fCol4b[6 * 6];		
			Set64x32 = new ModelSet();
			Set64x32.Head = MakeHead( false );			
			Set64x32.Torso = MakeTorso( false );	
			Set64x32.LeftLeg = MakeLeftLeg( 0, 16, 0.25f, 0f, false );			
			Set64x32.RightLeg = MakeRightLeg( 0, 16, 0, 0.25f, false );			
			Set64x32.LeftArm = MakeLeftArm( 40, 16, 0.5f, 0.25f, 4, false );			
			Set64x32.RightArm = MakeRightArm( 40, 16, 0.25f, 0.5f, 4, false );
			Set64x32.Hat = MakeHat( false );
			
			Set64x64 = new ModelSet();
			Set64x64.Head = MakeHead( true );		
			Set64x64.Torso = MakeTorso( true );			
			Set64x64.LeftLeg = MakeLeftLeg( 16, 48, 0, 0.25f, true );		
			Set64x64.RightLeg = MakeRightLeg( 0, 16, 0, 0.25f, true );		
			Set64x64.LeftArm = MakeLeftArm( 32, 48, 0.25f, 0.5f, 4, true );		
			Set64x64.RightArm = MakeRightArm( 40, 16, 0.25f, 0.5f, 4, true );			
			Set64x64.Hat = MakeHat( true );
			
			Set64x64Slim = new ModelSet();
			Set64x64Slim.Head = MakeHead( true );			
			Set64x64Slim.Torso = MakeTorso( true );	
			Set64x64Slim.LeftLeg = MakeLeftLeg( 16, 48, 0, 0.25f, true );		
			Set64x64Slim.RightLeg = MakeRightLeg( 0, 16, 0, 0.25f, true );		
			Set64x64Slim.LeftArm = MakeLeftArm( 32, 48, 0.25f, 0.4375f, 3, true );			
			Set64x64Slim.RightArm = MakeRightArm( 40, 16, 0.25f, 0.4375f, 3, true );	
			Set64x64Slim.Hat = MakeHat( true );
			vertices = null;
		}
		
		public void Dispose() {
			Set64x32.Dispose();
			Set64x64.Dispose();
			Set64x64Slim.Dispose();
		}
		
		ModelPart MakeLeftArm( int x, int y, float x1, float x2, int width, bool _64x64 ) {
			return MakePart( x, y, 4, 12, width, 4, width, 12, -x2, -x1, 0.875f, 1.625f, -0.125f, 0.125f, _64x64 );
		}
		
		ModelPart MakeRightArm( int x, int y, float x1, float x2, int width, bool _64x64 ) {
			return MakePart( x, y, 4, 12, width, 4, width, 12, x1, x2, 0.875f, 1.625f, -0.125f, 0.125f, _64x64 );
		}
		
		ModelPart MakeHead( bool _64x64 ) {
			return MakePart( 0, 0, 8, 8, 8, 8, 8, 8, -0.25f, 0.25f, 1.625f, 2.125f, -0.25f, 0.25f, _64x64 );
		}
		
		ModelPart MakeTorso( bool _64x64 ) {
			return MakePart( 16, 16, 4, 12, 8, 4, 8, 12, -0.25f, 0.25f, 0.875f, 1.625f, -0.125f, 0.125f, _64x64 );
		}
		
		ModelPart MakeHat( bool _64x64 ) {
			return MakePart( 32, 0, 8, 8, 8, 8, 8, 8, -0.3125f, 0.3125f, 1.5625f, 2.18775f, -0.3125f, 0.3125f, _64x64 );
		}	
		
		ModelPart MakeLeftLeg( int x, int y, float x1, float x2, bool _64x64 ) {
			return MakePart( x, y, 4, 12, 4, 4, 4, 12, -x2, -x1, 0f, 0.875f, -0.125f, 0.125f, _64x64 );
		}
		
		ModelPart MakeRightLeg( int x, int y, float x1, float x2, bool _64x64 ) {
			return MakePart( x, y, 4, 12, 4, 4, 4, 12, x1, x2, 0f, 0.875f, -0.125f, 0.125f, _64x64 );
		}
		
		ModelPart MakePart( int x, int y, int sidesW, int sidesH, int endsW, int endsH, int bodyW, int bodyH,
		                   float x1, float x2, float y1, float y2, float z1, float z2, bool _64x64 ) {
			index = 0;
			YPlane( x + sidesW, y, endsW, endsH, x2, x1, z2, z1, y2, _64x64 ); // top
			YPlane( x + sidesW + bodyW, y, endsW, endsH, x2, x1, z1, z2, y1, _64x64 ); // bottom
			ZPlane( x + sidesW, y + endsH, bodyW, bodyH, x2, x1, y1, y2, z1, _64x64 ); // front
			ZPlane( x + sidesW + bodyW + sidesW, y + endsH, bodyW, bodyH, x1, x2, y1, y2, z2, _64x64 ); // back
			XPlane( x + sidesW + bodyW, y + endsH, sidesW, sidesH, z1, z2, y1, y2, x1, _64x64 ); // left
			XPlane( x, y + endsH, sidesW, sidesH, z2, z1, y1, y2, x2, _64x64 ); // right
			return new ModelPart( vertices, graphics );
		}
		
		static TextureRectangle SkinTexCoords( int x, int y, int width, int height, float skinWidth, float skinHeight ) {
			return new TextureRectangle( x / skinWidth, y / skinHeight, width / skinWidth, height / skinHeight );
		}
		
		void XPlane( int texX, int texY, int texWidth, int texHeight,
		                   float z1, float z2, float y1, float y2, float x, bool _64x64 ) {
			TextureRectangle rec = SkinTexCoords( texX, texY, texWidth, texHeight, 64, _64x64 ? 64 : 32 );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y1, z1, rec.U1, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y2, z1, rec.U1, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y2, z2, rec.U2, rec.V1, col );
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y2, z2, rec.U2, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y1, z2, rec.U2, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y1, z1, rec.U1, rec.V2, col );
		}
		
		void YPlane( int texX, int texY, int texWidth, int texHeight,
		                   float x1, float x2, float z1, float z2, float y, bool _64x64 ) {
			TextureRectangle rec = SkinTexCoords( texX, texY, texWidth, texHeight, 64, _64x64 ? 64 : 32 );
			vertices[index++] = new VertexPos3fTex2fCol4b( x1, y, z1, rec.U1, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x2, y, z1, rec.U2, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x2, y, z2, rec.U2, rec.V2, col );
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x2, y, z2, rec.U2, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x1, y, z2, rec.U1, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x1, y, z1, rec.U1, rec.V1, col );
		}
		
		void ZPlane( int texX, int texY, int texWidth, int texHeight,
		                   float x1, float x2, float y1, float y2, float z, bool _64x64 ) {
			TextureRectangle rec = SkinTexCoords( texX, texY, texWidth, texHeight, 64, _64x64 ? 64 : 32 );
			vertices[index++] = new VertexPos3fTex2fCol4b( x1, y1, z, rec.U1, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x2, y1, z, rec.U2, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x2, y2, z, rec.U2, rec.V1, col );
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x2, y2, z, rec.U2, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x1, y2, z, rec.U1, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x1, y1, z, rec.U1, rec.V2, col );
		}
	}
}