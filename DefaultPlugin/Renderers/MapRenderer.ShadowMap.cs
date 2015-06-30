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
		Matrix4 biasedMvp;
		static Matrix4 bias = new Matrix4(
			0.5f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.5f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.5f, 0.0f,
			0.5f, 0.5f, 0.5f, 1.0f
		);
		
		void RenderShadowMap() {
			shadowShader.Bind();
			Matrix4 proj = Matrix4.CreatePerspectiveFieldOfView(
				(float)Math.PI / 2 - 0.01f, 1920f / 1080f, 0.1f, 800f );
			Matrix4 view = Matrix4.LookAt( new Vector3( 32, 32, 32 ), Vector3.Zero, Vector3.UnitY );
			mvp = view * proj;
			biasedMvp = mvp * bias;
			shadowShader.SetUniform( shadowShader.mvpLoc, ref mvp );
			
			shadowMap.BindForWriting( Window );
			api.FaceCulling = true;
			RenderSolidBatch();
			api.FaceCulling = false;
			RenderSpriteBatch();
			shadowMap.UnbindFromWriting( Window );
		}
		
		void BindForReadingShadowMap() {
			shader.SetUniform( shader.lightMVPLoc, ref biasedMvp );
			shader.SetUniform( shader.shadowImageLoc, 1 );
			shadowMap.BindForDepthReading( 1 );
		}
		
		void DisposeShadowMap() {
			shadowShader.Dispose();
			shadowMap.Dispose();
		}
	}
}