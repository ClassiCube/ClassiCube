using System;

namespace OpenTK.Graphics.OpenGL {

	[AttributeUsage( AttributeTargets.Struct | AttributeTargets.Class )]
    public sealed class InteropPatchAttribute : Attribute {
			
    }
	
	[InteropPatch]
	unsafe partial class GL {
		
		public static void AlphaFunc( AlphaFunction func, float value ) {
			Interop.Calli( (int)func, value, AlphaFuncAddress );
		} static IntPtr AlphaFuncAddress;

		public static void BindBuffer( BufferTarget target, int buffer ) {
			Interop.Calli( (int)target, buffer, BindBufferAddress );
		} static IntPtr BindBufferAddress;
		
		public static void BindBufferARB( BufferTarget target, int buffer ) {
			Interop.Calli( (int)target, buffer, BindBufferARBAddress );
		} static IntPtr BindBufferARBAddress;

		public static void BindTexture( TextureTarget target, int texture ) {
			Interop.Calli( (int)target, texture, BindTextureAddress );
		} static IntPtr BindTextureAddress;

		public static void BlendFunc( BlendingFactor sfactor, BlendingFactor dfactor ) {
			Interop.Calli( (int)sfactor, (int)dfactor, BlendFuncAddress );
		} static IntPtr BlendFuncAddress;
		
		public static void BufferData( BufferTarget target, IntPtr size, IntPtr data, BufferUsageHint usage ) {
			Interop.Calli( (int)target, size, data, (int)usage, BufferDataAddress );
		} static IntPtr BufferDataAddress;
		
		public static void BufferDataARB( BufferTarget target, IntPtr size, IntPtr data, BufferUsageHint usage ) {
			Interop.Calli( (int)target, size, data, (int)usage, BufferDataARBAddress );
		} static IntPtr BufferDataARBAddress;
		
		public static void BufferSubData( BufferTarget target, IntPtr offset, IntPtr size, IntPtr data ) {
			Interop.Calli( (int)target, offset, size, data, BufferSubDataAddress );
		} static IntPtr BufferSubDataAddress;
		
		public static void BufferSubDataARB( BufferTarget target, IntPtr offset, IntPtr size, IntPtr data ) {
			Interop.Calli( (int)target, offset, size, data, BufferSubDataARBAddress );
		} static IntPtr BufferSubDataARBAddress;

		public static void Clear( ClearBufferMask mask ) {
			Interop.Calli( (int)mask, ClearAddress );
		} static IntPtr ClearAddress;

		public static void ClearColor( float red, float green, float blue, float alpha ) {
			Interop.Calli( red, green, blue, alpha, ClearColorAddress );
		} static IntPtr ClearColorAddress;

		public static void ColorMask( bool red, bool green, bool blue, bool alpha ) {
			Interop.Calli( red ? (byte)1 : (byte)0, green ? (byte)1 : (byte)0, blue ? (byte)1 : (byte)0, alpha ? (byte)1 : (byte)0, ColorMaskAddress );
		} static IntPtr ColorMaskAddress;

		public static void ColorPointer( int size, PointerType type, int stride, IntPtr pointer ) {
			Interop.Calli( size, (int)type, stride, pointer, ColorPointerAddress );
		} static IntPtr ColorPointerAddress;

		public static void CullFace( CullFaceMode mode ) {
			Interop.Calli( (int)mode, CullFaceAddress );
		} static IntPtr CullFaceAddress;

		public static void DeleteBuffers( int n, int* buffers ) {
			Interop.Calli( n, buffers, DeleteBuffersAddress );
		} static IntPtr DeleteBuffersAddress;
		
		public static void DeleteBuffersARB( int n, int* buffers ) {
			Interop.Calli( n, buffers, DeleteBuffersARBAddress );
		} static IntPtr DeleteBuffersARBAddress;

		public static void DeleteTextures( int n, int* textures ) {
			Interop.Calli( n, textures, DeleteTexturesAddress );
		} static IntPtr DeleteTexturesAddress;
		
		public static void DepthFunc( DepthFunction func ) {
			Interop.Calli( (int)func, DepthFuncAddress );
		} static IntPtr DepthFuncAddress;
		
		public static void DepthMask( bool flag ) {
			Interop.Calli( flag ? (byte)1 : (byte)0, DepthMaskAddress );
		} static IntPtr DepthMaskAddress;

		public static void Disable( EnableCap cap ) {
			Interop.Calli( (int)cap, DisableAddress );
		} static IntPtr DisableAddress;
		
		public static void DisableClientState( ArrayCap cap ) {
			Interop.Calli( (int)cap, DisableClientStateAddress );
		} static IntPtr DisableClientStateAddress;
		
		public static void DrawArrays( BeginMode mode, int first, int count ) {
			Interop.Calli( (int)mode, first, count, DrawArraysAddress );
		} static IntPtr DrawArraysAddress;
		
		public static void DrawElements( BeginMode mode, int count, DrawElementsType type, IntPtr indices ) {
			Interop.Calli( (int)mode, count, (int)type, indices, DrawElementsAddress );
		} static IntPtr DrawElementsAddress;

		public static void Enable( EnableCap cap ) {
			Interop.Calli( (int)cap, EnableAddress );
		} static IntPtr EnableAddress;
		
		public static void EnableClientState( ArrayCap cap ) {
			Interop.Calli( (int)cap, EnableClientStateAddress );
		} static IntPtr EnableClientStateAddress;

		public static void Fogf( FogParameter pname, float param ) {
			Interop.Calli( (int)pname, param, FogfAddress );
		} static IntPtr FogfAddress;

		public static void Fogfv( FogParameter pname, float* param ) {
			Interop.Calli( (int)pname, param, FogfvAddress );
		} static IntPtr FogfvAddress;
		
		public static void Fogi( FogParameter pname, int param ) {
			Interop.Calli( (int)pname, param, FogiAddress );
		} static IntPtr FogiAddress;
		
		public static void GenBuffers( int n, int* buffers ) {
			Interop.Calli( n, buffers, GenBuffersAddress );
		} static IntPtr GenBuffersAddress;
		
		public static void GenBuffersARB( int n, int* buffers ) {
			Interop.Calli( n, buffers, GenBuffersARBAddress );
		} static IntPtr GenBuffersARBAddress;
		
		public static void GenTextures( int n, int* buffers ) {
			Interop.Calli( n, buffers, GenTexturesAddress );
		} static IntPtr GenTexturesAddress;
		
		public static ErrorCode GetError() {
			return (ErrorCode)Interop.Calli_Int32( GetErrorAddress );
		} static IntPtr GetErrorAddress;

		public static void GetFloatv( GetPName pname, float* @params ) {
			Interop.Calli( (int)pname, @params, GetFloatvAddress );
		} static IntPtr GetFloatvAddress;

		public static void GetIntegerv( GetPName pname, int* @params ) {
			Interop.Calli( (int)pname,  @params, GetIntegervAddress );
		} static IntPtr GetIntegervAddress;
		
		public static IntPtr GetString( StringName name ) {
			return Interop.Calli_IntPtr( (int)name, GetStringAddress );
		} static IntPtr GetStringAddress;

		public static void Hint( HintTarget target, HintMode mode ) {
			Interop.Calli( (int)target, (int)mode, HintAddress );
		} static IntPtr HintAddress;
		
		public static bool IsBuffer( int buffer ) {
			return Interop.Calli_UInt8( buffer, IsBufferAddress ) != 0;
		} static IntPtr IsBufferAddress;
		
		public static bool IsBufferARB( int buffer ) {
			return Interop.Calli_UInt8( buffer, IsBufferARBAddress ) != 0;
		} static IntPtr IsBufferARBAddress;

		public static bool IsTexture( int texture ) {
			return Interop.Calli_UInt8( texture, IsTextureAddress ) != 0;
		} static IntPtr IsTextureAddress;
		
		public static void LoadIdentity() {
			Interop.Calli( LoadIdentityAddress );
		} static IntPtr LoadIdentityAddress;

		public static void LoadMatrixf( float* m ) {
			Interop.Calli( m, LoadMatrixfAddress );
		} static IntPtr LoadMatrixfAddress;

		public static void MatrixMode( OpenGL.MatrixMode mode ) {
			Interop.Calli( (int)mode, MatrixModeAddress );
		} static IntPtr MatrixModeAddress;
		
		public static void MultMatrixf( float* m ) {
			Interop.Calli( m, MultMatrixfAddress );
		} static IntPtr MultMatrixfAddress;

		public static void PopMatrix() {
			Interop.Calli( PopMatrixAddress );
		} static IntPtr PopMatrixAddress;
		
		public static void PushMatrix() {
			Interop.Calli( PushMatrixAddress );
		} static IntPtr PushMatrixAddress;
		
		public static void ReadPixels( int x, int y, int width, int height, PixelFormat format, PixelType type, IntPtr pixels ) {
			Interop.Calli( x, y, width, height, (int)format, (int)type, pixels, ReadPixelsAddress );
		} static IntPtr ReadPixelsAddress;
		
		public static void ShadeModel( ShadingModel mode ) {
			Interop.Calli( (int)mode, ShadeModelAddress );
		} static IntPtr ShadeModelAddress;
		
		public static void TexCoordPointer( int size, PointerType type, int stride, IntPtr pointer ) {
			Interop.Calli( size, (int)type, stride, pointer, TexCoordPointerAddress );
		} static IntPtr TexCoordPointerAddress;

		public static void TexImage2D( TextureTarget target, int level, PixelInternalFormat publicformat, 
		                              int width, int height, PixelFormat format, PixelType type, IntPtr pixels ) {
			Interop.Calli( (int)target, level, (int)publicformat, width, height, 0, (int)format, (int)type, pixels, TexImage2DAddress );
		} static IntPtr TexImage2DAddress;
		
		public static void TexParameteri( TextureTarget target, TextureParameterName pname, int param ) {
			Interop.Calli( (int)target, (int)pname, param, TexParameteriAddress );
		} static IntPtr TexParameteriAddress;
		
		public static void TexSubImage2D( TextureTarget target, int level, int xoffset, int yoffset, 
		                                   int width, int height, PixelFormat format, PixelType type, IntPtr pixels) {
			Interop.Calli( (int)target, level, xoffset, yoffset, width, height, (int)format, (int)type, pixels, TexSubImage2DAddress );
		} static IntPtr TexSubImage2DAddress;
		
		public static void VertexPointer( int size, PointerType type, int stride, IntPtr pointer ) {
			Interop.Calli( size, (int)type, stride, pointer, VertexPointerAddress );
		} static IntPtr VertexPointerAddress;

		public static void Viewport( int x, int y, int width, int height ) {
			Interop.Calli( x, y, width, height, ViewportAddress );
		} static IntPtr ViewportAddress;
	}
}