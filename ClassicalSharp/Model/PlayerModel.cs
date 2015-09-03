using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Model {

	public class PlayerModel : IModel {
		
		ModelSet Set64x32, Set64x64, Set64x64Slim;
		public PlayerModel( Game window ) : base( window ) {
			vertices = new VertexPos3fTex2fCol4b[partVertices * ( 7 * 2 + 2 )];
			Set64x32 = new ModelSet();
			Set64x32.Head = MakeHead( false );
			Set64x32.Torso = MakeTorso( false );
			Set64x32.LeftLeg = MakeLeftLeg( 0, 16, 0.25f, 0f, false );
			Set64x32.RightLeg = MakeRightLeg( 0, 16, 0, 0.25f, false );
			Set64x32.LeftArm = MakeLeftArm( 40, 16, 0.5f, 0.25f, 4, false );
			Set64x32.RightArm = MakeRightArm( 40, 16, 0.25f, 0.5f, 4, false );
			Set64x32.Hat = MakeHat( false );
			
			Set64x64 = new ModelSet();
			Set64x64.Head = MakeHead( true );
			Set64x64.Torso = MakeTorso( true );
			Set64x64.LeftLeg = MakeLeftLeg( 16, 48, 0, 0.25f, true );
			Set64x64.RightLeg = MakeRightLeg( 0, 16, 0, 0.25f, true );
			Set64x64.LeftArm = MakeLeftArm( 32, 48, 0.25f, 0.5f, 4, true );
			Set64x64.RightArm = MakeRightArm( 40, 16, 0.25f, 0.5f, 4, true );
			Set64x64.Hat = MakeHat( true );
			
			Set64x64Slim = new ModelSet();
			Set64x64Slim.Head = Set64x64.Head;
			Set64x64Slim.Torso = Set64x64.Torso;
			Set64x64Slim.LeftLeg = Set64x64.LeftLeg;
			Set64x64Slim.RightLeg = Set64x64.RightLeg;
			Set64x64Slim.LeftArm = MakeLeftArm( 32, 48, 0.25f, 0.4375f, 3, true );
			Set64x64Slim.RightArm = MakeRightArm( 40, 16, 0.25f, 0.4375f, 3, true );
			Set64x64Slim.Hat = Set64x64.Hat;
			
			vb = graphics.CreateVb( vertices, VertexFormat.Pos3fTex2fCol4b );
			vertices = null;
			
			using( Bitmap bmp = new Bitmap( "char.png" ) ) {
				window.DefaultPlayerSkinType = Utils.GetSkinType( bmp );
				DefaultTexId = graphics.CreateTexture( bmp );
			}
		}
		
		ModelPart MakeLeftArm( int x, int y, float x1, float x2, int width, bool _64x64 ) {
			return MakePart( x, y, 4, 12, width, 4, width, 12, -x2, -x1, 0.75f, 1.5f, -0.125f, 0.125f, _64x64 );
		}
		
		ModelPart MakeRightArm( int x, int y, float x1, float x2, int width, bool _64x64 ) {
			return MakePart( x, y, 4, 12, width, 4, width, 12, x1, x2, 0.75f, 1.5f, -0.125f, 0.125f, _64x64 );
		}
		
		ModelPart MakeHead( bool _64x64 ) {
			return MakePart( 0, 0, 8, 8, 8, 8, 8, 8, -0.25f, 0.25f, 1.5f, 2f, -0.25f, 0.25f, _64x64 );
		}
		
		ModelPart MakeTorso( bool _64x64 ) {
			return MakePart( 16, 16, 4, 12, 8, 4, 8, 12, -0.25f, 0.25f, 0.75f, 1.5f, -0.125f, 0.125f, _64x64 );
		}
		
		ModelPart MakeHat( bool _64x64 ) {
			return MakePart( 32, 0, 8, 8, 8, 8, 8, 8, -0.3125f, 0.3125f, 1.4375f, 2.0625f, -0.3125f, 0.3125f, _64x64 );
		}
		
		ModelPart MakeLeftLeg( int x, int y, float x1, float x2, bool _64x64 ) {
			return MakePart( x, y, 4, 12, 4, 4, 4, 12, -x2, -x1, 0f, 0.75f, -0.125f, 0.125f, _64x64 );
		}
		
		ModelPart MakeRightLeg( int x, int y, float x1, float x2, bool _64x64 ) {
			return MakePart( x, y, 4, 12, 4, 4, 4, 12, x1, x2, 0f, 0.75f, -0.125f, 0.125f, _64x64 );
		}
		
		public override float NameYOffset {
			get { return 2.1375f; }
		}
		
		public override Vector3 CollisionSize {
			get { return new Vector3( 8 / 16f, 30 / 16f, 8 / 16f ); }
		}
		
		public override BoundingBox PickingBounds {
			get { return new BoundingBox( -8 / 16f, 0, -4 / 16f, 8 / 16f, 32 / 16f, 4 / 16f ); }
		}
		
		ModelSet model;
		protected override void DrawPlayerModel( Player p ) {
			graphics.Texturing = true;
			int texId = p.PlayerTextureId <= 0 ? DefaultTexId : p.PlayerTextureId;
			graphics.BindTexture( texId );		
			SkinType skinType = p.SkinType;
			model = Set64x32;
			if( skinType == SkinType.Type64x64 ) model = Set64x64;
			else if( skinType == SkinType.Type64x64Slim ) model = Set64x64Slim;
			
			DrawRotate( 0, 1.5f, 0, -p.PitchRadians, 0, 0, model.Head );
			model.Torso.Render( graphics );
			DrawRotate( 0, 0.75f, 0, p.leftLegXRot, 0, 0, model.LeftLeg );
			DrawRotate( 0, 0.75f, 0, p.rightLegXRot, 0, 0, model.RightLeg );
			DrawRotate( 0, 1.5f, 0, p.leftArmXRot, 0, p.leftArmZRot, model.LeftArm );
			DrawRotate( 0, 1.5f, 0, p.rightArmXRot, 0, p.rightArmZRot, model.RightArm );
			graphics.AlphaTest = true;
			DrawRotate( 0, 1.4375f, 0, -p.PitchRadians, 0, 0, model.Hat );
		}
		
		public override void Dispose() {
			graphics.DeleteVb( vb );
			graphics.DeleteTexture( ref DefaultTexId );
		}
		
		class ModelSet {
			
			public ModelPart Head, Torso, LeftLeg, RightLeg, LeftArm, RightArm, Hat;
		}
	}
}