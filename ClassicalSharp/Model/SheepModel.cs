using System;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Renderers;
using OpenTK;

namespace ClassicalSharp.Model {

	public class SheepModel : IModel {

		public bool Fur = true;
		
		public SheepModel( Game window ) : base( window ) {
			vertices = new ModelVertex[boxVertices * 6 * ( Fur ? 2 : 1 )];
			Head = BuildBox( MakeBoxBounds( -3, 16, -14, 3, 22, -6 )
			                .SetTexOrigin( 0, 0 ) );
			Torso = BuildRotatedBox( MakeRotatedBoxBounds( -4, 12, -8, 4, 18, 8 )
			                        .SetTexOrigin( 28, 8 ) );
			LeftLegFront = BuildBox( MakeBoxBounds( -5, 0, -7, -1, 12, -3 )
			                        .SetTexOrigin( 0, 16 ) );
			RightLegFront = BuildBox( MakeBoxBounds( 1, 0, -7, 5, 12, -3 )
			                         .SetTexOrigin( 0, 16 ) );
			LeftLegBack = BuildBox( MakeBoxBounds( -5, 0, 5, -1, 12, 9 )
			                       .SetTexOrigin( 0, 16 ) );
			RightLegBack = BuildBox( MakeBoxBounds( 1, 0, 5, 5, 12, 9 )
			                        .SetTexOrigin( 0, 16 ) );
			if( Fur )
				MakeFurModel();
		}
		
		
		void MakeFurModel() {
			FurHead = BuildBox( MakeBoxBounds( -3, -3, -3, 3, 3, 3 )
			                   .SetTexOrigin( 0, 0 )
			                   .SetModelBounds( -3.5f, 15.5f, -12.5f, 3.5f, 22.5f, -5.5f ) );
			FurTorso = BuildRotatedBox( MakeRotatedBoxBounds( -4, 12, -8, 4, 18, 8 )
			                           .SetTexOrigin( 28, 8 )
			                           .SetModelBounds( -6f, 10.5f, -10f, 6f, 19.5f, 10f ) );
			BoxDescription legDesc = MakeBoxBounds( -2, -3, -2, 2, 3, 2  )
				.SetTexOrigin( 0, 16 );
			
			FurLeftLegFront = BuildBox( legDesc.SetModelBounds( -5.5f, 5.5f, -7.5f, -0.5f, 12.5f, -2.5f ) );
			FurRightLegFront = BuildBox( legDesc.SetModelBounds( 0.5f, 5.5f, -7.5f, 5.5f, 12.5f, -2.5f ) );
			FurLeftLegBack = BuildBox( legDesc.SetModelBounds( -5.5f, 5.5f, 4.5f, -0.5f, 12.5f, 9.5f ) );
			FurRightLegBack = BuildBox( legDesc.SetModelBounds( 0.5f, 5.5f, 4.5f, 5.5f, 12.5f, 9.5f ) );
		}
		
		public override bool Bobbing {
			get { return true; }
		}
		
		public override float NameYOffset {
			get { return Fur ? 1.48125f: 1.075f; }
		}
		
		public override float GetEyeY( Player player ) {
			return 20/16f;
		}
		
		public override Vector3 CollisionSize {
			get { return new Vector3( 14/16f, 20/16f, 14/16f ); }
		}
		
		public override BoundingBox PickingBounds {
			get { return new BoundingBox( -6/16f, 0, -13/16f, 6/16f, 23/16f, 10/16f ); }
		}
		
		protected override void DrawPlayerModel( Player p ) {
			int texId = p.MobTextureId <= 0 ? cache.SheepTexId : p.MobTextureId;
			graphics.BindTexture( texId );
			DrawHeadRotate( 0, 18/16f, -8/16f, -p.PitchRadians, 0, 0, Head );
			
			DrawPart( Torso );
			DrawRotate( 0, 12/16f, -5/16f, p.anim.legXRot, 0, 0, LeftLegFront );
			DrawRotate( 0, 12/16f, -5/16f, -p.anim.legXRot, 0, 0, RightLegFront );
			DrawRotate( 0, 12/16f, 7/16f, -p.anim.legXRot, 0, 0, LeftLegBack );
			DrawRotate( 0, 12/16f, 7/16f, p.anim.legXRot, 0, 0, RightLegBack );
			graphics.UpdateDynamicIndexedVb( DrawMode.Triangles, cache.vb, cache.vertices, index, index * 6 / 4 );
			index = 0;
			
			if( Fur ) {
				graphics.BindTexture( cache.SheepFurTexId );				
				DrawHeadRotate( 0, 18/16f, -8/16f, -p.PitchRadians, 0, 0, FurHead );
				
				DrawPart( FurTorso );
				DrawRotate( 0, 12/16f, -5/16f, p.anim.legXRot, 0, 0, FurLeftLegFront );
				DrawRotate( 0, 12/16f, -5/16f, -p.anim.legXRot, 0, 0, FurRightLegFront );
				DrawRotate( 0, 12/16f, 7/16f, -p.anim.legXRot, 0, 0, FurLeftLegBack );
				DrawRotate( 0, 12/16f, 7/16f, p.anim.legXRot, 0, 0, FurRightLegBack );
				graphics.UpdateDynamicIndexedVb( DrawMode.Triangles, cache.vb, cache.vertices, index, index * 6 / 4 );
			}
		}
		
		ModelPart Head, Torso, LeftLegFront, RightLegFront, LeftLegBack, RightLegBack;
		ModelPart FurHead, FurTorso, FurLeftLegFront, FurRightLegFront, FurLeftLegBack, FurRightLegBack;
	}
}