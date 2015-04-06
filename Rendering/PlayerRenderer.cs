using System;
using System.Collections.Generic;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Renderers {
	
	public class PlayerRenderer {
		
		Vector3 pos;
		public Game Window;
		public OpenGLApi Graphics;
		public Player Player;
		Texture nameTexture;
		float nameWidth, nameHeight;
		public int TextureId = -1;
		int nameTextureId = -1;
		
		public PlayerRenderer( Player player, Game window ) {
			Player = player;
			Window = window;
			Graphics = window.Graphics;
			
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
		
		public void Render( double deltaTime, MapShader shader ) {
			pos = Player.Position;
			Player.Model.RenderModel( Player, this, shader );
			DrawName();
		}
		
		const float nameScale = 50f;
		private void DrawName() {
			Graphics.PushMatrix();
			Graphics.Translate( pos.X, pos.Y + Player.Model.NameYOffset, pos.Z );
			// Do this to always have names facing the player
			float yaw = Window.LocalPlayer.YawDegrees;
			Graphics.RotateY( 0f - yaw );
			// NOTE: Do this instead with network player's yaw to have names rotate with them instead.
			//Graphics.RotateY( 180f - yaw );
			Graphics.Scale( 1 / nameScale, -1 / nameScale, 1 / nameScale ); // -y to flip text
			Graphics.Translate( -nameWidth / 2f, -nameHeight, 0f );
			DrawTextureInWorld( ref nameTexture );
			Graphics.PopMatrix();
			Graphics.AlphaTest = false;
		}
		
		private void DrawTextureInWorld( ref Texture tex ) {
			Graphics.Texturing = true;
			Graphics.Bind2DTexture( tex.ID );
			
			float x1 = tex.X1, y1 = tex.Y1, x2 = tex.X2, y2 = tex.Y2;
			// Have to order them this way because it's a triangle strip.
			VertexPos3fTex2f[] vertices = {
				new VertexPos3fTex2f( x2, y1, 0, tex.U2, tex.V1 ),
				new VertexPos3fTex2f( x2, y2, 0, tex.U2, tex.V2 ),
				new VertexPos3fTex2f( x1, y1, 0, tex.U1, tex.V1 ),
				new VertexPos3fTex2f( x1, y2, 0, tex.U1, tex.V2 ),
			};
			Graphics.DrawVertices( DrawMode.TriangleStrip, vertices, 4 );
			Graphics.Texturing = false;
		}
	}
}