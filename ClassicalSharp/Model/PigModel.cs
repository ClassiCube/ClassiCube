using System;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Model {

	public class PigModel : IModel {
		
		public PigModel( Game window ) : base( window ) {
			vertices = new ModelVertex[boxVertices * 6];
			Head = BuildBox( MakeBoxBounds( -4, 8, -14, 4, 16, -6 )
			               .SetTexOrigin( 0, 0 ) );
			Torso = BuildRotatedBox( MakeRotatedBoxBounds( -5, 6, -8, 5, 14, 8 )
			                        .SetTexOrigin( 28, 8 ) );
			LeftLegFront = BuildBox( MakeBoxBounds( -5, 0, -7, -1, 6, -3 )
			                        .SetTexOrigin( 0, 16 ) );
			RightLegFront = BuildBox( MakeBoxBounds( 1, 0, -7, 5, 6, -3 )
			                         .SetTexOrigin( 0, 16 ) );
			LeftLegBack = BuildBox( MakeBoxBounds( -5, 0, 5, -1, 6, 9 )
			                       .SetTexOrigin( 0, 16 ) );
			RightLegBack = BuildBox( MakeBoxBounds( 1, 0, 5, 5, 6, 9 )
			                        .SetTexOrigin( 0, 16 ) );
		}
		
		public override bool Bobbing { get { return true; } }
		
		public override float NameYOffset { get { return 1.075f; } }
		
		public override float GetEyeY( Entity entity ) { return 12/16f; }
		
		public override Vector3 CollisionSize {
			get { return new Vector3( 14/16f, 14/16f, 14/16f ); }
		}
		
		public override BoundingBox PickingBounds {
			get { return new BoundingBox( -5/16f, 0, -14/16f, 5/16f, 16/16f, 9/16f ); }
		}
		
		protected override void DrawPlayerModel( Player p ) {
			int texId = p.MobTextureId <= 0 ? cache.PigTexId : p.MobTextureId;
			graphics.BindTexture( texId );
			DrawHeadRotate( 0, 12/16f, -6/16f, -p.PitchRadians, 0, 0, Head );

			DrawPart( Torso );
			DrawRotate( 0, 6/16f, -5/16f, p.anim.legXRot, 0, 0, LeftLegFront );
			DrawRotate( 0, 6/16f, -5/16f, -p.anim.legXRot, 0, 0, RightLegFront );
			DrawRotate( 0, 6/16f, 7/16f, -p.anim.legXRot, 0, 0, LeftLegBack );
			DrawRotate( 0, 6/16f, 7/16f, p.anim.legXRot, 0, 0, RightLegBack );
			graphics.UpdateDynamicIndexedVb( DrawMode.Triangles, cache.vb, cache.vertices, index, index * 6 / 4 );
		}
		
		ModelPart Head, Torso, LeftLegFront, RightLegFront, LeftLegBack, RightLegBack;
	}
}