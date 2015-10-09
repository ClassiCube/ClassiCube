using System;
using OpenTK;

namespace ClassicalSharp.Model {

	public class SkeletonModel : IModel {
		
		public SkeletonModel( Game window ) : base( window ) {
			vertices = new ModelVertex[partVertices * 6];
			Head = MakeHead();
			Torso = MakeTorso();
			LeftLeg = MakeLeftLeg( 3/16f, 1/16f );
			RightLeg = MakeRightLeg( 1/16f, 3/16f );
			LeftArm = MakeLeftArm( 6/16f, 4/16f );
			RightArm = MakeRightArm( 4/16f, 6/16f );
		}
		
		ModelPart MakeLeftArm( float x1, float x2 ) {
			return MakePart( 40, 16, 2, 12, 2, 2, 2, 12, -x2, -x1, 12/16f, 24/16f, -1/16f, 1/16f, false );
		}
		
		ModelPart MakeRightArm( float x1, float x2 ) {
			return MakePart( 40, 16, 2, 12, 2, 2, 2, 12, x1, x2, 12/16f, 24/16f, -1/16f, 1/16f, false );
		}
		
		ModelPart MakeHead() {
			return MakePart( 0, 0, 8, 8, 8, 8, 8, 8, -4/16f, 4/16f, 24/16f, 2f, -4/16f, 4/16f, false );
		}
		
		ModelPart MakeTorso() {
			return MakePart( 16, 16, 4, 12, 8, 4, 8, 12, -4/16f, 4/16f, 12/16f, 24/16f, -2/16f, 2/16f, false );
		}
		
		ModelPart MakeLeftLeg( float x1, float x2 ) {
			return MakePart( 0, 16, 2, 12, 2, 2, 2, 12, -x2, -x1, 0f, 12/16f, -1/16f, 1/16f, false );
		}
		
		ModelPart MakeRightLeg( float x1, float x2 ) {
			return MakePart( 0, 16, 2, 12, 2, 2, 2, 12, x1, x2, 0f, 12/16f, -1/16f, 1/16f, false );
		}
		
		public override float NameYOffset {
			get { return 2.075f; }
		}
		
		public override float GetEyeY( Player player ) {
			return 26/16f;
		}
		
		public override Vector3 CollisionSize {
			get { return new Vector3( 8/16f, 30/16f, 8/16f ); }
		}
		
		public override BoundingBox PickingBounds {
			get { return new BoundingBox( -4/16f, 0, -4/16f, 4/16f, 32/16f, 4/16f ); }
		}
		
		protected override void DrawPlayerModel( Player p ) {
			graphics.Texturing = true;
			graphics.AlphaTest = true;
			int texId = p.MobTextureId <= 0 ? cache.SkeletonTexId : p.MobTextureId;
			graphics.BindTexture( texId );
			
			DrawRotate( 0, 24/16f, 0, -p.PitchRadians, 0, 0, Head );
			DrawPart( Torso );
			DrawRotate( 0, 12/16f, 0, p.leftLegXRot, 0, 0, LeftLeg );
			DrawRotate( 0, 12/16f, 0, p.rightLegXRot, 0, 0, RightLeg );
			DrawRotate( -5/16f, 23/16f, 0, 90 * Utils.Deg2Rad, 0, p.leftArmZRot, LeftArm );
			DrawRotate( 5/16f, 23/16f, 0, 90 * Utils.Deg2Rad, 0, p.rightArmZRot, RightArm );
		}
		
		ModelPart Head, Torso, LeftLeg, RightLeg, LeftArm, RightArm;
	}
}