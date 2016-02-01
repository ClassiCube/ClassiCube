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
			using( Font font = new Font( game.FontName, 20 ) ) {
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
			Vector3 pos = Position; pos.Y += Model.NameYOffset;
			
			Vector3 p111, p121, p212, p222;
			FastColour col = FastColour.White;
			Vector2 size = new Vector2( nameTex.Width / 70f, nameTex.Height / 70f );
			Utils.CalcBillboardPoints( size, pos, ref game.View, out p111, out p121, out p212, out p222 );
			api.texVerts[0] = new VertexPos3fTex2fCol4b( p111, nameTex.U1, nameTex.V2, col );
			api.texVerts[1] = new VertexPos3fTex2fCol4b( p121, nameTex.U1, nameTex.V1, col );
			api.texVerts[2] = new VertexPos3fTex2fCol4b( p222, nameTex.U2, nameTex.V1, col );	
			api.texVerts[3] = new VertexPos3fTex2fCol4b( p212, nameTex.U2, nameTex.V2, col );
			
			api.SetBatchFormat( VertexFormat.Pos3fTex2fCol4b );
			api.UpdateDynamicIndexedVb( DrawMode.Triangles, api.texVb, api.texVerts, 4, 6 );
		}
	}
}