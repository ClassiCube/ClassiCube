using System;
using OpenTK.Graphics.OpenGL;
using OpenTK;
using OpenTK.Graphics;

namespace ClassicalSharp.GraphicsAPI {
	
	public class Framebuffer {
		
		public int Width, Height;		
		public int TexId, FboId;
		
		public Framebuffer( int width, int height ) {
			Width = width;
			Height = height;
		}
		
		public void Initalise() {
			GL.GenTextures( 1, out TexId );
			GL.BindTexture( TextureTarget.Texture2D, TexId );
			GL.TexParameter( TextureTarget.Texture2D, TextureParameterName.TextureWrapS, (int)TextureWrapMode.ClampToEdge );
			GL.TexParameter( TextureTarget.Texture2D, TextureParameterName.TextureWrapT, (int)TextureWrapMode.ClampToEdge );
			
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
		
		public static void BindFramebuffer( int fbo ) {
			GL.Ext.BindFramebuffer( FramebufferTarget.FramebufferExt, fbo );
		}
	}
}
