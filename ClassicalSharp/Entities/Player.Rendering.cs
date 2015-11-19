using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp {

	partial class Player {

		protected Texture nameTex;
		protected internal int PlayerTextureId = -1, MobTextureId = -1;
		
		public override void Despawn() {
			game.Graphics.DeleteTexture( ref PlayerTextureId );
			game.Graphics.DeleteTexture( ref nameTex.ID );
		}
		
		protected void InitRenderingData() {
			using( Font font = new Font( "Arial", 20 ) ) {
				DrawTextArgs args = new DrawTextArgs( DisplayName, font, true );
				nameTex = game.Drawer2D.MakeBitmappedTextTexture( ref args, 0, 0 );
			}
		}
		
		public void UpdateNameFont() {
			game.Graphics.DeleteTexture( ref nameTex );
			InitRenderingData();
		}
		
		protected void DrawName() {
			IGraphicsApi api = game.Graphics;
			api.BindTexture( nameTex.ID );
			
			float x1 = -nameTex.Width * 0.5f / 70f, y1 = nameTex.Height / 70f;
			float x2 = nameTex.Width * 0.5f / 70f, y2 = 0;
			// NOTE: Do this instead with network player's yaw to have names rotate with them instead.
			//yaw = Math.Pi - Player.YawRadians;
			float angle = game.LocalPlayer.YawRadians;
			float cosA = (float)Math.Cos( angle ), sinA = (float)Math.Sin( angle );
			Vector3 pos = Position;
			pos.Y += Model.NameYOffset;
			
			float u1 = nameTex.U1, u2 = nameTex.U2;
			if( game.Camera is ForwardThirdPersonCamera ) {
				u1 = nameTex.U2; u2 = nameTex.U1;
			}
			
			FastColour col = FastColour.White;
			api.texVerts[0] = new VertexPos3fTex2fCol4b( Utils.RotateY( x1, y1, 0, cosA, sinA ) + pos, u1, nameTex.V1, col );
			api.texVerts[1] = new VertexPos3fTex2fCol4b( Utils.RotateY( x2, y1, 0, cosA, sinA ) + pos, u2, nameTex.V1, col );
			api.texVerts[2] = new VertexPos3fTex2fCol4b( Utils.RotateY( x2, y2, 0, cosA, sinA ) + pos, u2, nameTex.V2, col );	
			api.texVerts[3] = new VertexPos3fTex2fCol4b( Utils.RotateY( x1, y2, 0, cosA, sinA ) + pos, u1, nameTex.V2, col );
			
			api.SetBatchFormat( VertexFormat.Pos3fTex2fCol4b );
			api.UpdateDynamicIndexedVb( DrawMode.Triangles, api.texVb, api.texVerts, 4, 6 );
		}
	}
}