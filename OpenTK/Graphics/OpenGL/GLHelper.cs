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
			
			BeginAddress = GetAddress( "glBegin" );
			BindBufferAddress = GetAddress( "glBindBuffer" );
			BindBufferARBAddress = GetAddress( "glBindBufferARB" );
			BindTextureAddress = GetAddress( "glBindTexture" );
			BlendFuncAddress = GetAddress( "glBlendFunc" );
			BufferDataAddress = GetAddress( "glBufferData" );
			BufferDataARBAddress = GetAddress( "glBufferDataARB" );
			BufferSubDataAddress = GetAddress( "glBufferSubData" );
			BufferSubDataARBAddress = GetAddress( "glBufferSubDataARB" );
			
			CallListAddress = GetAddress( "glCallList" );
			ClearAddress = GetAddress( "glClear" );
			ClearColorAddress = GetAddress( "glClearColor" );
			Color4ubAddress = GetAddress( "glColor4ub" );
			ColorMaskAddress = GetAddress( "glColorMask" );
			ColorPointerAddress = GetAddress( "glColorPointer" );
			CullFaceAddress = GetAddress( "glCullFace" );
			
			DeleteBuffersAddress = GetAddress( "glDeleteBuffers" );
			DeleteBuffersARBAddress = GetAddress( "glDeleteBuffersARB" );
			DeleteListsAddress = GetAddress( "glDeleteLists" );
			DeleteTexturesAddress = GetAddress( "glDeleteTextures" );
			DepthFuncAddress = GetAddress( "glDepthFunc" );
			DepthMaskAddress = GetAddress( "glDepthMask" );
			DisableAddress = GetAddress( "glDisable" );
			DisableClientStateAddress = GetAddress( "glDisableClientState" );
			DrawArraysAddress = GetAddress( "glDrawArrays" );
			DrawElementsAddress = GetAddress( "glDrawElements" );
			
			EnableAddress = GetAddress( "glEnable" );
			EnableClientStateAddress = GetAddress( "glEnableClientState" );
			EndAddress = GetAddress( "glEnd" );
			EndListAddress = GetAddress( "glEndList" );
			FogfAddress = GetAddress( "glFogf" );
			FogfvAddress = GetAddress( "glFogfv" );
			FogiAddress = GetAddress( "glFogi" );
			
			GenBuffersAddress = GetAddress( "glGenBuffers" );
			GenBuffersARBAddress = GetAddress( "glGenBuffersARB" );
			GenListsAddress = GetAddress( "glGenLists" );
			GenTexturesAddress = GetAddress( "glGenTextures" );
			GetErrorAddress = GetAddress( "glGetError" );
			GetFloatvAddress = GetAddress( "glGetFloatv" );
			GetIntegervAddress = GetAddress( "glGetIntegerv" );
			GetStringAddress = GetAddress( "glGetString" );
			GetTexImageAddress = GetAddress( "glGetTexImage" );
			
			HintAddress = GetAddress( "glHint" );			
			LoadIdentityAddress = GetAddress( "glLoadIdentity" );
			LoadMatrixfAddress = GetAddress( "glLoadMatrixf" );
			MatrixModeAddress = GetAddress( "glMatrixMode" );
			MultMatrixfAddress = GetAddress( "glMultMatrixf" );
			NewListAddress = GetAddress( "glNewList" );
			
			PopMatrixAddress = GetAddress( "glPopMatrix" );
			PushMatrixAddress = GetAddress( "glPushMatrix" );			
			ReadPixelsAddress = GetAddress( "glReadPixels" );			
			ShadeModelAddress = GetAddress( "glShadeModel" );
			
			TexCoord2fAddress = GetAddress( "glTexCoord2f" );
			TexCoordPointerAddress = GetAddress( "glTexCoordPointer" );
			TexImage2DAddress = GetAddress( "glTexImage2D" );
			TexParameteriAddress = GetAddress( "glTexParameteri" );
			TexSubImage2DAddress = GetAddress( "glTexSubImage2D" );
			Vertex3fAddress = GetAddress( "glVertex3f" );
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
