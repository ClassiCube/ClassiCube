using System;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Model {

	public class ChickenModel : IModel {
		
		public ChickenModel( Game window ) : base( window ) {
			vertices = new ModelVertex[boxVertices * 6 + quadVertices * 2 * 2];
			Head = MakeHead();
			Head2 = MakeHead2(); // TODO: Find a more appropriate name.
			Head3 = MakeHead3();
			Torso = MakeTorso();
			LeftLeg = MakeLeg( -3/16f, 0f, -2/16f, -1/16f );
			RightLeg = MakeLeg( 0f, 3/16f, 1/16f, 2/16f );
			LeftWing = MakeWing( -4/16f, -3/16f );
			RightWing = MakeWing( 3/16f, 4/16f );
		}
		
		ModelPart MakeHead() {
			return MakeBox( 0, 0, 3, 6, 4, 3, 4, 6, -2/16f, 2/16f, 9/16f, 15/16f, -6/16f, -3/16f );
		}
		
		ModelPart MakeHead2() {
			return MakeBox( 14, 4, 2, 2, 2, 2, 2, 2, -1/16f, 1/16f, 9/16f, 11/16f, -7/16f, -5/16f );
		}
		
		ModelPart MakeHead3() {
			return MakeBox( 14, 0, 2, 2, 4, 2, 4, 2, -2/16f, 2/16f, 11/16f, 13/16f, -8/16f, -6/16f );
		}
		
		ModelPart MakeTorso() {
			return MakeRotatedBox( 0, 9, 6, 8, 6, 6, 6, 8, -3/16f, 3/16f, 5/16f, 11/16f, -4/16f, 4/16f );
		}
		
		ModelPart MakeWing( float x1, float x2 ) {
			return MakeBox( 24, 13, 6, 4, 1, 6, 1, 4, x1, x2, 7/16f, 11/16f, -3/16f, 3/16f );
		}
		
		ModelPart MakeLeg( float x1, float x2, float legX1, float legX2 ) {
			const float y1 = 1/64f, y2 = 5/16f, z2 = 1/16f, z1 = -2/16f;		
			YQuad( 32, 0, 3, 3, x2, x1, z1, z2, y1 ); // bottom feet
			ZQuad( 36, 3, 1, 5, legX1, legX2, y1, y2, z2 ); // vertical part of leg
			return new ModelPart( index - 2 * 4, 2 * 4 );
		}
		
		public override float NameYOffset {
			get { return 1.0125f; }
		}
		
		public override float GetEyeY( Player player ) {
			return 14/16f;
		}
		
		public override Vector3 CollisionSize {
			get { return new Vector3( 8/16f, 12/16f, 8/16f ); }
		}
		
		public override BoundingBox PickingBounds {
			get { return new BoundingBox( -4/16f, 0, -8/16f, 4/16f, 15/16f, 4/16f ); }
		}
		
		protected override void DrawPlayerModel( Player p ) {
			graphics.Texturing = true;
			int texId = p.MobTextureId <= 0 ? cache.ChickenTexId : p.MobTextureId;
			graphics.BindTexture( texId );
			graphics.AlphaTest = true;
			
			DrawRotate( 0, 9/16f, -3/16f, -p.PitchRadians, 0, 0, Head );
			DrawRotate( 0, 9/16f, -3/16f, -p.PitchRadians, 0, 0, Head2 );
			DrawRotate( 0, 9/16f, -3/16f, -p.PitchRadians, 0, 0, Head3 );
			DrawPart( Torso );
			DrawRotate( 0, 5/16f, 1/16f, p.leftLegXRot, 0, 0, LeftLeg );
			DrawRotate( 0, 5/16f, 1/16f, p.rightLegXRot, 0, 0, RightLeg );
			DrawRotate( -3/16f, 11/16f, 0, 0, 0, -Math.Abs( p.leftArmXRot ), LeftWing );
			DrawRotate( 3/16f, 11/16f, 0, 0, 0, Math.Abs( p.rightArmXRot ), RightWing );
		}
		
		ModelPart Head, Head2, Head3, Torso, LeftLeg, RightLeg, LeftWing, RightWing;
	}
}