using System;
using OpenTK;

namespace ClassicalSharp.Model {

	public class SpiderModel : IModel {
		
		public SpiderModel( Game window ) : base( window ) {
			vertices = new ModelVertex[boxVertices * 5];
			Head = MakeHead();
			Link = MakeLink();
			End = MakeEnd();
			LeftLeg = MakeLeg( -19/16f, -3/16f );
			RightLeg = MakeLeg( 3/16f, 19/16f );
		}
		
		ModelPart MakeHead() {
			return MakeBox( 32, 4, 8, 8, 8, 8, 8, 8, -4/16f, 4/16f, 4/16f, 12/16f, -11/16f, -3/16f, false );
		}
		
		ModelPart MakeLink() {
			return MakeBox( 0, 0, 6, 6, 6, 6, 6, 6, -3/16f, 3/16f, 5/16f, 11/16f, 3/16f, -3/16f, false );
		}
		
		ModelPart MakeEnd() {
			return MakeBox( 0, 12, 12, 8, 10, 12, 10, 8, -5/16f, 5/16f, 4/16f, 12/16f, 3/16f, 15/16f, false );
		}
		
		ModelPart MakeLeg( float x1, float x2 ) {
			return MakeBox( 18, 0, 2, 2, 16, 2, 16, 2, x1, x2, 7/16f, 9/16f, -1/16f, 1/16f, false );
		}
		
		public override float NameYOffset {
			get { return 1.0125f; }
		}
		
		public override float GetEyeY( Player player ) {
			return 8/16f;
		}
		
		public override Vector3 CollisionSize {
			get { return new Vector3( 15/16f, 12/16f, 15/16f ); }
		}
		
		public override BoundingBox PickingBounds {
			get { return new BoundingBox( -5/16f, 0, -11/16f, 5/16f, 12/16f, 15/16f ); }
		}
		
		const float quarterPi = (float)( Math.PI / 4 );
		const float eighthPi = (float)( Math.PI / 8 );
		protected override void DrawPlayerModel( Player p ) {
			graphics.Texturing = true;
			int texId = p.MobTextureId <= 0 ? cache.SpiderTexId : p.MobTextureId;
			graphics.BindTexture( texId );
			graphics.AlphaTest = true;
			
			DrawRotate( 0, 8/16f, -3/16f, -p.PitchRadians, 0, 0, Head );
			DrawPart( Link );
			DrawPart( End );
			// TODO: leg animations
			DrawRotate( -3/16f, 8/16f, 0, 0, quarterPi, eighthPi, LeftLeg );
			DrawRotate( -3/16f, 8/16f, 0, 0, eighthPi, eighthPi, LeftLeg );
			DrawRotate( -3/16f, 8/16f, 0, 0, -eighthPi, eighthPi, LeftLeg );
			DrawRotate( -3/16f, 8/16f, 0, 0, -quarterPi, eighthPi, LeftLeg );
			DrawRotate( 3/16f, 8/16f, 0, 0, -quarterPi, -eighthPi, RightLeg );
			DrawRotate( 3/16f, 8/16f, 0, 0, -eighthPi, -eighthPi, RightLeg );
			DrawRotate( 3/16f, 8/16f, 0, 0, eighthPi, -eighthPi, RightLeg );
			DrawRotate( 3/16f, 8/16f, 0, 0, quarterPi, -eighthPi, RightLeg );
		}
		
		ModelPart Head, Link, End, LeftLeg, RightLeg;
	}
}