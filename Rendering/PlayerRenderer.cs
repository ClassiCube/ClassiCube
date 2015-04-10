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
			DrawName( shader );
		}
		
		const float nameScale = 1 / 50f;
		private void DrawName( MapShader shader ) {
			Matrix4 newMvp = Matrix4.CreateTranslation( -nameWidth / 2f, -nameHeight, 0 ) * 
				Matrix4.Scale( nameScale, -nameScale, nameScale ) *
				Matrix4.CreateRotationY( -Window.LocalPlayer.YawRadians ) * 
				Matrix4.CreateTranslation( pos.X, pos.Y + Player.Model.NameYOffset, pos.Z ) * Window.mvp;
			
			Graphics.SetUniform( shader.mvpLoc, ref newMvp );
			DrawTextureInWorld( ref nameTexture, shader );
			Graphics.SetUniform( shader.mvpLoc, ref Window.mvp );
		}
		
		private void DrawTextureInWorld( ref Texture tex, MapShader shader ) {
			Graphics.Bind2DTexture( tex.ID );			
			float x1 = tex.X1, y1 = tex.Y1, x2 = tex.X2, y2 = tex.Y2;
			VertexPos3fTex2fCol4b[] vertices = {
				new VertexPos3fTex2fCol4b( x2, y1, 0, tex.U2, tex.V1, FastColour.White ),
				new VertexPos3fTex2fCol4b( x2, y2, 0, tex.U2, tex.V2, FastColour.White ),
				new VertexPos3fTex2fCol4b( x1, y1, 0, tex.U1, tex.V1, FastColour.White ),
				new VertexPos3fTex2fCol4b( x1, y2, 0, tex.U1, tex.V2, FastColour.White ),
			};
			
			Graphics.BindVb( Graphics.vb2d );
			Graphics.UpdateDynamicVb( Graphics.vb2d, vertices, VertexFormat.VertexPos3fTex2fCol4b );
			shader.DrawVb( Graphics, DrawMode.TriangleStrip, Graphics.vb2d, 4 );
		}
	}
}