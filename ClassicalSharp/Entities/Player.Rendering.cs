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
			api = game.Graphics;
			
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
			api.BindTexture( nameTex.ID );
			
			float x1 = -nameTex.Width * 0.5f / 50f, y1 = nameTex.Height / 50f;
			float x2 = nameTex.Width * 0.5f / 50f, y2 = 0;
			// NOTE: Do this instead with network player's yaw to have names rotate with them instead.
			//yaw = Math.Pi - Player.YawRadians;
			float angle = game.LocalPlayer.YawRadians;
			float cosA = (float)Math.Cos( angle ), sinA = (float)Math.Sin( angle );
			Vector3 pos = Position;
			pos.Y += Model.NameYOffset;
			
			api.texVerts[0] = new VertexPos3fTex2f( Utils.RotateY( x1, y1, 0, cosA, sinA ) + pos, nameTex.U1, nameTex.V1 );
			api.texVerts[1] = new VertexPos3fTex2f( Utils.RotateY( x2, y1, 0, cosA, sinA ) + pos, nameTex.U2, nameTex.V1 );
			api.texVerts[2] = new VertexPos3fTex2f( Utils.RotateY( x2, y2, 0, cosA, sinA ) + pos, nameTex.U2, nameTex.V2 );		
			api.texVerts[3] = new VertexPos3fTex2f( Utils.RotateY( x1, y2, 0, cosA, sinA ) + pos, nameTex.U1, nameTex.V2 );
			
			api.BeginVbBatch( VertexFormat.Pos3fTex2f );
			api.DrawDynamicIndexedVb( DrawMode.Triangles, api.texVb, api.texVerts, 4, 6 );
			api.Texturing = false;
			api.AlphaTest = false;
		}
	}
}