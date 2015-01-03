using OpenTK;
using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Renderers;

namespace ClassicalSharp.Model {

	public class PlayerModel : IModel {
		
		ModelSet Set64x32, Set64x64, Set64x64Slim;
		public PlayerModel( Game window ) : base( window ) {
			vertices = new VertexPos3fTex2fCol4b[6 * 6];
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
			Set64x64Slim.Head = MakeHead( true );
			Set64x64Slim.Torso = MakeTorso( true );
			Set64x64Slim.LeftLeg = MakeLeftLeg( 16, 48, 0, 0.25f, true );
			Set64x64Slim.RightLeg = MakeRightLeg( 0, 16, 0, 0.25f, true );
			Set64x64Slim.LeftArm = MakeLeftArm( 32, 48, 0.25f, 0.4375f, 3, true );
			Set64x64Slim.RightArm = MakeRightArm( 40, 16, 0.25f, 0.4375f, 3, true );
			Set64x64Slim.Hat = MakeHat( true );
			vertices = null;
			
			using( Bitmap bmp = new Bitmap( "char.png" ) ) {
				window.DefaultPlayerSkinType = Utils.GetSkinType( bmp );
				DefaultSkinTextureId = graphics.LoadTexture( bmp );
			}
		}
		
		ModelPart MakeLeftArm( int x, int y, float x1, float x2, int width, bool _64x64 ) {
			return MakePart( x, y, 4, 12, width, 4, width, 12, -x2, -x1, 0.875f, 1.625f, -0.125f, 0.125f, _64x64 );
		}
		
		ModelPart MakeRightArm( int x, int y, float x1, float x2, int width, bool _64x64 ) {
			return MakePart( x, y, 4, 12, width, 4, width, 12, x1, x2, 0.875f, 1.625f, -0.125f, 0.125f, _64x64 );
		}
		
		ModelPart MakeHead( bool _64x64 ) {
			return MakePart( 0, 0, 8, 8, 8, 8, 8, 8, -0.25f, 0.25f, 1.625f, 2.125f, -0.25f, 0.25f, _64x64 );
		}
		
		ModelPart MakeTorso( bool _64x64 ) {
			return MakePart( 16, 16, 4, 12, 8, 4, 8, 12, -0.25f, 0.25f, 0.875f, 1.625f, -0.125f, 0.125f, _64x64 );
		}
		
		ModelPart MakeHat( bool _64x64 ) {
			return MakePart( 32, 0, 8, 8, 8, 8, 8, 8, -0.3125f, 0.3125f, 1.5625f, 2.18775f, -0.3125f, 0.3125f, _64x64 );
		}
		
		ModelPart MakeLeftLeg( int x, int y, float x1, float x2, bool _64x64 ) {
			return MakePart( x, y, 4, 12, 4, 4, 4, 12, -x2, -x1, 0f, 0.875f, -0.125f, 0.125f, _64x64 );
		}
		
		ModelPart MakeRightLeg( int x, int y, float x1, float x2, bool _64x64 ) {
			return MakePart( x, y, 4, 12, 4, 4, 4, 12, x1, x2, 0f, 0.875f, -0.125f, 0.125f, _64x64 );
		}
		
		public override float NameYOffset {
			get { return 2.2f; }
		}
		
		Vector3 pos;
		float yaw, pitch;
		ModelSet model;
		float rightLegXRot, rightArmXRot, rightArmZRot;
		float leftLegXRot, leftArmXRot, leftArmZRot;
		
		public override void RenderModel( Player player, PlayerRenderer renderer ) {
			pos = player.Position;
			yaw = player.YawDegrees;
			pitch = player.PitchDegrees;
			
			leftLegXRot = player.leftLegXRot * 180 / (float)Math.PI;
			leftArmXRot = player.leftArmXRot * 180 / (float)Math.PI;
			leftArmZRot = player.leftArmZRot * 180 / (float)Math.PI;
			rightLegXRot = player.rightLegXRot * 180 / (float)Math.PI;
			rightArmXRot = player.rightArmXRot * 180 / (float)Math.PI;
			rightArmZRot = player.rightArmZRot * 180 / (float)Math.PI;
			
			model = Set64x32;
			SkinType skinType = player.SkinType;
			if( skinType == SkinType.Type64x64 ) model = Set64x64;
			else if( skinType == SkinType.Type64x64Slim ) model = Set64x64Slim;
			
			graphics.PushMatrix();
			graphics.Translate( pos.X, pos.Y, pos.Z );
			graphics.RotateY( -yaw );
			DrawPlayerModel( player, renderer );
			graphics.PopMatrix();
		}
		
		private void DrawPlayerModel( Player player, PlayerRenderer renderer ) {
			graphics.Texturing = true;
			int texId = renderer.TextureId <= 0 ? DefaultSkinTextureId : renderer.TextureId;
			graphics.Bind2DTexture( texId );
			
			DrawRotateX( 0, 1.625f, 0, -pitch, model.Head );
			model.Torso.Render();
			DrawRotateX( 0, 0.875f, 0, leftLegXRot, model.LeftLeg );
			DrawRotateX( 0, 0.875f, 0, rightLegXRot, model.RightLeg );
			DrawRotateXZ( 0, 1.625f, 0, leftArmXRot, leftArmZRot, model.LeftArm );
			DrawRotateXZ( 0, 1.625f, 0, rightArmXRot, rightArmZRot, model.RightArm );
			graphics.AlphaTest = true;
			DrawRotateX( 0, 1.5625f, 0, -pitch, model.Hat );
		}
		
		public override void Dispose() {
			Set64x32.Dispose();
			Set64x64.Dispose();
			Set64x64Slim.Dispose();
			graphics.DeleteTexture( DefaultSkinTextureId );
		}
		
		class ModelSet {
			
			public ModelPart Head, Torso, LeftLeg, RightLeg, LeftArm, RightArm, Hat;
			
			public void Dispose() {
				Hat.Dispose();
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