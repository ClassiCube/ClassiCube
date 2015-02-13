using System;
using OpenTK;

namespace ClassicalSharp {

	public sealed class FallingBlockEntity : ObjectEntity {		
		
		public override float StepSize {
			get { return 0; }
		}
		
		public override OpenTK.Vector3 Size {
			get { return Vector3.One; }
		}
		
		public FallingBlockEntity( ObjectEntityType type, int data, Game window ) : base( type, data, window ) {
		}
	}
}