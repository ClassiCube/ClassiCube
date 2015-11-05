using System;
using OpenTK;

namespace ClassicalSharp.Model {

	public class ZombieModel : IModel {
		
		public ZombieModel( Game window ) : base( window ) {
			vertices = new ModelVertex[boxVertices * 6];
			Head = BuildBox( MakeBoxBounds( -4, 24, -4, 4, 32, 4 )
			                .SetTexOrigin( 0, 0 ) );
			Torso =  BuildBox( MakeBoxBounds( -4, 12, -2, 4, 24, 2 )
			                  .SetTexOrigin( 16, 16 ) );
			LeftLeg =  BuildBox( MakeBoxBounds( 0, 0, -2, -4, 12, 2 )
			                    .SetTexOrigin( 0, 16 ) );
			RightLeg = BuildBox( MakeBoxBounds( 0, 0, -2, 4, 12, 2 )
			                    .SetTexOrigin( 0, 16 ) );
			LeftArm = BuildBox( MakeBoxBounds( -4, 12, -2, -8, 24, 2 )
			                   .SetTexOrigin( 40, 16 ) );
			RightArm = BuildBox( MakeBoxBounds( 4, 12, -2, 8, 24, 2 )
			                    .SetTexOrigin( 40, 16 ) );
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