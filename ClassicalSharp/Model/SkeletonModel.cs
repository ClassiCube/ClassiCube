using System;
using OpenTK;

namespace ClassicalSharp.Model {

	public class SkeletonModel : IModel {
		
		public SkeletonModel( Game window ) : base( window ) {
			vertices = new VertexPos3fTex2fCol4b[partVertices * 6];
			Head = MakeHead();
			Torso = MakeTorso();
			LeftLeg = MakeLeftLeg( 0.1875f, 0.0625f );
			RightLeg = MakeRightLeg( 0.0625f, 0.1875f );
			LeftArm = MakeLeftArm( 0.375f, 0.25f );
			RightArm = MakeRightArm( 0.25f, 0.375f );
			
			if( cache.SkeletonTexId <= 0 )
				cache.SkeletonTexId = graphics.CreateTexture( "skeleton.png" );
		}
		
		ModelPart MakeLeftArm( float x1, float x2 ) {
			return MakePart( 40, 16, 2, 12, 2, 2, 2, 12, -x2, -x1, 0.75f, 1.5f, -0.0625f, 0.0625f, false );
		}
		
		ModelPart MakeRightArm( float x1, float x2 ) {
			return MakePart( 40, 16, 2, 12, 2, 2, 2, 12, x1, x2, 0.75f, 1.5f, -0.0625f, 0.0625f, false );
		}
		
		ModelPart MakeHead() {
			return MakePart( 0, 0, 8, 8, 8, 8, 8, 8, -0.25f, 0.25f, 1.5f, 2f, -0.25f, 0.25f, false );
		}
		
		ModelPart MakeTorso() {
			return MakePart( 16, 16, 4, 12, 8, 4, 8, 12, -0.25f, 0.25f, 0.75f, 1.5f, -0.125f, 0.125f, false );
		}
		
		ModelPart MakeLeftLeg( float x1, float x2 ) {
			return MakePart( 0, 16, 2, 12, 2, 2, 2, 12, -x2, -x1, 0f, 0.75f, -0.0625f, 0.0625f, false );
		}
		
		ModelPart MakeRightLeg( float x1, float x2 ) {
			return MakePart( 0, 16, 2, 12, 2, 2, 2, 12, x1, x2, 0f, 0.75f, -0.0625f, 0.0625f, false );
		}
		
		public override float NameYOffset {
			get { return 2.075f; }
		}
		
		public override Vector3 CollisionSize {
			get { return new Vector3( 8 / 16f, 30 / 16f, 8 / 16f ); }
		}
		
		public override BoundingBox PickingBounds {
			get { return new BoundingBox( -4 / 16f, 0, -4 / 16f, 4 / 16f, 32 / 16f, 4 / 16f ); }
		}
		
		protected override void DrawPlayerModel( Player p ) {
			graphics.Texturing = true;
			graphics.AlphaTest = true;
			int texId = p.MobTextureId <= 0 ? cache.SkeletonTexId : p.MobTextureId;
			graphics.BindTexture( texId );
			
			DrawRotate( 0, 1.5f, 0, -p.PitchRadians, 0, 0, Head );
			DrawPart( Torso );
			DrawRotate( 0, 0.75f, 0, p.leftLegXRot, 0, 0, LeftLeg );
			DrawRotate( 0, 0.75f, 0, p.rightLegXRot, 0, 0, RightLeg );
			DrawRotate( 0, 1.375f, 0, (float)Math.PI / 2, 0, p.leftArmZRot, LeftArm );
			DrawRotate( 0, 1.375f, 0, (float)Math.PI / 2, 0, p.rightArmZRot, RightArm );
		}
		
		ModelPart Head, Torso, LeftLeg, RightLeg, LeftArm, RightArm;
	}
}