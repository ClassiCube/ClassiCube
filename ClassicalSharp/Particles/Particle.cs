using System;
using OpenTK;

namespace ClassicalSharp.Particles {

	public abstract class Particle {
		
		public Vector3 Position;
		public Vector3 Velocity;
		public float Lifetime;
		protected Game game;
		protected Vector3 lastPos, nextPos;

		public abstract void Render( double delta, float t, VertexPos3fTex2fCol4b[] vertices, ref int index );
		
		public Particle( Game game ) { this.game = game; }
		
		public virtual bool Tick( double delta ) {
			Lifetime -= (float)delta;
			return Lifetime < 0;
		}
	}
}