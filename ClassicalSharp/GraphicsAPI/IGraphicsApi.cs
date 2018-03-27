// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using OpenTK;
#if ANDROID
using Android.Graphics;
#else
using System.Drawing.Imaging;
#endif

namespace ClassicalSharp.GraphicsAPI {
	
	/// <summary> Abstracts a 3D graphics rendering API. </summary>
	public abstract partial class IGraphicsApi {
		
		/// <summary> Maximum supported length of a dimension (width and height) of a 2D texture. </summary>
		public abstract int MaxTextureDimensions { get; }
		
		/// <summary> Sets whether texturing is applied when rasterizing primitives. </summary>
		public abstract bool Texturing { set; }
		
		public Matrix4 Projection, View;
		internal float MinZNear = 0.1f;
		readonly FastBitmap bmpBuffer = new FastBitmap();
		
		/// <summary> Returns whether this graphics api had a valid context. </summary>
		public bool LostContext;

		/// <summary> Event raised when a context is destroyed after having been previously lost. </summary>
		public event Action ContextLost;
		
		/// <summary> Event raised when a context is recreated after having been previously lost. </summary>
		public event Action ContextRecreated;
		
		/// <summary> Whether mipmapping of terrain textures is used. </summary>
		public bool Mipmaps;

		/// <summary> Whether the backend supports setting the number of custom mipmaps levels. </summary>
		public bool CustomMipmapsLevels;
		
		/// <summary> Delegate that is invoked when the current context is lost,
		/// and is repeatedly invoked until the context can be retrieved. </summary>
		public ScheduledTaskCallback LostContextFunction;
		
		
		/// <summary> Creates a new native texture with the specified dimensions and using the
		/// image data encapsulated by the Bitmap instance. </summary>
		/// <remarks> Note that should make every effort you can to ensure that the dimensions of the bitmap
		/// are powers of two, because otherwise they will not display properly on certain graphics cards.	<br/>
		/// This method returns -1 if the input image is not a 32bpp format. </remarks>
		public int CreateTexture(Bitmap bmp, bool managedPool, bool mipmaps) {
			if (!Platform.Is32Bpp(bmp)) {
				throw new ArgumentOutOfRangeException("Bitmap must be 32bpp");
			}
			
			bmpBuffer.SetData(bmp, true, true);
			return CreateTexture(bmpBuffer, managedPool, mipmaps);
		}
		
		/// <summary> Creates a new native texture with the specified dimensions and FastBitmap instance
		/// that encapsulates the pointer to the 32bpp image data.</summary>
		/// <remarks> Note that should make every effort you can to ensure that the dimensions are powers of two,
		/// because otherwise they will not display properly on certain graphics cards.	</remarks>
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
		
		/// <summary> Creates a new native texture with the specified dimensions and pointer to the 32bpp image data. </summary>
		/// <remarks> Note that should make every effort you can to ensure that the dimensions are powers of two,
		/// because otherwise they will not display properly on certain graphics cards.	</remarks>
		protected abstract int CreateTexture(int width, int height, IntPtr scan0, bool managedPool, bool mipmaps);
		
		/// <summary> Updates the sub-rectangle (x, y) -> (x + part.Width, y + part.Height)
		/// of the native texture associated with the given ID, with the pixels encapsulated in the 'part' instance. </summary>
		public abstract void UpdateTexturePart(int texId, int x, int y, FastBitmap part, bool mipmaps);
		
		/// <summary> Binds the given texture id so that it can be used for rasterization. </summary>
		public abstract void BindTexture(int texId);
		
		/// <summary> Frees all native resources held for the given texture id. </summary>
		public abstract void DeleteTexture(ref int texId);
		
		/// <summary> Frees all native resources held for the given texture id. </summary>
		public void DeleteTexture(ref Texture texture) { DeleteTexture(ref texture.ID); }
		
		/// <summary> Enables mipmapping for subsequent texture drawing. </summary>
		public abstract void EnableMipmaps();
		
		/// <summary> Disbles mipmapping for subsequent texture drawing. </summary>
		public abstract void DisableMipmaps();
		
		/// <summary> Gets or sets whether fog is currently enabled. </summary>
		public abstract bool Fog { get; set; }
		
		/// <summary> Sets the fog colour that is blended with final primitive colours. </summary>
		public abstract void SetFogColour(FastColour col);
		
		/// <summary> Sets the density of exp and exp^2 fog </summary>
		public abstract void SetFogDensity(float value);
		
		/// <summary> Sets the start radius of fog for linear fog. </summary>
		public abstract void SetFogStart(float value);
		
		/// <summary> Sets the end radius of fog for for linear fog. </summary>
		public abstract void SetFogEnd(float value);
		
		/// <summary> Sets the current fog mode. (linear, exp, or exp^2) </summary>
		public abstract void SetFogMode(Fog fogMode);
		
		/// <summary> Whether back facing primitives should be culled by the 3D graphics api. </summary>
		public abstract bool FaceCulling { set; }

		
		/// <summary> Whether alpha testing is currently enabled. </summary>
		public abstract bool AlphaTest { set; }
		
		/// <summary> Sets the alpha test compare function that is used when alpha testing is enabled. </summary>
		public abstract void AlphaTestFunc(CompareFunc func, float refValue);
		
		/// <summary> Whether alpha blending is currently enabled. </summary>
		public abstract bool AlphaBlending { set; }
		
		/// <summary> Sets the alpha blend function that is used when alpha blending is enabled. </summary>
		public abstract void AlphaBlendFunc(BlendFunc srcFunc, BlendFunc dstFunc);
		
		/// <summary> Clears the underlying back and/or front buffer. </summary>
		public abstract void Clear();
		
		/// <summary> Sets the colour the screen is cleared to when Clear() is called. </summary>
		public abstract void ClearColour(FastColour col);
		
		/// <summary> Whether depth testing is currently enabled. </summary>
		public abstract bool DepthTest { set; }
		
		/// <summary> Sets the depth test compare function that is used when depth testing is enabled. </summary>
		public abstract void DepthTestFunc(CompareFunc func);
		
		/// <summary> Whether writing to the colour buffer is enabled. </summary>
		public abstract void ColourWriteMask(bool r, bool g, bool b, bool a);
		
		/// <summary> Whether writing to the depth buffer is enabled. </summary>
		public abstract bool DepthWrite { set; }
		
		/// <summary> Whether blending between the alpha components of the texture and colour are performed. </summary>
		public abstract bool AlphaArgBlend { set; }
		
		/// <summary> Creates a vertex buffer that can have its data dynamically updated. </summary>
		public abstract int CreateDynamicVb(VertexFormat format, int maxVertices);
		
		/// <summary> Creates a static vertex buffer that has its data set at creation,
		/// but the vertex buffer's data cannot be updated after creation. </summary>
		public abstract int CreateVb(IntPtr vertices, VertexFormat format, int count);
		
		/// <summary> Creates a static index buffer that has its data set at creation,
		/// but the index buffer's data cannot be updated after creation. </summary>
		public abstract int CreateIb(IntPtr indices, int indicesCount);
		
		/// <summary> Sets the currently active vertex buffer to the given id. </summary>
		public abstract void BindVb(int vb);
		
		/// <summary> Sets the currently active index buffer to the given id. </summary>
		public abstract void BindIb(int ib);
		
		/// <summary> Frees all native resources held for the vertex buffer associated with the given id. </summary>
		public abstract void DeleteVb(ref int vb);
		
		/// <summary> Frees all native resources held for the index buffer associated with the given id. </summary>
		public abstract void DeleteIb(ref int ib);
		
		/// <summary> Informs the graphics API that the format of the vertex data used in subsequent
		/// draw calls will be in the given format. </summary>
		public abstract void SetBatchFormat(VertexFormat format);
		
		/// <summary> Binds and updates the data of the current dynamic vertex buffer's data.<br/>
		/// This method also replaces the dynamic vertex buffer's data first with the given vertices before drawing. </summary>
		public abstract void SetDynamicVbData(int vb, IntPtr vertices, int vCount);
		
		/// <summary> Draws the specified subset of the vertices in the current vertex buffer as lines. </summary>
		public abstract void DrawVb_Lines(int verticesCount);
		
		/// <summary> Draws the specified subset of the vertices in the current vertex buffer as triangles. </summary>
		public abstract void DrawVb_IndexedTris(int verticesCount, int startVertex);
		
		/// <summary> Draws the specified subset of the vertices in the current vertex buffer as triangles. </summary>
		public abstract void DrawVb_IndexedTris(int verticesCount);
		
		/// <summary> Optimised version of DrawIndexedVb for VertexFormat.Pos3fTex2fCol4b </summary>
		internal abstract void DrawIndexedVb_TrisT2fC4b(int verticesCount, int startVertex);
		
		protected static int[] strideSizes = new int[] { 16, 24 };
		
		
		/// <summary> Sets the matrix type that load/push/pop operations should be applied to. </summary>
		public abstract void SetMatrixMode(MatrixType mode);
		
		/// <summary> Sets the current matrix to the given matrix. </summary>
		public abstract void LoadMatrix(ref Matrix4 matrix);
		
		/// <summary> Sets the current matrix to the identity matrix. </summary>
		public abstract void LoadIdentityMatrix();
		
		/// <summary> Outputs a .png screenshot of the backbuffer to the specified file. </summary>
		public abstract void TakeScreenshot(string output, int width, int height);
		
		/// <summary> Adds a warning to chat if this graphics API has problems with the current user's GPU. </summary>
		public virtual bool WarnIfNecessary(Chat chat) { return false; }
		
		/// <summary> Informs the graphic api to update its state in preparation for a new frame. </summary>
		public abstract void BeginFrame(Game game);
		
		/// <summary> Informs the graphic api to update its state in preparation for the end of a frame,
		/// and to prepare that frame for display on the monitor. </summary>
		public abstract void EndFrame(Game game);
		
		/// <summary> Sets whether the graphics api should tie frame rendering to the refresh rate of the monitor. </summary>
		public abstract void SetVSync(Game game, bool value);
		
		/// <summary> Raised when the dimensions of the game's window have changed. </summary>
		public abstract void OnWindowResize(Game game);
		
				
		internal abstract void MakeApiInfo();		
		public string[] ApiInfo;
		
		public abstract void CalcOrthoMatrix(float width, float height, out Matrix4 matrix);
		public virtual void CalcPerspectiveMatrix(float fov, float aspect, float zNear, float zFar, out Matrix4 matrix) {
			Matrix4.CreatePerspectiveFieldOfView(fov, aspect, zNear, zFar, out matrix);
		}
		
		/// <summary> Sets the appropriate alpha testing/blending states necessary to render the given block. </summary>
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