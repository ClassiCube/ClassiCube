using System;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Model {

	public class PigModel : IModel {
		
		public PigModel( Game window ) : base( window ) {
			vertices = new VertexPos3fTex2fCol4b[partVertices * 6];
			Head = MakeHead();
			Torso = MakeTorso();
			LeftLegFront = MakeLeg( -0.3125f, -0.0625f, -0.4375f, -0.1875f );
			RightLegFront = MakeLeg( 0.0625f, 0.3125f, -0.4375f, -0.1875f );
			LeftLegBack = MakeLeg( -0.3125f, -0.0625f, 0.3125f, 0.5625f );
			RightLegBack = MakeLeg( 0.0625f, 0.3125f, 0.3125f, 0.5625f );
			
			vb = graphics.CreateVb( vertices, VertexFormat.Pos3fTex2fCol4b );
			vertices = null;
			DefaultTexId = graphics.CreateTexture( "pig.png" );
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
		
		protected override void DrawPlayerModel( Player p ) {
			graphics.Texturing = true;
			int texId = p.MobTextureId <= 0 ? DefaultTexId : p.MobTextureId;
			graphics.BindTexture( texId );
			
			DrawRotate( 0, 0.75f, -0.375f, -p.PitchRadians, 0, 0, Head );
			Torso.Render( graphics );
			DrawRotate( 0, 0.375f, -0.3125f, p.leftLegXRot, 0, 0, LeftLegFront );
			DrawRotate( 0, 0.375f, -0.3125f, p.rightLegXRot, 0, 0, RightLegFront );
			DrawRotate( 0, 0.375f, 0.4375f, p.rightLegXRot, 0, 0, LeftLegBack );
			DrawRotate( 0, 0.375f, 0.4375f, p.leftLegXRot, 0, 0, RightLegBack );
			graphics.AlphaTest = true;
		}
		
		public override void Dispose() {
			graphics.DeleteVb( vb );
			graphics.DeleteTexture( ref DefaultTexId );
		}
		
		ModelPart Head, Torso, LeftLegFront, RightLegFront, LeftLegBack, RightLegBack;
	}
}