using System;
using OpenTK;
using OpenTK.Graphics;

namespace ClassicalSharp.GraphicsAPI {

	public sealed class PostProcessor : IDisposable {
		
		Game window;
		OpenGLApi api;
		internal Framebuffer framebuffer;
		internal PostProcessingShader shader;
		public bool ApplyToGui = false;
		
		public PostProcessor( Game game ) {
			window = game;
			api = game.Graphics;
		}
		
		public void Init() {
			framebuffer = new Framebuffer( window.Width, window.Height );
			framebuffer.Initalise( true, true, false );
			window.Resize += Resize;
		}
		
		public void SetShader( PostProcessingShader shader ) {
			if( this.shader != null ) {
				this.shader.Dispose();
			}
			this.shader = shader;
			if( shader.ProgramId <= 0 ) {
				shader.Init( api );
			}
		}

		void Resize( object sender, EventArgs e ) {
			framebuffer.Dispose();
			framebuffer = new Framebuffer( window.Width, window.Height );
			framebuffer.Initalise( true, true, false );
			shader.RenderTargetResized( window.Width, window.Height );
		}
		
		public void Dispose() {
			framebuffer.Dispose();
			shader.Dispose();
			window.Resize -= Resize;
		}
		
		public void Apply() {
			int width = framebuffer.Width;
			int height = framebuffer.Height;
			shader.Bind();
			Matrix4 matrix = Matrix4.CreateOrthographicOffCenter( 0, width, height, 0, 0, 1 );
			shader.SetUniform( shader.projLoc, ref matrix );
			api.DepthTest = false;
			api.AlphaBlending = true;
			
			framebuffer.BindForColourReading( 0 );
			api.quads[0] = new VertexPos3fTex2fCol4b( width, 0, 0, 1, 1, FastColour.White );
			api.quads[1] = new VertexPos3fTex2fCol4b( width, height, 0, 1, 0, FastColour.White );
			api.quads[2] = new VertexPos3fTex2fCol4b( 0, 0, 0, 0, 1, FastColour.White );
			api.quads[3] = new VertexPos3fTex2fCol4b( 0, height, 0, 0, 0, FastColour.White );
			shader.DrawDynamic( DrawMode.TriangleStrip, VertexPos3fTex2fCol4b.Size, api.quadVb, api.quads, 4 );
			OpenTK.Graphics.OpenGL.GL.FrontFace( OpenTK.Graphics.OpenGL.FrontFaceDirection.Ccw );
		}
	}
}
