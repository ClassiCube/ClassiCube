using OpenTK;
using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Renderers;

namespace ClassicalSharp.Model {

	public class PigModel : IModel {
		
		ModelSet Set;
		public PigModel( Game window ) : base( window ) {
			vertices = new VertexPos3fTex2fCol4b[6 * 6];
			Set = new ModelSet();
			Set.Head = MakeHead();
			Set.Torso = MakeTorso();
			Set.LeftLegFront = MakeLeg( -0.3125f, -0.0625f, -0.4375f, -0.1875f );
			Set.RightLegFront = MakeLeg( 0.0625f, 0.3125f, -0.4375f, -0.1875f );
			Set.LeftLegBack = MakeLeg( -0.3125f, -0.0625f, 0.3125f, 0.5625f );
			Set.RightLegBack = MakeLeg( 0.0625f, 0.3125f, 0.3125f, 0.5625f );
			vertices = null;

			DefaultSkinTextureId = graphics.LoadTexture( "pig.png" );
		}
		
		ModelPart MakeHead() {
			return MakePart( 0, 0, 8, 8, 8, 8, 8, 8, -0.25f, 0.25f, 0.5f, 1f, -0.875f, -0.375f, false );
		}
		
		ModelPart MakeTorso() {
			//return MakePart( 28, 8, 8, 16, 10, 4, 8, 16, -0.3125f, 0.3125f, 0.375f, 0.875f, -0.5f, 0.5f, false );
			index = 0;
			const float x1 = -0.3125f, x2 = 0.3125f, y1 = 0.375f, y2 = 0.875f, z1 = -0.5f, z2 = 0.5f;
			
			YPlane( 54, 16, 10, 16, x1, x2, z1, z2, y2, false ); // top
			YPlane( 36, 16, 10, 16, x2, x1, z1, z2, y1, false ); // bottom
			ZPlane( 36, 8, 10, 8, x2, x1, y1, y2, z1, false ); // front
			ZPlane( 46, 8, 10, 8, x1, x2, y1, y2, z2, false ); // back
			XPlane( 46, 16, 8, 16, y1, y2, z2, z1, x1, false ); // left
			XPlane( 28, 16, 8, 16, y2, y1, z2, z1, x2, false ); // right
			// rotate left and right 90 degrees
			for( int i = index - 12; i < index; i++ ) {
				VertexPos3fTex2fCol4b vertex = vertices[i];
				float z = vertex.Z;
				vertex.Z = vertex.Y;
				vertex.Y = z;
				vertices[i] = vertex;
			}
			return new ModelPart( vertices, graphics );
		}
		
		ModelPart MakeLeg( float x1, float x2, float z1, float z2 ) {
			return MakePart( 0, 16, 4, 6, 4, 4, 4, 6, x1, x2, 0f, 0.375f, z1, z2, false );
		}
		
		public override float NameYOffset {
			get { return 1.7f; }
		}
		
		Vector3 pos;
		float yaw, pitch;
		float leftLegXRot, rightLegXRot;
		
		public override void RenderModel( Player player, PlayerRenderer renderer ) {
			pos = player.Position;
			yaw = player.YawDegrees;
			pitch = player.PitchDegrees;
			
			leftLegXRot = player.leftLegXRot * 180 / (float)Math.PI;
			rightLegXRot = player.rightLegXRot * 180 / (float)Math.PI;
			
			graphics.PushMatrix();
			graphics.Translate( pos.X, pos.Y, pos.Z );
			graphics.RotateY( -yaw );
			DrawPlayerModel( player, renderer );
			graphics.PopMatrix();
		}
		
		private void DrawPlayerModel( Player player, PlayerRenderer renderer ) {
			graphics.Texturing = true;
			int texId = DefaultSkinTextureId;
			graphics.Bind2DTexture( texId );
			
			DrawRotateX( 0, 0.75f, -0.375f, -pitch, Set.Head );
			Set.Torso.Render();
			DrawRotateX( 0, 0.375f, -0.3125f, leftLegXRot, Set.LeftLegFront );
			DrawRotateX( 0, 0.375f, -0.3125f, rightLegXRot, Set.RightLegFront );
			DrawRotateX( 0, 0.375f, 0.4375f, rightLegXRot, Set.LeftLegBack );
			DrawRotateX( 0, 0.375f, 0.4375f, leftLegXRot, Set.RightLegBack );
			graphics.AlphaTest = true;
		}
		
		public override void Dispose() {
			Set.Dispose();
			graphics.DeleteTexture( DefaultSkinTextureId );
		}
		
		class ModelSet {
			
			public ModelPart Head, Torso, LeftLegFront, RightLegFront, LeftLegBack, RightLegBack;
			
			public void Dispose() {
				RightLegFront.Dispose();
				LeftLegFront.Dispose();
				RightLegBack.Dispose();
				LeftLegBack.Dispose();
				Torso.Dispose();
				Head.Dispose();
			}
		}
	}
}