using System;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Model {

	public class ZombieModel : IModel {
		
		public ZombieModel( Game window ) : base( window ) {
			vertices = new ModelVertex[boxVertices * 6];
			Head = MakeHead();
			Torso = MakeTorso();
			LeftLeg = MakeLeftLeg( 4/16f, 0f );
			RightLeg = MakeRightLeg( 0, 4/16f );
			LeftArm = MakeLeftArm( 8/16f, 4/16f );
			RightArm = MakeRightArm( 4/16f, 8/16f );
		}
		
		ModelPart MakeLeftArm( float x1, float x2 ) {
			return MakeBox( 40, 16, 4, 12, 4, 4, 4, 12, -x2, -x1, 12/16f, 24/16f, -2/16f, 2/16f, false );
		}
		
		ModelPart MakeRightArm( float x1, float x2 ) {
			return MakeBox( 40, 16, 4, 12, 4, 4, 4, 12, x1, x2, 12/16f, 24/16f, -2/16f, 2/16f, false );
		}
		
		ModelPart MakeHead() {
			return MakeBox( 0, 0, 8, 8, 8, 8, 8, 8, -4/16f, 4/16f, 24/16f, 2f, -4/16f, 4/16f, false );
		}
		
		ModelPart MakeTorso() {
			return MakeBox( 16, 16, 4, 12, 8, 4, 8, 12, -4/16f, 4/16f, 12/16f, 24/16f, -2/16f, 2/16f, false );
		}
		
		ModelPart MakeLeftLeg( float x1, float x2 ) {
			return MakeBox( 0, 16, 4, 12, 4, 4, 4, 12, -x2, -x1, 0f, 12/16f, -2/16f, 2/16f, false );
		}
		
		ModelPart MakeRightLeg( float x1, float x2 ) {
			return MakeBox( 0, 16, 4, 12, 4, 4, 4, 12, x1, x2, 0f, 12/16f, -2/16f, 2/16f, false );
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
			int texId = p.MobTextureId <= 0 ? cache.ZombieTexId : p.MobTextureId;
			graphics.BindTexture( texId );
			
			DrawRotate( 0, 24/16f, 0, -p.PitchRadians, 0, 0, Head );
			DrawPart( Torso );
			DrawRotate( 0, 12/16f, 0, p.leftLegXRot, 0, 0, LeftLeg );
			DrawRotate( 0, 12/16f, 0, p.rightLegXRot, 0, 0, RightLeg );
			DrawRotate( -6/16f, 22/16f, 0, (float)Math.PI / 2, 0, p.leftArmZRot, LeftArm );
			DrawRotate( 6/16f, 22/16f, 0, (float)Math.PI / 2, 0, p.rightArmZRot, RightArm );
			graphics.AlphaTest = true;
		}
		
		ModelPart Head, Torso, LeftLeg, RightLeg, LeftArm, RightArm;
	}
}