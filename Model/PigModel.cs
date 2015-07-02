using OpenTK;
using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Renderers;

namespace ClassicalSharp.Model {

	public class PigModel : IModel {
		
		ModelSet Set;
		public PigModel( Game window ) : base( window ) {
			vertices = new VertexPos3fTex2fCol4b[partVertices * 6];
			Set = new ModelSet();
			Set.Head = MakeHead();
			Set.Torso = MakeTorso();
			Set.LeftLegFront = MakeLeg( -0.3125f, -0.0625f, -0.4375f, -0.1875f );
			Set.RightLegFront = MakeLeg( 0.0625f, 0.3125f, -0.4375f, -0.1875f );
			Set.LeftLegBack = MakeLeg( -0.3125f, -0.0625f, 0.3125f, 0.5625f );
			Set.RightLegBack = MakeLeg( 0.0625f, 0.3125f, 0.3125f, 0.5625f );
			
			vb = graphics.InitVb( vertices, VertexFormat.Pos3fTex2fCol4b );
			Set.SetVb( vb );
			vertices = null;
			DefaultTexId = graphics.LoadTexture( "pig.png" );
		}
		
		ModelPart MakeHead() {
			return MakePart( 0, 0, 8, 8, 8, 8, 8, 8, -0.25f, 0.25f, 0.5f, 1f, -0.875f, -0.375f, false );
		}
		
		ModelPart MakeTorso() {
			return MakeRotatedPart( 28, 8, 8, 16, 10, 8, 10, 16, -0.3125f, 0.3125f, 0.375f, 0.875f, -0.5f, 0.5f, false );
		}
		
		ModelPart MakeLeg( float x1, float x2, float z1, float z2 ) {
			return MakePart( 0, 16, 4, 6, 4, 4, 4, 6, x1, x2, 0f, 0.375f, z1, z2, false );
		}
		
		public override float NameYOffset {
			get { return 1.075f; }
		}
		
		public override Vector3 CollisionSize {
			get { return new Vector3( 14 / 16f, 14 / 16f, 14 / 16f ); }
		}
		
		public override BoundingBox PickingBounds {
			get { return new BoundingBox( -5 / 16f, 0, -14 / 16f, 5 / 16f, 16 / 16f, 9 / 16f ); }
		}
		
		protected override void DrawPlayerModel( Player player, PlayerRenderer renderer ) {
			graphics.Texturing = true;
			int texId = renderer.MobTextureId <= 0 ? DefaultTexId : renderer.MobTextureId;
			graphics.Bind2DTexture( texId );
			
			DrawRotate( 0, 0.75f, -0.375f, -pitch, 0, 0, Set.Head );
			Set.Torso.Render();
			DrawRotate( 0, 0.375f, -0.3125f, leftLegXRot, 0, 0, Set.LeftLegFront );
			DrawRotate( 0, 0.375f, -0.3125f, rightLegXRot, 0, 0, Set.RightLegFront );
			DrawRotate( 0, 0.375f, 0.4375f, rightLegXRot, 0, 0, Set.LeftLegBack );
			DrawRotate( 0, 0.375f, 0.4375f, leftLegXRot, 0, 0, Set.RightLegBack );
			graphics.AlphaTest = true;
		}
		
		public override void Dispose() {
			graphics.DeleteVb( vb );
			graphics.DeleteTexture( ref DefaultTexId );
		}
		
		class ModelSet {
			
			public ModelPart Head, Torso, LeftLegFront, RightLegFront, LeftLegBack, RightLegBack;
			
			public void SetVb( int vb ) {
				Head.Vb = Torso.Vb = LeftLegFront.Vb = RightLegFront.Vb
					= LeftLegBack.Vb = RightLegBack.Vb = vb;
			}
		}
	}
}