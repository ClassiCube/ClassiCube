using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using OpenTK.Graphics.OpenGL;
using GlPixelFormat = OpenTK.Graphics.OpenGL.PixelFormat;

namespace ClassicalSharp.GraphicsAPI {
	
	public struct TextureObj {
		
		public static readonly TextureObj Empty = default( TextureObj );
		
		public int TextureId;
		
		public static TextureObj LoadTexture( string path ) {
			if( !File.Exists( path ) ) {
				throw new FileNotFoundException( path + " not found" );
			}
			using( Bitmap bmp = new Bitmap( path ) ) {
				return new TextureObj( bmp );
			}
		}
		
		public TextureObj( Bitmap bmp ) {
			TextureId = 0;
			Rectangle rec = new Rectangle( 0, 0, bmp.Width, bmp.Height );
			BitmapData data = bmp.LockBits( rec, ImageLockMode.ReadOnly, bmp.PixelFormat );
			Create( bmp.Width, bmp.Height, data.Scan0 );
			bmp.UnlockBits( data );
		}
		
		public TextureObj( FastBitmap bmp ) {
			TextureId = 0;
			if( !bmp.IsLocked )
				bmp.LockBits();
			Create( bmp.Width, bmp.Height, bmp.Scan0 );
			bmp.UnlockBits();
		}
		
		public TextureObj( int width, int height, IntPtr scan0 ) {
			TextureId = 0;
			Create( width, height, scan0 );
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
			TextureId = texId;
		}
		
		public void Bind() {
			GL.BindTexture( TextureTarget.Texture2D, TextureId );
		}
		
		public void Bind( int unit ) {
			GL.ActiveTexture( (TextureUnit)( TextureUnit.Texture0 + unit ) );
			GL.BindTexture( TextureTarget.Texture2D, TextureId );
			GL.ActiveTexture( TextureUnit.Texture0 );
		}
		
		public void UpdatePart( int x, int y, FastBitmap part ) {
			GL.BindTexture( TextureTarget.Texture2D, TextureId );
			GL.TexSubImage2D( TextureTarget.Texture2D, 0, x, y, part.Width, part.Height,
			                 GlPixelFormat.Bgra, PixelType.UnsignedByte, part.Scan0 );
		}
		
		public unsafe void Delete() {
			if( TextureId <= 0 ) return;
			int texId = TextureId;
			GL.DeleteTextures( 1, &texId );
			TextureId = -1;
		}
		
		public bool IsInvalid {
			get { return TextureId <= 0; }
		}
	}
}
