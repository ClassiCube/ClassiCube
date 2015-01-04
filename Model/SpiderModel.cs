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
			//Set.LeftLegFront = MakeLeg( -0.1875f, 0f, -0.125f, -0.0625f );
			//Set.RightLegFront = MakeLeg( 0f, 0.1875f, 0.0625f, 0.125f );
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
			index = 0;
			const float x1 = -0.3125f, x2 = 0.3125f, y1 = 0.25f, y2 = 0.75f, z1 = 0.1875f, z2 = 0.9375f;
			
			YPlane( 12, 12, 10, 12, x2, x1, z2, z1, y2, false ); // top
			YPlane( 22, 12, 10, 12, x2, x1, z1, z2, y1, false ); // bottom
			ZPlane( 12, 24, 10, 8, x2, x1, y1, y2, z1, false ); // front
			ZPlane( 22, 24, 10, 8, x2, x1, y2, y1, z2, false ); // back
			XPlane( 0, 24, 12, 8, z2, z1, y1, y2, x1, false ); // left
			XPlane( 32, 24, 12, 8, z2, z1, y2, y1, x2, false ); // right
			return new ModelPart( vertices, 6 * 6, graphics );
		}
		
		public override float NameYOffset {
			get { return 1.0125f; }
		}
		
		protected override void DrawPlayerModel( Player player, PlayerRenderer renderer ) {
			graphics.Texturing = true;
			int texId = DefaultSkinTextureId;
			graphics.Bind2DTexture( texId );
			graphics.AlphaTest = true;
			
			DrawRotateX( 0, 0.5625f, -0.1875f, -pitch, Set.Head );
			Set.Link.Render();
			Set.End.Render();
			//DrawRotateX( 0, 0.3125f, 0.0625f, leftLegXRot, Set.LeftLegFront );
			//DrawRotateX( 0, 0.3125f, 0.0625f, rightLegXRot, Set.RightLegFront );
		}
		
		public override void Dispose() {
			Set.Dispose();
			graphics.DeleteTexture( DefaultSkinTextureId );
		}
		
		class ModelSet {
			
			public ModelPart Head, Link, End, LegLeft, LegRight;
			
			public void Dispose() {
				//RightLegFront.Dispose();
				//LeftLegFront.Dispose();
				End.Dispose();
				Head.Dispose();
				Link.Dispose();
			}
		}
	}
}