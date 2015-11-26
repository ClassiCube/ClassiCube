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
		
		public Particle( Game game, Vector3 pos, Vector3 velocity, double lifetime ) {
			this.game = game;
			Position = lastPos = nextPos = pos;
			Velocity = velocity;
			Lifetime = (float)lifetime;
		}
		
		public virtual bool Tick( double delta ) {
			Lifetime -= (float)delta;
			return Lifetime < 0;
		}
	}
}