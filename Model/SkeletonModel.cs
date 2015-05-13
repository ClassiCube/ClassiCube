using OpenTK;
using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Renderers;

namespace ClassicalSharp.Model {

	public class SkeletonModel : IModel {
		
		ModelSet Set;
		public SkeletonModel( Game window ) : base( window ) {
			vertices = new VertexPos3fTex2fCol4b[6 * 6];
			Set = new ModelSet();
			Set.Head = MakeHead();
			Set.Torso = MakeTorso();
			Set.LeftLeg = MakeLeftLeg( 0.1875f, 0.0625f );
			Set.RightLeg = MakeRightLeg( 0.0625f, 0.1875f );
			Set.LeftArm = MakeLeftArm( 0.375f, 0.25f );
			Set.RightArm = MakeRightArm( 0.25f, 0.375f );
			vertices = null;

			DefaultSkinTextureId = graphics.LoadTexture( "skeleton.png" );
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
		
		public override float NameYOffset {
			get { return 2.075f; }
		}
		
		protected override void DrawPlayerModel( Player player, PlayerRenderer renderer ) {
			graphics.Texturing = true;
			graphics.AlphaTest = true;
			int texId = DefaultSkinTextureId;
			graphics.Bind2DTexture( texId );
			
			DrawRotate( 0, 1.5f, 0, -pitch, 0, 0, Set.Head );
			Set.Torso.Render();
			DrawRotate( 0, 0.75f, 0, leftLegXRot, 0, 0, Set.LeftLeg );
			DrawRotate( 0, 0.75f, 0, rightLegXRot, 0, 0, Set.RightLeg );
			DrawRotate( 0, 1.375f, 0, 90f, 0, leftArmZRot, Set.LeftArm );
			DrawRotate( 0, 1.375f, 0, 90f, 0, rightArmZRot, Set.RightArm );			
		}
		
		public override void Dispose() {
			Set.Dispose();
			graphics.DeleteTexture( ref DefaultSkinTextureId );
		}
		
		class ModelSet {
			
			public ModelPart Head, Torso, LeftLeg, RightLeg, LeftArm, RightArm;
			
			public void Dispose() {
				RightArm.Dispose();
				LeftArm.Dispose();
				RightLeg.Dispose();
				LeftLeg.Dispose();
				Torso.Dispose();
				Head.Dispose();
			}
		}
	}
}