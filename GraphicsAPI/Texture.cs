using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using ClassicalSharp.GraphicsAPI;
using OpenTK.Graphics.OpenGL;
using GlPixelFormat = OpenTK.Graphics.OpenGL.PixelFormat;

namespace ClassicalSharp {
	
	public class Texture {
		
		public static Texture Empty {
			get { return new Texture(); }
		}
		
		public int ID;
		public int X1 = 0, Y1 = 0;
		public int TexWidth, TexHeight;
		public int Width, Height;
		public float U1, V1;
		// Normally just used for graphics cards that don't support non power of two textures.
		public float U2, V2;
		
		private Texture() {
		}
		
		public Texture( int width, int height, IntPtr scan0 ) {
			Create( width, height, scan0 );
		}
		
		public Texture( string path ) {
			if( !File.Exists( path ) ) {
				throw new FileNotFoundException( path + " not found" );
			}
			using( Bitmap bmp = new Bitmap( path ) ) {
				Rectangle rec = new Rectangle( 0, 0, bmp.Width, bmp.Height );
				BitmapData data = bmp.LockBits( rec, ImageLockMode.ReadOnly, bmp.PixelFormat );
				Create( data.Width, data.Height, data.Scan0 );
				bmp.UnlockBits( data );
			}
		}
		
		public Texture( Bitmap bmp ) {
			Rectangle rec = new Rectangle( 0, 0, bmp.Width, bmp.Height );
			BitmapData data = bmp.LockBits( rec, ImageLockMode.ReadOnly, bmp.PixelFormat );
			Create( data.Width, data.Height, data.Scan0 );
			bmp.UnlockBits( data );
		}
		
		public Texture( FastBitmap bmp ) {
			if( !bmp.IsLocked )
				bmp.LockBits();
			Create( bmp.Width, bmp.Height, bmp.Scan0 );
			bmp.UnlockBits();
		}
		
		unsafe void Create( int width, int height, IntPtr scan0 ) {
			if( !Utils.IsPowerOf2( width ) || !Utils.IsPowerOf2( height ) )
				Utils.LogWarning( "Creating a non power of two texture." );
			
			int texId = 0;
			GL.GenTextures( 1, &texId );
			GL.BindTexture( TextureTarget.Texture2D, texId );
			GL.TexParameter( TextureTarget.Texture2D, TextureParameterName.TextureMinFilter, (int)TextureMinFilter.Nearest );
			GL.TexParameter( TextureTarget.Texture2D, TextureParameterName.TextureMagFilter, (int)TextureMagFilter.Nearest );

			GL.TexImage2D( TextureTarget.Texture2D, 0, PixelInternalFormat.Rgba, width, height, 0,
			              GlPixelFormat.Bgra, PixelType.UnsignedByte, scan0 );
			
			ID = texId;
			TexWidth = width;
			TexHeight = height;
			U2 = 1;
			V2 = 1;
		}
		
		public void SetUsedSize( int width, int height, bool updateUV ) {
			Width = width;
			Height = height;
			if( updateUV ) {
				U2 = (float)width / TexWidth;
				V2 = (float)height / TexHeight;
			}
		}
		
		public void SetGuiLocation( int x, int y ) {
			X1 = x;
			Y1 = y;
		}
		
		public void SetTexData( TextureRectangle rec ) {
			SetTexData( rec.U1, rec.V1, rec.U2, rec.V2 );
		}
		
		public void SetTexData( float u1, float v1, float u2, float v2 ) {
			U1 = u1;
			V1 = v1;
			U2 = u2;
			V2 = v2;
		}
		
		public bool IsNotZeroTexture {
			get { return ID > 0; }
		}
		
		public bool IsValid {
			get { return GL.IsTexture( ID ); }
		}
		
		public void Bind() {
			GL.BindTexture( TextureTarget.Texture2D, ID );
		}
		
		public void Render( OpenGLApi graphics ) {
			GL.BindTexture( TextureTarget.Texture2D, ID );
			graphics.Draw2DTexture( this );
		}
		
		public void RenderNoBind( OpenGLApi graphics ) {
			graphics.Draw2DTexture( this );
		}
		
		public unsafe void Delete() {
			if( ID <= 0 ) return;
			int id = ID;
			GL.DeleteTextures( 1, &id );
			ID = -1;
		}
		
		public override string ToString() {
			return ID + String.Format( "({0}, {1} -> {2},{3}", X1, Y1, Width, Height );
		}
		
		public void UpdatePart( int x, int y, FastBitmap part ) {
			GL.BindTexture( TextureTarget.Texture2D, ID );
			GL.TexSubImage2D( TextureTarget.Texture2D, 0, x, y, part.Width, part.Height,
			                 GlPixelFormat.Bgra, PixelType.UnsignedByte, part.Scan0 );
		}
	}
}
