using System;
using OpenTK.Graphics.OpenGL;

namespace ClassicalSharp.GraphicsAPI {
	
	public sealed class Framebuffer : IDisposable  {
		
		public int Width, Height;
		public int FboId;
		public int ColourTexId, DepthTexId;
		bool depthOnly;
		ClearBufferMask mask = 0;
		
		public Framebuffer( int width, int height ) {
			Width = width;
			Height = height;
		}
		
		public unsafe void Initalise( bool attachColour, bool attachDepth, bool depthOnly ) {
			int fboId;
			GL.GenFramebuffers( 1, &fboId );
			FboId = fboId;
			GL.BindFramebuffer( FramebufferTarget.Framebuffer, FboId );
			
			if( attachColour ) {
				ColourTexId = AttachTexture( FramebufferAttachment.ColorAttachment0,
				                            PixelInternalFormat.Rgba, PixelFormat.Bgra );
				mask |= ClearBufferMask.ColorBufferBit;
			}
			if( attachDepth ) {
				DepthTexId = AttachTexture( FramebufferAttachment.DepthAttachment,
				                           PixelInternalFormat.DepthComponent, PixelFormat.DepthComponent );
				mask |= ClearBufferMask.DepthBufferBit;
			}
			
			this.depthOnly = depthOnly;
			GL.DrawBuffer( depthOnly ? DrawBufferMode.None : DrawBufferMode.ColorAttachment0 );
			GL.ReadBuffer( ReadBufferMode.None );
			
			FramebufferErrorCode status = GL.CheckFramebufferStatus( FramebufferTarget.Framebuffer );
			if( status != FramebufferErrorCode.FramebufferComplete ) {
				throw new InvalidOperationException( "FBO exception: " + status );
			}
			GL.BindFramebuffer( FramebufferTarget.Framebuffer, 0 );
		}
		
		unsafe int AttachTexture( FramebufferAttachment attachment, PixelInternalFormat inFormat, PixelFormat format ) {
			int texId;
			GL.GenTextures( 1, &texId );
			GL.BindTexture( TextureTarget.Texture2D, texId );
			GL.TexParameter( TextureTarget.Texture2D, TextureParameterName.TextureMinFilter, (int)TextureMinFilter.Nearest );
			GL.TexParameter( TextureTarget.Texture2D, TextureParameterName.TextureMagFilter, (int)TextureMagFilter.Nearest );
			GL.TexParameter( TextureTarget.Texture2D, TextureParameterName.TextureWrapS, (int)TextureWrapMode.ClampToEdge );
			GL.TexParameter( TextureTarget.Texture2D, TextureParameterName.TextureWrapT, (int)TextureWrapMode.ClampToEdge );
			
			GL.TexImage2D( TextureTarget.Texture2D, 0, inFormat, Width, Height, 0, format, PixelType.UnsignedByte, IntPtr.Zero );
			GL.FramebufferTexture2D( FramebufferTarget.Framebuffer, attachment, TextureTarget.Texture2D, texId, 0 );
			return texId;
		}
		
		public void BindForColourReading( int unit ) {
			GL.ActiveTexture( (TextureUnit)( TextureUnit.Texture0 + unit ) );
			GL.BindTexture( TextureTarget.Texture2D, ColourTexId );
			GL.ActiveTexture( TextureUnit.Texture0 );
		}
		
		public void BindForDepthReading( int unit ) {
			GL.ActiveTexture( (TextureUnit)( TextureUnit.Texture0 + unit ) );
			GL.BindTexture( TextureTarget.Texture2D, DepthTexId );
			GL.ActiveTexture( TextureUnit.Texture0 );
		}

		public void BindForWriting( Game game ) {
			GL.BindFramebuffer( FramebufferTarget.Framebuffer, FboId );
			if( depthOnly ) { // TODO: is this necessary?
				GL.ColorMask( false, false, false, false );
			}
			GL.Viewport( 0, 0, Width, Height );
			GL.Clear( mask );
		}
		
		public void UnbindFromWriting( Game game ) {
			GL.BindFramebuffer( FramebufferTarget.Framebuffer, 0 );
			if( depthOnly ) {
				GL.ColorMask( true, true, true, true );
			}
			GL.Viewport( 0, 0, game.Width, game.Height );
		}
		
		public unsafe void Dispose() {
			int fbid = FboId;
			GL.DeleteFramebuffers( 1, &fbid );
			if( ColourTexId > 0 ) {
				int texId = ColourTexId;
				GL.DeleteTextures( 1, &texId );
			}
			if( DepthTexId > 0 ) {
				int texId = DepthTexId;
				GL.DeleteTextures( 1, &texId );
			}
		}
	}
}
