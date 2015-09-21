using System;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Model {

	public class PigModel : IModel {
		
		public PigModel( Game window ) : base( window ) {
			vertices = new VertexPos3fTex2fCol4b[partVertices * 6];
			Head = MakeHead();
			Torso = MakeTorso();
			LeftLegFront = MakeLeg( -5/16f, -1/16f, -7/16f, -3/16f );
			RightLegFront = MakeLeg( 1/16f, 5/16f, -7/16f, -3/16f );
			LeftLegBack = MakeLeg( -5/16f, -1/16f, 5/16f, 9/16f );
			RightLegBack = MakeLeg( 1/16f, 5/16f, 5/16f, 9/16f );
		}
		
		ModelPart MakeHead() {
			return MakePart( 0, 0, 8, 8, 8, 8, 8, 8, -4/16f, 4/16f, 8/16f, 16/16f, -14/16f, -6/16f, false );
		}
		
		ModelPart MakeTorso() {
			return MakeRotatedPart( 28, 8, 8, 16, 10, 8, 10, 16, -5/16f, 5/16f, 6/16f, 14/16f, -8/16f, 8/16f, false );
		}
		
		ModelPart MakeLeg( float x1, float x2, float z1, float z2 ) {
			return MakePart( 0, 16, 4, 6, 4, 4, 4, 6, x1, x2, 0f, 6/16f, z1, z2, false );
		}
		
		public override float NameYOffset {
			get { return 1.075f; }
		}
		
		public override float GetEyeY( Player player ) {
			return 12/16f;
		}
		
		public override Vector3 CollisionSize {
			get { return new Vector3( 14/16f, 14/16f, 14/16f ); }
		}
		
		public override BoundingBox PickingBounds {
			get { return new BoundingBox( -5/16f, 0, -14/16f, 5/16f, 16/16f, 9/16f ); }
		}
		
		protected override void DrawPlayerModel( Player p ) {
			graphics.Texturing = true;
			int texId = p.MobTextureId <= 0 ? cache.PigTexId : p.MobTextureId;
			graphics.BindTexture( texId );
			
			DrawRotate( 0, 12/16f, -6/16f, -p.PitchRadians, 0, 0, Head );
			DrawPart( Torso );
			DrawRotate( 0, 6/16f, -5/16f, p.leftLegXRot, 0, 0, LeftLegFront );
			DrawRotate( 0, 6/16f, -5/16f, p.rightLegXRot, 0, 0, RightLegFront );
			DrawRotate( 0, 6/16f, 7/16f, p.rightLegXRot, 0, 0, LeftLegBack );
			DrawRotate( 0, 6/16f, 7/16f, p.leftLegXRot, 0, 0, RightLegBack );
			graphics.AlphaTest = true;
		}
		
		ModelPart Head, Torso, LeftLegFront, RightLegFront, LeftLegBack, RightLegBack;
	}
}