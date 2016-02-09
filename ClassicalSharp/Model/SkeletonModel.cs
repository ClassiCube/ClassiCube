using System;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Model {

	public class SkeletonModel : IModel {
		
		public SkeletonModel( Game window ) : base( window ) {
			vertices = new ModelVertex[boxVertices * 6];
			Head = BuildBox( MakeBoxBounds( -4, 24, -4, 4, 32, 4 )
			                .SetTexOrigin( 0, 0 ) );
			Torso = BuildBox( MakeBoxBounds( -4, 12, -2, 4, 24, 2 )
			                 .SetTexOrigin( 16, 16 ) );
			LeftLeg = BuildBox( MakeBoxBounds( -1, 0, -1, -3, 12, 1 )
			                   .SetTexOrigin( 0, 16 ) );
			RightLeg = BuildBox( MakeBoxBounds( 1, 0, -1, 3, 12, 1 )
				                    .SetTexOrigin( 0, 16 ) );
			LeftArm = BuildBox( MakeBoxBounds( -4, 12, -1, -6, 24, 1 )
			                   .SetTexOrigin( 40, 16 ) );
			RightArm = BuildBox( MakeBoxBounds( 4, 12, -1, 6, 24, 1 )
			                   .SetTexOrigin( 40, 16 ) );
		}
		
		public override bool Bobbing {
			get { return true; }
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
			int texId = p.MobTextureId <= 0 ? cache.SkeletonTexId : p.MobTextureId;
			graphics.BindTexture( texId );
			DrawHeadRotate( 0, 24/16f, 0, -p.PitchRadians, 0, 0, Head );

			DrawPart( Torso );
			DrawRotate( 0, 12/16f, 0, p.legXRot, 0, 0, LeftLeg );
			DrawRotate( 0, 12/16f, 0, -p.legXRot, 0, 0, RightLeg );
			DrawRotate( -5/16f, 23/16f, 0, 90 * Utils.Deg2Rad, 0, p.armZRot, LeftArm );
			DrawRotate( 5/16f, 23/16f, 0, 90 * Utils.Deg2Rad, 0, -p.armZRot, RightArm );
			graphics.UpdateDynamicIndexedVb( DrawMode.Triangles, cache.vb, cache.vertices, index, index * 6 / 4 );
		}
		
		ModelPart Head, Torso, LeftLeg, RightLeg, LeftArm, RightArm;
	}
}