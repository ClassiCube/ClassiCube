using System;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Renderers;
using OpenTK;

namespace ClassicalSharp.Model {

	public class ChickenModel : IModel {
		
		public ChickenModel( Game window ) : base( window ) {
			vertices = new VertexPos3fTex2fCol4b[partVertices * 6 + planeVertices * 2 * 2];
			Head = MakeHead();
			Head2 = MakeHead2(); // TODO: Find a more appropriate name.
			Head3 = MakeHead3();
			Torso = MakeTorso();
			LeftLeg = MakeLeg( -0.1875f, 0f, -0.125f, -0.0625f );
			RightLeg = MakeLeg( 0f, 0.1875f, 0.0625f, 0.125f );
			LeftWing = MakeWing( -0.25f, -0.1875f );
			RightWing = MakeWing( 0.1875f, 0.25f );
			
			vb = graphics.InitVb( vertices, VertexFormat.Pos3fTex2fCol4b );
			vertices = null;
			DefaultTexId = graphics.LoadTexture( "chicken.png" );
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
			return new ModelPart( index - 12, 2 * 6, graphics );
		}
		
		public override float NameYOffset {
			get { return 1.0125f; }
		}
		
		public override Vector3 CollisionSize {
			get { return new Vector3( 8 / 16f, 12 / 16f, 8 / 16f ); }
		}
		
		public override BoundingBox PickingBounds {
			get { return new BoundingBox( -4 / 16f, 0, -8 / 16f, 4 / 16f, 15 / 16f, 4 / 16f ); }
		}
		
		protected override void DrawPlayerModel( Player p ) {
			graphics.Texturing = true;
			int texId = p.MobTextureId <= 0 ? DefaultTexId : p.MobTextureId;
			graphics.Bind2DTexture( texId );
			graphics.AlphaTest = true;
			
			DrawRotate( 0, 0.5625f, -0.1875f, -p.PitchRadians, 0, 0, Head );
			DrawRotate( 0, 0.5625f, -0.1875f, -p.PitchRadians, 0, 0, Head2 );
			DrawRotate( 0, 0.5625f, -0.1875f, -p.PitchRadians, 0, 0, Head3 );
			Torso.Render( vb );
			DrawRotate( 0, 0.3125f, 0.0625f, p.leftLegXRot, 0, 0, LeftLeg );
			DrawRotate( 0, 0.3125f, 0.0625f, p.rightLegXRot, 0, 0, RightLeg );
			DrawRotate( -0.1875f, 0.6875f, 0, 0, 0, -Math.Abs( p.leftArmXRot ), LeftWing );
			DrawRotate( 0.1875f, 0.6875f, 0, 0, 0, Math.Abs( p.rightArmXRot ), RightWing );
		}
		
		public override void Dispose() {
			graphics.DeleteVb( vb );
			graphics.DeleteTexture( ref DefaultTexId );
		}
		
		ModelPart Head, Head2, Head3, Torso, LeftLeg, RightLeg, LeftWing, RightWing;
	}
}