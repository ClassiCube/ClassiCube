#region --- License ---
/* Copyright (c) 2006-2008 the OpenTK team.
 * See license.txt for license info
 * 
 * Contributions by Andy Gill.
 */
#endregion

using System;
using System.Diagnostics;

namespace OpenTK.Graphics.OpenGL
{
	/// <summary>
	/// OpenGL bindings for .NET, implementing the full OpenGL API, including extensions.
	/// </summary>
	public sealed partial class GL : BindingsBase
	{
		internal const string Library = "opengl32.dll";
		static readonly object sync_root = new object();

		static GL() { }
		
		public GL() : base( typeof( Core ) ) {
		}
		
		GraphicsContextBase context;
		protected override IntPtr GetAddress( string funcname ) {
			return context.GetAddress( funcname );
		}
		
		internal void LoadEntryPoints( GraphicsContextBase context ) {
			this.context = context;
			Debug.Write("Loading OpenGL function pointers... ");

			LoadDelegate( "glAlphaFunc", out Delegates.glAlphaFunc );
			LoadDelegate( "glBindBuffer", out Delegates.glBindBuffer );
			LoadDelegate( "glBindBufferARB", out Delegates.glBindBufferARB );
			LoadDelegate( "glBindTexture", out Delegates.glBindTexture );
			LoadDelegate( "glBlendFunc", out Delegates.glBlendFunc );
			LoadDelegate( "glBufferData", out Delegates.glBufferData );
			LoadDelegate( "glBufferDataARB", out Delegates.glBufferDataARB );
			LoadDelegate( "glBufferSubData", out Delegates.glBufferSubData );
			LoadDelegate( "glBufferSubDataARB", out Delegates.glBufferSubDataARB );
			LoadDelegate( "glClear", out Delegates.glClear );
			LoadDelegate( "glClearColor", out Delegates.glClearColor );
			LoadDelegate( "glClearDepth", out Delegates.glClearDepth );
			LoadDelegate( "glColorMask", out Delegates.glColorMask );
			LoadDelegate( "glColorPointer", out Delegates.glColorPointer );
			LoadDelegate( "glCullFace", out Delegates.glCullFace );
			LoadDelegate( "glDeleteBuffers", out Delegates.glDeleteBuffers );
			LoadDelegate( "glDeleteBuffersARB", out Delegates.glDeleteBuffersARB );
			LoadDelegate( "glDeleteTextures", out Delegates.glDeleteTextures );
			LoadDelegate( "glDepthFunc", out Delegates.glDepthFunc );
			LoadDelegate( "glDepthMask", out Delegates.glDepthMask );
			//LoadDelegate( "glDepthRange", out Delegates.glDepthRange );
			LoadDelegate( "glDisable", out Delegates.glDisable );
			LoadDelegate( "glDisableClientState", out Delegates.glDisableClientState );
			LoadDelegate( "glDrawArrays", out Delegates.glDrawArrays );
			LoadDelegate( "glDrawElements", out Delegates.glDrawElements );
			LoadDelegate( "glEnable", out Delegates.glEnable );
			LoadDelegate( "glEnableClientState", out Delegates.glEnableClientState );
			LoadDelegate( "glFogf", out Delegates.glFogf );
			LoadDelegate( "glFogfv", out Delegates.glFogfv );
			LoadDelegate( "glFogi", out Delegates.glFogi );
			LoadDelegate( "glGenBuffers", out Delegates.glGenBuffers );
			LoadDelegate( "glGenBuffersARB", out Delegates.glGenBuffersARB );
			LoadDelegate( "glGenTextures", out Delegates.glGenTextures );
			//LoadDelegate( "glGetError", out Delegates.glGetError );
			//LoadDelegate( "glGetFloatv", out Delegates.glGetFloatv );
			LoadDelegate( "glGetIntegerv", out Delegates.glGetIntegerv );
			LoadDelegate( "glGetString", out Delegates.glGetString );
			LoadDelegate( "glHint", out Delegates.glHint );
			LoadDelegate( "glIsBuffer", out Delegates.glIsBuffer );
			LoadDelegate( "glIsBufferARB", out Delegates.glIsBufferARB );
			LoadDelegate( "glIsTexture", out Delegates.glIsTexture );
			LoadDelegate( "glLoadIdentity", out Delegates.glLoadIdentity );
			LoadDelegate( "glLoadMatrixf", out Delegates.glLoadMatrixf );
			LoadDelegate( "glMatrixMode", out Delegates.glMatrixMode );
			LoadDelegate( "glMultMatrixf", out Delegates.glMultMatrixf );
			LoadDelegate( "glPopMatrix", out Delegates.glPopMatrix );
			LoadDelegate( "glPushMatrix", out Delegates.glPushMatrix );
			LoadDelegate( "glReadPixels", out Delegates.glReadPixels );
			//LoadDelegate( "glShadeModel", out Delegates.glShadeModel );
			LoadDelegate( "glTexCoordPointer", out Delegates.glTexCoordPointer );
			LoadDelegate( "glTexImage2D", out Delegates.glTexImage2D );
			LoadDelegate( "glTexParameteri", out Delegates.glTexParameteri );
			LoadDelegate( "glTexSubImage2D", out Delegates.glTexSubImage2D );
			LoadDelegate( "glVertexPointer", out Delegates.glVertexPointer );
			LoadDelegate( "glViewport", out Delegates.glViewport );
		}
	}
}
