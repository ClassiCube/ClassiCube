using OpenTK;
using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Renderers;

namespace ClassicalSharp.Model {

	public class ZombieModel : IModel {
		
		ModelSet Set;
		public ZombieModel( Game window ) : base( window ) {
			vertices = new VertexPos3fTex2fCol4b[partVertices * 6];
			Set = new ModelSet();
			Set.Head = MakeHead();
			Set.Torso = MakeTorso();
			Set.LeftLeg = MakeLeftLeg( 0.25f, 0f );
			Set.RightLeg = MakeRightLeg( 0, 0.25f );
			Set.LeftArm = MakeLeftArm( 0.5f, 0.25f );
			Set.RightArm = MakeRightArm( 0.25f, 0.5f );
			
			vb = graphics.InitVb( vertices, VertexFormat.Pos3fTex2fCol4b );
			Set.SetVb( vb );
			vertices = null;		
			DefaultTexId = graphics.LoadTexture( "zombie.png" );
		}
		
		ModelPart MakeLeftArm( float x1, float x2 ) {
			return MakePart( 40, 16, 4, 12, 4, 4, 4, 12, -x2, -x1, 0.75f, 1.5f, -0.125f, 0.125f, false );
		}
		
		ModelPart MakeRightArm( float x1, float x2 ) {
			return MakePart( 40, 16, 4, 12, 4, 4, 4, 12, x1, x2, 0.75f, 1.5f, -0.125f, 0.125f, false );
		}
		
		ModelPart MakeHead() {
			return MakePart( 0, 0, 8, 8, 8, 8, 8, 8, -0.25f, 0.25f, 1.5f, 2f, -0.25f, 0.25f, false );
		}
		
		ModelPart MakeTorso() {
			return MakePart( 16, 16, 4, 12, 8, 4, 8, 12, -0.25f, 0.25f, 0.75f, 1.5f, -0.125f, 0.125f, false );
		}
		
		ModelPart MakeLeftLeg( float x1, float x2 ) {
			return MakePart( 0, 16, 4, 12, 4, 4, 4, 12, -x2, -x1, 0f, 0.75f, -0.125f, 0.125f, false );
		}
		
		ModelPart MakeRightLeg( float x1, float x2 ) {
			return MakePart( 0, 16, 4, 12, 4, 4, 4, 12, x1, x2, 0f, 0.75f, -0.125f, 0.125f, false );
		}
		
		public override float NameYOffset {
			get { return 2.075f; }
		}
		
		public override Vector3 CollisionSize {
			get { return new Vector3( 8 / 16f, 30 / 16f, 8 / 16f ); }
		}
		
		public override BoundingBox PickingBounds {
			get { return new BoundingBox( -4 / 16f, 0, -4 / 16f, 4 / 16f, 32 / 16f, 4 / 16f ); }
		}
		
		protected override void DrawPlayerModel( Player player, PlayerRenderer renderer ) {
			graphics.Texturing = true;
			int texId = renderer.MobTextureId <= 0 ? DefaultTexId : renderer.MobTextureId;
			graphics.Bind2DTexture( texId );
			
			DrawRotate( 0, 1.5f, 0, -pitch, 0, 0, Set.Head );
			Set.Torso.Render();
			DrawRotate( 0, 0.75f, 0, leftLegXRot, 0, 0, Set.LeftLeg );
			DrawRotate( 0, 0.75f, 0, rightLegXRot, 0, 0, Set.RightLeg );
			DrawRotate( 0, 1.375f, 0, (float)Math.PI / 2, 0, leftArmZRot, Set.LeftArm );
			DrawRotate( 0, 1.375f, 0, (float)Math.PI / 2, 0, rightArmZRot, Set.RightArm );
			graphics.AlphaTest = true;
		}
		
		public override void Dispose() {
			graphics.DeleteVb( vb );
			graphics.DeleteTexture( ref DefaultTexId );
		}
		
		class ModelSet {
			
			public ModelPart Head, Torso, LeftLeg, RightLeg, LeftArm, RightArm;
			
			public void SetVb( int vb ) {
				Head.Vb = Torso.Vb = LeftLeg.Vb = RightLeg.Vb =
					LeftArm.Vb = RightArm.Vb = vb;
			}
		}
	}
}