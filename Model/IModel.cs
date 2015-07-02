using System;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Renderers;
using OpenTK;

namespace ClassicalSharp.Model {

	public abstract class IModel {
		
		protected Game window;
		protected IGraphicsApi graphics;
		protected const int planeVertices = 6;
		protected const int partVertices = 6 * planeVertices;
		
		public IModel( Game window ) {
			this.window = window;
			graphics = window.Graphics;
		}
		
		public abstract float NameYOffset { get; }
		
		public abstract Vector3 CollisionSize { get; }
		
		public abstract BoundingBox PickingBounds { get; }
		
		protected Vector3 pos;
		protected float yaw, pitch;
		protected float rightLegXRot, rightArmXRot, rightArmZRot;
		protected float leftLegXRot, leftArmXRot, leftArmZRot;
		public void RenderModel( Player player, PlayerRenderer renderer ) {
			pos = player.Position;
			yaw = player.YawRadians;
			pitch = player.PitchRadians;
			
			leftLegXRot = player.leftLegXRot;
			leftArmXRot = player.leftArmXRot;
			leftArmZRot = player.leftArmZRot;
			rightLegXRot = player.rightLegXRot;
			rightArmXRot = player.rightArmXRot;
			rightArmZRot = player.rightArmZRot;
			
			graphics.PushMatrix();
			Matrix4 mat = Matrix4.RotateY( -yaw ) * Matrix4.Translate( pos );
			graphics.MultiplyMatrix( ref mat );
			DrawPlayerModel( player, renderer );
			graphics.PopMatrix();
		}
		
		protected abstract void DrawPlayerModel( Player player, PlayerRenderer renderer );
		
		public abstract void Dispose();
		
		public int DefaultTexId; //{ get; protected set; }
		
		protected FastColour col = new FastColour( 178, 178, 178 );
		protected VertexPos3fTex2fCol4b[] vertices;
		protected int index = 0;
		protected int vb = 0;
		
		protected ModelPart MakePart( int x, int y, int sidesW, int sidesH, int endsW, int endsH, int bodyW, int bodyH,
		                             float x1, float x2, float y1, float y2, float z1, float z2, bool _64x64 ) {
			YPlane( x + sidesW, y, endsW, endsH, x2, x1, z2, z1, y2, _64x64 ); // top
			YPlane( x + sidesW + bodyW, y, endsW, endsH, x2, x1, z1, z2, y1, _64x64 ); // bottom
			ZPlane( x + sidesW, y + endsH, bodyW, bodyH, x2, x1, y1, y2, z1, _64x64 ); // front
			ZPlane( x + sidesW + bodyW + sidesW, y + endsH, bodyW, bodyH, x1, x2, y1, y2, z2, _64x64 ); // back
			XPlane( x, y + endsH, sidesW, sidesH, z2, z1, y1, y2, x2, _64x64 ); // left
			XPlane( x + sidesW + bodyW, y + endsH, sidesW, sidesH, z1, z2, y1, y2, x1, _64x64 ); // right
			return new ModelPart( this.vb, index - 36, 6 * 6, graphics );		
		}
		
		protected ModelPart MakeRotatedPart( int x, int y, int sidesW, int sidesH, int endsW, int endsH, int bodyW, int bodyH,
		                                    float x1, float x2, float y1, float y2, float z1, float z2, bool _64x64 ) {
			YPlane( x + sidesW + bodyW + sidesW, y + endsH, bodyW, bodyH, x1, x2, z1, z2, y2, _64x64 ); // top
			YPlane( x + sidesW, y + endsH, bodyW, bodyH, x2, x1, z1, z2, y1, _64x64 ); // bottom
			ZPlane( x + sidesW, y, endsW, endsH, x2, x1, y1, y2, z1, _64x64 ); // front
			ZPlane( x + sidesW + bodyW, y, endsW, endsH, x2, x1, y2, y1, z2, _64x64 ); // back
			XPlane( x, y + endsH, sidesW, sidesH, y2, y1, z2, z1, x2, _64x64 ); // left
			XPlane( x + sidesW + bodyW, y + endsH, sidesW, sidesH, y1, y2, z2, z1, x1, _64x64 ); // right
			// rotate left and right 90 degrees
			for( int i = index - 12; i < index; i++ ) {
				VertexPos3fTex2fCol4b vertex = vertices[i];
				float z = vertex.Z;
				vertex.Z = vertex.Y;
				vertex.Y = z;
				vertices[i] = vertex;
			}
			return new ModelPart( this.vb, index - 36, 6 * 6, graphics );	
		}
		
		protected static TextureRectangle SkinTexCoords( int x, int y, int width, int height, float skinWidth, float skinHeight ) {
			return new TextureRectangle( x / skinWidth, y / skinHeight, width / skinWidth, height / skinHeight );
		}
		
		protected void XPlane( int texX, int texY, int texWidth, int texHeight,
		                      float z1, float z2, float y1, float y2, float x, bool _64x64 ) {
			TextureRectangle rec = SkinTexCoords( texX, texY, texWidth, texHeight, 64, _64x64 ? 64 : 32 );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y1, z1, rec.U1, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y2, z1, rec.U1, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y2, z2, rec.U2, rec.V1, col );
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y2, z2, rec.U2, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y1, z2, rec.U2, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y1, z1, rec.U1, rec.V2, col );
		}
		
		protected void YPlane( int texX, int texY, int texWidth, int texHeight,
		                      float x1, float x2, float z1, float z2, float y, bool _64x64 ) {
			TextureRectangle rec = SkinTexCoords( texX, texY, texWidth, texHeight, 64, _64x64 ? 64 : 32 );
			vertices[index++] = new VertexPos3fTex2fCol4b( x1, y, z1, rec.U1, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x2, y, z1, rec.U2, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x2, y, z2, rec.U2, rec.V2, col );
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x2, y, z2, rec.U2, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x1, y, z2, rec.U1, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x1, y, z1, rec.U1, rec.V1, col );
		}
		
		protected void ZPlane( int texX, int texY, int texWidth, int texHeight,
		                      float x1, float x2, float y1, float y2, float z, bool _64x64 ) {
			TextureRectangle rec = SkinTexCoords( texX, texY, texWidth, texHeight, 64, _64x64 ? 64 : 32 );
			vertices[index++] = new VertexPos3fTex2fCol4b( x1, y1, z, rec.U1, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x2, y1, z, rec.U2, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x2, y2, z, rec.U2, rec.V1, col );
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x2, y2, z, rec.U2, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x1, y2, z, rec.U1, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x1, y1, z, rec.U1, rec.V2, col );
		}
		
		protected void DrawRotate( float x, float y, float z, float angleX, float angleY, float angleZ, ModelPart part ) {
			graphics.PushMatrix();
			Matrix4 mat = Matrix4.Translate( x, y, z );
			if( angleZ != 0 ) {
				mat = Matrix4.RotateZ( angleZ ) * mat;
			}
			if( angleY != 0 ) {
				mat = Matrix4.RotateY( angleY ) * mat;
			}
			if( angleX != 0 ) {
				mat = Matrix4.RotateX( angleX ) * mat;
			}
			mat = Matrix4.Translate( -x, -y, -z ) * mat;
			graphics.MultiplyMatrix( ref mat );
			part.Render();
			graphics.PopMatrix();
		}
	}
}