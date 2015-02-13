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
		
		public override void Render( double deltaTime, float t ) {
		}
		
		public override void Despawn() {
		}
		
		public override void SetLocation( LocationUpdate update, bool interpolate ) {
		}
		
		public override void Tick( double delta ) {
		}
	}
}