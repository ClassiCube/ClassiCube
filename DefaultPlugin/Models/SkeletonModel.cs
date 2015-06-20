using System;
using ClassicalSharp;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Model;
using ClassicalSharp.Renderers;

namespace DefaultPlugin {

	public class SkeletonModel : IModel {
		
		public SkeletonModel( Game window ) : base( window ) {
			vertices = new VertexPos3fTex2f[partVertices * 6];
			Head = MakeHead();
			Torso = MakeTorso();
			LeftLeg = MakeLeftLeg( 0.1875f, 0.0625f );
			RightLeg = MakeRightLeg( 0.0625f, 0.1875f );
			LeftArm = MakeLeftArm( 0.375f, 0.25f );
			RightArm = MakeRightArm( 0.25f, 0.375f );
			
			vb = graphics.InitVb( vertices, VertexPos3fTex2f.Size );
			vertices = null;
			DefaultTexId = graphics.LoadTexture( "skeleton.png" );
		}
		
		ModelPart MakeLeftArm( float x1, float x2 ) {
			return MakePart( 40, 16, 2, 12, 2, 2, 2, 12, -x2, -x1, 0.75f, 1.5f, -0.0625f, 0.0625f, false );
		}
		
		ModelPart MakeRightArm( float x1, float x2 ) {
			return MakePart( 40, 16, 2, 12, 2, 2, 2, 12, x1, x2, 0.75f, 1.5f, -0.0625f, 0.0625f, false );
		}
		
		ModelPart MakeHead() {
			return MakePart( 0, 0, 8, 8, 8, 8, 8, 8, -0.25f, 0.25f, 1.5f, 2f, -0.25f, 0.25f, false );
		}
		
		ModelPart MakeTorso() {
			return MakePart( 16, 16, 4, 12, 8, 4, 8, 12, -0.25f, 0.25f, 0.75f, 1.5f, -0.125f, 0.125f, false );
		}
		
		ModelPart MakeLeftLeg( float x1, float x2 ) {
			return MakePart( 0, 16, 2, 12, 2, 2, 2, 12, -x2, -x1, 0f, 0.75f, -0.0625f, 0.0625f, false );
		}
		
		ModelPart MakeRightLeg( float x1, float x2 ) {
			return MakePart( 0, 16, 2, 12, 2, 2, 2, 12, x1, x2, 0f, 0.75f, -0.0625f, 0.0625f, false );
		}
		
		public override string ModelName {
			get { return "skeleton"; }
		}
		
		public override float NameYOffset {
			get { return 2.075f; }
		}
		
		protected override void DrawPlayerModel( Player player, PlayerRenderer renderer ) {
			int texId = renderer.MobTextureId <= 0 ? DefaultTexId : renderer.MobTextureId;
			graphics.Bind2DTexture( texId );
			
			DrawRotate( 0, 1.5f, 0, -pitch, 0, 0, Head );
			DrawPart( Torso );
			DrawRotate( 0, 0.75f, 0, leftLegXRot, 0, 0, LeftLeg );
			DrawRotate( 0, 0.75f, 0, rightLegXRot, 0, 0, RightLeg );
			DrawRotate( 0, 1.375f, 0, 90f, 0, leftArmZRot, LeftArm );
			DrawRotate( 0, 1.375f, 0, 90f, 0, rightArmZRot, RightArm );			
		}
		
		public override void Dispose() {
			graphics.DeleteVb( vb );
			graphics.DeleteTexture( ref DefaultTexId );
		}
		
		ModelPart Head, Torso, LeftLeg, RightLeg, LeftArm, RightArm;
	}
}