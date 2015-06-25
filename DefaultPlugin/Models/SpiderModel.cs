using System;
using ClassicalSharp;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Model;
using ClassicalSharp.Renderers;

namespace DefaultPlugin {

	public class SpiderModel : IModel {
		
		public SpiderModel( Game window ) : base( window ) {
			vertices = new VertexPos3fTex2f[partVertices * 5];
			Head = MakeHead();
			Link = MakeLink();
			End = MakeEnd();
			LeftLeg = MakeLeg( -1.1875f, -0.1875f );
			RightLeg = MakeLeg( 0.1875f, 1.1875f );
			
			vb = graphics.InitVb( vertices, VertexPos3fTex2f.Size );
			vertices = null;
			DefaultTexId = TextureObj.LoadTexture( "spider.png" );
		}
		
		ModelPart MakeHead() {
			return MakePart( 32, 4, 8, 8, 8, 8, 8, 8, -0.25f, 0.25f, 0.25f, 0.75f, -0.6875f, -0.1875f, false );
		}
		
		ModelPart MakeLink() {
			return MakePart( 0, 0, 6, 6, 6, 6, 6, 6, -0.1875f, 0.1875f, 0.3125f, 0.6875f, 0.1875f, -0.1875f, false );
		}
		
		ModelPart MakeEnd() {
			return MakePart( 0, 12, 12, 8, 10, 12, 10, 8, -0.3125f, 0.3125f, 0.25f, 0.75f, 0.1875f, 0.9375f, false );
		}
		
		ModelPart MakeLeg( float x1, float x2 ) {
			return MakePart( 18, 0, 2, 2, 16, 2, 16, 2, x1, x2, 0.4375f, 0.5625f, -0.0625f, 0.0625f, false );
		}
		
		public override string ModelName {
			get { return "spider"; }
		}
		
		public override float NameYOffset {
			get { return 1.0125f; }
		}
		
		protected override void DrawPlayerModel( Player player, PlayerRenderer renderer ) {
			TextureObj texId = renderer.MobTexId.IsInvalid ? DefaultTexId : renderer.MobTexId;
			texId.Bind();
			
			DrawRotate( 0, 0.5f, -0.1875f, -pitch, 0, 0, Head );
			DrawPart( Link );
			DrawPart( End );
			// TODO: leg animations
			DrawRotate( -0.1875f, 0.5f, 0, 0, 45f, 22.5f, LeftLeg );
			DrawRotate( -0.1875f, 0.5f, 0, 0, 22.5f, 22.5f, LeftLeg );
			DrawRotate( -0.1875f, 0.5f, 0, 0, -22.5f, 22.5f, LeftLeg );
			DrawRotate( -0.1875f, 0.5f, 0, 0, -45f, 22.5f, LeftLeg );		
			DrawRotate( 0.1875f, 0.5f, 0, 0, -45f, -22.5f, RightLeg );
			DrawRotate( 0.1875f, 0.5f, 0, 0, -22.5f, -22.5f, RightLeg );
			DrawRotate( 0.1875f, 0.5f, 0, 0, 22.5f, -22.5f, RightLeg );
			DrawRotate( 0.1875f, 0.5f, 0, 0, 45f, -22.5f, RightLeg );
		}
		
		public override void Dispose() {
			graphics.DeleteVb( vb );
			DefaultTexId.Delete();
		}
		
		ModelPart Head, Link, End, LeftLeg, RightLeg;
	}
}