using System;
using OpenTK.Graphics.OpenGL;
using OpenTK;
using OpenTK.Graphics;

namespace ClassicalSharp.GraphicsAPI {
	
	public class ShadowMap {
		
		public int Width, Height;
		public int TexId, FboId;
		
		public ShadowMap( int width, int height ) {
			Width = width;
			Height = height;
		}
		
		public void Initalise() {
			GL.GenTextures( 1, out TexId );
			GL.BindTexture( TextureTarget.Texture2D, TexId );
			GL.TexParameter( TextureTarget.Texture2D, TextureParameterName.TextureMinFilter, (int)All.Linear );
			GL.TexParameter( TextureTarget.Texture2D, TextureParameterName.TextureMagFilter, (int)All.Linear );
			GL.TexParameter( TextureTarget.Texture2D, TextureParameterName.TextureWrapS, (int)TextureWrapMode.ClampToEdge );
			GL.TexParameter( TextureTarget.Texture2D, TextureParameterName.TextureWrapT, (int)TextureWrapMode.ClampToEdge );
			// Remove these and change sampler2dshadow to sampler2d if you want old shadow behaviour
			GL.TexParameter( TextureTarget.Texture2D, TextureParameterName.TextureCompareMode, (int)All.CompareRToTexture );
			GL.TexParameter( TextureTarget.Texture2D, TextureParameterName.TextureCompareFunc, (int)All.Lequal );
			GL.TexParameter( TextureTarget.Texture2D, TextureParameterName.DepthTextureMode, (int)All.Intensity );
			
			GL.TexImage2D( TextureTarget.Texture2D, 0, PixelInternalFormat.DepthComponent, Width, Height, 0,
			              PixelFormat.DepthComponent, PixelType.UnsignedByte, IntPtr.Zero );
			GL.Ext.GenFramebuffers( 1, out FboId );
			GL.Ext.BindFramebuffer( FramebufferTarget.FramebufferExt, FboId );
			
			GL.DrawBuffer( (DrawBufferMode)All.None );
			GL.ReadBuffer( (ReadBufferMode)All.None );
			GL.Ext.FramebufferTexture2D( FramebufferTarget.FramebufferExt, FramebufferAttachment.DepthAttachmentExt,
			                            TextureTarget.Texture2D, TexId, 0 );
			
			FramebufferErrorCode status = GL.Ext.CheckFramebufferStatus( FramebufferTarget.FramebufferExt );
			if( status != FramebufferErrorCode.FramebufferCompleteExt ) {
				throw new InvalidOperationException( "FBO exception: " + status );
			}
			GL.Ext.BindFramebuffer( FramebufferTarget.FramebufferExt, 0 );
		}
		
		public void BindForReading() {
			GL.Arb.ActiveTexture( TextureUnit.Texture1 );
			GL.BindTexture( TextureTarget.Texture2D, TexId );
			GL.Arb.ActiveTexture( TextureUnit.Texture0 );
		}
		
		public void BindForWriting() {
			GL.Ext.BindFramebuffer( FramebufferTarget.Framebuffer, FboId );
		}
		
		public void UnbindFromWriting() {
			GL.Ext.BindFramebuffer( FramebufferTarget.Framebuffer, 0 );
		}
		
		static Matrix4 bias = new Matrix4(
			0.5f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.5f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.5f, 0.0f,
			0.5f, 0.5f, 0.5f, 1.0f
		);
		
		public Matrix4 ShadowMatrix, BiasedShadowMatrix;
		public Vector3 LightPosition = new Vector3( 256, 160, 192 );
		public Vector3 LightTarget = new Vector3( 128, 128, 128 );

		public void SetupState( Game game ) {
			Matrix4 projection = Matrix4.CreatePerspectiveFieldOfView(
				(float)Math.PI / 2 - 0.01f, game.Width / (float)game.Height, 0.1f, 800f );
			
			Matrix4 modelview = Matrix4.LookAt( LightPosition, LightTarget, Vector3.UnitY );
			ShadowMatrix = modelview * projection;
			BiasedShadowMatrix = ShadowMatrix * bias;
			GL.ColorMask( false, false, false, false );
			GL.Viewport( 0, 0, Width, Height );
			GL.Clear( ClearBufferMask.DepthBufferBit );
		}
		
		public void RestoreState( Game game ) {
			GL.ColorMask( true, true, true, true );
			GL.Viewport( 0, 0, game.Width, game.Height );
		}
	}
}
