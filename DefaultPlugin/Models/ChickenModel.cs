using System;
using ClassicalSharp;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Model;
using ClassicalSharp.Renderers;

namespace DefaultPlugin {

	public class ChickenModel : IModel {
		
		public ChickenModel( Game window ) : base( window ) {
			vertices = new VertexPos3fTex2f[partVertices * 6 + planeVertices * 2 * 2];

			Head = MakeHead();
			Head2 = MakeHead2(); // TODO: Find a more appropriate name.
			Head3 = MakeHead3();
			Torso = MakeTorso();
			LeftLeg = MakeLeg( -0.1875f, 0f, -0.125f, -0.0625f );
			RightLeg = MakeLeg( 0f, 0.1875f, 0.0625f, 0.125f );
			LeftWing = MakeWing( -0.25f, -0.1875f );
			RightWing = MakeWing( 0.1875f, 0.25f );
			
			vb = graphics.InitVb( vertices, VertexPos3fTex2f.Size );
			vertices = null;
			DefaultTexId = TextureObj.LoadTexture( "chicken.png" );
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
			return MakeRotatedPart( 0, 9, 6, 8, 6, 6, 6, 8, -0.1875f, 0.1875f, 0.3125f, 0.6875f, -0.25f, 0.25f, false );
		}
		
		ModelPart MakeWing( float x1, float x2 ) {
			return MakePart( 24, 13, 6, 4, 1, 6, 1, 4, x1, x2, 0.4375f, 0.6875f, -0.1875f, 0.1875f, false );
		}
		
		ModelPart MakeLeg( float x1, float x2, float legX1, float legX2 ) {
			const float y1 = 0f, y2 = 0.3125f, z2 = 0.0625f, z1 = -0.125f;		
			YPlane( 32, 0, 3, 3, x2, x1, z1, z2, y1, false ); // bottom feet
			ZPlane( 36, 3, 1, 5, legX1, legX2, y1, y2, z2, false ); // vertical part of leg
			return new ModelPart( index - 12, 2 * 6, graphics, shader );
		}
		
		public override string ModelName {
			get { return "chicken"; }
		}
		
		public override float NameYOffset {
			get { return 1.0125f; }
		}
		
		protected override void DrawPlayerModel( Player player, PlayerRenderer renderer ) {
			TextureObj texId = renderer.MobTexId.IsInvalid ? DefaultTexId : renderer.MobTexId;
			texId.Bind();
			
			DrawRotate( 0, 0.5625f, -0.1875f, -pitch, 0, 0, Head );
			DrawRotate( 0, 0.5625f, -0.1875f, -pitch, 0, 0, Head2 );
			DrawRotate( 0, 0.5625f, -0.1875f, -pitch, 0, 0, Head3 );
			DrawPart( Torso );
			DrawRotate( 0, 0.3125f, 0.0625f, leftLegXRot, 0, 0, LeftLeg );
			DrawRotate( 0, 0.3125f, 0.0625f, rightLegXRot, 0, 0, RightLeg );
			DrawRotate( -0.1875f, 0.6875f, 0, 0, 0, -Math.Abs( leftArmXRot ), LeftWing );
			DrawRotate( 0.1875f, 0.6875f, 0, 0, 0, Math.Abs( rightArmXRot ), RightWing );
		}
		
		public override void Dispose() {
			graphics.DeleteVb( vb );
			DefaultTexId.Delete();
		}
	
		ModelPart Head, Head2, Head3, Torso, LeftLeg, RightLeg, LeftWing, RightWing;
	}
}