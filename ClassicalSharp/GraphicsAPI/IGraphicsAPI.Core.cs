// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;

namespace ClassicalSharp.GraphicsAPI {
	
	/// <summary> Abstracts a 3D graphics rendering API. </summary>
	public abstract partial class IGraphicsApi {
		
		protected void InitDynamicBuffers() {
			quadVb = CreateDynamicVb(VertexFormat.P3fC4b, 4);
			texVb = CreateDynamicVb(VertexFormat.P3fT2fC4b, 4);
		}
		
		public virtual void Dispose() {
			DeleteVb(ref quadVb);
			DeleteVb(ref texVb);
		}
		
		public void LoseContext(string reason) {
			LostContext = true;
			Utils.LogDebug("Lost graphics context" + reason);
			if (ContextLost != null) ContextLost();			
			
			DeleteVb(ref quadVb);
			DeleteVb(ref texVb);
		}
		
		public void RecreateContext() {
			LostContext = false;
			Utils.LogDebug("Recreating graphics context");
			if (ContextRecreated != null) ContextRecreated();
			
			InitDynamicBuffers();
		}
		
		
		/// <summary> Binds and draws the specified subset of the vertices in the current dynamic vertex buffer<br/>
		/// This method also replaces the dynamic vertex buffer's data first with the given vertices before drawing. </summary>
		public void UpdateDynamicVb_Lines<T>(int vb, T[] vertices, int vCount) where T : struct {
			SetDynamicVbData(vb, vertices, vCount);
			DrawVb_Lines(vCount);
		}
		
		/// <summary> Binds and draws the specified subset of the vertices in the current dynamic vertex buffer<br/>
		/// This method also replaces the dynamic vertex buffer's data first with the given vertices before drawing. </summary>
		public void UpdateDynamicVb_IndexedTris<T>(int vb, T[] vertices, int vCount) where T : struct {
			SetDynamicVbData(vb, vertices, vCount);
			DrawVb_IndexedTris(vCount * 6 / 4);
		}		
		
		
		internal VertexP3fC4b[] quadVerts = new VertexP3fC4b[4];
		internal int quadVb;
		public virtual void Draw2DQuad(float x, float y, float width, float height,
		                               FastColour col) {
			int c = col.Pack();
			quadVerts[0] = new VertexP3fC4b(x, y, 0, c);
			quadVerts[1] = new VertexP3fC4b(x + width, y, 0, c);
			quadVerts[2] = new VertexP3fC4b(x + width, y + height, 0, c);
			quadVerts[3] = new VertexP3fC4b(x, y + height, 0, c);
			SetBatchFormat(VertexFormat.P3fC4b);
			UpdateDynamicVb_IndexedTris(quadVb, quadVerts, 4);
		}
		
		public virtual void Draw2DQuad(float x, float y, float width, float height,
		                               FastColour topCol, FastColour bottomCol) {
			int c = topCol.Pack();
			quadVerts[0] = new VertexP3fC4b(x, y, 0, c);
			quadVerts[1] = new VertexP3fC4b(x + width, y, 0, c);
			c = bottomCol.Pack();
			quadVerts[2] = new VertexP3fC4b(x + width, y + height, 0, c);
			quadVerts[3] = new VertexP3fC4b(x, y + height, 0, c);
			SetBatchFormat(VertexFormat.P3fC4b);
			UpdateDynamicVb_IndexedTris(quadVb, quadVerts, 4);
		}
		
		internal VertexP3fT2fC4b[] texVerts = new VertexP3fT2fC4b[4];
		internal int texVb;
		public virtual void Draw2DTexture(ref Texture tex, FastColour col) {
			int index = 0;
			Make2DQuad(ref tex, col.Pack(), texVerts, ref index);
			SetBatchFormat(VertexFormat.P3fT2fC4b);
			UpdateDynamicVb_IndexedTris(texVb, texVerts, 4);
		}
		
		public static void Make2DQuad(ref Texture tex, int col,
		                              VertexP3fT2fC4b[] vertices, ref int index) {
			float x1 = tex.X, y1 = tex.Y, x2 = tex.X + tex.Width, y2 = tex.Y + tex.Height;
			#if USE_DX
			// NOTE: see "https://msdn.microsoft.com/en-us/library/windows/desktop/bb219690(v=vs.85).aspx",
			// i.e. the msdn article called "Directly Mapping Texels to Pixels (Direct3D 9)" for why we have to do this.
			x1 -= 0.5f; x2 -= 0.5f;
			y1 -= 0.5f; y2 -= 0.5f;
			#endif
			vertices[index++] = new VertexP3fT2fC4b(x1, y1, 0, tex.U1, tex.V1, col);
			vertices[index++] = new VertexP3fT2fC4b(x2, y1, 0, tex.U2, tex.V1, col);
			vertices[index++] = new VertexP3fT2fC4b(x2, y2, 0, tex.U2, tex.V2, col);
			vertices[index++] = new VertexP3fT2fC4b(x1, y2, 0, tex.U1, tex.V2, col);
		}
		bool hadFog;
		
		/// <summary> Updates the various matrix stacks and properties so that the graphics API state
		/// is suitable for rendering 2D quads and other 2D graphics to. </summary>
		public void Mode2D(float width, float height) {
			SetMatrixMode(MatrixType.Projection);
			PushMatrix();
			LoadOrthoMatrix(width, height);
			SetMatrixMode(MatrixType.Modelview);
			PushMatrix();
			LoadIdentityMatrix();
			
			DepthTest = false;
			AlphaBlending = true;
			hadFog = Fog;
			if (hadFog) Fog = false;
		}
		
		/// <summary> Updates the various matrix stacks and properties so that the graphics API state
		/// is suitable for rendering 3D vertices. </summary>
		public void Mode3D() {
			SetMatrixMode(MatrixType.Projection);
			PopMatrix(); // Get rid of orthographic 2D matrix.
			SetMatrixMode(MatrixType.Modelview);
			PopMatrix();
			
			DepthTest = true;
			AlphaBlending = false;
			if (hadFog) Fog = true;
		}
		
		internal unsafe int MakeDefaultIb() {
			const int maxIndices = 65536 / 4 * 6;
			ushort* indices = stackalloc ushort[maxIndices];			
			MakeIndices(indices, maxIndices);
			return CreateIb((IntPtr)indices, maxIndices);
		}
		
		internal unsafe void MakeIndices(ushort* indices, int iCount) {
			int element = 0;			
			for (int i = 0; i < iCount; i += 6) {
				*indices = (ushort)(element + 0); indices++;
				*indices = (ushort)(element + 1); indices++;
				*indices = (ushort)(element + 2); indices++;
				
				*indices = (ushort)(element + 2); indices++;
				*indices = (ushort)(element + 3); indices++;
				*indices = (ushort)(element + 0); indices++;
				element += 4;
			}
		}
				
		internal static unsafe void GenMipmaps(int lvlWidth, int lvlHeight, IntPtr lvlScan0, 
		                                       int width, int height, IntPtr scan0) {
			int* baseSrc = (int*)scan0, baseDst = (int*)lvlScan0;
			const int alphaMask = unchecked((int)0xFF000000);
			
			for (int y = 0; y < lvlHeight; y++) {
				int srcY = y * height / lvlHeight;
				int* src0 = baseSrc + srcY * width;
				int* src1 = src0 + width;
				int* dst = baseDst + y * lvlWidth;
				
				for (int x = 0; x < lvlWidth; x++) {
					int srcX = x * width / lvlWidth;
					int src00 = src0[srcX], src01 = src0[srcX + 1];
					int src10 = src1[srcX], src11 = src1[srcX + 1];
					
					// Average the four pixels in the original texture for this mipmapped pixel
					int r = (((src00 & 0x0000FF) >>  0) + ((src01 & 0x0000FF) >>  0) + ((src10 & 0x0000FF) >>  0) + ((src11 & 0x0000FF) >>  0)) >> 2;
					int g = (((src00 & 0x00FF00) >>  8) + ((src01 & 0x00FF00) >>  8) + ((src10 & 0x00FF00) >>  8) + ((src11 & 0x00FF00) >>  8)) >> 2;
					int b = (((src00 & 0xFF0000) >> 16) + ((src01 & 0xFF0000) >> 16) + ((src10 & 0xFF0000) >> 16) + ((src11 & 0xFF0000) >> 16)) >> 2;				
					dst[x] = r | (g << 8) | (b << 16) | (src00 & alphaMask);
				}
			}
		}
	}

	public enum VertexFormat {
		P3fC4b = 0, P3fT2fC4b = 1,
	}
	
	public enum CompareFunc {
		Always = 0,
		NotEqual = 1,
		Never = 2,
		
		Less = 3,
		LessEqual = 4,
		Equal = 5,
		GreaterEqual = 6,
		Greater = 7,
	}
	
	public enum BlendFunc {
		Zero = 0,
		One = 1,
		
		SourceAlpha = 2,
		InvSourceAlpha = 3,
		DestAlpha = 4,
		InvDestAlpha = 5,
	}
	
	public enum Fog {
		Linear = 0, Exp = 1, Exp2 = 2,
	}
	
	public enum MatrixType {
		Projection = 0, Modelview = 1, Texture = 2,
	}
}