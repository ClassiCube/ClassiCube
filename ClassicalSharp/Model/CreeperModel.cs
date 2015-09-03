using System;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Model {

	public class CreeperModel : IModel {
		
		public CreeperModel( Game window ) : base( window ) {
			vertices = new VertexPos3fTex2fCol4b[partVertices * 6];
			Head = MakeHead();
			Torso = MakeTorso();
			LeftLegFront = MakeLeg( -0.25f, 0, -0.375f, -0.125f );
			RightLegFront = MakeLeg( 0, 0.25f, -0.375f, -0.125f );
			LeftLegBack = MakeLeg( -0.25f, 0, 0.125f, 0.375f );
			RightLegBack = MakeLeg( 0, 0.25f, 0.125f, 0.375f );
			
			DefaultTexId = graphics.CreateTexture( "creeper.png" );
		}
		
		ModelPart MakeHead() {
			return MakePart( 0, 0, 8, 8, 8, 8, 8, 8, -0.25f, 0.25f, 1.125f, 1.625f, -0.25f, 0.25f, false );
		}
		
		ModelPart MakeTorso() {
			return MakePart( 16, 16, 4, 12, 8, 4, 8, 12, -0.25f, 0.25f, 0.375f, 1.125f, -0.125f, 0.125f, false );
		}
		
		ModelPart MakeLeg( float x1, float x2, float z1, float z2 ) {
			return MakePart( 0, 16, 4, 6, 4, 4, 4, 6, x1, x2, 0f, 0.375f, z1, z2, false );
		}
		
		public override float NameYOffset {
			get { return 1.7f; }
		}
		
		public override Vector3 CollisionSize {
			get { return new Vector3( 8 / 16f, 26 / 16f, 8 / 16f ); }
		}
		
		public override BoundingBox PickingBounds {
			get { return new BoundingBox( -4 / 16f, 0, -6 / 16f, 4 / 16f, 26 / 16f, 6 / 16f ); }
		}
		
		protected override void DrawPlayerModel( Player p ) {
			graphics.Texturing = true;
			int texId = p.MobTextureId <= 0 ? DefaultTexId : p.MobTextureId;
			graphics.BindTexture( texId );
			
			DrawRotate( 0, 1.125f, 0, -p.PitchRadians, 0, 0, Head );
			DrawPart( Torso );
			DrawRotate( 0, 0.375f, -0.125f, p.leftLegXRot, 0, 0, LeftLegFront );
			DrawRotate( 0, 0.375f, -0.125f, p.rightLegXRot, 0, 0, RightLegFront );
			DrawRotate( 0, 0.375f, 0.125f, p.rightLegXRot, 0, 0, LeftLegBack );
			DrawRotate( 0, 0.375f, 0.125f, p.leftLegXRot, 0, 0, RightLegBack );
			graphics.AlphaTest = true;
		}
		
		public override void Dispose() {
			graphics.DeleteTexture( ref DefaultTexId );
		}
		
		ModelPart Head, Torso, LeftLegFront, RightLegFront, LeftLegBack, RightLegBack;
	}
}