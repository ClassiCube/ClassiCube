using System;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Renderers;
using OpenTK;

namespace ClassicalSharp.Model {

	public class CreeperModel : IModel {
		
		ModelSet Set;
		public CreeperModel( Game window ) : base( window ) {
			vertices = new VertexPos3fTex2fCol4b[partVertices * 6];
			Set = new ModelSet();
			Set.Head = MakeHead();
			Set.Torso = MakeTorso();
			Set.LeftLegFront = MakeLeg( -0.25f, 0, -0.375f, -0.125f );
			Set.RightLegFront = MakeLeg( 0, 0.25f, -0.375f, -0.125f );
			Set.LeftLegBack = MakeLeg( -0.25f, 0, 0.125f, 0.375f );
			Set.RightLegBack = MakeLeg( 0, 0.25f, 0.125f, 0.375f );
			
			vb = graphics.InitVb( vertices, VertexFormat.Pos3fTex2fCol4b );
			Set.SetVb( vb );
			vertices = null;
			DefaultTexId = graphics.LoadTexture( "creeper.png" );
		}
		
		ModelPart MakeHead() {
			return MakePart( 0, 0, 8, 8, 8, 8, 8, 8, -0.25f, 0.25f, 1.125f, 1.625f, -0.25f, 0.25f, false );
		}
		
		ModelPart MakeTorso() {
			return MakePart( 16, 16, 4, 12, 8, 4, 8, 12, -0.25f, 0.25f, 0.375f, 1.125f, -0.125f, 0.125f, false );
		}
		
		ModelPart MakeLeg( float x1, float x2, float z1, float z2 ) {
			return MakePart( 0, 16, 4, 6, 4, 4, 4, 6, x1, x2, 0f, 0.375f, z1, z2, false );
		}
		
		public override float NameYOffset {
			get { return 1.7f; }
		}
		
		public override Vector3 CollisionSize {
			get { return new Vector3( 8 / 16f, 26 / 16f, 8 / 16f ); }
		}
		
		public override BoundingBox PickingBounds {
			get { return new BoundingBox( -4 / 16f, 0, -6 / 16f, 4 / 16f, 26 / 16f, 6 / 16f ); }
		}
		
		protected override void DrawPlayerModel( Player p, PlayerRenderer renderer ) {
			graphics.Texturing = true;
			int texId = renderer.MobTextureId <= 0 ? DefaultTexId : renderer.MobTextureId;
			graphics.Bind2DTexture( texId );
			
			DrawRotate( 0, 1.125f, 0, -p.PitchRadians, 0, 0, Set.Head );
			Set.Torso.Render();
			DrawRotate( 0, 0.375f, -0.125f, p.leftLegXRot, 0, 0, Set.LeftLegFront );
			DrawRotate( 0, 0.375f, -0.125f, p.rightLegXRot, 0, 0, Set.RightLegFront );
			DrawRotate( 0, 0.375f, 0.125f, p.rightLegXRot, 0, 0, Set.LeftLegBack );
			DrawRotate( 0, 0.375f, 0.125f, p.leftLegXRot, 0, 0, Set.RightLegBack );
			graphics.AlphaTest = true;
		}
		
		public override void Dispose() {
			graphics.DeleteVb( vb );
			graphics.DeleteTexture( ref DefaultTexId );
		}
		
		class ModelSet {
			
			public ModelPart Head, Torso, LeftLegFront, RightLegFront, LeftLegBack, RightLegBack;
			
			public void SetVb( int vb ) {
				Head.Vb = Torso.Vb = LeftLegFront.Vb = RightLegFront.Vb
					= LeftLegBack.Vb = RightLegBack.Vb = vb;
			}
		}
	}
}