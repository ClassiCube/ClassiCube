using System;
using System.Collections.Generic;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Model;
using OpenTK;

namespace ClassicalSharp.Renderers {
	
	public class PlayerRenderer {
		
		Vector3 pos;
		public Game Window;
		public OpenGLApi Graphics;
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
				List<DrawTextArgs> parts = Utils2D.SplitText( Graphics, player.DisplayName, true );
				Size size = Utils2D.MeasureSize( parts, font, true );
				nameTexture = Utils2D.MakeTextTexture( parts, font, size, 0, 0 );			
				nameWidth = size.Width;
				nameHeight = size.Height;
				nameTextureId = nameTexture.ID;
			}
		}
		
		public void Dispose() {
			Graphics.DeleteTexture( ref PlayerTextureId );
			Graphics.DeleteTexture( ref nameTextureId );
		}
		
		public void Render( double deltaTime ) {
			pos = Player.Position;
			ModelCache cache = Window.ModelCache;
			cache.Shader.SetUniform( cache.Shader.colourLoc, 0.7f );
			Player.Model.RenderModel( Player, this );
			DrawName();
		}
		
		const float invNameScale = 1 / 50f;
		private void DrawName() {
			ModelCache cache = Window.ModelCache;
			Matrix4 matrix = Window.MVP;
			matrix = Matrix4.Translation( pos.X, pos.Y + Player.Model.NameYOffset, pos.Z ) * matrix;
			// Do this to always have names facing the player
			float yaw = Window.LocalPlayer.YawRadians;
			matrix = Matrix4.RotateY( 0f - yaw ) * matrix;
			// NOTE: Do this instead with network player's yaw to have names rotate with them instead.
			//Graphics.RotateY( 180f - yaw );
			matrix = Matrix4.Scale( invNameScale, -invNameScale, invNameScale ) * matrix; // -y to flip text
			matrix = Matrix4.Translation( -nameWidth / 2f, -nameHeight, 0f ) * matrix;
			cache.Shader.SetUniform( cache.Shader.mvpLoc, ref matrix );
			// We have to duplicate code from IGraphicsApi because (currently) it only works with the gui shader.
			
			Texture tex = nameTexture;			
			VertexPos3fTex2f[] quads = cache.EntityNameVertices;
			int quadVb = cache.EntityNameVb;
			Graphics.Bind2DTexture( tex.ID );
			cache.Shader.SetUniform( cache.Shader.colourLoc, 1f );
			
			float x1 = tex.X1, y1 = tex.Y1, x2 = tex.X2, y2 = tex.Y2;
			// Have to order them this way because it's a triangle strip.
			quads[0] = new VertexPos3fTex2f( x2, y1, 0, tex.U2, tex.V1 );
			quads[1] = new VertexPos3fTex2f( x2, y2, 0, tex.U2, tex.V2 );
			quads[2] = new VertexPos3fTex2f( x1, y1, 0, tex.U1, tex.V1 );
			quads[3] = new VertexPos3fTex2f( x1, y2, 0, tex.U1, tex.V2 );
			cache.Shader.DrawDynamic( DrawMode.TriangleStrip, VertexPos3fTex2f.Size, quadVb, quads, 4 );
		}
	}
}