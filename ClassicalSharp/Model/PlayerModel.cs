using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Model {

	public class PlayerModel : IModel {
		
		ModelSet Set, SetSlim;
		public PlayerModel( Game window ) : base( window ) {
			vertices = new ModelVertex[partVertices * ( 7 + 2 )];
			Set = new ModelSet();
			Set.Head = MakeHead( false );
			Set.Torso = MakeTorso( false );
			Set.LeftLeg = MakeLeftLeg( 0, 16, 4/16f, 0f, false );
			Set.RightLeg = MakeRightLeg( 0, 16, 0, 4/16f, false );
			Set.LeftArm = MakeLeftArm( 40, 16, 8/16f, 4/16f, 4, false );
			Set.RightArm = MakeRightArm( 40, 16, 4/16f, 8/16f, 4, false );
			Set.Hat = MakeHat( false );
			
			SetSlim = new ModelSet();
			SetSlim.Head = Set.Head;
			SetSlim.Torso = Set.Torso;
			SetSlim.LeftLeg = Set.LeftLeg;
			SetSlim.RightLeg = Set.RightLeg;
			SetSlim.LeftArm = MakeLeftArm( 32, 48, 4/16f, 7/16f, 3, true );
			SetSlim.RightArm = MakeRightArm( 40, 16, 4/16f, 7/16f, 3, true );
			SetSlim.Hat = Set.Hat;
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
		
		public override float GetEyeY( Player player ) {
			return 26/16f;
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
			_64x64 = skinType != SkinType.Type64x32;
			model = skinType == SkinType.Type64x64Slim ? SetSlim : Set;
			
			DrawRotate( 0, 24/16f, 0, -p.PitchRadians, 0, 0, model.Head );
			DrawPart( model.Torso );
			DrawRotate( 0, 12/16f, 0, p.leftLegXRot, 0, 0, model.LeftLeg );
			DrawRotate( 0, 12/16f, 0, p.rightLegXRot, 0, 0, model.RightLeg );
			DrawRotate( -6/16f, 22/16f, 0, p.leftArmXRot, 0, p.leftArmZRot, model.LeftArm );
			DrawRotate( 6/16f, 22/16f, 0, p.rightArmXRot, 0, p.rightArmZRot, model.RightArm );
			
			graphics.AlphaTest = true;
			if( p.RenderHat ) {				
				DrawRotate( 0, 24f/16f, 0, -p.PitchRadians, 0, 0, model.Hat );
			}
		}
		
		class ModelSet {		
			public ModelPart Head, Torso, LeftLeg, RightLeg, LeftArm, RightArm, Hat;
		}
	}
}