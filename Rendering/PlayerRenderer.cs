using System;
using System.Collections.Generic;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Renderers {
	
	public class PlayerRenderer {
		
		Vector3 pos;
		float yaw, pitch;
		public Game Window;
		public IGraphicsApi Graphics;
		public bool Moves;
		public ModelSet Model;
		public Player Player;
		Texture nameTexture;
		float nameWidth, nameHeight;
		public int TextureId = -1;
		int nameTextureId = -1;
		float rightLegXRot, rightArmXRot, rightArmZRot;
		float leftLegXRot, leftArmXRot, leftArmZRot;
		
		public void SetSkinType( SkinType type ) {
			if( type == SkinType.Type64x32 ) Model = Window.ModelCache.Set64x32;
			else if( type == SkinType.Type64x64 ) Model = Window.ModelCache.Set64x64;
			else if( type == SkinType.Type64x64Slim ) Model = Window.ModelCache.Set64x64Slim;
			else Model = Window.ModelCache.Set64x32;
		}
		
		public PlayerRenderer( Player player, Game window ) {
			Player = player;
			Window = window;
			Graphics = window.Graphics;
			SetSkinType( window.DefaultPlayerSkinType );
			
			List<DrawTextArgs> parts = Utils.SplitText( Graphics, player.DisplayName, true );
			Size size = Utils2D.MeasureSize( Utils.StripColours( player.DisplayName ), "Arial", 14, true );
			nameTexture = Utils2D.MakeTextTexture( parts, "Arial", 14, size, 0, 0 );
			nameWidth = size.Width;
			nameHeight = size.Height;
			nameTextureId = nameTexture.ID;
		}
		
		public void Dispose() {
			Graphics.DeleteTexture( TextureId );
			Graphics.DeleteTexture( nameTextureId );
		}
		
		public void Render( double deltaTime ) {
			pos = Player.Position;
			yaw = Player.YawDegrees;
			pitch = Player.PitchDegrees;

			leftLegXRot = Player.leftLegXRot * 180 / (float)Math.PI;
			leftArmXRot = Player.leftArmXRot * 180 / (float)Math.PI;
			leftArmZRot = Player.leftArmZRot * 180 / (float)Math.PI;
			rightLegXRot = Player.rightLegXRot * 180 / (float)Math.PI;
			rightArmXRot = Player.rightArmXRot * 180 / (float)Math.PI;
			rightArmZRot = Player.rightArmZRot * 180 / (float)Math.PI;
			
			Graphics.PushMatrix();
			Graphics.Translate( pos.X, pos.Y, pos.Z );
			Graphics.RotateY( -yaw );
			DrawPlayerModel();
			Graphics.PopMatrix();
			DrawName();
		}

		private void DrawPlayerModel() {
			Graphics.Texturing = true;
			int texId = TextureId == -1 ? Window.DefaultPlayerTextureId : TextureId;
			Graphics.Bind2DTexture( texId );
			
			// Head
			Graphics.PushMatrix();
			Graphics.Translate( 0f, 1.625f, 0f );
			Graphics.RotateX( -pitch );
			Graphics.Translate( 0f, -1.625f, 0f );
			Model.Head.Render();
			Graphics.PopMatrix();
			
			// Torso
			Model.Torso.Render();
			
			// Left leg
			Graphics.PushMatrix();
			Graphics.Translate( 0f, 0.875f, 0f );
			Graphics.RotateX( leftLegXRot );
			Graphics.Translate( 0f, -0.875f, 0f );
			Model.LeftLeg.Render();
			Graphics.PopMatrix();
			
			// Right leg
			Graphics.PushMatrix();
			Graphics.Translate( 0f, 0.875f, 0f );
			Graphics.RotateX( rightLegXRot );
			Graphics.Translate( 0f, -0.875f, 0f );
			Model.RightLeg.Render();
			Graphics.PopMatrix();
			
			// Left arm
			Graphics.PushMatrix();
			Graphics.Translate( 0f, 1.625f, 0f );
			Graphics.RotateZ( leftArmZRot );
			Graphics.RotateX( leftArmXRot );
			Graphics.Translate( 0f, -1.625f, 0f );
			Model.LeftArm.Render();
			Graphics.PopMatrix();
			
			// Right arm
			Graphics.PushMatrix();
			Graphics.Translate( 0f, 1.625f, 0f );
			Graphics.RotateZ( rightArmZRot );
			Graphics.RotateX( rightArmXRot );
			Graphics.Translate( 0f, -1.625f, 0f );
			Model.RightArm.Render();
			Graphics.PopMatrix();
			
			// Hat
			Graphics.AlphaTest = true;
			Graphics.PushMatrix();
			Graphics.Translate( 0f, 1.5625f, 0f );
			Graphics.RotateX( -pitch );
			Graphics.Translate( 0f, -1.5625f, 0f );
			Model.Hat.Render();
			Graphics.PopMatrix();
		}
		
		const float nameScale = 50f;
		private void DrawName() {
			Graphics.PushMatrix();
			Graphics.Translate( pos.X, pos.Y + 2.2f, pos.Z );
			// Do this to always have names facing the player
			float yaw = Window.LocalPlayer.YawDegrees;
			Graphics.RotateY( 0f - yaw );
			// NOTE: Do this instead with network player's yaw to have names rotate with them instead.
			//Graphics.RotateY( 180f - yaw );
			Graphics.Scale( 1 / nameScale, -1 / nameScale, 1 / nameScale ); // -y to flip text
			Graphics.Translate( -nameWidth / 2f, -nameHeight, 0f );
			
			nameTexture.Render( Graphics );
			
			Graphics.PopMatrix();
			Graphics.Texturing = false;
			Graphics.AlphaTest = false;
		}
	}
}