using OpenTK;
using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Renderers;

namespace ClassicalSharp.Model {

	public class SheepModel : IModel {
		
		ModelSet Set;
		public bool Fur = true;
		int furTextureId;
		
		public SheepModel( Game window ) : base( window ) {
			vertices = new VertexPos3fTex2fCol4b[partVertices * 6 * ( Fur ? 2 : 1 )];
			Set = new ModelSet( Fur );
			Set.Head = MakeHead();
			Set.Torso = MakeTorso();
			Set.LeftLegFront = MakeLeg( -0.3125f, -0.0625f, -0.4375f, -0.1875f );
			Set.RightLegFront = MakeLeg( 0.0625f, 0.3125f, -0.4375f, -0.1875f );
			Set.LeftLegBack = MakeLeg( -0.3125f, -0.0625f, 0.3125f, 0.5625f );
			Set.RightLegBack = MakeLeg( 0.0625f, 0.3125f, 0.3125f, 0.5625f );
			if( Fur ) {
				Set.FurHead = MakeFurHead();
				Set.FurTorso = MakeFurTorso();
				Set.FurLeftLegFront = MakeFurLeg( -0.34375f, -0.03125f, -0.46875f, -0.15625f );
				Set.FurRightLegFront = MakeFurLeg( 0.03125f, 0.34375f, -0.46875f, -0.15625f );
				Set.FurLeftLegBack = MakeFurLeg( -0.34375f, -0.03125f, 0.28125f, 0.59375f );
				Set.FurRightLegBack = MakeFurLeg( 0.03125f, 0.34375f, 0.28125f, 0.59375f );
			}
			
			vb = graphics.InitVb( vertices, VertexFormat.Pos3fTex2fCol4b );
			Set.SetVb( vb );
			vertices = null;
			DefaultTexId = graphics.LoadTexture( "sheep.png" );
			furTextureId = graphics.LoadTexture( "sheep_fur.png" );
		}
		
		ModelPart MakeHead() {
			return MakePart( 0, 0, 8, 6, 6, 8, 6, 6, -0.1875f, 0.1875f, 1f, 1.375f, -0.875f, -0.375f, false );
		}
		
		ModelPart MakeTorso() {
			return MakeRotatedPart( 28, 8, 6, 16, 8, 6, 8, 16, -0.25f, 0.25f, 0.75f, 1.125f, -0.5f, 0.5f, false );
		}
		
		ModelPart MakeFurHead() {
			return MakePart( 0, 0, 6, 6, 6, 6, 6, 6, -0.21875f, 0.21875f, 0.96875f, 1.40625f, -0.78125f, -0.34375f, false );
		}
		
		ModelPart MakeFurTorso() {
			return MakeRotatedPart( 28, 8, 6, 16, 8, 6, 8, 16, -0.375f, 0.375f, 0.65625f, 1.21875f, -0.625f, 0.625f, false );
		}
		
		ModelPart MakeLeg( float x1, float x2, float z1, float z2 ) {
			return MakePart( 0, 16, 4, 12, 4, 4, 4, 12, x1, x2, 0f, 0.75f, z1, z2, false );
		}
		
		ModelPart MakeFurLeg( float x1, float x2, float z1, float z2 ) {
			return MakePart( 0, 16, 4, 6, 4, 4, 4, 6, x1, x2, 0.34375f, 0.78125f, z1, z2, false );
		}
		
		public override float NameYOffset {
			get { return Fur ? 1.48125f: 1.075f; }
		}
		
		public override Vector3 CollisionSize {
			get { return new Vector3( 14 / 16f, 20 / 16f, 14 / 16f ); }
		}
		
		public override BoundingBox PickingBounds {
			get { return new BoundingBox( -6 / 16f, 0, -13 / 16f, 6 / 16f, 23 / 16f, 10 / 16f ); }
		}
		
		protected override void DrawPlayerModel( Player player, PlayerRenderer renderer ) {
			graphics.Texturing = true;
			int texId = renderer.MobTextureId <= 0 ? DefaultTexId : renderer.MobTextureId;
			graphics.Bind2DTexture( texId );
			
			DrawRotate( 0, 1.125f, -0.5f, -pitch, 0, 0, Set.Head );
			Set.Torso.Render();
			DrawRotate( 0, 0.75f, -0.3125f, leftLegXRot, 0, 0, Set.LeftLegFront );
			DrawRotate( 0, 0.75f, -0.3125f, rightLegXRot, 0, 0, Set.RightLegFront );
			DrawRotate( 0, 0.75f, 0.4375f, rightLegXRot, 0, 0, Set.LeftLegBack );
			DrawRotate( 0, 0.75f, 0.4375f, leftLegXRot, 0, 0, Set.RightLegBack );
			graphics.AlphaTest = true;
			if( Fur ) {
				graphics.Bind2DTexture( furTextureId );
				Set.FurTorso.Render();
				DrawRotate( 0, 1.125f, -0.5f, -pitch, 0, 0, Set.FurHead );
				DrawRotate( 0, 0.75f, -0.3125f, leftLegXRot, 0, 0, Set.FurLeftLegFront );
				DrawRotate( 0, 0.75f, -0.3125f, rightLegXRot, 0, 0, Set.FurRightLegFront );
				DrawRotate( 0, 0.75f, 0.4375f, rightLegXRot, 0, 0, Set.FurLeftLegBack );
				DrawRotate( 0, 0.75f, 0.4375f, leftLegXRot, 0, 0, Set.FurRightLegBack );
			}
		}
		
		public override void Dispose() {
			graphics.DeleteVb( vb );
			graphics.DeleteTexture( ref DefaultTexId );
			if( Fur ) {
				graphics.DeleteTexture( ref furTextureId );
			}
		}
		
		class ModelSet {
			
			bool fur;
			public ModelPart Head, Torso, LeftLegFront, RightLegFront, LeftLegBack, RightLegBack;
			public ModelPart FurHead, FurTorso, FurLeftLegFront, FurRightLegFront,
			FurLeftLegBack, FurRightLegBack;
			public ModelSet( bool fur ) {
				this.fur = fur;
			}
			
			public void SetVb( int vb ) {
				Head.Vb = Torso.Vb = LeftLegFront.Vb = RightLegFront.Vb
					= LeftLegBack.Vb = RightLegBack.Vb = vb;
				if( fur ) {
					FurHead.Vb = FurTorso.Vb = FurLeftLegFront.Vb = FurRightLegFront.Vb
					= FurLeftLegBack.Vb = FurRightLegBack.Vb = vb;
				}
			}
		}
	}
}