#region --- License ---
/* Copyright (c) 2006-2008 the OpenTK team.
 * See license.txt for license info
 * 
 * Contributions by Andy Gill.
 */
#endregion

using System;

namespace OpenTK.Graphics.OpenGL {
	
	/// <summary> OpenGL bindings for .NET, implementing the full OpenGL API, including extensions. </summary>
	public sealed partial class GL : BindingsBase {
		
		static GL() { }
		
		GraphicsContextBase context;
		protected override IntPtr GetAddress( string funcname ) {
			return context.GetAddress( funcname );
		}
		
		internal void LoadEntryPoints( GraphicsContextBase context ) {
			this.context = context;
			Debug.Print("Loading OpenGL function pointers... ");

			AlphaFuncAddress = GetAddress( "glAlphaFunc" );
			BindBufferAddress = GetAddress( "glBindBuffer" );
			BindBufferARBAddress = GetAddress( "glBindBufferARB" );
			BindTextureAddress = GetAddress( "glBindTexture" );
			BlendFuncAddress = GetAddress( "glBlendFunc" );
			BufferDataAddress = GetAddress( "glBufferData" );
			BufferDataARBAddress = GetAddress( "glBufferDataARB" );
			BufferSubDataAddress = GetAddress( "glBufferSubData" );
			BufferSubDataARBAddress = GetAddress( "glBufferSubDataARB" );
			ClearAddress = GetAddress( "glClear" );
			ClearColorAddress = GetAddress( "glClearColor" );
			ColorMaskAddress = GetAddress( "glColorMask" );
			ColorPointerAddress = GetAddress( "glColorPointer" );
			CullFaceAddress = GetAddress( "glCullFace" );
			DeleteBuffersAddress = GetAddress( "glDeleteBuffers" );
			DeleteBuffersARBAddress = GetAddress( "glDeleteBuffersARB" );
			DeleteTexturesAddress = GetAddress( "glDeleteTextures" );
			DepthFuncAddress = GetAddress( "glDepthFunc" );
			DepthMaskAddress = GetAddress( "glDepthMask" );
			DisableAddress = GetAddress( "glDisable" );
			DisableClientStateAddress = GetAddress( "glDisableClientState" );
			DrawArraysAddress = GetAddress( "glDrawArrays" );
			DrawElementsAddress = GetAddress( "glDrawElements" );
			EnableAddress = GetAddress( "glEnable" );
			EnableClientStateAddress = GetAddress( "glEnableClientState" );
			FogfAddress = GetAddress( "glFogf" );
			FogfvAddress = GetAddress( "glFogfv" );
			FogiAddress = GetAddress( "glFogi" );
			GenBuffersAddress = GetAddress( "glGenBuffers" );
			GenBuffersARBAddress = GetAddress( "glGenBuffersARB" );
			GenTexturesAddress = GetAddress( "glGenTextures" );
			GetErrorAddress = GetAddress( "glGetError" );
			GetFloatvAddress = GetAddress( "glGetFloatv" );
			GetIntegervAddress = GetAddress( "glGetIntegerv" );
			GetStringAddress = GetAddress( "glGetString" );
			HintAddress = GetAddress( "glHint" );
			LoadIdentityAddress = GetAddress( "glLoadIdentity" );
			LoadMatrixfAddress = GetAddress( "glLoadMatrixf" );
			MatrixModeAddress = GetAddress( "glMatrixMode" );
			MultMatrixfAddress = GetAddress( "glMultMatrixf" );
			PopMatrixAddress = GetAddress( "glPopMatrix" );
			PushMatrixAddress = GetAddress( "glPushMatrix" );
			ReadPixelsAddress = GetAddress( "glReadPixels" );
			ShadeModelAddress = GetAddress( "glShadeModel" );
			TexCoordPointerAddress = GetAddress( "glTexCoordPointer" );
			TexImage2DAddress = GetAddress( "glTexImage2D" );
			TexParameteriAddress = GetAddress( "glTexParameteri" );
			TexSubImage2DAddress = GetAddress( "glTexSubImage2D" );
			VertexPointerAddress = GetAddress( "glVertexPointer" );
			ViewportAddress = GetAddress( "glViewport" );
		}
		
		public static void UseArbVboAddresses() {
			BindBufferAddress = BindBufferARBAddress;
			BufferDataAddress = BufferDataARBAddress;
			BufferSubDataAddress = BufferSubDataARBAddress;
			DeleteBuffersAddress = DeleteBuffersARBAddress;
			GenBuffersAddress = GenBuffersARBAddress;
		}
	}
}
