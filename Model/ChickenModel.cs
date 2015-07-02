using OpenTK;
using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Renderers;

namespace ClassicalSharp.Model {

	public class ChickenModel : IModel {
		
		ModelSet Set;
		public ChickenModel( Game window ) : base( window ) {
			vertices = new VertexPos3fTex2fCol4b[partVertices * 6 + planeVertices * 2 * 2];
			Set = new ModelSet();
			Set.Head = MakeHead();
			Set.Head2 = MakeHead2(); // TODO: Find a more appropriate name.
			Set.Head3 = MakeHead3();
			Set.Torso = MakeTorso();
			Set.LeftLeg = MakeLeg( -0.1875f, 0f, -0.125f, -0.0625f );
			Set.RightLeg = MakeLeg( 0f, 0.1875f, 0.0625f, 0.125f );
			Set.LeftWing = MakeWing( -0.25f, -0.1875f );
			Set.RightWing = MakeWing( 0.1875f, 0.25f );
			
			vb = graphics.InitVb( vertices, VertexFormat.Pos3fTex2fCol4b );
			Set.SetVb( vb );
			vertices = null;
			DefaultTexId = graphics.LoadTexture( "chicken.png" );
		}
		
		ModelPart MakeHead() {
			return MakePart( 0, 0, 3, 6, 4, 3, 4, 6, -0.125f, 0.125f, 0.5625f, 0.9375f, -0.375f, -0.1875f, false );
		}
		
		ModelPart MakeHead2() {
			return MakePart( 14, 4, 2, 2, 2, 2, 2, 2, -0.0625f, 0.0625f, 0.5625f, 0.6875f, -0.4375f, -0.3125f, false );
		}
		
		ModelPart MakeHead3() {
			return MakePart( 14, 0, 2, 2, 4, 2, 4, 2, -0.125f, 0.125f, 0.6875f, 0.8125f, -0.5f, -0.375f, false );
		}
		
		ModelPart MakeTorso() {
			return MakeRotatedPart( 0, 9, 6, 8, 6, 6, 6, 8, -0.1875f, 0.1875f, 0.3125f, 0.6875f, -0.25f, 0.25f, false );
		}
		
		ModelPart MakeWing( float x1, float x2 ) {
			return MakePart( 24, 13, 6, 4, 1, 6, 1, 4, x1, x2, 0.4375f, 0.6875f, -0.1875f, 0.1875f, false );
		}
		
		ModelPart MakeLeg( float x1, float x2, float legX1, float legX2 ) {
			const float y1 = 0f, y2 = 0.3125f, z2 = 0.0625f, z1 = -0.125f;		
			YPlane( 32, 0, 3, 3, x2, x1, z1, z2, y1, false ); // bottom feet
			ZPlane( 36, 3, 1, 5, legX1, legX2, y1, y2, z2, false ); // vertical part of leg
			return new ModelPart( vb, index - 12, 2 * 6, graphics );
		}
		
		public override float NameYOffset {
			get { return 1.0125f; }
		}
		
		public override Vector3 CollisionSize {
			get { return new Vector3( 8 / 16f, 12 / 16f, 8 / 16f ); }
		}
		
		public override BoundingBox PickingBounds {
			get { return new BoundingBox( -4 / 16f, 0, -8 / 16f, 4 / 16f, 15 / 16f, 4 / 16f ); }
		}
		
		protected override void DrawPlayerModel( Player player, PlayerRenderer renderer ) {
			graphics.Texturing = true;
			int texId = renderer.MobTextureId <= 0 ? DefaultTexId : renderer.MobTextureId;
			graphics.Bind2DTexture( texId );
			graphics.AlphaTest = true;
			
			DrawRotate( 0, 0.5625f, -0.1875f, -pitch, 0, 0, Set.Head );
			DrawRotate( 0, 0.5625f, -0.1875f, -pitch, 0, 0, Set.Head2 );
			DrawRotate( 0, 0.5625f, -0.1875f, -pitch, 0, 0, Set.Head3 );
			Set.Torso.Render();
			DrawRotate( 0, 0.3125f, 0.0625f, leftLegXRot, 0, 0, Set.LeftLeg );
			DrawRotate( 0, 0.3125f, 0.0625f, rightLegXRot, 0, 0, Set.RightLeg );
			DrawRotate( -0.1875f, 0.6875f, 0, 0, 0, -Math.Abs( leftArmXRot ), Set.LeftWing );
			DrawRotate( 0.1875f, 0.6875f, 0, 0, 0, Math.Abs( rightArmXRot ), Set.RightWing );
		}
		
		public override void Dispose() {
			graphics.DeleteVb( vb );
			graphics.DeleteTexture( ref DefaultTexId );
		}
		
		class ModelSet {
			
			public ModelPart Head, Head2, Head3, Torso, LeftLeg, RightLeg, LeftWing, RightWing;
			
			public void SetVb( int vb ) {
				Head.Vb = Head2.Vb = Head3.Vb = Torso.Vb = LeftLeg.Vb =
					RightLeg.Vb = LeftWing.Vb = RightWing.Vb = vb;
			}
		}
	}
}