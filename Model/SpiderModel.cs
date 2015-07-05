using OpenTK;
using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Renderers;

namespace ClassicalSharp.Model {

	public class SpiderModel : IModel {
		
		ModelSet Set;
		public SpiderModel( Game window ) : base( window ) {
			vertices = new VertexPos3fTex2fCol4b[partVertices * 5];
			Set = new ModelSet();
			Set.Head = MakeHead();
			Set.Link = MakeLink();
			Set.End = MakeEnd();
			Set.LeftLeg = MakeLeg( -1.1875f, -0.1875f );
			Set.RightLeg = MakeLeg( 0.1875f, 1.1875f );
			
			vb = graphics.InitVb( vertices, VertexFormat.Pos3fTex2fCol4b );
			Set.SetVb( vb );
			vertices = null;
			DefaultTexId = graphics.LoadTexture( "spider.png" );
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
		
		public override float NameYOffset {
			get { return 1.0125f; }
		}
		
		public override Vector3 CollisionSize {
			get { return new Vector3( 15 / 16f, 12 / 16f, 15 / 16f ); }
		}
		
		public override BoundingBox PickingBounds {
			get { return new BoundingBox( -5 / 16f, 0, -11 / 16f, 5 / 16f, 12 / 16f, 15 / 16f ); }
		}
		
		const float quarterPi = (float)( Math.PI / 4 );
		const float eighthPi = (float)( Math.PI / 8 );
		protected override void DrawPlayerModel( Player p, PlayerRenderer renderer ) {
			graphics.Texturing = true;
			int texId = renderer.MobTextureId <= 0 ? DefaultTexId : renderer.MobTextureId;
			graphics.Bind2DTexture( texId );
			graphics.AlphaTest = true;
			
			DrawRotate( 0, 0.5f, -0.1875f, -p.PitchRadians, 0, 0, Set.Head );
			Set.Link.Render();
			Set.End.Render();
			// TODO: leg animations
			DrawRotate( -0.1875f, 0.5f, 0, 0, quarterPi, eighthPi, Set.LeftLeg );
			DrawRotate( -0.1875f, 0.5f, 0, 0, eighthPi, eighthPi, Set.LeftLeg );
			DrawRotate( -0.1875f, 0.5f, 0, 0, -eighthPi, eighthPi, Set.LeftLeg );
			DrawRotate( -0.1875f, 0.5f, 0, 0, -quarterPi, eighthPi, Set.LeftLeg );		
			DrawRotate( 0.1875f, 0.5f, 0, 0, -quarterPi, -eighthPi, Set.RightLeg );
			DrawRotate( 0.1875f, 0.5f, 0, 0, -eighthPi, -eighthPi, Set.RightLeg );
			DrawRotate( 0.1875f, 0.5f, 0, 0, eighthPi, -eighthPi, Set.RightLeg );
			DrawRotate( 0.1875f, 0.5f, 0, 0, quarterPi, -eighthPi, Set.RightLeg );
		}
		
		public override void Dispose() {
			graphics.DeleteVb( vb );
			graphics.DeleteTexture( ref DefaultTexId );
		}
		
		class ModelSet {
			
			public ModelPart Head, Link, End, LeftLeg, RightLeg;
			
			public void SetVb( int vb ) {
				Head.Vb = Link.Vb = LeftLeg.Vb = RightLeg.Vb =
					End.Vb = vb;
			}
		}
	}
}