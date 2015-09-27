using System;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Renderers;
using OpenTK;

namespace ClassicalSharp.Model {

	public class SheepModel : IModel {

		public bool Fur = true;
		
		public SheepModel( Game window ) : base( window ) {
			vertices = new ModelVertex[partVertices * 6 * ( Fur ? 2 : 1 )];
			Head = MakeHead();
			Torso = MakeTorso();
			LeftLegFront = MakeLeg( -5/16f, -1/16f, -7/16f, -3/16f );
			RightLegFront = MakeLeg( 1/16f, 5/16f, -7/16f, -3/16f );
			LeftLegBack = MakeLeg( -5/16f, -1/16f, 5/16f, 9/16f );
			RightLegBack = MakeLeg( 1/16f, 5/16f, 5/16f, 9/16f );
			if( Fur ) {
				FurHead = MakeFurHead();
				FurTorso = MakeFurTorso();
				FurLeftLegFront = MakeFurLeg( -5.5f/16f, -0.5f/16f, -7.5f/16f, -2.5f/16f );
				FurRightLegFront = MakeFurLeg( 0.5f/16f, 5.5f/16f, -7.5f/16f, -2.5f/16f );
				FurLeftLegBack = MakeFurLeg( -5.5f/16f, -0.5f/16f, 4.5f/16f, 9.5f/16f );
				FurRightLegBack = MakeFurLeg( 0.5f/16f, 5.5f/16f, 4.5f/16f, 9.5f/16f );
			}
		}
		
		ModelPart MakeHead() {
			return MakePart( 0, 0, 8, 6, 6, 8, 6, 6, -3/16f, 3/16f, 16/16f, 22/16f, -14/16f, -6/16f, false );
		}
		
		ModelPart MakeTorso() {
			return MakeRotatedPart( 28, 8, 6, 16, 8, 6, 8, 16, -4/16f, 4/16f, 12/16f, 18/16f, -8/16f, 8/16f, false );
		}
		
		ModelPart MakeFurHead() {
			return MakePart( 0, 0, 6, 6, 6, 6, 6, 6, -3.5f/16f, 3.5f/16f, 15.5f/16f, 1.40625f, -12.5f/16f, -5.5f/16f, false );
		}
		
		ModelPart MakeFurTorso() {
			return MakeRotatedPart( 28, 8, 6, 16, 8, 6, 8, 16, -6/16f, 6/16f, 10.5f/16f, 1.21875f, -10/16f, 10/16f, false );
		}
		
		ModelPart MakeLeg( float x1, float x2, float z1, float z2 ) {
			return MakePart( 0, 16, 4, 12, 4, 4, 4, 12, x1, x2, 0f, 12/16f, z1, z2, false );
		}
		
		ModelPart MakeFurLeg( float x1, float x2, float z1, float z2 ) {
			return MakePart( 0, 16, 4, 6, 4, 4, 4, 6, x1, x2, 5.5f/16f, 12.5f/16f, z1, z2, false );
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
			graphics.Texturing = true;
			int texId = p.MobTextureId <= 0 ? cache.SheepTexId : p.MobTextureId;
			graphics.BindTexture( texId );
			
			DrawRotate( 0, 18/16f, -8/16f, -p.PitchRadians, 0, 0, Head );
			DrawPart( Torso );
			DrawRotate( 0, 12/16f, -5/16f, p.leftLegXRot, 0, 0, LeftLegFront );
			DrawRotate( 0, 12/16f, -5/16f, p.rightLegXRot, 0, 0, RightLegFront );
			DrawRotate( 0, 12/16f, 7/16f, p.rightLegXRot, 0, 0, LeftLegBack );
			DrawRotate( 0, 12/16f, 7/16f, p.leftLegXRot, 0, 0, RightLegBack );
			// Need to draw the two parts separately.
			graphics.DrawDynamicIndexedVb( DrawMode.Triangles, cache.vb, cache.vertices, index, index * 6 / 4 );
			graphics.AlphaTest = true;
			index = 0;
			
			if( Fur ) {
				graphics.BindTexture( cache.SheepFurTexId );
				DrawPart( FurTorso );
				DrawRotate( 0, 18/16f, -8/16f, -p.PitchRadians, 0, 0, FurHead );
				DrawRotate( 0, 12/16f, -5/16f, p.leftLegXRot, 0, 0, FurLeftLegFront );
				DrawRotate( 0, 12/16f, -5/16f, p.rightLegXRot, 0, 0, FurRightLegFront );
				DrawRotate( 0, 12/16f, 7/16f, p.rightLegXRot, 0, 0, FurLeftLegBack );
				DrawRotate( 0, 12/16f, 7/16f, p.leftLegXRot, 0, 0, FurRightLegBack );
			}
		}
		
		ModelPart Head, Torso, LeftLegFront, RightLegFront, LeftLegBack, RightLegBack;
		ModelPart FurHead, FurTorso, FurLeftLegFront, FurRightLegFront, FurLeftLegBack, FurRightLegBack;
	}
}