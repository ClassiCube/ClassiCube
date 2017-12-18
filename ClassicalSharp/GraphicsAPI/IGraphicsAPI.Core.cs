// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using OpenTK;

namespace ClassicalSharp.GraphicsAPI {
	
	/// <summary> Abstracts a 3D graphics rendering API. </summary>
	public abstract unsafe partial class IGraphicsApi {
		
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
		public void UpdateDynamicVb_Lines(int vb, VertexP3fC4b[] vertices, int vCount) {
			fixed (VertexP3fC4b* ptr = vertices) {
				SetDynamicVbData(vb, (IntPtr)ptr, vCount);
				DrawVb_Lines(vCount);
			}
		}
		
		/// <summary> Binds and draws the specified subset of the vertices in the current dynamic vertex buffer<br/>
		/// This method also replaces the dynamic vertex buffer's data first with the given vertices before drawing. </summary>
		public void UpdateDynamicVb_IndexedTris(int vb, VertexP3fC4b[] vertices, int vCount) {
			fixed (VertexP3fC4b* ptr = vertices) {
				SetDynamicVbData(vb, (IntPtr)ptr, vCount);
				DrawVb_IndexedTris(vCount);
			}
		}
		
		/// <summary> Binds and draws the specified subset of the vertices in the current dynamic vertex buffer<br/>
		/// This method also replaces the dynamic vertex buffer's data first with the given vertices before drawing. </summary>
		public void UpdateDynamicVb_IndexedTris(int vb, VertexP3fT2fC4b[] vertices, int vCount) {
			fixed (VertexP3fT2fC4b* ptr = vertices) {
				SetDynamicVbData(vb, (IntPtr)ptr, vCount);
				DrawVb_IndexedTris(vCount);
			}
		}
		
		
		internal VertexP3fC4b[] quadVerts = new VertexP3fC4b[4];
		internal int quadVb;
		public virtual void Draw2DQuad(int x, int y, int width, int height,
		                               FastColour col) {
			int c = col.Pack();
			quadVerts[0] = new VertexP3fC4b(x, y, 0, c);
			quadVerts[1] = new VertexP3fC4b(x + width, y, 0, c);
			quadVerts[2] = new VertexP3fC4b(x + width, y + height, 0, c);
			quadVerts[3] = new VertexP3fC4b(x, y + height, 0, c);
			SetBatchFormat(VertexFormat.P3fC4b);
			UpdateDynamicVb_IndexedTris(quadVb, quadVerts, 4);
		}
		
		public virtual void Draw2DQuad(int x, int y, int width, int height,
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
			
			VertexP3fT2fC4b v; v.Z = 0; v.Colour = col;
			v.X = x1; v.Y = y1; v.U = tex.U1; v.V = tex.V1; vertices[index++] = v;
			v.X = x2;           v.U = tex.U2;               vertices[index++] = v;
			v.Y = y2;                         v.V = tex.V2; vertices[index++] = v;
			v.X = x1;           v.U = tex.U1;               vertices[index++] = v;
		}
		bool hadFog;
		
		/// <summary> Updates the various matrix stacks and properties so that the graphics API state
		/// is suitable for rendering 2D quads and other 2D graphics to. </summary>
		public void Mode2D(int width, int height) {
			SetMatrixMode(MatrixType.Projection);
			Matrix4 ortho;
			CalcOrthoMatrix(width, height, out ortho);
			LoadMatrix(ref ortho);
			
			SetMatrixMode(MatrixType.Modelview);
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
			LoadMatrix(ref Projection);
			SetMatrixMode(MatrixType.Modelview);
			LoadMatrix(ref View);
			
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
		
		const int alphaMask = unchecked((int)0xFF000000);
		// Quoted from http://www.realtimerendering.com/blog/gpus-prefer-premultiplication/
		// The short version: if you want your renderer to properly handle textures with alphas when using
		// bilinear interpolation or mipmapping, you need to premultiply your PNG color data by their (unassociated) alphas.
		static int Average(int p1, int p2) {
			int a1 = ((p1 & alphaMask) >> 24) & 0xFF;
			int a2 = ((p2 & alphaMask) >> 24) & 0xFF;
			int aSum = (a1 + a2);
			aSum = aSum > 0 ? aSum : 1; // avoid divide by 0 below
			
			// Convert RGB to pre-multiplied form
			int r1 = ((p1 >> 16) & 0xFF) * a1, g1 = ((p1 >> 8) & 0xFF) * a1, b1 = (p1 & 0xFF) * a1;
			int r2 = ((p2 >> 16) & 0xFF) * a2, g2 = ((p2 >> 8) & 0xFF) * a2, b2 = (p2 & 0xFF) * a2;
			
			// https://stackoverflow.com/a/347376
			// We need to convert RGB back from the pre-multiplied average into normal form
			// ((r1 + r2) / 2) / ((a1 + a2) / 2)
			// but we just cancel out the / 2
			int aAve = aSum >> 1;
			int rAve = (r1 + r2) / aSum;
			int gAve = (g1 + g2) / aSum;
			int bAve = (b1 + b2) / aSum;
			
			return (aAve << 24) | (rAve << 16) | (gAve << 8) | bAve;
		}
		
		internal static unsafe void GenMipmaps(int width, int height, IntPtr lvlScan0, IntPtr scan0) {
			int* baseSrc = (int*)scan0, baseDst = (int*)lvlScan0;
			int srcWidth = width << 1;
			
			for (int y = 0; y < height; y++) {
				int srcY = (y << 1);
				int* src0 = baseSrc + srcY * srcWidth, src1 = src0 + srcWidth;
				int* dst = baseDst + y * width;
				
				for (int x = 0; x < width; x++) {
					int srcX = (x << 1);
					int src00 = src0[srcX], src01 = src0[srcX + 1];
					int src10 = src1[srcX], src11 = src1[srcX + 1];
					
					// bilinear filter this mipmap
					int ave0 = Average(src00, src01);
					int ave1 = Average(src10, src11);
					dst[x] = Average(ave0, ave1);
				}
			}
		}
		
		internal int MipmapsLevels(int width, int height) {
			int lvlsWidth = Utils.Log2(width), lvlsHeight = Utils.Log2(height);
			if (CustomMipmapsLevels) {
				int lvls = Math.Min(lvlsWidth, lvlsHeight);
				return Math.Min(lvls, 4);
			} else {
				return Math.Max(lvlsWidth, lvlsHeight);
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