using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp {

	partial class Player {

		protected IGraphicsApi api;
		protected Texture nameTex;
		protected internal int PlayerTextureId = -1, MobTextureId = -1;
		
		public override void Despawn() {
			if( api == null ) return;
			api.DeleteTexture( ref PlayerTextureId );
			api.DeleteTexture( ref nameTex.ID );
		}
		
		protected void InitRenderingData() {
			api = Window.Graphics;
			
			using( Font font = new Font( "Arial", 14 ) ) {
				DrawTextArgs args = new DrawTextArgs( api, DisplayName, true );
				nameTex = Utils2D.MakeTextTexture( font, 0, 0, ref args );
			}
		}
		
		protected void RenderModel( double deltaTime ) {
			Model.RenderModel( this );
			DrawName();
		}
		
		void DrawName() {
			api.Texturing = true;
			api.Bind2DTexture( nameTex.ID );
			
			float x1 = -nameTex.Width * 0.5f / 50f, y1 = nameTex.Height / 50f;
			float x2 = nameTex.Width * 0.5f / 50f, y2 = 0;
			// NOTE: Do this instead with network player's yaw to have names rotate with them instead.
			//yaw = Math.Pi - Player.YawRadians;
			float angle = Window.LocalPlayer.YawRadians;
			float cosA = (float)Math.Cos( angle ), sinA = (float)Math.Sin( angle );
			Vector3 pos = Position;
			pos.Y += Model.NameYOffset;
			
			// Inlined translation + rotation Y axis
			api.texVerts[0] = new VertexPos3fTex2f( cosA * x2 + pos.X, y1 + pos.Y, sinA * x2 + pos.Z, nameTex.U2, nameTex.V1 );
			api.texVerts[1] = new VertexPos3fTex2f( cosA * x2 + pos.X, y2 + pos.Y, sinA * x2 + pos.Z, nameTex.U2, nameTex.V2 );
			api.texVerts[2] = new VertexPos3fTex2f( cosA * x1 + pos.X, y1 + pos.Y, sinA * x1 + pos.Z, nameTex.U1, nameTex.V1 );
			api.texVerts[3] = new VertexPos3fTex2f( cosA * x1 + pos.X, y2 + pos.Y, sinA * x1 + pos.Z, nameTex.U1, nameTex.V2 );
			api.DrawDynamicVb( DrawMode.TriangleStrip, api.texVb, api.texVerts, VertexFormat.Pos3fTex2f, 4 );
			api.Texturing = false;
			api.AlphaTest = false;
		}
	}
}