using System;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Model {

	public class CreeperModel : IModel {
		
		public CreeperModel( Game window ) : base( window ) {
			vertices = new ModelVertex[boxVertices * 6];
			Head =  BuildBox( MakeBoxBounds( -4, 18, -4, 4, 26, 4 )
			                 .SetTexOrigin( 0, 0 ) );
			Torso = BuildBox( MakeBoxBounds( -4, 6, -2, 4, 18, 2 )
			                 .SetTexOrigin( 16, 16 ) );
			
			LeftLegFront = BuildBox( MakeBoxBounds( -4, 0, -6, 0, 6, -2 )
			         .SetTexOrigin( 0, 16 ) );
			RightLegFront = BuildBox( MakeBoxBounds( 0, 0, -6, 4, 6, -2 )
			         .SetTexOrigin( 0, 16 ) );
			LeftLegBack = BuildBox( MakeBoxBounds( -4, 0, 2, 0, 6, 6 )
			         .SetTexOrigin( 0, 16 ) );
			RightLegBack = BuildBox( MakeBoxBounds( 0, 0, 2, 4, 6, 6 )
			         .SetTexOrigin( 0, 16 ) );
		}
		
		public override bool Bobbing {
			get { return true; }
		}
		
		public override float NameYOffset {
			get { return 1.7f; }
		}
		
		public override float GetEyeY( Player player ) {
			return 22/16f;
		}
		
		public override Vector3 CollisionSize {
			get { return new Vector3( 8/16f, 26/16f, 8/16f ); }
		}
		
		public override BoundingBox PickingBounds {
			get { return new BoundingBox( -4/16f, 0, -6/16f, 4/16f, 26/16f, 6/16f ); }
		}
		
		protected override void DrawPlayerModel( Player p ) {
			int texId = p.MobTextureId <= 0 ? cache.CreeperTexId : p.MobTextureId;
			graphics.BindTexture( texId );
			DrawHeadRotate( 0, 18/16f, 0, -p.PitchRadians, 0, 0, Head );

			DrawPart( Torso );
			DrawRotate( 0, 6/16f, -2/16f, p.legXRot, 0, 0, LeftLegFront );
			DrawRotate( 0, 6/16f, -2/16f, -p.legXRot, 0, 0, RightLegFront );
			DrawRotate( 0, 6/16f, 2/16f, -p.legXRot, 0, 0, LeftLegBack );
			DrawRotate( 0, 6/16f, 2/16f, p.legXRot, 0, 0, RightLegBack );
			graphics.UpdateDynamicIndexedVb( DrawMode.Triangles, cache.vb, cache.vertices, index, index * 6 / 4 );
		}
		
		ModelPart Head, Torso, LeftLegFront, RightLegFront, LeftLegBack, RightLegBack;
	}
}