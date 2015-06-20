using System;
using ClassicalSharp;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Model;
using ClassicalSharp.Renderers;

namespace DefaultPlugin {

	public class CreeperModel : IModel {
		
		public CreeperModel( Game window ) : base( window ) {
			vertices = new VertexPos3fTex2f[partVertices * 6];
			Head = MakeHead();
			Torso = MakeTorso();
			LeftLegFront = MakeLeg( -0.25f, 0, -0.375f, -0.125f );
			RightLegFront = MakeLeg( 0, 0.25f, -0.375f, -0.125f );
			LeftLegBack = MakeLeg( -0.25f, 0, 0.125f, 0.375f );
			RightLegBack = MakeLeg( 0, 0.25f, 0.125f, 0.375f );
			
			vb = graphics.InitVb( vertices, VertexFormat.Pos3fTex2f );
			vertices = null;
			DefaultTexId = graphics.LoadTexture( "creeper.png" );
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
		
		public override string ModelName {
			get { return "creeper"; }
		}
		
		public override float NameYOffset {
			get { return 1.7f; }
		}
		
		protected override void DrawPlayerModel( Player player, PlayerRenderer renderer ) {
			int texId = renderer.MobTextureId <= 0 ? DefaultTexId : renderer.MobTextureId;
			graphics.Bind2DTexture( texId );
			
			DrawRotate( 0, 1.125f, 0, -pitch, 0, 0, Head );
			DrawPart( Torso );
			DrawRotate( 0, 0.375f, -0.125f, leftLegXRot, 0, 0, LeftLegFront );
			DrawRotate( 0, 0.375f, -0.125f, rightLegXRot, 0, 0, RightLegFront );
			DrawRotate( 0, 0.375f, 0.125f, rightLegXRot, 0, 0, LeftLegBack );
			DrawRotate( 0, 0.375f, 0.125f, leftLegXRot, 0, 0, RightLegBack );
		}
		
		public override void Dispose() {
			graphics.DeleteVb( vb );
			graphics.DeleteTexture( ref DefaultTexId );
		}
		
		ModelPart Head, Torso, LeftLegFront, RightLegFront, LeftLegBack, RightLegBack;
	}
}