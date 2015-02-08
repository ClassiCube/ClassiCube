using OpenTK;
using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Renderers;

namespace ClassicalSharp.Model {

	public class SpiderModel : IModel {
		
		ModelSet Set;
		public SpiderModel( Game window ) : base( window ) {
			vertices = new VertexPos3fTex2fCol4b[6 * 6];
			Set = new ModelSet();
			Set.Head = MakeHead();
			Set.Link = MakeLink();
			Set.End = MakeEnd();
			Set.LeftLeg = MakeLeg( -1.1875f, -0.1875f );
			Set.RightLeg = MakeLeg( 0.1875f, 1.1875f );
			vertices = null;

			DefaultSkinTextureId = graphics.LoadTexture( "spider.png" );
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
		
		protected override void DrawModelImpl( Player player, PlayerRenderer renderer ) {
			graphics.Texturing = true;
			int texId = DefaultSkinTextureId;
			graphics.Bind2DTexture( texId );
			graphics.AlphaTest = true;
			
			DrawRotate( 0, 0.5f, -0.1875f, -pitch, 0, 0, Set.Head );
			Set.Link.Render();
			Set.End.Render();
			DrawRotate( -0.1875f, 0.5f, 0, 0, 45f, 22.5f, Set.LeftLeg );
			DrawRotate( -0.1875f, 0.5f, 0, 0, 22.5f, 22.5f, Set.LeftLeg );
			DrawRotate( -0.1875f, 0.5f, 0, 0, -22.5f, 22.5f, Set.LeftLeg );
			DrawRotate( -0.1875f, 0.5f, 0, 0, -45f, 22.5f, Set.LeftLeg );		
			DrawRotate( 0.1875f, 0.5f, 0, 0, -45f, -22.5f, Set.RightLeg );
			DrawRotate( 0.1875f, 0.5f, 0, 0, -22.5f, -22.5f, Set.RightLeg );
			DrawRotate( 0.1875f, 0.5f, 0, 0, 22.5f, -22.5f, Set.RightLeg );
			DrawRotate( 0.1875f, 0.5f, 0, leftLegXRot, 45f, -22.5f, Set.RightLeg );
		}
		
		public override void Dispose() {
			Set.Dispose();
			graphics.DeleteTexture( DefaultSkinTextureId );
		}
		
		class ModelSet {
			
			public ModelPart Head, Link, End, LeftLeg, RightLeg;
			
			public void Dispose() {
				RightLeg.Dispose();
				LeftLeg.Dispose();
				End.Dispose();
				Head.Dispose();
				Link.Dispose();
			}
		}
	}
}