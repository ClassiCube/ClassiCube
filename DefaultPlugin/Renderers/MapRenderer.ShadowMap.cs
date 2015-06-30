using System;
using ClassicalSharp;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace DefaultPlugin {

	public sealed partial class StandardMapRenderer : MapRenderer {
		
		MapShadowPassShader shadowShader;
		Framebuffer shadowMap;
		
		void InitShadowMap() {
			shadowShader = new MapShadowPassShader();
			shadowShader.Init( api );
			shadowMap = new Framebuffer( 1920, 1080 );
			shadowMap.Initalise( false, true, true );
		}
		
		Matrix4 mvp;
		void RenderShadowMap() {
			shadowShader.Bind();
			
			const float near = 1.0f, far = 7.5f;
			Matrix4 ortho = Matrix4.CreateOrthographicOffCenter(
				-100f, 100f, -100f, 100f, near, far);
			Matrix4 view = Matrix4.LookAt( Vector3.Zero, new Vector3( 32, 32, 32 ), Vector3.UnitY );
			mvp = view * ortho;
			shadowShader.SetUniform( shadowShader.mvpLoc, ref mvp );
			
			shadowMap.BindForWriting( Window );
			api.FaceCulling = true;
			RenderSolidBatch();
			api.FaceCulling = false;
			RenderSpriteBatch();
			shadowMap.UnbindFromWriting( Window );
		}
		
		void BindForReadingShadowMap() {
			shader.SetUniform( shader.lightMVPLoc, ref mvp );
			shader.SetUniform( shader.shadowImageLoc, 1 );
			shadowMap.BindForDepthReading( 1 );
		}
		
		void DisposeShadowMap() {
			shadowShader.Dispose();
			shadowMap.Dispose();
		}
	}
}