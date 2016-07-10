// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.Events;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Renderers {

	public sealed class SkyboxRenderer : IGameComponent {
		
		int tex, vb = -1;
		Game game;
		const int count = 6 * 4;
		
		public bool ShouldRender { 
			get { return tex > 0 && !(game.EnvRenderer is MinimalEnvRenderer); }
		}
		
		public void Init( Game game ) {
			this.game = game;
			game.Events.TextureChanged += TextureChanged;
			game.Events.TexturePackChanged += TexturePackChanged;
			game.WorldEvents.EnvVariableChanged += EnvVariableChanged;
			game.WorldEvents.OnNewMap += OnNewMap;
			MakeVb();
		}
		
		public void Reset( Game game ) {
			game.Graphics.DeleteTexture( ref tex );
		}

		public void Ready( Game game ) { }
		public void OnNewMap( Game game ) { }
		public void OnNewMapLoaded( Game game ) { }
		
		public void Dispose() {
			game.Graphics.DeleteTexture( ref tex );
			game.Graphics.DeleteVb( vb );
			game.Events.TextureChanged -= TextureChanged;
			game.Events.TexturePackChanged -= TexturePackChanged;
			game.WorldEvents.EnvVariableChanged -= EnvVariableChanged;
			game.WorldEvents.OnNewMap -= OnNewMap;
		}

		void OnNewMap( object sender, EventArgs e ) {
			game.Graphics.DeleteVb( vb );
			MakeVb();
		}
		
		void EnvVariableChanged( object sender, EnvVarEventArgs e ) {
			if( e.Var != EnvVar.CloudsColour ) return;
			game.Graphics.DeleteVb( vb );
			MakeVb();
		}
		
		void TexturePackChanged( object sender, EventArgs e ) {
			game.Graphics.DeleteTexture( ref tex );
		}
		
		void TextureChanged( object sender, TextureEventArgs e ) {
			if( e.Name == "skybox.png" )
				game.UpdateTexture( ref tex, e.Name, e.Data, false );
		}
		
		
		public void Render( double deltaTime ) {
			game.Graphics.DepthWrite = false;
			game.Graphics.Texturing = true;
			game.Graphics.BindTexture( tex );
			game.Graphics.SetBatchFormat( VertexFormat.P3fT2fC4b );
			
			Vector3 pos = game.CurrentCameraPos;
			Matrix4 m = Matrix4.Identity;
			Vector2 rotation = game.Camera.GetCameraOrientation();
			m *= Matrix4.RotateY( rotation.X ); // yaw
			m *= Matrix4.RotateX( rotation.Y ); // pitch
			m = m * game.Camera.tiltM;
			game.Graphics.LoadMatrix( ref m );
			
			game.Graphics.BindVb( vb );
			game.Graphics.DrawIndexedVb( DrawMode.Triangles, count * 6 / 4, 0 );
			
			game.Graphics.Texturing = false;
			game.Graphics.LoadMatrix( ref game.View );
			game.Graphics.DepthWrite = true;
		}
		
		unsafe void MakeVb() {
			VertexP3fT2fC4b* vertices = stackalloc VertexP3fT2fC4b[count];
			IntPtr start = (IntPtr)vertices;
			const float pos = 0.5f;
			TextureRec rec;
			int col = game.World.Env.CloudsCol.Pack();
			
			// Render the front quad
			rec = new TextureRec( 1/4f, 1/2f, 1/4f, 1/2f );
			*vertices = new VertexP3fT2fC4b(  pos, -pos, -pos, rec.U1, rec.V2, col ); vertices++;
			*vertices = new VertexP3fT2fC4b( -pos, -pos, -pos, rec.U2, rec.V2, col ); vertices++;
			*vertices = new VertexP3fT2fC4b( -pos,  pos, -pos, rec.U2, rec.V1, col ); vertices++;
			*vertices = new VertexP3fT2fC4b(  pos,  pos, -pos, rec.U1, rec.V1, col ); vertices++;
			// Render the left quad
			rec = new TextureRec( 0/4f, 1/2f, 1/4f, 1/2f );
			*vertices = new VertexP3fT2fC4b(  pos, -pos,  pos, rec.U1, rec.V2, col ); vertices++;
			*vertices = new VertexP3fT2fC4b(  pos, -pos, -pos, rec.U2, rec.V2, col ); vertices++;
			*vertices = new VertexP3fT2fC4b(  pos,  pos, -pos, rec.U2, rec.V1, col ); vertices++;
			*vertices = new VertexP3fT2fC4b(  pos,  pos,  pos, rec.U1, rec.V1, col ); vertices++;
			// Render the back quad
			rec = new TextureRec( 3/4f, 1/2f, 1/4f, 1/2f );
			*vertices = new VertexP3fT2fC4b( -pos, -pos,  pos, rec.U1, rec.V2, col ); vertices++;
			*vertices = new VertexP3fT2fC4b(  pos, -pos,  pos, rec.U2, rec.V2, col ); vertices++;
			*vertices = new VertexP3fT2fC4b(  pos,  pos,  pos, rec.U2, rec.V1, col ); vertices++;
			*vertices = new VertexP3fT2fC4b( -pos,  pos,  pos, rec.U1, rec.V1, col ); vertices++;
			// Render the right quad
			rec = new TextureRec( 2/4f, 1/2f, 1/4f, 1/2f );
			*vertices = new VertexP3fT2fC4b( -pos, -pos, -pos, rec.U1, rec.V2, col ); vertices++;
			*vertices = new VertexP3fT2fC4b( -pos, -pos,  pos, rec.U2, rec.V2, col ); vertices++;
			*vertices = new VertexP3fT2fC4b( -pos,  pos,  pos, rec.U2, rec.V1, col ); vertices++;
			*vertices = new VertexP3fT2fC4b( -pos,  pos, -pos, rec.U1, rec.V1, col ); vertices++;
			// Render the top quad
			rec = new TextureRec( 1/4f, 0/2f, 1/4f, 1/2f );
			*vertices = new VertexP3fT2fC4b( -pos,  pos, -pos, rec.U2, rec.V2, col ); vertices++;
			*vertices = new VertexP3fT2fC4b( -pos,  pos,  pos, rec.U2, rec.V1, col ); vertices++;
			*vertices = new VertexP3fT2fC4b(  pos,  pos,  pos, rec.U1, rec.V1, col ); vertices++;
			*vertices = new VertexP3fT2fC4b(  pos,  pos, -pos, rec.U1, rec.V2, col ); vertices++;
			// Render the bottom quad
			rec = new TextureRec( 2/4f, 0/2f, 1/4f, 1/2f );
			*vertices = new VertexP3fT2fC4b( -pos, -pos, -pos, rec.U2, rec.V2, col ); vertices++;
			*vertices = new VertexP3fT2fC4b( -pos, -pos,  pos, rec.U2, rec.V1, col ); vertices++;
			*vertices = new VertexP3fT2fC4b(  pos, -pos,  pos, rec.U1, rec.V1, col ); vertices++;
			*vertices = new VertexP3fT2fC4b(  pos, -pos, -pos, rec.U1, rec.V2, col ); vertices++;
			vb = game.Graphics.CreateVb( start, VertexFormat.P3fT2fC4b, count );			
		}
	}
}