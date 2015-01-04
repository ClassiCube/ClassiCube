using OpenTK;
using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Renderers;

namespace ClassicalSharp.Model {

	public class ChickenModel : IModel {
		
		ModelSet Set;
		public ChickenModel( Game window ) : base( window ) {
			vertices = new VertexPos3fTex2fCol4b[6 * 6];
			Set = new ModelSet();
			Set.Head = MakeHead();
			Set.Head2 = MakeHead2(); // TODO: Find a more appropriate name.
			Set.Head3 = MakeHead3();
			Set.Torso = MakeTorso();
			Set.LeftLegFront = MakeLeg( -0.1875f, 0f, -0.125f, -0.0625f );
			Set.RightLegFront = MakeLeg( 0f, 0.1875f, 0.0625f, 0.125f );
			vertices = null;

			DefaultSkinTextureId = graphics.LoadTexture( "chicken.png" );
		}
		
		ModelPart MakeHead() {
			return MakePart( 0, 0, 3, 6, 4, 3, 4, 6, -0.125f, 0.125f, 0.5625f, 0.9375f, -0.375f, -0.1875f, false );
		}
		
		ModelPart MakeHead2() {
			return MakePart( 14, 4, 2, 2, 2, 2, 2, 2, -0.0625f, 0.0625f, 0.5625f, 0.6875f, -0.4375f, -0.3125f, false );
		}
		
		ModelPart MakeHead3() {
			return MakePart( 14, 0, 2, 2, 4, 2, 4, 2, -0.125f, 0.125f, 0.6875f, 0.8125f, -0.5f, -0.375f, false );
		}
		
		ModelPart MakeTorso() {
			index = 0;
			const float x1 = -0.1875f, x2 = 0.1875f, y1 = 0.3125f, y2 = 0.6875f, z1 = -0.25f, z2 = 0.25f;
			
			YPlane( 18, 15, 6, 8, x1, x2, z1, z2, y2, false ); // top
			YPlane( 6, 15, 6, 8, x2, x1, z1, z2, y1, false ); // bottom
			ZPlane( 6, 9, 6, 6, x2, x1, y1, y2, z1, false ); // front
			ZPlane( 12, 9, 6, 6, x2, x1, y2, y1, z2, false ); // back
			XPlane( 12, 15, 6, 8, y1, y2, z2, z1, x1, false ); // left
			XPlane( 0, 15, 6, 8, y2, y1, z2, z1, x2, false ); // right
			// rotate left and right 90 degrees
			for( int i = index - 12; i < index; i++ ) {
				VertexPos3fTex2fCol4b vertex = vertices[i];
				float z = vertex.Z;
				vertex.Z = vertex.Y;
				vertex.Y = z;
				vertices[i] = vertex;
			}
			return new ModelPart( vertices, 6 * 6, graphics );
		}
		
		ModelPart MakeLeg( float x1, float x2, float legX1, float legX2 ) {
			index = 0;
			const float y1 = 0f, y2 = 0.3125f, z2 = 0.0625f, z1 = -0.125f;		
			YPlane( 32, 0, 3, 3, x2, x1, z1, z2, y1, false ); // bottom feet
			ZPlane( 36, 3, 1, 5, legX1, legX2, y1, y2, z2, false ); // vertical part of leg
			return new ModelPart( vertices, 2 * 6, graphics );
		}
		
		public override float NameYOffset {
			get { return 1.0125f; }
		}
		
		protected override void DrawPlayerModel( Player player, PlayerRenderer renderer ) {
			graphics.Texturing = true;
			int texId = DefaultSkinTextureId;
			graphics.Bind2DTexture( texId );
			graphics.AlphaTest = true;
			
			DrawRotateX( 0, 0.5625f, -0.1875f, -pitch, Set.Head );
			DrawRotateX( 0, 0.5625f, -0.1875f, -pitch, Set.Head2 );
			DrawRotateX( 0, 0.5625f, -0.1875f, -pitch, Set.Head3 );
			Set.Torso.Render();
			DrawRotateX( 0, 0.3125f, 0.0625f, leftLegXRot, Set.LeftLegFront );
			DrawRotateX( 0, 0.3125f, 0.0625f, rightLegXRot, Set.RightLegFront );
		}
		
		public override void Dispose() {
			Set.Dispose();
			graphics.DeleteTexture( DefaultSkinTextureId );
		}
		
		class ModelSet {
			
			public ModelPart Head, Head2, Head3, Torso, LeftLegFront, RightLegFront;
			
			public void Dispose() {
				RightLegFront.Dispose();
				LeftLegFront.Dispose();
				Torso.Dispose();
				Head.Dispose();
				Head2.Dispose();
				Head3.Dispose();
			}
		}
	}
}