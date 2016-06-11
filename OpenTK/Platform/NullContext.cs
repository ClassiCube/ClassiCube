using System;
using OpenTK.Graphics;

namespace OpenTK.Platform {

	internal sealed class NullContext : GraphicsContextBase {
		
		public NullContext() {
			Debug.Print( "doing nothing" );
		}

		public override void SwapBuffers() {
		}

		public override void MakeCurrent( IWindowInfo window ) {
		}

		public override bool IsCurrent {
			get { return true; }
		}

		public override bool VSync {
			get { return true; }
			set { }
		}

		public override void LoadAll() {
		}

		public override IntPtr GetAddress(string funcName) {
			return IntPtr.Zero;
		}

		public override string ToString() {
			return "(empty)";
		}

		protected override void Dispose(bool calledManually) {
		}
	}
}
