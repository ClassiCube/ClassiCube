using System;
using System.Collections.Generic;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Renderers {
	
	public class PlayerRenderer {

		public Game Window;
		public IGraphicsApi Graphics;
		public Player Player;
		Texture nameTexture;
		float nameWidth, nameHeight;
		public int PlayerTextureId = -1, MobTextureId = -1;
		int nameTextureId = -1;
		
		public PlayerRenderer( Player player, Game window ) {
			Player = player;
			Window = window;
			Graphics = window.Graphics;
			
			using( Font font = new Font( "Arial", 14 ) ) {
				DrawTextArgs args = new DrawTextArgs( Graphics, player.DisplayName, true );
				nameTexture = Utils2D.MakeTextTexture( font, 0, 0, ref args );
				nameWidth = nameTexture.Width;
				nameHeight = nameTexture.Height;
				nameTextureId = nameTexture.ID;
			}
		}
		
		public void Dispose() {
			Graphics.DeleteTexture( ref PlayerTextureId );
			Graphics.DeleteTexture( ref nameTextureId );
		}
		
		public void Render( double deltaTime ) {
			Player.Model.RenderModel( Player, this );
			DrawName();
		}
		
		const float nameScale = 1 / 50f;
		private void DrawName() {
			Vector3 pos = Player.Position;
			Graphics.PushMatrix();
			Matrix4 mat = Matrix4.Translate( pos.X, pos.Y + Player.Model.NameYOffset, pos.Z );
			// Do this to always have names facing the player
			float yaw = Window.LocalPlayer.YawRadians;
			mat = Matrix4.RotateY( 0f - yaw ) * mat;
			// NOTE: Do this instead with network player's yaw to have names rotate with them instead.
			//Graphics.RotateY( 180f - yaw );
			mat = Matrix4.Scale( nameScale, -nameScale, nameScale ) * mat; // -y to flip text
			mat = Matrix4.Translate( -nameWidth / 2f, -nameHeight, 0f ) * mat;
			Graphics.MultiplyMatrix( ref mat );
			nameTexture.Render( Graphics );
			
			Graphics.PopMatrix();
			Graphics.Texturing = false;
			Graphics.AlphaTest = false;
		}
	}
}