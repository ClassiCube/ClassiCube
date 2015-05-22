//#define INCLUDE_DIRECTX
// necessary dll references: (Managed DirectX)
// Microsoft.DirectX
// Microsoft.DirectX.Direct3D
// Microsoft.DirectX.Direct3DX
#if INCLUDE_DIRECTX
using System;
using System.Drawing;
using System.Runtime.InteropServices;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using D3D = Microsoft.DirectX.Direct3D;
using Matrix4 = OpenTK.Matrix4;

namespace ClassicalSharp.GraphicsAPI {

    public class DirectXApi : IGraphicsApi {

        public Device device;
        Caps caps;
        D3D.Texture[] textures;
        VertexBuffer[] buffers;
        const int texBufferSize = 512;
        const int vertexBufferSize = 2048;
        MatrixStack viewStack, projStack, texStack;
        MatrixStack curStack;
        PrimitiveType[] modeMappings = new PrimitiveType[] {
			PrimitiveType.TriangleList, PrimitiveType.LineList,
			PrimitiveType.PointList, PrimitiveType.TriangleStrip,
			PrimitiveType.LineStrip, PrimitiveType.TriangleFan,
		};

        public DirectXApi( Device device ) {
            this.device = device;
            caps = device.DeviceCaps;
            viewStack = new MatrixStack( 32, m => device.SetTransform( TransformType.View, m ) );
            projStack = new MatrixStack( 4, m => device.SetTransform( TransformType.Projection, m ) );
            texStack = new MatrixStack( 4, m => device.SetTransform( TransformType.Texture1, m ) ); // TODO: Texture0?
        }

        public override bool AlphaTest {
            set { device.RenderState.AlphaTestEnable = value; }
        }

        public override bool AlphaBlending {
            set { device.RenderState.AlphaBlendEnable = value; }
        }

        Compare[] compareFuncs = new Compare[] {
			Compare.Always, Compare.NotEqual, Compare.Never, Compare.Less,
			Compare.LessEqual, Compare.Equal, Compare.GreaterEqual, Compare.Greater,
		};
        public override void AlphaTestFunc( AlphaFunc func, float value ) {
            device.RenderState.AlphaFunction = compareFuncs[(int)func];
            device.RenderState.ReferenceAlpha = (int)( value * 255f );
        }

        BlendOperation[] blendEqs = new BlendOperation[] {
			BlendOperation.Add, BlendOperation.Max,
			BlendOperation.Min, BlendOperation.Subtract,
			BlendOperation.RevSubtract,
		};
        public override void AlphaBlendEq( BlendEquation eq ) {
            device.RenderState.BlendOperation = blendEqs[(int)eq];
        }

        Blend[] blendFuncs = new Blend[] {
			Blend.Zero, Blend.One,
			Blend.SourceAlpha, Blend.InvSourceAlpha,
			Blend.DestinationAlpha, Blend.InvDestinationAlpha,
		};
        public override void AlphaBlendFunc( BlendFunc srcFunc, BlendFunc destFunc ) {
            device.RenderState.SourceBlend = blendFuncs[(int)srcFunc];
            device.RenderState.DestinationBlend = blendFuncs[(int)destFunc];
        }

        public override bool Fog {
            set { device.RenderState.FogEnable = value; }
        }

        public override void SetFogColour( FastColour col ) {
            device.RenderState.FogColor = col.ToColor();
        }

        public override void SetFogDensity( float value ) {
            device.RenderState.FogDensity = value;
        }

        public override void SetFogEnd( float value ) {
            device.RenderState.FogEnd = value;
        }

        FogMode[] modes = new FogMode[] { FogMode.Linear, FogMode.Exp, FogMode.Exp2 };
        public override void SetFogMode( Fog mode ) {
            device.RenderState.FogTableMode = modes[(int)mode];
        }

        public override void SetFogStart( float value ) {
            device.RenderState.FogStart = value;
        }


        public override bool SupportsNonPowerOf2Textures {
            get {
                return !caps.TextureCaps.SupportsPower2 &&
                    !caps.TextureCaps.SupportsNonPower2Conditional;
            }
        }

        public override int MaxTextureDimensions {
            get {
                return Math.Min( caps.MaxTextureHeight, caps.MaxTextureWidth );
            }
        }

        public override int LoadTexture( FastBitmap bmp ) {
            return LoadTexture( bmp.Bitmap );
        }

        public override int LoadTexture( Bitmap bmp ) {
            D3D.Texture texture = D3D.Texture.FromBitmap( device, bmp, Usage.None, Pool.Managed );

            for( int i = 1; i < textures.Length; i++ ) {
                if( textures[i] == null ) {
                    textures[i] = texture;
                    return i;
                }
            }

            int oldLength = textures.Length;
            Array.Resize( ref textures, textures.Length + texBufferSize );
            textures[oldLength] = texture;
            return oldLength;
        }

        public override void Bind2DTexture( int texId ) {
            if( texId == 0 ) {
                device.SetTexture( 0, null );
            }
            device.SetTexture( 0, textures[texId] );
        }

        public override bool Texturing {
            set {
                if( !value ) device.SetTexture( 0, null );
            }
        }

        public override void DeleteTexture( int texId ) {
            if( texId <= 0 || texId >= textures.Length ) return;

            D3D.Texture texture = textures[texId];
            if( texture != null ) {
                texture.Dispose();
            }
            textures[texId] = null;
        }

        Color lastClearCol = Color.Black;
        public override void Clear() {
            device.Clear( ClearFlags.Target | ClearFlags.ZBuffer, lastClearCol, 1f, 0 );
        }

        public override void ClearColour( FastColour col ) {
            lastClearCol = col.ToColor();
        }

        public override void ColourMask( bool red, bool green, bool blue, bool alpha ) {
            ColorWriteEnable flags = 0;
            if( red ) flags |= ColorWriteEnable.Red;
            if( green ) flags |= ColorWriteEnable.Green;
            if( blue ) flags |= ColorWriteEnable.Blue;
            if( alpha ) flags |= ColorWriteEnable.Alpha;
            device.RenderState.ColorWriteEnable = flags;
        }

        public override void DepthTestFunc( DepthFunc func ) {
            device.RenderState.ZBufferFunction = compareFuncs[(int)func];
        }

        public override bool DepthTest {
            set { device.RenderState.ZBufferEnable = value; }
        }

        public override bool DepthWrite {
            set { device.RenderState.ZBufferWriteEnable = value; }
        }

        public override void DrawVertices( DrawMode mode, VertexPos3fCol4b[] vertices, int count ) {
            device.VertexFormat = VertexFormats.Position | VertexFormats.Diffuse;
            device.DrawUserPrimitives( modeMappings[(int)mode], count, vertices );
        }

        public override void DrawVertices( DrawMode mode, VertexPos3fTex2f[] vertices, int count ) {
            device.VertexFormat = VertexFormats.Position | VertexFormats.Texture1;
            device.DrawUserPrimitives( modeMappings[(int)mode], count, vertices );
        }

        public override void DrawVertices( DrawMode mode, VertexPos3fTex2fCol4b[] vertices, int count ) {
            device.VertexFormat = VertexFormats.Position | VertexFormats.Diffuse | VertexFormats.Texture1; // TODO: Texture0?
            device.DrawUserPrimitives( modeMappings[(int)mode], vcount, vertices );
        }

        FillMode[] fillModes = new FillMode[] { FillMode.Point, FillMode.WireFrame, FillMode.Solid };
        public override void SetFillType( FillType type ) {
            device.RenderState.FillMode = fillModes[(int)type];
        }

        #region Vertex buffers

        VertexFormats[] formatMapping = new VertexFormats[] {
			VertexFormats.Position,
			VertexFormats.Position | VertexFormats.Texture1,
			VertexFormats.Position | VertexFormats.Diffuse,
			VertexFormats.Position | VertexFormats.Texture1 | VertexFormats.Diffuse,
		};

        public override int InitVb<T>( T[] vertices, DrawMode mode, VertexFormat format, int count ) {
            VertexBuffer buffer = CreateVb( vertices, count, format );

            // Find first free slot
            for( int i = 1; i < buffers.Length; i++ ) {
                if( buffers[i] == null ) {
                    buffers[i] = buffer;
                    return i;
                }
            }
            // Otherwise resize and add 2048 more elements
            int oldLength = buffers.Length;
            Array.Resize( ref buffers, buffers.Length + vertexBufferSize );
            buffers[oldLength] = buffer;

            return oldLength;
        }

        unsafe VertexBuffer CreateVb<T>( T[] vertices, int count, VertexFormat format ) {
            int sizeInBytes = GetSizeInBytes( count, format );
            VertexFormats d3dFormat = formatMapping[(int)format];

            VertexBuffer buffer = new VertexBuffer( device, sizeInBytes, Usage.None, d3dFormat, Pool.Managed );
            GraphicsStream vbData = buffer.Lock( 0, sizeInBytes, LockFlags.None );
            GCHandle handle = GCHandle.Alloc( vertices, GCHandleType.Pinned );
            IntPtr source = Marshal.UnsafeAddrOfPinnedArrayElement( vertices, 0 );
            IntPtr dest = vbData.InternalData;
            // TODO: check memcpy actually works and doesn't explode.
            memcpy( source, dest, sizeInBytes );
            buffer.Unlock();
            handle.Free();
            return buffer;
        }

        unsafe void memcpy( IntPtr sourcePtr, IntPtr destPtr, int count ) {
            byte* src = (byte*)sourcePtr;
            byte* dst = (byte*)destPtr;

            while( count >= 4 ) {
                *( (int*)dst ) = *( (int*)src );
                dst += 4;
                src += 4;
                count -= 4;
            }
            // Handle non-aligned last 0-3 bytes.
            for( int i = 0; i < count; i++ ) {
                *dst = *src;
                dst++;
                src++;
            }
        }

        public override void DeleteVb( int id ) {
            if( id <= 0 || id >= buffers.Length ) return;
            VertexBuffer buffer = buffers[id];
            if( buffer != null ) {
                buffer.Dispose();
                buffers[id] = null;
            }
        }

        public override void DrawVbPos3f( DrawMode mode, int id, int verticesCount ) {
            VertexBuffer buffer = buffers[id];
            device.SetStreamSource( 0, buffer, 0 );
            device.VertexFormat = VertexFormats.Position;
            device.DrawPrimitives( modeMappings[(int)mode], 0, verticesCount / 3 );
        }

        public override void DrawVbPos3fTex2f( DrawMode mode, int id, int verticesCount ) {
            VertexBuffer buffer = buffers[id];
            device.SetStreamSource( 0, buffer, 0 );
            device.VertexFormat = VertexFormats.Position | VertexFormats.Texture1;
            device.DrawPrimitives( modeMappings[(int)mode], 0, verticesCount / 3 );
        }

        public override void DrawVbPos3fCol4b( DrawMode mode, int id, int verticesCount ) {
            VertexBuffer buffer = buffers[id];
            device.SetStreamSource( 0, buffer, 0 );
            device.VertexFormat = VertexFormats.Position | VertexFormats.Diffuse;
            device.DrawPrimitives( modeMappings[(int)mode], 0, verticesCount / 3 );
        }

        public override void DrawVbPos3fTex2fCol4b( DrawMode mode, int id, int verticesCount ) {
            VertexBuffer buffer = buffers[id];
            device.SetStreamSource( 0, buffer, 0 );
            device.VertexFormat = VertexFormats.Position | VertexFormats.Texture1 | VertexFormats.Diffuse;
            device.DrawPrimitives( modeMappings[(int)mode], 0, verticesCount / 3 );
        }

        public override void BeginVbBatch( VertexFormat format ) {
            VertexFormats d3dFormat = formatMapping[(int)format];
            device.VertexFormat = d3dFormat;
        }

        public override void DrawVbBatch( DrawMode mode, int id, int verticesCount ) {
            VertexBuffer buffer = buffers[id];
            device.SetStreamSource( 0, buffer, 0 );
            device.DrawPrimitives( modeMappings[(int)mode], 0, verticesCount / 3 );
        }

        public override void EndVbBatch() {
        }

        #endregion


        #region Matrix manipulation

        public override void SetMatrixMode( MatrixType mode ) {
            if( mode == MatrixType.Modelview ) {
                curStack = viewStack;
            } else if( mode == MatrixType.Projection ) {
                curStack = projStack;
            } else if( mode == MatrixType.Texture ) {
                curStack = texStack;
            }
        }

        public unsafe override void LoadMatrix( ref Matrix4 matrix ) {
            Matrix4 transposed = Matrix4.Transpose( matrix );
            Matrix dxMatrix = *(Matrix*)&transposed;
            curStack.SetTop( ref dxMatrix );
        }

        public override void LoadIdentityMatrix() {
            Matrix identity = Matrix.Identity;
            curStack.SetTop( ref identity );
        }

        public override void PushMatrix() {
            curStack.Push();
        }

        public override void PopMatrix() {
            curStack.Pop();
        }

        public unsafe override void MultiplyMatrix( ref Matrix4 matrix ) {
            Matrix4 transposed = Matrix4.Transpose( matrix );
            Matrix dxMatrix = *(Matrix*)&transposed;
            curStack.MultiplyTop( ref dxMatrix );
        }

        public override void Translate( float x, float y, float z ) {
            Matrix matrix = Matrix.Translation( x, y, z );
            curStack.MultiplyTop( ref matrix );
        }

        public override void RotateX( float degrees ) {
            Matrix matrix = Matrix.RotationX( degrees * 0.01745329251f ); // PI / 180
            curStack.MultiplyTop( ref matrix );
        }

        public override void RotateY( float degrees ) {
            Matrix matrix = Matrix.RotationY( degrees * 0.01745329251f );
            curStack.MultiplyTop( ref matrix );
        }

        public override void RotateZ( float degrees ) {
            Matrix matrix = Matrix.RotationZ( degrees * 0.01745329251f );
            curStack.MultiplyTop( ref matrix );
        }

        public override void Scale( float x, float y, float z ) {
            Matrix matrix = Matrix.Scaling( x, y, z );
            curStack.MultiplyTop( ref matrix );
        }

        class MatrixStack
        {
            Matrix[] stack;
            int stackIndex;
            Action<Matrix> dxSetMatrix;

            public MatrixStack( int capacity, Action<Matrix> dxSetter ) {
                stack = new Matrix[capacity];
                stack[0] = Matrix.Identity;
                dxSetMatrix = dxSetter;
            }

            public void Push() {
                stack[stackIndex + 1] = stack[stackIndex]; // mimic GL behaviour
                stackIndex++; // exact same, we don't need to update DirectX state.
            }

            public void SetTop( ref Matrix matrix ) {
                stack[stackIndex] = matrix;
                dxSetMatrix( matrix );
            }

            public void MultiplyTop( ref Matrix matrix ) {
                stack[stackIndex] *= matrix;
                dxSetMatrix( stack[stackIndex] );
            }

            public Matrix GetTop() {
                return stack[stackIndex];
            }

            public void Pop() {
                stackIndex--;
                dxSetMatrix( stack[stackIndex] );
            }
        }

        #endregion


        public override void PrintApiSpecificInfo() {
            Console.WriteLine( "D3D tex memory available: " + device.AvailableTextureMemory );
            Console.WriteLine( "D3D software vertex processing: " + device.SoftwareVertexProcessing );
        }

        public override void TakeScreenshot( string output, Size size ) {
            using( Surface backbuffer = device.GetBackBuffer( 0, 0, BackBufferType.Mono ) ) {
                SurfaceLoader.Save( output, ImageFileFormat.Png, backbuffer );
                // D3DX SurfaceLoader is the easiest way.. I tried to save manually and failed, as according to MSDN:
                // "This method fails on render targets unless they were created as lockable
                // (or, in the case of back buffers, with LockableBackBuffer of a PresentFlag)."
            }
        }
    }
}
#endif