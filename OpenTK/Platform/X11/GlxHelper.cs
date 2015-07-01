#region --- License ---
/* Copyright (c) 2006, 2007 Stefanos Apostolopoulos
 * See license.txt for license info
 */
#endregion

using System;

namespace OpenTK.Platform.X11
{
	partial class Glx : BindingsBase
	{
		const string Library = "libGL.so.1";
		static readonly object sync_root = new object();

		// Disable BeforeFieldInit optimization.
		static Glx() { }
		
		public Glx() : base( typeof( Delegates ), typeof( Imports ), 3 ) {
		}

		protected override object SyncRoot
		{
			get { return sync_root; }
		}

		protected override IntPtr GetAddress (string funcname)
		{
			return Glx.GetProcAddress(funcname);
		}
	}
}
