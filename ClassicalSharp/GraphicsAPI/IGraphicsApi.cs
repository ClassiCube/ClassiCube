// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using OpenTK;
using System.IO;
#if ANDROID
using Android.Graphics;
#else
using System.Drawing.Imaging;
#endif

namespace ClassicalSharp.GraphicsAPI {
	
	/// <summary> Abstracts a 3D graphics rendering API. </summary>
	public abstract partial class IGraphicsApi {
		
		public abstract int MaxTextureDimensions { get; }
		public abstract bool Texturing { set; }
		
		public Matrix4 Projection, View;
		internal float MinZNear; // MinZNear = 0.1f;
		
		public bool LostContext;
		public event Action ContextLost;
		public event Action ContextRecreated;
		public ScheduledTaskCallback LostContextFunction;
		
		public bool Mipmaps;
		public bool CustomMipmapsLevels;
		
		readonly FastBitmap bmpBuffer = new FastBitmap();
		public int CreateTexture(Bitmap bmp, bool managedPool, bool mipmaps) {
			bmpBuffer.SetData(bmp, true, true);
			return CreateTexture(bmpBuffer, managedPool, mipmaps);
		}
		
		public int CreateTexture(FastBitmap bmp, bool managedPool, bool mipmaps) {
			if (!Utils.IsPowerOf2(bmp.Width) || !Utils.IsPowerOf2(bmp.Height)) {
				throw new ArgumentOutOfRangeException("Bitmap must have power of two dimensions");
			}
			if (LostContext) throw new InvalidOperationException("Cannot create texture when context lost");
			
			if (!bmp.IsLocked) bmp.LockBits();
			int texId = CreateTexture(bmp.Width, bmp.Height, bmp.Scan0, managedPool, mipmaps);
			bmp.UnlockBits();
			return texId;
		}
		
		protected abstract int CreateTexture(int width, int height, IntPtr scan0, bool managedPool, bool mipmaps);
		public abstract void UpdateTexturePart(int texId, int x, int y, FastBitmap part, bool mipmaps);
		
		public abstract void BindTexture(int texId);
		public abstract void DeleteTexture(ref int texId);
		public void DeleteTexture(ref Texture texture) { DeleteTexture(ref texture.ID); }
		
		public abstract void EnableMipmaps();
		public abstract void DisableMipmaps();
		
		public abstract bool Fog { get; set; }
		public abstract void SetFogCol(PackedCol col);
		public abstract void SetFogDensity(float value);
		public abstract void SetFogEnd(float value);
		public abstract void SetFogMode(Fog fogMode);
		
		public abstract bool FaceCulling { set; }

		public abstract bool AlphaTest { set; }
		public abstract void AlphaTestFunc(CompareFunc func, float refValue);
		
		public abstract bool AlphaBlending { set; }
		public abstract void AlphaBlendFunc(BlendFunc srcFunc, BlendFunc dstFunc);
		
		public abstract void Clear();
		public abstract void ClearCol(PackedCol col);
		
		public abstract bool DepthTest { set; }
		public abstract void DepthTestFunc(CompareFunc func);
		
		public abstract void ColWriteMask(bool r, bool g, bool b, bool a);
		public abstract bool DepthWrite { set; }
		
		/// <summary> Whether blending between the alpha components of the texture and colour are performed. </summary>
		public abstract bool AlphaArgBlend { set; }
		
		public abstract int CreateDynamicVb(VertexFormat format, int maxVertices);
		public abstract int CreateVb(IntPtr vertices, VertexFormat format, int count);
		public abstract int CreateIb(IntPtr indices, int indicesCount);
		
		public abstract void BindVb(int vb);
		public abstract void BindIb(int ib);
		public abstract void DeleteVb(ref int vb);
		public abstract void DeleteIb(ref int ib);
		
		/// <summary> Informs the graphics API that the format of the vertex data used in subsequent
		/// draw calls will be in the given format. </summary>
		public abstract void SetBatchFormat(VertexFormat format);
		
		/// <summary> Binds and updates the data of the current dynamic vertex buffer's data.<br/>
		/// This method also replaces the dynamic vertex buffer's data first with the given vertices before drawing. </summary>
		public abstract void SetDynamicVbData(int vb, IntPtr vertices, int vCount);
		
		public abstract void DrawVb_Lines(int verticesCount);
		public abstract void DrawVb_IndexedTris(int verticesCount, int startVertex);
		public abstract void DrawVb_IndexedTris(int verticesCount);
		
		/// <summary> Optimised version of DrawIndexedVb for VertexFormat.Pos3fTex2fCol4b </summary>
		internal abstract void DrawIndexedVb_TrisT2fC4b(int verticesCount, int startVertex);
		
		protected static int[] strideSizes = new int[] { 16, 24 };
		
		public abstract void SetMatrixMode(MatrixType mode);
		public abstract void LoadMatrix(ref Matrix4 matrix);
		public abstract void LoadIdentityMatrix();
		
		public abstract void TakeScreenshot(Stream output, int width, int height);		
		public virtual bool WarnIfNecessary(Chat chat) { return false; }
		
		public abstract void BeginFrame(Game game);
		public abstract void EndFrame(Game game);
		
		public abstract void SetVSync(Game game, bool value);
		public abstract void OnWindowResize(Game game);
		
		internal abstract void MakeApiInfo();
		public string[] ApiInfo;
		
		public abstract void CalcOrthoMatrix(float width, float height, out Matrix4 matrix);
		public virtual void CalcPerspectiveMatrix(float fov, float aspect, float zNear, float zFar, out Matrix4 matrix) {
			Matrix4.CreatePerspectiveFieldOfView(fov, aspect, zNear, zFar, out matrix);
		}
		
		public void SetupAlphaState(byte draw) {
			if (draw == DrawType.Translucent)      AlphaBlending = true;
			if (draw == DrawType.Transparent)      AlphaTest = true;
			if (draw == DrawType.TransparentThick) AlphaTest = true;
			if (draw == DrawType.Sprite)           AlphaTest = true;
		}
		
		public void RestoreAlphaState(byte draw) {
			if (draw == DrawType.Translucent)      AlphaBlending = false;
			if (draw == DrawType.Transparent)      AlphaTest = false;
			if (draw == DrawType.TransparentThick) AlphaTest = false;
			if (draw == DrawType.Sprite)           AlphaTest = false;
		}
	}
}