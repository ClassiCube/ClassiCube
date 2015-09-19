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
			Set64x32.LeftLeg = MakeLeftLeg( 0, 16, 4/16f, 0f, false );
			Set64x32.RightLeg = MakeRightLeg( 0, 16, 0, 4/16f, false );
			Set64x32.LeftArm = MakeLeftArm( 40, 16, 8/16f, 4/16f, 4, false );
			Set64x32.RightArm = MakeRightArm( 40, 16, 4/16f, 8/16f, 4, false );
			Set64x32.Hat = MakeHat( false );
			
			Set64x64 = new ModelSet();
			Set64x64.Head = MakeHead( true );
			Set64x64.Torso = MakeTorso( true );
			Set64x64.LeftLeg = MakeLeftLeg( 16, 48, 0, 4/16f, true );
			Set64x64.RightLeg = MakeRightLeg( 0, 16, 0, 4/16f, true );
			Set64x64.LeftArm = MakeLeftArm( 32, 48, 4/16f, 8/16f, 4, true );
			Set64x64.RightArm = MakeRightArm( 40, 16, 4/16f, 8/16f, 4, true );
			Set64x64.Hat = MakeHat( true );
			
			Set64x64Slim = new ModelSet();
			Set64x64Slim.Head = Set64x64.Head;
			Set64x64Slim.Torso = Set64x64.Torso;
			Set64x64Slim.LeftLeg = Set64x64.LeftLeg;
			Set64x64Slim.RightLeg = Set64x64.RightLeg;
			Set64x64Slim.LeftArm = MakeLeftArm( 32, 48, 4/16f, 7/16f, 3, true );
			Set64x64Slim.RightArm = MakeRightArm( 40, 16, 4/16f, 7/16f, 3, true );
			Set64x64Slim.Hat = Set64x64.Hat;
		}
		
		ModelPart MakeLeftArm( int x, int y, float x1, float x2, int width, bool _64x64 ) {
			return MakePart( x, y, 4, 12, width, 4, width, 12, -x2, -x1, 12f/16, 24f/16f, -2f/16, 2f/16, _64x64 );
		}
		
		ModelPart MakeRightArm( int x, int y, float x1, float x2, int width, bool _64x64 ) {
			return MakePart( x, y, 4, 12, width, 4, width, 12, x1, x2, 12f/16, 24f/16f, -2f/16, 2f/16, _64x64 );
		}
		
		ModelPart MakeHead( bool _64x64 ) {
			return MakePart( 0, 0, 8, 8, 8, 8, 8, 8, -4f/16, 4f/16, 24f/16f, 32f/16f, -4f/16, 4f/16, _64x64 );
		}
		
		ModelPart MakeTorso( bool _64x64 ) {
			return MakePart( 16, 16, 4, 12, 8, 4, 8, 12, -4f/16, 4f/16, 12f/16, 24f/16f, -2f/16, 2f/16, _64x64 );
		}
		
		ModelPart MakeHat( bool _64x64 ) {
			return MakePart( 32, 0, 8, 8, 8, 8, 8, 8, -4.5f/16, 4.5f/16, 23.5f/16f, 32.5f/16, -4.5f/16, 4.5f/16, _64x64 );
		}
		
		ModelPart MakeLeftLeg( int x, int y, float x1, float x2, bool _64x64 ) {
			return MakePart( x, y, 4, 12, 4, 4, 4, 12, -x2, -x1, 0f, 12f/16, -2f/16, 2f/16, _64x64 );
		}
		
		ModelPart MakeRightLeg( int x, int y, float x1, float x2, bool _64x64 ) {
			return MakePart( x, y, 4, 12, 4, 4, 4, 12, x1, x2, 0f, 12f/16, -2f/16, 2f/16, _64x64 );
		}
		
		public override float NameYOffset {
			get { return 2.1375f; }
		}
		
		public override float EyeY {
			get { return 26/16f; }
		}
		
		public override Vector3 CollisionSize {
			get { return new Vector3( 8/16f, 28.5f/16f, 8/16f ); }
		}
		
		public override BoundingBox PickingBounds {
			get { return new BoundingBox( -8/16f, 0, -4/16f, 8/16f, 32/16f, 4/16f ); }
		}
		
		ModelSet model;
		protected override void DrawPlayerModel( Player p ) {
			graphics.Texturing = true;
			int texId = p.PlayerTextureId <= 0 ? cache.HumanoidTexId : p.PlayerTextureId;
			graphics.BindTexture( texId );
			
			SkinType skinType = p.SkinType;
			model = Set64x32;
			if( skinType == SkinType.Type64x64 ) model = Set64x64;
			else if( skinType == SkinType.Type64x64Slim ) model = Set64x64Slim;
			
			DrawRotate( 0, 24/16f, 0, -p.PitchRadians, 0, 0, model.Head );
			DrawPart( model.Torso );
			DrawRotate( 0, 12/16f, 0, p.leftLegXRot, 0, 0, model.LeftLeg );
			DrawRotate( 0, 12/16f, 0, p.rightLegXRot, 0, 0, model.RightLeg );
			DrawRotate( -2/16f, 24/16f, 0, p.leftArmXRot, 0, p.leftArmZRot, model.LeftArm );
			DrawRotate( 2/16f, 24/16f, 0, p.rightArmXRot, 0, p.rightArmZRot, model.RightArm );
			graphics.AlphaTest = true;
			DrawRotate( 0, 23.5f/16f, 0, -p.PitchRadians, 0, 0, model.Hat );		
		}
		
		class ModelSet {
			
			public ModelPart Head, Torso, LeftLeg, RightLeg, LeftArm, RightArm, Hat;
		}
	}
}