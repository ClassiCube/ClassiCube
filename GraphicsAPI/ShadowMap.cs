using System;
using OpenTK.Graphics.OpenGL;
using OpenTK;
using OpenTK.Graphics;

namespace ClassicalSharp.GraphicsAPI {
	
	public abstract class ShadowMap {
		
		public Vector3 LightPosition;
		public Vector3 LightTarget;
		public Matrix4 ShadowMatrix;
		
		public abstract void Create( int width, int height );
		
		public abstract void BindForReading( IGraphicsApi graphics );
		
		public abstract void BindForWriting( IGraphicsApi graphics );
		
		public abstract void SetupState( IGraphicsApi graphics, Game game );
		
		public abstract void RestoreState( IGraphicsApi graphics, Game g );
		
		public abstract void Dispose();
	}
	
	public sealed class OpenGLShadowMap : ShadowMap {
		
		int depthTextureId, fboId;
		int fboWidth, fboHeight;
		public override void Create( int width, int height ) {
			GL.GenTextures( 1, out depthTextureId );
			GL.BindTexture( TextureTarget.Texture2D, depthTextureId );
			
			GL.TexParameter( TextureTarget.Texture2D, TextureParameterName.TextureMinFilter, (int)All.Nearest );
			GL.TexParameter( TextureTarget.Texture2D, TextureParameterName.TextureMagFilter, (int)All.Nearest );
			GL.TexParameter( TextureTarget.Texture2D, TextureParameterName.TextureWrapS, (int)All.Clamp );
			GL.TexParameter( TextureTarget.Texture2D, TextureParameterName.TextureWrapT, (int)All.Clamp );
			GL.TexImage2D( TextureTarget.Texture2D, 0, PixelInternalFormat.DepthComponent,
			              width, height, 0, PixelFormat.DepthComponent,
			              PixelType.UnsignedByte, IntPtr.Zero );
			GL.BindTexture( TextureTarget.Texture2D, 0);
			
			GL.Ext.GenFramebuffers( 1, out fboId );
			GL.Ext.BindFramebuffer( FramebufferTarget.Framebuffer, fboId );
			GL.DrawBuffer( DrawBufferMode.None );
			GL.ReadBuffer( (ReadBufferMode)DrawBufferMode.None );
			
			GL.Ext.FramebufferTexture2D( FramebufferTarget.Framebuffer, FramebufferAttachment.DepthAttachment,
			                            TextureTarget.Texture2D, depthTextureId, 0 );
			FramebufferErrorCode status = GL.Ext.CheckFramebufferStatus( FramebufferTarget.Framebuffer );
			if( status != FramebufferErrorCode.FramebufferComplete ) Console.WriteLine( "ERROR FBO: " + status );
			
			GL.Ext.BindFramebuffer( FramebufferTarget.Framebuffer, 0 );
			GL.TexParameter( TextureTarget.Texture2D, TextureParameterName.TextureWrapS, (int)All.Repeat );
			GL.TexParameter( TextureTarget.Texture2D, TextureParameterName.TextureWrapT, (int)All.Repeat );
			fboWidth = width;
			fboHeight = height;
		}
		
		public override void BindForReading( IGraphicsApi graphics ) {
			GL.Ext.BindFramebuffer( FramebufferTarget.Framebuffer, fboId );
		}
		
		public override void BindForWriting( IGraphicsApi graphics ) {
			GL.Arb.ActiveTexture( TextureUnit.Texture1 );
			GL.BindTexture( TextureTarget.Texture2D, depthTextureId );
			GL.Arb.ActiveTexture( TextureUnit.Texture0 );
		}
		
		public override void SetupState( IGraphicsApi graphics, Game game ) {
			graphics.SetMatrixMode( MatrixType.Projection );
			graphics.PushMatrix();
			Matrix4 projection = Matrix4.CreatePerspectiveFieldOfView(
				(float)Math.PI / 4, game.Width / (float)game.Height, 1, 40000 );
			graphics.LoadMatrix( ref projection );
			
			graphics.SetMatrixMode( MatrixType.Modelview );
			graphics.PushMatrix();
			Matrix4 modelview = Matrix4.LookAt( LightPosition, LightTarget, Vector3.UnitY );
			graphics.LoadMatrix( ref modelview );
			ShadowMatrix = modelview * projection * bias;
			graphics.ColourMask( false, false, false, false );
			GL.Viewport( 0, 0, fboWidth, fboHeight );
		}
		
		public override void RestoreState( IGraphicsApi graphics, Game game ) {
			graphics.SetMatrixMode( MatrixType.Projection );
			graphics.PopMatrix();
			graphics.SetMatrixMode( MatrixType.Modelview );
			graphics.PopMatrix();
			graphics.ColourMask( true, true, true, true );
			GL.Viewport( 0, 0, game.Width, game.Height );
		}
		static Matrix4 bias = new Matrix4(
			0.5f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.5f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.5f, 0.0f,
			0.5f, 0.5f, 0.5f, 1.0f
		);
		
		public override void Dispose() {
			GL.DeleteTexture( depthTextureId );
			GL.Ext.DeleteFramebuffers( 1, ref fboId );
		}
	}
}
